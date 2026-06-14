#include "project_reader.h"
#include "api_reference_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace reaforge {
namespace host {

namespace {

// ---- function pointer storage (singleton; mutex-guarded) ----
struct ReaperFns {
    CountTracks_fn        count_tracks = nullptr;
    GetTrack_fn           get_track = nullptr;
    GetTrackName_fn       get_track_name = nullptr;
    TrackFX_GetCount_fn   track_fx_get_count = nullptr;
    TrackFX_GetFXName_fn  track_fx_get_fx_name = nullptr;
    GetProjectName_fn     get_project_name = nullptr;
    Master_GetTempo_fn    master_get_tempo = nullptr;
    GetSetProjectInfo_fn  get_set_project_info = nullptr;
    CountSelectedMediaItems_fn count_selected_media_items = nullptr;
    GetSelectedMediaItem_fn    get_selected_media_item = nullptr;
    GetMediaItemTrack_fn       get_media_item_track = nullptr;
};

ReaperFns& fns() {
    static ReaperFns instance;
    return instance;
}
std::mutex fns_mtx_;

template <typename T>
void set_fn(T& slot, T fn) {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    slot = fn;
}

// ---- list_artifacts helpers ----
std::string forward_slashes(const std::string& p) {
    std::string out;
    out.reserve(p.size());
    for (char c : p) {
        out.push_back(c == '\\' ? '/' : c);
    }
    return out;
}

struct FolderSpec {
    const char* subfolder;   // "Effects", "Scripts", "FXChains"
    const char* suffix;      // ".jsfx", ".lua", ".RfxChain"
    const char* kind;        // "jsfx", "lua", "fx_chain"
};

constexpr FolderSpec kFolders[] = {
    {"Effects",  ".jsfx",     "jsfx"},
    {"Scripts",  ".lua",      "lua"},
    {"FXChains", ".RfxChain", "fx_chain"},
};

void scan_folder(const fs::path& dir, const FolderSpec& spec,
                 std::vector<ArtifactEntry>& out) {
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        return;  // missing-folder tolerance: emit nothing
    }
    for (auto it = fs::directory_iterator(dir, ec);
         !ec && it != fs::directory_iterator();
         it.increment(ec)) {
        const auto& entry = *it;
        if (!entry.is_regular_file(ec)) continue;
        const std::string fname = entry.path().filename().string();
        // Case-insensitive suffix check (REAPER accepts .RFXCHAIN too).
        if (fname.size() < std::strlen(spec.suffix)) continue;
        std::string tail = fname.substr(fname.size() - std::strlen(spec.suffix));
        std::transform(tail.begin(), tail.end(), tail.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::string want = spec.suffix;
        std::transform(want.begin(), want.end(), want.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (tail != want) continue;

        ArtifactEntry e;
        std::error_code ec2;
        auto abs = fs::absolute(entry.path(), ec2);
        e.path = forward_slashes(abs.string());
        e.size = static_cast<std::size_t>(fs::file_size(entry.path(), ec2));
        auto ftime = fs::last_write_time(entry.path(), ec2);
        // Convert file_time_type to seconds since epoch (best-effort).
        // Using std::chrono::clock_cast where available; fall back to 0 on error.
        try {
            using clock = std::chrono::system_clock;
            auto sctp = std::chrono::time_point_cast<clock::duration>(
                ftime - fs::file_time_type::clock::now() + clock::now());
            e.mtime = std::chrono::duration_cast<std::chrono::seconds>(
                sctp.time_since_epoch()).count();
        } catch (...) {
            e.mtime = 0;
        }
        e.kind = spec.kind;
        out.push_back(std::move(e));
    }
}

// ---- get_api_reference helpers ----
ApiReferenceResult lookup_api_reference(const std::string& target) {
    ApiReferenceResult r;
    if (target.empty()) {
        r.error_code = "MISSING_TARGET";
        r.error_message = "target query parameter is required";
        return r;
    }
    const auto& m = api_reference_map();
    auto it = m.find(target);
    if (it == m.end()) {
        r.error_code = "INVALID_TARGET";
        r.error_message = "unknown target: " + target;
        return r;
    }
    r.ok = true;
    r.target = target;
    r.reference = it->second;
    return r;
}

// ---- get_state helpers ----

}

ListArtifactsResult list_artifacts(const std::string& base_dir,
                                    const std::string& kind_filter) {
    ListArtifactsResult out;
    // Validate kind_filter (empty = all).
    if (!kind_filter.empty()) {
        bool ok = false;
        for (const auto& s : kFolders) {
            if (kind_filter == s.kind) { ok = true; break; }
        }
        if (!ok) {
            out.invalid_kind = true;
            out.invalid_kind_value = kind_filter;
            return out;
        }
    }
    fs::path base(base_dir);
    for (const auto& s : kFolders) {
        if (!kind_filter.empty() && kind_filter != s.kind) continue;
        scan_folder(base / s.subfolder / "ReaForge", s, out.entries);
    }
    return out;
}

ApiReferenceResult get_api_reference(const std::string& target) {
    return lookup_api_reference(target);
}

GetStateResult get_state(bool summary) {
    GetStateResult r;
    // Snapshot function pointers (no lock held during REAPER calls).
    CountTracks_fn        p_count_tracks;
    GetTrack_fn           p_get_track;
    GetTrackName_fn       p_get_track_name;
    TrackFX_GetCount_fn   p_track_fx_get_count;
    TrackFX_GetFXName_fn  p_track_fx_get_fx_name;
    GetProjectName_fn     p_get_project_name;
    Master_GetTempo_fn    p_master_get_tempo;
    GetSetProjectInfo_fn  p_get_set_project_info;
    CountSelectedMediaItems_fn p_count_sel_items;
    GetSelectedMediaItem_fn    p_get_sel_item;
    GetMediaItemTrack_fn       p_get_item_track;
    {
        std::lock_guard<std::mutex> lock(fns_mtx_);
        p_count_tracks = fns().count_tracks;
        p_get_track = fns().get_track;
        p_get_track_name = fns().get_track_name;
        p_track_fx_get_count = fns().track_fx_get_count;
        p_track_fx_get_fx_name = fns().track_fx_get_fx_name;
        p_get_project_name = fns().get_project_name;
        p_master_get_tempo = fns().master_get_tempo;
        p_get_set_project_info = fns().get_set_project_info;
        p_count_sel_items = fns().count_selected_media_items;
        p_get_sel_item = fns().get_selected_media_item;
        p_get_item_track = fns().get_media_item_track;
    }

    // If any pointer is null, REAPER isn't initialized. Return error.
    if (!p_count_tracks || !p_get_track || !p_get_track_name ||
        !p_track_fx_get_count || !p_track_fx_get_fx_name ||
        !p_get_project_name || !p_master_get_tempo ||
        !p_get_set_project_info || !p_count_sel_items ||
        !p_get_sel_item || !p_get_item_track) {
        r.error_code = "REAPER_NOT_AVAILABLE";
        r.error_message = "REAPER function pointers not initialized";
        return r;
    }

    // Project name.
    char name_buf[256] = {0};
    p_get_project_name(nullptr, name_buf, sizeof(name_buf));
    std::string project_name = name_buf;

    // BPM.
    double bpm = p_master_get_tempo();

    // Sample rate (PROJECT_SRATE).
    double srate = p_get_set_project_info(nullptr, "PROJECT_SRATE", 0.0, false);

    // Track enumeration.
    int n_tracks = p_count_tracks(nullptr);
    if (n_tracks < 0) n_tracks = 0;

    // Pre-compute per-track selected-item counts.
    // We need to know which track owns each selected item. O(n_sel * n_tracks)
    // is fine for MVP scale (hundreds of items, dozens of tracks).
    int n_sel_items = p_count_sel_items(nullptr);
    std::vector<int> per_track_sel(n_tracks, 0);
    for (int s = 0; s < n_sel_items; ++s) {
        void* item = p_get_sel_item(nullptr, s);
        if (!item) continue;
        void* tr = p_get_item_track(item);
        if (!tr) continue;
        // Identify which track index tr is by comparing pointers.
        for (int t = 0; t < n_tracks; ++t) {
            if (p_get_track(nullptr, t) == tr) {
                per_track_sel[t]++;
                break;
            }
        }
    }

    nlohmann::json j;
    j["project_name"] = project_name;
    j["sample_rate"] = static_cast<int>(srate);
    j["bpm"] = bpm;
    j["tracks"] = nlohmann::json::array();

    for (int t = 0; t < n_tracks; ++t) {
        void* tr = p_get_track(nullptr, t);
        if (!tr) continue;
        char tname[256] = {0};
        p_get_track_name(tr, tname, sizeof(tname));
        int fx_count = p_track_fx_get_count(tr);

        nlohmann::json tj;
        tj["id"] = t;
        tj["name"] = tname;
        tj["selected"] = (per_track_sel[t] > 0);
        tj["fx_count"] = fx_count;
        if (!summary) {
            nlohmann::json fx_names = nlohmann::json::array();
            // Cap at 16 per track per R3 mitigation in the design.
            int cap = std::min(fx_count, 16);
            for (int f = 0; f < cap; ++f) {
                char fname[256] = {0};
                p_track_fx_get_fx_name(tr, f, fname, sizeof(fname));
                fx_names.push_back(fname);
            }
            tj["fx_names"] = fx_names;
        }
        tj["selected_item_count"] = per_track_sel[t];
        j["tracks"].push_back(std::move(tj));
    }
    j["selected_items_count"] = n_sel_items;

    r.ok = true;
    r.json = j.dump();
    return r;
}

void reset_reaper_function_pointers() {
    std::lock_guard<std::mutex> lock(fns_mtx_);
    fns() = ReaperFns{};
}

void set_reaper_fn_count_tracks(CountTracks_fn fn)             { set_fn(fns().count_tracks, fn); }
void set_reaper_fn_get_track(GetTrack_fn fn)                   { set_fn(fns().get_track, fn); }
void set_reaper_fn_get_track_name(GetTrackName_fn fn)          { set_fn(fns().get_track_name, fn); }
void set_reaper_fn_track_fx_get_count(TrackFX_GetCount_fn fn)  { set_fn(fns().track_fx_get_count, fn); }
void set_reaper_fn_track_fx_get_fx_name(TrackFX_GetFXName_fn fn) { set_fn(fns().track_fx_get_fx_name, fn); }
void set_reaper_fn_get_project_name(GetProjectName_fn fn)      { set_fn(fns().get_project_name, fn); }
void set_reaper_fn_master_get_tempo(Master_GetTempo_fn fn)      { set_fn(fns().master_get_tempo, fn); }
void set_reaper_fn_get_set_project_info(GetSetProjectInfo_fn fn) { set_fn(fns().get_set_project_info, fn); }
void set_reaper_fn_count_selected_media_items(CountSelectedMediaItems_fn fn) { set_fn(fns().count_selected_media_items, fn); }
void set_reaper_fn_get_selected_media_item(GetSelectedMediaItem_fn fn)       { set_fn(fns().get_selected_media_item, fn); }
void set_reaper_fn_get_media_item_track(GetMediaItemTrack_fn fn)             { set_fn(fns().get_media_item_track, fn); }

}
}
