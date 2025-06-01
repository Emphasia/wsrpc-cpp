cmake_minimum_required(VERSION 3.14...3.31)

project(wsrpc_cli LANGUAGES CXX)

include(../cmake/tools.cmake)

include(../cmake/cxxopts.cmake)

CPMAddPackage(NAME wsrpc SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "wsrpc")

target_link_libraries(${PROJECT_NAME} wsrpc::wsrpc cxxopts)
