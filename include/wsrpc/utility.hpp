#pragma once

#include <cstddef>
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

}  // namespace wsrpc