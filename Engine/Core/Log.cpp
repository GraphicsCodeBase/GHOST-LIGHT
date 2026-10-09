// Thread-safe log implementation. Every line is flushed so a crash never loses the tail of the log.
#include "Core/Log.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <string>

#include <windows.h>

namespace ghost::core {

namespace {

std::mutex g_mutex;
std::ofstream g_file;
std::atomic<int> g_errorCount{0};
std::atomic<int> g_warningCount{0};

std::chrono::steady_clock::time_point startTime() {
    static const auto start = std::chrono::steady_clock::now();
    return start;
}

const char* levelTag(LogLevel level) {
    switch (level) {
    case LogLevel::Info: return "info ";
    case LogLevel::Warning: return "WARN ";
    case LogLevel::Error: return "ERROR";
    }
    return "?????";
}

// Console text colors make warnings and errors stand out in the run.bat window.
WORD levelColor(LogLevel level) {
    switch (level) {
    case LogLevel::Warning: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case LogLevel::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY;
    default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

} // namespace

void Log::openFile(const std::filesystem::path& path) {
    std::scoped_lock lock(g_mutex);
    std::error_code ignored;
    std::filesystem::create_directories(path.parent_path(), ignored);
    g_file.open(path, std::ios::out | std::ios::trunc);
}

void Log::closeFile() {
    std::scoped_lock lock(g_mutex);
    g_file.close();
}

void Log::write(LogLevel level, std::string_view message) {
    if (level == LogLevel::Error) {
        ++g_errorCount;
    } else if (level == LogLevel::Warning) {
        ++g_warningCount;
    }

    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime()).count();
    const std::string line = std::format("[{:8.3f}] {} {}\n", seconds, levelTag(level), message);

    std::scoped_lock lock(g_mutex);
    const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (level != LogLevel::Info) {
        SetConsoleTextAttribute(console, levelColor(level));
    }
    std::fputs(line.c_str(), stdout);
    std::fflush(stdout);
    if (level != LogLevel::Info) {
        SetConsoleTextAttribute(console, levelColor(LogLevel::Info));
    }
    OutputDebugStringA(line.c_str());
    if (g_file.is_open()) {
        g_file << line;
        g_file.flush();
    }
}

int Log::errorCount() {
    return g_errorCount.load();
}

int Log::warningCount() {
    return g_warningCount.load();
}

} // namespace ghost::core
