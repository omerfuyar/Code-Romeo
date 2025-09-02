#pragma once

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#define PLATFORM_LINUX 0
#define PLATFORM_MACOS 0
#elif defined(__linux__)
#define _POSIX_C_SOURCE 200809L
#define PLATFORM_LINUX 1
#define PLATFORM_WINDOWS 0
#define PLATFORM_MACOS 0
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MACOS 1
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX 0
#else
#error "Unsupported platform."
#endif

#if PLATFORM_LINUX || PLATFORM_MACOS
#define PLATFORM_UNIX 1
#else
#define PLATFORM_UNIX 0
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#pragma region Math Constants
// These constants have a suffix of '_M' to not conflict with any system variables or macros.

// The value of Pi
#define PI_M 3.14159265f

// The value of Euler's number
#define E_M 2.71828183f

// The square root of 2_
#define SQRT2_M 1.41421356f

// The square root of 3
#define SQRT3_M 1.73205081f

// The square root of 5
#define SQRT5_M 2.23606798f

// The Earth's gravity in m/s^2
#define GRAVITY_M 9.80665f

// The Universal gravitational constant in m^3 kg^-1 s^-2
#define G_M 6.67430e-11f

// The speed of light in m/s
#define C_M 299792458.0f

#pragma endregion Constants

#pragma endregion Functions and Macros

#define TEMP_BUFFER_SIZE 128

#if PLATFORM_WINDOWS
#define PATH_DELIMETER_CHAR '\\'
#define PATH_DELIMETER_STR "\\"

#define FileOpen(filePtr, fileName, mode) (fopen_s(&filePtr, fileName, mode) == 0)
#define MemoryCopy(destination, size, source) memcpy_s(destination, size, source, size)
#define LocalTime(timerIntPtr, timerStructPtr) localtime_s(timerStructPtr, timerIntPtr)

#elif PLATFORM_UNIX
#define PATH_DELIMETER_CHAR '/'
#define PATH_DELIMETER_STR "/"

#define FileOpen(filePtr, fileName, mode) ((filePtr = fopen(fileName, mode)) != NULL)
#define MemoryCopy(destination, size, source) memcpy(destination, source, size)
#define LocalTime(timerIntPtr, timerStructPtr) localtime_r(timerIntPtr, timerStructPtr)

#endif

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

/// @brief Logs a debug message to the debug log file.
/// @param header The header of the log message, like "INFO", "WARNING", "ERROR", etc.
/// @param format The format string for the log message, similar to printf.
/// @param ... The arguments for the format string.
/// @note The log message is written to a file named 'DEBUG_FILE_NAME'. Directory and name can be changed by modifying the macro.
void DebugLog(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Terminates and closes necessary utilities and exits the program.
/// @param exitCode The code to pass to exit() function.
/// @param message The message to show to the console.
void Terminate(int exitCode, char *message);

/// @brief Gets the executable file directory.
/// @return The null terminated C string : "path/to/exe/"
char *GetExecutablePath();

#pragma region Functions and Macros

#pragma region Debug Log

#define DEBUG_INFO_ENABLED true
#define DEBUG_WARNING_ENABLED true
#define DEBUG_ERROR_ENABLED true
#define DEBUG_ASSERT_ENABLED true

#define DEBUG_TERMINATE_ON_ERROR true
#define DEBUG_TERMINATE_ON_ASSERT true

#define DEBUG_FLUSH_AFTER_LOG true

#define DEBUG_TIME_FORMAT "%H:%M:%S"
#define DEBUG_FILE_NAME "debug.log"

#if DEBUG_INFO_ENABLED == false
#define DebugInfo(format, ...)
#else
#define DebugInfo(format, ...)                                                        \
    do                                                                                \
    {                                                                                 \
        DebugLog(false, "INFO", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_WARNING_ENABLED == false
#define DebugWarning(format, ...)
#else
#define DebugWarning(format, ...)                                                        \
    do                                                                                   \
    {                                                                                    \
        DebugLog(false, "WARNING", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_ERROR_ENABLED == false
#define DebugError(format, ...)
#else
#define DebugError(format, ...)                                                                           \
    do                                                                                                    \
    {                                                                                                     \
        DebugLog(DEBUG_TERMINATE_ON_ERROR, "ERROR", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_ASSERT_ENABLED == false
#define DebugAssert(condition, format, ...) condition
#else
#define DebugAssert(condition, format, ...)                                                                                \
    do                                                                                                                     \
    {                                                                                                                      \
        if (!(condition))                                                                                                  \
        {                                                                                                                  \
            DebugLog(DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
        }                                                                                                                  \
    } while (false)
#endif

#define DebugAssertNullPointerCheck(ptr) DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)
#define DebugAssertFileOpenCheck(filePtr, fileName, mode) DebugAssert(FileOpen(filePtr, fileName, mode), "File open failed for %s", fileName)

#pragma endregion Debug

#pragma region Macros

#pragma endregion Macros
