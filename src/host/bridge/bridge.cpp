#include "bridge.h"

#include <thread>

namespace reaforge {

namespace {
std::thread::id g_main_thread_id;

double stub_cursor_position() { return 0.0; }
double stub_tempo() { return 120.0; }
int stub_track_count() { return 4; }
bool stub_track_name(int index, std::string& out) {
    static const char* names[] = {"Drums", "Bass", "Keys", "Vocals"};
    if (index < 0 || index >= 4) return false;
    out = names[index];
    return true;
}
double stub_master_volume() { return 0.85; }

bool is_main_thread() {
    return std::this_thread::get_id() == g_main_thread_id;
}
}

void Bridge::install_main_thread_id() {
    g_main_thread_id = std::this_thread::get_id();
}

BridgeCallResult Bridge::get_cursor_position() {
    if (!is_main_thread()) return {BridgeStatus::NotOnMainThread, std::nullopt, std::nullopt};
    return {BridgeStatus::Ok, stub_cursor_position(), std::nullopt};
}

BridgeCallResult Bridge::get_project_tempo() {
    if (!is_main_thread()) return {BridgeStatus::NotOnMainThread, std::nullopt, std::nullopt};
    return {BridgeStatus::Ok, stub_tempo(), std::nullopt};
}

BridgeCallResult Bridge::get_track_count() {
    if (!is_main_thread()) return {BridgeStatus::NotOnMainThread, std::nullopt, std::nullopt};
    return {BridgeStatus::Ok, static_cast<double>(stub_track_count()), std::nullopt};
}

BridgeCallResult Bridge::get_track_name(int index) {
    if (!is_main_thread()) return {BridgeStatus::NotOnMainThread, std::nullopt, std::nullopt};
    std::string name;
    if (!stub_track_name(index, name)) {
        return {BridgeStatus::OutOfRange, std::nullopt, std::nullopt};
    }
    return {BridgeStatus::Ok, std::nullopt, name};
}

BridgeCallResult Bridge::get_master_track_volume() {
    if (!is_main_thread()) return {BridgeStatus::NotOnMainThread, std::nullopt, std::nullopt};
    return {BridgeStatus::Ok, stub_master_volume(), std::nullopt};
}

}
