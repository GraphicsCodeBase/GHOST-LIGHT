// PassBuilder: records a pass's declarations into the graph (resolved and validated in RenderGraph::compile).
#include "Graphics/RenderGraph/PassBuilder.h"

#include "Core/Log.h"
#include "Graphics/RenderGraph/RenderGraph.h"

namespace ghost::graphics::rendergraph {

TextureHandle PassBuilder::create(const std::string& name, const TextureDesc& desc) {
    const uint32_t index = m_graph.findOrAddTexture(name);
    RenderGraph::Texture& texture = m_graph.m_textures[index];
    if (!texture.declared) {
        texture.desc = desc;
        texture.declared = true;
    } else if (texture.desc.format != desc.format || texture.desc.scale != desc.scale || texture.desc.lifetime != desc.lifetime) {
        core::Log::warning("Render graph: pass '{}' re-creates texture '{}' with a different description; the first one is kept.",
                           m_graph.m_passes[m_pass].name, name);
    }
    texture.referenced = true;
    return {index, false};
}

const TextureDesc* PassBuilder::find(const std::string& name) const {
    const auto found = m_graph.m_textureLookup.find(name);
    if (found == m_graph.m_textureLookup.end()) {
        return nullptr;
    }
    const RenderGraph::Texture& texture = m_graph.m_textures[found->second];
    return texture.declared ? &texture.desc : nullptr;
}

TextureHandle PassBuilder::sample(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::Sampled, false);
}

TextureHandle PassBuilder::samplePrevious(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::Sampled, true);
}

TextureHandle PassBuilder::readStorage(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::StorageRead, false);
}

TextureHandle PassBuilder::writeStorage(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::StorageWrite, false);
}

TextureHandle PassBuilder::readWriteStorage(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::StorageReadWrite, false);
}

TextureHandle PassBuilder::colorAttachment(const std::string& name, LoadOp load, VkClearColorValue clear) {
    VkClearValue value{};
    value.color = clear;
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::ColorAttachment, false, load, value);
}

TextureHandle PassBuilder::depthAttachment(const std::string& name, LoadOp load, float clearDepth) {
    VkClearValue value{};
    value.depthStencil = {clearDepth, 0};
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::DepthAttachment, false, load, value);
}

TextureHandle PassBuilder::copySource(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::TransferSrc, false);
}

TextureHandle PassBuilder::copyDestination(const std::string& name) {
    return m_graph.addTextureAccess(m_pass, name, RenderGraph::Access::TransferDst, false);
}

BufferHandle PassBuilder::createBuffer(const std::string& name, const BufferDesc& desc) {
    const uint32_t index = m_graph.findOrAddBuffer(name);
    RenderGraph::Buffer& buffer = m_graph.m_buffers[index];
    if (!buffer.declared) {
        buffer.desc = desc;
        buffer.declared = true;
    }
    buffer.referenced = true;
    return {index};
}

BufferHandle PassBuilder::readBuffer(const std::string& name) {
    return m_graph.addBufferAccess(m_pass, name, false);
}

BufferHandle PassBuilder::writeBuffer(const std::string& name) {
    return m_graph.addBufferAccess(m_pass, name, true);
}

} // namespace ghost::graphics::rendergraph
