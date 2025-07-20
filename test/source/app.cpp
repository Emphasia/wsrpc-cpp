#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <doctest/doctest.h>

#include <wsrpc/app.hpp>

TEST_SUITE("app")
{
  TEST_CASE("App::handler_t")
  {
    auto handler1 = [](wsrpc::rawjson_t) -> void { return; };
    static_assert(not std::is_convertible_v<decltype(handler1), wsrpc::App::handler_t>);

    auto handler2 = []() -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_convertible_v<decltype(handler2), wsrpc::App::handler_t>);

    auto handler3 = [](wsrpc::rawjson_t) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler3), wsrpc::App::handler_t>);

    auto handler4 = [](const wsrpc::rawjson_t) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler4), wsrpc::App::handler_t>);

    auto handler5 = [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler5), wsrpc::App::handler_t>);

    auto handler6 = [](wsrpc::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_convertible_v<decltype(handler6), wsrpc::App::handler_t>);

    auto handler7 = [p = std::make_unique<int>()](const wsrpc::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_copy_constructible_v<decltype(handler7)>);
    static_assert(std::is_convertible_v<decltype(handler7), wsrpc::App::handler_t>);
  }

  TEST_CASE("App construction")
  {
    wsrpc::App app;

    // Test that app is constructed with default handlers
    REQUIRE(app.handlers.registry.size() == 1);
    CHECK(app.handlers.registry.contains("echo"));
  }

  TEST_CASE("App registration and unregistration")
  {
    wsrpc::App app;

    // Test registering a handler
    app.regist("test_method", [](const wsrpc::rawjson_t&) {
      wsrpc::package_t package{R"({"result": "success"})", {}};
      return package;
    });

    CHECK(app.handlers.registry.size() == 2);
    CHECK(app.handlers.registry.contains("test_method"));

    // Test registering another handler for the same method (should replace)
    app.regist("test_method", [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
      wsrpc::package_t package{R"({"result": "updated"})", {}};
      return package;
    });

    CHECK(app.handlers.registry.size() == 2);

    // Test unregistering a handler
    app.unregist("test_method");
    CHECK(app.handlers.registry.size() == 1);
  }

  TEST_CASE("App handle method")
  {
    wsrpc::App app;

    // Test handling a non-existent method
    auto result = app.handle("nonexistent_method", "{}");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == "Method Unavaiable : \"nonexistent_method\"");

    // Test handling an existing method
    app.regist("test_method", [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
      wsrpc::package_t package{R"({"result": "success"})", {}};
      return package;
    });

    auto result2 = app.handle("test_method", R"({"param": "value"})");
    REQUIRE(result2.has_value());
    CHECK(result2.value().first == R"({"result": "success"})");
    CHECK(result2.value().second.empty());

    // Test handling a method that throws an exception
    app.regist("throwing_method", [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
      throw std::runtime_error("Test exception");
    });

    auto result3 = app.handle("throwing_method", "{}");
    REQUIRE_FALSE(result3.has_value());
    CHECK(result3.error() == "Internal Error : \"throwing_method\"");
  }

  TEST_CASE("App thread safety" * doctest::timeout(10.0))
  {
    wsrpc::App app;

    // Add initial method
    app.regist("initial_method", [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
      wsrpc::package_t package{R"({"result": "initial"})", {}};
      return package;
    });

    const int num_threads = std::thread::hardware_concurrency();
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    // Launch threads that will perform concurrent operations
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&app, &success_count, t]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (int i = 0; i < operations_per_thread; ++i) {
          int operation = dis(gen);

          if (operation <= 30) {
            // 30% chance: Register a new method
            std::string method_name = "method_" + std::to_string(t) + "_" + std::to_string(i);
            app.regist(method_name, [method_name](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
              wsrpc::package_t package{R"({"result": "registered"})", {}};
              return package;
            });
          }
          else if (operation <= 50) {
            // 20% chance: Unregister a method
            std::string method_name = "method_" + std::to_string(t) + "_" + std::to_string(i / 2);
            app.unregist(method_name);
          }
          else {
            // 50% chance: Handle a method
            std::string method_name = (dis(gen) % 2 == 0) ? "initial_method" : "nonexistent_method";
            auto result = app.handle(method_name, "{}");
            if (result.has_value() || !result.has_value()) {
              // Just to prevent compiler optimizations
              success_count.fetch_add(1);
            }
          }

          // Small delay to increase chance of thread interleaving
          std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
      });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    // Verify that the app is still functional
    auto result = app.handle("initial_method", "{}");
    CHECK(result.has_value());

    // Just check that we did some operations
    CHECK(success_count.load() > 0);

    // Note: We can't check exact registry size because of the random nature of the test
    // But the main point is that no data races or crashes occurred
  }

  TEST_CASE("App concurrent handle calls" * doctest::timeout(5.0))
  {
    wsrpc::App app;

    // Register a method that takes some time to execute
    app.regist("slow_method", [](const wsrpc::rawjson_t&) -> wsrpc::App::return_t {
      // Small sleep to increase chance of concurrent execution
      std::this_thread::sleep_for(std::chrono::milliseconds(3));
      wsrpc::package_t package{R"({"result": "slow"})", {}};
      return package;
    });

    const int num_threads = std::thread::hardware_concurrency();
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    // Launch threads that will all call the same method concurrently
    for (int t = 0; t < num_threads; ++t) {
      threads.emplace_back([&app, &success_count]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (int i = 0; i < operations_per_thread; ++i) {
          auto result = app.handle("slow_method", "{}");
          if (result.has_value()) {
            success_count.fetch_add(1);
          }
          // Small delay to vary timing
          std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
        }
      });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    // Verify that all calls were successful
    CHECK(success_count.load() == num_threads * operations_per_thread);
  }
}