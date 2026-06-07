#include "host.h"
#include "panel.h"
#include "context_menu.h"
#include "extension_loader.h"

#include <cstdio>

namespace reaforge {
namespace host {

namespace {
bool g_initialized = false;
}

int init() {
    if (g_initialized) return 0;
    std::fprintf(stderr, "reaforge: host::init\n");

    auto manifests = loader::scan();
    std::fprintf(stderr, "reaforge: found %zu extensions in %s\n",
                 manifests.size(), loader::extensions_dir().c_str());

    if (!context_menu::register_hooks()) {
        std::fprintf(stderr, "reaforge: context_menu::register_hooks failed\n");
        return 1;
    }
    if (!panel::create()) {
        std::fprintf(stderr, "reaforge: panel::create failed\n");
        context_menu::unregister_hooks();
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
    context_menu::unregister_hooks();
    g_initialized = false;
}

}
}
