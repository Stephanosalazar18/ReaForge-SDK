#include "main_thread_queue.h"

namespace reaforge {
namespace host {

void MainThreadQueue::enqueue(std::function<void()> fn) {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.emplace_back(std::move(fn));
    }
    cv_.notify_one();
}

bool MainThreadQueue::submit_void(std::function<void()> fn, std::chrono::milliseconds timeout) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.emplace_back([promise, fn = std::move(fn)]() mutable {
            try {
                fn();
                promise->set_value(true);
            } catch (...) {
                promise->set_value(false);
            }
        });
    }
    cv_.notify_one();
    if (future.wait_for(timeout) == std::future_status::ready) {
        return future.get();
    }
    return false;
}

std::size_t MainThreadQueue::drain_all() {
    std::deque<std::function<void()>> local;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        local.swap(queue_);
    }
    for (auto& fn : local) {
        try {
            fn();
        } catch (...) {
            // Swallow — main thread must not crash on a bad task.
        }
    }
    return local.size();
}

void MainThreadQueue::clear() {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.clear();
}

std::size_t MainThreadQueue::size() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.size();
}

MainThreadQueue& global_queue() {
    static MainThreadQueue instance;
    return instance;
}

} // namespace host
} // namespace reaforge
