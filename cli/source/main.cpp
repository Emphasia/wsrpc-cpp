#include <exception>
#include <iostream>
#include <string>

#include <cxxopts.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <wsrpc/version.h>
#include <wsrpc/wsrpc.h>

void terminate_handler() noexcept
{
  auto ex_ptr = std::current_exception();
  if (ex_ptr) {
    try {
      std::rethrow_exception(ex_ptr);
    }
    catch (const std::exception& e) {
      SPDLOG_CRITICAL("Uncaught Exception: {}", e.what());
    }
    catch (...) {
      SPDLOG_CRITICAL("Uncaught Exception: Unknown type");
    }
  }
  else {
    SPDLOG_CRITICAL("Terminate called without active exception");
  }
  std::abort();
}

auto init_logger() -> void
{
  auto logger = spdlog::stdout_color_mt("main");
#ifdef NDEBUG
  logger->set_level(spdlog::level::info);
  logger->set_pattern("%Y-%m-%d %T.%e | %^%L%$ | %s:%# | %v");
#else
  logger->set_level(spdlog::level::debug);
  logger->set_pattern("%Y-%m-%d %T.%e | %^%-4!l%$ | %s:%# | %t | %v");
#endif
  spdlog::set_default_logger(logger);
}

auto cli(int argc, char** argv) -> int
{
  static const auto level = fmt::format(spdlog::level::to_string_view(spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL)));

  cxxopts::Options options(*argv, "A program to welcome the world!");
  options.add_options()                                                                    //
    ("h,help", "Print the help")                                                           //
    ("v,version", "Print the version number")                                              //
    ("l,level", "Set the log level", cxxopts::value<std::string>()->default_value(level))  //
    ;

  if (argc == 1) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  try {
    auto result = options.parse(argc, argv);

    if (result["help"].as<bool>()) {
      std::cout << options.help() << std::endl;
      return 0;
    }

    if (result["version"].as<bool>()) {
      std::cout << "wsrpc, version " << WSRPC_VERSION << std::endl;
      return 0;
    }

    spdlog::set_level(spdlog::level::from_str(result["level"].as<std::string>()));
  }
  catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    std::cerr << std::endl;
    std::cerr << options.help() << std::endl;
    return 1;
  }

  return 0;
}

int main(int argc, char** argv)
{
  init_logger();
  std::set_terminate(terminate_handler);

  cli(argc, argv);

  SPDLOG_DEBUG("debugging...");

  return 0;
}
