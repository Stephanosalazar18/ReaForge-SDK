#pragma once

#include <string>
#include <vector>

namespace reaforge {
namespace host {

// One artifact entry as returned by GET /v1/artifacts.
struct ArtifactEntry {
    std::string path;   // forward-slash absolute
    std::size_t size = 0;
    long long mtime = 0;  // seconds since epoch
    std::string kind;    // "jsfx" | "lua" | "fx_chain"
};

// Pure-FS enumerator. Walks the 3 ReaForge/ subfolders under base_dir and
// returns their contents. Missing folders yield an empty list (no error).
// `kind` filter: empty = all 3 kinds, else one of "jsfx" | "lua" | "fx_chain".
// Invalid `kind` returns empty list and a populated invalid_kind=true result
// (callers translate to 400 INVALID_KIND).
struct ListArtifactsResult {
    std::vector<ArtifactEntry> entries;
    bool invalid_kind = false;
    std::string invalid_kind_value;  // echoes the bad value
};

ListArtifactsResult list_artifacts(const std::string& base_dir,
                                    const std::string& kind_filter);

// Result of a get_api_reference call. `ok=false` => error_code is set
// (MISSING_TARGET or INVALID_TARGET). `ok=true` => target/reference populated.
struct ApiReferenceResult {
    bool ok = false;
    std::string target;
    std::string reference;
    std::string error_code;    // MISSING_TARGET | INVALID_TARGET
    std::string error_message;
};

ApiReferenceResult get_api_reference(const std::string& target);

// Result of a get_state call. `ok=true` => json populated as a string
// (nlohmann::json::dump()). `ok=false` => error_code is set
// (REAPER_NOT_AVAILABLE when function pointers are null, or
// PROJECT_NOT_OPEN when no project is open).
struct GetStateResult {
    bool ok = false;
    std::string json;
    std::string error_code;
    std::string error_message;
};

// Returns a compact JSON projection of the current REAPER project. Pure
// REAPER-API bound; requires the function pointer slots to be set
// (see reaper_bind.{h,cpp}). If any pointer is null, returns ok=false
// with error_code="REAPER_NOT_AVAILABLE". If CountTracks returns 0
// (no project), returns ok=false with error_code="PROJECT_NOT_OPEN".
// summary=true omits fx_names[] per track (keeps fx_count).
GetStateResult get_state(bool summary);

// Reset the function pointer slots to null. Used by tests and at shutdown.
void reset_reaper_function_pointers();

// Setters for the function pointer slots. In REAPER, the host captures
// each via `rec->GetFunc("Name")` at init. In tests, use mocks.
using CountTracks_fn        = int (*)(void* proj);
using GetTrack_fn           = void* (*)(void* proj, int idx);
using GetTrackName_fn       = bool (*)(void* track, char* buf, int buf_sz);
using TrackFX_GetCount_fn   = int (*)(void* track);
using TrackFX_GetFXName_fn  = bool (*)(void* track, int fx, char* buf, int buf_sz);
using GetProjectName_fn     = void (*)(void* proj, char* buf, int buf_sz);
using Master_GetTempo_fn    = double (*)();
using GetSetProjectInfo_fn  = double (*)(void* proj, const char* desc, double value, bool is_set);
using CountSelectedMediaItems_fn = int (*)(void* proj);
using GetSelectedMediaItem_fn    = void* (*)(void* proj, int selitem);
using GetMediaItemTrack_fn       = void* (*)(void* item);

void set_reaper_fn_count_tracks(CountTracks_fn fn);
void set_reaper_fn_get_track(GetTrack_fn fn);
void set_reaper_fn_get_track_name(GetTrackName_fn fn);
void set_reaper_fn_track_fx_get_count(TrackFX_GetCount_fn fn);
void set_reaper_fn_track_fx_get_fx_name(TrackFX_GetFXName_fn fn);
void set_reaper_fn_get_project_name(GetProjectName_fn fn);
void set_reaper_fn_master_get_tempo(Master_GetTempo_fn fn);
void set_reaper_fn_get_set_project_info(GetSetProjectInfo_fn fn);
void set_reaper_fn_count_selected_media_items(CountSelectedMediaItems_fn fn);
void set_reaper_fn_get_selected_media_item(GetSelectedMediaItem_fn fn);
void set_reaper_fn_get_media_item_track(GetMediaItemTrack_fn fn);

}
}
