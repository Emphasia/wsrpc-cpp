cmake_minimum_required(VERSION 3.14...3.31)

# ---- Create project ----

project(
  wsrpc
  VERSION 1.0.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a build directory and run CMake from there."
  )
endif()

# ---- Set options ----

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(wsrpc_STANDALONE ON)
else()
  set(wsrpc_STANDALONE OFF)
endif()
message(STATUS "wsrpc_STANDALONE: ${wsrpc_STANDALONE}")
option(wsrpc_BUILD_FORMAT "Generate the format target." ${wsrpc_STANDALONE})
option(wsrpc_BUILD_TEST "Generate the test target." ${wsrpc_STANDALONE})
option(wsrpc_BUILD_DOC "Generate the doc target." ${wsrpc_STANDALONE})
option(wsrpc_BUILD_CLI "Generate the cli target." ${wsrpc_STANDALONE})
option(wsrpc_BUILD_INSTALL "Generate the install target." ON)

# ---- Add source files ----

file(GLOB_RECURSE files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

add_library(${PROJECT_NAME} STATIC ${files})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
target_include_directories(${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

# ---- Add dependencies ----

include(cmake/CPM.cmake)
include(cmake/fmt.cmake)
include(cmake/spdlog.cmake)
include(cmake/glaze.cmake)
include(cmake/uWebSockets.cmake)
include(cmake/thread_pool.cmake)

# ---- Link dependencies ----

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    fmt::fmt
    spdlog::spdlog
    glaze::glaze
  PRIVATE
    uWebSockets
    BS_thread_pool
)

# ---- Set dependencies ----

target_compile_definitions(${PROJECT_NAME} PUBLIC "SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>")

# ---- Fix paths ----

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.)
endif()

# ---- Create format ----

if(wsrpc_BUILD_FORMAT)
  message(STATUS "enabling wsrpc_BUILD_FORMAT")
  CPMAddPackage(
    URI "gh:TheLartians/Format.cmake@1.8.3"
    OPTIONS "FORMAT_SKIP_CMAKE ON" "FORMAT_SKIP_CLANG OFF")
  add_dependencies(${PROJECT_NAME} check-format)
endif()

# ---- Create test ----

if(wsrpc_BUILD_TEST)
  message(STATUS "enabling wsrpc_BUILD_TEST")
  add_subdirectory(test)
endif()

# ---- Create doc ----

if(wsrpc_BUILD_DOC)
  # message(STATUS "enabling wsrpc_BUILD_DOC")
  # add_subdirectory(doc)
endif()

# ---- Create cli ----

if(wsrpc_BUILD_CLI)
  message(STATUS "enabling wsrpc_BUILD_CLI")
  add_subdirectory(cli)
endif()

# ---- Create package ----

if(wsrpc_BUILD_INSTALL)
  message(STATUS "enabling wsrpc_BUILD_INSTALL")
  CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")
  packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    DISABLE_VERSION_SUFFIX ON
    NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include
    VERSION_HEADER ${PROJECT_NAME}/version.h
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES "fmt;spdlog;glaze"
  )
  install(TARGETS uSockets EXPORT wsrpcTargets)
  install(TARGETS uWebSockets EXPORT wsrpcTargets)
  install(TARGETS BS_thread_pool EXPORT wsrpcTargets)
endif()
