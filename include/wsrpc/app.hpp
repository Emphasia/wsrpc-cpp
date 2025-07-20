#pragma once

#include <expected>
#include <flat_map>
#include <functional>
#include <string>

#include <spdlog/spdlog.h>

#include "message.hpp"

namespace wsrpc
{

struct App
{
  using return_t = std::expected<package_t, std::string>;
  using handler_t = std::move_only_function<return_t(rawjson_t)>;

  std::flat_map<std::string, handler_t> handlers = {};

  App()
  {
    regist("echo", [](const rawjson_t& params) -> return_t { return package_t{params, {}}; });
  }

  virtual ~App() = default;

  App(const App&) = delete;
  App(App&&) = default;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = default;

  void regist(const std::string& method, handler_t&& handler)
  {
    SPDLOG_INFO("Registering method: {}", method);
    handlers.insert_or_assign(method, std::move(handler));
  }

  void unregist(const std::string& method)
  {
    SPDLOG_INFO("Unregistering method: {}", method);
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
