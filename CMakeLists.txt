cmake_minimum_required(VERSION 3.28)
project(containers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # error out rather than silently downgrade
set(CMAKE_CXX_EXTENSIONS OFF)         # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # so your editor knows your flags

add_library(containers INTERFACE)
target_include_directories(containers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(containers INTERFACE -Wall -Wextra -Wpedantic -Werror)

add_executable(tests tests/main.cpp)
target_link_libraries(tests PRIVATE containers)