#include <exception>
#include <iostream>
#include <string>

#include <cxxopts.hpp>
#include <fmt/format.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <wsrpc/server.hpp>
#include <wsrpc/version.h>

static constexpr auto log_level = spdlog::level::level_enum(SPDLOG_ACTIVE_LEVEL);

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

auto cli(const int argc, const char* const argv[]) -> wsrpc::Options
{
  const auto log_level_str = fmt::to_string(spdlog::level::to_string_view(log_level));

  cxxopts::Options options(*argv, "A program to welcome the world!");
  options.add_options()                                                                            //
    ("help", "Print the help")                                                                     //
    ("version", "Print the version number")                                                        //
    ("l,level", "Set the log level", cxxopts::value<std::string>()->default_value(log_level_str))  //
    ("h,host", "Set the listening host", cxxopts::value<std::string>()->default_value("0.0.0.0"))  //
    ("p,port", "Set the listening port", cxxopts::value<int>()->default_value("8080"))             //
    ("t,timeout", "Set the timeout before exit", cxxopts::value<size_t>()->default_value("60"))    //
    ;

  if (argc == 1) {
    std::cout << options.help() << std::endl;
    std::exit(0);
  }

  try {
    const auto result = options.parse(argc, argv);

    if (result["help"].as<bool>()) {
      std::cout << options.help() << std::endl;
      std::exit(0);
    }

    if (result["version"].as<bool>()) {
      std::cout << "wsrpc, version " << WSRPC_VERSION << std::endl;
      std::exit(0);
    }

    spdlog::set_level(spdlog::level::from_str(result["level"].as<std::string>()));

    return {
      .host = result["host"].as<std::string>(),
      .port = result["port"].as<int>(),
      .timeout_s = result["timeout"].as<size_t>()};
  }
  catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    std::cerr << std::endl;
    std::cerr << options.help() << std::endl;
    std::exit(1);
  }
}

int main(const int argc, const char* const argv[])
{
  init_logger();

  std::set_terminate(terminate_handler);

  SPDLOG_DEBUG("debugging...");

  wsrpc::Options options = cli(argc, argv);

  wsrpc::Server server;
  server.serve(options);

  return 0;
}
