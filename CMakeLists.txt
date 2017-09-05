cmake_minimum_required(VERSION 3.9)
project(prickly_zsh_theme)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp config.h)
add_compile_options(-Ofast)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake_include.h.in ${CMAKE_CURRENT_BINARY_DIR}/cmake_include.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(prickly-prompt ${SOURCE_FILES})

find_library(Libgit2 REQUIRED)
target_link_libraries(prickly-prompt git2)
