// Frame timing with an exponential moving average for a stable FPS readout.
#include "Core/FrameTimer.h"

#include <algorithm>

namespace ghost::core {

void FrameTimer::tick() {
    const auto now = std::chrono::steady_clock::now();
    if (!m_started) {
        m_start = now;
        m_last = now;
        m_started = true;
        return;
    }

    const double rawDelta = std::chrono::duration<double>(now - m_last).count();
    m_last = now;
    m_deltaSeconds = std::min(rawDelta, kMaxDeltaSeconds);
    m_elapsedSeconds = std::chrono::duration<double>(now - m_start).count();
    ++m_frameIndex;

    if (rawDelta > 0.0) {
        const double fps = 1.0 / rawDelta;
        m_smoothedFps = (m_smoothedFps == 0.0) ? fps : m_smoothedFps * 0.95 + fps * 0.05;
    }
}

} // namespace ghost::core
