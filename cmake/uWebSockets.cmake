include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/uNetworking/uWebSockets

CPMAddPackage(
  NAME uWebSockets
  VERSION 20.74.0
  GIT_REPOSITORY https://github.com/uNetworking/uWebSockets
  GIT_TAG v20.74.0
  DOWNLOAD_ONLY YES
  SYSTEM YES
  GIT_SUBMODULES "uSockets"
)

enable_language(C)

file(GLOB uSockets_SRCS
  ${uWebSockets_SOURCE_DIR}/uSockets/src/*.c
  ${uWebSockets_SOURCE_DIR}/uSockets/src/eventing/*.c
)
add_library(uSockets STATIC ${uSockets_SRCS})
target_include_directories(uSockets SYSTEM PUBLIC
  ${uWebSockets_SOURCE_DIR}/uSockets/src
)
target_compile_definitions(uSockets PRIVATE
  LIBUS_NO_SSL
)

add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets SYSTEM INTERFACE ${uWebSockets_SOURCE_DIR}/src)
target_compile_definitions(uWebSockets INTERFACE UWS_NO_ZLIB)
target_link_libraries(uWebSockets INTERFACE uSockets)

install(TARGETS uSockets EXPORT wsrpcTargets)
install(TARGETS uWebSockets EXPORT wsrpcTargets)
