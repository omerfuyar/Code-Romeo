# CMAKE_BINARY_DIR          : Build directory (same directory as main CMakeLists.txt)
# CMAKE_SOURCE_DIR          : Directory where top-level CMakeLists.txt is located
# CMAKE_CURRENT_SOURCE_DIR  : Directory of the current CMakeLists.txt file

cmake_minimum_required(VERSION 3.22)

project(Code-Romeo C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/arc)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cglm)

target_compile_options(glfw PRIVATE -w)
target_compile_options(cglm PRIVATE -w)

file(GLOB_RECURSE PROJECT_SOURCE
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/src/*.c
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/src/glad.c PROPERTIES COMPILE_FLAGS "-w")

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCE})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cglm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/stb/include
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    if(CMAKE_C_COMPILER MATCHES "clang-cl")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /Zi /Od /W4 /permissive- /GS /WX /wd4324
    )
    elseif(CMAKE_C_COMPILER MATCHES "clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -g -O0 -Wall -Werror -Wextra -Wshadow -Wpedantic -Wconversion
            -Wnull-dereference -Wunused-result -Wno-strict-prototypes
            -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-value
            -fstack-protector-strong
    )
    elseif(CMAKE_C_COMPILER MATCHES "cl")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /Zi /Od /W4 /permissive- /GS /WX /wd4324
    )
    elseif(CMAKE_C_COMPILER MATCHES "gcc")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -g -O0 -Wall -Werror -Wextra -Wshadow -Wpedantic -Wconversion
            -Wnull-dereference -Wunused-result -Wno-strict-prototypes
            -Wno-unused-value -fstack-protector-strong
    )
    else()
        message(WARNING "Unknown/unsupported compiler. No specific compiler flags set.")
    endif()
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    if(CMAKE_C_COMPILER MATCHES "clang-cl")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /O2 /DNDEBUG
        )
    elseif(CMAKE_C_COMPILER MATCHES "clang")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -O3 -DNDEBUG
        )
    elseif(CMAKE_C_COMPILER MATCHES "cl")
        target_compile_options(${PROJECT_NAME} PRIVATE
            /O2 /DNDEBUG
        )
    elseif(CMAKE_C_COMPILER MATCHES "gcc")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -O3 -DNDEBUG
        )
    else()
        message(WARNING "Unknown/unsupported compiler. No specific compiler flags set.")
    endif()
else()
    message(WARNING "Unknown build type. No specific compiler flags set.")
endif()

get_target_property(ROMEO_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
message(STATUS "[Romeo] : Build info : ${CMAKE_BUILD_TYPE} | ${CMAKE_SYSTEM_NAME} | ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "[Romeo] : Compiler info : ${CMAKE_C_COMPILER_ID} | ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "[Romeo] : Compile options : ${ROMEO_COMPILE_OPTIONS}")

if((ENABLE_ASAN OR ENABLE_UBSAN) AND NOT CMAKE_C_COMPILER MATCHES "clang")
    message(FATAL_ERROR "Sanitizers are only compatible with LLVM")
endif()

if(ENABLE_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    message(STATUS "Address Sanitizer enabled")
endif()

if(ENABLE_UBSAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
    message(STATUS "Undefined Behavior Sanitizer enabled")
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC
    opengl32
    gdi32
    cglm
    glfw)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKit_LIBRARY IOKit REQUIRED)
    find_library(CoreVideo_LIBRARY CoreVideo REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        ${COCOA_LIBRARY}
        ${OpenGL_LIBRARY}
        ${IOKit_LIBRARY}
        ${CoreVideo_LIBRARY}
        cglm
        glfw
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PUBLIC dl cglm glfw)
endif()

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
