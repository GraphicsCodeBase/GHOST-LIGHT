// Param registration (a thread-local "technique under construction") and JSON reading per type.
#include "Graphics/TechniqueRuntime/Param.h"

#include "Graphics/TechniqueRuntime/Technique.h"

#include <cctype>

#include <nlohmann/json.hpp>

namespace ghost::graphics::techniques {

namespace {

thread_local Technique* t_owner = nullptr;

std::string lowerCamelCase(const std::string& label) {
    std::string key;
    bool upperNext = false;
    for (const char c : label) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            upperNext = !key.empty();
            continue;
        }
        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        key += upperNext ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : (key.empty() ? lower : c);
        upperNext = false;
    }
    return key;
}

} // namespace

void ParamBase::beginRegistration(Technique* owner) {
    t_owner = owner;
}

ParamBase::ParamBase(const char* label, Type type, float minimum, float maximum)
    : m_label(label), m_key(lowerCamelCase(label)), m_type(type), m_minimum(minimum), m_maximum(maximum) {
    if (t_owner) {
        t_owner->m_params.push_back(this);
    }
}

template <>
bool Param<bool>::fromJson(const nlohmann::json& value) {
    if (!value.is_boolean()) {
        return false;
    }
    m_value = value.get<bool>();
    return true;
}

template <>
bool Param<int>::fromJson(const nlohmann::json& value) {
    if (!value.is_number_integer()) {
        return false;
    }
    m_value = value.get<int>();
    return true;
}

template <>
bool Param<float>::fromJson(const nlohmann::json& value) {
    if (!value.is_number()) {
        return false;
    }
    m_value = value.get<float>();
    return true;
}

template <>
bool Param<glm::vec3>::fromJson(const nlohmann::json& value) {
    if (!value.is_array() || value.size() != 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number()) {
        return false;
    }
    m_value = glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
    return true;
}

template class Param<bool>;
template class Param<int>;
template class Param<float>;
template class Param<glm::vec3>;

} // namespace ghost::graphics::techniques
