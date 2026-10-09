// VK_CHECK: wraps Vulkan calls that must succeed; a failure logs the call, the result and the location, then stops.
#pragma once

#include <volk.h>

namespace ghost::graphics::vulkan {

const char* resultToString(VkResult result);

// VK_ERROR_DEVICE_LOST is routed to GpuCrashReporter so the report names the pass the GPU was running.
[[noreturn]] void vulkanCallFailed(VkResult result, const char* call, const char* file, int line);

} // namespace ghost::graphics::vulkan

// Negative VkResults are errors; positive ones (VK_INCOMPLETE, VK_SUBOPTIMAL_KHR, ...) are not.
#define VK_CHECK(call)                                                                              \
    do {                                                                                            \
        const VkResult ghostVkResult_ = (call);                                                     \
        if (ghostVkResult_ < 0) {                                                                   \
            ::ghost::graphics::vulkan::vulkanCallFailed(ghostVkResult_, #call, __FILE__, __LINE__); \
        }                                                                                           \
    } while (false)
