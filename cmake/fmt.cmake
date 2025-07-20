include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/fmtlib/fmt

CPMAddPackage(
  NAME fmt
  VERSION 11.1.4
  URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.zip
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "FMT_INSTALL ON"
)
