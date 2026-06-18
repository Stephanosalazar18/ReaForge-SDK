#include "artifact_writer.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace reaforge {
namespace host {

namespace {

constexpr const char* kNameRegex = "^[A-Za-z0-9_\\-]{1,64}$";

AddRemoveReaScript_fn g_add_remove_reascript;
Main_OnCommand_fn g_main_on_command;
std::mutex g_fn_mtx;
bool g_fn_initialized = false;

// Global resource path — set by mvp_host.cpp from reaper.GetResourcePath().
// When non-empty, resource_base_dir() uses this instead of the env var.
std::string g_resource_path;

void ensure_default_fn() {
    std::lock_guard<std::mutex> lock(g_fn_mtx);
    if (!g_fn_initialized) {
        // Default mock: returns -1 (failure). Tests override via set_xxx.
        g_add_remove_reascript = [](bool, int, const char*, bool) -> int { return -1; };
        g_fn_initialized = true;
    }
}

std::string forward_slashes(const std::string& p) {
    std::string out;
    out.reserve(p.size());
    for (char c : p) {
        out.push_back(c == '\\' ? '/' : c);
    }
    return out;
}

bool is_under(const fs::path& candidate, const fs::path& root) {
    auto norm_candidate = fs::weakly_canonical(candidate);
    auto norm_root = fs::weakly_canonical(root);
    auto it_c = norm_candidate.begin();
    auto it_r = norm_root.begin();
    for (; it_r != norm_root.end(); ++it_c, ++it_r) {
        if (it_c == norm_candidate.end()) return false;
        if (*it_c != *it_r) return false;
    }
    return true;
}

// Atomic write: write to <path>.tmp then rename to <path>.
bool atomic_write(const fs::path& path, const std::string& content, std::string& err) {
    fs::path tmp = path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) {
            err = "cannot open tmp file for write: " + tmp.string();
            return false;
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.close();
        if (!out.good()) {
            err = "tmp write failed: " + tmp.string();
            return false;
        }
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) {
        err = "rename failed: " + ec.message();
        std::error_code ec2;
        fs::remove(tmp, ec2);
        return false;
    }
    return true;
}

WriteResult do_write(const std::string& subfolder,
                     const std::string& suffix,
                     const std::string& name,
                     const std::string& content,
                     bool overwrite) {
    WriteResult r;
    if (!is_valid_name(name)) {
        r.error_code = "INVALID_NAME";
        r.error_message = "name must match ^[A-Za-z0-9_\\-]{1,64}$";
        return r;
    }

    fs::path base(resource_base_dir());
    fs::path target_dir = base / subfolder / "ReaForge";
    fs::path target = target_dir / (name + suffix);

    // Defense-in-depth: confirm resolved path is under the ReaForge/ subfolder.
    if (!is_under(target, target_dir)) {
        r.error_code = "INVALID_NAME";
        r.error_message = "name escapes target folder";
        return r;
    }

    std::error_code ec;
    if (fs::exists(target, ec) && !overwrite) {
        r.error_code = "FILE_EXISTS";
        r.error_message = "target exists; pass overwrite=true to replace";
        r.path = forward_slashes(fs::absolute(target).string());
        return r;
    }

    std::error_code ec2;
    fs::create_directories(target_dir, ec2);
    // Ignore ec2 — atomic_write will fail visibly if dir is unwritable.

    std::string write_err;
    if (!atomic_write(target, content, write_err)) {
        r.error_code = "WRITE_FAILED";
        r.error_message = write_err;
        return r;
    }

    r.ok = true;
    r.path = forward_slashes(fs::absolute(target).string());
    r.bytes_written = content.size();
    return r;
}

}

bool is_valid_name(const std::string& name) {
    static const std::regex re(kNameRegex);
    return std::regex_match(name, re);
}

void set_add_remove_reascript(AddRemoveReaScript_fn fn) {
    std::lock_guard<std::mutex> lock(g_fn_mtx);
    g_add_remove_reascript = std::move(fn);
    g_fn_initialized = true;
}

void reset_add_remove_reascript() {
    std::lock_guard<std::mutex> lock(g_fn_mtx);
    g_add_remove_reascript = nullptr;
    g_fn_initialized = false;
}

void set_main_on_command_for_writer(Main_OnCommand_fn fn) {
    std::lock_guard<std::mutex> lock(g_fn_mtx);
    g_main_on_command = std::move(fn);
}

std::string resource_base_dir() {
    // 1. Check the global resource path (set by mvp_host.cpp from REAPER API).
    if (!g_resource_path.empty()) return g_resource_path;
    // 2. Check the env var (for tests without REAPER).
    const char* env = std::getenv("REAFORGE_FIXTURE_DIR");
    if (env && *env) return env;
    // 3. Last resort fallback.
    return "/tmp/reaforge_default_resource";
}

void set_resource_path_from_reaper(const std::string& path) {
    g_resource_path = path;
    std::fprintf(stderr, "reaforge: resource path set to %s\n", path.c_str());
}

WriteResult save_jsfx(const std::string& name,
                      const std::string& code,
                      bool overwrite) {
    return do_write("Effects", ".jsfx", name, code, overwrite);
}

WriteResult save_lua(const std::string& name,
                     const std::string& code,
                     bool register_action,
                     bool overwrite,
                     bool run_action) {
    WriteResult r = do_write("Scripts", ".lua", name, code, overwrite);
    if (!r.ok) return r;

    if (register_action) {
        ensure_default_fn();
        if (!g_add_remove_reascript) {
            r.ok = false;
            r.error_code = "REGISTER_FAILED";
            r.error_message = "AddRemoveReaScript not initialized";
            return r;
        }
        // Real signature: AddRemoveReaScript(bool add, int sectionID, const char* scriptfn, bool commit).
        g_add_remove_reascript(false, 0, r.path.c_str(), true);
        int new_id = g_add_remove_reascript(true, 0, r.path.c_str(), true);
        if (new_id <= 0) {
            r.ok = false;
            r.error_code = "REGISTER_FAILED";
            r.error_message = "AddRemoveReaScript returned non-positive id";
            r.action_id.reset();
            return r;
        }
        r.action_id = new_id;

        // Auto-execute the script via Main_OnCommand so the user doesn't
        // need to go to Actions → find → run manually.
        if (run_action) {
            if (g_main_on_command) {
                g_main_on_command(new_id, 0);
                r.executed = true;
            } else {
                r.executed = false;
                r.execute_warning = "Main_OnCommand not initialized; run the action manually";
            }
        }
    }
    return r;
}

WriteResult save_fx_chain(const std::string& name,
                          const std::string& content,
                          bool overwrite) {
    return do_write("FXChains", ".RfxChain", name, content, overwrite);
}

}
}
