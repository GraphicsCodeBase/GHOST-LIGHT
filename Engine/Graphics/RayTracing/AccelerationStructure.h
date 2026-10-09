// One Vulkan acceleration structure (a BLAS or a TLAS) together with the buffer that stores it.
#pragma once

#include "Graphics/Vulkan/GpuBuffer.h"

#include <string>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
}

namespace ghost::graphics::raytracing {

class AccelerationStructure {
public:
    AccelerationStructure() = default;
    ~AccelerationStructure();
    AccelerationStructure(AccelerationStructure&& other) noexcept;
    AccelerationStructure& operator=(AccelerationStructure&& other) noexcept;
    AccelerationStructure(const AccelerationStructure&) = delete;
    AccelerationStructure& operator=(const AccelerationStructure&) = delete;

    // `size` comes from vkGetAccelerationStructureBuildSizesKHR (or a compacted-size query).
    void create(const vulkan::Device& device, VkAccelerationStructureTypeKHR type, VkDeviceSize size, const std::string& name);
    void destroy();

    bool valid() const { return m_handle != VK_NULL_HANDLE; }
    VkAccelerationStructureKHR handle() const { return m_handle; }
    VkDeviceAddress address() const { return m_address; } // what TLAS instances reference
    VkDeviceSize size() const { return m_buffer.size(); }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    vulkan::GpuBuffer m_buffer;
    VkAccelerationStructureKHR m_handle = VK_NULL_HANDLE;
    VkDeviceAddress m_address = 0;
};

} // namespace ghost::graphics::raytracing
