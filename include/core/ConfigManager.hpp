#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>
#include <sstream>

class ConfigManager
{
public:
    /*
     * @brief Construct a new config manager object
     * @param config_path path to the JSON configuration file
     * @throws std::runtime_error if file missing, unparseable, or invalid schema
     */

    explicit ConfigManager(const std::string &config_path);

    // --- Ingestion Layer ---
    int getCollectorPort() const;
    int getMaxConnections() const;

    // --- Queue ---
    size_t getQueueMaxSize() const;

    // --- Worker Pool ---
    size_t getWorkerCount() const;

    // --- Storage ---
    std::string getStorageDirectory() const;

    // --- Internal Logger ---
    std::string getLogLevel() const;
    std::string getLogFile() const;

    // --- Roload (stub for now) ---
    void reload();

private:
    nlohmann::json data_;

    /*
     * @brief Recursively traverse a dot-separeated key path (e.g., "collector.port")
     * @tparam T the expected return type (int, size_t, string, etc)
     * @param key_path Dot-separated path
     * @param default_value value to return if key is missing
     * @return The value, or default if missing
     * @throws std::runtime_error if the key exists but type does not match T
     */

    template <typename T>

    T getValue(const std::string &key_path, T default_value) const;

    /*
     * @brief validate that all top-level sections exist
     * @throws std::runtime_error if a required section is missing
     */
    void validateSchema() const;
};

template <typename T>
T ConfigManager::getValue(const std::string &key_path, T default_value) const
{
    // Split the path by "."
    std::vector<std::string> keys;
    std::stringstream ss(key_path);
    std::string key;

    while (std::getline(ss, key, '.'))
    {
        if (!key.empty())
        {
            keys.push_back(key);
        }
    }

    // Traverse the JSON
    nlohmann::json current = data_;
    for (const auto &k : keys)
    {
        if (current.contains(k))
        {
            current = current[k];
        }
        else
        {
            // key missing: return default
            return default_value;
        }
    }

    // key found: check if the type matches, otherwise throw
    try
    {
        return current.get<T>();
    }
    catch (const nlohmann::json::type_error &e)
    {
        throw std::runtime_error("Type mismatch for key: " + key_path + ", " + e.what());
    }
}
