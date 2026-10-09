// Loader environment setup (process-local: SetEnvironmentVariable never touches the user's system settings).
#include "Graphics/Vulkan/LoaderConfiguration.h"

#include "Core/Log.h"
#include "Core/Paths.h"

#include <filesystem>
#include <system_error>

#include <windows.h>

namespace ghost::graphics::vulkan {

namespace {

// Implicit layers that are pure overlays/capture hooks. Loader filter syntax: comma list, '*' wildcards.
constexpr const wchar_t* kDisabledLayers = L"*steam*,*eos*,*obs*,*rtss*,*bandicam*,*overlay*,*fossilize*";

bool isSet(const wchar_t* name) {
    return GetEnvironmentVariableW(name, nullptr, 0) != 0;
}

} // namespace

bool LoaderConfiguration::apply() {
    if (!isSet(L"VK_LOADER_LAYERS_DISABLE")) {
        SetEnvironmentVariableW(L"VK_LOADER_LAYERS_DISABLE", kDisabledLayers);
    }

    // VK_LAYER_PATH replaces the explicit-layer search (registry, installed SDKs) instead of adding to it.
    // Set it even when the folder is missing: an old SDK layer must never stand in for the pinned one.
    const std::filesystem::path layerDir = core::Paths::tools() / "validation-layers";
    SetEnvironmentVariableW(L"VK_LAYER_PATH", layerDir.c_str());

    std::error_code ec;
    return std::filesystem::exists(layerDir / "VkLayer_khronos_validation.json", ec);
}

} // namespace ghost::graphics::vulkan
