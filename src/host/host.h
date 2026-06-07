#pragma once

#include <string>

namespace reaforge {

struct InvocationRequest {
    std::string id;
    std::string fn;
    std::string args;
};

struct InvocationResult {
    bool ok = false;
    std::string result;
    std::string error;
};

}
