// A VMA-allocated 2D image with a default view covering all mips (render targets and textures).
#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

#include <string>

namespace ghost::graphics::vulkan {

class Device;

class GpuImage {
public:
    struct Desc {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t mipLevels = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = 0;
        std::string name;
    };

    GpuImage() = default;
    ~GpuImage();
    GpuImage(GpuImage&& other) noexcept;
    GpuImage& operator=(GpuImage&& other) noexcept;
    GpuImage(const GpuImage&) = delete;
    GpuImage& operator=(const GpuImage&) = delete;

    void create(const Device& device, const Desc& desc);
    void destroy();

    bool valid() const { return m_image != VK_NULL_HANDLE; }
    VkImage image() const { return m_image; }
    VkImageView view() const { return m_view; }
    VkFormat format() const { return m_format; }
    VkExtent2D extent() const { return m_extent; }
    uint32_t mipLevels() const { return m_mipLevels; }
    VkImageAspectFlags aspect() const { return aspectOf(m_format); }

    static bool isDepthFormat(VkFormat format);
    static VkImageAspectFlags aspectOf(VkFormat format);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent{};
    uint32_t m_mipLevels = 1;
};

} // namespace ghost::graphics::vulkan
