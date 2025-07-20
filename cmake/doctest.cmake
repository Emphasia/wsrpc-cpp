include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/doctest/doctest

CPMAddPackage(
  NAME doctest
  VERSION 2.4.12
  URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.12.zip
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS
    "DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF"
    "DOCTEST_WITH_TESTS OFF"
    "DOCTEST_NO_INSTALL OFF"
)
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
