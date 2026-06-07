#pragma once

#include <string>

namespace reaforge {
namespace host {
namespace panel {

bool create();
void destroy();
void render();
void on_reload(const std::string& extension_id);

}
}
}
