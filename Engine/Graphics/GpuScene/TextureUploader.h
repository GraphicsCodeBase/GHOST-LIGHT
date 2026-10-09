// Uploads CPU images into GPU textures with a full mip chain (generated with blits) and registers them as bindless.
#pragma once

#include "Assets/ImageData.h"
#include "Graphics/Vulkan/GpuImage.h"

#include <span>
#include <vector>

namespace ghost::graphics::vulkan {
class Device;
class BindlessDescriptors;
class ImmediateSubmit;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::scene {

class TextureUploader {
public:
    struct Texture {
        vulkan::GpuImage image;
        uint32_t bindlessIndex = UINT32_MAX; // kNoTexture when the source image was empty
    };

    // Same order as `images`. Uploads in batches (staging memory stays bounded) and waits for the GPU.
    // HDR (RGBA32F) images get a single mip level.
    static std::vector<Texture> upload(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::ImmediateSubmit& submit,
                                       std::span<const assets::ImageData* const> images);
};

} // namespace ghost::graphics::scene
