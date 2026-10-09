// Root-relative path system: finds the repo root from the executable's location and resolves every engine path from it.
#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ghost::core {

class Paths {
public:
    // Walks up from the executable's folder to the nearest folder containing run.bat (the repo root).
    // Returns false when the executable is not inside a GHOST LIGHT checkout.
    static bool initialize();

    static const std::filesystem::path& root();
    static std::filesystem::path content();       // Content/: scenes, prefabs, assets
    static std::filesystem::path user();          // User/: machine-specific state (gitignored)
    static std::filesystem::path build();         // Build/: logs and build output (gitignored)
    static std::filesystem::path tools();         // .tools/: downloaded tools such as Slang and validation layers
    static std::filesystem::path shaderLibrary(); // ShaderLibrary/: shared Slang modules
    static std::filesystem::path techniques();    // Techniques/: technique folders

    // Resolves a path written in a scene or prefab file (UTF-8, relative to Content/).
    static std::filesystem::path resolveContent(std::string_view relativeUtf8);

    // Root-relative, forward-slash form for messages, e.g. "Content/Scenes/Sponza.scene.json".
    static std::string display(const std::filesystem::path& path);

    // UTF-8 conversions. std::filesystem::path::string() uses the ANSI code page and breaks non-ASCII user names.
    static std::string toUtf8(const std::filesystem::path& path);
    static std::filesystem::path fromUtf8(std::string_view utf8);
};

} // namespace ghost::core
