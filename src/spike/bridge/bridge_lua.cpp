#include "bridge.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string>

namespace reaforge {

namespace {

Bridge& bridge() {
    static Bridge b;
    return b;
}

int l_get_cursor_position(lua_State* L) {
    auto r = bridge().get_cursor_position();
    if (r.status != BridgeStatus::Ok) {
        return luaL_error(L, "bridge error: status=%d", static_cast<int>(r.status));
    }
    lua_pushnumber(L, r.number.value_or(0.0));
    return 1;
}

int l_get_project_tempo(lua_State* L) {
    auto r = bridge().get_project_tempo();
    if (r.status != BridgeStatus::Ok) {
        return luaL_error(L, "bridge error: status=%d", static_cast<int>(r.status));
    }
    lua_pushnumber(L, r.number.value_or(0.0));
    return 1;
}

int l_get_track_count(lua_State* L) {
    auto r = bridge().get_track_count();
    if (r.status != BridgeStatus::Ok) {
        return luaL_error(L, "bridge error: status=%d", static_cast<int>(r.status));
    }
    lua_pushinteger(L, static_cast<lua_Integer>(r.number.value_or(0.0)));
    return 1;
}

int l_get_track_name(lua_State* L) {
    int index = static_cast<int>(luaL_checkinteger(L, 1));
    auto r = bridge().get_track_name(index);
    if (r.status == BridgeStatus::OutOfRange) {
        lua_pushnil(L);
        return 1;
    }
    if (r.status != BridgeStatus::Ok) {
        return luaL_error(L, "bridge error: status=%d", static_cast<int>(r.status));
    }
    lua_pushstring(L, r.text.value_or("").c_str());
    return 1;
}

int l_get_master_track_volume(lua_State* L) {
    auto r = bridge().get_master_track_volume();
    if (r.status != BridgeStatus::Ok) {
        return luaL_error(L, "bridge error: status=%d", static_cast<int>(r.status));
    }
    lua_pushnumber(L, r.number.value_or(0.0));
    return 1;
}

}

void register_lua_bridge(lua_State* L) {
    lua_newtable(L);
    lua_pushcfunction(L, l_get_cursor_position);       lua_setfield(L, -2, "get_cursor_position");
    lua_pushcfunction(L, l_get_project_tempo);         lua_setfield(L, -2, "get_project_tempo");
    lua_pushcfunction(L, l_get_track_count);           lua_setfield(L, -2, "get_track_count");
    lua_pushcfunction(L, l_get_track_name);            lua_setfield(L, -2, "get_track_name");
    lua_pushcfunction(L, l_get_master_track_volume);   lua_setfield(L, -2, "get_master_track_volume");
    lua_setglobal(L, "reaforge");
}

}
