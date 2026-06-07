#include "host.h"
#include "panel.h"

#include <cstdio>
#include <cstdlib>

namespace reaforge {
namespace host {

namespace {
bool g_initialized = false;
}

int init() {
    if (g_initialized) return 0;
    std::fprintf(stderr, "reaforge: host::init\n");
    if (!panel::create()) {
        std::fprintf(stderr, "reaforge: panel::create failed\n");
        return 1;
    }
    g_initialized = true;
    std::fprintf(stderr, "reaforge: host::init done\n");
    return 0;
}

void shutdown() {
    if (!g_initialized) return;
    std::fprintf(stderr, "reaforge: host::shutdown\n");
    panel::destroy();
    g_initialized = false;
}

}
}
