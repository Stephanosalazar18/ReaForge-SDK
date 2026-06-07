#pragma once

extern "C" {
#include <lua.h>
}

#include <string>

namespace reaforge {

class LuaRuntime {
public:
    bool init();
    void shutdown();
    bool eval(const std::string& source, std::string& out_result, std::string& out_error);
    lua_State* state() { return L_; }

private:
    lua_State* L_ = nullptr;
};

}
