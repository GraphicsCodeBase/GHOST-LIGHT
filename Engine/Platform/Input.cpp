// Input state tracking: events set bits, beginFrame() clears the per-frame edges and deltas.
#include "Platform/Input.h"

namespace ghost::platform {

uint8_t Input::keyState(Key key) const {
    const auto index = static_cast<size_t>(key);
    return (m_uiKeyboard || index >= m_keys.size()) ? 0 : m_keys[index];
}

uint8_t Input::buttonState(MouseButton button) const {
    const auto index = static_cast<size_t>(button);
    return (m_uiMouse || index >= m_buttons.size()) ? 0 : m_buttons[index];
}

bool Input::isDown(Key key) const {
    return (keyState(key) & kDown) != 0;
}

bool Input::wasPressed(Key key) const {
    return (keyState(key) & kPressed) != 0;
}

bool Input::wasReleased(Key key) const {
    return (keyState(key) & kReleased) != 0;
}

bool Input::isDown(MouseButton button) const {
    return (buttonState(button) & kDown) != 0;
}

bool Input::wasPressed(MouseButton button) const {
    return (buttonState(button) & kPressed) != 0;
}

bool Input::wasReleased(MouseButton button) const {
    return (buttonState(button) & kReleased) != 0;
}

bool Input::isShiftDown() const {
    return isDown(Key::LeftShift) || isDown(Key::RightShift);
}

bool Input::isControlDown() const {
    return isDown(Key::LeftControl) || isDown(Key::RightControl);
}

glm::vec2 Input::mouseDelta() const {
    return m_uiMouse ? glm::vec2(0.0f) : m_mouseDelta;
}

float Input::wheelDelta() const {
    return m_uiMouse ? 0.0f : m_wheel;
}

void Input::setUiCapture(bool keyboard, bool mouse) {
    m_uiKeyboard = keyboard;
    m_uiMouse = mouse;
}

void Input::beginFrame() {
    for (uint8_t& state : m_keys) {
        state &= kDown;
    }
    for (uint8_t& state : m_buttons) {
        state &= kDown;
    }
    m_mouseDelta = glm::vec2(0.0f);
    m_wheel = 0.0f;
}

void Input::onKey(int key, bool down) {
    if (key < 0 || static_cast<size_t>(key) >= m_keys.size()) {
        return; // GLFW_KEY_UNKNOWN (-1) and keys beyond the table
    }
    uint8_t& state = m_keys[static_cast<size_t>(key)];
    if (down && !(state & kDown)) {
        state |= kDown | kPressed;
    } else if (!down && (state & kDown)) {
        state = static_cast<uint8_t>((state & ~kDown) | kReleased);
    }
}

void Input::onMouseButton(int button, bool down) {
    if (button < 0 || static_cast<size_t>(button) >= m_buttons.size()) {
        return;
    }
    uint8_t& state = m_buttons[static_cast<size_t>(button)];
    if (down && !(state & kDown)) {
        state |= kDown | kPressed;
    } else if (!down && (state & kDown)) {
        state = static_cast<uint8_t>((state & ~kDown) | kReleased);
    }
}

void Input::onCursorMoved(double x, double y) {
    const glm::vec2 position(static_cast<float>(x), static_cast<float>(y));
    if (m_hasMousePosition) {
        m_mouseDelta += position - m_mousePosition;
    }
    m_mousePosition = position;
    m_hasMousePosition = true;
}

void Input::onScroll(double steps) {
    m_wheel += static_cast<float>(steps);
}

} // namespace ghost::platform
