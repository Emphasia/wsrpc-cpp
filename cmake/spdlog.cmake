include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/gabime/spdlog

CPMAddPackage(
  NAME spdlog
  VERSION 1.15.3
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.zip
  SYSTEM YES
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
    "SPDLOG_INSTALL ON"
)
