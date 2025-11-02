#pragma once

#pragma region Platform Detection

#if defined(_WIN32)

/// @brief Current platform is Windows.
#define RJGLOBAL_PLATFORM_WINDOWS 1
/// @brief Current platform is not Linux.
#define RJGLOBAL_PLATFORM_LINUX 0
/// @brief Current platform is not MacOS.
#define RJGLOBAL_PLATFORM_MACOS 0
/// @brief Platform name string.
#define RJGLOBAL_PLATFORM "WINDOWS"

#elif defined(__linux__)

/// @brief Current platform is Linux.
#define RJGLOBAL_PLATFORM_LINUX 1
/// @brief Current platform is not Windows.
#define RJGLOBAL_PLATFORM_WINDOWS 0
/// @brief Current platform is not MacOS.
#define RJGLOBAL_PLATFORM_MACOS 0
/// @brief Platform name string.
#define RJGLOBAL_PLATFORM "LINUX"

#elif defined(__APPLE__) && defined(__MACH__)

/// @brief Current platform is MacOS.
#define RJGLOBAL_PLATFORM_MACOS 1
/// @brief Current platform is not Windows.
#define RJGLOBAL_PLATFORM_WINDOWS 0
/// @brief Current platform is not Linux.
#define RJGLOBAL_PLATFORM_LINUX 0
/// @brief Platform name string.
#define RJGLOBAL_PLATFORM "MACOS"

#else

#pragma error("Unsupported platform.")

#endif

#if RJGLOBAL_PLATFORM_LINUX || RJGLOBAL_PLATFORM_MACOS

/// @brief Current platform is Unix-like.
#define RJGLOBAL_PLATFORM_UNIX 1

#else

/// @brief Current platform is not a Unix-like.
#define RJGLOBAL_PLATFORM_UNIX 0

#endif

#pragma endregion Platform Detection

#pragma region Compiler Detection

#if defined(__clang__)

/// @brief Clang compiler is detected.
#define RJGLOBAL_COMPILER_CLANG 1
/// @brief GCC compiler is not detected.
#define RJGLOBAL_COMPILER_GCC 0
/// @brief MSVC compiler is not detected.
#define RJGLOBAL_COMPILER_MSVC 0
/// @brief Compiler version number.
#define RJGLOBAL_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
/// @brief Compiler name string.
#define RJGLOBAL_COMPILER_NAME "CLANG"

#elif defined(__GNUC__)

/// @brief Clang compiler is not detected.
#define RJGLOBAL_COMPILER_CLANG 0
/// @brief GCC compiler is detected.
#define RJGLOBAL_COMPILER_GCC 1
/// @brief MSVC compiler is not detected.
#define RJGLOBAL_COMPILER_MSVC 0
/// @brief Compiler version number.
#define RJGLOBAL_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
/// @brief Compiler name string.
#define RJGLOBAL_COMPILER_NAME "GCC"

#elif defined(_MSC_VER)

/// @brief Clang compiler is not detected.
#define RJGLOBAL_COMPILER_CLANG 0
/// @brief GCC compiler is not detected.
#define RJGLOBAL_COMPILER_GCC 0
/// @brief MSVC compiler is detected.
#define RJGLOBAL_COMPILER_MSVC 1
/// @brief Compiler version number.
#define RJGLOBAL_COMPILER_VERSION _MSC_VER
/// @brief Compiler name string.
#define RJGLOBAL_COMPILER_NAME "MSVC"

#else

#pragma error("Unsupported compiler.")

#endif

#pragma endregion Compiler Detection

#pragma region Architecture Detection

#if defined(_M_X64) || defined(__x86_64__)

/// @brief x64 architecture is detected.
#define RJGLOBAL_ARCHITECTURE_X64 1
/// @brief x86 architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_X86 0
/// @brief ARM architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_ARM 0
/// @brief Architecture name string.
#define RJGLOBAL_ARCHITECTURE "X64"

#elif defined(_M_IX86) || defined(__i386__)

/// @brief x86 architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_X64 0
/// @brief x86 architecture is detected.
#define RJGLOBAL_ARCHITECTURE_X86 1
/// @brief ARM architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_ARM 0
/// @brief Architecture name string.
#define RJGLOBAL_ARCHITECTURE "X86"

#elif defined(_M_ARM) || defined(__arm__) || defined(__aarch64__)

/// @brief x64 architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_X64 0
/// @brief x86 architecture is not detected.
#define RJGLOBAL_ARCHITECTURE_X86 0
/// @brief ARM architecture is detected.
#define RJGLOBAL_ARCHITECTURE_ARM 1
/// @brief Architecture name string.
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

/// @brief Character used for path delimiters.
#define RJGLOBAL_PATH_DELIMETER_CHAR '\\'
/// @brief String used for path delimiters.
#define RJGLOBAL_PATH_DELIMETER_STR "\\"

#elif RJGLOBAL_PLATFORM_UNIX

/// @brief Character used for path delimiters.
#define RJGLOBAL_PATH_DELIMETER_CHAR '/'
/// @brief String used for path delimiters.
#define RJGLOBAL_PATH_DELIMETER_STR "/"

#endif

/// @brief Size of the temporary buffer used in various operations.
#define RJGLOBAL_TEMP_BUFFER_SIZE (size_t)128

/// @brief Macro wrapper for file opening to use it in if statements.
#define RJGlobal_FileOpen(filePtr, fileName, mode) ((filePtr = fopen(fileName, mode)) != NULL)
/// @brief Macro wrapper for memory copy operation.
#define RJGlobal_MemoryCopy(destination, size, source) memcpy(destination, source, size)
/// @brief Macro wrapper for memory set operation.
#define RJGlobal_MemorySet(destination, size, value) memset(destination, value, size)
/// @brief Macro wrapper for memory move operation.
#define RJGlobal_MemoryMove(destination, size, source) memmove(destination, source, size)

