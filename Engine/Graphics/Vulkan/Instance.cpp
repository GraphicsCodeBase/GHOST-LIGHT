// Instance creation through vk-bootstrap, with the validation layer from .tools/ and synchronization validation in Debug.
#include "Graphics/Vulkan/Instance.h"

#include "Core/Log.h"
#include "Graphics/Vulkan/LoaderConfiguration.h"

#include <cstring>

#include <windows.h>

namespace ghost::graphics::vulkan {

namespace {

// Validation messages become engine log entries: errors fail the smoke test, warnings stay visible.
VKAPI_ATTR VkBool32 VKAPI_CALL onValidationMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* data, void* /*userData*/) {
    const char* id = data->pMessageIdName ? data->pMessageIdName : "";
    // The loader confirming it disabled an overlay layer (LoaderConfiguration asked it to) is expected, not a warning.
    if (std::strstr(data->pMessage, "forced disabled because name matches filter") != nullptr) {
        core::Log::info("[Vulkan loader] {}", data->pMessage);
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        core::Log::error("[Vulkan validation] {}\n           {}", id, data->pMessage);
        if (IsDebuggerPresent()) {
            __debugbreak(); // the call stack in the debugger points at the offending Vulkan call
        }
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        core::Log::warning("[Vulkan validation] {}\n           {}", id, data->pMessage);
    }
    return VK_FALSE;
}

} // namespace

Instance::~Instance() {
    destroy();
}

bool Instance::create(bool wantValidation) {
    const bool layerFilesPresent = LoaderConfiguration::apply();

    if (volkInitialize() != VK_SUCCESS) {
        core::Log::error("The Vulkan runtime (vulkan-1.dll) could not be loaded. Install or update the NVIDIA driver.");
        return false;
    }
    if (volkGetInstanceVersion() < VK_API_VERSION_1_3) {
        core::Log::error("The Vulkan driver only supports Vulkan {}.{}; GHOST LIGHT needs 1.3. Update the GPU driver.",
                         VK_API_VERSION_MAJOR(volkGetInstanceVersion()), VK_API_VERSION_MINOR(volkGetInstanceVersion()));
        return false;
    }

    auto system = vkb::SystemInfo::get_system_info(vkGetInstanceProcAddr);
    if (!system) {
        core::Log::error("Could not query Vulkan instance information: {}", system.error().message());
        return false;
    }

    vkb::InstanceBuilder builder(vkGetInstanceProcAddr);
    builder.set_app_name("GHOST LIGHT")
        .set_engine_name("GHOST LIGHT")
        .require_api_version(1, 3, 0)
        .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME)
        .enable_extension("VK_KHR_win32_surface");

    m_validationEnabled = wantValidation && system->validation_layers_available;
    if (m_validationEnabled) {
        for (const VkLayerProperties& layer : system->available_layers) {
            if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
                core::Log::info("Validation layer {}.{}.{} from .tools/validation-layers", VK_API_VERSION_MAJOR(layer.specVersion),
                                VK_API_VERSION_MINOR(layer.specVersion), VK_API_VERSION_PATCH(layer.specVersion));
            }
        }
        builder.request_validation_layers(true).set_debug_callback(onValidationMessage);
        builder.set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
        // Synchronization validation catches missing or wrong barriers, the bug class a render graph exists to prevent.
        if (system->is_extension_available(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)) {
            builder.enable_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)
                .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
        }
        m_debugUtilsEnabled = system->debug_utils_available; // vk-bootstrap enables it for the messenger
    } else if (system->debug_utils_available) {
        // Object names still help RenderDoc and Nsight when validation is off.
        builder.enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        m_debugUtilsEnabled = true;
    }

    if (wantValidation && !m_validationEnabled) {
        core::Log::warning("{}", layerFilesPresent
                                     ? "Validation layers are in .tools/validation-layers but the loader did not accept them; running without validation."
                                     : "Validation layers are not installed (.tools/validation-layers). Run run.bat to install them; running without validation.");
    }

    auto built = builder.build();
    if (!built) {
        core::Log::error("Could not create the Vulkan instance: {}", built.error().message());
        return false;
    }
    m_instance = built.value();
    m_created = true;
    volkLoadInstance(m_instance.instance);

    core::Log::info("Vulkan instance {}.{} created; validation {}", VK_API_VERSION_MAJOR(volkGetInstanceVersion()),
                    VK_API_VERSION_MINOR(volkGetInstanceVersion()), m_validationEnabled ? "ON (incl. synchronization)" : "off");
    return true;
}

void Instance::destroy() {
    if (m_created) {
        vkb::destroy_instance(m_instance);
        m_created = false;
    }
}

} // namespace ghost::graphics::vulkan
