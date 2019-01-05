cmake_minimum_required(VERSION 3.0)

project(yet C CXX)

set(CMAKE_CXX_FLAGS
  "-g \
  -pthread \
  -std=c++0x \
  -pipe \
  -fPIC \
  -Wall \
  -Wextra \
  -Wunused-parameter \
  -Woverloaded-virtual \
  -Wpointer-arith \
  -Wshadow \
  -Wwrite-strings \
  -Wcast-align"
  )
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

if(APPLE)
  message(STATUS "os: APPLE")
  set(COMMON_LIBS pthread)
elseif(UNIX)
  message(STATUS "os: UNIX")
  set(COMMON_LIBS pthread rt)
  set(CMAKE_EXE_LINKER_FLAGS "-pthread -static-libstdc++")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(yet)
include_directories(third_party/asio-asio-1-12-2/asio/include)
include_directories(third_party/spdlog-1.2.1/include)
add_subdirectory(yet)
