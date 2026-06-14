#include "http_server.h"
#include "artifact_writer.h"
#include "project_reader.h"
#include "refresh.h"

// httplib + json are vendored at third_party/.
#include "httplib.h"
#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace reaforge {
namespace host {

namespace {

// Hardcoded version for /v1/health. Bump at MVP release.
constexpr const char* kVersion = "0.1.0";

bool parse_bool_default_false(const nlohmann::json& j, const char* key) {
    if (!j.contains(key)) return false;
    if (!j[key].is_boolean()) return false;
    return j[key].get<bool>();
}

std::string extract_field(const nlohmann::json& j, const char* key) {
    if (!j.contains(key)) return "";
    if (!j[key].is_string()) return "";
    return j[key].get<std::string>();
}

// Parse a JSON body. On failure, fills *err_code and *err_msg and returns false.
bool parse_json_body(const std::string& body, nlohmann::json& out,
                     std::string& err_code, std::string& err_msg) {
    if (body.empty()) {
        err_code = "INVALID_JSON";
        err_msg = "empty body";
        return false;
    }
    try {
        out = nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        err_code = "INVALID_JSON";
        err_msg = e.what();
        return false;
    }
    if (!out.is_object()) {
        err_code = "INVALID_JSON";
        err_msg = "expected object";
        return false;
    }
    return true;
}

void respond_error(httplib::Response& res, int status,
                   const std::string& code, const std::string& message) {
    nlohmann::json out;
    out["error"] = code;
    out["message"] = message;
    res.status = status;
    res.set_content(out.dump(), "application/json");
}

void respond_write_result(httplib::Response& res, const WriteResult& r,
                          bool include_action_id_if_present) {
    if (r.ok) {
        nlohmann::json out;
        out["path"] = r.path;
        out["bytes_written"] = r.bytes_written;
        if (include_action_id_if_present && r.action_id.has_value()) {
            out["action_id"] = *r.action_id;
        }
        res.status = 200;
        res.set_content(out.dump(), "application/json");
        return;
    }
    int status = 500;
    if (r.error_code == "INVALID_NAME" || r.error_code == "INVALID_JSON") status = 400;
    else if (r.error_code == "FILE_EXISTS") status = 409;
    // REGISTER_FAILED is 500 per spec.
    nlohmann::json out;
    out["error"] = r.error_code;
    out["message"] = r.error_message;
    if (!r.path.empty()) out["path"] = r.path;
    res.status = status;
    res.set_content(out.dump(), "application/json");
}

}

std::string HttpServer::error_envelope(const std::string& code,
                                       const std::string& message,
                                       const std::string& extra_json) {
    nlohmann::json out;
    out["error"] = code;
    out["message"] = message;
    if (!extra_json.empty()) {
        try {
            auto extra = nlohmann::json::parse(extra_json);
            for (auto it = extra.begin(); it != extra.end(); ++it) {
                out[it.key()] = it.value();
            }
        } catch (...) {
            // ignore bad extra
        }
    }
    return out.dump();
}

HttpServer::HttpServer() = default;

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start(int port) {
    if (running_.load()) return true;
    if (port < 0) {
        const char* env = std::getenv("REAFORGE_PORT");
        if (env && *env) {
            port = std::atoi(env);
        }
    }
    if (port < 0) port = 7800;
    port_ = port;

    server_ = std::make_unique<httplib::Server>();
    server_->set_read_timeout(5, 0);
    server_->set_write_timeout(5, 0);
    server_->set_payload_max_length(16 * 1024 * 1024);  // 16 MB; jsfx payloads can be big
    register_routes();

    if (!server_->bind_to_port("0.0.0.0", port_)) {
        std::fprintf(stderr, "reaforge: http_server failed to bind 0.0.0.0:%d\n", port_);
        server_.reset();
        return false;
    }

    running_.store(true);
    thread_ = std::thread([this]() {
        server_->listen_after_bind();
    });
    std::fprintf(stderr, "reaforge: http_server listening on 0.0.0.0:%d\n", port_);
    return true;
}

int HttpServer::start_any() {
    if (running_.load()) return port_;

    server_ = std::make_unique<httplib::Server>();
    server_->set_read_timeout(5, 0);
    server_->set_write_timeout(5, 0);
    server_->set_payload_max_length(16 * 1024 * 1024);
    register_routes();

    int port = server_->bind_to_any_port("127.0.0.1");
    if (port <= 0) {
        std::fprintf(stderr, "reaforge: http_server failed to bind to any port on 127.0.0.1\n");
        server_.reset();
        return -1;
    }
    port_ = port;

    running_.store(true);
    thread_ = std::thread([this]() {
        server_->listen_after_bind();
    });
    std::fprintf(stderr, "reaforge: http_server listening on 127.0.0.1:%d (test mode)\n", port_);
    return port_;
}

void HttpServer::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (server_) {
        server_->stop();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    server_.reset();
    std::fprintf(stderr, "reaforge: http_server stopped\n");
}

void HttpServer::register_routes() {
    if (!server_) return;

    // GET /v1/health — always available, used by bridge keepalive.
    server_->Get("/v1/health", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json out;
        out["ok"] = true;
        out["version"] = kVersion;
        res.status = 200;
        res.set_content(out.dump(), "application/json");
    });

