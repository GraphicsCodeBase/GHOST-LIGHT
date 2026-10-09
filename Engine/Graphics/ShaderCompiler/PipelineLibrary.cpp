// Pipeline creation from SPIR-V, file watching, and hot reload with "last good pipeline keeps running" semantics.
#include "Graphics/ShaderCompiler/PipelineLibrary.h"

#include "Core/Log.h"
#include "Graphics/ShaderCompiler/ShaderCompiler.h"
#include "Graphics/Vulkan/BindlessDescriptors.h"
#include "Graphics/Vulkan/DebugUtils.h"
#include "Graphics/Vulkan/DeletionQueue.h"
#include "Graphics/Vulkan/Device.h"

#include <array>
#include <system_error>

namespace ghost::graphics::shader {

namespace {

constexpr auto kPollInterval = std::chrono::milliseconds(250);

std::filesystem::file_time_type modificationTime(const std::filesystem::path& path) {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(path, ec);
    return ec ? std::filesystem::file_time_type::min() : time;
}

} // namespace

PipelineLibrary::~PipelineLibrary() {
    shutdown();
}

void PipelineLibrary::initialize(const vulkan::Device& device, const vulkan::BindlessDescriptors& bindless,
                                 vulkan::DeletionQueue& deletionQueue, ShaderCompiler& compiler) {
    m_device = &device;
    m_bindless = &bindless;
    m_deletionQueue = &deletionQueue;
    m_compiler = &compiler;
}

void PipelineLibrary::shutdown() {
    if (!m_device) {
        return;
    }
    for (Entry& entry : m_entries) {
        if (entry.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device->handle(), entry.pipeline, nullptr);
        }
    }
    m_entries.clear();
    m_errors.clear();
    m_device = nullptr;
}

VkPipelineLayout PipelineLibrary::layout() const {
    return m_bindless->pipelineLayout();
}

VkPipeline PipelineLibrary::pipeline(PipelineHandle handle) const {
    return handle < m_entries.size() ? m_entries[handle].pipeline : VK_NULL_HANDLE;
}

PipelineHandle PipelineLibrary::addGraphics(const GraphicsPipelineDesc& desc) {
    return add(desc.name, {desc.vertex, desc.fragment},
               [this, desc](std::span<const std::vector<uint32_t>> spirv) { return buildGraphics(desc, spirv); });
}

PipelineHandle PipelineLibrary::addCompute(const ComputePipelineDesc& desc) {
    return add(desc.name, {desc.compute}, [this, desc](std::span<const std::vector<uint32_t>> spirv) { return buildCompute(desc, spirv); });
}

PipelineHandle PipelineLibrary::addCustom(std::string name, std::vector<ShaderEntryPoint> entryPoints, Builder build) {
    return add(std::move(name), std::move(entryPoints), std::move(build));
}

PipelineHandle PipelineLibrary::add(std::string name, std::vector<ShaderEntryPoint> entryPoints, Builder build) {
    Entry entry;
    entry.name = std::move(name);
    entry.entryPoints = std::move(entryPoints);
    entry.build = std::move(build);
    m_entries.push_back(std::move(entry));
    rebuild(m_entries.back(), /*initial*/ true, 0);
    refreshErrors();
    return static_cast<PipelineHandle>(m_entries.size() - 1);
}

void PipelineLibrary::rebuild(Entry& entry, bool initial, uint64_t lastSubmittedFrame) {
    const auto start = std::chrono::steady_clock::now();
    const CompileResult result = m_compiler->compile(entry.entryPoints);

    // Watch whatever was read (on failure at least the entry files), stamped now so an unchanged broken file isn't retried every poll.
    entry.watched.clear();
    for (const std::filesystem::path& path : result.dependencies) {
        entry.watched.push_back({path, modificationTime(path)});
    }

    VkPipeline built = result.ok ? entry.build(result.spirv) : VK_NULL_HANDLE;
    if (built == VK_NULL_HANDLE) {
        entry.failed = true;
        entry.error = {entry.name, result.errorFile, result.errorLine,
                       result.ok ? "pipeline creation failed (see log)" : result.errorMessage, result.diagnostics};
        const std::string where = result.errorFile.empty() ? entry.name : result.errorFile + "(" + std::to_string(result.errorLine) + ")";
        if (initial) {
            // A shader that is broken at startup is a real error: run.bat test must fail on it.
            core::Log::error("Shader '{}' failed to compile: {}: {}", entry.name, where, entry.error.message);
        } else {
            core::Log::warning("Shader '{}' failed to compile, keeping the last working version: {}: {}", entry.name, where,
                               entry.error.message);
        }
        return;
    }

    if (entry.pipeline != VK_NULL_HANDLE) {
        const VkPipeline old = entry.pipeline;
        const VkDevice device = m_device->handle();
        m_deletionQueue->push(lastSubmittedFrame, [device, old] { vkDestroyPipeline(device, old, nullptr); });
    }
    entry.pipeline = built;
    entry.failed = false;
    vulkan::DebugUtils::setName(built, entry.name);
    if (!initial) {
        ++m_reloadCount;
        const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        core::Log::info("Shader '{}' reloaded ({:.0f} ms)", entry.name, ms);
    }
}

