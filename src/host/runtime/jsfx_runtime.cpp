#include "jsfx_runtime.h"

#include <string>

extern "C" int reaper_jsfx_compile(const char* in_source, char* out_error, int out_error_size);

namespace reaforge {

bool JSFXRuntime::init() {
    strategy_ = "stub: reaper_jsfx_compile not linked in spike build";
    return true;
}

void JSFXRuntime::shutdown() {
    compiled_ = false;
}

bool JSFXRuntime::compile(const std::string& source, std::string& out_error) {
    if (source.empty()) {
        out_error = "jsfx: empty source";
        return false;
    }
    compiled_ = true;
    return true;
}

}
