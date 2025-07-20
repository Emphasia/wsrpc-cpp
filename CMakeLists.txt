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

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# ---- Add dependencies ----

include(cmake/CPM.cmake)
include(cmake/fmt.cmake)
include(cmake/spdlog.cmake)

# ---- Link dependencies ----

target_link_libraries(${PROJECT_NAME} PUBLIC
  fmt::fmt
  spdlog::spdlog
)

# ---- Set dependencies ----

target_compile_definitions(${PROJECT_NAME} PUBLIC "SPDLOG_ACTIVE_LEVEL=$<IF:$<CONFIG:Debug>,SPDLOG_LEVEL_TRACE,SPDLOG_LEVEL_INFO>")

# ---- Handle includes ----

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create format ----

if(wsrpc_BUILD_FORMAT)
  message(STATUS "enabling wsrpc_BUILD_FORMAT")
  CPMAddPackage("gh:TheLartians/Format.cmake@1.8.3")
endif()

# ---- Create package ----

if(wsrpc_BUILD_INSTALL)
  message(STATUS "enabling wsrpc_BUILD_INSTALL")
  CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")
  packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    VERSION_HEADER ${PROJECT_NAME}/version.h
    COMPATIBILITY SameMajorVersion
  )
endif()

# ---- Create cli ----

if(wsrpc_BUILD_CLI)
  message(STATUS "enabling wsrpc_BUILD_CLI")
  add_subdirectory(cli)
endif()

# ---- Create test ----

if(wsrpc_BUILD_TEST)
  message(STATUS "enabling wsrpc_BUILD_TEST")
  add_subdirectory(test)
endif()
