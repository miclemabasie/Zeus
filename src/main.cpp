#include "core/ConfigManager.hpp"
#include "core/Logger.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        // 1. Load config
        std::string config_path = "config/zeus.json";
        if (!std::filesystem::exists(config_path))
        {
            config_path = "../config/zeus.json";
        }
        ConfigManager config(config_path);

        // 2. Initialize logger with config
        Logger::getInstance().initialize(config);

        // 3. Now use it anywhere
        Logger::getInstance().info("=== Zeus Smoke Test ===");
        Logger::getInstance().info("Loading config from: " + config_path);

        Logger::getInstance().info("  Collector Port: " + std::to_string(config.getCollectorPort()));
        Logger::getInstance().info("  Max Connections: " + std::to_string(config.getMaxConnections()));
        Logger::getInstance().info("  Queue Max Size: " + std::to_string(config.getQueueMaxSize()));
        Logger::getInstance().info("  Worker Count: " + std::to_string(config.getWorkerCount()));

        Logger::getInstance().debug("This is a debug message (won't show at INFO level)");
        Logger::getInstance().warn("This is a warning example");
        Logger::getInstance().error("This is an error example");

        Logger::getInstance().info("Foundation is solid. Ready for module 2!");
        Logger::getInstance().flush();

        std::cout << "Check logs/zeus.log for output!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}