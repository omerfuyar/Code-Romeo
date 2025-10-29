#include "utilities/Timer.h"
#include "utilities/Maths.h"

void TimePoint_Update(TimePoint *timePoint)
{
    DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    struct timespec currentTime = {0, 0};
    timespec_get(&currentTime, TIME_UTC);

    timePoint->seconds = currentTime.tv_sec;
    timePoint->nanoseconds = currentTime.tv_nsec;
}

float TimePoint_ToMilliseconds(const TimePoint *timePoint)
{
    DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    return ((float)timePoint->seconds * 1000.0f) + ((float)timePoint->nanoseconds / 1000000.0f);
}

Timer Timer_Create(const char *title)
{
    Timer timer;

    size_t titleLength = strlen(title);

    timer.title = (char *)malloc(titleLength + 1);
    DebugAssertNullPointerCheck(timer.title);
    MemoryCopy(timer.title, titleLength + 1, title);
    timer.title[titleLength] = '\0';

    timer.isRunning = false;

    timer.startTime = (TimePoint){0, 0};
    timer.endTime = (TimePoint){0, 0};

    return timer;
}

void Timer_Destroy(Timer *timer)
{
    DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    size_t titleLength = strlen(timer->title);

    char tempTitle[RJ_TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(RJ_TEMP_BUFFER_SIZE - 1, titleLength), timer->title);
    tempTitle[Min(RJ_TEMP_BUFFER_SIZE - 1, titleLength)] = '\0';

    free(timer->title);
    timer->title = NULL;

    DebugInfo("Timer '%s' destroyed.", tempTitle);
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

float Timer_GetElapsedMilliseconds(const Timer *timer)
{
    DebugAssertNullPointerCheck(timer);

    TimePoint elapsedTime = Timer_GetElapsedTime(timer);
    return TimePoint_ToMilliseconds(&elapsedTime);
}
