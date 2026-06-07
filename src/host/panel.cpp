#include "panel.h"

#include <cstdio>
#include <string>

namespace reaforge {
namespace host {
namespace panel {

namespace {
bool created_ = false;
}

bool create() {
    if (created_) return true;
    std::fprintf(stderr, "reaforge: panel::create stub\n");
    created_ = true;
    return true;
}

void destroy() {
    if (!created_) return;
    std::fprintf(stderr, "reaforge: panel::destroy stub\n");
    created_ = false;
}

void render() {
    // PR 3 (panel implementation) replaces this with ReaImGui widgets.
    // For PR 1, we only verify the symbol is exported and the lifecycle
    // hooks fire from the host entry point.
}

void on_reload(const std::string& extension_id) {
    std::fprintf(stderr, "reaforge: panel::on_reload(%s) stub\n", extension_id.c_str());
}

}
}
}
