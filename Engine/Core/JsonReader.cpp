// JsonReader: typed field access with path tracking; line numbers come from nlohmann's diagnostic positions.
#include "Core/JsonReader.h"

#include <algorithm>

namespace ghost::core {

namespace {

const nlohmann::json& nullJson() {
    static const nlohmann::json value = nullptr;
    return value;
}

const char* typeName(const nlohmann::json& value) {
    return value.type_name();
}

} // namespace

int JsonReader::Context::lineOf(size_t byteOffset) const {
    if (byteOffset == std::string::npos || byteOffset > text.size()) {
        return 0;
    }
    return 1 + static_cast<int>(std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(byteOffset), '\n'));
}

JsonReader::JsonReader(Context& context, const nlohmann::json& value, std::string path)
    : m_context(&context), m_value(&value), m_path(std::move(path)) {}

bool JsonReader::has(std::string_view key) const {
    return m_value->is_object() && m_value->contains(key);
}

size_t JsonReader::size() const {
    return (m_value->is_array() || m_value->is_object()) ? m_value->size() : 0;
}

std::vector<std::string> JsonReader::keys() const {
    std::vector<std::string> result;
    if (m_value->is_object()) {
        for (auto it = m_value->begin(); it != m_value->end(); ++it) {
            result.push_back(it.key());
        }
    }
    return result;
}

JsonReader JsonReader::child(std::string_view key) const {
    const std::string childPath = m_path.empty() ? std::string(key) : m_path + "." + std::string(key);
    if (!has(key)) {
        return JsonReader(*m_context, nullJson(), childPath);
    }
    return JsonReader(*m_context, (*m_value)[std::string(key)], childPath);
}

JsonReader JsonReader::element(size_t index) const {
    const std::string childPath = m_path + "[" + std::to_string(index) + "]";
    if (!m_value->is_array() || index >= m_value->size()) {
        return JsonReader(*m_context, nullJson(), childPath);
    }
    return JsonReader(*m_context, (*m_value)[index], childPath);
}

std::string JsonReader::location() const {
    // Values without a position (missing keys) report the file only.
    size_t position = std::string::npos;
#if JSON_DIAGNOSTIC_POSITIONS
    position = m_value->start_pos();
#endif
    const int line = m_context->lineOf(position);
    std::string where = m_context->file;
    if (line > 0) {
        where += "(" + std::to_string(line) + ")";
    }
    return where + ": " + (m_path.empty() ? std::string("(root)") : m_path);
}

void JsonReader::error(std::string_view message) const {
    m_context->errors.push_back(location() + ": " + std::string(message));
}

void JsonReader::warning(std::string_view message) const {
    m_context->warnings.push_back(location() + ": " + std::string(message));
}

bool JsonReader::expectObject(std::string_view what) const {
    if (m_value->is_object()) {
        return true;
    }
    error("expected " + std::string(what) + " (a { } object), got " + typeName(*m_value));
    return false;
}

void JsonReader::warnUnknownKeys(std::initializer_list<std::string_view> known) const {
    if (!m_value->is_object()) {
        return;
    }
    for (auto it = m_value->begin(); it != m_value->end(); ++it) {
        if (std::find(known.begin(), known.end(), std::string_view(it.key())) == known.end()) {
            std::string list;
            for (std::string_view k : known) {
                list += (list.empty() ? "" : ", ") + std::string(k);
            }
            child(it.key()).warning("unknown key, ignored (known keys: " + list + ")");
        }
    }
}

std::string JsonReader::string(std::string_view key, const std::string& fallback) const {
    const JsonReader field = child(key);
    if (!field.exists()) {
        return fallback;
    }
    if (!field.raw().is_string()) {
        field.error(std::string("expected a string, got ") + typeName(field.raw()));
        return fallback;
    }
    return field.raw().get<std::string>();
}

float JsonReader::number(std::string_view key, float fallback) const {
    const JsonReader field = child(key);
    if (!field.exists()) {
        return fallback;
    }
    if (!field.raw().is_number()) {
        field.error(std::string("expected a number, got ") + typeName(field.raw()));
        return fallback;
    }
    return field.raw().get<float>();
}

int JsonReader::integer(std::string_view key, int fallback) const {
    const JsonReader field = child(key);
    if (!field.exists()) {
        return fallback;
    }
    if (!field.raw().is_number_integer()) {
        field.error(std::string("expected a whole number, got ") + typeName(field.raw()));
        return fallback;
    }
    return field.raw().get<int>();
}

bool JsonReader::boolean(std::string_view key, bool fallback) const {
    const JsonReader field = child(key);
    if (!field.exists()) {
        return fallback;
    }
    if (!field.raw().is_boolean()) {
        field.error(std::string("expected true or false, got ") + typeName(field.raw()));
        return fallback;
    }
    return field.raw().get<bool>();
}

template <int N>
bool JsonReader::readFloats(std::string_view key, float (&out)[N]) const {
    const JsonReader field = child(key);
    if (!field.exists()) {
        return false;
    }
    const nlohmann::json& value = field.raw();
    if (!value.is_array() || value.size() != N ||
        !std::all_of(value.begin(), value.end(), [](const nlohmann::json& v) { return v.is_number(); })) {
        field.error("expected an array of " + std::to_string(N) + " numbers, e.g. " + (N == 2 ? "[0, 0]" : N == 3 ? "[0, 0, 0]" : "[0, 0, 0, 1]"));
        return false;
    }
    for (int i = 0; i < N; ++i) {
        out[i] = value[static_cast<size_t>(i)].get<float>();
    }
    return true;
}

glm::vec2 JsonReader::vec2(std::string_view key, glm::vec2 fallback) const {
    float v[2];
    return readFloats(key, v) ? glm::vec2(v[0], v[1]) : fallback;
}

glm::vec3 JsonReader::vec3(std::string_view key, glm::vec3 fallback) const {
    float v[3];
    return readFloats(key, v) ? glm::vec3(v[0], v[1], v[2]) : fallback;
}

glm::vec4 JsonReader::vec4(std::string_view key, glm::vec4 fallback) const {
    float v[4];
    return readFloats(key, v) ? glm::vec4(v[0], v[1], v[2], v[3]) : fallback;
}

} // namespace ghost::core
