#pragma once

#include "RJGlobal.h"

#define TIMER_MAX_TITLE_LENGTH (RJ_TEMP_BUFFER_SIZE / 8)

#pragma region typedefs

/// @brief Represents a point in time or interval with seconds and nanoseconds precision.
typedef struct TimePoint
{
    time_t seconds;
    time_t nanoseconds;
} TimePoint;

/// @brief Represents a timer that can be used for measuring elapsed time. Should be used with helper functions.
typedef struct Timer
{
    char title[TIMER_MAX_TITLE_LENGTH];
    TimePoint startTime;
    TimePoint endTime;
    bool isRunning;
} Timer;

#pragma endregion typedefs

/// @brief Gets the current time point in seconds and nanoseconds.
/// @param timePoint Time Point to update with the current time.
void TimePoint_Update(TimePoint *timePoint);

/// @brief Converts a TimePoint to milliseconds.
/// @param timePoint Pointer to the TimePoint to convert.
float TimePoint_ToMilliseconds(const TimePoint *timePoint);

/// @brief Creates a new timer.
/// @param title Label for the timer. Null terminated.
/// @return Timer instance.
Timer Timer_Create(const char *title);

/// @brief Starts the timer, updating its start time to the current time.
/// @param timer Timer to start.
void Timer_Start(Timer *timer);

/// @brief Stops the timer, updating its end time to the current time.
/// @param timer Timer to stop.
void Timer_Stop(Timer *timer);

/// @brief Updates the timers end time and resets the start time to the end time. Does not check if the timer is running.
/// @param timer Timer to reset.
void Timer_Reset(Timer *timer);

/// @brief Gets the elapsed time of the timer in nanoseconds. Does not stop the timer or update its end time. User must stop the timer before calling this.
/// @param timer Pointer to the timer to get elapsed time from.
/// @return Elapsed time of the timer in nanoseconds.
time_t Timer_GetElapsedNanoseconds(const Timer *timer);

/// @brief Gets the elapsed time of the timer in milliseconds. Does not stop the timer or update its end time. User must stop the timer before calling this.
/// @param timer Pointer to the timer to get elapsed time from.
/// @return Elapsed time of the timer in milliseconds.
float Timer_GetElapsedMilliseconds(const Timer *timer);
