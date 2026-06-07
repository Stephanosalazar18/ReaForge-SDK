#pragma once

#include "host.h"
#include "runtime/lua_runtime.h"
#include "runtime/quickjs_runtime.h"
#include "runtime/jsfx_runtime.h"

namespace reaforge {

class Executor {
public:
    bool init();
    void shutdown();
    InvocationResult invoke(const InvocationRequest& req);

    LuaRuntime& lua() { return lua_; }
    QuickJSRuntime& quickjs() { return quickjs_; }
    JSFXRuntime& jsfx() { return jsfx_; }

private:
    LuaRuntime lua_;
    QuickJSRuntime quickjs_;
    JSFXRuntime jsfx_;
};

}
