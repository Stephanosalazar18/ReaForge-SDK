// Test http_server routing + JSON envelope. Boots a real httplib on 127.0.0.1.
#include "artifact_writer.h"
#include "http_server.h"
#include "project_reader.h"

#include "httplib.h"
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace reaforge::host;
using namespace std::chrono_literals;

static int rc_ = 0;

static void check(bool cond, const char* what) {
    if (cond) {
        std::fprintf(stderr, "OK: %s\n", what);
    } else {
        std::fprintf(stderr, "FAIL: %s\n", what);
        rc_ = 1;
    }
}

static fs::path make_fixture(const std::string& tag) {
    // Use a monotonic counter so each call gets a unique directory even when
    // std::rand() returns the same value across calls in the same process.
    static std::atomic<int> seq{0};
    int n = seq.fetch_add(1);
    fs::path base = fs::temp_directory_path() /
                    ("reaforge_http_" + tag + "_" + std::to_string(n));
    fs::remove_all(base);
    fs::create_directories(base);
    return base;
}

// Start a server on a free port and return the bound port. Uses HttpServer::start_any()
// which internally calls bind_to_any_port on the same Server instance (no probe race).
static int start_and_get_port(HttpServer& s) {
    int port = s.start_any();
    if (port <= 0) {
        std::fprintf(stderr, "FAIL: could not start server\n");
        rc_ = 1;
        return -1;
    }
    return port;
}

static nlohmann::json parse(const std::string& s) {
    return nlohmann::json::parse(s);
}

static void test_health() {
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Get("/v1/health");
    check(res != nullptr, "GET /v1/health returns");
    check(res && res->status == 200, "GET /v1/health → 200");
    bool ok = false;
    if (res) {
        auto j = parse(res->body);
        ok = j.contains("ok") && j["ok"] == true && j.contains("version");
    }
    check(ok, "body has {ok:true, version:...}");
    s.stop();
}

static void test_unknown_route_404() {
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Get("/v1/does-not-exist");
    check(res != nullptr, "unknown route returns");
    check(res && res->status == 404, "unknown route → 404");
    s.stop();
}

static void test_save_jsfx_endpoint() {
    fs::path tmp = make_fixture("e_jsfx");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);

    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/save/jsfx", R"({"name":"srv_jsfx","code":"desc:srv\n"})",
                      "application/json");
    check(res && res->status == 200, "POST /v1/save/jsfx → 200");
    if (res && res->status == 200) {
        auto j = parse(res->body);
        check(j["path"].get<std::string>().find("Effects/ReaForge/srv_jsfx.jsfx") != std::string::npos,
              "response path contains Effects/ReaForge/srv_jsfx.jsfx");
        check(j["bytes_written"].get<int>() > 0, "response has bytes_written > 0");
    }
    check(fs::exists(tmp / "Effects" / "ReaForge" / "srv_jsfx.jsfx"), "jsfx on disk");
    s.stop();
    fs::remove_all(tmp);
}

static void test_save_lua_endpoint_no_register() {
    fs::path tmp = make_fixture("e_lua");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/save/lua", R"({"name":"srv_lua","code":"-- x\n"})",
                      "application/json");
    check(res && res->status == 200, "POST /v1/save/lua → 200");
    if (res && res->status == 200) {
        auto j = parse(res->body);
        check(!j.contains("action_id"), "no action_id when register_action absent");
    }
    check(fs::exists(tmp / "Scripts" / "ReaForge" / "srv_lua.lua"), "lua on disk");
    s.stop();
    fs::remove_all(tmp);
}

static void test_save_lua_endpoint_with_register() {
    fs::path tmp = make_fixture("e_luareg");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    set_add_remove_reascript([](bool, int, const char*, bool) -> int { return 7777; });
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/save/lua",
                      R"({"name":"srv_luareg","code":"-- y\n","register_action":true})",
                      "application/json");
    check(res && res->status == 200, "POST /v1/save/lua with register → 200");
    if (res && res->status == 200) {
        auto j = parse(res->body);
        check(j.contains("action_id") && j["action_id"] == 7777, "action_id present");
    }
    reset_add_remove_reascript();
    s.stop();
    fs::remove_all(tmp);
}

