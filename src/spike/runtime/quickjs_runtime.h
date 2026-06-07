#pragma once

#include <quickjs.h>

namespace reaforge {

class QuickJSRuntime {
public:
    bool init();
    void shutdown();
    bool eval(const std::string& source, std::string& out_result, std::string& out_error);
    JSContext* context() { return ctx_; }

private:
    JSRuntime* rt_ = nullptr;
    JSContext* ctx_ = nullptr;
};

}
