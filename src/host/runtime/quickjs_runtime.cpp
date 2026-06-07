#include "quickjs_runtime.h"

#include <string>

namespace reaforge {

bool QuickJSRuntime::init() {
    rt_ = JS_NewRuntime();
    if (!rt_) return false;
    ctx_ = JS_NewContext(rt_);
    if (!ctx_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
        return false;
    }
    return true;
}

void QuickJSRuntime::shutdown() {
    if (ctx_) {
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }
    if (rt_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
    }
}

bool QuickJSRuntime::eval(const std::string& source, std::string& out_result, std::string& out_error) {
    if (!ctx_) {
        out_error = "quickjs: runtime not initialized";
        return false;
    }
    JSValue val = JS_Eval(ctx_, source.c_str(), source.size(), "<spike>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        JSValue exc = JS_GetException(ctx_);
        const char* str = JS_ToCString(ctx_, exc);
        out_error = std::string("quickjs: ") + (str ? str : "unknown");
        if (str) JS_FreeCString(ctx_, str);
        JS_FreeValue(ctx_, exc);
        JS_FreeValue(ctx_, val);
        return false;
    }
    if (JS_IsString(val)) {
        const char* str = JS_ToCString(ctx_, val);
        out_result = str ? str : "";
        if (str) JS_FreeCString(ctx_, str);
    } else if (JS_IsNumber(val)) {
        double n;
        JS_ToFloat64(ctx_, &n, val);
        out_result = std::to_string(n);
    } else if (JS_IsBool(val)) {
        out_result = JS_ToBool(ctx_, val) ? "true" : "false";
    }
    JS_FreeValue(ctx_, val);
    return true;
}

}
