#pragma once

#include <functional>
#include <optional>
#include <string>

#if defined(_WIN32)
// Forward-declare SetEnvironmentVariableA from kernel32 (linked by default).
// We avoid pulling in <windows.h> because its headers have ordering issues
// when included from C++ TUs that also use standard library types.
extern "C" __declspec(dllimport) int __stdcall SetEnvironmentVariableA(
    const char* lpName, const char* lpValue);
#endif

namespace reaforge {
namespace host {

// Validates a name against the canonical regex. Pure function.
bool is_valid_name(const std::string& name);

// REAPER function pointer shape. In REAPER, captured at init:
//   auto fn = reinterpret_cast<AddRemoveReaScript_fn>(
//       reaper_api_table["AddRemoveReaScript"]);
// In tests, a lambda that records the call without doing anything.
using AddRemoveReaScript_fn =
    std::function<int(bool add, int sectionID, const char* scriptfn, bool commit)>;

// Set the REAPER function pointer (or a mock in tests). Thread-safe; called
// from host::init() before the HTTP server starts.
void set_add_remove_reascript(AddRemoveReaScript_fn fn);

// For tests: reset to a default mock that returns -1 (registration failure).
void reset_add_remove_reascript();

struct WriteResult {
    bool ok = false;
    std::string path;          // forward-slash absolute
    std::size_t bytes_written = 0;
    std::optional<int> action_id;  // populated only for save_lua with register_action=true
    std::string error_code;    // empty on success; one of: INVALID_NAME, FILE_EXISTS, WRITE_FAILED, REGISTER_FAILED
    std::string error_message;
};

// Base dir = <REAPER resource>/. For PR 4 it's parameterized via env var
// REAFORGE_FIXTURE_DIR so tests don't need REAPER.
std::string resource_base_dir();

// Cross-platform setenv wrapper. On POSIX we use setenv; on Windows we use the
// Win32 SetEnvironmentVariable API instead of the CRT's _putenv_s. The CRT
// _putenv_s in MSVC has historically been buggy when called repeatedly with the
// same variable name (it sometimes fails with errno 3 or corrupts the env
// block). The Win32 API is the canonical Windows way and is well-tested.
inline void setenv_or_putenv(const char* name, const char* value, int overwrite) {
#if defined(_WIN32)
    (void)overwrite;
    ::SetEnvironmentVariableA(name, value);
#else
    ::setenv(name, value, overwrite);
#endif
}

// Writes <base>/Effects/ReaForge/<name>.jsfx atomically.
WriteResult save_jsfx(const std::string& name,
                      const std::string& code,
                      bool overwrite);

// Writes <base>/Scripts/ReaForge/<name>.lua atomically.
// If register_action=true, calls AddRemoveReaScript via the captured fn.
WriteResult save_lua(const std::string& name,
                     const std::string& code,
                     bool register_action,
                     bool overwrite);

// Writes <base>/FXChains/ReaForge/<name>.RfxChain atomically.
WriteResult save_fx_chain(const std::string& name,
                          const std::string& content,
                          bool overwrite);

}
}
