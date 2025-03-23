include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/jarro2783/cxxopts

CPMAddPackage(
  NAME cxxopts
  VERSION 3.2.0
  URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.2.0.zip
  OPTIONS "CXXOPTS_BUILD_EXAMPLES OFF" "CXXOPTS_BUILD_TESTS OFF" "CXXOPTS_ENABLE_INSTALL ON"
)
