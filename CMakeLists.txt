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
# -fsanitize=address -fsanitize=undefined

# Build directory setup
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

# Define include directories
include_directories(${PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/include
)

# Source files setup
file(GLOB_RECURSE PROJECT_SOURCE
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# Define the executable target
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCE}
)

# Find and link libraries
if(UNIX)
    find_package(raylib CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
elseif(WIN32)
    find_package(raylib CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib winmm gdi32 opengl32 user32 kernel32 msvcrt shell32)
endif()
