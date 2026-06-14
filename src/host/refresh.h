#pragma once

#include <functional>
#include <string>
#include <vector>

namespace reaforge {
namespace host {

// Result of a refresh call. `refreshed_at` is an ISO 8601 UTC timestamp with
// millisecond precision (e.g. "2026-06-13T19:33:18.123Z"). `warnings` is the
// list of human-readable strings the LLM should surface to the user; the JSFX
// rescan warning is always present per the design's R-new mitigation
// (REAPER's FX browser does not auto-rescan for JSFX in <Effects>/ReaForge/).
struct RefreshResult {
    std::string refreshed_at;  // ISO 8601 UTC
    std::vector<std::string> warnings;
};

// Trigger a REAPER cache rescan. The function pointer slots for
// Main_OnCommand and NamedCommandLookup are captured at REAPER init via
// rec->GetFunc("Name"); in tests, override via set_refresh_fns().
//
// Lane: runtime-reaper. The real REAPER API is only callable from REAPER's
// main thread; the http_server marshals to the main thread before invoking
// this function (see http_server.cpp). The cpp-unit test for this module
// exercises the response shape and warning semantics with a mocked function
// pointer; the actual REAPER behavior is verified in PR 7 on REAPER Windows.
RefreshResult refresh_now();

// REAPER function pointer signatures for refresh.
using Main_OnCommand_raw_fn = void (*)(int command, int flag);
using NamedCommandLookup_raw_fn = int (*)(const char* command_name);

// std::function versions — use these from host.cpp so lambdas work.
using Main_OnCommand_fn = std::function<void(int command, int flag)>;
using NamedCommandLookup_fn = std::function<int(const char* command_name)>;

// Inject a Main_OnCommand implementation (raw function pointer or lambda).
// Called from host.cpp at REAPER init.
void set_main_on_command(Main_OnCommand_fn fn);

// Inject a NamedCommandLookup implementation.
void set_named_command_lookup(NamedCommandLookup_fn fn);

// Reset the function pointer slots to null. Used by tests and at shutdown.
void reset_refresh_fns();

// Legacy raw-pointer setter (used by tests that mock raw function pointers).
void set_refresh_fns(Main_OnCommand_raw_fn moc, NamedCommandLookup_raw_fn ncl);

}
}
