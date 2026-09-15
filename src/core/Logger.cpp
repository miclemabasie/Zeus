#include <core/Logger.hpp>
#include <core/ConfigManager.hpp>

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

// Singleton
Logger &Logger::getInstance()
{
    static Logger instance;
    return instance;
}

// Constructor / Destructor
Logger::Logger()
    : current_level_(LogLevel::INFO),
      output_stream_(&std::cout),
      initialized_(false),
      owns_file_stream_(false)
{
}

Logger::~Logger()
{
    if (file_stream_.is_open())
    {
        file_stream_.flush();
        file_stream_.close();
    }
}
// Initialize Logger - reads from ConfigManager
void Logger::initialize(const ConfigManager &config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Set the level from config
    current_level_ = stringToLevel(config.getLogLevel());

    // 2. Open log file
    std::string file_path = config.getLogFile();

    // create parent directories if needed
    fs::path p(file_path);
    if (p.has_parent_path())
    {
        fs::create_directories(p.parent_path());
    }

    file_stream_.open(file_path, std::ios::app); // Append mode
    if (!file_stream_.is_open())
    {
        throw std::runtime_error("Logger: failed to open log file: " + file_path);
    }

    output_stream_ = &file_stream_;
    owns_file_stream_ = true;
    initialized_ = true;
}

// Log methods (public entries)
void Logger::debug(const std::string &msg) { log(LogLevel::DEBUG, msg); }
void Logger::info(const std::string &msg) { log(LogLevel::INFO, msg); }
void Logger::warn(const std::string &msg) { log(LogLevel::WARN, msg); }
void Logger::error(const std::string &msg) { log(LogLevel::ERROR, msg); }

void Logger::log(LogLevel level, const std::string &msg)
{
    // Early exit if below treshold (avoid locking overhead)
    if (level < current_level_)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Double-check inside the lock (another thread may have changed the level)
    if (level < current_level_)
        return;

    if (!output_stream_ || !output_stream_->good())
        return;

    (*output_stream_) << formatMessage(level, msg) << std::endl;
}

// Format Helpers
std::string Logger::formatMessage(LogLevel level, const std::string &msg)
{
    std::ostringstream oss;

    oss << "[" << getTimestamp() << "]"
        << "[" << levelToString(level) << "]"
        << msg;

    return oss.str();
}

std::string Logger::getTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf{};

#if defined(_WIN32)
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const
{
    switch ((level))
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::OFF:
        return "OFF";
    }

    return "?????";
}

LogLevel Logger::stringToLevel(const std::string &str)
{
    if (str == "DEBUG")
        return LogLevel::DEBUG;
    if (str == "INFO")
        return LogLevel::INFO;
    if (str == "WARN")
        return LogLevel::WARN;
    if (str == "ERROR")
        return LogLevel::ERROR;
    if (str == "OFF")
        return LogLevel::OFF;

    return LogLevel::INFO;
}

void Logger::setLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

LogLevel Logger::getLevel() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return current_level_;
}

void Logger::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_stream_)
        output_stream_->flush();
}

// ============================================================
// TESTING HOOKS
// ============================================================
void Logger::setOutputStream(std::ostream &stream)
{
    std::lock_guard<std::mutex> lock(mutex_);
    output_stream_ = &stream;
    owns_file_stream_ = false; // We don't own this stream
}

void Logger::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open())
    {
        file_stream_.flush();
        file_stream_.close();
    }
    current_level_ = LogLevel::INFO;
    output_stream_ = &std::cout;
    initialized_ = false;
    owns_file_stream_ = false;
}