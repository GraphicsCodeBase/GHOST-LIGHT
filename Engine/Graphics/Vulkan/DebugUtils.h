// Debug names and command-buffer labels (VK_EXT_debug_utils) so RenderDoc, Nsight and validation messages show readable names.
#pragma once

#include <volk.h>

#include <string_view>

namespace ghost::graphics::vulkan {

class DebugUtils {
public:
    // Called once the device exists. Names and labels are silently skipped when the extension is unavailable.
    static void initialize(VkDevice device, bool available);

    static void setName(VkObjectType type, uint64_t handle, std::string_view name);

    // One overload per handle type, so call sites read setName(buffer, "GBuffer.Normal").
    static void setName(VkBuffer handle, std::string_view name) { setName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkImage handle, std::string_view name) { setName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkImageView handle, std::string_view name) { setName(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkSampler handle, std::string_view name) { setName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkSemaphore handle, std::string_view name) { setName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkFence handle, std::string_view name) { setName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkCommandPool handle, std::string_view name) { setName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkCommandBuffer handle, std::string_view name) { setName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkQueue handle, std::string_view name) { setName(VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkPipeline handle, std::string_view name) { setName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkPipelineLayout handle, std::string_view name) { setName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkShaderModule handle, std::string_view name) { setName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkDescriptorSet handle, std::string_view name) { setName(VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkDescriptorSetLayout handle, std::string_view name) { setName(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkDescriptorPool handle, std::string_view name) { setName(VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkQueryPool handle, std::string_view name) { setName(VK_OBJECT_TYPE_QUERY_POOL, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkSwapchainKHR handle, std::string_view name) { setName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<uint64_t>(handle), name); }
    static void setName(VkAccelerationStructureKHR handle, std::string_view name) { setName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, reinterpret_cast<uint64_t>(handle), name); }

    // Labelled regions show up as named groups in RenderDoc / Nsight captures.
    static void beginLabel(VkCommandBuffer cmd, std::string_view name);
    static void endLabel(VkCommandBuffer cmd);
};

} // namespace ghost::graphics::vulkan
