// Deferred destruction: GPU objects replaced mid-run (hot-reloaded pipelines, resized images) are destroyed once
// every frame that might still use them has finished on the GPU.
#pragma once

#include <cstdint>
#include <deque>
#include <functional>

namespace ghost::graphics::vulkan {

class DeletionQueue {
public:
    DeletionQueue() = default;
    ~DeletionQueue();
    DeletionQueue(const DeletionQueue&) = delete;
    DeletionQueue& operator=(const DeletionQueue&) = delete;

    // Runs `destroy` once the GPU has completed frame `lastFrameUsingIt` (a FrameScheduler timeline value).
    void push(uint64_t lastFrameUsingIt, std::function<void()> destroy);
    // Runs every entry whose frame has completed.
    void flush(uint64_t completedFrame);
    // Runs everything now (call after vkDeviceWaitIdle).
    void flushAll();

private:
    struct Entry {
        uint64_t frame;
        std::function<void()> destroy;
    };
    std::deque<Entry> m_entries;
};

} // namespace ghost::graphics::vulkan
