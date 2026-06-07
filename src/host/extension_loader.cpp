#include "extension_loader.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace reaforge {
namespace host {
namespace loader {

namespace fs = std::filesystem;

namespace {

const std::string kDefaultDir = "ReaForge/extensions";

}

std::string g_extensions_dir_override;

std::string read_file(const fs::path& path) {
    std::ifstream f(path);
    if (!f) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool parse_manifest(const std::string& lua_source, Manifest& out, std::string& err) {
    lua_State* L = luaL_newstate();
    if (!L) { err = "luaL_newstate failed"; return false; }
    luaL_openlibs(L);
    int rc = luaL_loadstring(L, lua_source.c_str());
    if (rc != LUA_OK) { err = std::string("load: ") + lua_tostring(L, -1); lua_close(L); return false; }
    rc = lua_pcall(L, 0, 1, 0);
    if (rc != LUA_OK) { err = std::string("pcall: ") + lua_tostring(L, -1); lua_close(L); return false; }
    if (!lua_istable(L, -1)) { err = "manifest must return a table"; lua_close(L); return false; }

    auto get = [&](const char* k) -> std::string {
        lua_getfield(L, -1, k);
        std::string v;
        if (lua_isstring(L, -1)) v = lua_tostring(L, -1);
        lua_pop(L, 1);
        return v;
    };

    out.id = get("id");
    out.name = get("name");
    out.runtime = get("runtime");
    out.target = get("target");
    out.entry = get("entry");
    if (out.entry.empty()) out.entry = "run";
    lua_close(L);
    return !(out.id.empty() || out.runtime.empty() || out.target.empty());
}

}

const std::string& extensions_dir() {
    if (!g_extensions_dir_override.empty()) return g_extensions_dir_override;
    static const std::string kFallback = kDefaultDir;
    return kFallback;
}

std::vector<Manifest> scan() {
    std::vector<Manifest> out;
    fs::path dir = extensions_dir();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
        std::fprintf(stderr, "reaforge: created extensions dir %s\n", dir.c_str());
        return out;
    }
    if (!fs::is_directory(dir)) return out;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        fs::path manifest_path = entry.path() / "manifest.lua";
        if (!fs::exists(manifest_path)) continue;
        std::string src = read_file(manifest_path);
        if (src.empty()) continue;
        Manifest m;
        std::string err;
        if (!parse_manifest(src, m, err)) {
            std::fprintf(stderr, "reaforge: bad manifest in %s: %s\n",
                         manifest_path.c_str(), err.c_str());
            continue;
        }
        m.source_path = (entry.path() / (m.id + "." + m.runtime)).string();
        out.push_back(std::move(m));
    }
    return out;
}

bool reload(const std::string& id) {
    fs::path dir = extensions_dir();
    if (!fs::exists(dir)) return false;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        fs::path manifest_path = entry.path() / "manifest.lua";
        if (!fs::exists(manifest_path)) continue;
        std::string src = read_file(manifest_path);
        Manifest m;
        std::string err;
        if (!parse_manifest(src, m, err)) continue;
        if (m.id == id) {
            std::fprintf(stderr, "reaforge: reloaded %s\n", id.c_str());
            return true;
        }
    }
    return false;
}

}
}
}
