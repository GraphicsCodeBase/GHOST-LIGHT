// A VMA-allocated buffer with its device address (shaders reach buffers through addresses, not descriptors).
#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <string>

namespace ghost::graphics::vulkan {

class Device;

class GpuBuffer {
public:
    enum class Memory {
        GpuOnly,  // fastest for the GPU; filled through a staging upload
        Upload,   // CPU writes every frame (persistently mapped, write-combined)
        Readback, // GPU writes, CPU reads (persistently mapped, cached)
    };

    struct Desc {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = 0; // SHADER_DEVICE_ADDRESS is always added
        Memory memory = Memory::GpuOnly;
        std::string name;
    };

    GpuBuffer() = default;
    ~GpuBuffer();
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;
    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    void create(const Device& device, const Desc& desc);
    void destroy();

    bool valid() const { return m_buffer != VK_NULL_HANDLE; }
    VkBuffer handle() const { return m_buffer; }
    VkDeviceAddress address() const { return m_address; }
    VkDeviceSize size() const { return m_size; }
    // Non-null for Upload and Readback memory.
    void* mapped() const { return m_mapped; }
    // Readback memory may be cached but not coherent: call before the CPU reads what the GPU wrote.
    void invalidate() const;

private:
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkDeviceAddress m_address = 0;
    VkDeviceSize m_size = 0;
    void* m_mapped = nullptr;
};

} // namespace ghost::graphics::vulkan
