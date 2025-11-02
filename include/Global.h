#pragma once

#pragma region Platform Detection

#if defined(_WIN32)
#define RJGLOBAL_PLATFORM_WINDOWS 1
#define RJGLOBAL_PLATFORM_LINUX 0
#define RJGLOBAL_PLATFORM_MACOS 0
#define RJGLOBAL_PLATFORM "WINDOWS"

#elif defined(__linux__)
#define RJGLOBAL_PLATFORM_LINUX 1
#define RJGLOBAL_PLATFORM_WINDOWS 0
#define RJGLOBAL_PLATFORM_MACOS 0
#define RJGLOBAL_PLATFORM "LINUX"

#elif defined(__APPLE__) && defined(__MACH__)
#define RJGLOBAL_PLATFORM_MACOS 1
#define RJGLOBAL_PLATFORM_WINDOWS 0
#define RJGLOBAL_PLATFORM_LINUX 0
#define RJGLOBAL_PLATFORM "MACOS"

#else
#pragma error("Unsupported platform.")
#endif

#if RJGLOBAL_PLATFORM_LINUX || RJGLOBAL_PLATFORM_MACOS
#define RJGLOBAL_PLATFORM_UNIX 1
#else
#define RJGLOBAL_PLATFORM_UNIX 0
#endif

#pragma endregion Platform Detection

#pragma region Compiler Detection

#if defined(__clang__)
#define RJGLOBAL_COMPILER_CLANG 1
#define RJGLOBAL_COMPILER_GCC 0
#define RJGLOBAL_COMPILER_MSVC 0
#define RJGLOBAL_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#define RJGLOBAL_COMPILER_NAME "CLANG"

#elif defined(__GNUC__)
#define RJGLOBAL_COMPILER_CLANG 0
#define RJGLOBAL_COMPILER_GCC 1
#define RJGLOBAL_COMPILER_MSVC 0
#define RJGLOBAL_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define RJGLOBAL_COMPILER_NAME "GCC"

#elif defined(_MSC_VER)
#define RJGLOBAL_COMPILER_CLANG 0
#define RJGLOBAL_COMPILER_GCC 0
#define RJGLOBAL_COMPILER_MSVC 1
#define RJGLOBAL_COMPILER_VERSION _MSC_VER
#define RJGLOBAL_COMPILER_NAME "MSVC"

#else
#pragma error("Unsupported compiler.")
#endif

#pragma endregion Compiler Detection

#pragma region Architecture Detection

#if defined(_M_X64) || defined(__x86_64__)
#define RJGLOBAL_ARCHITECTURE_X64 1
#define RJGLOBAL_ARCHITECTURE_X86 0
#define RJGLOBAL_ARCHITECTURE_ARM 0
#define RJGLOBAL_ARCHITECTURE "X64"

#elif defined(_M_IX86) || defined(__i386__)
#define RJGLOBAL_ARCHITECTURE_X64 0
#define RJGLOBAL_ARCHITECTURE_X86 1
#define RJGLOBAL_ARCHITECTURE_ARM 0
#define RJGLOBAL_ARCHITECTURE "X86"

#elif defined(_M_ARM) || defined(__arm__) || defined(__aarch64__)
#define RJGLOBAL_ARCHITECTURE_X64 0
#define RJGLOBAL_ARCHITECTURE_X86 0
#define RJGLOBAL_ARCHITECTURE_ARM 1
#define RJGLOBAL_ARCHITECTURE "ARM"

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

#if RJGLOBAL_PLATFORM_WINDOWS
#define RJGLOBAL_PATH_DELIMETER_CHAR '\\'
#define RJGLOBAL_PATH_DELIMETER_STR "\\"
#elif RJGLOBAL_PLATFORM_UNIX
#define RJGLOBAL_PATH_DELIMETER_CHAR '/'
#define RJGLOBAL_PATH_DELIMETER_STR "/"
#endif

#define RJGlobal_FileOpen(filePtr, fileName, mode) ((filePtr = fopen(fileName, mode)) != NULL)
#define RJGlobal_MemoryCopy(destination, size, source) memcpy(destination, source, size)
#define RJGlobal_MemorySet(destination, size, value) memset(destination, value, size)
#define RJGlobal_MemoryMove(destination, size, source) memmove(destination, source, size)

#define RJGLOBAL_TEMP_BUFFER_SIZE (size_t)128

#pragma region Typedefs

typedef void (*RJGlobal_VoidFunIntCharPtrPtr)(int, char **);

typedef void (*RJGlobal_VoidFunFloat)(float);

