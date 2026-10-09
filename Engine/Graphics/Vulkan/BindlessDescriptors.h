// The one global descriptor set (bindless sampled images, samplers, storage images) and the one pipeline layout all pipelines share.
#pragma once

#include <volk.h>

#include <cstdint>
#include <vector>

namespace ghost::graphics::vulkan {

class Device;

class BindlessDescriptors {
public:
    // Must match ShaderLibrary/Bindless.slang.
    static constexpr uint32_t kSampledImageBinding = 0;
    static constexpr uint32_t kSamplerBinding = 1;
    static constexpr uint32_t kStorageImageBinding = 2;
    static constexpr uint32_t kMaxSampledImages = 16384;
    static constexpr uint32_t kMaxSamplers = 32;
    static constexpr uint32_t kMaxStorageImages = 4096;
    // Push constants carry buffer device addresses and per-pass parameters. 256 bytes is guaranteed on every RTX GPU.
    static constexpr uint32_t kPushConstantSize = 256;

    // Fixed sampler slots created at startup (same order in Bindless.slang).
    enum Sampler : uint32_t { LinearRepeat = 0, LinearClamp = 1, NearestClamp = 2, AnisotropicRepeat = 3, SamplerCount };

    BindlessDescriptors() = default;
    ~BindlessDescriptors();
    BindlessDescriptors(const BindlessDescriptors&) = delete;
    BindlessDescriptors& operator=(const BindlessDescriptors&) = delete;

    bool create(const Device& device);
    void destroy();

    // Slots are written immediately (update-after-bind), so they can change while earlier frames are in flight
    // as long as those frames don't use the slot being changed. Returns the index shaders use.
    uint32_t addSampledImage(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    void updateSampledImage(uint32_t index, VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    void freeSampledImage(uint32_t index);
    uint32_t addStorageImage(VkImageView view);
    void updateStorageImage(uint32_t index, VkImageView view);
    void freeStorageImage(uint32_t index);

    VkDescriptorSetLayout setLayout() const { return m_setLayout; }
    VkDescriptorSet set() const { return m_set; }
    VkPipelineLayout pipelineLayout() const { return m_pipelineLayout; }

    // Binds the global set at set 0 for the given bind point.
    void bind(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint) const;

private:
    struct Slots {
        std::vector<uint32_t> freeList;
        uint32_t next = 0;
        uint32_t capacity = 0;
        uint32_t allocate();
        void release(uint32_t index) { freeList.push_back(index); }
    };

    void writeImage(uint32_t binding, VkDescriptorType type, uint32_t index, VkImageView view, VkImageLayout layout);
    void createSamplers();

    const Device* m_device = nullptr;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
    VkDescriptorSet m_set = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkSampler> m_samplers;
    Slots m_sampledImages;
    Slots m_storageImages;
};

} // namespace ghost::graphics::vulkan
