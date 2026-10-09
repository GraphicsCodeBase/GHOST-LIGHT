// GpuImage: dedicated-friendly VMA allocation, default view, depth/color aspect handling.
#include "Graphics/Vulkan/GpuImage.h"

#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <utility>

namespace ghost::graphics::vulkan {

bool GpuImage::isDepthFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return true;
    default:
        return false;
    }
}

VkImageAspectFlags GpuImage::aspectOf(VkFormat format) {
    return isDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
}

GpuImage::~GpuImage() {
    destroy();
}

GpuImage::GpuImage(GpuImage&& other) noexcept {
    *this = std::move(other);
}

GpuImage& GpuImage::operator=(GpuImage&& other) noexcept {
    if (this != &other) {
        destroy();
        m_device = std::exchange(other.m_device, VK_NULL_HANDLE);
        m_allocator = std::exchange(other.m_allocator, VK_NULL_HANDLE);
        m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
        m_view = std::exchange(other.m_view, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_format = std::exchange(other.m_format, VK_FORMAT_UNDEFINED);
        m_extent = std::exchange(other.m_extent, VkExtent2D{});
        m_mipLevels = std::exchange(other.m_mipLevels, 1u);
    }
    return *this;
}

void GpuImage::create(const Device& device, const Desc& desc) {
    destroy();
    m_device = device.handle();
    m_allocator = device.allocator();
    m_format = desc.format;
    m_extent = {desc.width, desc.height};
    m_mipLevels = desc.mipLevels;

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = desc.format;
    imageInfo.extent = {desc.width, desc.height, 1};
    imageInfo.mipLevels = desc.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = desc.usage;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    // Render targets get their own memory block: they are large, long-lived and resized together.
    if (desc.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT)) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    VK_CHECK(vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr));

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = desc.format;
    viewInfo.subresourceRange = {aspectOf(desc.format), 0, desc.mipLevels, 0, 1};
    VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_view));

    if (!desc.name.empty()) {
        DebugUtils::setName(m_image, desc.name);
        DebugUtils::setName(m_view, desc.name + ".View");
        vmaSetAllocationName(m_allocator, m_allocation, desc.name.c_str());
    }
}

void GpuImage::destroy() {
    if (m_view != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_view, nullptr);
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_allocator, m_image, m_allocation);
    }
    m_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_allocation = VK_NULL_HANDLE;
}

} // namespace ghost::graphics::vulkan
