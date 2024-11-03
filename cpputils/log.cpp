#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main() {
    // Create a console logger with color output
    auto console_logger = spdlog::stdout_color_mt("console");
    console_logger->set_level(spdlog::level::info);  // Set log level to info

    // Log messages at various severity levels
    console_logger->trace("This is a trace message, usually very verbose.");
    console_logger->debug("This is a debug message.");
    console_logger->info("This is an info message.");
    console_logger->warn("This is a warning message.");
    console_logger->error("This is an error message.");
    console_logger->critical("This is a critical message.");

    // Example of formatted logging
    int value = 42;
    console_logger->info("Logging a formatted message: Value = {}", value);

    // Create a file logger
    auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/example.log");
    file_logger->set_level(spdlog::level::warn);  // Log only warnings and above to the file

    // Logging to file
    file_logger->warn("This warning message will be logged to a file.");
    file_logger->error("This error message will be logged to a file.");

    // Flush all loggers
    spdlog::shutdown();

    return 0;
}
