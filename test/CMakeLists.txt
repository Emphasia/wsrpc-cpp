cmake_minimum_required(VERSION 3.14...3.31)

project(wsrpc_tests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_COVERAGE "Enable test coverage" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/doctest.cmake)

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})

target_compile_definitions(${PROJECT_NAME} PRIVATE ROOTPATH="${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
target_link_libraries(${PROJECT_NAME} wsrpc::wsrpc doctest::doctest)

# enable compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wpedantic -Wextra -Werror -Wno-error=unused-parameter)
elseif(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC /W4 /WX /wd4100)
  target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
endif()

if(ENABLE_ASAN)
  target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
  target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address)
endif()

# ---- Add tests ----

enable_testing()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_COVERAGE)
  find_program(GCOVR_PATH gcovr REQUIRED)
  target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g -fprofile-arcs -ftest-coverage --coverage)
  target_link_options(${PROJECT_NAME} PUBLIC -fprofile-arcs -ftest-coverage --coverage)
  add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND}
    COMMAND ${GCOVR_PATH}
            --root ${CMAKE_SOURCE_DIR} --object-directory ${PROJECT_BINARY_DIR}
            --print-summary --html-details --output ${CMAKE_BINARY_DIR}/coverage.html
            --exclude-unreachable-branches --exclude-throw-branches
            --exclude '.*/usr/.*'
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Generating code coverage report"
  )
endif()
