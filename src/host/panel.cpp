#include "panel.h"
#include "context_menu.h"
#include "extension_loader.h"

#include <cstdio>
#include <string>
#include <vector>

namespace reaforge {
namespace host {
namespace panel {

namespace {
bool created_ = false;
}

bool create() {
    if (created_) return true;
    std::fprintf(stderr, "reaforge: panel::create\n");
    created_ = true;
    return true;
}

void destroy() {
    if (!created_) return;
    std::fprintf(stderr, "reaforge: panel::destroy\n");
    created_ = false;
}

void render() {
    if (!created_) return;
    auto manifests = loader::scan();
    std::fprintf(stderr, "reaforge: panel render — %zu extensions\n", manifests.size());
    for (const auto& m : manifests) {
        std::fprintf(stderr, "  - %s [%s] target=%s\n",
                     m.id.c_str(), m.runtime.c_str(), m.target.c_str());
    }
}

void on_reload(const std::string& extension_id) {
    std::fprintf(stderr, "reaforge: panel::on_reload(%s)\n", extension_id.c_str());
    if (loader::reload(extension_id)) {
        context_menu::invalidate_cache();
    }
}

}
}
}
