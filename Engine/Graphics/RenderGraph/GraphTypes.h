// Render graph vocabulary: pass kinds, resource lifetimes and sizes, texture/buffer descriptions and handles.
#pragma once

#include <volk.h>

#include <cstdint>

namespace ghost::graphics::rendergraph {

// Decides which pipeline stages a pass's resource accesses synchronize with.
enum class PassKind { Raster, Compute, RayTracing, Transfer };

enum class Lifetime {
    Transient,  // contents are undefined at the start of every frame
    Persistent, // contents survive from frame to frame (e.g. path tracer accumulation)
    History,    // two copies swapped every frame: write this frame's, read last frame's with samplePrevious()
};

// Size relative to the render resolution (Fixed uses width/height).
enum class Scale { Full, Half, Quarter, Fixed };

struct TextureDesc {
    VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    Scale scale = Scale::Full;
    Lifetime lifetime = Lifetime::Transient;
    uint32_t width = 0;  // Scale::Fixed only
    uint32_t height = 0;
};

struct BufferDesc {
    VkDeviceSize size = 0;
    Lifetime lifetime = Lifetime::Persistent;
};

enum class LoadOp { Clear, Load, DontCare };

struct TextureHandle {
    uint32_t index = UINT32_MAX;
    bool previous = false; // last frame's copy of a History texture
    bool valid() const { return index != UINT32_MAX; }
};

struct BufferHandle {
    uint32_t index = UINT32_MAX;
    bool valid() const { return index != UINT32_MAX; }
};

} // namespace ghost::graphics::rendergraph
