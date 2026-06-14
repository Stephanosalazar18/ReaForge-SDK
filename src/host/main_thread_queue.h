#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

namespace reaforge {
namespace host {

// Marshals work onto a consumer thread (e.g. REAPER's main loop).
// Pure C++17; no REAPER dep, fully unit-testable.
class MainThreadQueue {
public:
    // Fire-and-forget enqueue. Returns immediately. The consumer will
    // eventually invoke `fn` and discard the result.
    void enqueue(std::function<void()> fn);

    // Enqueue and block until `fn` completes (or timeout elapses).
    // For non-void F: returns std::nullopt on timeout, else the value.
    template <typename F>
    auto submit(F&& fn, std::chrono::milliseconds timeout)
        -> std::optional<std::invoke_result_t<F>>;

    // Convenience overload for void-returning callables.
    // Returns true if completed within timeout, false on timeout.
    bool submit_void(std::function<void()> fn, std::chrono::milliseconds timeout);

    // Drain all pending tasks. Called by the consumer thread (REAPER main loop).
    // Returns the number of tasks executed.
    std::size_t drain_all();

    // For tests / shutdown: clear all pending tasks without running them.
    void clear();

    // Number of tasks waiting in the queue.
    std::size_t size() const;

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> queue_;
};

}
}

namespace reaforge {
namespace host {

template <typename F>
auto MainThreadQueue::submit(F&& fn, std::chrono::milliseconds timeout)
    -> std::optional<std::invoke_result_t<F>> {
    using R = std::invoke_result_t<F>;

    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();

    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.emplace_back([promise, fn = std::forward<F>(fn)]() mutable {
            try {
                if constexpr (std::is_void_v<R>) {
                    fn();
                    promise->set_value();
                } else {
                    promise->set_value(fn());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
    }
    cv_.notify_one();

    if (future.wait_for(timeout) == std::future_status::ready) {
        if constexpr (std::is_void_v<R>) {
            future.get();
            return std::optional<R>{};  // empty optional
        } else {
            return future.get();
        }
    }
    return std::nullopt;
}

// Global convenience functions for the MVP host. In production, the queue is
// drained by REAPER's main thread via a timer or hook. For the MVP, the HTTP
// handler thread calls suspend-and-drain directly for runtime-reaper calls.
MainThreadQueue& global_queue();

inline void shutdown() {
    global_queue().clear();
}

inline void start_polling() {
    // In a real REAPER extension, this would register a timer to call
    // drain_all() on the main thread. For the MVP, the HTTP handler thread
    // drains synchronously when needed.
}

} // namespace host
} // namespace reaforge
