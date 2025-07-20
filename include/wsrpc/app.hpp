#pragma once

#include <expected>
#include <flat_map>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "message.hpp"

namespace wsrpc
{

struct App
{
  using rawjson_t = std::string;
  using binary_t = std::vector<std::byte>;
  using attachs_t = std::vector<binary_t>;
  using package_t = std::pair<rawjson_t, attachs_t>;
  using return_t = std::expected<package_t, std::string>;
  using handler_t = std::move_only_function<return_t(rawjson_t)>;

  std::flat_map<std::string, handler_t> handlers;

  App()
  {
  }

  void regist(const std::string& method, handler_t&& handler)
  {
    handlers.insert_or_assign(method, std::move(handler));
  }

  void unregist(const std::string& method)
  {
    handlers.erase(method);
  }

  return_t handle(const std::string& method, const rawjson_t& params) noexcept
  {
    auto func = handlers.find(method);
    if (func == handlers.end()) {
      return std::unexpected(error::format(error::METHOD_UNAVAIABLE, '"' + method + '"'));
    }
    auto& handler = func->second;
    try {
      return std::invoke(handler, params);
    }
    catch (const std::exception& e) {
      SPDLOG_ERROR("Uncaught Exception: {}", e.what());
    }
    catch (...) {
      SPDLOG_CRITICAL("Uncaught Exception: Unknown type");
    }
    return std::unexpected(error::format(error::INTERNAL_ERROR, '"' + method + '"'));
  }
};

}  // namespace wsrpc