bool PipelineLibrary::changedOnDisk(const Entry& entry) const {
    for (const WatchedFile& file : entry.watched) {
        if (modificationTime(file.path) != file.time) {
            return true;
        }
    }
    return false;
}

void PipelineLibrary::update(uint64_t lastSubmittedFrame) {
    const auto now = std::chrono::steady_clock::now();
    if (now - m_lastPoll < kPollInterval) {
        return;
    }
    m_lastPoll = now;

    bool anyRebuilt = false;
    for (Entry& entry : m_entries) {
        if (changedOnDisk(entry)) {
            rebuild(entry, /*initial*/ false, lastSubmittedFrame);
            anyRebuilt = true;
        }
    }
    if (anyRebuilt) {
        refreshErrors();
    }
}

void PipelineLibrary::refreshErrors() {
    m_errors.clear();
    for (const Entry& entry : m_entries) {
        if (entry.failed) {
            m_errors.push_back(entry.error);
        }
    }
}

VkShaderModule PipelineLibrary::createModule(const std::vector<uint32_t>& spirv) const {
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = spirv.size() * sizeof(uint32_t);
    info.pCode = spirv.data();
    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device->handle(), &info, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

VkPipeline PipelineLibrary::buildGraphics(const GraphicsPipelineDesc& desc, std::span<const std::vector<uint32_t>> spirv) const {
    const VkShaderModule vertex = createModule(spirv[0]);
    const VkShaderModule fragment = createModule(spirv[1]);

    // Slang names every SPIR-V entry point "main".
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex, "main", nullptr};
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment, "main", nullptr};

    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO}; // vertex pulling via BDA
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = desc.topology;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = desc.polygonMode;
    raster.cullMode = desc.cullMode;
    raster.frontFace = desc.frontFace;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = desc.depthCompare;

    std::vector<VkPipelineColorBlendAttachmentState> blendStates(desc.colorFormats.size());
    for (VkPipelineColorBlendAttachmentState& blend : blendStates) {
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (desc.alphaBlend) {
            blend.blendEnable = VK_TRUE;
            blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.colorBlendOp = VK_BLEND_OP_ADD;
            blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend.alphaBlendOp = VK_BLEND_OP_ADD;
        }
    }
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = static_cast<uint32_t>(blendStates.size());
    blend.pAttachments = blendStates.data();

    const std::array<VkDynamicState, 2> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = static_cast<uint32_t>(desc.colorFormats.size());
    rendering.pColorAttachmentFormats = desc.colorFormats.data();
    rendering.depthAttachmentFormat = desc.depthFormat;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.pNext = &rendering;
    info.stageCount = static_cast<uint32_t>(stages.size());
    info.pStages = stages.data();
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &inputAssembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = m_bindless->pipelineLayout();

    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = (vertex && fragment)
                                ? vkCreateGraphicsPipelines(m_device->handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline)
                                : VK_ERROR_INITIALIZATION_FAILED;
    if (vertex) vkDestroyShaderModule(m_device->handle(), vertex, nullptr);
    if (fragment) vkDestroyShaderModule(m_device->handle(), fragment, nullptr);
    return result == VK_SUCCESS ? pipeline : VK_NULL_HANDLE;
}

VkPipeline PipelineLibrary::buildCompute(const ComputePipelineDesc& /*desc*/, std::span<const std::vector<uint32_t>> spirv) const {
    const VkShaderModule module = createModule(spirv[0]);
    if (!module) {
        return VK_NULL_HANDLE;
    }
    VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, module, "main", nullptr};
    info.layout = m_bindless->pipelineLayout();
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = vkCreateComputePipelines(m_device->handle(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
    vkDestroyShaderModule(m_device->handle(), module, nullptr);
    return result == VK_SUCCESS ? pipeline : VK_NULL_HANDLE;
}

} // namespace ghost::graphics::shader
