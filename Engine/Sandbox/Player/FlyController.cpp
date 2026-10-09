// FlyController: mouse look while the right button is held (cursor captured), keyboard movement relative to the view.
#include "Sandbox/Player/FlyController.h"

#include "Platform/Window.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace ghost::sandbox {

namespace {

constexpr float kDegreesPerPixel = 0.12f;
constexpr float kMaxPitch = 89.0f;
constexpr float kWheelStep = 1.2f; // speed multiplier per wheel notch
constexpr float kMinSpeed = 0.1f;
constexpr float kMaxSpeed = 200.0f;

} // namespace

void FlyController::teleport(const glm::vec3& position, float yawDeg, float pitchDeg) {
    m_position = position;
    m_yawDeg = yawDeg;
    m_pitchDeg = std::clamp(pitchDeg, -kMaxPitch, kMaxPitch);
}

glm::vec3 FlyController::forward() const {
    const float yaw = glm::radians(m_yawDeg);
    const float pitch = glm::radians(m_pitchDeg);
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

glm::vec3 FlyController::right() const {
    const float yaw = glm::radians(m_yawDeg);
    return {std::cos(yaw), 0.0f, std::sin(yaw)};
}

void FlyController::update(platform::Window& window, float deltaSeconds, float& speed) {
    const platform::Input& input = window.input();

    if (input.wasPressed(platform::MouseButton::Right)) {
        m_looking = true;
        window.setCursorCaptured(true);
    }
    if (m_looking && !input.isDown(platform::MouseButton::Right)) {
        m_looking = false;
        window.setCursorCaptured(false);
    }
    if (m_looking) {
        const glm::vec2 delta = input.mouseDelta();
        m_yawDeg = std::fmod(m_yawDeg + delta.x * kDegreesPerPixel, 360.0f);
        m_pitchDeg = std::clamp(m_pitchDeg - delta.y * kDegreesPerPixel, -kMaxPitch, kMaxPitch);
    }

    const float wheel = input.wheelDelta();
    if (wheel != 0.0f) {
        speed = std::clamp(speed * std::pow(kWheelStep, wheel), kMinSpeed, kMaxSpeed);
    }

    glm::vec3 move(0.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (input.isDown(platform::Key::W)) {
        move += forward();
    }
    if (input.isDown(platform::Key::S)) {
        move -= forward();
    }
    if (input.isDown(platform::Key::D)) {
        move += right();
    }
    if (input.isDown(platform::Key::A)) {
        move -= right();
    }
    if (input.isDown(platform::Key::E)) {
        move += up;
    }
    if (input.isDown(platform::Key::Q)) {
        move -= up;
    }
    if (glm::dot(move, move) > 0.0f) {
        float multiplier = 1.0f;
        if (input.isShiftDown()) {
            multiplier *= 4.0f;
        }
        if (input.isControlDown()) {
            multiplier *= 0.25f;
        }
        m_position += glm::normalize(move) * speed * multiplier * deltaSeconds;
    }
}

} // namespace ghost::sandbox
