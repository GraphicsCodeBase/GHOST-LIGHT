// Minimal PNG encoder (8-bit RGBA, uncompressed "stored" deflate blocks): screenshots without an extra library.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace ghost::debugtools {

class PngWriter {
public:
    // rgba: width * height * 4 bytes, rows top to bottom. Creates missing folders. Returns false and fills error on failure.
    static bool writeRgba8(const std::filesystem::path& path, uint32_t width, uint32_t height, const uint8_t* rgba, std::string& error);
};

} // namespace ghost::debugtools
