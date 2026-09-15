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
        std::cout << "Loading config from '" << config_path << "' ..." << std::endl;

        ConfigManager config(config_path);
        std::cout << "=== Config loaded successfully ===" << std::endl;

        // config.showConfigScheme();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}