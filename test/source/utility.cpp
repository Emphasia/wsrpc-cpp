#include <string>

#include <doctest/doctest.h>

#include <wsrpc/utility.hpp>

TEST_SUITE("utility")
{
  TEST_CASE("sv function")
  {
    // Test with empty vector
    std::vector<std::byte> empty_data;
    auto empty_sv = wsrpc::sv(empty_data);
    CHECK(empty_sv.empty());
    CHECK(empty_sv.size() == 0);

    // Test with data
    std::vector<std::byte> data{std::byte('H'), std::byte('e'), std::byte('l'), std::byte('l'), std::byte('o')};
    auto data_sv = wsrpc::sv(data);
    CHECK_FALSE(data_sv.empty());
    CHECK(data_sv.size() == 5);
    CHECK(std::string(data_sv) == "Hello");
  }
}