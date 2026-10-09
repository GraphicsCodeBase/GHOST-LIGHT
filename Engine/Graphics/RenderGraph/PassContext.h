// What a pass sees while recording: its command buffer, the bindless indices / addresses of its resources, and
// small helpers (bind pipeline + bindless set, push constants, dispatch, full-screen draw).
#pragma once

#include "Graphics/RenderGraph/GraphTypes.h"

#include <cstdint>

#include <volk.h>

namespace ghost::graphics::rendergraph {

class RenderGraph;

class PassContext {
public:
    PassContext(RenderGraph& graph, PassKind kind, VkCommandBuffer cmd, uint64_t frameIndex)
        : m_graph(graph), m_kind(kind), m_cmd(cmd), m_frameIndex(frameIndex) {}

    VkCommandBuffer cmd() const { return m_cmd; }
    uint64_t frameIndex() const { return m_frameIndex; }

    VkExtent2D extent(TextureHandle texture) const;
    uint32_t sampledIndex(TextureHandle texture) const; // index into gTextures[] (ShaderLibrary/Bindless.slang)
    uint32_t storageIndex(TextureHandle texture) const; // index into gStorageImages[]
    // False on the first frame after a History texture was (re)created: last frame's copy holds garbage.
    bool historyValid(TextureHandle texture) const;
    VkImage image(TextureHandle texture) const;
    VkBuffer buffer(BufferHandle buffer) const;
    VkDeviceAddress bufferAddress(BufferHandle buffer) const;
    VkPipelineLayout layout() const;

    // Binds the pipeline for this pass's kind and the global bindless set.
    void bindPipeline(VkPipeline pipeline) const;
    void pushConstants(const void* data, uint32_t size) const;
    template <typename T>
    void pushConstants(const T& data) const {
        static_assert(sizeof(T) <= 256, "push constants are limited to 256 bytes");
        pushConstants(&data, static_cast<uint32_t>(sizeof(T)));
    }
    void dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ = 1) const;
    // One thread per pixel of `size`, in groups of groupX x groupY.
    void dispatchForSize(VkExtent2D size, uint32_t groupX = 8, uint32_t groupY = 8) const;
    void drawFullscreenTriangle() const;

private:
    RenderGraph& m_graph;
    PassKind m_kind;
    VkCommandBuffer m_cmd;
    uint64_t m_frameIndex;
};

} // namespace ghost::graphics::rendergraph
