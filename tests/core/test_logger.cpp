#include <gtest/gtest.h>
#include "core/Logger.hpp"
#include "core/ConfigManager.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset logger to a clean state
        Logger::getInstance().reset();
        Logger::getInstance().setLevel(LogLevel::DEBUG);
    }

    void TearDown() override
    {
        Logger::getInstance().reset();
    }
};

// ========== TEST 1: Level Suppression ==========
TEST_F(LoggerTest, SuppressesLowerLevels)
{
    std::ostringstream captured;
    Logger::getInstance().setOutputStream(captured);
    Logger::getInstance().setLevel(LogLevel::WARN);

    Logger::getInstance().debug("debug msg");
    Logger::getInstance().info("info msg");
    Logger::getInstance().warn("warn msg");
    Logger::getInstance().error("error msg");

    std::string output = captured.str();
    EXPECT_EQ(output.find("debug msg"), std::string::npos);
    EXPECT_EQ(output.find("info msg"), std::string::npos);
    EXPECT_NE(output.find("warn msg"), std::string::npos);
    EXPECT_NE(output.find("error msg"), std::string::npos);
}

// ========== TEST 2: All Levels Pass Through at DEBUG ==========
TEST_F(LoggerTest, AllLevelsAtDebug)
{
    std::ostringstream captured;
    Logger::getInstance().setOutputStream(captured);
    Logger::getInstance().setLevel(LogLevel::DEBUG);

    Logger::getInstance().debug("d");
    Logger::getInstance().info("i");
    Logger::getInstance().warn("w");
    Logger::getInstance().error("e");

    std::string output = captured.str();
    EXPECT_NE(output.find("[DEBUG]"), std::string::npos);
    EXPECT_NE(output.find("[INFO ]"), std::string::npos);
    EXPECT_NE(output.find("[WARN ]"), std::string::npos);
    EXPECT_NE(output.find("[ERROR]"), std::string::npos);
}

// ========== TEST 3: Format Contains Timestamp ==========
TEST_F(LoggerTest, OutputContainsTimestamp)
{
    std::ostringstream captured;
    Logger::getInstance().setOutputStream(captured);
    Logger::getInstance().setLevel(LogLevel::INFO);

    Logger::getInstance().info("test message");

    std::string output = captured.str();
    // Timestamp format: [YYYY-MM-DD HH:MM:SS.mmm]
    EXPECT_NE(output.find("["), std::string::npos);
    EXPECT_NE(output.find("-"), std::string::npos);
    EXPECT_NE(output.find(":"), std::string::npos);
}

// ========== TEST 4: SetLevel / GetLevel ==========
TEST_F(LoggerTest, SetAndGetLevel)
{
    Logger::getInstance().setLevel(LogLevel::ERROR);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::ERROR);

    Logger::getInstance().setLevel(LogLevel::DEBUG);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::DEBUG);
}

// ========== TEST 5: Singleton Returns Same Instance ==========
TEST_F(LoggerTest, SingletonIsSameInstance)
{
    Logger &a = Logger::getInstance();
    Logger &b = Logger::getInstance();
    EXPECT_EQ(&a, &b);
}

// ========== TEST 6: Reset Clears Level ==========
TEST_F(LoggerTest, ResetRestoresDefaults)
{
    Logger::getInstance().setLevel(LogLevel::ERROR);
    Logger::getInstance().reset();
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::INFO);
}

// ========== TEST 7: Thread Safety (Smoke Test) ==========
TEST_F(LoggerTest, ThreadSafety)
{
    std::ostringstream captured;
    Logger::getInstance().setOutputStream(captured);
    Logger::getInstance().setLevel(LogLevel::INFO);

    constexpr int num_threads = 4;
    constexpr int logs_per_thread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back([i]()
                             {
            for (int j = 0; j < logs_per_thread; ++j) {
                Logger::getInstance().info(
                    "thread=" + std::to_string(i) + " msg=" + std::to_string(j));
            } });
    }
    for (auto &t : threads)
        t.join();

    // Count occurrences of "INFO"
    std::string output = captured.str();
    size_t count = 0, pos = 0;
    while ((pos = output.find("[INFO ]", pos)) != std::string::npos)
    {
        ++count;
        pos += 7;
    }
    EXPECT_EQ(count, num_threads * logs_per_thread);
}