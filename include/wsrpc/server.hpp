#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>

#include "wsrpc/app.hpp"
#include "wsrpc/message.hpp"
#include "wsrpc/utility.hpp"

namespace wsrpc
{

struct Options
{
  std::string host = "127.0.0.1";
  int port = 8080;
  size_t timeout_secs = 5;
  size_t threads_num = std::clamp((int)std::thread::hardware_concurrency() / 3, 8, 24);
};

class Server
{
public:
  using factory_t = std::move_only_function<std::unique_ptr<App>()>;

public:
  explicit Server(factory_t&& app_factory) : app_factory(std::move(app_factory))
  {
  }

  void operator()(const Options& options);

private:
  factory_t app_factory;
};

template <std::derived_from<App> App_t = App>
requires std::default_initializable<App_t>
void serve(const Options& options)
{
  Server server = Server([] { return std::make_unique<App_t>(); });
  server(options);
}

using packet_t = struct
{
  rawjson_t resp;
  attachs_t atts;
};

inline packet_t process(App& app, std::string_view raw)
{
  TIMEIT_(0);
  request_t request{};
  response_t response{.result = "null"};
  auto pack = [](const response_t& resp, attachs_t&& atts = {}) -> packet_t {
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

}  // namespace wsrpc
