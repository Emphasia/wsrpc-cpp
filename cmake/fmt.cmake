include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/fmtlib/fmt.git

CPMAddPackage(
  NAME fmt
  VERSION 11.1.4
  URL https://github.com/fmtlib/fmt/archive/refs/tags/11.1.4.zip
  OPTIONS "FMT_INSTALL ON"
)
