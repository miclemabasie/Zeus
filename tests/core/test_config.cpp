#include <gtest/gtest.h>
#include "core/ConfigManager.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class ConfigManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "zeus_test_config";
        fs::create_directories(test_dir_);
        test_config_path_ = test_dir_ / "zeus.json";
    }

    void TearDown() override
    {
        fs::remove_all(test_dir_);
    }

    void writeConfig(const std::string &content)
    {
        std::ofstream file(test_config_path_);
        file << content;
        file.close();
    }

    fs::path test_dir_;
    fs::path test_config_path_;
};

TEST_F(ConfigManagerTest, LoadsValidConfig)
{
    std::string json = R"({
        "collector": { "port": 9999, "max_connections": 5000 },
        "queue": { "max_size": 100000 },
        "worker_pool": { "count": 4 },
        "storage": { "directory": "/tmp/zeus" },
        "logger": { "level": "DEBUG", "file": "test.log" }
    })";
    writeConfig(json);

    ConfigManager config(test_config_path_.string());
    EXPECT_EQ(config.getCollectorPort(), 9999);
    EXPECT_EQ(config.getMaxConnections(), 5000);
    EXPECT_EQ(config.getQueueMaxSize(), 100000);
    EXPECT_EQ(config.getWorkerCount(), 4);
    EXPECT_EQ(config.getStorageDirectory(), "/tmp/zeus");
    EXPECT_EQ(config.getLogLevel(), "DEBUG");
    EXPECT_EQ(config.getLogFile(), "test.log");
}

TEST_F(ConfigManagerTest, ThrowsOnMissingFile)
{
    fs::path missing_path = test_dir_ / "does_not_exist.json";
    EXPECT_THROW({ ConfigManager config(missing_path.string()); }, std::runtime_error);
}

TEST_F(ConfigManagerTest, ThrowsOnMalformedJSON)
{
    std::string json = R"({
        "collector": { "port": 8080, }  // trailing comma is invalid
    })";
    writeConfig(json);
    EXPECT_THROW({ ConfigManager config(test_config_path_.string()); }, std::runtime_error);
}

TEST_F(ConfigManagerTest, ThrowsOnMissingTopLevelSection)
{
    std::string json = R"({
        "collector": { "port": 8080 }
    })";
    writeConfig(json);
    EXPECT_THROW({ ConfigManager config(test_config_path_.string()); }, std::runtime_error);
}

TEST_F(ConfigManagerTest, UsesDefaultsWhenNestedKeysMissing)
{
    std::string json = R"({
        "collector": {},
        "queue": {},
        "worker_pool": {},
        "storage": {},
        "logger": {}
    })";
    writeConfig(json);

    ConfigManager config(test_config_path_.string());
    EXPECT_EQ(config.getCollectorPort(), 8080);
    EXPECT_EQ(config.getMaxConnections(), 10000);
    EXPECT_EQ(config.getQueueMaxSize(), 500000);
    EXPECT_GE(config.getWorkerCount(), 1);
    EXPECT_EQ(config.getStorageDirectory(), "./logs");
    EXPECT_EQ(config.getLogLevel(), "INFO");
    EXPECT_EQ(config.getLogFile(), "zeus.log");
}