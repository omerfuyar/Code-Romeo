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

#pragma region Functions and Macros

#define TEMP_BUFFER_SIZE 128

#if PLATFORM_WINDOWS
#define PATH_DELIMETER_CHAR '\\'
#define PATH_DELIMETER_STR "\\"

#define FileOpen(filePtr, fileName, mode) (fopen_s(&filePtr, fileName, mode) == 0)
#define MemoryCopy(destination, size, source) memcpy_s(destination, size, source, size)
#define MemorySet(destination, value, size) memset(destination, value, size)
#define LocalTime(timerIntPtr, timerStructPtr) localtime_s(timerStructPtr, timerIntPtr)

#elif PLATFORM_UNIX
#define PATH_DELIMETER_CHAR '/'
#define PATH_DELIMETER_STR "/"

#define FileOpen(filePtr, fileName, mode) ((filePtr = fopen(fileName, mode)) != NULL)
#define MemoryCopy(destination, size, source) memcpy_s(destination, size, source, size)
#define MemorySet(destination, value, size) memset(destination, value, size) a
#define LocalTime(timerIntPtr, timerStructPtr) localtime_r(timerIntPtr, timerStructPtr)

#endif

/// @brief Logs a debug message to the debug log file. Use wrapper macros for ease of use.
/// @param header The header of the log message, like "INFO", "WARNING", "ERROR", etc.
/// @param format The format string for the log message, similar to printf.
/// @param ... The arguments for the format string.
/// @note The log message is written to a file named 'DEBUG_FILE_NAME'. Directory and name can be changed by modifying the macro.
void Global_DebugLog(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...);

/// @brief Terminates and closes necessary utilities and exits the program.
/// @param exitCode The code to pass to exit() function.
/// @param message The message to show to the console.
void Global_Terminate(int exitCode, char *message);

typedef void (*FunIntCharptrToVoid)(int, char *);

/// @brief Sets the callback function for the global application terminate function. After setting, terminate function calls the callback function before its own instructions.
/// @param terminateCallback Function to call when terminate is called. Should not exit the program.
void Global_SetTerminateCallback(FunIntCharptrToVoid terminateCallback);

/// @brief Gets the executable file directory.
/// @return The null terminated C string : "path/to/exe/"
char *Global_GetExecutablePath();

#pragma endregion Functions and Macros

#pragma region Debug Log

#define DEBUG_INFO_ENABLED true
#define DEBUG_WARNING_ENABLED true
#define DEBUG_ERROR_ENABLED true
#define DEBUG_ASSERT_ENABLED true

#define DEBUG_TERMINATE_ON_ERROR true
#define DEBUG_TERMINATE_ON_ASSERT true

#define DEBUG_FLUSH_AFTER_LOG false

#define DEBUG_TIME_FORMAT "%H:%M:%S"
#define DEBUG_FILE_NAME "debug.log"

#if DEBUG_INFO_ENABLED == false
#define DebugInfo(format, ...)
#else
#define DebugInfo(format, ...)                                                               \
    do                                                                                       \
    {                                                                                        \
        Global_DebugLog(false, "INFO", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_WARNING_ENABLED == false
#define DebugWarning(format, ...)
#else
#define DebugWarning(format, ...)                                                               \
    do                                                                                          \
    {                                                                                           \
        Global_DebugLog(false, "WARNING", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_ERROR_ENABLED == false
#define DebugError(format, ...)
#else
#define DebugError(format, ...)                                                                                  \
    do                                                                                                           \
    {                                                                                                            \
        Global_DebugLog(DEBUG_TERMINATE_ON_ERROR, "ERROR", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
    } while (false)
#endif

#if DEBUG_ASSERT_ENABLED == false
#define DebugAssert(condition, format, ...) condition
#define DebugAssertNullPointerCheck(ptr)
#define DebugAssertFileOpenCheck(filePtr, fileName, mode) FileOpen(filePtr, fileName, mode)
#else
#define DebugAssertNullPointerCheck(ptr) DebugAssert(ptr != NULL, "Pointer '%s' cannot be NULL.", #ptr)
#define DebugAssertFileOpenCheck(filePtr, fileName, mode) DebugAssert(FileOpen(filePtr, fileName, mode), "File open failed for %s", fileName)
#define DebugAssert(condition, format, ...)                                                                                       \
    do                                                                                                                            \
    {                                                                                                                             \
        if (!(condition))                                                                                                         \
        {                                                                                                                         \
            Global_DebugLog(DEBUG_TERMINATE_ON_ASSERT, "ASSERTION FAILURE", __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
        }                                                                                                                         \
    } while (false)
#endif

#pragma endregion Debug
