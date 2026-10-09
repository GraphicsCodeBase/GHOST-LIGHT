// Owns every pipeline built from Slang shaders. Watches their source files, recompiles on change, and keeps the
// last working pipeline when a change doesn't compile (the error goes to the on-screen overlay instead of a crash).
#pragma once

#include "Graphics/ShaderCompiler/PipelineDesc.h"
#include "Graphics/ShaderCompiler/ShaderEntryPoint.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include <volk.h>

namespace ghost::graphics::vulkan {
class Device;
class BindlessDescriptors;
class DeletionQueue;
} // namespace ghost::graphics::vulkan

namespace ghost::graphics::shader {

class ShaderCompiler;

using PipelineHandle = uint32_t;
inline constexpr PipelineHandle kInvalidPipeline = UINT32_MAX;

struct ShaderError {
    std::string pipeline;
    std::string file; // root-relative
    int line = 0;
    std::string message;
    std::string diagnostics; // full compiler output
};

class PipelineLibrary {
public:
    // Turns compiled SPIR-V (one module per entry point, same order) into a pipeline; returns VK_NULL_HANDLE on failure.
    using Builder = std::function<VkPipeline(std::span<const std::vector<uint32_t>> spirv)>;

    PipelineLibrary() = default;
    ~PipelineLibrary();
    PipelineLibrary(const PipelineLibrary&) = delete;
    PipelineLibrary& operator=(const PipelineLibrary&) = delete;

    void initialize(const vulkan::Device& device, const vulkan::BindlessDescriptors& bindless, vulkan::DeletionQueue& deletionQueue,
                    ShaderCompiler& compiler);
    void shutdown();

    // Compiles immediately. If that fails the handle is still valid: pipeline() returns VK_NULL_HANDLE until a fix compiles.
    PipelineHandle addGraphics(const GraphicsPipelineDesc& desc);
    PipelineHandle addCompute(const ComputePipelineDesc& desc);
    // For pipelines with extra state (ray tracing pipelines + shader binding tables).
    PipelineHandle addCustom(std::string name, std::vector<ShaderEntryPoint> entryPoints, Builder build);

    VkPipeline pipeline(PipelineHandle handle) const;
    VkPipelineLayout layout() const;

    // Once per frame, before recording: checks watched files (at most 4x per second) and rebuilds changed pipelines.
    // Replaced pipelines are destroyed after GPU frame `lastSubmittedFrame` completes.
    void update(uint64_t lastSubmittedFrame);

    const std::vector<ShaderError>& errors() const { return m_errors; }
    uint64_t reloadCount() const { return m_reloadCount; }

private:
    struct WatchedFile {
        std::filesystem::path path;
        std::filesystem::file_time_type time;
    };
    struct Entry {
        std::string name;
        std::vector<ShaderEntryPoint> entryPoints;
        Builder build;
        VkPipeline pipeline = VK_NULL_HANDLE;
        std::vector<WatchedFile> watched;
        bool failed = false;
        ShaderError error;
    };

    PipelineHandle add(std::string name, std::vector<ShaderEntryPoint> entryPoints, Builder build);
    void rebuild(Entry& entry, bool initial, uint64_t lastSubmittedFrame);
    bool changedOnDisk(const Entry& entry) const;
    void refreshErrors();
    VkPipeline buildGraphics(const GraphicsPipelineDesc& desc, std::span<const std::vector<uint32_t>> spirv) const;
    VkPipeline buildCompute(const ComputePipelineDesc& desc, std::span<const std::vector<uint32_t>> spirv) const;
    VkShaderModule createModule(const std::vector<uint32_t>& spirv) const;

    const vulkan::Device* m_device = nullptr;
    const vulkan::BindlessDescriptors* m_bindless = nullptr;
    vulkan::DeletionQueue* m_deletionQueue = nullptr;
    ShaderCompiler* m_compiler = nullptr;
    std::vector<Entry> m_entries;
    std::vector<ShaderError> m_errors;
    uint64_t m_reloadCount = 0;
    std::chrono::steady_clock::time_point m_lastPoll{};
};

} // namespace ghost::graphics::shader
