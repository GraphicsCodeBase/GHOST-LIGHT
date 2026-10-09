// GHOST_ASSERT: always-on invariant check that logs the message, expression, file and line, then stops the process.
#pragma once

namespace ghost::core {

[[noreturn]] void assertionFailed(const char* expression, const char* message, const char* file, int line);

} // namespace ghost::core

#define GHOST_ASSERT(condition, message)                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            ::ghost::core::assertionFailed(#condition, (message), __FILE__, __LINE__);    \
        }                                                                                 \
    } while (false)
