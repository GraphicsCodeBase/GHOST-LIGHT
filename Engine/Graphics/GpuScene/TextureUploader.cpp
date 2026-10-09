// Texture upload: staging copy into mip 0, blit each mip from the previous one, end in SHADER_READ_ONLY_OPTIMAL.
#include "Graphics/GpuScene/TextureUploader.h"

#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/GpuBuffer.h"
#include "Graphics/Vulkan/ImmediateSubmit.h"

#include <algorithm>
#include <bit>
#include <cstring>

namespace ghost::graphics::scene {

namespace {

constexpr VkDeviceSize kBatchBytes = 256ull * 1024 * 1024; // staging memory per submit

VkFormat formatOf(assets::ImageData::Format format) {
    switch (format) {
    case assets::ImageData::Format::Rgba8Srgb: return VK_FORMAT_R8G8B8A8_SRGB;
    case assets::ImageData::Format::Rgba32Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
    default: return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

void barrier(VkCommandBuffer cmd, VkImage image, uint32_t baseMip, uint32_t mipCount, VkImageLayout from, VkImageLayout to,
             VkPipelineStageFlags2 srcStage, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = srcStage;
    b.srcAccessMask = srcAccess;
    b.dstStageMask = dstStage;
    b.dstAccessMask = dstAccess;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, baseMip, mipCount, 0, 1};
    VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dependency);
}

void recordUpload(VkCommandBuffer cmd, const vulkan::GpuImage& image, VkBuffer staging, VkDeviceSize offset) {
    const uint32_t mips = image.mipLevels();
    const VkExtent2D extent = image.extent();
    barrier(cmd, image.image(), 0, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    VkBufferImageCopy copy{};
    copy.bufferOffset = offset;
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyBufferToImage(cmd, staging, image.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    int32_t width = static_cast<int32_t>(extent.width);
    int32_t height = static_cast<int32_t>(extent.height);
    for (uint32_t mip = 1; mip < mips; ++mip) {
        barrier(cmd, image.image(), mip - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT);
        const int32_t nextWidth = std::max(width / 2, 1);
        const int32_t nextHeight = std::max(height / 2, 1);
        VkImageBlit blit{};
        blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, 0, 1};
        blit.srcOffsets[1] = {width, height, 1};
        blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, 1};
        blit.dstOffsets[1] = {nextWidth, nextHeight, 1};
        vkCmdBlitImage(cmd, image.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);
        width = nextWidth;
        height = nextHeight;
    }
    // Every frame afterwards samples these from any shader stage.
    if (mips > 1) {
        barrier(cmd, image.image(), 0, mips - 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    }
    barrier(cmd, image.image(), mips - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
}

} // namespace

std::vector<TextureUploader::Texture> TextureUploader::upload(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless,
                                                              vulkan::ImmediateSubmit& submit,
                                                              std::span<const assets::ImageData* const> images) {
    std::vector<Texture> textures(images.size());
    size_t next = 0;
    while (next < images.size()) {
        // Gather a batch that fits the staging budget (at least one image, however large).
        size_t end = next;
        VkDeviceSize batchBytes = 0;
        while (end < images.size()) {
            const assets::ImageData* image = images[end];
            const VkDeviceSize bytes = (image && image->valid()) ? ((image->pixels.size() + 15) & ~VkDeviceSize{15}) : 0;
            if (end > next && batchBytes + bytes > kBatchBytes) {
                break;
            }
            batchBytes += bytes;
            ++end;
        }

        vulkan::GpuBuffer staging;
        staging.create(device, {std::max<VkDeviceSize>(batchBytes, 16), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vulkan::GpuBuffer::Memory::Upload,
                                "TextureStaging"});
        std::vector<VkDeviceSize> offsets(end - next, 0);
        VkDeviceSize offset = 0;
        for (size_t i = next; i < end; ++i) {
            const assets::ImageData* image = images[i];
            if (!image || !image->valid()) {
                continue;
            }
            std::memcpy(static_cast<uint8_t*>(staging.mapped()) + offset, image->pixels.data(), image->pixels.size());
            offsets[i - next] = offset;
            offset += (image->pixels.size() + 15) & ~VkDeviceSize{15};

            vulkan::GpuImage::Desc desc;
            desc.width = image->width;
            desc.height = image->height;
            desc.format = formatOf(image->format);
            desc.mipLevels = image->format == assets::ImageData::Format::Rgba32Float
                                 ? 1u
                                 : static_cast<uint32_t>(std::bit_width(std::max(image->width, image->height)));
            desc.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            desc.name = image->name;
            textures[i].image.create(device, desc);
        }

        submit.run([&](VkCommandBuffer cmd) {
            for (size_t i = next; i < end; ++i) {
                if (textures[i].image.valid()) {
                    recordUpload(cmd, textures[i].image, staging.handle(), offsets[i - next]);
                }
            }
        });
        for (size_t i = next; i < end; ++i) {
            if (textures[i].image.valid()) {
                textures[i].bindlessIndex = bindless.addSampledImage(textures[i].image.view());
            }
        }
        next = end;
    }
    return textures;
}

} // namespace ghost::graphics::scene
