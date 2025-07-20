#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include <wsrpc/version.h>
#include <wsrpc/wsrpc.h>

TEST_CASE("wsrpc version")
{
  static_assert(std::string_view(WSRPC_VERSION) == std::string_view("1.0.0"));
  CHECK(std::string(WSRPC_VERSION) == std::string("1.0.0"));
}
