// Test project_reader against a fixture dir. No REAPER dep.
#include "project_reader.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace reaforge::host;
using json = nlohmann::json;

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
                    ("reaforge_pr_" + tag + "_" + std::to_string(std::rand()));
    fs::remove_all(base);
    fs::create_directories(base);
    return base;
}

static void touch(const fs::path& p, const std::string& contents) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << contents;
}

static void test_list_artifacts_missing_folder_returns_empty() {
    fs::path tmp = make_fixture("missing");
    auto r = list_artifacts(tmp.string(), "");
    check(r.entries.empty(), "missing folder → empty list");
    check(!r.invalid_kind, "no invalid_kind when kind empty");
    fs::remove_all(tmp);
}

static void test_list_artifacts_finds_jsfx() {
    fs::path tmp = make_fixture("jsfx");
    touch(tmp / "Effects" / "ReaForge" / "tape_sat.jsfx", "desc:tape\n");
    touch(tmp / "Effects" / "ReaForge" / "soft_clip.jsfx", "desc:clip\n");
    // noise: a file in the wrong folder should NOT show up
    touch(tmp / "Effects" / "ReaForge" / "not_a_jsfx.txt", "txt");
    touch(tmp / "Scripts" / "ReaForge" / "double_vel.lua", "-- y\n");

    auto r = list_artifacts(tmp.string(), "jsfx");
    check(r.entries.size() == 2, "two jsfx in Effects/ReaForge/");
    bool all_kind = std::all_of(r.entries.begin(), r.entries.end(),
                                [](const ArtifactEntry& e) { return e.kind == "jsfx"; });
    check(all_kind, "all entries have kind=jsfx");
    bool has_path = std::any_of(r.entries.begin(), r.entries.end(),
                                [](const ArtifactEntry& e) {
                                    return e.path.find("Effects/ReaForge/tape_sat.jsfx") != std::string::npos;
                                });
    check(has_path, "tape_sat.jsfx path present");
    bool no_backslash = std::all_of(r.entries.begin(), r.entries.end(),
                                    [](const ArtifactEntry& e) {
                                        return e.path.find('\\') == std::string::npos;
                                    });
    check(no_backslash, "paths use forward slashes");
    fs::remove_all(tmp);
}

static void test_list_artifacts_kind_filter_lua() {
    fs::path tmp = make_fixture("lua");
    touch(tmp / "Effects" / "ReaForge" / "a.jsfx", "x");
    touch(tmp / "Scripts" / "ReaForge" / "a.lua", "-- a");
    touch(tmp / "Scripts" / "ReaForge" / "b.lua", "-- b");
    auto r = list_artifacts(tmp.string(), "lua");
    check(r.entries.size() == 2, "two lua scripts");
    check(std::all_of(r.entries.begin(), r.entries.end(),
                      [](const ArtifactEntry& e) { return e.kind == "lua"; }),
          "all entries kind=lua");
    fs::remove_all(tmp);
}

static void test_list_artifacts_kind_filter_fx_chain() {
    fs::path tmp = make_fixture("fxc");
    touch(tmp / "FXChains" / "ReaForge" / "vocal_slap.RfxChain", "<FXCHAIN/>");
    auto r = list_artifacts(tmp.string(), "fx_chain");
    check(r.entries.size() == 1, "one fx_chain");
    check(r.entries[0].kind == "fx_chain", "kind=fx_chain");
    check(r.entries[0].path.find("FXChains/ReaForge/vocal_slap.RfxChain") != std::string::npos,
          "path includes FXChains/ReaForge/vocal_slap.RfxChain");
    fs::remove_all(tmp);
}

static void test_list_artifacts_invalid_kind() {
    fs::path tmp = make_fixture("invkind");
    auto r = list_artifacts(tmp.string(), "python");
    check(r.entries.empty(), "no entries on invalid kind");
    check(r.invalid_kind, "invalid_kind flag set");
    check(r.invalid_kind_value == "python", "invalid_kind_value echoes input");
    fs::remove_all(tmp);
}

