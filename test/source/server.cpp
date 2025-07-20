#include <filesystem>

#include <doctest/doctest.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <wsrpc/app.hpp>
#include <wsrpc/message.hpp>
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

  TEST_CASE("Server serve function echo")
  {
    static const auto host = "127.0.0.1";
    static const auto port = 9001;

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

  TEST_CASE("Server serve function data")
  {
    static const auto path = std::filesystem::path(ROOTPATH);
    static const auto host = "127.0.0.1";
    static const auto port = 9001;

    struct AppT : wsrpc::App
    {
      struct Data
      {
        glz::json_t json_commit;
        glz::json_t json_tree;
        wsrpc::binary_t jpg_404;
        wsrpc::binary_t jpg_landing;
      };

      static Data load()
      {
        const std::filesystem::path _path = path / "data";
        return Data{
          .json_commit = glz::read_json<glz::json_t>(wsrpc::read_text(_path / "latest-commit@pbr-book.json")).value(),
          .json_tree = glz::read_json<glz::json_t>(wsrpc::read_text(_path / "tree-commit-info@pbr-book.json")).value(),
          .jpg_404 = wsrpc::read_bytes(_path / "404@pbr-book.jpg"),
          .jpg_landing = wsrpc::read_bytes(_path / "landing@pbr-book.jpg"),
        };
      }

      const Data data = load();

      AppT() : App()
      {
        regist("test0", [&](const wsrpc::rawjson_t&) -> wsrpc::package_t {
          return {data.json_tree.dump().value(), {data.jpg_landing}};
        });
        regist("test1", [&](const wsrpc::rawjson_t&) -> wsrpc::package_t {
          auto j = data.json_tree;
          j["data"] = glz::write_base64(wsrpc::sv(data.jpg_landing));
          return {j.dump().value(), {}};
        });
      }
    };

    wsrpc::Server<AppT> server;
    auto s = std::jthread([&]() {
      server.serve(host, port);
      // TODO
      server.serve(host, port);
    });

    auto ret = std::system(
      fmt::to_string(
        fmt::join(
          {
            //
            fmt::format("cd {}", path.string()),                                 //
            fmt::format("python client.py ws://{}:{} {}", host, port, "test0"),  //
            fmt::format("python client.py ws://{}:{} {}", host, port, "test1"),  //
          },                                                                     //
          " && "))
        .c_str());
    std::cout << "py exited with: " << ret << std::endl;
    CHECK(ret == 0);
  }
}