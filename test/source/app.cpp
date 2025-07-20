#include <memory>

#include <doctest/doctest.h>

#include <wsrpc/app.hpp>

TEST_SUITE("app")
{
  TEST_CASE("App::handler_t")
  {
    auto handler1 = [](wsrpc::App::rawjson_t) -> void { return; };
    static_assert(not std::is_convertible_v<decltype(handler1), wsrpc::App::handler_t>);

    auto handler2 = []() -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_convertible_v<decltype(handler2), wsrpc::App::handler_t>);

    auto handler3 = [](wsrpc::App::rawjson_t) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler3), wsrpc::App::handler_t>);

    auto handler4 = [](const wsrpc::App::rawjson_t) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler4), wsrpc::App::handler_t>);

    auto handler5 = [](const wsrpc::App::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(std::is_convertible_v<decltype(handler5), wsrpc::App::handler_t>);

    auto handler6 = [](wsrpc::App::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_convertible_v<decltype(handler6), wsrpc::App::handler_t>);

    auto handler7 = [p = std::make_unique<int>()](const wsrpc::App::rawjson_t&) -> wsrpc::App::return_t { return {}; };
    static_assert(not std::is_copy_constructible_v<decltype(handler7)>);
    static_assert(std::is_convertible_v<decltype(handler7), wsrpc::App::handler_t>);
  }

  TEST_CASE("App construction")
  {
    wsrpc::App app;

    // Test that app is constructed with empty handlers
    CHECK(app.handlers.empty());
  }

  TEST_CASE("App registration and unregistration")
  {
    wsrpc::App app;

    // Test registering a handler
    app.regist("test_method", [](const wsrpc::App::rawjson_t&) {
      wsrpc::App::package_t package{R"({"result": "success"})", {}};
      return package;
    });

    CHECK(app.handlers.size() == 1);
    CHECK(app.handlers.contains("test_method"));

    // Test registering another handler for the same method (should replace)
    app.regist("test_method", [](const wsrpc::App::rawjson_t&) -> wsrpc::App::return_t {
      wsrpc::App::package_t package{R"({"result": "updated"})", {}};
      return package;
    });

    CHECK(app.handlers.size() == 1);

    // Test unregistering a handler
    app.unregist("test_method");
    CHECK(app.handlers.empty());
  }

  TEST_CASE("App handle method")
  {
    wsrpc::App app;

    // Test handling a non-existent method
    auto result = app.handle("nonexistent_method", "{}");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == "Method Unavaiable : \"nonexistent_method\"");

    // Test handling an existing method
    app.regist("test_method", [](const wsrpc::App::rawjson_t&) -> wsrpc::App::return_t {
      wsrpc::App::package_t package{R"({"result": "success"})", {}};
      return package;
    });

    auto result2 = app.handle("test_method", R"({"param": "value"})");
    REQUIRE(result2.has_value());
    CHECK(result2.value().first == R"({"result": "success"})");
    CHECK(result2.value().second.empty());

    // Test handling a method that throws an exception
    app.regist("throwing_method", [](const wsrpc::App::rawjson_t&) -> wsrpc::App::return_t {
      throw std::runtime_error("Test exception");
    });

    auto result3 = app.handle("throwing_method", "{}");
    REQUIRE_FALSE(result3.has_value());
    CHECK(result3.error() == "Internal Error : \"throwing_method\"");
  }
}