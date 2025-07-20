#include <doctest/doctest.h>

#include <wsrpc/server.hpp>

TEST_SUITE("server")
{
  TEST_CASE("SocketData initialization")
  {
    wsrpc::Server::SocketData socket_data;

    // Test that SocketData is properly constructed with an App
    CHECK(socket_data.app.handlers.empty());
  }

  TEST_CASE("Server init function")
  {
    wsrpc::Server::SocketData socket_data;

    // Test that init function can be called without throwing
    CHECK_NOTHROW(wsrpc::Server::init(socket_data));
  }

  TEST_CASE("Server handle function with valid request")
  {
    wsrpc::Server::SocketData socket_data;

    // Register a test handler
    socket_data.app.regist("test_method", [](const std::string& params) {
      wsrpc::App::package_t package{R"({"result": "success"})", {}};
      return wsrpc::App::return_t(package);
    });

    // Test handling a valid request
    std::string_view valid_request = R"({"id": "1", "method": "test_method", "params": {}})";
    auto result = wsrpc::Server::handle(socket_data, valid_request);

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.first);
    REQUIRE_FALSE(pe);
    CHECK(response.id == "1");
    CHECK(response.result.str == R"({"result": "success"})");
    CHECK_FALSE(response.error.has_value());
  }

  TEST_CASE("Server handle function with invalid JSON")
  {
    wsrpc::Server::SocketData socket_data;

    // Test handling an invalid request (malformed JSON)
    std::string_view invalid_request = R"({"id": "1", "method": "test_method")";  // Missing closing brace
    auto result = wsrpc::Server::handle(socket_data, invalid_request);

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.first);
    REQUIRE_FALSE(pe);
    CHECK(response.id.empty());
    CHECK(response.result.str.empty());
    REQUIRE(response.error.has_value());
    CHECK(response.error.value().find("Invalid Request") != std::string::npos);
  }

  TEST_CASE("Server handle function with unknown method")
  {
    wsrpc::Server::SocketData socket_data;

    // Test handling a request for an unregistered method
    std::string_view unknown_request = R"({"id": "1", "method": "unknown_method", "params": {}})";
    auto result = wsrpc::Server::handle(socket_data, unknown_request);

    // Parse the response to check it
    wsrpc::response_t response{};
    auto pe = glz::read_json(response, result.first);
    REQUIRE_FALSE(pe);
    CHECK(response.id == "1");
    CHECK(response.result.str.empty());
    REQUIRE(response.error.has_value());
    CHECK(response.error.value().find("Method Unavaiable") != std::string::npos);
  }
}