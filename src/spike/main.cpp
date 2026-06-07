#include "executor.h"
#include "bridge/bridge.h"

#include <iostream>
#include <string>

namespace reaforge {

int run_spike_main() {
    Bridge::install_main_thread_id();
    Executor exec;
    if (!exec.init()) {
        std::cerr << "host: executor init failed\n";
        return 1;
    }
    std::cout << "host: lua strategy: lua 5.4\n";
    std::cout << "host: quickjs strategy: quickjs-ng\n";
    std::cout << "host: jsfx strategy: " << exec.jsfx().strategy() << "\n";
    exec.shutdown();
    return 0;
}

}

int main() {
    return reaforge::run_spike_main();
}
