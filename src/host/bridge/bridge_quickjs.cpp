#include "bridge.h"

#include <quickjs.h>
#include <string>

namespace reaforge {

namespace {

Bridge& bridge() {
    static Bridge b;
    return b;
}

JSValue js_get_cursor_position(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    auto r = bridge().get_cursor_position();
    if (r.status != BridgeStatus::Ok) {
        return JS_ThrowTypeError(ctx, "bridge error: status=%d", static_cast<int>(r.status));
    }
    return JS_NewFloat64(ctx, r.number.value_or(0.0));
}

JSValue js_get_project_tempo(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    auto r = bridge().get_project_tempo();
    if (r.status != BridgeStatus::Ok) {
        return JS_ThrowTypeError(ctx, "bridge error: status=%d", static_cast<int>(r.status));
    }
    return JS_NewFloat64(ctx, r.number.value_or(0.0));
}

JSValue js_get_track_count(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    auto r = bridge().get_track_count();
    if (r.status != BridgeStatus::Ok) {
        return JS_ThrowTypeError(ctx, "bridge error: status=%d", static_cast<int>(r.status));
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(r.number.value_or(0.0)));
}

JSValue js_get_track_name(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "get_track_name(index): missing arg");
    int32_t index;
    if (JS_ToInt32(ctx, &index, argv[0])) return JS_EXCEPTION;
    auto r = bridge().get_track_name(index);
    if (r.status == BridgeStatus::OutOfRange) {
        return JS_NULL;
    }
    if (r.status != BridgeStatus::Ok) {
        return JS_ThrowTypeError(ctx, "bridge error: status=%d", static_cast<int>(r.status));
    }
    return JS_NewString(ctx, r.text.value_or("").c_str());
}

JSValue js_get_master_track_volume(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    auto r = bridge().get_master_track_volume();
    if (r.status != BridgeStatus::Ok) {
        return JS_ThrowTypeError(ctx, "bridge error: status=%d", static_cast<int>(r.status));
    }
    return JS_NewFloat64(ctx, r.number.value_or(0.0));
}

}

void register_quickjs_bridge(JSContext* ctx) {
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "get_cursor_position",
        JS_NewCFunction(ctx, js_get_cursor_position, "get_cursor_position", 0));
    JS_SetPropertyStr(ctx, obj, "get_project_tempo",
        JS_NewCFunction(ctx, js_get_project_tempo, "get_project_tempo", 0));
    JS_SetPropertyStr(ctx, obj, "get_track_count",
        JS_NewCFunction(ctx, js_get_track_count, "get_track_count", 0));
    JS_SetPropertyStr(ctx, obj, "get_track_name",
        JS_NewCFunction(ctx, js_get_track_name, "get_track_name", 1));
    JS_SetPropertyStr(ctx, obj, "get_master_track_volume",
        JS_NewCFunction(ctx, js_get_master_track_volume, "get_master_track_volume", 0));
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "reaforge", obj);
    JS_FreeValue(ctx, global);
}

}
