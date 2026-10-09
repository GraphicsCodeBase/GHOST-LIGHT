// stb_image wrappers. Files are read through wide paths so non-ASCII folder names work.
#include "Assets/ImageLoader.h"

#include "Core/Paths.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#include <stb_image.h>

namespace ghost::assets {

namespace {

// One block read: character-by-character stream iteration is very slow and contends badly when many threads load at once.
bool readFile(const std::filesystem::path& path, std::vector<uint8_t>& bytes, std::string& error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error = core::Paths::display(path) + ": cannot open (missing asset? run.bat downloads assets listed in Content/AssetManifest.json)";
        return false;
    }
    bytes.resize(static_cast<size_t>(file.tellg()));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
        error = core::Paths::display(path) + ": read error";
        return false;
    }
    return true;
}

} // namespace

bool ImageLoader::loadFromMemory(const uint8_t* bytes, size_t size, ImageData::Format format, std::string name, ImageData& out, std::string& error) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(bytes, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels) {
        error = name + ": cannot decode image (" + stbi_failure_reason() + ")";
        return false;
    }
    out.name = std::move(name);
    out.width = static_cast<uint32_t>(width);
    out.height = static_cast<uint32_t>(height);
    out.format = format;
    out.pixels.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);
    stbi_image_free(pixels);
    return true;
}

bool ImageLoader::loadFile(const std::filesystem::path& path, ImageData::Format format, ImageData& out, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes, error)) {
        return false;
    }
    return loadFromMemory(bytes.data(), bytes.size(), format, core::Paths::display(path), out, error);
}

bool ImageLoader::loadHdrFile(const std::filesystem::path& path, ImageData& out, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes, error)) {
        return false;
    }
    int width = 0;
    int height = 0;
    int channels = 0;
    float* pixels = stbi_loadf_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
    if (!pixels) {
        error = core::Paths::display(path) + ": cannot decode HDR image (" + stbi_failure_reason() + ")";
        return false;
    }
    out.name = core::Paths::display(path);
    out.width = static_cast<uint32_t>(width);
    out.height = static_cast<uint32_t>(height);
    out.format = ImageData::Format::Rgba32Float;
    const size_t byteCount = static_cast<size_t>(width) * height * 4 * sizeof(float);
    out.pixels.resize(byteCount);
    std::memcpy(out.pixels.data(), pixels, byteCount);
    stbi_image_free(pixels);
    return true;
}

} // namespace ghost::assets
