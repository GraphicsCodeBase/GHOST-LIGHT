// Timestamp query pools per frame slot, host-reset (Vulkan 1.2 hostQueryReset), read back without stalling.
#include "Graphics/RenderGraph/GpuTimers.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <algorithm>

namespace ghost::graphics::rendergraph {

GpuTimers::~GpuTimers() {
    destroy();
}

void GpuTimers::create(const vulkan::Device& device, uint32_t framesInFlight) {
    m_device = &device;
    m_slotCount = std::min(framesInFlight, kMaxFrames);
    m_nanosecondsPerTick = device.timestampPeriod();

    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.physicalDevice(), &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device.physicalDevice(), &familyCount, families.data());
    m_supported = families[device.graphicsQueueFamily()].timestampValidBits > 0 && m_nanosecondsPerTick > 0.0f;
    if (!m_supported) {
        core::Log::warning("GPU timestamps are not supported on this queue; pass timings are disabled");
        return;
    }

    for (uint32_t i = 0; i < m_slotCount; ++i) {
        VkQueryPoolCreateInfo info{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = kMaxScopes * 2;
        VK_CHECK(vkCreateQueryPool(device.handle(), &info, nullptr, &m_slots[i].pool));
        vkResetQueryPool(device.handle(), m_slots[i].pool, 0, kMaxScopes * 2);
        vulkan::DebugUtils::setName(m_slots[i].pool, "GpuTimers." + std::to_string(i));
    }
}

void GpuTimers::destroy() {
    if (!m_device) {
        return;
    }
    for (Slot& slot : m_slots) {
        if (slot.pool) {
            vkDestroyQueryPool(m_device->handle(), slot.pool, nullptr);
        }
        slot = Slot{};
    }
    m_device = nullptr;
}

void GpuTimers::beginFrame(uint32_t slotIndex) {
    if (!m_supported) {
        return;
    }
    m_current = slotIndex % m_slotCount;
    Slot& slot = m_slots[m_current];
    const uint32_t count = static_cast<uint32_t>(slot.names.size());
    if (count > 0) {
        std::vector<uint64_t> ticks(count * 2);
        const VkResult result = vkGetQueryPoolResults(m_device->handle(), slot.pool, 0, count * 2, ticks.size() * sizeof(uint64_t),
                                                      ticks.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
        if (result == VK_SUCCESS) {
            m_results.clear();
            for (uint32_t i = 0; i < count; ++i) {
                const double ms = static_cast<double>(ticks[i * 2 + 1] - ticks[i * 2]) * m_nanosecondsPerTick * 1e-6;
                m_results.push_back({slot.names[i], ms});
            }
            m_totalMilliseconds = static_cast<double>(ticks[count * 2 - 1] - ticks[0]) * m_nanosecondsPerTick * 1e-6;
        }
        vkResetQueryPool(m_device->handle(), slot.pool, 0, count * 2);
    }
    slot.names.clear();
}

uint32_t GpuTimers::begin(VkCommandBuffer cmd, const std::string& name) {
    Slot& slot = m_slots[m_current];
    if (!m_supported || slot.names.size() >= kMaxScopes) {
        return UINT32_MAX;
    }
    const uint32_t scope = static_cast<uint32_t>(slot.names.size());
    slot.names.push_back(name);
    vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, slot.pool, scope * 2);
    return scope;
}

void GpuTimers::end(VkCommandBuffer cmd, uint32_t scope) {
    if (scope == UINT32_MAX) {
        return;
    }
    vkCmdWriteTimestamp2(cmd, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_slots[m_current].pool, scope * 2 + 1);
}

} // namespace ghost::graphics::rendergraph
