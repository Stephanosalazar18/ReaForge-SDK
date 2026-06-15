// ReaForge MVP host — REAPER plugin entry for the agentic MVP.
// Captures AddRemoveReaScript and Main_OnCommand function pointers,
// injects them into the greenfield modules, starts the HTTP server
// on port 7800, and begins the main-thread queue poller.
//
// Build: compiled into reaper_reaforge_host.dll alongside the 5 greenfield
// modules. Uses REAPER SDK headers from third_party/reaper-sdk/sdk/.

#include "reaper_plugin.h"
#include "http_server.h"
#include "artifact_writer.h"
#include "refresh.h"
#include "main_thread_queue.h"

#include <cstdio>
#include <cstdlib>

namespace {

reaforge::host::HttpServer g_server;

// The REAPER SDK returns function pointers as void* via GetFunc().
// Capture one and cast to FnPtr, logging if missing.
template <typename FnPtr>
FnPtr capture(void* (*getfunc)(const char*), const char* name) {
    auto ptr = reinterpret_cast<FnPtr>(getfunc(name));
    if (!ptr) std::fprintf(stderr, "reaforge: REAPER API '%s' not found\n", name);
    return ptr;
}

} // namespace

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
    REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec) {

    (void)hInstance;

    if (!rec) {
        g_server.stop();
        reaforge::host::shutdown();
        std::fprintf(stderr, "reaforge: shutdown\n");
        return 0;
    }

    if (rec->caller_version != REAPER_PLUGIN_VERSION) {
        std::fprintf(stderr, "reaforge: incompatible REAPER version\n");
        return 0;
    }

    // ---- Capture the 2 critical function pointers ----
    auto getfunc = rec->GetFunc;

    // AddRemoveReaScript(bool add, int sectionID, const char* scriptfn, bool commit) -> int
    typedef int (*ARAS_fn)(bool, int, const char*, bool);
    auto aras = capture<ARAS_fn>(getfunc, "AddRemoveReaScript");

    // Main_OnCommand(int command, int flag) -> void
    typedef void (*MOC_fn)(int, int);
    auto moc = capture<MOC_fn>(getfunc, "Main_OnCommand");

    // GetResourcePath(char* buf, int bufsz) -> void
    // Returns REAPER's resource directory (e.g. %APPDATA%\REAPER\).
    typedef void (*GRP_fn)(char*, int);
    auto grp = capture<GRP_fn>(getfunc, "GetResourcePath");

    // ---- Inject into greenfield modules ----
    if (aras) {
        reaforge::host::set_add_remove_reascript(
            [aras](bool add, int sid, const char* fn, bool commit) -> int {
                return aras(add, sid, fn, commit);
            });
    }

    if (moc) {
        reaforge::host::set_main_on_command(
            [moc](int cmd, int flag) { moc(cmd, flag); });
    }

    // Set the REAPER resource path so artifacts land in <resource>/Effects/ReaForge/
    // instead of /tmp/reaforge_default_resource/.
    if (grp) {
        char buf[2048] = {};
        grp(buf, sizeof(buf) - 1);
        reaforge::host::set_resource_path_from_reaper(buf);
    }

    // ---- Start the HTTP server ----
    int port = 7800;
    const char* env_port = std::getenv("REAFORGE_PORT");
    if (env_port && env_port[0]) port = std::atoi(env_port);

    if (!g_server.start(port)) {
        std::fprintf(stderr, "reaforge: HTTP server failed on port %d\n", port);
        return 0;
    }
    std::fprintf(stderr, "reaforge: listening on 0.0.0.0:%d\n", port);

    // ---- Start the main-thread queue poller ----
    reaforge::host::start_polling();

    std::fprintf(stderr, "reaforge: MVP host ready\n");
    return 1;
}