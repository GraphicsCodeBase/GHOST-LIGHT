// VK_EXT_debug_utils object names and labels; no-ops when the extension is not enabled.
#include "Graphics/Vulkan/DebugUtils.h"

#include <string>

namespace ghost::graphics::vulkan {

namespace {

VkDevice g_device = VK_NULL_HANDLE;
bool g_available = false;

} // namespace

void DebugUtils::initialize(VkDevice device, bool available) {
    g_device = device;
    g_available = available && vkSetDebugUtilsObjectNameEXT != nullptr && vkCmdBeginDebugUtilsLabelEXT != nullptr;
}

void DebugUtils::setName(VkObjectType type, uint64_t handle, std::string_view name) {
    if (!g_available || handle == 0) {
        return;
    }
    const std::string terminated(name);
    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.objectType = type;
    info.objectHandle = handle;
    info.pObjectName = terminated.c_str();
    vkSetDebugUtilsObjectNameEXT(g_device, &info);
}

void DebugUtils::beginLabel(VkCommandBuffer cmd, std::string_view name) {
    if (!g_available) {
        return;
    }
    const std::string terminated(name);
    VkDebugUtilsLabelEXT label{VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    label.pLabelName = terminated.c_str();
    vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
}

void DebugUtils::endLabel(VkCommandBuffer cmd) {
    if (g_available) {
        vkCmdEndDebugUtilsLabelEXT(cmd);
    }
}

} // namespace ghost::graphics::vulkan
