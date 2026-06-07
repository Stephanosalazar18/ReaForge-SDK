#include "lua_runtime.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>

namespace reaforge {

bool LuaRuntime::init() {
    L_ = luaL_newstate();
    if (!L_) return false;
    luaL_openlibs(L_);
    return true;
}

void LuaRuntime::shutdown() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaRuntime::eval(const std::string& source, std::string& out_result, std::string& out_error) {
    if (!L_) {
        out_error = "lua: runtime not initialized";
        return false;
    }
    int rc = luaL_loadstring(L_, source.c_str());
    if (rc != LUA_OK) {
        out_error = std::string("lua: ") + lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    rc = lua_pcall(L_, 0, 1, 0);
    if (rc != LUA_OK) {
        out_error = std::string("lua: ") + lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return false;
    }
    if (lua_isstring(L_, -1)) {
        out_result = lua_tostring(L_, -1);
    } else if (lua_isnumber(L_, -1)) {
        out_result = std::to_string(lua_tonumber(L_, -1));
    }
    lua_pop(L_, 1);
    return true;
}

}
