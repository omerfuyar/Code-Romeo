#pragma once

#pragma region Platform Detection

#if defined(_WIN32)
#define RJ_PLATFORM_WINDOWS 1
#define RJ_PLATFORM_LINUX 0
#define RJ_PLATFORM_MACOS 0
#define RJ_PLATFORM "WINDOWS"

#elif defined(__linux__)
#define RJ_PLATFORM_LINUX 1
#define RJ_PLATFORM_WINDOWS 0
#define RJ_PLATFORM_MACOS 0
#define RJ_PLATFORM "LINUX"

#elif defined(__APPLE__) && defined(__MACH__)
#define RJ_PLATFORM_MACOS 1
#define RJ_PLATFORM_WINDOWS 0
#define RJ_PLATFORM_LINUX 0
#define RJ_PLATFORM "MACOS"

#else
#pragma error("Unsupported platform.")
#endif

#if RJ_PLATFORM_LINUX || RJ_PLATFORM_MACOS
#define RJ_PLATFORM_UNIX 1
#else
#define RJ_PLATFORM_UNIX 0
#endif

#pragma endregion Platform Detection

#pragma region Compiler Detection

#if defined(__clang__)
#define RJ_COMPILER_CLANG 1
#define RJ_COMPILER_GCC 0
#define RJ_COMPILER_MSVC 0
#define RJ_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#define RJ_COMPILER_NAME "CLANG"

#elif defined(__GNUC__)
#define RJ_COMPILER_CLANG 0
#define RJ_COMPILER_GCC 1
#define RJ_COMPILER_MSVC 0
#define RJ_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define RJ_COMPILER_NAME "GCC"

#elif defined(_MSC_VER)
#define RJ_COMPILER_CLANG 0
#define RJ_COMPILER_GCC 0
#define RJ_COMPILER_MSVC 1
#define RJ_COMPILER_VERSION _MSC_VER
#define RJ_COMPILER_NAME "MSVC"

#else
#pragma error("Unsupported compiler.")
#endif

#pragma endregion Compiler Detection

#pragma region Architecture Detection

#if defined(_M_X64) || defined(__x86_64__)
#define RJ_ARCHITECTURE_X64 1
#define RJ_ARCHITECTURE_X86 0
#define RJ_ARCHITECTURE_ARM 0
#define RJ_ARCHITECTURE "X64"

#elif defined(_M_IX86) || defined(__i386__)
#define RJ_ARCHITECTURE_X64 0
#define RJ_ARCHITECTURE_X86 1
#define RJ_ARCHITECTURE_ARM 0
#define RJ_ARCHITECTURE "X86"

#elif defined(_M_ARM) || defined(__arm__) || defined(__aarch64__)
#define RJ_ARCHITECTURE_X64 0
#define RJ_ARCHITECTURE_X86 0
#define RJ_ARCHITECTURE_ARM 1
#define RJ_ARCHITECTURE "ARM"

#else
#pragma error("Unsupported architecture.")
#endif

#define _POSIX_C_SOURCE 200809L
#define _CRT_SECURE_NO_WARNINGS

#pragma endregion Architecture Detection

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#if RJ_PLATFORM_WINDOWS
#define RJ_PATH_DELIMETER_CHAR '\\'
#define RJ_PATH_DELIMETER_STR "\\"
#elif RJ_PLATFORM_UNIX
#define RJ_PATH_DELIMETER_CHAR '/'
#define RJ_PATH_DELIMETER_STR "/"
#endif

#define FileOpen(filePtr, fileName, mode) ((filePtr = fopen(fileName, mode)) != NULL)
#define MemoryCopy(destination, size, source) memcpy(destination, source, size)
#define MemorySet(destination, size, value) memset(destination, value, size)
#define MemoryMove(destination, size, source) memmove(destination, source, size)

#define RJ_TEMP_BUFFER_SIZE (size_t)128

#pragma region Typedefs

typedef void (*VoidFunIntCharPtrPtr)(int, char **);

typedef void (*VoidFunFloat)(float);

typedef void (*VoidFunIntCharptr)(int, char *);

#pragma endregion Typedefs

#pragma region Functions and Macros

