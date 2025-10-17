#include "utilities/Timer.h"

void TimePoint_Update(TimePoint *timePoint)
{
    DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    struct timespec currentTime = {0, 0};
    timespec_get(&currentTime, TIME_UTC);

    timePoint->seconds = currentTime.tv_sec;
    timePoint->nanoseconds = currentTime.tv_nsec;
}

Timer Timer_Create(const char *title)
{
    Timer timer;
    timer.title = title; // todo fix
    timer.isRunning = false;

    timer.startTime = (TimePoint){0, 0};
    timer.endTime = (TimePoint){0, 0};

    return timer;
}

void Timer_Start(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    if (timer->isRunning)
    {
        DebugWarning("Timer '%s' is already running. Cannot start.", timer->title);
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
        DebugWarning("Timer '%s' is not running. Cannot stop.", timer->title);
        return;
    }

    TimePoint_Update(&timer->endTime);

    timer->isRunning = false;
}

void Timer_Reset(Timer *timer)
{
    DebugAssertNullPointerCheck(timer);

    TimePoint_Update(&timer->endTime);
    timer->startTime = timer->endTime;
}

TimePoint Timer_GetElapsedTime(const Timer *timer)
{
    DebugAssertNullPointerCheck(timer);

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

time_t Timer_GetElapsedNanoseconds(const Timer *timer)
{
    DebugAssertNullPointerCheck(timer);

    TimePoint elapsedTime = Timer_GetElapsedTime(timer);
    return elapsedTime.seconds * 1000000000 + elapsedTime.nanoseconds;
}
