cmake_minimum_required(VERSION 3.14...3.31)

project(wsrpc_cli LANGUAGES CXX)

include(../cmake/tools.cmake)

include(../cmake/cxxopts.cmake)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "wsrpc-server")

target_link_libraries(${PROJECT_NAME} PRIVATE wsrpc::wsrpc cxxopts)
