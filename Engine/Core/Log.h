// Engine-wide logging: leveled messages to the console, the debugger output window and a log file in Build/.
#pragma once

#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

namespace ghost::core {

enum class LogLevel { Info, Warning, Error };

class Log {
public:
    // Starts writing to a file (truncating it). Messages logged before this only reach the console.
    static void openFile(const std::filesystem::path& path);
    static void closeFile();

    template <typename... Args>
    static void info(std::format_string<Args...> format, Args&&... args) {
        write(LogLevel::Info, std::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void warning(std::format_string<Args...> format, Args&&... args) {
        write(LogLevel::Warning, std::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void error(std::format_string<Args...> format, Args&&... args) {
        write(LogLevel::Error, std::format(format, std::forward<Args>(args)...));
    }

    static void write(LogLevel level, std::string_view message);

    // Totals since startup. The smoke test fails when errorCount() is not zero.
    static int errorCount();
    static int warningCount();
};

} // namespace ghost::core
