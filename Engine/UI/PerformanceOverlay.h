// Small corner readout: frame rate, frame time, GPU name and whether validation is on.
#pragma once

#include <string>

namespace ghost::ui {

class PerformanceOverlay {
public:
    struct Stats {
        double fps = 0.0;
        std::string gpuName;
        bool validation = false;
        int width = 0;
        int height = 0;
    };
    static void draw(const Stats& stats);
};

} // namespace ghost::ui
