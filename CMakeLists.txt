# CMAKE_BINARY_DIR : Build directory which is in the same directory with main CMakeLists.txt file
# CMAKE_SOURCE_DIR : The directory where the top-level CMakeLists.txt file is located
# CMAKE_CURRENT_SOURCE_DIR : The directory of the current CMakeLists.txt file

cmake_minimum_required(VERSION 3.25)

# Project
project(Code-Romeo C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

# Options
option(ENABLE_ASAN  "Enable AddressSanitizer"                          OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer"                OFF)
option(ENABLE_WERROR "Treat warnings as errors (project sources only)" ON)

if(NOT CMAKE_C_COMPILER)
    find_program(CLANG_PATH clang)
    if(CLANG_PATH)
        set(CMAKE_C_COMPILER clang CACHE STRING "C compiler" FORCE)
        message(STATUS "Setting C compiler to: ${CMAKE_C_COMPILER}")
    else()
        message(FATAL_ERROR "clang compiler not found, but is preferred by this project.")
    endif()
endif()

message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")

# Compiler Arguments
set(WARNING_FLAGS
    -Wall -Wextra -Wshadow -Wpedantic -Wconversion
    -Wnull-dereference -Wunused-result
    -Wno-gnu-zero-variadic-macro-arguments
)

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
    ${CMAKE_SOURCE_DIR}/dependencies/glad/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE})

# Sanitizers
if(ENABLE_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE clang_rt.asan-x86_64 -fno-omit-frame-pointer)
    #target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE clang_rt.asan-x86_64)
    #target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    message(STATUS "Target '${PROJECT_NAME}' will be built with sanitizers: address")
endif()

if(ENABLE_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
    message(STATUS "Target '${PROJECT_NAME}' will be built with sanitizers: undefined")
endif()
    
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_FLAGS})

if (ENABLE_WERROR)
    target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
endif()

if(NOT ENABLE_ASAN AND NOT ENABLE_UBSAN)
    add_compile_options(-g -fstack-protector-strong)
endif()

# Dependencies
add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/cglm)

target_compile_options(glfw PRIVATE -w)
target_compile_options(cglm PRIVATE -w)

# Linking
if (WIN32)    
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 gdi32 cglm_headers glfw ucrt)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKit_LIBRARY IOKit REQUIRED)
    find_library(CoreVideo_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY} ${OpenGL_LIBRARY} ${IOKit_LIBRARY} ${CoreVideo_LIBRARY} cglm_headers glfw)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl cglm_headers glfw)
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
    message(WARNING "Project resource directory not found: ${PROJECT_RESOURCE_DIR}")
endif()