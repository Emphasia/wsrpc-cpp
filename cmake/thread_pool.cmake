include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

# https://github.com/bshoshany/thread-pool

CPMAddPackage(
  NAME BS_thread_pool
  VERSION 5.0.0
  URL https://github.com/bshoshany/thread-pool/archive/refs/tags/v5.0.0.zip
  SYSTEM YES
)

add_library(BS_thread_pool INTERFACE)
target_include_directories(BS_thread_pool SYSTEM INTERFACE ${BS_thread_pool_SOURCE_DIR}/include)

install(TARGETS BS_thread_pool EXPORT wsrpcTargets)
