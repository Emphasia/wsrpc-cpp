#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>

#include <wsrpc/utility.hpp>

int main(int argc, char** argv)
{
  wsrpc::init_logger();
  wsrpc::init_exception_handler();

  doctest::Context context(argc, argv);
  return context.run();
}