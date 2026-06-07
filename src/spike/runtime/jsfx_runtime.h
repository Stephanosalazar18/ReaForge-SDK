#pragma once

#include <string>

namespace reaforge {

class JSFXRuntime {
public:
    bool init();
    void shutdown();
    const char* strategy() const { return strategy_; }
    bool compile(const std::string& source, std::string& out_error);

private:
    const char* strategy_ = "uninitialized";
    bool compiled_ = false;
};

}
