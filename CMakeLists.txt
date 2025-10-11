# CMAKE_BINARY_DIR          : Build directory (same directory as main CMakeLists.txt)
# CMAKE_SOURCE_DIR          : Directory where top-level CMakeLists.txt is located
# CMAKE_CURRENT_SOURCE_DIR  : Directory of the current CMakeLists.txt file

cmake_minimum_required(VERSION 3.22)

project(Code-Romeo C)

# C Standard Configuration
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

# Compiler Information
message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")

# External Dependencies
add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/cglm)

target_compile_options(glfw PRIVATE -w)
target_compile_options(cglm PRIVATE -w)

# Project Source Files
file(GLOB_RECURSE PROJECT_SOURCE
    PRIVATE ${CMAKE_SOURCE_DIR}/src/*.c
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glad/src/*.c
)

# Library Target Configuration
add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCE})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler Options
target_compile_options(${PROJECT_NAME} PRIVATE
    -isystem${CMAKE_SOURCE_DIR}/dependencies/glad/include
    -isystem${CMAKE_SOURCE_DIR}/dependencies/glfw/include
    -isystem${CMAKE_SOURCE_DIR}/dependencies/cglm/include
    -isystem${CMAKE_SOURCE_DIR}/dependencies/stb/include
)

# Build-specific compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        -g -O0 -Wall -Wextra -Wshadow -Wpedantic -Wconversion
        -Wnull-dereference -Wunused-result -Wno-strict-prototypes
        -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-command-line-argument
        -Wno-unused-value -fstack-protector-strong -Werror
    >
    $<$<CONFIG:Release>:
        -O3 -DNDEBUG
    >
)

# Sanitizers Configuration
if((ENABLE_ASAN OR ENABLE_UBSAN) AND NOT CMAKE_C_COMPILER MATCHES "clang")
    message(FATAL_ERROR "Sanitizers are only compatible with LLVM")
endif()

# Address Sanitizer
if(ENABLE_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    message(STATUS "Address Sanitizer enabled")
endif()

# Undefined Behavior Sanitizer
if(ENABLE_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
    message(STATUS "Undefined Behavior Sanitizer enabled")
endif()

# Platform-Specific Linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    gdi32
    cglm
    glfw)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKit_LIBRARY IOKit REQUIRED)
    find_library(CoreVideo_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY}
        ${OpenGL_LIBRARY}
        ${IOKit_LIBRARY}
        ${CoreVideo_LIBRARY}
        cglm
        glfw
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl cglm glfw)
endif()

# Installation Rules
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
