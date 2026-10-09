// What a pass declares during setup: the textures and buffers it creates, reads and writes. The graph turns these
// declarations into allocations, bindless indices and barriers, so pass code never writes a barrier by hand.
#pragma once

#include "Graphics/RenderGraph/GraphTypes.h"

#include <string>

namespace ghost::graphics::rendergraph {

class RenderGraph;

class PassBuilder {
public:
    PassBuilder(RenderGraph& graph, uint32_t passIndex) : m_graph(graph), m_pass(passIndex) {}

    // Declares a texture this pass creates (first declaration wins). Later passes refer to it by name.
    TextureHandle create(const std::string& name, const TextureDesc& desc);

    // Reads in a shader through the bindless sampled-image array (ctx.sampledIndex(handle)).
    TextureHandle sample(const std::string& name);
    // Reads last frame's copy of a History texture.
    TextureHandle samplePrevious(const std::string& name);
    // Storage-image access through the bindless storage array (ctx.storageIndex(handle)).
    TextureHandle readStorage(const std::string& name);
    TextureHandle writeStorage(const std::string& name);
    TextureHandle writeStorage(const std::string& name, const TextureDesc& desc) { create(name, desc); return writeStorage(name); }
    TextureHandle readWriteStorage(const std::string& name);
    // Raster passes: the graph begins/ends dynamic rendering around the pass with these attachments.
    TextureHandle colorAttachment(const std::string& name, LoadOp load = LoadOp::Clear, VkClearColorValue clear = {});
    TextureHandle depthAttachment(const std::string& name, LoadOp load = LoadOp::Clear, float clearDepth = 0.0f); // reverse-Z: far = 0
    // Transfer passes.
    TextureHandle copySource(const std::string& name);
    TextureHandle copyDestination(const std::string& name);

    BufferHandle createBuffer(const std::string& name, const BufferDesc& desc);
    BufferHandle readBuffer(const std::string& name);
    BufferHandle writeBuffer(const std::string& name);

private:
    RenderGraph& m_graph;
    uint32_t m_pass;
};

} // namespace ghost::graphics::rendergraph
