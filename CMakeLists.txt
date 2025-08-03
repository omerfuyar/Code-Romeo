# CMAKE_BINARY_DIR : Build directory which is in the same directory with main CMakeLists.txt file
# CMAKE_SOURCE_DIR : The directory where the top-level CMakeLists.txt file is located
# CMAKE_CURRENT_SOURCE_DIR : The directory of the current CMakeLists.txt file

cmake_minimum_required(VERSION 3.25)

# Cmake project setup
project(Code-Romeo C)

# Set compiler flags and standard.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall -Wextra -Wshadow -Wpedantic -Wconversion -Werror -Wnull-dereference -Wunused-result -Wno-gnu-zero-variadic-macro-arguments -Wno-int-conversion -fstack-protector-strong -fno-common")
# -fsanitize=address -fsanitize=undefined : breaks the build

# Build directory setup
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

# Include files setup
include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_SOURCE_DIR}/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glad/include
    SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/dependencies/glfw/include
)

# Source files setup
file(GLOB_RECURSE PROJECT_SOURCE
    ${CMAKE_SOURCE_DIR}/dependencies/glad/src/*.c 
    # glad requires compilation but glfw is handled by CMake
    ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCE}
)

# Additional dependencies setup
add_subdirectory(dependencies/glfw)
set_target_properties(glfw PROPERTIES COMPILE_FLAGS "-w")

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32 gdi32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKit_LIBRARY IOKit REQUIRED)
    find_library(CoreVideo_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COCOA_LIBRARY} ${OpenGL_LIBRARY} ${IOKit_LIBRARY} ${CoreVideo_LIBRARY})
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl)
endif()
