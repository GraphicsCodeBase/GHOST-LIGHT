// Turns VK_ERROR_DEVICE_LOST into a readable report: the last GPU checkpoints (which pass was running) and what to check.
#pragma once

#include <volk.h>

namespace ghost::graphics::vulkan {

class GpuCrashReporter {
public:
    static void initialize(VkDevice device, VkQueue queue, bool checkpointsAvailable);

    // Records a breadcrumb the GPU passes when it reaches this point in the command buffer.
    // The label pointer is stored by the driver, so it must stay valid (string literals or long-lived pass names).
    static void checkpoint(VkCommandBuffer cmd, const char* label);

    // Logs the report and stops the process with exit code 4.
    [[noreturn]] static void reportDeviceLost(const char* where);
};

} // namespace ghost::graphics::vulkan
