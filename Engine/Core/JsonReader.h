// Reads typed fields out of parsed JSON and reports problems as "file(line): field.path: message" without throwing.
// One Context collects every problem of a file, so a single load shows all of them at once.
#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nlohmann/json.hpp>

namespace ghost::core {

class JsonReader {
public:
    struct Context {
        std::string file;        // display path, e.g. "Content/Scenes/Sponza.scene.json"
        std::string text;        // the file's text, for turning byte offsets into line numbers
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        int lineOf(size_t byteOffset) const;
    };

    JsonReader(Context& context, const nlohmann::json& value, std::string path);

    bool exists() const { return !m_value->is_null(); }
    bool isObject() const { return m_value->is_object(); }
    bool isArray() const { return m_value->is_array(); }
    bool has(std::string_view key) const;
    size_t size() const;
    std::vector<std::string> keys() const;
    const nlohmann::json& raw() const { return *m_value; }
    const std::string& path() const { return m_path; }

    // Missing key -> a reader whose exists() is false.
    JsonReader child(std::string_view key) const;
    JsonReader element(size_t index) const;

    // Missing key -> fallback, silently. Present but wrong type -> error + fallback.
    std::string string(std::string_view key, const std::string& fallback = {}) const;
    float number(std::string_view key, float fallback) const;
    int integer(std::string_view key, int fallback) const;
    bool boolean(std::string_view key, bool fallback) const;
    glm::vec2 vec2(std::string_view key, glm::vec2 fallback) const;
    glm::vec3 vec3(std::string_view key, glm::vec3 fallback) const;
    glm::vec4 vec4(std::string_view key, glm::vec4 fallback) const;

    // True when this value is an object; otherwise reports `what` as expected.
    bool expectObject(std::string_view what) const;
    // Warns about keys that are not in `known` (catches typos like "posiiton").
    void warnUnknownKeys(std::initializer_list<std::string_view> known) const;

    void error(std::string_view message) const;
    void warning(std::string_view message) const;

private:
    std::string location() const;
    template <int N>
    bool readFloats(std::string_view key, float (&out)[N]) const;

    Context* m_context;
    const nlohmann::json* m_value;
    std::string m_path;
};

} // namespace ghost::core
