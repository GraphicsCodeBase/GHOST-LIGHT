// JSON file reading and writing with error messages that point at the exact file, line and column.
#include "Core/JsonFile.h"

#include "Core/Paths.h"

#include <fstream>
#include <iterator>
#include <system_error>

namespace ghost::core {

namespace {

// nlohmann messages start with "[json.exception.parse_error.101] "; the id means nothing to a reader.
std::string withoutExceptionId(const char* what) {
    std::string message = what;
    if (!message.empty() && message.front() == '[') {
        const size_t close = message.find("] ");
        if (close != std::string::npos) {
            message.erase(0, close + 2);
        }
    }
    return message;
}

} // namespace

JsonFile::LoadResult JsonFile::load(const std::filesystem::path& path) {
    LoadResult result;
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        result.error = Paths::display(path) + ": cannot open the file (does it exist?)";
        return result;
    }
    result.text.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

    try {
        result.value = nlohmann::json::parse(result.text, nullptr, /*allow_exceptions*/ true, /*ignore_comments*/ true);
        result.ok = true;
    } catch (const nlohmann::json::exception& e) {
        result.error = Paths::display(path) + ": " + withoutExceptionId(e.what());
    }
    return result;
}

bool JsonFile::save(const std::filesystem::path& path, const nlohmann::json& value, std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = Paths::display(temporary) + ": cannot write the file";
            return false;
        }
        stream << value.dump(2) << '\n';
        if (!stream) {
            error = Paths::display(temporary) + ": write failed (disk full?)";
            return false;
        }
    }
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        error = Paths::display(path) + ": cannot replace the file (" + ec.message() + ")";
        return false;
    }
    return true;
}

} // namespace ghost::core
