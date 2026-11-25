cmake_minimum_required(VERSION 3.16)
project(TypeMaster LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
)
FetchContent_MakeAvailable(httplib)

add_executable(typemaster src/main.cpp)
target_link_libraries(typemaster PRIVATE httplib::httplib)
if(WIN32)
  target_link_libraries(typemaster PRIVATE ws2_32)
endif()
