#include "executor.h"
#include "bridge/bridge.h"
#include "bridge/bridge_bindings.h"

#include <string>

namespace reaforge {

bool Executor::init() {
    if (!lua_.init()) return false;
    if (!quickjs_.init()) {
        lua_.shutdown();
        return false;
    }
    if (!jsfx_.init()) {
        quickjs_.shutdown();
        lua_.shutdown();
        return false;
    }
    register_lua_bridge(lua_.state());
    register_quickjs_bridge(quickjs_.context());
    return true;
}

void Executor::shutdown() {
    jsfx_.shutdown();
    quickjs_.shutdown();
    lua_.shutdown();
}

InvocationResult Executor::invoke(const InvocationRequest& req) {
    InvocationResult out;
    if (req.id.empty() || req.fn.empty()) {
        out.ok = false;
        out.error = "executor: id and fn are required";
        return out;
    }

    if (req.id == "lua-demo") {
        std::string source = "return " + req.fn + "()";
        std::string r, e;
        if (lua_.eval(source, r, e)) {
            out.ok = true;
            out.result = r;
        } else {
            out.ok = false;
            out.error = e;
        }
        return out;
    }

    if (req.id == "js-demo") {
        std::string source = "globalThis." + req.fn + "()";
        std::string r, e;
        if (quickjs_.eval(source, r, e)) {
            out.ok = true;
            out.result = r;
        } else {
            out.ok = false;
            out.error = e;
        }
        return out;
    }

    if (req.id == "jsfx-demo") {
        out.ok = false;
        out.error = "spike: jsfx bridge not implemented";
        return out;
    }

    out.ok = false;
    out.error = "executor: unknown id " + req.id;
    return out;
}

}
