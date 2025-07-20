#include <doctest/doctest.h>

#include <wsrpc/message.hpp>

TEST_SUITE("message")
{
  TEST_CASE("request_t struct")
  {
    wsrpc::request_t req;

    // Test default construction
    CHECK(req.id.empty());
    CHECK(req.method.empty());
    CHECK(req.params.str.empty());

    // Test construction with values
    wsrpc::request_t req2{"1", "test_method", R"({"param": "value"})"};
    CHECK(req2.id == "1");
    CHECK(req2.method == "test_method");
    CHECK(req2.params.str == R"({"param": "value"})");
  }

  TEST_CASE("response_t struct")
  {
    wsrpc::response_t res;

    // Test default construction
    CHECK(res.id.empty());
    CHECK(res.result.str.empty());
    CHECK_FALSE(res.error.has_value());

    // Test construction with values
    wsrpc::response_t res2{"1", R"({"result": "success"})", std::nullopt};
    CHECK(res2.id == "1");
    CHECK(res2.result.str == R"({"result": "success"})");
    CHECK_FALSE(res2.error.has_value());

    // Test with error value
    wsrpc::response_t res3{"2", "{}", "Error message"};
    CHECK(res3.id == "2");
    CHECK(res3.result.str == "{}");
    REQUIRE(res3.error.has_value());
    CHECK(res3.error.value() == "Error message");
  }

  TEST_CASE("error functions")
  {
    // Test error formatting
    CHECK(wsrpc::error::format(wsrpc::error::INVALID_REQUEST, "MI1") == "Invalid Request : MI1");
    CHECK(wsrpc::error::format(wsrpc::error::INVALID_RESPONSE, "MI2") == "Invalid Response : MI2");
    CHECK(wsrpc::error::format(wsrpc::error::METHOD_UNAVAIABLE, "MI3") == "Method Unavaiable : MI3");
    CHECK(wsrpc::error::format(wsrpc::error::INVALID_PARAMS, "MI4") == "Invalid Params : MI4");
    CHECK(wsrpc::error::format(wsrpc::error::INTERNAL_ERROR, "MI5") == "Internal Error : MI5");
  }
}