static void test_save_fx_chain_endpoint() {
    fs::path tmp = make_fixture("e_fxc");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    std::string body = R"({"name":"srv_fxc","content":"<FXCHAIN></FXCHAIN>"})";
    auto res = c.Post("/v1/save/fx-chain", body, "application/json");
    check(res && res->status == 200, "POST /v1/save/fx-chain → 200");
    check(fs::exists(tmp / "FXChains" / "ReaForge" / "srv_fxc.RfxChain"),
          "fx-chain on disk");
    s.stop();
    fs::remove_all(tmp);
}

static void test_dup_without_overwrite_409() {
    fs::path tmp = make_fixture("e_dup");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    c.Post("/v1/save/jsfx", R"({"name":"dup","code":"v1"})", "application/json");
    auto res = c.Post("/v1/save/jsfx", R"({"name":"dup","code":"v2"})", "application/json");
    check(res && res->status == 409, "duplicate without overwrite → 409");
    if (res) {
        auto j = parse(res->body);
        check(j.contains("error") && j["error"] == "FILE_EXISTS", "error code FILE_EXISTS");
    }
    s.stop();
    fs::remove_all(tmp);
}

static void test_bad_name_400() {
    fs::path tmp = make_fixture("e_bname");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/save/jsfx", R"({"name":"../escape","code":"x"})",
                      "application/json");
    check(res && res->status == 400, "bad name → 400");
    if (res) {
        auto j = parse(res->body);
        check(j["error"] == "INVALID_NAME", "error code INVALID_NAME");
    }
    s.stop();
    fs::remove_all(tmp);
}

static void test_bad_json_400() {
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/save/jsfx", "not json at all", "application/json");
    check(res && res->status == 400, "bad JSON → 400");
    if (res) {
        auto j = parse(res->body);
        check(j["error"] == "INVALID_JSON", "error code INVALID_JSON");
    }
    s.stop();
}

static void test_refresh_stub_501() {
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    auto res = c.Post("/v1/refresh", "{}", "application/json");
    check(res && res->status == 501, "POST /v1/refresh → 501 in PR 4");
    s.stop();
}

// PR 5: GET /v1/artifacts — missing folders tolerated, kind filter works.
static void test_artifacts_endpoint() {
    fs::path tmp = make_fixture("e_art");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    // Empty fixture: all 3 folders missing → empty list, no error.
    {
        HttpServer s;
        int port = start_and_get_port(s);
        if (port <= 0) return;
        std::this_thread::sleep_for(50ms);
        httplib::Client c("127.0.0.1", port);
        c.set_connection_timeout(1, 0);
        auto res = c.Get("/v1/artifacts");
        check(res && res->status == 200, "GET /v1/artifacts empty → 200");
        if (res && res->status == 200) {
            auto j = parse(res->body);
            check(j.contains("artifacts") && j["artifacts"].is_array() && j["artifacts"].empty(),
                  "empty artifacts list");
        }
        s.stop();
    }
    // Populate one folder, query with kind filter.
    {
        fs::create_directories(tmp / "Effects" / "ReaForge");
        fs::create_directories(tmp / "Scripts" / "ReaForge");
        fs::create_directories(tmp / "FXChains" / "ReaForge");
        std::ofstream a(tmp / "Effects" / "ReaForge" / "tap.jsfx", std::ios::binary);
        a << "desc:tap\n";
        a.close();
        std::ofstream b(tmp / "Effects" / "ReaForge" / "clip.jsfx", std::ios::binary);
        b << "desc:clip\n";
        b.close();
        std::ofstream c_lua(tmp / "Scripts" / "ReaForge" / "doubler.lua", std::ios::binary);
        c_lua << "-- y\n";
        c_lua.close();
    }
    {
        HttpServer s;
        int port = start_and_get_port(s);
        if (port <= 0) return;
        std::this_thread::sleep_for(50ms);
        httplib::Client c("127.0.0.1", port);
        c.set_connection_timeout(1, 0);
        // kind=jsfx → 2 entries
        auto res = c.Get("/v1/artifacts?kind=jsfx");
        check(res && res->status == 200, "GET /v1/artifacts?kind=jsfx → 200");
        if (res && res->status == 200) {
            auto j = parse(res->body);
            check(j["artifacts"].size() == 2, "kind=jsfx returns 2");
            bool all_jsfx = std::all_of(j["artifacts"].begin(), j["artifacts"].end(),
                [](const nlohmann::json& e) { return e["kind"] == "jsfx"; });
            check(all_jsfx, "all entries kind=jsfx");
        }
        // kind=lua → 1 entry
        auto res2 = c.Get("/v1/artifacts?kind=lua");
        check(res2 && res2->status == 200, "GET /v1/artifacts?kind=lua → 200");
        if (res2 && res2->status == 200) {
            auto j = parse(res2->body);
            check(j["artifacts"].size() == 1, "kind=lua returns 1");
        }
        // no kind → 3 entries (2 jsfx + 1 lua; FXChains missing → tolerated)
        auto res3 = c.Get("/v1/artifacts");
        check(res3 && res3->status == 200, "GET /v1/artifacts → 200");
        if (res3 && res3->status == 200) {
            auto j = parse(res3->body);
            check(j["artifacts"].size() == 3, "merged list returns 3");
        }
        // invalid kind → 400
        auto res4 = c.Get("/v1/artifacts?kind=python");
        check(res4 && res4->status == 400, "kind=python → 400");
        if (res4 && res4->status == 400) {
            auto j = parse(res4->body);
            check(j["error"] == "INVALID_KIND", "error code INVALID_KIND");
        }
        s.stop();
    }
    fs::remove_all(tmp);
}

