// AccelerationStructure: storage buffer + VkAccelerationStructureKHR + its device address.
#include "Graphics/RayTracing/AccelerationStructure.h"

#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <utility>

namespace ghost::graphics::raytracing {

AccelerationStructure::~AccelerationStructure() {
    destroy();
}

AccelerationStructure::AccelerationStructure(AccelerationStructure&& other) noexcept {
    *this = std::move(other);
}

AccelerationStructure& AccelerationStructure::operator=(AccelerationStructure&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
        m_buffer = std::move(other.m_buffer);
        m_handle = std::exchange(other.m_handle, VK_NULL_HANDLE);
        m_address = std::exchange(other.m_address, 0);
    }
    return *this;
}

void AccelerationStructure::create(const vulkan::Device& device, VkAccelerationStructureTypeKHR type, VkDeviceSize size,
                                   const std::string& name) {
    destroy();
    m_device = device.handle();
    m_buffer.create(device, {size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, vulkan::GpuBuffer::Memory::GpuOnly, name + ".Buffer"});

    VkAccelerationStructureCreateInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    info.buffer = m_buffer.handle();
    info.size = size;
    info.type = type;
    VK_CHECK(vkCreateAccelerationStructureKHR(m_device, &info, nullptr, &m_handle));
    vulkan::DebugUtils::setName(m_handle, name);

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = m_handle;
    m_address = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addressInfo);
}

void AccelerationStructure::destroy() {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(m_device, m_handle, nullptr);
    }
    m_handle = VK_NULL_HANDLE;
    m_address = 0;
    m_buffer.destroy();
}

} // namespace ghost::graphics::raytracing
