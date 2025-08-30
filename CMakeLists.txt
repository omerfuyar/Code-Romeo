# CMAKE_BINARY_DIR : Build directory which is in the same directory with main CMakeLists.txt file
# CMAKE_SOURCE_DIR : The directory where the top-level CMakeLists.txt file is located
# CMAKE_CURRENT_SOURCE_DIR : The directory of the current CMakeLists.txt file

# CMake configurations
cmake_minimum_required(VERSION 3.22)

# Project
project(Code-Romeo C)

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behaviour Sanitizer" OFF)

if((ENABLE_ASAN OR ENABLE_UBSAN) AND NOT CMAKE_C_COMPILER MATCHES "clang")
    message(FATAL_ERROR "Sanitizers are only compatible with LLVM")
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")

# Includes
include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glad/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glfw/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/cglm/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/stb/include
)

# Sources
file(GLOB_RECURSE PROJECT_SOURCE
    PRIVATE ${CMAKE_SOURCE_DIR}/src/*.c
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glad/src/*.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE})

# Compiler Arguments
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wshadow -Wpedantic -Wconversion -Wnull-dereference -Wunused-result -Wno-strict-prototypes -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-command-line-argument -fstack-protector-strong -Werror")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

if(ENABLE_ASAN)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    message(STATUS "Address Sanitizer enabled")
endif()

if(ENABLE_UBSAN)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=undefined")
    message(STATUS "Undefined Behavior Sanitizer enabled")
endif()

# Dependencies
add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/cglm)

target_compile_options(glfw PRIVATE -w)
target_compile_options(cglm PRIVATE -w)

# Linking
if (WIN32)    
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 gdi32 cglm glfw)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKit_LIBRARY IOKit REQUIRED)
    find_library(CoreVideo_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY} ${OpenGL_LIBRARY} ${IOKit_LIBRARY} ${CoreVideo_LIBRARY} cglm glfw)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl cglm glfw)
endif()

# Resource copy
set(PROJECT_RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(PROJECT_BUILD_RESOURCE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources")
if (EXISTS ${PROJECT_RESOURCE_DIR})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_RESOURCE_DIR}"
        "${PROJECT_BUILD_RESOURCE_DIR}"
        COMMENT "Copying resources to build directory"
    )
    message(STATUS "Project resource directory copied to: ${PROJECT_BUILD_RESOURCE_DIR}")
else()
    message(FATAL_ERROR "Project resource directory not found: ${PROJECT_RESOURCE_DIR}")
endif()