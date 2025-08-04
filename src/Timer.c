#include "utilities/Timer.h"
#include <time.h>

void TimePoint_Update(TimePoint *timePoint)
{
    DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    struct timespec currentTime = {0, 0};
    timespec_get(&currentTime, TIME_UTC);

    timePoint->seconds = currentTime.tv_sec;
    timePoint->nanoseconds = currentTime.tv_nsec;
}

time_t TimePoint_ToMilliseconds(TimePoint *timePoint)
{
    return timePoint->seconds * 1000 + timePoint->nanoseconds / 1000000;
}

Timer Timer_CreateStack(String title)
{
    Timer timer;
    timer.title = title;
    timer.isRunning = false;

    timer.startTime = TIMEPOINT_NULL;
    timer.endTime = TIMEPOINT_NULL;

    return timer;
}

Timer *Timer_CreateHeap(String title)
{
    Timer *timer = (Timer *)malloc(sizeof(Timer));

    if (timer == NULL)
    {
        DebugError("Memory allocation failed for Timer.");
        return NULL;
    }

    timer->title = title;
    timer->isRunning = false;

    timer->startTime = TIMEPOINT_NULL;
    timer->endTime = TIMEPOINT_NULL;

    return timer;
}

void Timer_DestroyHeap(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    free(timer);

    timer = NULL;

    DebugInfo("Timer destroyed successfully.");
}

void Timer_Start(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    if (timer->isRunning)
    {
        DebugWarning("Timer is already running. Cannot start.");
        return;
    }

    timer->isRunning = true;

    TimePoint_Update(&timer->startTime);
}

void Timer_Stop(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    if (!timer->isRunning)
    {
        DebugWarning("Timer is not running. Cannot stop.");
        return;
    }

    TimePoint_Update(&timer->endTime);

    timer->isRunning = false;
}

void Timer_Reset(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    timer->startTime = timer->endTime;
}

TimePoint Timer_GetElapsedTime(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    TimePoint elapsedTime;
    elapsedTime.seconds = timer->endTime.seconds - timer->startTime.seconds;
    elapsedTime.nanoseconds = timer->endTime.nanoseconds - timer->startTime.nanoseconds;

    if (elapsedTime.nanoseconds < 0)
    {
        elapsedTime.seconds -= 1;
        elapsedTime.nanoseconds += 1000000000;
    }

    return elapsedTime;
}

time_t Timer_GetElapsedNanoseconds(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    TimePoint elapsedTime = Timer_GetElapsedTime(timer);
    return elapsedTime.seconds * 1000000000 + elapsedTime.nanoseconds;
}
