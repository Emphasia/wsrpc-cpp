#include <doctest/doctest.h>
#include <fmt/format.h>

#include <wsrpc/server.hpp>

std::optional<std::string> execute(const std::string& command)
{
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) return std::nullopt;

  char buffer[128];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }

  pclose(pipe);
  return result;
}

TEST_SUITE("server")
{
  TEST_CASE("SocketData initialization")
  {
    wsrpc::Server<>::SocketData socket_data;

    CHECK(socket_data.app != nullptr);
  }

  TEST_CASE("Server handle function with valid request")
  {
    wsrpc::Server<>::SocketData socket_data;

    // Register a test handler
    socket_data.app->regist("test_method", [](const wsrpc::rawjson_t&) {
      wsrpc::package_t package{R"({"result": "success"})", {}};
      return package;
    });

    // Test handling a valid request
    std::string_view valid_request = R"({"id": "1", "method": "test_method", "params": {}})";
    auto result = wsrpc::Server<>::handle(socket_data, valid_request);

    CHECK_FALSE(glz::validate_json(result.resp));

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.resp);
    REQUIRE_FALSE(pe);
    CHECK(response.id == "1");
    CHECK(response.result.str == R"({"result": "success"})");
    CHECK_FALSE(response.error.has_value());
  }

  TEST_CASE("Server handle function with invalid JSON")
  {
    wsrpc::Server<>::SocketData socket_data;

    // Test handling an invalid request (malformed JSON)
    std::string_view invalid_request = R"({"id": "1", "method": "test_method")";  // Missing closing brace
    auto result = wsrpc::Server<>::handle(socket_data, invalid_request);

    CHECK_FALSE(glz::validate_json(result.resp));

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.resp);
    REQUIRE_FALSE(pe);
    // CHECK(response.id.empty());
    // CHECK(response.result.str.empty());
    REQUIRE(response.error.has_value());
    CHECK(response.error.value().starts_with("Invalid Request : "));
  }

  TEST_CASE("Server handle function with unknown method")
  {
    wsrpc::Server<>::SocketData socket_data;

    // Test handling a request for an unregistered method
    std::string_view unknown_request = R"({"id": "1", "method": "unknown_method", "params": {}})";
    auto result = wsrpc::Server<>::handle(socket_data, unknown_request);

    CHECK_FALSE(glz::validate_json(result.resp));

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.resp);
    REQUIRE_FALSE(pe);
    CHECK(response.id == "1");
    // CHECK(response.result.str.empty());
    REQUIRE(response.error.has_value());
    CHECK(response.error.value() == "Method Unavaiable : \"unknown_method\"");
  }

  TEST_CASE("Server serve function")
  {
    constexpr auto host = "127.0.0.1";
    constexpr auto port = 9001;

    wsrpc::Server server;
    auto s = std::jthread([&]() { server.serve(host, port); });

    auto code = R"(
import sys
import asyncio
import aiohttp

async def main(url):
    session = aiohttp.ClientSession()
    client = await session.ws_connect(url)
    req = '{\"id\":\"1\",\"method\":\"echo\",\"params\":{}}'
    await client.send_str(req)
    res = await client.receive_str()
    print(res, end='')
    await client.close()
    await session.close()

if __name__ == '__main__':
    target = sys.argv[1]
    asyncio.run(main(target))
)";
    auto ret = execute(fmt::format("python -c \"{}\" ws://{}:{}", code, host, port).c_str());
    REQUIRE(ret);
    CHECK(*ret == R"({"id":"1","result":{}})");
  }
}