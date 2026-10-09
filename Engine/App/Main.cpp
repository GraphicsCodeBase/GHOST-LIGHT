// GhostLight.exe entry point: reads the command line and runs the engine.
#include "App/CommandLine.h"
#include "App/Engine.h"

int main(int argc, char** argv) {
    ghost::app::EngineOptions options;
    if (!ghost::app::CommandLine::parse(argc, argv, options)) {
        return 2;
    }
    ghost::app::Engine engine(options);
    if (!engine.initialize()) {
        return 1;
    }
    return engine.run();
}