// PR 5: GET /v1/api-reference — embedded payload, valid + invalid targets.
static void test_api_reference_endpoint() {
    HttpServer s;
    int port = start_and_get_port(s);
    if (port <= 0) return;
    std::this_thread::sleep_for(50ms);
    httplib::Client c("127.0.0.1", port);
    c.set_connection_timeout(1, 0);
    for (const std::string& target : {"jsfx", "reascript_lua", "fx_chain_format"}) {
        std::string path = std::string("/v1/api-reference?target=") + target;
        auto res = c.Get(path);
        check(res && res->status == 200, ("GET /v1/api-reference?target=" + target + " → 200").c_str());
        if (res && res->status == 200) {
            auto j = parse(res->body);
            check(j["target"] == target, "target echoed");
            check(j["reference"].is_string() && !j["reference"].get<std::string>().empty(),
                  "reference is non-empty string");
        }
    }
    // missing target → 400 MISSING_TARGET
    {
        auto res = c.Get("/v1/api-reference");
        check(res && res->status == 400, "missing target → 400");
        if (res && res->status == 400) {
            auto j = parse(res->body);
            check(j["error"] == "MISSING_TARGET", "error code MISSING_TARGET");
        }
    }
    // invalid target → 400 INVALID_TARGET
    {
        auto res = c.Get("/v1/api-reference?target=python");
        check(res && res->status == 400, "invalid target → 400");
        if (res && res->status == 400) {
            auto j = parse(res->body);
            check(j["error"] == "INVALID_TARGET", "error code INVALID_TARGET");
            check(j.contains("target") && j["target"] == "python", "target echoed in error");
        }
    }
    s.stop();
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::fprintf(stderr, "test_http_server: starting\n");
    test_health();
    test_unknown_route_404();
    test_save_jsfx_endpoint();
    test_save_lua_endpoint_no_register();
    test_save_lua_endpoint_with_register();
    test_save_fx_chain_endpoint();
    test_dup_without_overwrite_409();
    test_bad_name_400();
    test_bad_json_400();
    test_refresh_stub_501();
    test_artifacts_endpoint();
    test_api_reference_endpoint();
    std::fprintf(stderr, "test_http_server: %s\n", rc_ == 0 ? "PASS" : "FAIL");
    return rc_;
}
