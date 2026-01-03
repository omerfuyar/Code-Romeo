#pragma once

#pragma region Platform Detection

#define RJ_PLATFORM_WINDOWS 0
#define RJ_PLATFORM_LINUX 1
#define RJ_PLATFORM_MACOS 2

#if defined(_WIN32)

/// @brief Current platform specifier. Use it with RJ_PLATFORM_<...> macros.
#define RJ_PLATFORM RJ_PLATFORM_WINDOWS
/// @brief Platform name string.
#define RJ_PLATFORM_STRING "WINDOWS"

#elif defined(__linux__)

/// @brief Current platform specifier. Use it with RJ_PLATFORM_<...> macros.
#define RJ_PLATFORM RJ_PLATFORM_LINUX
/// @brief Platform name string.
#define RJ_PLATFORM_STRING "LINUX"

#elif defined(__APPLE__) && defined(__MACH__)

/// @brief Current platform specifier. Use it with RJ_PLATFORM_<...> macros.
#define RJ_PLATFORM RJ_PLATFORM_MACOS
/// @brief Platform name string.
#define RJ_PLATFORM_STRING "MACOS"

#else

#pragma error("Unsupported platform.")

#endif

#if RJ_PLATFORM == RJ_PLATFORM_LINUX || RJ_PLATFORM == RJ_PLATFORM_MACOS

/// @brief Current platform is Unix-like.
#define RJ_PLATFORM_UNIX 1

#else

/// @brief Current platform is not a Unix-like.
#define RJ_PLATFORM_UNIX 0

#endif

#pragma endregion Platform Detection

#pragma region Compiler Detection

#define RJ_COMPILER_CLANG 0
#define RJ_COMPILER_GCC 1
#define RJ_COMPILER_MSVC 2

#if defined(__clang__)

/// @brief Current compiler specifier. Use it with RJ_COMPILER_<...> macros.
#define RJ_COMPILER RJ_COMPILER_CLANG
/// @brief Current compiler version number.
#define RJ_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
/// @brief Current compiler name string.
#define RJ_COMPILER_STRING "CLANG"

#elif defined(__GNUC__)

/// @brief Current compiler specifier. Use it with RJ_COMPILER_<...> macros.
#define RJ_COMPILER RJ_COMPILER_GCC
/// @brief Current compiler version number.
#define RJ_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
/// @brief Compiler name string.
#define RJ_COMPILER_NAME "GCC"

#elif defined(_MSC_VER)

/// @brief Current compiler specifier. Use it with RJ_COMPILER_<...> macros.
#define RJ_COMPILER RJ_COMPILER_MSVC
/// @brief Current compiler version number.
#define RJ_COMPILER_VERSION _MSC_VER
/// @brief Current compiler name string.
#define RJ_COMPILER_STRING "MSVC"

#else

#pragma error("Unsupported compiler.")

#endif

#pragma endregion Compiler Detection

#define _POSIX_C_SOURCE 200809L
#define _CRT_SECURE_NO_WARNINGS

#pragma endregion Architecture Detection

#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <float.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <limits.h>

/// @brief Size of the temporary buffer used in various operations.
#define RJ_TEMP_BUFFER_SIZE (RJ_Size)128

/// @brief Invalid index constant for RJ_Size type.
#define RJ_INDEX_INVALID ((RJ_Size)UINT32_MAX)

/// @brief Macro wrapper for file opening to use it in if statements.
#define RJ_FileOpen(filePointer, fileName, mode) (((filePointer) = fopen(fileName, mode)) != NULL)
/// @brief Macro wrapper for memory allocation operation. Pass char if the pointer type os void.
#define RJ_Allocate(type, pointer, count) (((pointer) = (type *)calloc((count), sizeof(type))) != NULL)
/// @brief Macro wrapper for memory reallocation operation. Pass char if the pointer type os void.
#define RJ_Reallocate(type, pointer, newCount) (((pointer) = (type *)realloc((pointer), sizeof(type) * (newCount))) != NULL)

/// @brief Macro wrapper for returning error code directly for file open. Use in functions that return RJ_Result. Variadic parameter is for cleanup commands if failed.
#define RJ_ReturnFileOpen(filePointer, fileName, mode, ...)                          \
    do                                                                               \
    {                                                                                \
        if (!RJ_FileOpen(filePointer, fileName, mode))                               \
        {                                                                            \
            __VA_ARGS__                                                              \
            RJ_DebugWarning("Failed to open file: %s with mode %s", fileName, mode); \
            return RJ_ERROR_FILE;                                                    \
        }                                                                            \
    } while (0)

