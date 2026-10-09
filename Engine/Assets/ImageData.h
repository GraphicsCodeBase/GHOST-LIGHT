// Decoded image pixels on the CPU (RGBA8 or RGBA32F), waiting to be uploaded to the GPU.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ghost::assets {

struct ImageData {
    enum class Format {
        Rgba8,       // linear data: normal maps, metal/roughness
        Rgba8Srgb,   // color: base color, emissive
        Rgba32Float, // HDR: environment maps
    };

    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    Format format = Format::Rgba8;
    std::vector<uint8_t> pixels; // tightly packed rows, top to bottom

    bool valid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

} // namespace ghost::assets
