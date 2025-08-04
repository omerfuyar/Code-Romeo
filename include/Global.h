#pragma once

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MACOS 1
#else
#error "Unsupported platform."
#endif

#if PLATFORM_LINUX || PLATFORM_MACOS
#define PLATFORM_UNIX 1
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#define TEMP_TITLE_BUFFER_SIZE 128

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

#pragma region Functions

/// @brief Logs a debug message to the debug log file.
/// @param header The header of the log message, like "INFO", "WARNING", "ERROR", etc.
/// @param format The format string for the log message, similar to printf.
/// @param ... The arguments for the format string.
/// @note The log message is written to a file named 'DEBUG_FILE_NAME'. Directory and name can be changed by modifying the macro.
void DebugLog(const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Terminates and closes necessary utilities and exits the program.
/// @param exitCode The code to pass to exit() function.
void Terminate(int exitCode);

/// @brief Allocates memory from the arena. Then must be freed with ArenaFree. Does not uses debug functions to leave it to the user. Must be used only one at the same time.
/// @param size The size of the memory block to allocate.
/// @return A pointer to the allocated memory, or NULL if the allocation failed.
void *ArenaAllocate(size_t size);

/// @brief Frees memory allocated from the arena.
/// @param ptr A pointer to the memory block to free.
void ArenaFree(void *ptr);

#pragma endregion Functions

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
#define DebugInfo(format, ...)                                                 \
    do                                                                         \
    {                                                                          \
        DebugLog("INFO", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_WARNING_ENABLED == false
#define DebugWarning(format, ...)
#else
#define DebugWarning(format, ...)                                                 \
    do                                                                            \
    {                                                                             \
        DebugLog("WARNING", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_ERROR_ENABLED == false
#define DebugError(format, ...)
#else
#define DebugError(format, ...)                                                 \
    do                                                                          \
    {                                                                           \
        DebugLog("ERROR", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
        if (DEBUG_TERMINATE_ON_ERROR)                                           \
        {                                                                       \
            Terminate(EXIT_FAILURE);                                            \
        }                                                                       \
    } while (false)
#endif

#if DEBUG_ASSERT_ENABLED == false
#define DebugAssert(condition, format, ...)
#else
#define DebugAssert(condition, format, ...)                                                     \
    do                                                                                          \
    {                                                                                           \
        if (!(condition))                                                                       \
        {                                                                                       \
            DebugLog("ASSERTION FAILURE", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
            if (DEBUG_TERMINATE_ON_ASSERT)                                                      \
            {                                                                                   \
                Terminate(EXIT_FAILURE);                                                        \
            }                                                                                   \
        }                                                                                       \
    } while (false)
#endif

#define DebugAssertNullPointerCheck(ptr) DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)

#pragma endregion Debug
