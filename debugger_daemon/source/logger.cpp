#include <cstdio>
#include <ctime>

#include <fmt/format.h>
#include <fmt/time.h>

#include "logger.h"
#include "utils.h"

namespace Logger {

/// File name of the log file
const char* LogFileName = "debugger.log";

/// File pointer to our log file
static std::FILE* log_file = nullptr;

/**
 * Gets the current date and time as a string of the format
 * YYYY-MM-DD HH:mm:ss
 */
static std::string GetCurrentDateTime() {
    std::time_t time = std::time(nullptr);
    return ""; //fmt::format("{:%Y-%m-%d %H:%M:%s}.", *std::localtime(&time));
}

void Initialize() {
    ASSERT(log_file == nullptr);
    log_file = std::fopen(LogFileName, "a");
    Log(fmt::format("Session started at {}", GetCurrentDateTime()));
}

void Finalize() {
    Log(fmt::format("Session ended at {}", GetCurrentDateTime()));
    std::fclose(log_file);
    log_file = nullptr;
}

void Log(const std::string& message) {
    ASSERT(log_file != nullptr);
    std::fprintf(log_file, "%s\n", message.c_str());
    std::fflush(log_file);
}

}