# Create a static library for all core modules
add_library(zeus_core STATIC
    core/ConfigManager.cpp  
    core/Logger.cpp
)

# Link dependencies to the core library
target_link_libraries(zeus_core
    PUBLIC nlohmann_json::nlohmann_json
)

# The main executable (smoke test)
add_executable(zeus main.cpp)

# Link the core library and dependencies
target_link_libraries(zeus
    PRIVATE zeus_core
    PUBLIC nlohmann_json::nlohmann_json
)

# Automatically copy the config file to the build directory
add_custom_command(TARGET zeus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config/zeus.json
        ${CMAKE_BINARY_DIR}/config/zeus.json
    COMMENT "Copying config file to build directory"
)