typedef void (*RJGlobal_VoidFunIntCharptr)(int, char *);

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
/// @note The log message is written to a file named 'RJ_DEBUG_FILE_NAME' macro which is defined in the header. Directory and name can be changed by modifying the macro.
void RJGlobal_Log(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Gets the executable file directory.
/// @return The null terminated C string : "path/to/exe/"
const char *RJGlobal_GetExecutablePath();

/// @brief Runs the main application loop and calls the setup and loop callbacks.
/// @param argc Command line argument count
/// @param argv Command line argument values
/// @note This function will run indefinitely until the loop callback is set to NULL. if the setup callback is NULL it returns normally.
void RJGlobal_Run(int argc, char **argv);

/// @brief Terminates and cleans up the internals, terminates after calling the terminate callback .
/// @param exitCode The code to pass to exit() call.
/// @param message The message to show to the console before exiting.
void RJGlobal_Terminate(int exitCode, char *message);

#pragma region Callbacks

/// @brief Sets the setup callback function that gets called once at application start
/// @param setupCallback Function to call during application setup
void RJGlobal_SetSetupCallback(RJGlobal_VoidFunIntCharPtrPtr setupCallback);

/// @brief Sets the main loop callback function that gets called every frame
/// @param loopCallback Function to call every frame, receives deltatime in seconds as parameter
void RJGlobal_SetLoopCallback(RJGlobal_VoidFunFloat loopCallback);

/// @brief Sets the callback function for the global application terminate function. After setting, terminate function calls the callback function before its own instructions.
/// @param terminateCallback Function to call when terminate is called. Should not exit the program. Receives exit code and exit message as parameters.
void RJGlobal_SetTerminateCallback(RJGlobal_VoidFunIntCharptr terminateCallback);

#pragma region Callbacks

#pragma endregion Functions and Macros

#pragma region Debug Log

#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define RJGLOBAL_BUILD_DEBUG true
#else
#define RJGLOBAL_BUILD_DEBUG false
#endif

#define RJ_DEBUG_SAFE_LOGGING RJGLOBAL_BUILD_DEBUG
#define RJ_DEBUG_FLUSH_AFTER_LOG RJ_DEBUG_SAFE_LOGGING

#define RJGLOBAL_DEBUG_INFO RJGLOBAL_BUILD_DEBUG
#define RJGLOBAL_DEBUG_WARNING RJGLOBAL_BUILD_DEBUG
#define RJGLOBAL_DEBUG_ERROR RJGLOBAL_BUILD_DEBUG
#define RJGLOBAL_DEBUG_ASSERT RJGLOBAL_BUILD_DEBUG

#define RJGLOBAL_DEBUG_TERMINATE_ON_ERROR RJ_DEBUG_SAFE_LOGGING
#define RJGLOBAL_DEBUG_TERMINATE_ON_ASSERT RJ_DEBUG_SAFE_LOGGING

#define RJ_DEBUG_TIME_FORMAT "%H:%M:%S"
#define RJ_DEBUG_FILE_NAME "debug.log"

#define RJGlobal_DebugLog(terminate, header, format, ...)                                     \
    do                                                                                        \
    {                                                                                         \
        RJGlobal_Log(terminate, header, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)

#if RJGLOBAL_DEBUG_INFO == false
#define RJGlobal_DebugInfo(format, ...)
#else
#define RJGlobal_DebugInfo(format, ...)                          \
    do                                                           \
    {                                                            \
        RJGlobal_DebugLog(false, "INFO", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJGLOBAL_DEBUG_WARNING == false
#define RJGlobal_DebugWarning(format, ...)
#else
#define RJGlobal_DebugWarning(format, ...)                          \
    do                                                              \
    {                                                               \
        RJGlobal_DebugLog(false, "WARNING", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJGLOBAL_DEBUG_ERROR == false
#define RJGlobal_DebugError(format, ...)
#else
#define RJGlobal_DebugError(format, ...)                                                      \
    do                                                                                        \
    {                                                                                         \
        RJGlobal_DebugLog(RJGLOBAL_DEBUG_TERMINATE_ON_ERROR, "ERROR", format, ##__VA_ARGS__); \
    } while (false)
#endif

#if RJGLOBAL_DEBUG_ASSERT == false
#define RJGlobal_DebugAssert(condition, format, ...) (void)(condition)

#define RJGlobal_DebugAssertNullPointerCheck(ptr)

#define RJGlobal_DebugAssertFileOpenCheck(filePtr, fileName, mode) RJGlobal_FileOpen(filePtr, fileName, mode)

#else
#define RJGlobal_DebugAssert(condition, format, ...)                                                           \
    do                                                                                                         \
    {                                                                                                          \
        if (!(condition))                                                                                      \
        {                                                                                                      \
            RJGlobal_DebugLog(RJGLOBAL_DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", format, ##__VA_ARGS__); \
        }                                                                                                      \
    } while (false)

#define RJGlobal_DebugAssertNullPointerCheck(ptr) \
    RJGlobal_DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)

#define RJGlobal_DebugAssertFileOpenCheck(filePtr, fileName, mode) \
    RJGlobal_DebugAssert(RJGlobal_FileOpen(filePtr, fileName, mode), "File open failed for %s", fileName)

#endif

#pragma endregion Debug
