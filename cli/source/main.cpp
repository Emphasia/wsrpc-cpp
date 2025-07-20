#include <string>

#include <cxxopts.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <wsrpc/version.h>
#include <wsrpc/wsrpc.h>

auto init_logger() -> void
{
  auto logger = spdlog::stdout_color_mt("main");
#ifdef NDEBUG
  logger->set_level(spdlog::level::info);
  logger->set_pattern("[%Y-%m-%d %T.%e] [%^%L%$] [%s:%#] %v");
#else
  logger->set_level(spdlog::level::debug);
  logger->set_pattern("%Y-%m-%d %T.%e | %^%-5l%$ | %s:%# | %t | %v");
#endif
  spdlog::set_default_logger(logger);
}

// auto main(int argc, char** argv) -> int {
//   cxxopts::Options options(*argv, "A program to welcome the world!");

//   std::string language;
//   std::string name;

//   // clang-format off
//   options.add_options()
//     ("h,help", "Show help")
//     ("v,version", "Print the current version number")
//   ;
//   // clang-format on

//   auto result = options.parse(argc, argv);

//   if (result["help"].as<bool>()) {
//     std::cout << options.help() << std::endl;
//     return 0;
//   }

//   if (result["version"].as<bool>()) {
//     std::cout << "wsrpc, version " << wsrpc_VERSION << std::endl;
//     return 0;
//   }

//   return 0;
// }

int main()
{
  init_logger();

  SPDLOG_DEBUG("program debugging");

  SPDLOG_INFO("program completed successfully");
  return 0;
}
