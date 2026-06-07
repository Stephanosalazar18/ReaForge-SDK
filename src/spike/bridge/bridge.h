#pragma once

#include <optional>
#include <string>

namespace reaforge {

enum class BridgeStatus {
    Ok,
    OutOfRange,
    NotOnMainThread,
    NotAvailable,
};

struct BridgeCallResult {
    BridgeStatus status = BridgeStatus::Ok;
    std::optional<double> number;
    std::optional<std::string> text;
};

class Bridge {
public:
    static void install_main_thread_id();

    BridgeCallResult get_cursor_position();
    BridgeCallResult get_project_tempo();
    BridgeCallResult get_track_count();
    BridgeCallResult get_track_name(int index);
    BridgeCallResult get_master_track_volume();
};

}