    // POST /v1/save/jsfx
    server_->Post("/v1/save/jsfx", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        std::string ec, em;
        if (!parse_json_body(req.body, body, ec, em)) {
            respond_error(res, 400, ec, em);
            return;
        }
        std::string name = extract_field(body, "name");
        std::string code = extract_field(body, "code");
        bool overwrite = parse_bool_default_false(body, "overwrite");
        WriteResult r = save_jsfx(name, code, overwrite);
        respond_write_result(res, r, /*include_action_id_if_present=*/false);
    });

    // POST /v1/save/lua
    server_->Post("/v1/save/lua", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        std::string ec, em;
        if (!parse_json_body(req.body, body, ec, em)) {
            respond_error(res, 400, ec, em);
            return;
        }
        std::string name = extract_field(body, "name");
        std::string code = extract_field(body, "code");
        bool register_action = parse_bool_default_false(body, "register_action");
        bool overwrite = parse_bool_default_false(body, "overwrite");
        WriteResult r = save_lua(name, code, register_action, overwrite);
        respond_write_result(res, r, /*include_action_id_if_present=*/true);
    });

    // POST /v1/save/fx-chain
    server_->Post("/v1/save/fx-chain", [](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body;
        std::string ec, em;
        if (!parse_json_body(req.body, body, ec, em)) {
            respond_error(res, 400, ec, em);
            return;
        }
        std::string name = extract_field(body, "name");
        std::string content = extract_field(body, "content");
        bool overwrite = parse_bool_default_false(body, "overwrite");
        WriteResult r = save_fx_chain(name, content, overwrite);
        respond_write_result(res, r, /*include_action_id_if_present=*/false);
    });

    // POST /v1/refresh — PR 6 implementation. Calls refresh::refresh_now()
    // which marshals to REAPER's main thread (via the http_server worker
    // thread blocking on a future) and returns {refreshed_at, warnings[]}.
    // The JSFX manual-rescan warning is always present per the design.
    server_->Post("/v1/refresh", [](const httplib::Request&, httplib::Response& res) {
        RefreshResult r = refresh_now();
        nlohmann::json out;
        out["refreshed_at"] = r.refreshed_at;
        nlohmann::json warns = nlohmann::json::array();
        for (const auto& w : r.warnings) warns.push_back(w);
        out["warnings"] = std::move(warns);
        res.status = 200;
        res.set_content(out.dump(), "application/json");
    });

    // The 3 Read endpoints (state, artifacts, api-reference) are stubbed for PR 4.
    // PR 5 wires artifacts and api-reference; get_state wiring lands in this PR too
    // (in the get_state commit) since the REAPER-bound route is inseparable from
    // the REAPER function pointer plumbing.

    server_->Get("/v1/state", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json out;
        out["error"] = "NOT_IMPLEMENTED";
        out["message"] = "see extension-read-api; PR 5 lands the body";
        res.status = 501;
        res.set_content(out.dump(), "application/json");
    });

    // GET /v1/artifacts?kind=<jsfx|lua|fx_chain> — enumerate ReaForge/ subfolders.
    server_->Get("/v1/artifacts", [](const httplib::Request& req, httplib::Response& res) {
        std::string kind;
        if (req.has_param("kind")) kind = req.get_param_value("kind");
        // Base dir = REAFORGE_FIXTURE_DIR for tests; production swaps for
        // reaper.GetResourcePath() in a follow-up (artifact_writer has the same
        // pattern, see resource_base_dir()).
        std::string base = resource_base_dir();
        auto r = list_artifacts(base, kind);
        if (r.invalid_kind) {
            nlohmann::json out;
            out["error"] = "INVALID_KIND";
            out["message"] = "kind must be one of: jsfx, lua, fx_chain";
            out["kind"] = r.invalid_kind_value;
            res.status = 400;
            res.set_content(out.dump(), "application/json");
            return;
        }
        nlohmann::json out;
        out["artifacts"] = nlohmann::json::array();
        for (const auto& e : r.entries) {
            nlohmann::json je;
            je["path"] = e.path;
            je["size"] = e.size;
            je["mtime"] = e.mtime;
            je["kind"] = e.kind;
            out["artifacts"].push_back(std::move(je));
        }
        res.status = 200;
        res.set_content(out.dump(), "application/json");
    });

    // GET /v1/api-reference?target=<jsfx|reascript_lua|fx_chain_format>
    server_->Get("/v1/api-reference", [](const httplib::Request& req, httplib::Response& res) {
        std::string target;
        if (req.has_param("target")) target = req.get_param_value("target");
        auto r = get_api_reference(target);
        if (!r.ok) {
            int status = (r.error_code == "MISSING_TARGET") ? 400 : 400;
            nlohmann::json out;
            out["error"] = r.error_code;
            out["message"] = r.error_message;
            if (!target.empty()) out["target"] = target;
            res.status = status;
            res.set_content(out.dump(), "application/json");
            return;
        }
        nlohmann::json out;
        out["target"] = r.target;
        out["reference"] = r.reference;
        res.status = 200;
        res.set_content(out.dump(), "application/json");
    });

    // 404 fallback for unknown routes.
    server_->set_error_handler([](const httplib::Request& req, httplib::Response& res) {
        if (res.status >= 400 && res.body.empty()) {
            nlohmann::json out;
            out["error"] = "NOT_FOUND";
            out["message"] = "no route for " + req.method + " " + req.path;
            res.set_content(out.dump(), "application/json");
        }
    });
    server_->set_exception_handler([](const httplib::Request&, httplib::Response& res,
                                      std::exception_ptr ep) {
        std::string msg = "unknown";
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            msg = e.what();
        } catch (...) {
            msg = "non-std exception";
        }
        nlohmann::json out;
        out["error"] = "INTERNAL";
        out["message"] = msg;
        res.status = 500;
        res.set_content(out.dump(), "application/json");
    });
}

}
}