static void test_list_artifacts_merged_no_kind() {
    fs::path tmp = make_fixture("merged");
    touch(tmp / "Effects" / "ReaForge" / "x.jsfx", "a");
    touch(tmp / "Scripts" / "ReaForge" / "y.lua", "b");
    // FXChains/ReaForge/ missing — should yield 2 entries, no error
    auto r = list_artifacts(tmp.string(), "");
    check(r.entries.size() == 2, "merged list has 2 (missing FXChains tolerated)");
    fs::remove_all(tmp);
}

static void test_get_api_reference_jsfx() {
    auto r = get_api_reference("jsfx");
    check(r.ok, "jsfx ok");
    check(r.target == "jsfx", "target=jsfx");
    check(!r.reference.empty(), "reference is non-empty");
    check(r.reference.find("JSFX") != std::string::npos ||
          r.reference.find("jsfx") != std::string::npos,
          "reference mentions jsfx");
    check(r.error_code.empty(), "no error code on success");
}

static void test_get_api_reference_reascript_lua() {
    auto r = get_api_reference("reascript_lua");
    check(r.ok, "reascript_lua ok");
    check(r.target == "reascript_lua", "target=reascript_lua");
    check(r.reference.find("Undo") != std::string::npos ||
          r.reference.find("reaper") != std::string::npos,
          "reference mentions reaper/undo");
}

static void test_get_api_reference_fx_chain_format() {
    auto r = get_api_reference("fx_chain_format");
    check(r.ok, "fx_chain_format ok");
    check(r.reference.find("FXCHAIN") != std::string::npos ||
          r.reference.find("RfxChain") != std::string::npos,
          "reference mentions FXCHAIN/RfxChain");
}

static void test_get_api_reference_missing_target() {
    auto r = get_api_reference("");
    check(!r.ok, "empty target not ok");
    check(r.error_code == "MISSING_TARGET", "error_code=MISSING_TARGET");
}

static void test_get_api_reference_invalid_target() {
    auto r = get_api_reference("python");
    check(!r.ok, "unknown target not ok");
    check(r.error_code == "INVALID_TARGET", "error_code=INVALID_TARGET");
    check(r.error_message.find("python") != std::string::npos,
          "error_message echoes target");
}

static void test_get_state_no_reaper_returns_error() {
    reset_reaper_function_pointers();
    auto r = get_state(false);
    check(!r.ok, "no REAPER → not ok");
    check(r.error_code == "REAPER_NOT_AVAILABLE", "error_code=REAPER_NOT_AVAILABLE");
    check(r.json.empty(), "json is empty on error");
}

static void test_get_state_no_reaper_summary_true() {
    reset_reaper_function_pointers();
    auto r = get_state(true);
    check(!r.ok, "no REAPER + summary=true → not ok");
    check(r.error_code == "REAPER_NOT_AVAILABLE", "error_code=REAPER_NOT_AVAILABLE");
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    std::fprintf(stderr, "test_project_reader: starting\n");

    test_list_artifacts_missing_folder_returns_empty();
    test_list_artifacts_finds_jsfx();
    test_list_artifacts_kind_filter_lua();
    test_list_artifacts_kind_filter_fx_chain();
    test_list_artifacts_invalid_kind();
    test_list_artifacts_merged_no_kind();
    test_get_api_reference_jsfx();
    test_get_api_reference_reascript_lua();
    test_get_api_reference_fx_chain_format();
    test_get_api_reference_missing_target();
    test_get_api_reference_invalid_target();
    test_get_state_no_reaper_returns_error();
    test_get_state_no_reaper_summary_true();

    std::fprintf(stderr, "test_project_reader: %s\n", rc_ == 0 ? "PASS" : "FAIL");
    return rc_;
}
