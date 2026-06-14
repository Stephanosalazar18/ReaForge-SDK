// Test http_server routing + JSON envelope. Boots a real httplib on 127.0.0.1.
#include "artifact_writer.h"
#include "http_server.h"

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
    fs::path base = fs::temp_directory_path() /
                    ("reaforge_http_" + tag + "_" + std::to_string(std::rand()));
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
    std::fprintf(stderr, "test_http_server: %s\n", rc_ == 0 ? "PASS" : "FAIL");
    return rc_;
}
