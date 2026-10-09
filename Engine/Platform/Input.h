// Keyboard and mouse state for the current frame: held keys, press/release edges, mouse delta and wheel.
#pragma once

#include <array>
#include <cstdint>

#include <glm/vec2.hpp>

namespace ghost::platform {

// Values match GLFW key codes so the window forwards them without a lookup table.
enum class Key : int {
    Space = 32, Apostrophe = 39, Comma = 44, Minus = 45, Period = 46, Slash = 47,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete, Right, Left, Down, Up, PageUp, PageDown, Home, End,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl, LeftAlt, LeftSuper, RightShift, RightControl, RightAlt, RightSuper,
};

enum class MouseButton : int { Left = 0, Right = 1, Middle = 2 };

class Input {
public:
    bool isDown(Key key) const;
    bool wasPressed(Key key) const;  // went down this frame
    bool wasReleased(Key key) const; // went up this frame
    bool isDown(MouseButton button) const;
    bool wasPressed(MouseButton button) const;
    bool wasReleased(MouseButton button) const;
    bool isShiftDown() const;
    bool isControlDown() const;

    glm::vec2 mousePosition() const { return m_mousePosition; } // window coordinates
    glm::vec2 mouseDelta() const;                                // movement since last frame
    float wheelDelta() const;                                    // scroll steps since last frame

    // While the UI owns a device (e.g. typing into an ImGui field), queries for that device report nothing.
    void setUiCapture(bool keyboard, bool mouse);

    // Event intake, called by Window.
    void beginFrame();
    void onKey(int key, bool down);
    void onMouseButton(int button, bool down);
    void onCursorMoved(double x, double y);
    void onScroll(double steps);

private:
    static constexpr uint8_t kDown = 1;
    static constexpr uint8_t kPressed = 2;
    static constexpr uint8_t kReleased = 4;

    uint8_t keyState(Key key) const;
    uint8_t buttonState(MouseButton button) const;

    std::array<uint8_t, 512> m_keys{};
    std::array<uint8_t, 8> m_buttons{};
    glm::vec2 m_mousePosition{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    float m_wheel = 0.0f;
    bool m_hasMousePosition = false;
    bool m_uiKeyboard = false;
    bool m_uiMouse = false;
};

} // namespace ghost::platform
