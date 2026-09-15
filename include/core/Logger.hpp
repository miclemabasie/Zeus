#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    OFF = 4
};

class ConfigManager; // Forwared declaration (need reference)

class Logger
{
public:
    // logger is accessed only through a getisntance method
    static Logger &getInstance(); // singleton

    // Since we are using the singleton, no copy or move should be allowed
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    void initialize(const ConfigManager &config);

    // Loggine methods
    void debug(const std::string &msg);
    void info(const std::string &msg);
    void warn(const std::string &msg);
    void error(const std::string &msg);

    // Control
    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    void flush();

    // Testing hooks
    // Redirect log output to a custom stream (for tests)
    void setOutputStream(std::ostream &stream);

    // reset the logger to a clean state;
    void reset();

private:
    // Private constructor/destructor (singleton)
    Logger();
    ~Logger();

    // Internal helpers
    void log(LogLevel level, const std::string &msg);
    std::string formatMessage(LogLevel level, const std::string &msg);
    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;
    static LogLevel stringToLevel(const std::string &str);

    // State
    LogLevel current_level_;
    std::ofstream file_stream_;
    std::ostream *output_stream_; // points to file Or stdout or test stream
    mutable std::mutex mutex_;    // Lock logger between threads
    bool initialized_;
    bool owns_file_stream_; // who controls ownership of the file? logger or external resource
};