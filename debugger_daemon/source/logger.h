#pragma once

#include <string>

namespace Logger {

/**
 * Initializes the logging subsystem.
 */
void Initialize();

/**
 * Finalizes the logging subsystem, closing any file pointers that may be in use.
 */
void Finalize();

/**
 * Logs a message to the log file.
 * @param message The message to log.
 */
void Log(const std::string& message);
}