#pragma region Typedefs

/// @brief Function pointer type used in setup callback function.
typedef void (*RJGlobal_VoidFunIntCharPtrPtr)(int, char **);

/// @brief Function pointer type used in main loop callback function.
typedef void (*RJGlobal_VoidFunFloat)(float);

/// @brief Function pointer type used in terminate callback function.
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
/// @note The log message is written to a file named 'RJGLOBAL_DEBUG_FILE_NAME' macro which is defined in the header. Directory and name can be changed by modifying the macro.
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

/// @brief Sets the setup callback function that gets called once at application start
/// @param setupCallback Function to call during application setup
void RJGlobal_SetSetupCallback(RJGlobal_VoidFunIntCharPtrPtr setupCallback);

/// @brief Sets the main loop callback function that gets called every frame
/// @param loopCallback Function to call every frame, receives deltatime in seconds as parameter
void RJGlobal_SetLoopCallback(RJGlobal_VoidFunFloat loopCallback);

/// @brief Sets the callback function for the global application terminate function. After setting, terminate function calls the callback function before its own instructions.
/// @param terminateCallback Function to call when terminate is called. Should not exit the program. Receives exit code and exit message as parameters.
void RJGlobal_SetTerminateCallback(RJGlobal_VoidFunIntCharptr terminateCallback);

#pragma endregion Functions and Macros

#pragma region Debug Log

#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)

/// @brief Build is debug build.
#define RJGLOBAL_BUILD_DEBUG true

#else

/// @brief Build is release build.
#define RJGLOBAL_BUILD_DEBUG false

#endif

/// @brief Safe logging is enabled. Controls internal file operations to prevent crashes during logging. Activating this may reduce performance of logging but increases stability.
#define RJGLOBAL_DEBUG_SAFE_LOGGING RJGLOBAL_BUILD_DEBUG
/// @brief Flush log file after every log entry. May reduce performance but ensures all logs are written to file immediately.
#define RJGLOBAL_DEBUG_FLUSH_AFTER_LOG RJGLOBAL_DEBUG_SAFE_LOGGING

/// @brief Info level logging macros enabled.
#define RJGLOBAL_DEBUG_INFO RJGLOBAL_BUILD_DEBUG
/// @brief Warning level logging macros enabled.
#define RJGLOBAL_DEBUG_WARNING RJGLOBAL_BUILD_DEBUG
/// @brief Error level logging macros enabled.
#define RJGLOBAL_DEBUG_ERROR RJGLOBAL_BUILD_DEBUG
/// @brief Assertion macros enabled.
#define RJGLOBAL_DEBUG_ASSERT RJGLOBAL_BUILD_DEBUG

/// @brief Terminate application on error log.
#define RJGLOBAL_DEBUG_TERMINATE_ON_ERROR RJGLOBAL_DEBUG_SAFE_LOGGING
/// @brief Terminate application on assertion failure.
#define RJGLOBAL_DEBUG_TERMINATE_ON_ASSERT RJGLOBAL_DEBUG_SAFE_LOGGING

/// @brief Time format for debug log entries. Uses strftime format. Used when logging to the debug log file.
#define RJGLOBAL_DEBUG_TIME_FORMAT "%H:%M:%S"
/// @brief Debug log file name. Can be changed to modify the log file location and name.
#define RJGLOBAL_DEBUG_FILE_NAME "debug.log"

/// @brief Macro wrapper for RJGlobal_Log file to pass file, line and function automatically.
#define RJGlobal_DebugLog(terminate, header, format, ...)                                     \
    do                                                                                        \
    {                                                                                         \
        RJGlobal_Log(terminate, header, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)

#if RJGLOBAL_DEBUG_INFO == false

#define RJGlobal_DebugInfo(format, ...)

#else

/// @brief Logs an info level message to the debug log.
#define RJGlobal_DebugInfo(format, ...)                          \
    do                                                           \
    {                                                            \
        RJGlobal_DebugLog(false, "INFO", format, ##__VA_ARGS__); \
    } while (false)

#endif

#if RJGLOBAL_DEBUG_WARNING == false

#define RJGlobal_DebugWarning(format, ...)

#else

/// @brief Logs a warning level message to the debug log.
#define RJGlobal_DebugWarning(format, ...)                          \
    do                                                              \
    {                                                               \
        RJGlobal_DebugLog(false, "WARNING", format, ##__VA_ARGS__); \
    } while (false)

#endif

#if RJGLOBAL_DEBUG_ERROR == false

#define RJGlobal_DebugError(format, ...)

#else

/// @brief Logs an error level message to the debug log and terminates the application if configured.
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

/// @brief Logs an assertion failure message to the debug log and terminates the application on failure if configured.
#define RJGlobal_DebugAssert(condition, format, ...)                                                           \
    do                                                                                                         \
    {                                                                                                          \
        if (!(condition))                                                                                      \
        {                                                                                                      \
            RJGlobal_DebugLog(RJGLOBAL_DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", format, ##__VA_ARGS__); \
        }                                                                                                      \
    } while (false)

/// @brief Asserts that the given pointer is not NULL. Logs and terminates on failure if configured.
#define RJGlobal_DebugAssertNullPointerCheck(ptr) \
    RJGlobal_DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)

/// @brief Asserts that the file was opened successfully. Logs and terminates on failure if configured.
#define RJGlobal_DebugAssertFileOpenCheck(filePtr, fileName, mode) \
    RJGlobal_DebugAssert(RJGlobal_FileOpen(filePtr, fileName, mode), "File open failed for %s", fileName)

#endif

#pragma endregion Debug
