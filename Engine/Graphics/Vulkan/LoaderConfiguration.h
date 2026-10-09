// Configures the Vulkan loader for this process before it loads: validation layers from .tools/, overlay layers kept out.
#pragma once

namespace ghost::graphics::vulkan {

class LoaderConfiguration {
public:
    // Must run before volkInitialize(). Points the loader's explicit-layer search at .tools/validation-layers
    // (so an installed Vulkan SDK can never be picked up instead) and disables known overlay layers
    // (Steam, Epic, OBS, RTSS, ...) that inject into every Vulkan app and produce errors that aren't ours.
    // RenderDoc and Nsight keep working because only overlays are disabled.
    // Returns true when the repo's validation layer files are present.
    static bool apply();
};

} // namespace ghost::graphics::vulkan
