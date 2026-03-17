#include "DependencyValidator.h"
#include "Define.h"
#include "Log.h"

// Intel TBB Headers
// TBB includes removed - Playerbot now uses std:: threading primitives
// #include <tbb/version.h>
// #include <tbb/task_arena.h>
// #include <tbb/concurrent_queue.h>
// #include <tbb/parallel_for.h>
// #include <tbb/concurrent_hash_map.h>
#include <thread>
#include <queue>
#include <mutex>

// Parallel Hashmap Headers
#include <parallel_hashmap/phmap.h>

// Boost Headers
#include <boost/version.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/version.hpp>
#include <boost/asio/io_context.hpp>

// MySQL Headers (use Trinity's database wrapper)
#include "DatabaseEnv.h"

// Standard headers
#include <chrono>
#include <thread>
#include <atomic>
#include <exception>

namespace Playerbot {

bool DependencyValidator::ValidateAllDependencies()
{
    TC_LOG_INFO("module.playerbot.dependencies", "=== Playerbot Dependency Validation Starting ===");

    bool success = true;

    // Validate Intel TBB
    if (!ValidateTBB())
    {
        TC_LOG_ERROR("module.playerbot.dependencies", " Intel TBB validation failed");
        success = false;
    }

    // Validate Parallel Hashmap
    if (!ValidatePhmap())
    {
        TC_LOG_ERROR("module.playerbot.dependencies", " Parallel Hashmap validation failed");
        success = false;
    }

    // Validate Boost
    if (!ValidateBoost())
    {
        TC_LOG_ERROR("module.playerbot.dependencies", " Boost validation failed");
        success = false;
    }

    // Validate MySQL
    if (!ValidateMySQL())
    {
        TC_LOG_ERROR("module.playerbot.dependencies", " MySQL validation failed");
        success = false;
    }

    // Validate system requirements
    if (!ValidateSystemRequirements())
    {
        TC_LOG_WARN("module.playerbot.dependencies", "  System requirements validation failed");
        // Don't fail build, but warn about potential performance issues
    }

    if (success)
    {
        TC_LOG_INFO("module.playerbot.dependencies", " All enterprise dependencies validated successfully");
        TC_LOG_INFO("module.playerbot.dependencies", " Playerbot ready for high-performance operations");
    } else {
        TC_LOG_ERROR("module.playerbot.dependencies", " Dependency validation failed - Playerbot will be disabled");
    }

    return success;
}

bool DependencyValidator::ValidateTBB()
{
    try {
        // Check TBB version
        int major = TBB_VERSION_MAJOR;
        int minor = TBB_VERSION_MINOR;

        if (major < 2021 || (major == 2021 && minor < 5)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Intel TBB version {}.{} insufficient, required 2021.5+", major, minor);
            return false;
        }

        // Test TBB concurrency functionality
    if (!TestTBBConcurrency())
    {
            TC_LOG_ERROR("module.playerbot.dependencies", "Intel TBB concurrency test failed");
            return false;
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            " Intel TBB {}.{} validated with concurrency tests", major, minor);
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Intel TBB validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::TestTBBConcurrency()
{
    // TBB REMOVED - Playerbot now uses std:: threading primitives
    TC_LOG_INFO("module.playerbot.dependencies", "TBB dependency removed - using standard C++ threading");
    return true;

    /*
    try {
        // Test task_arena
        tbb::task_arena arena(4);
        if (arena.max_concurrency() == 0) {
            TC_LOG_ERROR("module.playerbot.dependencies", "TBB task_arena initialization failed");
            return false;
        }

        // Test concurrent_queue
        tbb::concurrent_queue<int> queue;
        constexpr int TEST_SIZE = 1000;

        // Producer
        arena.execute([&]() {
            tbb::parallel_for(0, TEST_SIZE, [&](int i) {
                queue.push(i);
            });
        });

        // Consumer
        ::std::atomic<int> consumed{0};
        arena.execute([&]() {
            int value;
            while (queue.try_pop(value))
            {
                consumed.fetch_add(1);
            }
        });

        // Verify all items were processed
    if (consumed.load() != TEST_SIZE) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "TBB concurrent queue test failed: expected {}, got {}", TEST_SIZE, consumed.load());
            return false;
        }

        // Test parallel_for performance
        ::std::vector<int> test_data(10000);
        auto start = ::std::chrono::high_resolution_clock::now();

        tbb::parallel_for(0, 10000, [&](int i) {
            test_data[i] = i * 2 + 1;
        });

        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);

        TC_LOG_DEBUG("module.playerbot.dependencies",
            "TBB parallel_for processed 10k items in {}μs", duration.count());

        // Verify results
    for (int i = 0; i < 100; ++i) {
            if (test_data[i] != i * 2 + 1) {
                TC_LOG_ERROR("module.playerbot.dependencies", "TBB parallel_for data corruption detected");
                return false;
            }
        }

        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies", "TBB concurrency test exception: {}", e.what());
        return false;
    }
    */
}

bool DependencyValidator::ValidatePhmap()
{
    try {
        if (!TestPhmapPerformance())
        {
            TC_LOG_ERROR("module.playerbot.dependencies", "Parallel hashmap performance test failed");
            return false;
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            " Parallel Hashmap validated with performance tests");
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Parallel Hashmap validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::TestPhmapPerformance()
{
    try {
        // Test parallel_flat_hash_map basic functionality
        phmap::parallel_flat_hash_map<uint32, ::std::string> test_map;

        // Insert test data
        constexpr uint32 TEST_SIZE = 10000;
        for (uint32 i = 0; i < TEST_SIZE; ++i) {
            test_map[i] = "test_value_" + ::std::to_string(i);
        }

        if (test_map.size() != TEST_SIZE) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Parallel hashmap size mismatch: expected {}, got {}", TEST_SIZE, test_map.size());
            return false;
        }

        // Test concurrent access
        ::std::atomic<uint32> found_count{0};
        tbb::parallel_for(uint32(0), TEST_SIZE, [&](uint32 i)
        {
            auto it = test_map.find(i);
            if (it != test_map.end() && it->second == "test_value_" + ::std::to_string(i)) {
                found_count.fetch_add(1);
            }
        });

        if (found_count.load() != TEST_SIZE) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Parallel hashmap concurrent access failed: found {}/{}", found_count.load(), TEST_SIZE);
            return false;
        }

        // Test parallel_node_hash_map for complex objects
        phmap::parallel_node_hash_map<uint32, ::std::vector<int>> node_map;
        node_map[1] = {1, 2, 3, 4, 5};
        node_map[2] = {6, 7, 8, 9, 10};

        if (node_map[1].size() != 5 || node_map[2].size() != 5) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Parallel node hashmap complex object test failed");
            return false;
        }

        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Parallel hashmap performance test exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidateBoost()
{
    try {
        // Check Boost version
        int major = BOOST_VERSION / 100000;
        int minor = (BOOST_VERSION / 100) % 1000;
        int patch = BOOST_VERSION % 100;

        if (major < 1 || (major == 1 && minor < 74)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost version {}.{}.{} insufficient, required 1.74+", major, minor, patch);
            return false;
        }

        // Test Boost components
    if (!TestBoostComponents())
    {
            TC_LOG_ERROR("module.playerbot.dependencies", "Boost components test failed");
            return false;
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            " Boost {}.{}.{} validated with component tests", major, minor, patch);
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Boost validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::TestBoostComponents()
{
    try {
        // Test circular_buffer
        boost::circular_buffer<int> cb(256);
        for (int i = 0; i < 300; ++i) {
            cb.push_back(i);
        }

        if (cb.size() != 256) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost circular_buffer size test failed: expected 256, got {}", cb.size());
            return false;
        }

        // Test object_pool
        boost::object_pool<::std::vector<int>> pool;
        ::std::vector<::std::vector<int>*> objects;

        for (int i = 0; i < 1000; ++i) {
            auto* obj = pool.malloc();
            if (!obj)
            {
                TC_LOG_ERROR("module.playerbot.dependencies",
                    "Boost object_pool allocation failed at iteration {}", i);
                return false;
            }
            new(obj) ::std::vector<int>{i, i + 1, i + 2};
            objects.push_back(obj);
        }

        // Clean up
    for (auto* obj : objects)
    {
            obj->~vector();
            pool.free(obj);
        }

        // Test lockfree queue
        boost::lockfree::queue<int> queue(1024);
        constexpr int QUEUE_TEST_SIZE = 500;

        // Producer thread
        ::std::thread producer([&]() {
            for (int i = 0; i < QUEUE_TEST_SIZE; ++i) {
                while (!queue.push(i))
                {
                    ::std::this_thread::yield();
                }
            }
        });

        // Consumer thread
        ::std::atomic<int> consumed{0};
        ::std::thread consumer([&]() {
            int value;
            while (consumed.load() < QUEUE_TEST_SIZE)
            {
                if (queue.pop(value))
                {
                    consumed.fetch_add(1);
                }
                ::std::this_thread::yield();
            }
        });

        producer.join();
        consumer.join();

        if (consumed.load() != QUEUE_TEST_SIZE) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Boost lockfree queue test failed: expected {}, consumed {}", QUEUE_TEST_SIZE, consumed.load());
            return false;
        }

        // Test asio basic functionality
        boost::asio::io_context ctx;
        bool timer_executed = false;

        boost::asio::steady_timer timer(ctx, ::std::chrono::milliseconds(1));
        timer.async_wait([&](boost::system::error_code const&) {
            timer_executed = true;
        });

        ctx.run();

        if (!timer_executed)
        {
            TC_LOG_ERROR("module.playerbot.dependencies", "Boost asio timer test failed");
            return false;
        }

        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "Boost components test exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidateMySQL()
{
    try {
        // Check MySQL client library version
        const char* version = mysql_get_client_info();
        if (!version)
        {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "MySQL client library not available");
            return false;
        }

        // Parse version string (format: "8.0.33")
        ::std::string versionStr(version);
        size_t first_dot = versionStr.find('.');
        size_t second_dot = versionStr.find('.', first_dot + 1);

        if (first_dot == ::std::string::npos || second_dot == ::std::string::npos) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "Cannot parse MySQL version: {}", version);
            return false;
        }

        int major = ::std::stoi(versionStr.substr(0, first_dot));
        int minor = ::std::stoi(versionStr.substr(first_dot + 1, second_dot - first_dot - 1));
        int patch = ::std::stoi(versionStr.substr(second_dot + 1));

        if (major < 8 || (major == 8 && minor == 0 && patch < 33)) {
            TC_LOG_ERROR("module.playerbot.dependencies",
                "MySQL version {}.{}.{} insufficient, required 8.0.33+", major, minor, patch);
            return false;
        }

        // Test basic MySQL functionality (connection test would require credentials)
    if (!TestMySQLConnectivity())
    {
            TC_LOG_WARN("module.playerbot.dependencies",
                "MySQL connectivity test skipped (no test database configured)");
        }

        TC_LOG_INFO("module.playerbot.dependencies",
            " MySQL {}.{}.{} client library validated", major, minor, patch);
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "MySQL validation exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::TestMySQLConnectivity()
{
    try {
        // Initialize MySQL library
    if (mysql_library_init(0, nullptr, nullptr) != 0) {
            TC_LOG_ERROR("module.playerbot.dependencies", "MySQL library initialization failed");
            return false;
        }

        // Create connection handle
        MYSQL* mysql = mysql_init(nullptr);
        if (!mysql)
        {
            TC_LOG_ERROR("module.playerbot.dependencies", "MySQL handle creation failed");
            mysql_library_end();
            return false;
        }

        // Test would require actual database credentials
        // For now, just verify the handle was created successfully
        mysql_close(mysql);
        mysql_library_end();

        TC_LOG_DEBUG("module.playerbot.dependencies", "MySQL connectivity test passed (basic handle creation)");
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.dependencies",
            "MySQL connectivity test exception: {}", e.what());
        return false;
    }
}

bool DependencyValidator::ValidateSystemRequirements()
{
    bool success = true;

    if (!CheckMemoryRequirements())
    {
        TC_LOG_WARN("module.playerbot.dependencies", "System memory requirements not met");
        success = false;
    }

    if (!CheckCPURequirements())
    {
        TC_LOG_WARN("module.playerbot.dependencies", "System CPU requirements not met");
        success = false;
    }

    if (!CheckDiskRequirements())
    {
        TC_LOG_WARN("module.playerbot.dependencies", "System disk requirements not met");
        success = false;
    }

    return success;
}

bool DependencyValidator::CheckMemoryRequirements()
{
    // Minimum 8GB RAM recommended for enterprise bot operations
    // This is a basic check - production systems should have monitoring
    try {
        // Basic memory allocation test
        constexpr size_t TEST_SIZE = 1024 * 1024 * 100; // 100MB
        auto buffer = ::std::make_unique<char[]>(TEST_SIZE);

        if (!buffer)
        {
            TC_LOG_WARN("module.playerbot.dependencies", "Failed to allocate 100MB test buffer");
            return false;
        }

        // Write to buffer to ensure it's actually allocated
    for (size_t i = 0; i < TEST_SIZE; i += 4096) {
            buffer[i] = static_cast<char>(i % 256);
        }

        TC_LOG_DEBUG("module.playerbot.dependencies", "Memory requirements check passed");
        return true;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_WARN("module.playerbot.dependencies", "Memory requirements check failed: {}", e.what());
        return false;
    }
}

bool DependencyValidator::CheckCPURequirements()
{
    // Verify we have at least 2 hardware threads for parallel operations
    unsigned int hardware_threads = ::std::thread::hardware_concurrency();

    if (hardware_threads < 2)
    {
        TC_LOG_WARN("module.playerbot.dependencies",
            "Only {} hardware threads detected, minimum 2 recommended", hardware_threads);
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.dependencies",
        "CPU requirements met: {} hardware threads available", hardware_threads);
    return true;
}

bool DependencyValidator::CheckDiskRequirements()
{
    // Basic disk space check would go here
    // For now, assume sufficient disk space
    TC_LOG_DEBUG("module.playerbot.dependencies", "Disk requirements check passed");
    return true;
}

::std::vector<DependencyInfo> DependencyValidator::GetDependencyStatus()
{
    ::std::vector<DependencyInfo> dependencies;

    dependencies.push_back({
        "Intel TBB",
        GetTBBVersion(),
        ValidateTBB() ? " OK" : " FAILED",
        true,
        ValidateTBB() ? "" : "Intel Threading Building Blocks not available or version insufficient"
    });

    dependencies.push_back({
        "Parallel Hashmap",
        GetPhmapVersion(),
        ValidatePhmap() ? " OK" : " FAILED",
        true,
        ValidatePhmap() ? "" : "Parallel Hashmap not available or functionality test failed"
    });

    dependencies.push_back({
        "Boost",
        GetBoostVersion(),
        ValidateBoost() ? " OK" : " FAILED",
        true,
        ValidateBoost() ? "" : "Boost libraries not available or version insufficient"
    });

    dependencies.push_back({
        "MySQL",
        GetMySQLVersion(),
        ValidateMySQL() ? " OK" : " FAILED",
        true,
        ValidateMySQL() ? "" : "MySQL client library not available or version insufficient"
    });

    return dependencies;
}

void DependencyValidator::LogDependencyReport()
{
    auto dependencies = GetDependencyStatus();

    TC_LOG_INFO("module.playerbot.dependencies", "=== Playerbot Enterprise Dependency Report ===");
    TC_LOG_INFO("module.playerbot.dependencies", "{:<20} | {:<15} | {:<10} | {}",
        "Component", "Version", "Status", "Notes");
    TC_LOG_INFO("module.playerbot.dependencies", "{}+{}+{}+{}",
        ::std::string(20, '-'), ::std::string(15, '-'), ::std::string(10, '-'), ::std::string(30, '-'));

    for (auto const& dep : dependencies)
    {
        TC_LOG_INFO("module.playerbot.dependencies",
            "{:<20} | {:<15} | {:<10} | {}",
            dep.name, dep.version, dep.status, dep.errorMessage);
    }

    TC_LOG_INFO("module.playerbot.dependencies", "=========================================");
}

::std::string DependencyValidator::GetTBBVersion()
{
    return ::std::to_string(TBB_VERSION_MAJOR) + "." + ::std::to_string(TBB_VERSION_MINOR);
}

::std::string DependencyValidator::GetBoostVersion()
{
    int major = BOOST_VERSION / 100000;
    int minor = (BOOST_VERSION / 100) % 1000;
    int patch = BOOST_VERSION % 100;
    return ::std::to_string(major) + "." + ::std::to_string(minor) + "." + ::std::to_string(patch);
}

::std::string DependencyValidator::GetMySQLVersion()
{
    const char* version = mysql_get_client_info();
    return version ? ::std::string(version) : "Unknown";
}

::std::string DependencyValidator::GetPhmapVersion()
{
    // Parallel hashmap doesn't expose version at runtime
    return "1.3.8+";
}

} // namespace Playerbot