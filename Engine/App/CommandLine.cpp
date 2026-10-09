// Command-line parsing: --scene <path>, --frames <n>, --help.
#include "App/CommandLine.h"

#include <charconv>
#include <cstdio>
#include <string_view>

namespace ghost::app {

namespace {

void printUsage() {
    std::printf(
        "Usage: GhostLight.exe [options]\n"
        "  --scene <path>   scene to open, relative to Content/ (e.g. Scenes/Sponza.scene.json)\n"
        "  --frames <n>     quit after n frames (for automated runs)\n"
        "  --help           show this text\n");
}

} // namespace

bool CommandLine::parse(int argc, char** argv, EngineOptions& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        const bool hasValue = i + 1 < argc;
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return false;
        }
        if (arg == "--scene" && hasValue) {
            options.scene = argv[++i];
        } else if (arg == "--frames" && hasValue) {
            const std::string_view value = argv[++i];
            uint64_t frames = 0;
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), frames);
            if (error != std::errc{} || end != value.data() + value.size()) {
                std::fprintf(stderr, "--frames expects a number, got '%.*s'\n", static_cast<int>(value.size()), value.data());
                return false;
            }
            options.maxFrames = frames;
        } else {
            std::fprintf(stderr, "Unknown or incomplete option '%.*s'\n", static_cast<int>(arg.size()), arg.data());
            printUsage();
            return false;
        }
    }
    return true;
}

} // namespace ghost::app
