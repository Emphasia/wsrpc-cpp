include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/stephenberry/glaze

CPMAddPackage(
  NAME glaze
  VERSION 5.5.0
  URL https://github.com/stephenberry/glaze/archive/refs/tags/v5.5.0.zip
  SYSTEM YES
  OPTIONS
    "glaze_INCLUDES_WITH_SYSTEM OFF"
    "glaze_BUILD_TESTS OFF"
    "glaze_ENABLE_AVX2 ON"
)
