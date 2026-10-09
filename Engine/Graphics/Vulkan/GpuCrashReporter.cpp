// GPU crash report via VK_NV_device_diagnostic_checkpoints: breadcrumbs in command buffers, read back after a device loss.
#include "Graphics/Vulkan/GpuCrashReporter.h"

#include "Core/Log.h"

#include <vector>

#include <windows.h>

namespace ghost::graphics::vulkan {

namespace {

VkDevice g_device = VK_NULL_HANDLE;
VkQueue g_queue = VK_NULL_HANDLE;
bool g_checkpoints = false;

const char* stageName(VkPipelineStageFlags stage) {
    if (stage & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) return "started";
    if (stage & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT) return "finished";
    return "in progress";
}

} // namespace

void GpuCrashReporter::initialize(VkDevice device, VkQueue queue, bool checkpointsAvailable) {
    g_device = device;
    g_queue = queue;
    g_checkpoints = checkpointsAvailable && vkCmdSetCheckpointNV != nullptr && vkGetQueueCheckpointDataNV != nullptr;
}

void GpuCrashReporter::checkpoint(VkCommandBuffer cmd, const char* label) {
    if (g_checkpoints) {
        vkCmdSetCheckpointNV(cmd, label);
    }
}

void GpuCrashReporter::reportDeviceLost(const char* where) {
    core::Log::error("GPU crash or hang (VK_ERROR_DEVICE_LOST), noticed in {}.", where);
    if (g_checkpoints && g_queue != VK_NULL_HANDLE) {
        uint32_t count = 0;
        vkGetQueueCheckpointDataNV(g_queue, &count, nullptr);
        std::vector<VkCheckpointDataNV> data(count, VkCheckpointDataNV{VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV});
        vkGetQueueCheckpointDataNV(g_queue, &count, data.data());
        if (count == 0) {
            core::Log::error("  No GPU checkpoints were reached in the failing submission.");
        }
        for (const VkCheckpointDataNV& checkpoint : data) {
            const char* label = checkpoint.pCheckpointMarker ? static_cast<const char*>(checkpoint.pCheckpointMarker) : "?";
            core::Log::error("  last checkpoint: pass \"{}\" ({})", label, stageName(checkpoint.stage));
        }
    } else {
        core::Log::error("  (GPU checkpoints unavailable on this driver: the failing pass cannot be named.)");
    }
    core::Log::error(
        "  What to check: an infinite loop or very long loop in the shader of that pass (Windows resets the GPU after ~2 s),\n"
        "  out-of-bounds buffer-device-address reads, an index past the end of a bindless array, or a TLAS/BLAS used while\n"
        "  being rebuilt. Re-run in Debug (validation on) and capture with Nsight Graphics for the exact shader.");
    core::Log::closeFile();
    TerminateProcess(GetCurrentProcess(), 4);
    for (;;) {
    }
}

} // namespace ghost::graphics::vulkan
