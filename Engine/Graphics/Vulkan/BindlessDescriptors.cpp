// Bindless descriptor set (update-after-bind, partially bound) plus the shared pipeline layout and default samplers.
#include "Graphics/Vulkan/BindlessDescriptors.h"

#include "Core/Assert.h"
#include "Core/Log.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/VulkanCheck.h"

#include <array>

namespace ghost::graphics::vulkan {

uint32_t BindlessDescriptors::Slots::allocate() {
    if (!freeList.empty()) {
        const uint32_t index = freeList.back();
        freeList.pop_back();
        return index;
    }
    GHOST_ASSERT(next < capacity, "Bindless descriptor array is full");
    return next++;
}

BindlessDescriptors::~BindlessDescriptors() {
    destroy();
}

bool BindlessDescriptors::create(const Device& device) {
    m_device = &device;
    const VkDevice vk = device.handle();

    if (device.properties().limits.maxPushConstantsSize < kPushConstantSize) {
        core::Log::error("The GPU allows only {} bytes of push constants; GHOST LIGHT needs {}.",
                         device.properties().limits.maxPushConstantsSize, kPushConstantSize);
        return false;
    }

    const std::array<VkDescriptorSetLayoutBinding, 4> bindings{{
        {kSampledImageBinding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMaxSampledImages, VK_SHADER_STAGE_ALL, nullptr},
        {kSamplerBinding, VK_DESCRIPTOR_TYPE_SAMPLER, kMaxSamplers, VK_SHADER_STAGE_ALL, nullptr},
        {kStorageImageBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageImages, VK_SHADER_STAGE_ALL, nullptr},
        {kAccelerationStructureBinding, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, kMaxAccelerationStructures, VK_SHADER_STAGE_ALL, nullptr},
    }};
    // Partially bound: unused slots may stay empty. Update-after-bind: slots can be written while the set is bound.
    constexpr VkDescriptorBindingFlags kFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                                VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    const std::array<VkDescriptorBindingFlags, 4> bindingFlags{kFlags, kFlags, kFlags, kFlags};

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
    flagsInfo.pBindingFlags = bindingFlags.data();
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    VK_CHECK(vkCreateDescriptorSetLayout(vk, &layoutInfo, nullptr, &m_setLayout));
    DebugUtils::setName(m_setLayout, "Bindless.SetLayout");

    const std::array<VkDescriptorPoolSize, 4> poolSizes{{
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMaxSampledImages},
        {VK_DESCRIPTOR_TYPE_SAMPLER, kMaxSamplers},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageImages},
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, kMaxAccelerationStructures},
    }};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    VK_CHECK(vkCreateDescriptorPool(vk, &poolInfo, nullptr, &m_pool));
    DebugUtils::setName(m_pool, "Bindless.Pool");

    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_setLayout;
    VK_CHECK(vkAllocateDescriptorSets(vk, &allocInfo, &m_set));
    DebugUtils::setName(m_set, "Bindless.Set");

    VkPushConstantRange pushRange{VK_SHADER_STAGE_ALL, 0, kPushConstantSize};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_setLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    VK_CHECK(vkCreatePipelineLayout(vk, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
    DebugUtils::setName(m_pipelineLayout, "Bindless.PipelineLayout");

    m_sampledImages.capacity = kMaxSampledImages;
    m_storageImages.capacity = kMaxStorageImages;
    createSamplers();
    return true;
}

void BindlessDescriptors::createSamplers() {
    auto makeSampler = [&](VkFilter filter, VkSamplerAddressMode address, float anisotropy, const char* name) {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.magFilter = filter;
        info.minFilter = filter;
        info.mipmapMode = filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = address;
        info.addressModeV = address;
        info.addressModeW = address;
        info.anisotropyEnable = anisotropy > 1.0f ? VK_TRUE : VK_FALSE;
        info.maxAnisotropy = anisotropy;
        info.maxLod = VK_LOD_CLAMP_NONE;
        VkSampler sampler = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSampler(m_device->handle(), &info, nullptr, &sampler));
        DebugUtils::setName(sampler, name);
        m_samplers.push_back(sampler);
    };
    makeSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 1.0f, "Sampler.LinearRepeat");
    makeSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, "Sampler.LinearClamp");
    makeSampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, "Sampler.NearestClamp");
    makeSampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, "Sampler.AnisotropicRepeat");

    std::array<VkDescriptorImageInfo, SamplerCount> infos{};
    for (uint32_t i = 0; i < SamplerCount; ++i) {
        infos[i].sampler = m_samplers[i];
    }
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = kSamplerBinding;
    write.descriptorCount = SamplerCount;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.pImageInfo = infos.data();
    vkUpdateDescriptorSets(m_device->handle(), 1, &write, 0, nullptr);
}

