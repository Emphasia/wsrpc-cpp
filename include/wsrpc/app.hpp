#pragma once

#include <expected>
#include <flat_map>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>

#include <spdlog/spdlog.h>

#include "wsrpc/message.hpp"

namespace wsrpc
{

class App
{
public:
  using return_t = std::expected<package_t, std::string>;
  using handler_t = std::move_only_function<return_t(rawjson_t)>;

public:
  struct
  {
    std::shared_mutex mutex = {};
    std::flat_map<std::string, std::shared_ptr<handler_t>> registry = {};
  } handlers = {};

public:
  App()
  {
    SPDLOG_INFO("App created");
    regist("echo", [](const rawjson_t& params) -> return_t { return package_t{params, {}}; });
  }

  virtual ~App()
  {
    SPDLOG_INFO("App destroyed");
  }

  App(const App&) = delete;
  App(App&&) = delete;
  App& operator=(const App&) = delete;
  App& operator=(App&&) = delete;

  void regist(const std::string& method, handler_t&& handler)
  {
    SPDLOG_INFO("Registering method: {}", method);
    auto& [mutex, registry] = handlers;
    std::lock_guard lock(mutex);
    registry.insert_or_assign(method, std::make_shared<handler_t>(std::move(handler)));
  }

  void unregist(const std::string& method)
  {
    SPDLOG_INFO("Unregistering method: {}", method);
    auto& [mutex, registry] = handlers;
    std::lock_guard lock(mutex);
    registry.erase(method);
  }

  return_t handle(const std::string& method, const rawjson_t& params)
  {
    std::shared_ptr<handler_t> handler;
    {
      auto& [mutex, registry] = handlers;
      std::lock_guard lock(mutex);
      auto func = registry.find(method);
      if (func == registry.end()) {
        return std::unexpected(error::format(error::METHOD_UNAVAIABLE, '"' + method + '"'));
      }
      handler = func->second;
    }
    try {
      return std::invoke(*handler, params);
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
