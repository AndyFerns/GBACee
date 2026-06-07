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

target_compile_definitions(gbcee PRIVATE SDL_MAIN_HANDLED)
target_link_libraries(gbcee PRIVATE SDL2::SDL2)

# ----------------------------------------
# Build Test ROMs
# ----------------------------------------

# Define the list of assembly files you want to compile
set(ROM_SOURCES 
    "tests/roms/test_roms/ppu/ppu_test.asm"
    "tests/roms/test_roms/comprehensive_test.asm"
)

foreach(ASM_FILE IN LISTS ROM_SOURCES)
    get_filename_component(ROM_DIR ${ASM_FILE} DIRECTORY)
    get_filename_component(ROM_NAME ${ASM_FILE} NAME_WE)
    
    set(OBJ_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${ROM_DIR}/${ROM_NAME}.o")
    set(GB_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${ROM_DIR}/${ROM_NAME}.gb")

    # Tell CMake how to generate the .gb file from the .asm file
    add_custom_command(
        OUTPUT ${GB_FILE}
        COMMAND rgbasm -o ${OBJ_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE}
        COMMAND rgblink -o ${GB_FILE} ${OBJ_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE}
        COMMENT "Building Game Boy ROM: ${GB_FILE}"
    )
    
    # Keep track of all generated ROMs
    list(APPEND ROM_TARGETS ${GB_FILE})
endforeach()

# Create a 'roms' target that builds all of them automatically
add_custom_target(roms ALL DEPENDS ${ROM_TARGETS})