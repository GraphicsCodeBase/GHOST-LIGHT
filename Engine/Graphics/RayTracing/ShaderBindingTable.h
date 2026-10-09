// Shader binding table: the shader group handles of one ray tracing pipeline laid out in the raygen / miss / hit
// regions vkCmdTraceRaysKHR reads (no per-record data: shaders reach everything through the frame constants).
#pragma once

#include "Graphics/Vulkan/GpuBuffer.h"

#include <cstdint>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
}

namespace ghost::graphics::raytracing {

class ShaderBindingTable {
public:
    // Pipeline groups must be ordered: ray generation, `missCount` misses, `hitGroupCount` hit groups.
    void build(const vulkan::Device& device, VkPipeline pipeline, uint32_t missCount, uint32_t hitGroupCount, const char* name);
    void destroy();
    bool valid() const { return m_buffer.valid(); }

    const VkStridedDeviceAddressRegionKHR& rayGeneration() const { return m_rayGeneration; }
    const VkStridedDeviceAddressRegionKHR& miss() const { return m_miss; }
    const VkStridedDeviceAddressRegionKHR& hit() const { return m_hit; }
    const VkStridedDeviceAddressRegionKHR& callable() const { return m_callable; }

    // Releases the buffer to the caller (deferred destruction while frames in flight still use it).
    vulkan::GpuBuffer release();

private:
    vulkan::GpuBuffer m_buffer;
    VkStridedDeviceAddressRegionKHR m_rayGeneration{};
    VkStridedDeviceAddressRegionKHR m_miss{};
    VkStridedDeviceAddressRegionKHR m_hit{};
    VkStridedDeviceAddressRegionKHR m_callable{};
};

} // namespace ghost::graphics::raytracing
