#include <doctest/doctest.h>
#include <wsrpc/wsrpc.h>
#include <wsrpc/version.h>

#include <string>

TEST_CASE("wsrpc version") {
  static_assert(std::string_view(wsrpc_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(wsrpc_VERSION) == std::string("1.0.0"));
}
