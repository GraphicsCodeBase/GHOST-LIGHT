// GPU selection and the logical device: picks an RTX-capable GPU, enables Vulkan 1.3 + ray tracing, owns the queue and the VMA allocator.
#pragma once

// volk first: vk_mem_alloc.h only declares vmaImportVulkanFunctionsFromVolk when volk.h is already included.
#include <volk.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <string>

namespace ghost::graphics::vulkan {

class Instance;

class Device {
public:
    Device() = default;
    ~Device();
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Returns false after logging why no GPU qualified (e.g. not an RTX card, driver too old).
    bool create(const Instance& instance, VkSurfaceKHR surface);
    void destroy();

    VkDevice handle() const { return m_device.device; }
    VkPhysicalDevice physicalDevice() const { return m_device.physical_device.physical_device; }
    const vkb::Device& bootstrap() const { return m_device; }
    VkQueue graphicsQueue() const { return m_graphicsQueue; }
    uint32_t graphicsQueueFamily() const { return m_graphicsQueueFamily; }
    VmaAllocator allocator() const { return m_allocator; }

    const std::string& name() const { return m_device.physical_device.name; }
    const VkPhysicalDeviceProperties& properties() const { return m_device.physical_device.properties; }
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingProperties() const { return m_rayTracingProperties; }
    const VkPhysicalDeviceAccelerationStructurePropertiesKHR& accelerationStructureProperties() const { return m_accelerationStructureProperties; }
    // Nanoseconds per timestamp tick (for GPU timers).
    float timestampPeriod() const { return m_device.physical_device.properties.limits.timestampPeriod; }

    void waitIdle() const;

private:
    void queryProperties();
    bool createAllocator(const Instance& instance, bool memoryBudget);

    vkb::Device m_device;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
    bool m_created = false;
};

} // namespace ghost::graphics::vulkan
