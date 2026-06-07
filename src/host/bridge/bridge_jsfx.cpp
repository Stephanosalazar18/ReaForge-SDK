#include "../runtime/jsfx_runtime.h"

#include <string>

namespace reaforge {

const char* jsfx_bridge_unsupported() {
    return "spike: bridge to JSFX is not implemented (JSFX has no general function-call surface)";
}

}
