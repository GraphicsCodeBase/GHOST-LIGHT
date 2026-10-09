// RayTracingPipeline: SPIR-V modules -> stages + groups (general raygen/miss, triangle hit groups) -> VkPipeline + SBT.
#include "Graphics/RayTracing/RayTracingPipeline.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"
#include "Graphics/Vulkan/GpuBuffer.h"

#include <memory>

namespace ghost::graphics::raytracing {

namespace {

VkShaderStageFlagBits stageFlag(shader::ShaderStage stage) {
    switch (stage) {
    case shader::ShaderStage::RayGeneration: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case shader::ShaderStage::Miss: return VK_SHADER_STAGE_MISS_BIT_KHR;
    case shader::ShaderStage::ClosestHit: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case shader::ShaderStage::AnyHit: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case shader::ShaderStage::Intersection: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    default: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    }
}

} // namespace

RayTracingPipeline::~RayTracingPipeline() {
    shutdown();
}

void RayTracingPipeline::initialize(shader::PipelineLibrary& pipelines, const vulkan::Device& device, vulkan::DeletionQueue& deletionQueue,
                                    RayTracingPipelineDesc desc) {
    m_pipelines = &pipelines;
    m_device = &device;
    m_deletionQueue = &deletionQueue;
    m_desc = std::move(desc);
    m_entryPoints.clear();
    m_entryPoints.push_back(m_desc.rayGeneration);
    m_entryPoints.insert(m_entryPoints.end(), m_desc.misses.begin(), m_desc.misses.end());
    for (const RayTracingHitGroup& group : m_desc.hitGroups) {
        if (group.closestHit) {
            m_entryPoints.push_back(*group.closestHit);
        }
        if (group.anyHit) {
            m_entryPoints.push_back(*group.anyHit);
        }
    }
    m_handle = pipelines.addCustom(m_desc.name, m_entryPoints, [this](std::span<const std::vector<uint32_t>> spirv) { return build(spirv); });
}

void RayTracingPipeline::shutdown() {
    m_table.destroy(); // the caller waited for the GPU
    m_tablePipeline = VK_NULL_HANDLE;
}

VkPipeline RayTracingPipeline::build(std::span<const std::vector<uint32_t>> spirv) const {
    const VkDevice device = m_device->handle();
    std::vector<VkShaderModule> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    bool modulesOk = true;
    for (size_t i = 0; i < spirv.size(); ++i) {
        VkShaderModuleCreateInfo moduleInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleInfo.codeSize = spirv[i].size() * sizeof(uint32_t);
        moduleInfo.pCode = spirv[i].data();
        VkShaderModule module = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &moduleInfo, nullptr, &module) != VK_SUCCESS) {
            modulesOk = false;
            break;
        }
        modules.push_back(module);
        // Slang names every SPIR-V entry point "main".
        stages.push_back({VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, stageFlag(m_entryPoints[i].stage), module, "main", nullptr});
    }

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (modulesOk) {
        auto general = [](uint32_t stage) {
            VkRayTracingShaderGroupCreateInfoKHR group{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
            group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            group.generalShader = stage;
            group.closestHitShader = VK_SHADER_UNUSED_KHR;
            group.anyHitShader = VK_SHADER_UNUSED_KHR;
            group.intersectionShader = VK_SHADER_UNUSED_KHR;
            return group;
        };
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
        uint32_t stage = 0;
        groups.push_back(general(stage++));
        for (size_t i = 0; i < m_desc.misses.size(); ++i) {
            groups.push_back(general(stage++));
        }
        for (const RayTracingHitGroup& hitGroup : m_desc.hitGroups) {
            VkRayTracingShaderGroupCreateInfoKHR group = general(VK_SHADER_UNUSED_KHR);
            group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            group.closestHitShader = hitGroup.closestHit ? stage++ : VK_SHADER_UNUSED_KHR;
            group.anyHitShader = hitGroup.anyHit ? stage++ : VK_SHADER_UNUSED_KHR;
            groups.push_back(group);
        }

        VkRayTracingPipelineCreateInfoKHR info{VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
        info.stageCount = static_cast<uint32_t>(stages.size());
        info.pStages = stages.data();
        info.groupCount = static_cast<uint32_t>(groups.size());
        info.pGroups = groups.data();
        info.maxPipelineRayRecursionDepth = m_desc.maxRecursionDepth;
        info.layout = m_pipelines->layout();
        if (vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
            core::Log::error("Ray tracing pipeline '{}': vkCreateRayTracingPipelinesKHR failed", m_desc.name);
            pipeline = VK_NULL_HANDLE;
        }
    }
    for (VkShaderModule module : modules) {
        vkDestroyShaderModule(device, module, nullptr);
    }
    return pipeline;
}

bool RayTracingPipeline::prepare(uint64_t lastFrameUsingOldTable) {
    const VkPipeline current = pipeline();
    if (current == VK_NULL_HANDLE) {
        return false;
    }
    if (current != m_tablePipeline) {
        if (m_table.valid()) {
            auto old = std::make_shared<vulkan::GpuBuffer>(m_table.release());
            m_deletionQueue->push(lastFrameUsingOldTable, [old] { old->destroy(); });
        }
        const std::string name = m_desc.name + ".SBT";
        m_table.build(*m_device, current, static_cast<uint32_t>(m_desc.misses.size()), static_cast<uint32_t>(m_desc.hitGroups.size()),
                      name.c_str());
        m_tablePipeline = current;
    }
    return true;
}

VkPipeline RayTracingPipeline::pipeline() const {
    return m_pipelines ? m_pipelines->pipeline(m_handle) : VK_NULL_HANDLE;
}

void RayTracingPipeline::traceRays(VkCommandBuffer cmd, uint32_t width, uint32_t height, uint32_t depth) const {
    vkCmdTraceRaysKHR(cmd, &m_table.rayGeneration(), &m_table.miss(), &m_table.hit(), &m_table.callable(), width, height, depth);
}

} // namespace ghost::graphics::raytracing
