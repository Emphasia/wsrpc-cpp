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

template <std::derived_from<App> App_t = App>
requires std::default_initializable<App_t>
struct Server
{
  static inline std::atomic<unsigned int> count{0};

  /* ws->getUserData returns one of these */
  struct SocketData
  {
    using Pool_t = BS::thread_pool<>;
    std::shared_ptr<Pool_t> pool =
      std::make_shared<Pool_t>(std::clamp((int)std::thread::hardware_concurrency() / 3, 8, 24));
    std::unique_ptr<App_t> app = std::make_unique<App_t>();
  };

  using package_t = struct
  {
    rawjson_t resp;
    attachs_t atts;
  };

  static package_t handle(SocketData& sd, std::string_view raw)
  {
    TIMEIT;
    assert(sd.app);
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
    auto result = sd.app->handle(request.method, request.params.str);
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

  static void serve(const std::string host, const int port)
  {
    ScheduledTask task;
    uWS::App u;
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
           task.cancel();
         },
       .message =
         [&]([[maybe_unused]] auto* ws, std::string_view message, uWS::OpCode opCode) {
           /* A message received */
           SPDLOG_DEBUG("Message received: {}, {}", std::to_string(opCode), message);
           auto& sd = *ws->getUserData();
           switch (opCode) {
             case uWS::OpCode::TEXT: {
               sd.pool->detach_task([loop = uWS::Loop::get(), ws, message = std::string(message)]() {
                 if (us_socket_is_closed(0, (us_socket_t*)ws)) return;
                 auto& sd = *ws->getUserData();
                 assert(not glz::validate_json(message));
                 auto pkg = handle(sd, message);
                 SPDLOG_DEBUG("Response +{} generated: {}", pkg.atts.size(), pkg.resp);
                 assert(not glz::validate_json(pkg.resp));
                 loop->defer([ws, pkg = std::move(pkg)]() { Server::reply(ws, pkg); });
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
           SPDLOG_DEBUG("Message ping received: {}", message);
         },
       .pong =
         [&]([[maybe_unused]] auto* ws, std::string_view message) {
           /* A pong message received */
           SPDLOG_DEBUG("Message pong received: {}", message);
         },
       .close =
         [&]([[maybe_unused]] auto* ws, int code, std::string_view message) {
           /* This connection closing */
           SPDLOG_INFO("Socket closed: {}, {}", code, message);
           SPDLOG_INFO("Remote at {}:{}", ws->getRemoteAddressAsText(), us_socket_remote_port(0, (us_socket_t*)ws));
           count--;
           uWS::Loop::get()->defer([&]() {
             if (count > 0) return;
             SPDLOG_INFO("Exiting in 5 seconds...");
             task.schedule(
               "exit",
               [&]() {
                 SPDLOG_INFO("Exiting...");
                 u.close();
               },
               std::chrono::seconds(5));
           });
         }});
    u.listen(host, port, [&](auto* listen_socket) {
      if (!listen_socket) {
        SPDLOG_CRITICAL("Unavailable on {}:{}", host, port);
        throw std::runtime_error("Unavailable");
      }
      SPDLOG_INFO("Listening on {}:{}", host, port);
    });
    u.run();
  }
};

}  // namespace wsrpc
