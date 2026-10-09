// A hot-reloadable ray tracing pipeline and its shader binding table: what passes and techniques use to trace rays.
#pragma once

#include "Graphics/RayTracing/RayTracingPipelineDesc.h"
#include "Graphics/RayTracing/ShaderBindingTable.h"
#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include <cstdint>
#include <span>
#include <vector>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class DeletionQueue;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::raytracing {

class RayTracingPipeline {
public:
    RayTracingPipeline() = default;
    ~RayTracingPipeline();
    // The pipeline library keeps a builder that points at this object: it never moves.
    RayTracingPipeline(const RayTracingPipeline&) = delete;
    RayTracingPipeline& operator=(const RayTracingPipeline&) = delete;

    // Compiles now; a compile error is reported like any shader error and the pipeline stays unusable until fixed.
    void initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue,
                    RayTracingPipelineDesc desc);
    void shutdown();

    // Call while recording, before tracing: false while the shaders don't compile. After a hot reload it rebuilds the
    // shader binding table (the old one is destroyed once frame `lastFrameUsingOldTable` has finished).
    bool prepare(uint64_t lastFrameUsingOldTable);
    VkPipeline pipeline() const;
    // One ray generation invocation per (x, y, z). Bind the pipeline (PassContext::bindPipeline) and push constants first.
    void traceRays(VkCommandBuffer cmd, uint32_t width, uint32_t height, uint32_t depth = 1) const;

private:
    VkPipeline build(std::span<const std::vector<uint32_t>> spirv) const;

    shader::PipelineLibrary* m_pipelines = nullptr;
    const vulkan::Device* m_device = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    RayTracingPipelineDesc m_desc;
    std::vector<shader::ShaderEntryPoint> m_entryPoints; // flattened: raygen, misses, then each hit group's shaders
    shader::PipelineHandle m_handle = shader::kInvalidPipeline;
    VkPipeline m_tablePipeline = VK_NULL_HANDLE; // the pipeline m_table was built from
    ShaderBindingTable m_table;
};

} // namespace ghost::graphics::raytracing
