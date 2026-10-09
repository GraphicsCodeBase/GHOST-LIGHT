// Small corner readout: frame rate, frame time, GPU name, render mode, whether validation is on, GPU pass timings.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace ghost::ui {

class PerformanceOverlay {
public:
    struct Stats {
        double fps = 0.0;
        std::string gpuName;
        bool validation = false;
        int width = 0;
        int height = 0;
        std::string mode;                                          // e.g. "Raster" or "Path traced, 120 samples"
        double gpuMilliseconds = 0.0;                              // whole frame on the GPU
        std::vector<std::pair<std::string, double>> passTimings;   // render graph pass -> ms
    };
    static void draw(const Stats& stats);
};

} // namespace ghost::ui
