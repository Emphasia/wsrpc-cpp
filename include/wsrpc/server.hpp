#pragma once

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>

#include <App.h>
#include <BS_thread_pool.hpp>
#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "app.hpp"
#include "message.hpp"
#include "utility.hpp"

namespace wsrpc
{
struct Options
{
  std::string host = "127.0.0.1";
  int port = 8080;
  size_t timeout_secs = 5;
  size_t threads_num = std::clamp((int)std::thread::hardware_concurrency() / 3, 8, 24);
};

template <std::derived_from<App> App_t = App>
requires std::default_initializable<App_t>
struct Server
{
  static inline std::atomic<unsigned int> count{0};

  /* ws->getUserData returns one of these */
  struct SocketData
  {
    std::unique_ptr<BS::wdc_thread_pool> pool;
    std::unique_ptr<App_t> app;
  };

  static void build(SocketData& sd, size_t threads_num = std::thread::hardware_concurrency())
  {
    SPDLOG_INFO("Building data for socket...");
    SPDLOG_INFO("Making pool with threads: {}...", threads_num);
    sd.pool = std::make_unique<BS::wdc_thread_pool>(threads_num);
    SPDLOG_INFO("Making app...");
    sd.app = std::make_unique<App_t>();
  }

  static void destroy(SocketData& sd)
  {
    SPDLOG_INFO("Destroying data for socket...");
    SPDLOG_DEBUG("Stopping pool with tasks: {}...", sd.pool->get_tasks_total());
    sd.pool->purge();
    SPDLOG_DEBUG("Waiting pool with tasks: {}...", sd.pool->get_tasks_total());
    sd.pool->wait();
    SPDLOG_INFO("Destroying pool...");
    sd.pool.reset();
    SPDLOG_INFO("Destroying app...");
    sd.app.reset();
  }

  using package_t = struct
  {
    rawjson_t resp;
    attachs_t atts;
  };

  static package_t handle(App_t& app, std::string_view raw)
  {
    TIMEIT;
    request_t request{};
    response_t response{.result = "null"};
    auto pack = [](const response_t& resp, attachs_t&& atts = {}) -> package_t {
      assert(resp);
      auto pr = glz::write_json(resp);
      if (!pr) [[unlikely]] {
        auto error_msg = error::format(error::INVALID_RESPONSE, glz::format_error(pr.error()));
        SPDLOG_ERROR(error_msg);
        auto parse_error = resp;
        parse_error.error = error_msg;
        return {glz::write_json(parse_error).value(), {}};
      }
      return {std::move(pr).value(), std::move(atts)};
    };
    auto pe = glz::read_json(request, raw);
    if (pe || !request) [[unlikely]] {
      if (!request.id.empty()) response.id = request.id;
      auto error_msg = error::format(error::INVALID_REQUEST, pe ? glz::format_error(pe, raw) : "field invalid");
      SPDLOG_ERROR(error_msg);
      response.error = error_msg;
      return pack(response);
    }
    response.id = request.id;
    auto result = app.handle(request.method, request.params.str);
    if (!result) {
      SPDLOG_ERROR("Error calling {}: {}", raw, result.error());
      response.error = result.error();
      return pack(response);
    }
    response.result = std::move(result.value().first);
    return pack(response, std::move(result.value().second));
  }

  static void reply(auto* ws, const package_t& pkg)
  {
    for (auto& att : pkg.atts | std::views::reverse)  //
      ws->send(sv(att), uWS::OpCode::BINARY);
    ws->send(pkg.resp, uWS::OpCode::TEXT);
  }

