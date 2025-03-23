cmake_minimum_required(VERSION 3.14...3.31)

# ---- Create project ----

project(
  Greeter
  VERSION 1.0
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

set(Greeter_STANDALONE "$<IF:$<STREQUAL:${CMAKE_CURRENT_SOURCE_DIR},${CMAKE_SOURCE_DIR}>,ON,OFF>")
option(Greeter_BUILD_DOC "Generate the doc target." ${Greeter_STANDALONE})
option(Greeter_BUILD_TEST "Generate the test target." ${Greeter_STANDALONE})
option(Greeter_BUILD_INSTALL "Generate the install target." ON)

# ---- Add source files ----

file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# ---- Add dependencies ----

include(cmake/CPM.cmake)
include(cmake/fmt.cmake)

# ---- Link dependencies ----

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

# ---- Handle includes ----

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create test ----

if(Greeter_BUILD_TEST)
  add_subdirectory(test)
endif()

# ---- Create package ----

if(Greeter_BUILD_INSTALL)
  CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")
  packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    VERSION_HEADER $<TOLOWER:${PROJECT_NAME}>/version.h
    COMPATIBILITY SameMajorVersion
    DEPENDENCIES "fmt 10.2.1"
  )
endif()
