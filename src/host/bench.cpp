#include "executor.h"
#include "bridge/bridge.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace reaforge {

namespace {

struct Percentiles {
    double p50 = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
};

Percentiles compute_percentiles(std::vector<double> us_sorted) {
    Percentiles p;
    if (us_sorted.empty()) return p;
    auto pick = [&](double q) {
        size_t idx = static_cast<size_t>(q * (us_sorted.size() - 1));
        return us_sorted[idx];
    };
    p.p50 = pick(0.50);
    p.p95 = pick(0.95);
    p.p99 = pick(0.99);
    return p;
}

std::vector<double> time_invokes(Executor& exec, const std::string& id, const std::string& fn, int n) {
    std::vector<double> us;
    us.reserve(n);
    InvocationRequest req{id, fn, ""};
    for (int i = 0; i < n; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        auto r = exec.invoke(req);
        auto t1 = std::chrono::steady_clock::now();
        (void)r;
        double us_count = std::chrono::duration<double, std::micro>(t1 - t0).count();
        us.push_back(us_count);
    }
    return us;
}

const char* verdict(double p95_us, double budget_us) {
    return p95_us <= budget_us ? "PASS" : "FAIL";
}

int run_bench_main(int argc, char** argv) {
    int n = 1000;
    double budget_us = 5000.0;
    std::string out_path = "spike-results.md";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
        else if (a.rfind("--budget-us=", 0) == 0) budget_us = std::atof(a.c_str() + 13);
        else if (a.rfind("--out=", 0) == 0) out_path = a.substr(6);
    }

    Bridge::install_main_thread_id();
    Executor exec;
    if (!exec.init()) {
        std::cerr << "bench: executor init failed\n";
        return 1;
    }

    auto warmup = time_invokes(exec, "lua-demo", "1+1", 50);
    (void)warmup;

    auto lua_us = time_invokes(exec, "lua-demo", "1+1", n);
    std::sort(lua_us.begin(), lua_us.end());
    auto lua_p = compute_percentiles(lua_us);

    auto js_us = time_invokes(exec, "js-demo", "1+1", n);
    std::sort(js_us.begin(), js_us.end());
    auto js_p = compute_percentiles(js_us);

    auto jsfx_us = time_invokes(exec, "jsfx-demo", "noop", n);
    std::sort(jsfx_us.begin(), jsfx_us.end());
    auto jsfx_p = compute_percentiles(jsfx_us);

    exec.shutdown();

    bool pass = (lua_p.p95 <= budget_us)
             && (js_p.p95 <= budget_us)
             && (jsfx_p.p95 <= budget_us);

    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "bench: cannot open " << out_path << "\n";
        return 1;
    }
    out << "# ReaForge Spike Results\n\n";
    out << "## Throughput (lower is better)\n\n";
    out << "| Runtime | p50 (us) | p95 (us) | p99 (us) | Budget (us) | Verdict |\n";
    out << "|---|---|---|---|---|---|\n";
    out << "| Lua 5.4 | " << lua_p.p50 << " | " << lua_p.p95 << " | " << lua_p.p99
        << " | " << budget_us << " | " << verdict(lua_p.p95, budget_us) << " |\n";
    out << "| QuickJS-ng | " << js_p.p50 << " | " << js_p.p95 << " | " << js_p.p99
        << " | " << budget_us << " | " << verdict(js_p.p95, budget_us) << " |\n";
    out << "| JSFX (stub) | " << jsfx_p.p50 << " | " << jsfx_p.p95 << " | " << jsfx_p.p99
        << " | " << budget_us << " | " << verdict(jsfx_p.p95, budget_us) << " |\n\n";
    out << "## Integration Findings\n\n";
    out << "- Lua 5.4 linked via system liblua5.4-dev.\n";
    out << "- QuickJS-ng vendored as a git submodule at `third_party/quickjs-ng/`.\n";
    out << "- JSFX runtime is a stub; `reaper_jsfx_compile` is not linked in the spike build.\n";
    out << "- REAPER C++ SDK headers vendored at `third_party/reaper-sdk/` but not yet linked.\n\n";
    out << "## Verdict\n\n";
    out << (pass ? "**GO**" : "**NO-GO**") << "\n";
    out.close();

    std::cout << "Wrote " << out_path << "\n";
    return pass ? 0 : 1;
}

}

int main(int argc, char** argv) {
    return reaforge::run_bench_main(argc, argv);
}
