// Param<T>: a technique setting that shows up in the technique panel and loads from the scene file. Declare it as a
// member: `Param<float> radius{"Radius", 1.0f, 0.1f, 5.0f};` then read it like a value (`float r = radius;`).
// Supported types: bool, int, float, glm::vec3 (a color).
#pragma once

#include <string>
#include <type_traits>

#include <glm/vec3.hpp>
#include <nlohmann/json_fwd.hpp>

namespace ghost::graphics::techniques {

class Technique;

class ParamBase {
public:
    enum class Type { Bool, Int, Float, Color };

    virtual ~ParamBase() = default;
    ParamBase(const ParamBase&) = delete;
    ParamBase& operator=(const ParamBase&) = delete;

    const std::string& label() const { return m_label; }
    // Scene-file key: the label in lowerCamelCase ("Rays per pixel" -> "raysPerPixel").
    const std::string& key() const { return m_key; }
    Type type() const { return m_type; }
    float minimum() const { return m_minimum; }
    float maximum() const { return m_maximum; }

    // For UI widgets: points at a bool, int, float or float[3] depending on type().
    virtual void* data() = 0;
    virtual void reset() = 0;
    // False (and unchanged) when the JSON value has the wrong type.
    virtual bool fromJson(const nlohmann::json& value) = 0;

    // Called by Technique's constructor: Params constructed next belong to `owner`.
    static void beginRegistration(Technique* owner);

protected:
    ParamBase(const char* label, Type type, float minimum, float maximum);

private:
    std::string m_label;
    std::string m_key;
    Type m_type;
    float m_minimum;
    float m_maximum;
};

template <typename T>
class Param : public ParamBase {
    static_assert(std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, glm::vec3>,
                  "Param<T> supports bool, int, float and glm::vec3");

public:
    // Numbers: label, default, range. Bools and colors: label, default.
    Param(const char* label, T defaultValue, float minimum = 0.0f, float maximum = 1.0f)
        : ParamBase(label, typeOf(), minimum, maximum), m_value(defaultValue), m_default(defaultValue) {}

    operator T() const { return m_value; }
    T get() const { return m_value; }
    Param& operator=(T value) {
        m_value = value;
        return *this;
    }

    void* data() override { return &m_value; }
    void reset() override { m_value = m_default; }
    bool fromJson(const nlohmann::json& value) override;

private:
    static constexpr Type typeOf() {
        if constexpr (std::is_same_v<T, bool>) {
            return Type::Bool;
        } else if constexpr (std::is_same_v<T, int>) {
            return Type::Int;
        } else if constexpr (std::is_same_v<T, float>) {
            return Type::Float;
        } else {
            return Type::Color;
        }
    }

    T m_value;
    T m_default;
};

// Defined in Param.cpp for every supported type.
template <>
bool Param<bool>::fromJson(const nlohmann::json& value);
template <>
bool Param<int>::fromJson(const nlohmann::json& value);
template <>
bool Param<float>::fromJson(const nlohmann::json& value);
template <>
bool Param<glm::vec3>::fromJson(const nlohmann::json& value);
extern template class Param<bool>;
extern template class Param<int>;
extern template class Param<float>;
extern template class Param<glm::vec3>;

} // namespace ghost::graphics::techniques
