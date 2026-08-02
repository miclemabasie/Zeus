add_library(zeus_core STATIC
    core/configManager.cpp
    # other modules will be added here 
)

# Link dependencies to the core library
target_link_libraries(zeus_core
    PUBLIC nlohman_json::nlohman_json
)

# The main executable (smoke test)
add_executable(zeus main.cpp)

# Link the core library and dependencies
target_link_libraries(zeus
    PRIVATE zeus_core
    PUBLIC nlohman_json::nlohman_json
)


# Automatically copy the config file to the build directory for easy testing
add_custom_command(TARGET zeus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config/zeus.json
        ${CMAKE_BINARY_DIR}/config/zeus.json
    COMMEND "Copying config file to build directory..."
)