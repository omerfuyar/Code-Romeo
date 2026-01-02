#include "utilities/Timer.h"

#define Timer_Min(a, b) ((a) < (b) ? (a) : (b))
#define Timer_Max(a, b) ((a) > (b) ? (a) : (b))

#pragma region Source Only

static TimePoint Timer_GetElapsedTime(const Timer *timer)
{
    RJ_DebugAssertNullPointerCheck(timer);

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

#pragma endregion Source Only

void TimePoint_Update(TimePoint *timePoint)
{
    RJ_DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    struct timespec currentTime = {0, 0};
    timespec_get(&currentTime, TIME_UTC);

    timePoint->seconds = currentTime.tv_sec;
    timePoint->nanoseconds = currentTime.tv_nsec;
}

float TimePoint_ToMilliseconds(const TimePoint *timePoint)
{
    RJ_DebugAssert(timePoint != NULL, "Null pointer passed as parameter.");

    return ((float)timePoint->seconds * 1000.0f) + ((float)timePoint->nanoseconds / 1000000.0f);
}

Timer Timer_Create(const char *title)
{
    Timer timer = {0};

    if (title == NULL)
    {
        title = "Timer";
    }

    size_t titleLength = strlen(title);
    if (titleLength >= TIMER_MAX_TITLE_LENGTH)
    {
        RJ_DebugWarning("Timer title '%s' is longer than the maximum length of %d characters. It will be truncated.", title, TIMER_MAX_TITLE_LENGTH - 1);
    }

    titleLength = Timer_Min(TIMER_MAX_TITLE_LENGTH - 1, titleLength);

    memcpy(timer.title, title, titleLength);
    timer.title[titleLength] = '\0';

    timer.isRunning = false;

    timer.startTime = (TimePoint){0, 0};
    timer.endTime = (TimePoint){0, 0};

    return timer;
}

void Timer_Start(Timer *timer)
{
    RJ_DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    if (timer->isRunning)
    {
        RJ_DebugWarning("Timer '%s' is already running. Cannot start.", timer->title);
        return;
    }

    timer->isRunning = true;

    TimePoint_Update(&timer->startTime);
}

void Timer_Stop(Timer *timer)
{
    RJ_DebugAssert(timer != NULL, "Null pointer passed as parameter.");

    if (!timer->isRunning)
    {
        RJ_DebugWarning("Timer '%s' is not running. Cannot stop.", timer->title);
        return;
    }

    TimePoint_Update(&timer->endTime);

    timer->isRunning = false;
}

void Timer_Reset(Timer *timer)
{
    RJ_DebugAssertNullPointerCheck(timer);

    TimePoint_Update(&timer->endTime);
    timer->startTime = timer->endTime;
}

time_t Timer_GetElapsedNanoseconds(const Timer *timer)
{
    RJ_DebugAssertNullPointerCheck(timer);

    TimePoint elapsedTime = Timer_GetElapsedTime(timer);
    return elapsedTime.seconds * 1000000000 + elapsedTime.nanoseconds;
}

float Timer_GetElapsedMilliseconds(const Timer *timer)
{
    RJ_DebugAssertNullPointerCheck(timer);

    TimePoint elapsedTime = Timer_GetElapsedTime(timer);
    return TimePoint_ToMilliseconds(&elapsedTime);
}
