// GpuBuffer: VMA allocation, device address query, persistent mapping for CPU-visible memory.
#include "Graphics/Vulkan/GpuBuffer.h"

#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <utility>

namespace ghost::graphics::vulkan {

GpuBuffer::~GpuBuffer() {
    destroy();
}

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept {
    *this = std::move(other);
}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this != &other) {
        destroy();
        m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
        m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_address = std::exchange(other.m_address, 0);
        m_size = std::exchange(other.m_size, 0);
        m_mapped = std::exchange(other.m_mapped, nullptr);
    }
    return *this;
}

void GpuBuffer::create(const Device& device, const Desc& desc) {
    destroy();
    m_allocator = device.allocator();

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = desc.size;
    bufferInfo.usage = desc.usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (desc.memory == Memory::Upload) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else if (desc.memory == Memory::Readback) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocationInfo info{};
    VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, &m_buffer, &m_allocation, &info));
    m_size = desc.size;
    m_mapped = info.pMappedData;

    VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addressInfo.buffer = m_buffer;
    m_address = vkGetBufferDeviceAddress(device.handle(), &addressInfo);

    if (!desc.name.empty()) {
        DebugUtils::setName(m_buffer, desc.name);
        vmaSetAllocationName(m_allocator, m_allocation, desc.name.c_str());
    }
}

void GpuBuffer::invalidate() const {
    if (m_allocation != VK_NULL_HANDLE) {
        VK_CHECK(vmaInvalidateAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE));
    }
}

void GpuBuffer::destroy() {
    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
    }
    m_buffer = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
    m_address = 0;
    m_size = 0;
    m_mapped = nullptr;
}

} // namespace ghost::graphics::vulkan
