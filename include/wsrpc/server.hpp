#pragma once

#include <cassert>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <App.h>
#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "app.hpp"
#include "message.hpp"
#include "utility.hpp"

namespace wsrpc
{

struct Server
{
  /* ws->getUserData returns one of these */
  struct SocketData
  {
    /* Fill with user data */
    App app{};
  };

  static void init([[maybe_unused]] SocketData& sd)
  {
  }

  using attachs_t = std::vector<std::vector<std::byte>>;
  using package_t = std::pair<std::string, attachs_t>;

  static package_t handle(SocketData& sd, std::string_view raw)
  {
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
    auto result = sd.app.handle(request.method, request.params.str);
    if (!result) {
      SPDLOG_ERROR("Error calling {}: {}", raw, result.error());
      response.error = result.error();
      return pack(response);
    }
    response.result = std::move(result.value().first);
    return pack(response, std::move(result.value().second));
  }

  static void serve(const std::string host, const int port)
  {
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
           /* Open event here, you may access ws->getUserData() which points to a SocketData struct */
           SPDLOG_INFO("Socket opened");
           init(*ws->getUserData());
           SPDLOG_INFO("Socket initialized");
         },
       .message =
         [&]([[maybe_unused]] auto* ws, std::string_view message, uWS::OpCode opCode) {
           /* You may access ws->getUserData() here */
           SPDLOG_DEBUG("Message {} received", std::to_string(opCode));
           switch (opCode) {
             case uWS::OpCode::TEXT: {
               assert(not glz::validate_json(message));
               auto [resp, atts] = handle(*ws->getUserData(), message);
               SPDLOG_DEBUG("Response +{} generated: {}", atts.size(), resp);
               assert(not glz::validate_json(resp));
               for (auto& att : atts) ws->send(sv(att), uWS::OpCode::BINARY);
               ws->send(resp, uWS::OpCode::TEXT);
               break;
             }
             case uWS::OpCode::BINARY: {
               SPDLOG_ERROR("Binary message received but not supported");
               break;
             }
             default:
               SPDLOG_CRITICAL("Unexpected opcode", std::to_string(opCode));
               break;
           }
         },
       .dropped =
         [&]([[maybe_unused]] auto* ws, std::string_view message, uWS::OpCode opCode) {
           /* A message was dropped due to set maxBackpressure and closeOnBackpressureLimit limit */
           SPDLOG_WARN("Message dropped: {}, {}", std::to_string(opCode), message);
         },
       .drain =
         [&]([[maybe_unused]] auto* ws) {
           /* Check ws->getBufferedAmount() here */
           SPDLOG_WARN("Message drained");
         },
       .ping =
         [&]([[maybe_unused]] auto* ws, std::string_view message) {
           /* Not implemented yet */
           SPDLOG_DEBUG("Message ping received: {}", message);
         },
       .pong =
         [&]([[maybe_unused]] auto* ws, std::string_view message) {
           /* Not implemented yet */
           SPDLOG_DEBUG("Message pong received: {}", message);
         },
       .close =
         [&]([[maybe_unused]] auto* ws, int code, std::string_view message) {
           /* You may access ws->getUserData() here */
           SPDLOG_INFO("Socket closed: {}, {}", code, message);
           uWS::Loop::get()->defer([&]() {
             SPDLOG_INFO("Exiting...");
             u.close();  // TODO
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