/// @brief Macro wrapper for returning error code directly for memory allocation. Use in functions that return RJ_Result. Variadic parameter is for cleanup commands if failed.
#define RJ_ReturnAllocate(type, pointer, count, ...)                                                                          \
    do                                                                                                                        \
    {                                                                                                                         \
        if (!RJ_Allocate(type, pointer, count))                                                                               \
        {                                                                                                                     \
            __VA_ARGS__                                                                                                       \
            RJ_DebugWarning("Memory allocation failed for %zu bytes for type '%s'.", (RJ_Size)(count) * sizeof(type), #type); \
            return RJ_ERROR_ALLOCATION;                                                                                       \
        }                                                                                                                     \
    } while (0)

/// @brief Macro wrapper for returning error code directly for memory reallocation. Use in functions that return RJ_Result. Variadic parameter is for cleanup commands if failed.
#define RJ_ReturnReallocate(type, pointer, newCount, ...)                                                                          \
    do                                                                                                                             \
    {                                                                                                                              \
        if (!RJ_Reallocate(type, pointer, newCount))                                                                               \
        {                                                                                                                          \
            __VA_ARGS__                                                                                                            \
            RJ_DebugWarning("Memory reallocation failed for %zu bytes for type '%s'.", (RJ_Size)(newCount) * sizeof(type), #type); \
            return RJ_ERROR_ALLOCATION;                                                                                            \
        }                                                                                                                          \
    } while (0)

#pragma region Typedefs

/// @brief Function pointer type used in setup callback function.
typedef void (*RJ_VoidFunIntCharPtrPtr)(int, char **);

/// @brief Function pointer type used in main loop callback function.
typedef void (*RJ_VoidFunFloat)(float);

/// @brief Function pointer type used in terminate callback function.
typedef void (*RJ_VoidFunIntCharPtr)(int, char *);

/// @brief Size type to use for entire project
typedef uint32_t RJ_Size;

typedef enum RJ_Result
{
    RJ_OK = 0,
    RJ_ERROR_ALLOCATION,
    RJ_ERROR_FILE,
    RJ_ERROR_DEPENDENCY,
    RJ_ERROR_NOT_FOUND,
} RJ_Result;

#if RJ_COMPILER == RJ_COMPILER_GCC || RJ_COMPILER == RJ_COMPILER_CLANG
#define RJ_ResultDef __attribute__((warn_unused_result)) RJ_Result
#elif RJ_COMPILER == RJ_COMPILER_MSVC
#define RJ_ResultDef _Check_return_ RJ_Result
#else
#define RJ_ResultDef RJ_Result
#endif

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
RJ_ResultDef RJ_Log(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Gets the executable file directory.
/// @return The null terminated C string : "path/to/exe/"
const char *RJ_GetExecutablePath(void);

/// @brief Runs the main application loop and calls the setup and loop callbacks.
/// @param argc Command line argument count
/// @param argv Command line argument values
/// @note This function will run indefinitely until the loop callback is set to NULL. if the setup callback is NULL it returns normally.
void RJ_Run(int argc, char **argv);

/// @brief Terminates and cleans up the internals, terminates after calling the terminate callback .
/// @param exitCode The code to pass to exit() call.
/// @param message The message to show to the console before exiting.
void RJ_Terminate(int exitCode, char *message);

/// @brief Sets the setup callback function that gets called once at application start
/// @param setupCallback Function to call during application setup
void RJ_SetSetupCallback(RJ_VoidFunIntCharPtrPtr setupCallback);

/// @brief Sets the main loop callback function that gets called every frame
/// @param loopCallback Function to call every frame, receives deltatime in seconds as parameter
void RJ_SetLoopCallback(RJ_VoidFunFloat loopCallback);

/// @brief Sets the callback function for the global application terminate function. After setting, terminate function calls the callback function before its own instructions.
/// @param terminateCallback Function to call when terminate is called. Should not exit the program. Receives exit code and exit message as parameters.
void RJ_SetTerminateCallback(RJ_VoidFunIntCharPtr terminateCallback);

#pragma endregion Functions and Macros

#pragma region Debug Log

#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)

/// @brief Build is debug build.
#define RJ_BUILD_DEBUG true

#else

/// @brief Build is release build.
#define RJ_BUILD_DEBUG false

#endif

/// @brief Safe logging is enabled. Controls internal file operations to prevent crashes during logging. Activating this may reduce performance of logging but increases stability.
#define RJ_DEBUG_SAFE_LOGGING RJ_BUILD_DEBUG
/// @brief Flush log file after every log entry. May reduce performance but ensures all logs are written to file immediately.
#define RJ_DEBUG_FLUSH_AFTER_LOG RJ_DEBUG_SAFE_LOGGING

#ifndef RJ_DEBUG_INFO
/// @brief Info level logging macros enabled.
#define RJ_DEBUG_INFO RJ_BUILD_DEBUG
#endif

#ifndef RJ_DEBUG_WARNING
/// @brief Warning level logging macros enabled.
#define RJ_DEBUG_WARNING RJ_BUILD_DEBUG
#endif

#ifndef RJ_DEBUG_ERROR
/// @brief Error level logging macros enabled.
#define RJ_DEBUG_ERROR RJ_BUILD_DEBUG
#endif

#ifndef RJ_DEBUG_ASSERT
/// @brief Assertion macros enabled.
#define RJ_DEBUG_ASSERT RJ_BUILD_DEBUG
#endif

#ifndef RJ_DEBUG_TERMINATE_ON_ERROR
/// @brief Terminate application on error log.
#define RJ_DEBUG_TERMINATE_ON_ERROR RJ_DEBUG_SAFE_LOGGING
#endif

#ifndef RJ_DEBUG_TERMINATE_ON_ASSERT
/// @brief Terminate application on assertion failure.
#define RJ_DEBUG_TERMINATE_ON_ASSERT RJ_DEBUG_SAFE_LOGGING
#endif

/// @brief Time format for debug log entries. Uses strftime format. Used when logging to the debug log file.
#define RJ_DEBUG_TIME_FORMAT "%H:%M:%S"
/// @brief Debug log file name. Can be changed to modify the log file location and name.
#define RJ_DEBUG_FILE_NAME "debug.log"

/// @brief Macro wrapper for RJ_Log file to pass file, line and function automatically.
#define RJ_DebugLog(terminate, header, format, ...)                                     \
    do                                                                                  \
    {                                                                                   \
        RJ_Log(terminate, header, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)

#if RJ_DEBUG_INFO == false

#define RJ_DebugInfo(format, ...)

#else

/// @brief Logs an info level message to the debug log.
#define RJ_DebugInfo(format, ...)                          \
    do                                                     \
    {                                                      \
        RJ_DebugLog(false, "INFO", format, ##__VA_ARGS__); \
    } while (false)

#endif

#if RJ_DEBUG_WARNING == false

#define RJ_DebugWarning(format, ...)

#else

/// @brief Logs a warning level message to the debug log.
#define RJ_DebugWarning(format, ...)                          \
    do                                                        \
    {                                                         \
        RJ_DebugLog(false, "WARNING", format, ##__VA_ARGS__); \
    } while (false)

#endif

#if RJ_DEBUG_ERROR == false

#define RJ_DebugError(format, ...)

#else

/// @brief Logs an error level message to the debug log and terminates the application if configured.
#define RJ_DebugError(format, ...)                                                \
    do                                                                            \
    {                                                                             \
        RJ_DebugLog(RJ_DEBUG_TERMINATE_ON_ERROR, "ERROR", format, ##__VA_ARGS__); \
    } while (false)

#endif

#if RJ_DEBUG_ASSERT == false

#define RJ_DebugAssert(condition, format, ...) (void)(condition)
#define RJ_DebugAssertNullPointerCheck(ptr)

#else

/// @brief Logs an assertion failure message to the debug log and terminates the application on failure if configured.
#define RJ_DebugAssert(condition, format, ...)                                                     \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            RJ_DebugLog(RJ_DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", format, ##__VA_ARGS__); \
        }                                                                                          \
    } while (false)

/// @brief Asserts that the given pointer is not NULL. Logs and terminates on failure if configured.
#define RJ_DebugAssertNullPointerCheck(pointer) \
    RJ_DebugAssert(pointer != NULL, "Pointer '%s' cannot be NULL.", #pointer)

#endif

#pragma endregion Debug
