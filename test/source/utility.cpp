#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include <doctest/doctest.h>

#include <wsrpc/utility.hpp>

TEST_SUITE("utility")
{
  TEST_CASE("sv function")
  {
    // Test with empty vector
    std::vector<std::byte> empty_data;
    auto empty_sv = wsrpc::sv(empty_data);
    CHECK(empty_sv.empty());
    CHECK(empty_sv.size() == 0);

    // Test with data
    std::vector<std::byte> data{std::byte('H'), std::byte('e'), std::byte('l'), std::byte('l'), std::byte('o')};
    auto data_sv = wsrpc::sv(data);
    CHECK_FALSE(data_sv.empty());
    CHECK(data_sv.size() == 5);
    CHECK(std::string(data_sv) == "Hello");
  }

  TEST_CASE("ScheduledTask schedule")
  {
    std::atomic<bool> executed{false};
    {
      wsrpc::ScheduledTask task("test_task", [&executed]() { executed = true; });
      task.schedule(std::chrono::milliseconds(10));

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    CHECK(executed.load() == true);
  }

  TEST_CASE("ScheduledTask cancel")
  {
    std::atomic<bool> executed{false};
    {
      wsrpc::ScheduledTask task("test_task", [&executed]() { executed = true; });
      task.schedule(std::chrono::milliseconds(100));
      task.cancel();

      std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    CHECK(executed.load() == false);
  }

  TEST_CASE("ScheduledTask reschedule")
  {
    std::atomic<int> execution_count{0};
    {
      wsrpc::ScheduledTask task("test_task", [&execution_count]() { execution_count++; });

      task.schedule(std::chrono::milliseconds(50));
      // Wait a bit for the first scheduling
      std::this_thread::sleep_for(std::chrono::milliseconds(25));

      // Reschedule - this should cancel the first and schedule a new one
      task.schedule(std::chrono::milliseconds(50));

      // Wait for the rescheduled task to complete
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Should only execute once (the rescheduled one)
    CHECK(execution_count.load() == 1);
  }
}