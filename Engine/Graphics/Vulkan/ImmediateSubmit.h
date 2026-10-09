// One-off GPU work outside the frame loop (uploads at load time): record, submit, wait. Simple and blocking on purpose.
#pragma once

#include <functional>

#include <volk.h>

namespace ghost::graphics::vulkan {

class Device;

class ImmediateSubmit {
public:
    ImmediateSubmit() = default;
    ~ImmediateSubmit();
    ImmediateSubmit(const ImmediateSubmit&) = delete;
    ImmediateSubmit& operator=(const ImmediateSubmit&) = delete;

    void create(const Device& device);
    void destroy();
    // Records `record` into a fresh command buffer, submits it on the graphics queue and waits until it has finished.
    void run(const std::function<void(VkCommandBuffer)>& record);

private:
    const Device* m_device = nullptr;
    VkCommandPool m_pool = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;
};

} // namespace ghost::graphics::vulkan
