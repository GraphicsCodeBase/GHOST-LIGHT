// PNG = signature + IHDR + IDAT (zlib stream of filter byte + row, stored blocks) + IEND, each chunk CRC32-checked.
#include "DebugTools/PngWriter.h"

#include "Core/Paths.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <system_error>
#include <vector>

namespace ghost::debugtools {

namespace {

uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0xFFFFFFFFu) {
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            t[n] = c;
        }
        return t;
    }();
    for (size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc;
}

void appendBigEndian(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

void appendChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& data) {
    appendBigEndian(out, static_cast<uint32_t>(data.size()));
    const size_t typeStart = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    appendBigEndian(out, crc32(out.data() + typeStart, out.size() - typeStart) ^ 0xFFFFFFFFu);
}

} // namespace

bool PngWriter::writeRgba8(const std::filesystem::path& path, uint32_t width, uint32_t height, const uint8_t* rgba, std::string& error) {
    // Raw scanlines: filter type 0 (none) + RGBA row.
    const size_t rowBytes = static_cast<size_t>(width) * 4;
    std::vector<uint8_t> raw;
    raw.reserve((rowBytes + 1) * height);
    for (uint32_t y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgba + y * rowBytes, rgba + (y + 1) * rowBytes);
    }

    // zlib stream with stored deflate blocks (max 65535 bytes each) and an Adler-32 trailer.
    std::vector<uint8_t> zlib = {0x78, 0x01};
    uint32_t adlerA = 1;
    uint32_t adlerB = 0;
    for (size_t offset = 0; offset < raw.size() || raw.empty();) {
        const size_t length = std::min<size_t>(65535, raw.size() - offset);
        const bool last = offset + length >= raw.size();
        zlib.push_back(last ? 1 : 0);
        zlib.push_back(static_cast<uint8_t>(length));
        zlib.push_back(static_cast<uint8_t>(length >> 8));
        zlib.push_back(static_cast<uint8_t>(~length));
        zlib.push_back(static_cast<uint8_t>(~length >> 8));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + length));
        for (size_t i = offset; i < offset + length; ++i) {
            adlerA = (adlerA + raw[i]) % 65521u;
            adlerB = (adlerB + adlerA) % 65521u;
        }
        offset += length;
        if (last) {
            break;
        }
    }
    appendBigEndian(zlib, (adlerB << 16) | adlerA);

    std::vector<uint8_t> header;
    appendBigEndian(header, width);
    appendBigEndian(header, height);
    header.insert(header.end(), {8, 6, 0, 0, 0}); // 8 bits, RGBA, deflate, adaptive filtering, no interlace

    std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    appendChunk(png, "IHDR", header);
    appendChunk(png, "IDAT", zlib);
    appendChunk(png, "IEND", {});

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        error = "cannot write " + core::Paths::display(path);
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(file);
}

} // namespace ghost::debugtools
