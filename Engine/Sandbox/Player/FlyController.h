// FlyController: the sandbox's free-flying camera. Hold the right mouse button to look around; WASD moves, Space/C go
// up/down, Shift is 4x faster, Ctrl 4x slower, the mouse wheel changes the base speed.
#pragma once

#include <glm/vec3.hpp>

namespace ghost::platform {
class Window;
}

namespace ghost::sandbox {

class FlyController {
public:
    // Same conventions as scene bookmarks: yaw 0 looks down -Z, 90 down +X; positive pitch looks up.
    void teleport(const glm::vec3& position, float yawDeg, float pitchDeg);
    // Applies this frame's input. `speed` (meters per second) is the persisted base speed; the wheel changes it.
    void update(platform::Window& window, float deltaSeconds, float& speed);

    glm::vec3 position() const { return m_position; }
    glm::vec3 forward() const;
    glm::vec3 right() const;
    float yawDeg() const { return m_yawDeg; }
    float pitchDeg() const { return m_pitchDeg; }

    float fovYDeg = 60.0f;
    float nearPlane = 0.05f;

private:
    glm::vec3 m_position{0.0f};
    float m_yawDeg = 0.0f;
    float m_pitchDeg = 0.0f;
    bool m_looking = false;
};

} // namespace ghost::sandbox
