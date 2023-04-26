cmake_minimum_required(VERSION 3.20)
project(coroutine)
set(CMAKE_TOOLCHAIN_FILE /home/ljj/vcpkg/scripts/buildsystems/vcpkg.cmake)
find_package(fmt CONFIG REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Werror=return-type)
include_directories(inc)
file(
    GLOB_RECURSE
    SOURCE_FILES
    src/*.cpp
)

add_executable(coroutine ${SOURCE_FILES})
target_link_libraries(coroutine PRIVATE fmt::fmt)
target_link_libraries(coroutine PRIVATE -lpthread)
