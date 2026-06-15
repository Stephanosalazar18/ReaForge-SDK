// Test if setenv_or_putenv with Windows path causes issues
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "artifact_writer.h"

int main() {
    using namespace reaforge::host;
    namespace fs = std::filesystem;

    fs::path tmp = fs::temp_directory_path() / "reaforge_setenv_probe";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    fprintf(stderr, "tmp = %s\n", tmp.string().c_str());
    fprintf(stderr, "tmp has %zu chars\n", tmp.string().size());

    setenv_or_putenv("REAFORGE_FIXTURE_DIR", tmp.string().c_str(), 1);

    const char* env = std::getenv("REAFORGE_FIXTURE_DIR");
    fprintf(stderr, "getenv returned: %s\n", env ? env : "(null)");

    fprintf(stderr, "calling save_jsfx...\n");
    auto a = save_jsfx("probe", "v1", false);
    fprintf(stderr, "save_jsfx returned: ok=%d path=%s\n", (int)a.ok, a.path.c_str());
    if (!a.ok) {
        fprintf(stderr, "  err=%s msg=%s\n", a.error_code.c_str(), a.error_message.c_str());
    }

    fprintf(stderr, "calling save_jsfx overwrite=true...\n");
    auto b = save_jsfx("probe", "v2", true);
    fprintf(stderr, "save_jsfx overwrite returned: ok=%d path=%s\n", (int)b.ok, b.path.c_str());

    fs::path expected = tmp / "Effects" / "ReaForge" / "probe.jsfx";
    fprintf(stderr, "checking file exists: %s -> %s\n", expected.string().c_str(),
            fs::exists(expected) ? "YES" : "NO");

    fprintf(stderr, "opening ifstream...\n");
    std::ifstream f(expected);
    if (!f) {
        fprintf(stderr, "ifstream failed to open\n");
    } else {
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        fprintf(stderr, "content = '%s'\n", content.c_str());
    }

    fs::remove_all(tmp);
    return 0;
}
