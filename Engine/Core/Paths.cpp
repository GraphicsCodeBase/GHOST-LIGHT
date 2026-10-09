// Repo-root discovery and path helpers. No absolute path is ever stored in a committed file.
#include "Core/Paths.h"

#include "Core/Assert.h"

#include <system_error>

#include <windows.h>

namespace ghost::core {

namespace {

std::filesystem::path g_root;

} // namespace

bool Paths::initialize() {
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return false;
    }
    buffer.resize(length);

    std::filesystem::path current = std::filesystem::path(buffer).parent_path();
    for (;;) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(current / "run.bat", ec)) {
            g_root = current;
            return true;
        }
        const std::filesystem::path parent = current.parent_path();
        if (parent == current) {
            return false;
        }
        current = parent;
    }
}

const std::filesystem::path& Paths::root() {
    GHOST_ASSERT(!g_root.empty(), "Paths::initialize() must succeed before paths are used");
    return g_root;
}

std::filesystem::path Paths::content() {
    return root() / "Content";
}

std::filesystem::path Paths::user() {
    return root() / "User";
}

std::filesystem::path Paths::build() {
    return root() / "Build";
}

std::filesystem::path Paths::tools() {
    return root() / ".tools";
}

std::filesystem::path Paths::shaderLibrary() {
    return root() / "ShaderLibrary";
}

std::filesystem::path Paths::techniques() {
    return root() / "Techniques";
}

std::filesystem::path Paths::resolveContent(std::string_view relativeUtf8) {
    return (content() / fromUtf8(relativeUtf8)).lexically_normal();
}

std::string Paths::display(const std::filesystem::path& path) {
    if (!g_root.empty()) {
        const std::filesystem::path relative = path.lexically_proximate(g_root);
        if (!relative.empty() && *relative.begin() != "..") {
            return toUtf8(relative.generic_wstring());
        }
    }
    return toUtf8(path.generic_wstring());
}

std::string Paths::toUtf8(const std::filesystem::path& path) {
    const std::wstring& wide = path.native();
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), size, nullptr, nullptr);
    return utf8;
}

std::filesystem::path Paths::fromUtf8(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return std::filesystem::path(wide);
}

} // namespace ghost::core
