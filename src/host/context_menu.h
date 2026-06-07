#pragma once

#include <string>
#include <vector>

namespace reaforge {
namespace host {
namespace context_menu {

struct MenuEntry {
    std::string extension_id;
    std::string label;
};

bool register_hooks();
void unregister_hooks();
void invalidate_cache();

}
}
}
