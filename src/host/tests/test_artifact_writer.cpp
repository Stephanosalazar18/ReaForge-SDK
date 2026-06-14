// Test artifact_writer against a fixture dir. No REAPER dep.
#include "artifact_writer.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace reaforge::host;

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
                    ("reaforge_aw_" + tag + "_" + std::to_string(std::rand()));
    fs::remove_all(base);
    fs::create_directories(base);
    return base;
}

static void test_name_regex() {
    check(is_valid_name("ok_name-1"), "valid alphanumeric+_- name");
    check(!is_valid_name(""), "empty name rejected");
    check(!is_valid_name("../escape"), "slash in name rejected");
    check(!is_valid_name("with space"), "space in name rejected");
    check(!is_valid_name(".hidden"), "leading dot rejected");
    check(!is_valid_name(std::string(65, 'a')), "name >64 chars rejected");
    check(is_valid_name(std::string(64, 'a')), "name 64 chars accepted");
}

static void test_save_jsfx_creates_file() {
    fs::path tmp = make_fixture("jsfx_ok");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);

    auto r = save_jsfx("tape_sat", "desc:tape\n", false);
    check(r.ok, "save_jsfx ok");
    check(r.path.find("Effects/ReaForge/tape_sat.jsfx") != std::string::npos,
          "path contains Effects/ReaForge/<name>.jsfx");
    check(r.path.find('\\') == std::string::npos, "path uses forward slashes");
    check(r.bytes_written == std::string("desc:tape\n").size(), "bytes_written matches content size");
    check(fs::exists(tmp / "Effects" / "ReaForge" / "tape_sat.jsfx"), "file exists on disk");
    fs::remove_all(tmp);
}

static void test_save_jsfx_refuses_overwrite() {
    fs::path tmp = make_fixture("jsfx_dup");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);

    auto first = save_jsfx("foo", "v1", false);
    check(first.ok, "first save_jsfx ok");
    auto second = save_jsfx("foo", "v2", false);
    check(!second.ok, "second save_jsfx without overwrite refused");
    check(second.error_code == "FILE_EXISTS", "error_code is FILE_EXISTS");

    std::ifstream f(tmp / "Effects" / "ReaForge" / "foo.jsfx");
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    check(content == "v1", "original content preserved");
    fs::remove_all(tmp);
}

static void test_overwrite_replaces() {
    fs::path tmp = make_fixture("jsfx_ow");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    auto a = save_jsfx("bar", "v1", false);
    check(a.ok, "first save ok");
    auto b = save_jsfx("bar", "v2_replaced", true);
    check(b.ok, "overwrite ok");
    std::ifstream f(tmp / "Effects" / "ReaForge" / "bar.jsfx");
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    check(content == "v2_replaced", "content replaced with overwrite=true");
    fs::remove_all(tmp);
}

static void test_save_lua_no_register() {
    fs::path tmp = make_fixture("lua_basic");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    auto r = save_lua("hello", "-- hi\n", false, false);
    check(r.ok, "save_lua ok without register");
    check(!r.action_id.has_value(), "action_id absent when register_action=false");
    check(fs::exists(tmp / "Scripts" / "ReaForge" / "hello.lua"), "lua file on disk");
    fs::remove_all(tmp);
}

static void test_save_lua_with_register_mock() {
    fs::path tmp = make_fixture("lua_reg");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    int unreg_calls = 0;
    int reg_calls = 0;
    std::atomic<int> captured_id{0};
    set_add_remove_reascript(
        [&unreg_calls, &reg_calls, &captured_id](bool add, int /*sectionID*/,
                                                const char* /*path*/, bool /*commit*/) -> int {
            if (add) {
                reg_calls++;
                captured_id = 1234;
                return 1234;
            }
            unreg_calls++;
            return 0;
        });
    auto r = save_lua("reg_me", "-- x\n", true, false);
    check(r.ok, "save_lua with register ok");
    check(r.action_id.has_value() && *r.action_id == 1234, "action_id captured from mock");
    check(unreg_calls == 1, "unregister called once before register");
    check(reg_calls == 1, "register called once");
    reset_add_remove_reascript();
    fs::remove_all(tmp);
}

static void test_register_failed_does_not_lose_file() {
    fs::path tmp = make_fixture("lua_fail");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    set_add_remove_reascript(
        [](bool, int, const char*, bool) -> int { return -1; });  // always fail
    (void)0;
    auto r = save_lua("will_stay", "-- x\n", true, false);
    check(!r.ok, "save_lua with failing register returns !ok");
    check(r.error_code == "REGISTER_FAILED", "error_code is REGISTER_FAILED");
    check(fs::exists(tmp / "Scripts" / "ReaForge" / "will_stay.lua"),
          "file remains on disk even when register fails (best-effort)");
    reset_add_remove_reascript();
    fs::remove_all(tmp);
}

static void test_save_fx_chain() {
    fs::path tmp = make_fixture("fxchain_ok");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    std::string content = "<FXCHAIN>\n<FX ID=\"VST:ReaEQ\">\n</FX>\n</FXCHAIN>\n";
    auto r = save_fx_chain("vocal_slap", content, false);
    check(r.ok, "save_fx_chain ok");
    check(r.path.find("FXChains/ReaForge/vocal_slap.RfxChain") != std::string::npos,
          "path contains FXChains/ReaForge/<name>.RfxChain");
    check(fs::exists(tmp / "FXChains" / "ReaForge" / "vocal_slap.RfxChain"),
          "fxchain file on disk");
    fs::remove_all(tmp);
}

static void test_atomic_write_leaves_no_tmp() {
    fs::path tmp = make_fixture("atomic");
    ::setenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);
    auto r = save_jsfx("clean", "X", false);
    check(r.ok, "save ok");
    // After successful write, no .tmp should remain.
    bool any_tmp = false;
    for (auto& e : fs::directory_iterator(tmp / "Effects" / "ReaForge")) {
        if (e.path().extension() == ".tmp") { any_tmp = true; break; }
    }
    check(!any_tmp, "no .tmp files left after successful write");
    fs::remove_all(tmp);
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::fprintf(stderr, "test_artifact_writer: starting\n");
    test_name_regex();
    test_save_jsfx_creates_file();
    test_save_jsfx_refuses_overwrite();
    test_overwrite_replaces();
    test_save_lua_no_register();
    test_save_lua_with_register_mock();
    test_register_failed_does_not_lose_file();
    test_save_fx_chain();
    test_atomic_write_leaves_no_tmp();
    std::fprintf(stderr, "test_artifact_writer: %s\n", rc_ == 0 ? "PASS" : "FAIL");
    return rc_;
}
