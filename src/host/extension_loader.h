#pragma once

#include <string>
#include <vector>

namespace reaforge {
namespace host {
namespace loader {

struct Manifest {
    std::string id;
    std::string name;
    std::string runtime;
    std::string target;
    std::string entry;
    std::string source_path;
};

extern std::string g_extensions_dir_override;

const std::string& extensions_dir();
std::vector<Manifest> scan();
bool reload(const std::string& id);

}
}
}
