// ShaderBindingTable: group handles copied into an aligned, host-visible buffer (one region per shader kind).
#include "Graphics/RayTracing/ShaderBindingTable.h"

#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <cstring>
#include <utility>
#include <vector>

namespace ghost::graphics::raytracing {

namespace {

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

void ShaderBindingTable::build(const vulkan::Device& device, VkPipeline pipeline, uint32_t missCount, uint32_t hitGroupCount,
                               const char* name) {
    destroy();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& properties = device.rayTracingProperties();
    const VkDeviceSize handleSize = properties.shaderGroupHandleSize;
    const VkDeviceSize handleStride = alignUp(handleSize, properties.shaderGroupHandleAlignment);
    const VkDeviceSize baseAlignment = properties.shaderGroupBaseAlignment;

    const uint32_t groupCount = 1 + missCount + hitGroupCount;
    std::vector<uint8_t> handles(groupCount * handleSize);
    VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(device.handle(), pipeline, 0, groupCount, handles.size(), handles.data()));

    // Each region starts on the base alignment; the ray generation region's size must equal its stride.
    const VkDeviceSize rayGenerationSize = alignUp(handleStride, baseAlignment);
    const VkDeviceSize missSize = alignUp(missCount * handleStride, baseAlignment);
    const VkDeviceSize hitSize = alignUp(hitGroupCount * handleStride, baseAlignment);
    const VkDeviceSize total = rayGenerationSize + missSize + hitSize;

    // Extra room so the table can start on the base alignment whatever address the allocation gets.
    m_buffer.create(device, {total + baseAlignment, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, vulkan::GpuBuffer::Memory::Upload, name});
    const VkDeviceAddress start = alignUp(m_buffer.address(), baseAlignment);
    uint8_t* mapped = static_cast<uint8_t*>(m_buffer.mapped()) + (start - m_buffer.address());

    uint32_t group = 0;
    auto writeRegion = [&](VkDeviceSize offset, uint32_t count) {
        for (uint32_t i = 0; i < count; ++i, ++group) {
            std::memcpy(mapped + offset + i * handleStride, handles.data() + group * handleSize, handleSize);
        }
    };
    writeRegion(0, 1);
    writeRegion(rayGenerationSize, missCount);
    writeRegion(rayGenerationSize + missSize, hitGroupCount);

    m_rayGeneration = {start, rayGenerationSize, rayGenerationSize};
    m_miss = missCount ? VkStridedDeviceAddressRegionKHR{start + rayGenerationSize, handleStride, missSize} : VkStridedDeviceAddressRegionKHR{};
    m_hit = hitGroupCount ? VkStridedDeviceAddressRegionKHR{start + rayGenerationSize + missSize, handleStride, hitSize}
                          : VkStridedDeviceAddressRegionKHR{};
    m_callable = {};
}

vulkan::GpuBuffer ShaderBindingTable::release() {
    m_rayGeneration = m_miss = m_hit = m_callable = {};
    return std::move(m_buffer);
}

void ShaderBindingTable::destroy() {
    m_buffer.destroy();
    m_rayGeneration = m_miss = m_hit = m_callable = {};
}

} // namespace ghost::graphics::raytracing
