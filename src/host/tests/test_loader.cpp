#include "extension_loader.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path make_temp_dir(const std::string& tag) {
    fs::path base = fs::temp_directory_path() /
                    ("reaforge_test_" + tag + "_" + std::to_string(std::rand()));
    fs::create_directories(base);
    return base;
}

void write_manifest(const fs::path& dir, const std::string& name,
                    const std::string& id, const std::string& target) {
    fs::create_directories(dir / name);
    std::ofstream f(dir / name / "manifest.lua");
    f << "return { id = \"" << id << "\", name = \"" << name
      << "\", runtime = \"lua\", target = \"" << target << "\", entry = \"run\" }\n";
}

int test_scan_three_extensions() {
    fs::path tmp = make_temp_dir("scan");
    write_manifest(tmp, "alpha", "alpha", "track");
    write_manifest(tmp, "beta",  "beta",  "item");
    write_manifest(tmp, "gamma", "gamma", "master");

    using namespace reaforge::host::loader;
    g_extensions_dir_override = tmp.string();
    auto manifests = scan();

    int rc = 0;
    if (manifests.size() != 3) {
        std::fprintf(stderr, "FAIL: expected 3 manifests, got %zu\n", manifests.size());
        rc = 1;
    } else {
        std::fprintf(stderr, "OK: scan found 3 manifests\n");
    }
    fs::remove_all(tmp);
    return rc;
}

int test_reload_finds_edited_manifest() {
    fs::path tmp = make_temp_dir("reload");
    write_manifest(tmp, "delta", "delta", "track");

    using namespace reaforge::host::loader;
    g_extensions_dir_override = tmp.string();

    int rc = 0;
    if (!reload("delta")) {
        std::fprintf(stderr, "FAIL: reload(delta) returned false\n");
        rc = 1;
    } else {
        std::fprintf(stderr, "OK: reload(delta) returned true\n");
    }
    fs::remove_all(tmp);
    return rc;
}

int test_reload_unknown_id_returns_false() {
    fs::path tmp = make_temp_dir("reload_miss");
    write_manifest(tmp, "epsilon", "epsilon", "item");

    using namespace reaforge::host::loader;
    g_extensions_dir_override = tmp.string();

    int rc = 0;
    if (reload("nonexistent")) {
        std::fprintf(stderr, "FAIL: reload(nonexistent) returned true\n");
        rc = 1;
    } else {
        std::fprintf(stderr, "OK: reload(nonexistent) returned false\n");
    }
    fs::remove_all(tmp);
    return rc;
}

}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    int rc = 0;
    rc |= test_scan_three_extensions();
    rc |= test_reload_finds_edited_manifest();
    rc |= test_reload_unknown_id_returns_false();
    return rc;
}