void BindlessDescriptors::writeImage(uint32_t binding, VkDescriptorType type, uint32_t index, VkImageView view, VkImageLayout layout) {
    VkDescriptorImageInfo info{VK_NULL_HANDLE, view, layout};
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = binding;
    write.dstArrayElement = index;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;
    vkUpdateDescriptorSets(m_device->handle(), 1, &write, 0, nullptr);
}

uint32_t BindlessDescriptors::addSampledImage(VkImageView view, VkImageLayout layout) {
    const uint32_t index = m_sampledImages.allocate();
    updateSampledImage(index, view, layout);
    return index;
}

void BindlessDescriptors::updateSampledImage(uint32_t index, VkImageView view, VkImageLayout layout) {
    writeImage(kSampledImageBinding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, index, view, layout);
}

void BindlessDescriptors::freeSampledImage(uint32_t index) {
    m_sampledImages.release(index);
}

uint32_t BindlessDescriptors::addStorageImage(VkImageView view) {
    const uint32_t index = m_storageImages.allocate();
    updateStorageImage(index, view);
    return index;
}

void BindlessDescriptors::updateStorageImage(uint32_t index, VkImageView view) {
    writeImage(kStorageImageBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, index, view, VK_IMAGE_LAYOUT_GENERAL);
}

void BindlessDescriptors::freeStorageImage(uint32_t index) {
    m_storageImages.release(index);
}

void BindlessDescriptors::setAccelerationStructure(uint32_t index, VkAccelerationStructureKHR accelerationStructure) {
    GHOST_ASSERT(index < kMaxAccelerationStructures, "Acceleration structure slot out of range");
    VkWriteDescriptorSetAccelerationStructureKHR asInfo{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asInfo.accelerationStructureCount = 1;
    asInfo.pAccelerationStructures = &accelerationStructure;
    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.pNext = &asInfo;
    write.dstSet = m_set;
    write.dstBinding = kAccelerationStructureBinding;
    write.dstArrayElement = index;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    vkUpdateDescriptorSets(m_device->handle(), 1, &write, 0, nullptr);
}

void BindlessDescriptors::bind(VkCommandBuffer cmd, VkPipelineBindPoint bindPoint) const {
    vkCmdBindDescriptorSets(cmd, bindPoint, m_pipelineLayout, 0, 1, &m_set, 0, nullptr);
}

void BindlessDescriptors::destroy() {
    if (!m_device) {
        return;
    }
    const VkDevice vk = m_device->handle();
    for (VkSampler sampler : m_samplers) {
        vkDestroySampler(vk, sampler, nullptr);
    }
    m_samplers.clear();
    if (m_pipelineLayout) vkDestroyPipelineLayout(vk, m_pipelineLayout, nullptr);
    if (m_pool) vkDestroyDescriptorPool(vk, m_pool, nullptr);
    if (m_setLayout) vkDestroyDescriptorSetLayout(vk, m_setLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
    m_pool = VK_NULL_HANDLE;
    m_setLayout = VK_NULL_HANDLE;
    m_set = VK_NULL_HANDLE;
    m_device = nullptr;
}

} // namespace ghost::graphics::vulkan
