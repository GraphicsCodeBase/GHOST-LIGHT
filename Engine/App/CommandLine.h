// Parses GhostLight.exe command-line options into EngineOptions.
#pragma once

#include "App/EngineOptions.h"

namespace ghost::app {

class CommandLine {
public:
    // Returns false (after printing usage) on unknown or malformed options.
    static bool parse(int argc, char** argv, EngineOptions& options);
};

} // namespace ghost::app
