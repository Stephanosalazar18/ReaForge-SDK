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
    Main_OnCommand_fn     main_on_command;
    NamedCommandLookup_fn named_command_lookup;
};

RefreshFns& fns() {
    static RefreshFns instance;
    return instance;
}

std::mutex fns_mtx_;

} // anonymous namespace

// ---- timestamp helpers ----
std::string iso8601_utc_now() {
    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto t = clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;
    std::ostringstream os;
    os << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setw(3) << std::setfill('0') << ms.count() << 'Z';
    return os.str();
}

// std::function setters (called from host.cpp with lambdas)
void set_main_on_command(Main_OnCommand_fn fn) {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns().main_on_command = std::move(fn);
}

void set_named_command_lookup(NamedCommandLookup_fn fn) {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns().named_command_lookup = std::move(fn);
}

// Legacy: wrap raw pointers as std::function for test compatibility.
void set_refresh_fns(Main_OnCommand_raw_fn moc, NamedCommandLookup_raw_fn ncl) {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    if (moc) fns().main_on_command = std::function<void(int,int)>(moc);
    else fns().main_on_command = nullptr;
    if (ncl) fns().named_command_lookup = std::function<int(const char*)>(ncl);
    else fns().named_command_lookup = nullptr;
}

void reset_refresh_fns() {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns().main_on_command = nullptr;
    fns().named_command_lookup = nullptr;
}

// Returns the SWS-extension "Refresh" action command ID if SWS is installed,
// or 0 if the function pointer is null.
constexpr const char* kSwsRefreshCommandName = "_S&M_REFRESH";

RefreshResult refresh_now() {
    RefreshResult r;

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
            std::fprintf(stderr, "reaforge: refresh: '%s' not found (SWS extension missing?); action list NOT rescanned\n",
                         kSwsRefreshCommandName);
            r.warnings.push_back(
                std::string("'") + kSwsRefreshCommandName +
                "' not found (SWS extension missing?); action list NOT rescanned");
        }
    } else {
        std::fprintf(stderr, "reaforge: refresh: REAPER function pointers not initialized; skipping action-list rescan (this is expected in cpp-unit tests)\n");
    }

    r.refreshed_at = iso8601_utc_now();
    r.warnings.push_back(
        "JSFX rescan requires manual \"Scan for new plug-ins\" in REAPER's FX browser");

    return r;
}

} // namespace reaforge::host
} // namespace reaforge
