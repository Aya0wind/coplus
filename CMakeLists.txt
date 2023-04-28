cmake_minimum_required(VERSION 3.20)
set(CMAKE_TOOLCHAIN_FILE /home/ljj/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Werror=return-type)
project(coroutine)
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
include_directories(inc)

# find_package(fmt CONFIG REQUIRED)
file(
    GLOB_RECURSE
    SOURCE_FILES
    src/*.cpp
)
add_executable(coroutine ${SOURCE_FILES})

# target_link_libraries(coroutine PRIVATE fmt::fmt)
target_link_libraries(coroutine PRIVATE -lpthread)
