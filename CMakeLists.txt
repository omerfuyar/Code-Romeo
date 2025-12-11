cmake_minimum_required(VERSION 3.15)
project(CodeRomeo C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Check for submodules
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/dependencies/shuild/shuild.h")
    message(FATAL_ERROR 
        "Submodules not initialized!\n"
        "Please run: git submodule update --init --recursive")
endif()

# Platform-specific definitions
if(WIN32)
    add_compile_definitions(_GLFW_WIN32)
endif()

# Add CGLM dependency
add_subdirectory(dependencies/cglm)

# Add GLFW dependency
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)

# GLAD source
set(GLAD_SOURCES
    dependencies/glad/src/glad.c
)

# Code-Romeo sources
file(GLOB RJGLOBAL_SOURCES
    src/RJGlobal.c
)

file(GLOB UTILITIES_SOURCES
    src/utilities/*.c
)

file(GLOB TOOLS_SOURCES
    src/tools/*.c
)

file(GLOB SYSTEMS_SOURCES
    src/systems/*.c
)

# Main library
add_library(CodeRomeo STATIC
    ${RJGLOBAL_SOURCES}
    ${UTILITIES_SOURCES}
    ${TOOLS_SOURCES}
    ${SYSTEMS_SOURCES}
    ${GLAD_SOURCES}
)

# Include directories
target_include_directories(CodeRomeo PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/dependencies
    ${CMAKE_SOURCE_DIR}/dependencies/cglm/include
    ${CMAKE_SOURCE_DIR}/dependencies/glad/include
    ${CMAKE_SOURCE_DIR}/dependencies/glfw/include
    ${CMAKE_SOURCE_DIR}/dependencies/miniaudio
    ${CMAKE_SOURCE_DIR}/dependencies/stb
)

# Link libraries
target_link_libraries(CodeRomeo PUBLIC
    cglm
    glfw
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(CodeRomeo PUBLIC
        m  # Math library
        dl # Dynamic loading
    )
elseif(APPLE)
    target_link_libraries(CodeRomeo PUBLIC
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(CodeRomeo PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    )
    
    # Additional warnings for debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(CodeRomeo PRIVATE
            -Wshadow
            -Wconversion
            -Wnull-dereference
            -Wunused-result
        )
    endif()
elseif(MSVC)
    target_compile_options(CodeRomeo PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Zi /Od>
        $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )
endif()

# Sanitizers
if(ENABLE_ASAN)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
        target_compile_options(CodeRomeo PRIVATE -fsanitize=address)
        target_link_options(CodeRomeo PRIVATE -fsanitize=address)
    elseif(MSVC)
        target_compile_options(CodeRomeo PRIVATE /fsanitize=address)
    endif()
endif()

if(ENABLE_UBSAN)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
        target_compile_options(CodeRomeo PRIVATE -fsanitize=undefined)
        target_link_options(CodeRomeo PRIVATE -fsanitize=undefined)
    endif()
endif()

# CGLM static definition
target_compile_definitions(CodeRomeo PUBLIC CGLM_STATIC)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS CodeRomeo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/CodeRomeo
    FILES_MATCHING PATTERN "*.h"
)

# Print configuration summary
message(STATUS "")
message(STATUS "Code-Romeo Configuration Summary")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "AddressSanitizer: ${ENABLE_ASAN}")
message(STATUS "UndefinedBehaviorSanitizer: ${ENABLE_UBSAN}")
message(STATUS "")
