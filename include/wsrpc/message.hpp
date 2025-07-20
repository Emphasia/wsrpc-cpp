#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <glaze/glaze.hpp>

namespace wsrpc
{

using rawjson_t = std::string;
using binary_t = std::vector<std::byte>;
using attachs_t = std::vector<binary_t>;
using package_t = std::pair<rawjson_t, attachs_t>;

struct request_t
{
  std::string id{};
  std::string method{};
  glz::raw_json params{};
  operator bool() const
  {
    return !id.empty() && !method.empty() && !params.str.empty();
  }
};

struct response_t
{
  std::string id{};
  glz::raw_json result{};
  std::optional<std::string> error{};
  operator bool() const
  {
    return !id.empty() && (!result.str.empty() || error.has_value());
  }
};

namespace error
{
inline auto format(const std::string_view& type, const std::string& msg)
{
  auto f = fmt::format("{} : {}", type, msg);
  return f;
}
static constexpr std::string_view INVALID_REQUEST = "Invalid Request";
static constexpr std::string_view INVALID_RESPONSE = "Invalid Response";
static constexpr std::string_view METHOD_UNAVAIABLE = "Method Unavaiable";
static constexpr std::string_view INVALID_PARAMS = "Invalid Params";
static constexpr std::string_view INTERNAL_ERROR = "Internal Error";
}  // namespace error

}  // namespace wsrpc
