// Loads and saves JSON files with readable errors (file, line, column) instead of exceptions escaping to callers.
#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace ghost::core {

class JsonFile {
public:
    struct LoadResult {
        bool ok = false;
        nlohmann::json value;
        std::string error; // e.g. "Content/Scenes/X.scene.json: parse error at line 4, column 7: ..."
    };

    // Parses the file. // and /* */ comments are allowed so hand-edited scene files can be annotated.
    static LoadResult load(const std::filesystem::path& path);

    // Writes with 2-space indentation via a temporary file, so a crash never leaves a half-written file.
    static bool save(const std::filesystem::path& path, const nlohmann::json& value, std::string& error);
};

} // namespace ghost::core