/// @brief Logs a debug message to the debug log file. Use wrapper macros for ease of use.
/// @param terminate Whether to terminate the application after logging
/// @param header The header of the log message, like "INFO", "WARNING", "ERROR", etc.
/// @param file The source file name where the log is called from
/// @param line The line number where the log is called from
/// @param function The function name where the log is called from
/// @param format The format string for the log message, similar to printf.
/// @param ... The arguments for the format string.
/// @note The log message is written to a file named 'RJ_DEBUG_FILE_NAME'. Directory and name can be changed by modifying the macro.
void Global_DebugLog(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Gets the executable file directory.
/// @return The null terminated C string : "path/to/exe/"
const char *Global_GetExecutablePath();

/// @brief Runs the main application loop with setup and loop callbacks
/// @param argc Command line argument count
/// @param argv Command line argument values
void Global_Run(int argc, char **argv);

/// @brief Terminates and closes necessary utilities and exits the program.
/// @param exitCode The code to pass to exit() function.
/// @param message The message to show to the console.
void Global_Terminate(int exitCode, char *message);

#pragma region Callbacks

/// @brief Sets the setup callback function that gets called once at application start
/// @param setupCallback Function to call during application setup
void Global_SetSetupCallback(VoidFunIntCharPtrPtr setupCallback);

/// @brief Sets the main loop callback function that gets called every frame
/// @param loopCallback Function to call every frame, receives deltatime in seconds as parameter
void Global_SetLoopCallback(VoidFunFloat loopCallback);

/// @brief Sets the callback function for the global application terminate function. After setting, terminate function calls the callback function before its own instructions.
/// @param terminateCallback Function to call when terminate is called. Should not exit the program. Receives exit code and exit message as parameters.
void Global_SetTerminateCallback(VoidFunIntCharptr terminateCallback);

#pragma region Callbacks

#pragma endregion Functions and Macros

#pragma region Debug Log

#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define RJ_BUILD_DEBUG true
#else
#define RJ_BUILD_DEBUG false
#endif

#define RJ_DEBUG_INFO RJ_BUILD_DEBUG
#define RJ_DEBUG_WARNING RJ_BUILD_DEBUG
#define RJ_DEBUG_ERROR RJ_BUILD_DEBUG
#define RJ_DEBUG_ASSERT RJ_BUILD_DEBUG

#define RJ_DEBUG_TERMINATE_ON_ERROR true
#define RJ_DEBUG_TERMINATE_ON_ASSERT true

#define RJ_DEBUG_SAFE_LOGGING false
#define RJ_DEBUG_FLUSH_AFTER_LOG RJ_DEBUG_SAFE_LOGGING

#define RJ_DEBUG_TIME_FORMAT "%H:%M:%S"
#define RJ_DEBUG_FILE_NAME "debug.log"

#define DebugLog(terminate, header, format, ...)                                                 \
    do                                                                                           \
    {                                                                                            \
        Global_DebugLog(terminate, header, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)

#if RJ_DEBUG_INFO == false
#define DebugInfo(format, ...)
#else
#define DebugInfo(format, ...)                          \
    do                                                  \
    {                                                   \
        DebugLog(false, "INFO", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJ_DEBUG_WARNING == false
#define DebugWarning(format, ...)
#else
#define DebugWarning(format, ...)                          \
    do                                                     \
    {                                                      \
        DebugLog(false, "WARNING", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJ_DEBUG_ERROR == false
#define DebugError(format, ...)
#else
#define DebugError(format, ...)                                                \
    do                                                                         \
    {                                                                          \
        DebugLog(RJ_DEBUG_TERMINATE_ON_ERROR, "ERROR", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJ_DEBUG_ASSERT == false
#define DebugAssert(condition, format, ...) (void)(condition)

#define DebugAssertNullPointerCheck(ptr)

#define DebugAssertFileOpenCheck(filePtr, fileName, mode) FileOpen(filePtr, fileName, mode)

#else
#define DebugAssert(condition, format, ...)                                                     \
    do                                                                                          \
    {                                                                                           \
        if (!(condition))                                                                       \
        {                                                                                       \
            DebugLog(RJ_DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", format, ##__VA_ARGS__); \
        }                                                                                       \
    } while (false)

#define DebugAssertNullPointerCheck(ptr) \
    DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)

#define DebugAssertFileOpenCheck(filePtr, fileName, mode) \
    DebugAssert(FileOpen(filePtr, fileName, mode), "File open failed for %s", fileName)

#endif

#pragma endregion Debug
