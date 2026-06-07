#include "context_menu.h"
#include "extension_loader.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace reaforge {
namespace host {
namespace context_menu {

namespace {

bool g_registered = false;
std::vector<MenuEntry> g_track_cache;
std::vector<MenuEntry> g_item_cache;

void rebuild_cache() {
    auto manifests = loader::scan();
    g_track_cache.clear();
    g_item_cache.clear();
    for (const auto& m : manifests) {
        MenuEntry e{m.id, m.name.empty() ? m.id : m.name};
        if (m.target == "track" || m.target == "master") g_track_cache.push_back(e);
        if (m.target == "item" || m.target == "midi_item") g_item_cache.push_back(e);
    }
    std::fprintf(stderr, "reaforge: context menu cache rebuilt: %zu track, %zu item\n",
                 g_track_cache.size(), g_item_cache.size());
}

void on_menu_click(const std::string& extension_id, const std::string& target_kind, int target_index) {
    std::fprintf(stderr, "reaforge: menu click id=%s kind=%s index=%d\n",
                 extension_id.c_str(), target_kind.c_str(), target_index);
    // Real REAPER dispatch happens here: build InvocationRequest and
    // call executor.invoke({id, fn=entry, args=...}). Stays a stub in
    // this branch; the real call is wired once the host is loaded
    // into REAPER.
}

}

bool register_hooks() {
    if (g_registered) return true;
    rebuild_cache();
    g_registered = true;
    std::fprintf(stderr, "reaforge: context_menu::register_hooks (stub: would call HookCustomMenu here)\n");
    return true;
}

void unregister_hooks() {
    if (!g_registered) return;
    g_registered = false;
    g_track_cache.clear();
    g_item_cache.clear();
    std::fprintf(stderr, "reaforge: context_menu::unregister_hooks\n");
}

void invalidate_cache() {
    rebuild_cache();
}

}
}
}
