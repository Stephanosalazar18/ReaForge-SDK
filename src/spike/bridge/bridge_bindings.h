#pragma once

#include <lua.h>
#include <quickjs.h>

namespace reaforge {

void register_lua_bridge(lua_State* L);
void register_quickjs_bridge(JSContext* ctx);

}
