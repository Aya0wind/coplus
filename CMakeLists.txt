cmake_minimum_required(VERSION 3.21)
project(coroutine)
set(CMAKE_TOOLCHAIN_FILE /Users/li/.clion-vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Werror=return-type)
file(
    GLOB_RECURSE
    SOURCE_FILES
    src/*.cpp
    src/*.hpp
)
find_package(fmt REQUIRED CONFIG)
add_executable(coroutine ${SOURCE_FILES})
target_include_directories(coroutine PUBLIC inc)
target_link_libraries(coroutine fmt::fmt)
