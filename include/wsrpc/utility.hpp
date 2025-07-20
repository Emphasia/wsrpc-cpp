#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <fstream>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

namespace wsrpc
{

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

  void schedule(std::string_view name, Task&& task, std::chrono::milliseconds delay)
  {
    assert(not name.empty() and task and delay > std::chrono::milliseconds(0));
    name_ = std::string(name);
    task_ = std::move(task);
    scheduled_time_ = std::chrono::steady_clock::now() + delay;
    SPDLOG_DEBUG("{} scheduled with {}", name_, delay);
    worker_thread_ = std::jthread([this] { run(); });
  }

  void cancel()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!task_) return;
    task_ = nullptr;
    lock.unlock();
    SPDLOG_DEBUG("{} canceled", name_);
    cv_.notify_all();
  }

private:
  void run()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_until(lock, scheduled_time_, [this] { return not bool(task_); });
    if (task_) {
      SPDLOG_DEBUG("{} executing...", name_);
      task_();
    }
  }

private:
  std::string name_;
  Task task_;
  std::chrono::steady_clock::time_point scheduled_time_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::jthread worker_thread_;
};

class Timer
{
public:
  explicit Timer(std::string_view context) : context_(context), start_(std::chrono::high_resolution_clock::now())
  {
  }

  ~Timer()
  {
    if (!cancelled_) {
      const auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> ms = end - start_;
      SPDLOG_DEBUG("{} took {:.3f} ms", context_, ms.count());
    }
  }

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  void cancel() noexcept
  {
    cancelled_ = true;
  }

private:
  std::string_view context_;
  const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
  bool cancelled_ = false;
};

#define TIMEIT wsrpc::Timer _timeit_timer(__FUNCTION__)

}  // namespace wsrpc
