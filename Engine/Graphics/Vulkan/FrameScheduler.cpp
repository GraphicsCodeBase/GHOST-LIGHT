// Frames-in-flight bookkeeping with a timeline semaphore (no per-frame fences) and vkQueueSubmit2.
#include "Graphics/Vulkan/FrameScheduler.h"

#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <string>

namespace ghost::graphics::vulkan {

FrameScheduler::~FrameScheduler() {
    destroy();
}

bool FrameScheduler::create(const Device& device) {
    m_device = &device;
    const VkDevice vk = device.handle();

    VkSemaphoreTypeCreateInfo timelineType{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    timelineType.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineType.initialValue = 0;
    VkSemaphoreCreateInfo timelineInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    timelineInfo.pNext = &timelineType;
    VK_CHECK(vkCreateSemaphore(vk, &timelineInfo, nullptr, &m_timeline));
    DebugUtils::setName(m_timeline, "FrameTimeline");

    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        Slot& slot = m_slots[i];
        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = device.graphicsQueueFamily();
        VK_CHECK(vkCreateCommandPool(vk, &poolInfo, nullptr, &slot.pool));

        VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = slot.pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(vk, &allocInfo, &slot.cmd));

        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(vk, &semaphoreInfo, nullptr, &slot.imageAvailable));

        const std::string index = std::to_string(i);
        DebugUtils::setName(slot.pool, "Frame" + index + ".CommandPool");
        DebugUtils::setName(slot.cmd, "Frame" + index + ".CommandBuffer");
        DebugUtils::setName(slot.imageAvailable, "Frame" + index + ".ImageAvailable");
    }
    return true;
}

VkCommandBuffer FrameScheduler::beginFrame() {
    Slot& slot = m_slots[m_slot];
    if (slot.submittedValue > 0) {
        VkSemaphoreWaitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_timeline;
        waitInfo.pValues = &slot.submittedValue;
        VK_CHECK(vkWaitSemaphores(m_device->handle(), &waitInfo, UINT64_MAX));
    }
    VK_CHECK(vkResetCommandPool(m_device->handle(), slot.pool, 0));

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(slot.cmd, &beginInfo));
    m_recording = true;
    return slot.cmd;
}

void FrameScheduler::submit(VkSemaphore waitImageAvailable, VkSemaphore signalRenderFinished) {
    Slot& slot = m_slots[m_slot];
    VK_CHECK(vkEndCommandBuffer(slot.cmd));
    m_recording = false;

    const uint64_t signalValue = m_frameNumber + 1;

    VkSemaphoreSubmitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    waitInfo.semaphore = waitImageAvailable;
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT;

    VkSemaphoreSubmitInfo signalInfos[2]{};
    uint32_t signalCount = 0;
    signalInfos[signalCount] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signalInfos[signalCount].semaphore = m_timeline;
    signalInfos[signalCount].value = signalValue;
    signalInfos[signalCount].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    ++signalCount;
    if (signalRenderFinished != VK_NULL_HANDLE) {
        signalInfos[signalCount] = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalInfos[signalCount].semaphore = signalRenderFinished;
        signalInfos[signalCount].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        ++signalCount;
    }

    VkCommandBufferSubmitInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdInfo.commandBuffer = slot.cmd;

    VkSubmitInfo2 submit{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.waitSemaphoreInfoCount = waitImageAvailable != VK_NULL_HANDLE ? 1u : 0u;
    submit.pWaitSemaphoreInfos = &waitInfo;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;
    submit.signalSemaphoreInfoCount = signalCount;
    submit.pSignalSemaphoreInfos = signalInfos;
    VK_CHECK(vkQueueSubmit2(m_device->graphicsQueue(), 1, &submit, VK_NULL_HANDLE));

    slot.submittedValue = signalValue;
    m_frameNumber = signalValue;
    m_slot = (m_slot + 1) % kFramesInFlight;
}

void FrameScheduler::cancelFrame() {
    if (m_recording) {
        VK_CHECK(vkEndCommandBuffer(m_slots[m_slot].cmd));
        m_recording = false;
    }
}

uint64_t FrameScheduler::completedFrame() const {
    uint64_t value = 0;
    VK_CHECK(vkGetSemaphoreCounterValue(m_device->handle(), m_timeline, &value));
    return value;
}

void FrameScheduler::destroy() {
    if (!m_device) {
        return;
    }
    const VkDevice vk = m_device->handle();
    for (Slot& slot : m_slots) {
        if (slot.imageAvailable) vkDestroySemaphore(vk, slot.imageAvailable, nullptr);
        if (slot.pool) vkDestroyCommandPool(vk, slot.pool, nullptr);
        slot = Slot{};
    }
    if (m_timeline) {
        vkDestroySemaphore(vk, m_timeline, nullptr);
        m_timeline = VK_NULL_HANDLE;
    }
    m_device = nullptr;
}

} // namespace ghost::graphics::vulkan
