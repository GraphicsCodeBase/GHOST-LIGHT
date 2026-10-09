// Measures frame time: delta seconds for simulation, a smoothed FPS for display, and the frame counter.
#pragma once

#include <chrono>
#include <cstdint>

namespace ghost::core {

class FrameTimer {
public:
    // Call once at the start of every frame.
    void tick();

    // Seconds since the previous tick, clamped so a breakpoint or window drag doesn't produce a huge step.
    double deltaSeconds() const { return m_deltaSeconds; }
    double elapsedSeconds() const { return m_elapsedSeconds; }
    double smoothedFps() const { return m_smoothedFps; }
    uint64_t frameIndex() const { return m_frameIndex; }

private:
    static constexpr double kMaxDeltaSeconds = 0.25;

    std::chrono::steady_clock::time_point m_start{};
    std::chrono::steady_clock::time_point m_last{};
    double m_deltaSeconds = 0.0;
    double m_elapsedSeconds = 0.0;
    double m_smoothedFps = 0.0;
    uint64_t m_frameIndex = 0;
    bool m_started = false;
};

} // namespace ghost::core
