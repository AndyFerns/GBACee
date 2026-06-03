cmake_minimum_required(VERSION 3.16)
project(GBCee C)

set(CMAKE_C_STANDARD 11)

# ----------------------------------------
# Collect all source files
# ----------------------------------------
file(GLOB_RECURSE SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.c
)

# ----------------------------------------
# Create executable
# ----------------------------------------
add_executable(gbcee ${SOURCES})

# ----------------------------------------
# Include directories
# ----------------------------------------

# 1. Root includes folder
target_include_directories(gbcee PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

# 2. ALL subdirectories inside includes/ (for flat includes like "cpu.h")
file(GLOB_RECURSE INCLUDE_SUBDIRS LIST_DIRECTORIES true
    ${PROJECT_SOURCE_DIR}/includes/*
)

foreach(dir ${INCLUDE_SUBDIRS})
    if(IS_DIRECTORY ${dir})
        target_include_directories(gbcee PRIVATE ${dir})
    endif()
endforeach()

# ----------------------------------------
# SDL2 (MinGW)
# ----------------------------------------

find_package(SDL2 REQUIRED)

target_link_libraries(gbcee PRIVATE SDL2::SDL2)
