// Render graph implementation: pass setup -> texture allocation -> per-frame barriers, dynamic rendering and timing.
#include "Graphics/RenderGraph/RenderGraph.h"

#include "Core/Log.h"
#include "Graphics/RenderGraph/GpuTimers.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/GpuCrashReporter.h"

#include <algorithm>
#include <memory>

namespace ghost::graphics::rendergraph {

namespace {

constexpr VkAccessFlags2 kWriteAccess = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                                        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                        VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

VkAttachmentLoadOp toVulkan(LoadOp load) {
    switch (load) {
    case LoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
    default: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

bool sameDesc(const TextureDesc& a, const TextureDesc& b) {
    return a.format == b.format && a.scale == b.scale && a.lifetime == b.lifetime && a.width == b.width && a.height == b.height;
}

} // namespace

VkPipelineStageFlags2 RenderGraph::shaderStages(PassKind kind) {
    switch (kind) {
    case PassKind::Raster: return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    case PassKind::Compute: return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    case PassKind::RayTracing: return VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
    case PassKind::Transfer: return VK_PIPELINE_STAGE_2_COPY_BIT;
    }
    return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
}

RenderGraph::AccessInfo RenderGraph::describe(Access access, PassKind kind) {
    const VkPipelineStageFlags2 shaders = shaderStages(kind);
    switch (access) {
    case Access::Sampled:
        return {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, shaders, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, false};
    case Access::StorageRead:
        return {VK_IMAGE_LAYOUT_GENERAL, shaders, VK_ACCESS_2_SHADER_STORAGE_READ_BIT, false};
    case Access::StorageWrite:
        return {VK_IMAGE_LAYOUT_GENERAL, shaders, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, true};
    case Access::StorageReadWrite:
        return {VK_IMAGE_LAYOUT_GENERAL, shaders, VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, true};
    case Access::ColorAttachment:
        return {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, true};
    case Access::DepthAttachment:
        return {VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, true};
    case Access::TransferSrc:
        return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, false};
    case Access::TransferDst:
        return {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, true};
    }
    return {};
}

RenderGraph::~RenderGraph() {
    shutdown();
}

void RenderGraph::initialize(const vulkan::Device& device, vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue) {
    m_device = &device;
    m_bindless = &bindless;
    m_deletionQueue = &deletionQueue;
}

void RenderGraph::shutdown() {
    if (!m_device) {
        return;
    }
    // The caller waited for the GPU: free everything immediately.
    for (Texture& texture : m_textures) {
        for (uint32_t p = 0; p < 2; ++p) {
            if (texture.sampledIndex[p] != UINT32_MAX) m_bindless->freeSampledImage(texture.sampledIndex[p]);
            if (texture.storageIndex[p] != UINT32_MAX) m_bindless->freeStorageImage(texture.storageIndex[p]);
            texture.images[p].destroy();
        }
    }
    m_textures.clear();
    m_buffers.clear();
    m_textureLookup.clear();
    m_bufferLookup.clear();
    m_passes.clear();
    m_passNames.clear();
    m_device = nullptr;
}

void RenderGraph::reset() {
    m_passes.clear();
    m_passNames.clear();
    for (Texture& texture : m_textures) {
        texture.declared = false;
        texture.referenced = false;
    }
    for (Buffer& buffer : m_buffers) {
        buffer.declared = false;
        buffer.referenced = false;
    }
    m_setupDirty = true;
}

void RenderGraph::declareExternal(const std::string& name, VkFormat format) {
    Texture& texture = m_textures[findOrAddTexture(name)];
    texture.external = true;
    texture.declared = true;
    texture.referenced = true;
    texture.desc = TextureDesc{format, Scale::Full, Lifetime::Transient};
    m_setupDirty = true;
}

void RenderGraph::addPass(std::string name, PassKind kind, SetupFn setup, ExecuteFn execute) {
    m_labels.push_back(name);
    Pass pass;
    pass.name = std::move(name);
    pass.kind = kind;
    pass.setup = std::move(setup);
    pass.execute = std::move(execute);
    pass.label = m_labels.back().c_str();
    m_passNames.push_back(pass.name);
    m_passes.push_back(std::move(pass));
    m_setupDirty = true;
}

void RenderGraph::setPassEnabled(const std::string& name, bool enabled) {
    for (Pass& pass : m_passes) {
        if (pass.name == name) {
            pass.enabled = enabled;
        }
    }
}

uint32_t RenderGraph::findOrAddTexture(const std::string& name) {
    const auto found = m_textureLookup.find(name);
    if (found != m_textureLookup.end()) {
        return found->second;
    }
    Texture texture;
    texture.name = name;
    m_textures.push_back(std::move(texture));
    const uint32_t index = static_cast<uint32_t>(m_textures.size() - 1);
    m_textureLookup.emplace(name, index);
    return index;
}

uint32_t RenderGraph::findOrAddBuffer(const std::string& name) {
    const auto found = m_bufferLookup.find(name);
    if (found != m_bufferLookup.end()) {
        return found->second;
    }
    Buffer buffer;
    buffer.name = name;
    m_buffers.push_back(std::move(buffer));
    const uint32_t index = static_cast<uint32_t>(m_buffers.size() - 1);
    m_bufferLookup.emplace(name, index);
    return index;
}

TextureHandle RenderGraph::addTextureAccess(uint32_t pass, const std::string& name, Access access, bool previous, LoadOp load, VkClearValue clear) {
    const uint32_t index = findOrAddTexture(name);
    m_textures[index].referenced = true;
    m_passes[pass].textures.push_back({index, previous, access, load, clear});
    return {index, previous};
}

BufferHandle RenderGraph::addBufferAccess(uint32_t pass, const std::string& name, bool write) {
    const uint32_t index = findOrAddBuffer(name);
    m_buffers[index].referenced = true;
    m_passes[pass].buffers.push_back({index, write});
    return {index};
}

void RenderGraph::runSetups() {
    for (uint32_t i = 0; i < m_passes.size(); ++i) {
        Pass& pass = m_passes[i];
        pass.textures.clear();
        pass.buffers.clear();
        pass.valid = true;
        PassBuilder builder(*this, i);
        pass.setup(builder);
    }

    for (Texture& texture : m_textures) {
        texture.usage = texture.external ? 0 : (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT); // every texture is viewable
    }
    for (Pass& pass : m_passes) {
        for (const TextureAccess& access : pass.textures) {
            Texture& texture = m_textures[access.texture];
            const bool storage = access.access == Access::StorageRead || access.access == Access::StorageWrite ||
                                 access.access == Access::StorageReadWrite;
            std::string problem;
            if (!texture.declared) {
                problem = "uses texture '" + texture.name + "', but no pass creates it";
            } else if (access.previous && texture.desc.lifetime != Lifetime::History) {
                problem = "reads the previous frame of '" + texture.name + "', which is not a History texture";
            } else if (storage && vulkan::GpuImage::isDepthFormat(texture.desc.format)) {
                problem = "uses depth texture '" + texture.name + "' as a storage image";
            } else if (texture.external && storage) {
                problem = "uses external texture '" + texture.name + "' as a storage image";
            }
            if (!problem.empty()) {
                core::Log::error("Render graph: pass '{}' {}. The pass is skipped.", pass.name, problem);
                pass.valid = false;
                continue;
            }
            switch (access.access) {
            case Access::Sampled: texture.usage |= VK_IMAGE_USAGE_SAMPLED_BIT; break;
            case Access::StorageRead:
            case Access::StorageWrite:
            case Access::StorageReadWrite: texture.usage |= VK_IMAGE_USAGE_STORAGE_BIT; break;
            case Access::ColorAttachment: texture.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; break;
            case Access::DepthAttachment: texture.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; break;
            case Access::TransferSrc: texture.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; break;
            case Access::TransferDst: texture.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT; break;
            }
        }
        for (const BufferAccess& access : pass.buffers) {
            if (!m_buffers[access.buffer].declared) {
                core::Log::error("Render graph: pass '{}' uses buffer '{}', but no pass creates it. The pass is skipped.", pass.name,
                                 m_buffers[access.buffer].name);
                pass.valid = false;
            }
        }
    }
}

VkExtent2D RenderGraph::extentFor(const TextureDesc& desc, VkExtent2D renderSize) const {
    VkExtent2D extent = renderSize;
    switch (desc.scale) {
    case Scale::Full: break;
    case Scale::Half: extent = {(renderSize.width + 1) / 2, (renderSize.height + 1) / 2}; break;
    case Scale::Quarter: extent = {(renderSize.width + 3) / 4, (renderSize.height + 3) / 4}; break;
    case Scale::Fixed: extent = {desc.width, desc.height}; break;
    }
    return {std::max(extent.width, 1u), std::max(extent.height, 1u)};
}

uint32_t RenderGraph::physicalIndex(const Texture& texture, bool previous) const {
    if (texture.desc.lifetime != Lifetime::History) {
        return 0;
    }
    return previous ? (texture.current ^ 1u) : texture.current;
}

void RenderGraph::allocateTexture(Texture& texture, VkExtent2D extent, uint64_t /*lastSubmittedFrame*/) {
    const uint32_t count = texture.desc.lifetime == Lifetime::History ? 2 : 1;
    for (uint32_t p = 0; p < count; ++p) {
        vulkan::GpuImage::Desc desc;
        desc.width = extent.width;
        desc.height = extent.height;
        desc.format = texture.desc.format;
        desc.usage = texture.usage;
        desc.name = count == 2 ? texture.name + (p == 0 ? ".A" : ".B") : texture.name;
        texture.images[p].create(*m_device, desc);
        if (texture.usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            texture.sampledIndex[p] = m_bindless->addSampledImage(texture.images[p].view());
        }
        if (texture.usage & VK_IMAGE_USAGE_STORAGE_BIT) {
            texture.storageIndex[p] = m_bindless->addStorageImage(texture.images[p].view());
        }
        texture.state[p] = SyncState{};
    }
    texture.extent = extent;
    texture.allocatedDesc = texture.desc;
    texture.allocatedUsage = texture.usage;
    texture.current = 0;
    texture.framesSinceAllocation = 0;
}

void RenderGraph::releaseTexture(Texture& texture, uint64_t lastSubmittedFrame) {
    for (uint32_t p = 0; p < 2; ++p) {
        if (!texture.images[p].valid()) {
            continue;
        }
        // Frames in flight may still read this image (and its bindless slots): destroy it once they're done.
        auto image = std::make_shared<vulkan::GpuImage>(std::move(texture.images[p]));
        const uint32_t sampled = texture.sampledIndex[p];
        const uint32_t storage = texture.storageIndex[p];
        vulkan::BindlessDescriptors* bindless = m_bindless;
        m_deletionQueue->push(lastSubmittedFrame, [image, sampled, storage, bindless] {
            image->destroy();
            if (sampled != UINT32_MAX) bindless->freeSampledImage(sampled);
            if (storage != UINT32_MAX) bindless->freeStorageImage(storage);
        });
        texture.sampledIndex[p] = UINT32_MAX;
        texture.storageIndex[p] = UINT32_MAX;
        texture.state[p] = SyncState{};
    }
}

bool RenderGraph::compile(VkExtent2D renderSize, uint64_t lastSubmittedFrame) {
    if (m_setupDirty) {
        runSetups();
        m_setupDirty = false;
    }

    for (Texture& texture : m_textures) {
        if (texture.external) {
            continue;
        }
        if (!texture.referenced || !texture.declared) {
            releaseTexture(texture, lastSubmittedFrame);
            continue;
        }
        const VkExtent2D extent = extentFor(texture.desc, renderSize);
        const bool needsAllocation = !texture.images[0].valid() || extent.width != texture.extent.width ||
                                     extent.height != texture.extent.height || !sameDesc(texture.desc, texture.allocatedDesc) ||
                                     texture.usage != texture.allocatedUsage;
        if (needsAllocation) {
            releaseTexture(texture, lastSubmittedFrame);
            allocateTexture(texture, extent, lastSubmittedFrame);
        }
    }

    for (Buffer& buffer : m_buffers) {
        const bool wanted = buffer.referenced && buffer.declared;
        if (buffer.buffer.valid() && (!wanted || buffer.allocatedSize != buffer.desc.size)) {
            auto old = std::make_shared<vulkan::GpuBuffer>(std::move(buffer.buffer));
            m_deletionQueue->push(lastSubmittedFrame, [old] { old->destroy(); });
            buffer.allocatedSize = 0;
        }
        if (wanted && !buffer.buffer.valid()) {
            vulkan::GpuBuffer::Desc desc;
            desc.size = buffer.desc.size;
            desc.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            desc.name = buffer.name;
            buffer.buffer.create(*m_device, desc);
            buffer.allocatedSize = buffer.desc.size;
            buffer.state = SyncState{};
        }
    }

    m_renderSize = renderSize;
    return std::all_of(m_passes.begin(), m_passes.end(), [](const Pass& pass) { return pass.valid; });
}

void RenderGraph::bindExternal(const std::string& name, VkImage image, VkImageView view, VkExtent2D extent, VkImageLayout finalLayout) {
    const auto found = m_textureLookup.find(name);
    if (found == m_textureLookup.end()) {
        return;
    }
    Texture& texture = m_textures[found->second];
    texture.externalImage = image;
    texture.externalView = view;
    texture.extent = extent;
    texture.finalLayout = finalLayout;
}

void RenderGraph::transitionTexture(const TextureAccess& access, PassKind kind, std::vector<VkImageMemoryBarrier2>& barriers) {
    Texture& texture = m_textures[access.texture];
    const uint32_t p = physicalIndex(texture, access.previous);
    SyncState& state = texture.state[p];
    const AccessInfo want = describe(access.access, kind);

    VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture.external ? texture.externalImage : texture.images[p].image();
    barrier.subresourceRange = {vulkan::GpuImage::aspectOf(texture.desc.format), 0, VK_REMAINING_MIP_LEVELS, 0, 1};

    if (want.write || state.layout != want.layout) {
        // Writes (and layout changes) wait for the previous write and every read since it.
        barrier.srcStageMask = state.writeStages | state.readStages;
        barrier.srcAccessMask = state.writeAccess;
        barrier.dstStageMask = want.stages;
        barrier.dstAccessMask = want.access;
        barrier.oldLayout = state.layout;
        barrier.newLayout = want.layout;
        barriers.push_back(barrier);
        state.layout = want.layout;
        state.writeStages = want.stages;
        state.writeAccess = want.write ? (want.access & kWriteAccess) : VK_ACCESS_2_NONE;
        state.readStages = want.write ? VK_PIPELINE_STAGE_2_NONE : want.stages;
        state.visibleStages = want.stages;
        return;
    }
    // Read after read in the same layout: only a stage that hasn't seen the last write yet needs a barrier.
    if (state.writeStages != VK_PIPELINE_STAGE_2_NONE && (want.stages & ~state.visibleStages) != 0) {
        barrier.srcStageMask = state.writeStages;
        barrier.srcAccessMask = state.writeAccess;
        barrier.dstStageMask = want.stages;
        barrier.dstAccessMask = want.access;
        barrier.oldLayout = state.layout;
        barrier.newLayout = state.layout;
        barriers.push_back(barrier);
        state.visibleStages |= want.stages;
    }
    state.readStages |= want.stages;
}

void RenderGraph::transitionBuffer(const BufferAccess& access, PassKind kind, std::vector<VkMemoryBarrier2>& barriers) {
    SyncState& state = m_buffers[access.buffer].state;
    const VkPipelineStageFlags2 stages = shaderStages(kind);
    const bool transfer = kind == PassKind::Transfer;
    const VkAccessFlags2 accessMask = access.write ? (transfer ? VK_ACCESS_2_TRANSFER_WRITE_BIT : VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)
                                                   : (transfer ? VK_ACCESS_2_TRANSFER_READ_BIT : VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
    VkMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    if (access.write) {
        if ((state.writeStages | state.readStages) != VK_PIPELINE_STAGE_2_NONE) {
            barrier.srcStageMask = state.writeStages | state.readStages;
            barrier.srcAccessMask = state.writeAccess;
            barrier.dstStageMask = stages;
            barrier.dstAccessMask = accessMask;
            barriers.push_back(barrier);
        }
        state.writeStages = stages;
        state.writeAccess = accessMask;
        state.readStages = VK_PIPELINE_STAGE_2_NONE;
        state.visibleStages = stages;
        return;
    }
    if (state.writeStages != VK_PIPELINE_STAGE_2_NONE && (stages & ~state.visibleStages) != 0) {
        barrier.srcStageMask = state.writeStages;
        barrier.srcAccessMask = state.writeAccess;
        barrier.dstStageMask = stages;
        barrier.dstAccessMask = accessMask;
        barriers.push_back(barrier);
        state.visibleStages |= stages;
    }
    state.readStages |= stages;
}

void RenderGraph::beginRendering(VkCommandBuffer cmd, const Pass& pass) {
    std::vector<VkRenderingAttachmentInfo> colors;
    VkRenderingAttachmentInfo depth{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    bool hasDepth = false;
    VkExtent2D extent{};
    for (const TextureAccess& access : pass.textures) {
        if (access.access != Access::ColorAttachment && access.access != Access::DepthAttachment) {
            continue;
        }
        const Texture& texture = m_textures[access.texture];
        const uint32_t p = physicalIndex(texture, access.previous);
        VkRenderingAttachmentInfo info{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        info.imageView = texture.external ? texture.externalView : texture.images[p].view();
        info.loadOp = toVulkan(access.load);
        info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        info.clearValue = access.clear;
        extent = texture.extent;
        if (access.access == Access::ColorAttachment) {
            info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colors.push_back(info);
        } else {
            info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depth = info;
            hasDepth = true;
        }
    }

    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea = {{0, 0}, extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = static_cast<uint32_t>(colors.size());
    rendering.pColorAttachments = colors.data();
    rendering.pDepthAttachment = hasDepth ? &depth : nullptr;
    vkCmdBeginRendering(cmd, &rendering);

    const VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
    const VkRect2D scissor{{0, 0}, extent};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void RenderGraph::execute(VkCommandBuffer cmd, uint64_t frameIndex, GpuTimers* timers) {
    // Frame start: history textures swap, and copies about to be overwritten start from "undefined" (no stale contents
    // kept). Stage masks are kept so the first barrier still waits for the previous frame's use of the image.
    for (Texture& texture : m_textures) {
        if (!texture.referenced) {
            continue;
        }
        if (texture.external) {
            // The acquire semaphore is waited on at COLOR_ATTACHMENT_OUTPUT: chain the first barrier to that stage.
            texture.state[0] = SyncState{};
            texture.state[0].writeStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            continue;
        }
        if (texture.desc.lifetime == Lifetime::History) {
            texture.current ^= 1u;
            texture.state[texture.current].layout = VK_IMAGE_LAYOUT_UNDEFINED;
        } else if (texture.desc.lifetime == Lifetime::Transient) {
            texture.state[0].layout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    std::vector<VkImageMemoryBarrier2> imageBarriers;
    std::vector<VkMemoryBarrier2> memoryBarriers;
    for (Pass& pass : m_passes) {
        if (!pass.enabled || !pass.valid) {
            continue;
        }
        const uint32_t scope = timers ? timers->begin(cmd, pass.name) : UINT32_MAX;
        vulkan::DebugUtils::beginLabel(cmd, pass.name);
        vulkan::GpuCrashReporter::checkpoint(cmd, pass.label);

        imageBarriers.clear();
        memoryBarriers.clear();
        for (const TextureAccess& access : pass.textures) {
            transitionTexture(access, pass.kind, imageBarriers);
        }
        for (const BufferAccess& access : pass.buffers) {
            transitionBuffer(access, pass.kind, memoryBarriers);
        }
        if (!imageBarriers.empty() || !memoryBarriers.empty()) {
            VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
            dependency.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
            dependency.pImageMemoryBarriers = imageBarriers.data();
            dependency.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
            dependency.pMemoryBarriers = memoryBarriers.data();
            vkCmdPipelineBarrier2(cmd, &dependency);
        }

        if (pass.kind == PassKind::Raster) {
            beginRendering(cmd, pass);
        }
        PassContext context(*this, pass.kind, cmd, frameIndex);
        pass.execute(context);
        if (pass.kind == PassKind::Raster) {
            vkCmdEndRendering(cmd);
        }

        vulkan::DebugUtils::endLabel(cmd);
        if (timers) {
            timers->end(cmd, scope);
        }
    }

    // External images leave the graph in the layout their owner expects (the swapchain: PRESENT_SRC).
    imageBarriers.clear();
    for (Texture& texture : m_textures) {
        if (!texture.external || !texture.referenced || texture.externalImage == VK_NULL_HANDLE) {
            continue;
        }
        SyncState& state = texture.state[0];
        if (state.layout == texture.finalLayout) {
            continue;
        }
        VkImageMemoryBarrier2 barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcStageMask = state.writeStages | state.readStages;
        barrier.srcAccessMask = state.writeAccess;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstAccessMask = VK_ACCESS_2_NONE;
        barrier.oldLayout = state.layout;
        barrier.newLayout = texture.finalLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.externalImage;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        imageBarriers.push_back(barrier);
        state.layout = texture.finalLayout;
    }
    if (!imageBarriers.empty()) {
        VkDependencyInfo dependency{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
        dependency.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
        dependency.pImageMemoryBarriers = imageBarriers.data();
        vkCmdPipelineBarrier2(cmd, &dependency);
    }

    for (Texture& texture : m_textures) {
        if (texture.referenced && !texture.external && texture.framesSinceAllocation < UINT32_MAX) {
            ++texture.framesSinceAllocation;
        }
    }
}

std::vector<RenderGraph::TextureInfo> RenderGraph::textures() const {
    std::vector<TextureInfo> infos;
    for (const Texture& texture : m_textures) {
        if (texture.referenced && texture.declared) {
            infos.push_back({texture.name, texture.desc.format, texture.extent, texture.desc.lifetime, texture.external});
        }
    }
    return infos;
}

} // namespace ghost::graphics::rendergraph
