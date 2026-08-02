#include "core/ConfigManager.hpp"
#include <iostream>

int main(int argc, char *argv[])
{

    try
    {
        // Hardcode the path relative to the build directory
        // The Cmake POST_BUILD step copies zeus.json to build/config/
        std::string config_path = "config/zeus.json";

        // Fallback to source path if run from different location
        if (!std::filesystem::exists(config_path))
        {
            config_path = "../config/zeus.json";
        }

        std::cout << "=== Zeus Smoke Test ===" << std::endl;
        std::cout << "Loading config from: " << config_path << std::endl;

        ConfigManager config(config_path);

        std::cout << "Config loaded successfully!" << std::endl;
        std::cout << "  Collector Port: " << config.getCollectorPort() << std::endl;
        std::cout << "  Max Connections: " << config.getMaxConnections() << std::endl;
        std::cout << "  Queue Max Size: " << config.getQueueMaxSize() << std::endl;
        std::cout << "  Worker Count: " << config.getWorkerCount() << std::endl;
        std::cout << "  Storage Dir: " << config.getStorageDirectory() << std::endl;
        std::cout << "  Log Level: " << config.getLogLevel() << std::endl;
        std::cout << "  Log File: " << config.getLogFile() << std::endl;

        std::cout << std::endl
                  << "Foundation is solid. Ready for module 2!" << std::endl;

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}