  static void serve(const Options& options)
  {
    uWS::App u;
    ScheduledTask shutdown("exit", [&]() {
      u.getLoop()->defer([&]() {
        SPDLOG_INFO("Exiting...");
        u.close();
        SPDLOG_INFO("Exited");
      });
    });
    u.ws<SocketData>(
      "/*",
      {/* Settings */
       .compression = uWS::DISABLED,
       .maxPayloadLength = 10 * 1024 * 1024,
       .idleTimeout = 60,
       .maxBackpressure = 100 * 1024 * 1024,
       .closeOnBackpressureLimit = false,
       .resetIdleTimeoutOnSend = true,
       .sendPingsAutomatically = true,

       /* Handlers */
       .upgrade = nullptr,
       .open =
         [&]([[maybe_unused]] auto* ws) {
           /* This connection opened */
           SPDLOG_INFO("Socket opened");
           SPDLOG_INFO("Remote at {}:{}", ws->getRemoteAddressAsText(), us_socket_remote_port(0, (us_socket_t*)ws));
           count++;
           shutdown.cancel();
           auto& sd = *ws->getUserData();
           build(sd, options.threads_num);
         },
       .message =
         [&]([[maybe_unused]] auto* ws, std::string_view message, uWS::OpCode opCode) {
           /* A message received */
           SPDLOG_TRACE("Message received: {}, {}", std::to_string(opCode), message);
           auto& sd = *ws->getUserData();
           switch (opCode) {
             case uWS::OpCode::TEXT: {
               sd.pool->detach_task([&u, ws, message = std::string(message)]() {
                 if (us_socket_is_closed(0, (us_socket_t*)ws)) return;
                 auto& sd = *ws->getUserData();
                 assert(not glz::validate_json(message));
                 auto pkg = handle(*sd.app, message);
                 SPDLOG_TRACE("Response +{} generated: {}", pkg.atts.size(), pkg.resp);
                 assert(not glz::validate_json(pkg.resp));
                 u.getLoop()->defer([ws, pkg = std::move(pkg)]() { reply(ws, pkg); });
               });
               break;
             }
             case uWS::OpCode::BINARY: {
               SPDLOG_ERROR("Binary message received but not supported");
               break;
             }
             default:
               SPDLOG_CRITICAL("Unexpected OpCode: {}", std::to_string(opCode));
               break;
           }
         },
       .dropped =
         [&]([[maybe_unused]] auto* ws, std::string_view message, uWS::OpCode opCode) {
           /* A sending message dropped */
           SPDLOG_WARN("Message dropped: {}, {}", std::to_string(opCode), message);
         },
       .drain =
         [&]([[maybe_unused]] auto* ws) {
           /* All sending messages drained */
           SPDLOG_DEBUG("Message drained");
         },
       .ping =
         [&]([[maybe_unused]] auto* ws, std::string_view message) {
           /* A ping message received */
           SPDLOG_TRACE("Message ping received: {}", message);
         },
       .pong =
         [&]([[maybe_unused]] auto* ws, std::string_view message) {
           /* A pong message received */
           SPDLOG_TRACE("Message pong received: {}", message);
         },
       .close =
         [&]([[maybe_unused]] auto* ws, int code, std::string_view message) {
           /* This connection closing */
           SPDLOG_INFO("Socket closed: {}, {}", code, message);
           SPDLOG_INFO("Remote at {}:{}", ws->getRemoteAddressAsText(), us_socket_remote_port(0, (us_socket_t*)ws));
           auto& sd = *ws->getUserData();
           destroy(sd);
           count--;
           if (count == 0) {
             SPDLOG_INFO("Exiting in {} seconds...", options.timeout_secs);
             shutdown.schedule(std::chrono::seconds(options.timeout_secs));
           }
         }});
    u.listen(options.host, options.port, [&](auto* listen_socket) {
      if (!listen_socket) {
        SPDLOG_CRITICAL("Unavailable on {}:{}", options.host, options.port);
        throw std::runtime_error("Unavailable");
      }
      SPDLOG_INFO("Listening on {}:{}", options.host, options.port);
      SPDLOG_INFO("Exiting in {} seconds...", options.timeout_secs);
      shutdown.schedule(std::chrono::seconds(options.timeout_secs));
    });
    u.run();
  }
};

}  // namespace wsrpc
