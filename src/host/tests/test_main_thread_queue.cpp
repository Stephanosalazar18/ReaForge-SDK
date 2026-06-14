// Test the main_thread_queue. No REAPER dep. Pure C++ threading.
#include "main_thread_queue.h"

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

using namespace reaforge::host;
using namespace std::chrono_literals;

static int rc_ = 0;

static void check(bool cond, const char* what) {
    if (cond) {
        std::fprintf(stderr, "OK: %s\n", what);
    } else {
        std::fprintf(stderr, "FAIL: %s\n", what);
        rc_ = 1;
    }
}

static void test_fire_and_forget_runs() {
    MainThreadQueue q;
    int counter = 0;
    q.enqueue([&counter]() { counter = 1; });
    std::size_t n = q.drain_all();
    check(n == 1, "drain_all returns count of executed tasks");
    check(counter == 1, "fire-and-forget task actually ran");
}

static void test_submit_returns_value() {
    MainThreadQueue q;
    std::thread consumer([&q]() {
        // Simulate main loop: drain periodically.
        for (int i = 0; i < 20; ++i) {
            q.drain_all();
            std::this_thread::sleep_for(10ms);
        }
    });
    auto result = q.submit([]() { return 42; }, 2000ms);
    check(result.has_value(), "submit returns value within timeout");
    check(result.has_value() && *result == 42, "submitted value is correct");
    consumer.join();
}

static void test_submit_void() {
    MainThreadQueue q;
    bool ran = false;
    std::thread consumer([&q, &ran]() {
        for (int i = 0; i < 20; ++i) {
            q.drain_all();
            std::this_thread::sleep_for(10ms);
            if (ran) break;
        }
    });
    auto result = q.submit_void([&ran]() { ran = true; }, 2000ms);
    check(result, "submit_void returns true on completion");
    check(ran, "void task actually ran");
    consumer.join();
}

static void test_drain_empty() {
    MainThreadQueue q;
    check(q.drain_all() == 0, "drain_all on empty queue returns 0");
    check(q.size() == 0, "size() reports 0 on empty queue");
}

static void test_clear_drops_pending() {
    MainThreadQueue q;
    q.enqueue([]() { /* never runs */ });
    check(q.size() == 1, "queue size 1 after enqueue");
    q.clear();
    check(q.size() == 0, "queue size 0 after clear");
    check(q.drain_all() == 0, "drain_all returns 0 after clear");
}

int main() {
    std::fprintf(stderr, "test_main_thread_queue: starting\n");
    test_fire_and_forget_runs();
    test_submit_returns_value();
    test_submit_void();
    test_drain_empty();
    test_clear_drops_pending();
    std::fprintf(stderr, "test_main_thread_queue: %s\n", rc_ == 0 ? "PASS" : "FAIL");
    return rc_;
}
