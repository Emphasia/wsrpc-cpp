#pragma once

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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

static constexpr std::string_view base64_chars =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

inline std::string encode_base64(const std::vector<std::byte>& input)
{
  std::string encoded;
  int i = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  for (const auto& c : input) {
    char_array_3[i++] = std::to_integer<unsigned char>(c);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; i < 4; i++) {
        encoded += base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }

  if (i) {
    for (int j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (int j = 0; j < i + 1; j++) {
      encoded += base64_chars[char_array_4[j]];
    }

    while (i++ < 3) {
      encoded += '=';
    }
  }

  return encoded;
}

}  // namespace wsrpc