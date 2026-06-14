#include "refresh.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace reaforge {
namespace host {

namespace {

// ---- function pointer storage (singleton; mutex-guarded) ----
struct RefreshFns {
    Main_OnCommand_fn     main_on_command = nullptr;
    NamedCommandLookup_fn named_command_lookup = nullptr;
};

RefreshFns& fns() {
    static RefreshFns instance;
    return instance;
}

std::mutex fns_mtx_;

// ---- timestamp helpers ----
std::string iso8601_utc_now() {
    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto t = clock::to_time_t(now);
    std::tm tm_buf{};
    gmtime_r(&t, &tm_buf);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;
    std::ostringstream os;
    os << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';
    return os.str();
}

}

// Returns the SWS-extension "Refresh" action command ID if SWS is installed,
// or 0 if the function pointer is null. We treat absence of SWS the same as
// a null function pointer (no refresh attempted).
//
// The "_S&M_REFRESH" name is the SWS/S&M extension's named command that
// rescans the action list and FX list. It is the standard cross-REAPER
// fallback for native refresh APIs. If SWS is not installed, REAPER will
// still receive the call but the result is a no-op — which is fine for the
// agent's purpose: the agent must always tell the user about the JSFX
// manual rescan (see warnings below).
constexpr const char* kSwsRefreshCommandName = "_S&M_REFRESH";

RefreshResult refresh_now() {
    RefreshResult r;

    // Snapshot the function pointers (no lock held during REAPER calls).
    Main_OnCommand_fn     p_moc;
    NamedCommandLookup_fn p_ncl;
    {
        std::lock_guard<std::mutex> lock(fns_mtx_);
        p_moc = fns().main_on_command;
        p_ncl = fns().named_command_lookup;
    }

    if (p_moc && p_ncl) {
        int cmd_id = p_ncl(kSwsRefreshCommandName);
        if (cmd_id > 0) {
            p_moc(cmd_id, 0);
        } else {
            // SWS not installed (or _S&M_REFRESH missing) — best-effort no-op.
            std::fprintf(stderr,
                "reaforge: refresh: '%s' not found (SWS extension missing?); "
                "action list NOT rescanned\n", kSwsRefreshCommandName);
            r.warnings.push_back(
                "Action List rescan skipped: SWS extension not installed "
                "(_S&M_REFRESH not found)");
        }
    } else {
        // Function pointers not initialized (we are in a unit test, or
        // REAPER has not bound the API yet). Log a debug line; do not warn
        // the user — the cpp-unit test depends on this path.
        std::fprintf(stderr,
            "reaforge: refresh: REAPER function pointers not initialized; "
            "skipping action-list rescan (this is expected in cpp-unit tests)\n");
    }

    // JSFX rescan is always a manual step in REAPER's FX browser — the
    // public SDK does not expose a programmatic rescan for <Effects>/ReaForge/.
    // Per the design (R-new mitigation), the agent must surface this to the
    // user so they can right-click FX Browser → "Scan for new plug-ins" (or
    // restart REAPER) to make the new JSFX appear.
    r.warnings.push_back(
        "JSFX rescan requires manual FX Browser -> Scan for new plug-ins "
        "(or REAPER restart); <Effects>/ReaForge/ is not auto-rescanned");

    r.refreshed_at = iso8601_utc_now();
    return r;
}

void set_refresh_fns(Main_OnCommand_fn moc, NamedCommandLookup_fn ncl) {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns().main_on_command = moc;
    fns().named_command_lookup = ncl;
}

void reset_refresh_fns() {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns() = RefreshFns{};
}

}
}
