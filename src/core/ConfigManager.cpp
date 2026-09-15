#include "core/ConfigManager.hpp"
#include <fstream>
#include <iostream> // only for fallback with path check
#include <filesystem>
#include <thread>

namespace fs = std::filesystem;

// implement the interface functions from the .hpp file

// === Constructor ===
ConfigManager::ConfigManager(const std::string &config_path)
{
    // 1. check if file exists
    if (!fs::exists(config_path))
    {
        throw std::runtime_error("Config file not found: " + config_path);
    }

    // 2. Read file contents
    std::ifstream file(config_path);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open config file: " + config_path);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    file.close();

    // 3. Parse JSON
    try
    {
        data_ = nlohmann::json::parse(content);
    }
    catch (const nlohmann::json::parse_error &e)
    {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }

    // 4. validate the schema
    validateSchema();
}

// === Schema Validataion ===
void ConfigManager::validateSchema() const
{
    // Requred top-level sections
    const std::vector<std::string> required_sections = {
        "collector", "queue", "worker_pool", "storage", "logger"};

    for (const auto &section : required_sections)
    {
        if (!data_.contains(section))
        {
            throw std::runtime_error("Missing required section: " + section);
        }

        // Ensure the section is an object, not an array of string
        if (!data_[section].is_object())
        {
            throw std::runtime_error("Section '" + section + "' must be a JSON object.");
        }
    }
}

// === PUBLIC GETTERS ===

// Each getter calls getValue with the dot-notation path and a sensible default.

int ConfigManager::getCollectorPort() const
{
    return getValue<int>("collector.port", 8080);
}

int ConfigManager::getMaxConnections() const
{
    return getValue<int>("collector.max_connections", 10000);
}

size_t ConfigManager::getQueueMaxSize() const
{
    return getValue<size_t>("queue.max_size", 500000);
}

size_t ConfigManager::getWorkerCount() const
{
    // if the user doesn't specify, use the hardware concurrency as a smart default.
    unsigned int default_workers = std::thread::hardware_concurrency();
    if (default_workers == 0)
        default_workers = 4;
    return getValue<size_t>("worker_pool.count", default_workers);
}

std::string ConfigManager::getStorageDirectory() const
{
    // Fallback to local ".logs" directory if the system path is not writable.
    // the default in the JSON is "/var/log/zeus", but we fallback to "./logs" safely

    std::string defualt_dir = "./logs";
    return getValue<std::string>("storage.directory", defualt_dir);
}

std::string ConfigManager::getLogLevel() const
{
    return getValue<std::string>("logger.level", "INFO");
}

std::string ConfigManager::getLogFile() const
{
    return getValue<std::string>("logger.file", "zeus.log");
}

// === Reload (STUB) ===
void ConfigManager::reload()
{
    // just throw and error for now
    throw std::runtime_error("reload() is not yet implemented in this version");
}

void ConfigManager::showConfigScheme() const
{
    std::cout << "=== Current Config Values ===" << std::endl;
    std::cout << "  Collector Port: " << getCollectorPort() << std::endl;
    std::cout << "  Max Connections: " << getMaxConnections() << std::endl;
    std::cout << "  Queue Max Size: " << getQueueMaxSize() << std::endl;
    std::cout << "  Worker Count: " << getWorkerCount() << std::endl;
    std::cout << "  Storage Dir: " << getStorageDirectory() << std::endl;
    std::cout << "  Log Level: " << getLogLevel() << std::endl;
    std::cout << "  Log File: " << getLogFile() << std::endl;
}