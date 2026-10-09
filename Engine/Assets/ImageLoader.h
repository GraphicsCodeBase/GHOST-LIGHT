// Decodes PNG/JPG (to RGBA8) and Radiance .hdr (to RGBA32F) with stb_image.
#pragma once

#include "Assets/ImageData.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace ghost::assets {

class ImageLoader {
public:
    static bool loadFromMemory(const uint8_t* bytes, size_t size, ImageData::Format format, std::string name, ImageData& out, std::string& error);
    static bool loadFile(const std::filesystem::path& path, ImageData::Format format, ImageData& out, std::string& error);
    // Equirectangular environment maps: always RGBA32F.
    static bool loadHdrFile(const std::filesystem::path& path, ImageData& out, std::string& error);
};

} // namespace ghost::assets
