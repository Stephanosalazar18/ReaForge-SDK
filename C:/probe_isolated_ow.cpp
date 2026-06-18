// Test ONLY test_overwrite_replaces in isolation
#include <cstdio>
#include <filesystem>
#include "artifact_writer.h"

namespace fs = std::filesystem;
using namespace reaforge::host;

static int rc = 0;
static void check(bool cond, const char* what) {
    if (cond) std::fprintf(stderr, "OK: %s\n", what);
    else { std::fprintf(stderr, "FAIL: %s\n", what); rc = 1; }
}

int main() {
    fs::path tmp = fs::temp_directory_path() / "reaforge_isolated_ow";
    fs::remove_all(tmp);
    fs::create_directories(tmp);
    setenv_or_putenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);

    std::fprintf(stderr, "step 1: save_jsfx v1\n");
    auto a = save_jsfx("bar", "v1", false);
    check(a.ok, "first save ok");
    std::fprintf(stderr, "step 2: save_jsfx v2 overwrite\n");
    auto b = save_jsfx("bar", "v2_replaced", true);
    check(b.ok, "overwrite ok");
    std::fprintf(stderr, "step 3: ifstream\n");
    std::ifstream f(tmp / "Effects" / "ReaForge" / "bar.jsfx");
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    check(content == "v2_replaced", "content replaced with overwrite=true");
    std::fprintf(stderr, "step 4: remove_all\n");
    fs::remove_all(tmp);
    std::fprintf(stderr, "step 5: done\n");
    return rc;
}
