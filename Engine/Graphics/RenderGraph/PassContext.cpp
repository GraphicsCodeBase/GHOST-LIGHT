// PassContext: resolves handles to this frame's physical resources and wraps the common recording calls.
#include "Graphics/RenderGraph/PassContext.h"

#include "Graphics/RenderGraph/RenderGraph.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"

namespace ghost::graphics::rendergraph {

namespace {

VkPipelineBindPoint bindPointFor(PassKind kind) {
    switch (kind) {
    case PassKind::Raster: return VK_PIPELINE_BIND_POINT_GRAPHICS;
    case PassKind::RayTracing: return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
    default: return VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

} // namespace

VkExtent2D PassContext::extent(TextureHandle texture) const {
    return m_graph.m_textures[texture.index].extent;
}

uint32_t PassContext::sampledIndex(TextureHandle texture) const {
    const auto& t = m_graph.m_textures[texture.index];
    return t.sampledIndex[m_graph.physicalIndex(t, texture.previous)];
}

uint32_t PassContext::storageIndex(TextureHandle texture) const {
    const auto& t = m_graph.m_textures[texture.index];
    return t.storageIndex[m_graph.physicalIndex(t, texture.previous)];
}

bool PassContext::historyValid(TextureHandle texture) const {
    const auto& t = m_graph.m_textures[texture.index];
    return t.desc.lifetime == Lifetime::History && t.framesSinceAllocation >= 1;
}

VkImage PassContext::image(TextureHandle texture) const {
    const auto& t = m_graph.m_textures[texture.index];
    return t.external ? t.externalImage : t.images[m_graph.physicalIndex(t, texture.previous)].image();
}

VkBuffer PassContext::buffer(BufferHandle buffer) const {
    return m_graph.m_buffers[buffer.index].buffer.handle();
}

VkDeviceAddress PassContext::bufferAddress(BufferHandle buffer) const {
    return m_graph.m_buffers[buffer.index].buffer.address();
}

VkPipelineLayout PassContext::layout() const {
    return m_graph.m_bindless->pipelineLayout();
}

void PassContext::bindPipeline(VkPipeline pipeline) const {
    const VkPipelineBindPoint bindPoint = bindPointFor(m_kind);
    vkCmdBindPipeline(m_cmd, bindPoint, pipeline);
    m_graph.m_bindless->bind(m_cmd, bindPoint);
}

void PassContext::pushConstants(const void* data, uint32_t size) const {
    vkCmdPushConstants(m_cmd, layout(), VK_SHADER_STAGE_ALL, 0, size, data);
}

void PassContext::dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const {
    vkCmdDispatch(m_cmd, groupsX, groupsY, groupsZ);
}

void PassContext::dispatchForSize(VkExtent2D size, uint32_t groupX, uint32_t groupY) const {
    vkCmdDispatch(m_cmd, (size.width + groupX - 1) / groupX, (size.height + groupY - 1) / groupY, 1);
}

void PassContext::drawFullscreenTriangle() const {
    vkCmdDraw(m_cmd, 3, 1, 0, 0);
}

} // namespace ghost::graphics::rendergraph
