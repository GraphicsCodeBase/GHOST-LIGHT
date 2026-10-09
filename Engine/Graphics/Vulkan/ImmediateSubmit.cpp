// ImmediateSubmit: a transient command pool and a fence; the pool is reset after every run.
#include "Graphics/Vulkan/ImmediateSubmit.h"

#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

namespace ghost::graphics::vulkan {

ImmediateSubmit::~ImmediateSubmit() {
    destroy();
}

void ImmediateSubmit::create(const Device& device) {
    m_device = &device;
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = device.graphicsQueueFamily();
    VK_CHECK(vkCreateCommandPool(device.handle(), &poolInfo, nullptr, &m_pool));
    DebugUtils::setName(m_pool, "ImmediateSubmit.Pool");
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VK_CHECK(vkCreateFence(device.handle(), &fenceInfo, nullptr, &m_fence));
    DebugUtils::setName(m_fence, "ImmediateSubmit.Fence");
}

void ImmediateSubmit::destroy() {
    if (!m_device) {
        return;
    }
    vkDestroyFence(m_device->handle(), m_fence, nullptr);
    vkDestroyCommandPool(m_device->handle(), m_pool, nullptr);
    m_device = nullptr;
}

void ImmediateSubmit::run(const std::function<void(VkCommandBuffer)>& record) {
    const VkDevice device = m_device->handle();
    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = m_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &cmd));

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
    record(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdInfo.commandBuffer = cmd;
    VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;
    VK_CHECK(vkResetFences(device, 1, &m_fence));
    VK_CHECK(vkQueueSubmit2(m_device->graphicsQueue(), 1, &submit, m_fence));
    VK_CHECK(vkWaitForFences(device, 1, &m_fence, VK_TRUE, UINT64_MAX));
    VK_CHECK(vkResetCommandPool(device, m_pool, 0));
}

} // namespace ghost::graphics::vulkan
