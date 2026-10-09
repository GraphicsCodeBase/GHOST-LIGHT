// Frames in flight: per-frame command buffers, plus one timeline semaphore that tells the CPU when a frame's GPU work is done.
#pragma once

#include <volk.h>

#include <array>
#include <cstdint>

namespace ghost::graphics::vulkan {

class Device;

class FrameScheduler {
public:
    // Two frames in flight: the CPU records frame N+1 while the GPU renders frame N.
    static constexpr uint32_t kFramesInFlight = 2;

    FrameScheduler() = default;
    ~FrameScheduler();
    FrameScheduler(const FrameScheduler&) = delete;
    FrameScheduler& operator=(const FrameScheduler&) = delete;

    bool create(const Device& device);
    void destroy();

    // Waits until the GPU has finished the last frame recorded in this slot, then returns its command buffer, begun.
    VkCommandBuffer beginFrame();
    // Semaphore the swapchain signals when this frame's image is ready to be drawn into.
    VkSemaphore imageAvailable() const { return m_slots[m_slot].imageAvailable; }
    // Ends recording and submits. Waits on imageAvailable (if not null) before writing color output,
    // signals renderFinished (if not null) for present, and always advances the timeline.
    void submit(VkSemaphore waitImageAvailable, VkSemaphore signalRenderFinished);
    // Abandons the current frame without submitting (e.g. the swapchain went out of date during acquire).
    void cancelFrame();

    uint32_t slot() const { return m_slot; }
    uint64_t frameNumber() const { return m_frameNumber; }  // frames submitted so far
    // Timeline value the GPU has reached: frame N is complete once completedFrame() >= N.
    uint64_t completedFrame() const;
    VkSemaphore timeline() const { return m_timeline; }

private:
    struct Slot {
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        uint64_t submittedValue = 0; // timeline value signalled by this slot's last submit
    };

    const Device* m_device = nullptr;
    std::array<Slot, kFramesInFlight> m_slots{};
    VkSemaphore m_timeline = VK_NULL_HANDLE;
    uint64_t m_frameNumber = 0;
    uint32_t m_slot = 0;
    bool m_recording = false;
};

} // namespace ghost::graphics::vulkan
