#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <fstream>
#include <functional>
#include <mutex>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fmt/chrono.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace wsrpc
{

inline void init_logger()
{
  if (spdlog::get("wsrpc")) return;
  auto logger = spdlog::stderr_color_mt("wsrpc");
#ifdef NDEBUG
  logger->set_level(spdlog::level::info);
  logger->set_pattern("%Y-%m-%d %T.%e | %^%L%$ | %s:%# | %v");
#else
  logger->set_level(spdlog::level::debug);
  logger->set_pattern("%Y-%m-%d %T.%e | %^%-4!l%$ | %s:%# | %t | %v");
#endif
  spdlog::set_default_logger(logger);
}

inline void init_exception_handler()
{
  std::set_terminate([] {
    auto ex_ptr = std::current_exception();
    if (ex_ptr) {
      try {
        std::rethrow_exception(ex_ptr);
      }
      catch (const std::exception& e) {
        SPDLOG_CRITICAL("Uncaught Exception: {}", e.what());
      }
      catch (...) {
        SPDLOG_CRITICAL("Uncaught Exception: Unknown type");
      }
    }
    else {
      SPDLOG_CRITICAL("Terminate called without active exception");
    }
    std::abort();
  });
}

inline std::string_view sv(const std::vector<std::byte>& data)
{
  if (data.empty()) {
    return std::string_view();
  }
  return std::string_view(reinterpret_cast<const char*>(data.data()), data.size());
}

inline std::vector<std::byte> read_bytes(const std::string& filePath)
{
  // Open file in binary mode at the end
  std::ifstream in(filePath, std::ios::binary | std::ios::ate);
  if (!in) {
    throw std::runtime_error("Cannot open file: " + filePath);
  }

  // Determine file size
  std::streampos fileSize = in.tellg();
  if (fileSize == -1 || !in) {
    throw std::runtime_error("Error getting file size: " + filePath);
  }

  // Check if file size exceeds size_t limits
  auto size = static_cast<std::uintmax_t>(fileSize);
  if (size > std::numeric_limits<std::size_t>::max()) {
    throw std::runtime_error("File too large: " + filePath);
  }

  // Seek to beginning
  in.seekg(0, std::ios::beg);
  if (!in) {
    throw std::runtime_error("Seek to beginning failed: " + filePath);
  }

  // Read into byte vector
  std::vector<std::byte> content;
  content.resize(static_cast<std::size_t>(size));
  if (size > 0) {
    in.read(reinterpret_cast<char*>(content.data()), static_cast<std::streamsize>(size));
    if (in.gcount() != static_cast<std::streamsize>(size)) {
      throw std::runtime_error("Read incomplete: " + filePath);
    }
  }
  return content;
}

inline std::string read_text(const std::string& filePath)
{
  // Open file in binary mode at the end
  std::ifstream in(filePath, std::ios::binary | std::ios::ate);
  if (!in) {
    throw std::runtime_error("Cannot open file: " + filePath);
  }

  // Determine file size
  std::streampos fileSize = in.tellg();
  if (fileSize == -1 || !in) {
    throw std::runtime_error("Error getting file size: " + filePath);
  }

  // Check if file size exceeds size_t limits
  auto size = static_cast<std::uintmax_t>(fileSize);
  if (size > std::numeric_limits<std::size_t>::max()) {
    throw std::runtime_error("File too large: " + filePath);
  }

  // Seek to beginning
  in.seekg(0, std::ios::beg);
  if (!in) {
    throw std::runtime_error("Seek to beginning failed: " + filePath);
  }

  // Read into string
  std::string content;
  content.resize(static_cast<std::size_t>(size));
  if (size > 0) {
    in.read(&content[0], static_cast<std::streamsize>(size));
    if (in.gcount() != static_cast<std::streamsize>(size)) {
      throw std::runtime_error("Read incomplete: " + filePath);
    }
  }
  return content;
}

class ScheduledTask
{
public:
  using Task = std::move_only_function<void()>;

  ScheduledTask(std::string_view name, Task&& task)
  {
    assert(not name.empty() and task);
    name_ = std::string(name);
    task_ = std::move(task);
    canceled_.store(true);
  }

  ~ScheduledTask()
  {
    cancel();
  }

  void schedule(std::chrono::milliseconds delay)
  {
    assert(delay > std::chrono::milliseconds(0));
    cancel();
    SPDLOG_DEBUG("{} scheduled with {}", name_, delay);
    std::lock_guard<std::mutex> lock(mutex_);
    scheduled_time_ = std::chrono::steady_clock::now() + delay;
    canceled_.store(false);
    assert(!worker_thread_.joinable());
    worker_thread_ = std::jthread([this] { run(); });
  }

  void cancel()
  {
    if (canceled_.exchange(true)) return;
    cv_.notify_all();
    worker_thread_ = {};
    SPDLOG_DEBUG("{} canceled", name_);
  }

private:
  void run()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_until(lock, scheduled_time_, [this] { return canceled_.load(); });
    if (!canceled_) {
      SPDLOG_DEBUG("{} executing...", name_);
      task_();
    }
  }

private:
  std::string name_;
  Task task_;
  std::mutex mutex_;
  std::atomic<bool> canceled_;
  std::chrono::steady_clock::time_point scheduled_time_;
  std::condition_variable cv_;
  std::jthread worker_thread_;
};

class Timer
{
public:
  explicit inline Timer(
    std::string_view context,
    spdlog::level::level_enum level = spdlog::level::debug,
    std::source_location location = std::source_location::current())
    : context_(context), level_(level), location_(location), start_(std::chrono::high_resolution_clock::now())
  {
  }

  inline ~Timer()
  {
    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> ms = end - start_;
    const spdlog::source_loc loc{location_.file_name(), static_cast<int>(location_.line()), location_.function_name()};
    spdlog::log(loc, level_, "{} took {:.3f} ms", context_, ms.count());
  }

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

private:
  const std::string_view context_;
  const spdlog::level::level_enum level_;
  const std::source_location location_;
  const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

#define TIMEIT wsrpc::Timer _timeit_timer(__FUNCTION__)
#define TIMEIT_(_level) wsrpc::Timer _timeit_timer(__FUNCTION__, spdlog::level::level_enum(_level))

}  // namespace wsrpc
