// Assertion failure handler: log, break into an attached debugger, then terminate without a blocking dialog.
#include "Core/Assert.h"

#include "Core/Log.h"

#include <windows.h>

namespace ghost::core {

void assertionFailed(const char* expression, const char* message, const char* file, int line) {
    Log::error("Assertion failed: {}\n           expression: {}\n           at {}:{}", message, expression, file, line);
    Log::closeFile();
    if (IsDebuggerPresent()) {
        __debugbreak();
    }
    // TerminateProcess instead of abort(): abort() pops a modal dialog in Debug builds, which would hang run.bat test.
    TerminateProcess(GetCurrentProcess(), 3);
    for (;;) {
    }
}

} // namespace ghost::core
