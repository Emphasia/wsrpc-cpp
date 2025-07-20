#pragma once

#include <chrono>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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