#include "RJGlobal.h"

#if RJGLOBAL_PLATFORM == RJGLOBAL_PLATFORM_WINDOWS
#include <windows.h>
#define RJGlobal_GetExePath(buffer, bufferSize) GetModuleFileName(NULL, buffer, bufferSize)

#elif RJGLOBAL_PLATFORM == RJGLOBAL_PLATFORM_UNIX
#include <unistd.h>
#define RJGlobal_GetExePath(buffer, bufferSize) readlink("/proc/self/exe", buffer, bufferSize)

#endif

// #pragma message("Build info: " RJGLOBAL_PLATFORM " | " RJGLOBAL_COMPILER_NAME " | " RJGLOBAL_ARCHITECTURE)

#pragma region Source Only

FILE *RJGLOBAL_DEBUG_FILE = NULL;
char *RJGLOBAL_DEBUG_FILE_NAME_STR = NULL;
char *RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH = NULL;
bool RJGLOBAL_TERMINATE_BYPASS_CLEANUP = false;

RJGlobal_VoidFunIntCharPtrPtr RJGLOBAL_SETUP_CALLBACK = NULL;
RJGlobal_VoidFunFloat RJGLOBAL_LOOP_CALLBACK = NULL;
RJGlobal_VoidFunIntCharPtr RJGLOBAL_TERMINATE_CALLBACK = NULL;

#pragma endregion Source Only

void RJGlobal_Log(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {.tv_nsec = 0, .tv_sec = 0};
    struct tm timer = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 0, .tm_mon = 0, .tm_year = 0, .tm_wday = 0, .tm_yday = 0, .tm_isdst = 0};
    char timeBuffer[RJGLOBAL_TEMP_BUFFER_SIZE / 4];

    timespec_get(&tempSpec, TIME_UTC);
    timer = *localtime(&tempSpec.tv_sec);
    strftime(timeBuffer, sizeof(timeBuffer), RJGLOBAL_DEBUG_TIME_FORMAT, &timer);

    if (RJGLOBAL_DEBUG_FILE == NULL)
    {
        RJGlobal_DebugAssertAllocationCheck(char, RJGLOBAL_DEBUG_FILE_NAME_STR, RJGlobal_StringLength(RJGlobal_GetExecutablePath()) + RJGlobal_StringLength(RJGLOBAL_DEBUG_FILE_NAME) + 1);

        RJGlobal_MemoryCopy(RJGLOBAL_DEBUG_FILE_NAME_STR, RJGlobal_StringLength(RJGlobal_GetExecutablePath()), RJGlobal_GetExecutablePath());
        RJGlobal_MemoryCopy(RJGLOBAL_DEBUG_FILE_NAME_STR + RJGlobal_StringLength(RJGlobal_GetExecutablePath()), RJGlobal_StringLength(RJGLOBAL_DEBUG_FILE_NAME), RJGLOBAL_DEBUG_FILE_NAME);

        RJGLOBAL_DEBUG_FILE_NAME_STR[RJGlobal_StringLength(RJGlobal_GetExecutablePath()) + RJGlobal_StringLength(RJGLOBAL_DEBUG_FILE_NAME)] = '\0';

        remove(RJGLOBAL_DEBUG_FILE_NAME_STR);

        if (!RJGlobal_FileOpen(RJGLOBAL_DEBUG_FILE, RJGLOBAL_DEBUG_FILE_NAME_STR, "a"))
        {
            char buffer[RJGLOBAL_TEMP_BUFFER_SIZE] = {0};
            snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", RJGLOBAL_DEBUG_FILE_NAME_STR);
            fprintf(stderr, "%s", buffer);
            RJGLOBAL_TERMINATE_BYPASS_CLEANUP = true;
            RJGlobal_Terminate(EXIT_FAILURE, buffer);
        }

        fprintf(RJGLOBAL_DEBUG_FILE, "[%s:%03ld] : [INFO] :\nLog file created successfully.\n", timeBuffer, tempSpec.tv_nsec / 1000000);
    }
#if RJGLOBAL_DEBUG_SAFE_LOGGING
    else if (!RJGlobal_FileOpen(RJGLOBAL_DEBUG_FILE, RJGLOBAL_DEBUG_FILE_NAME_STR, "a"))
    {
        char buffer[RJGLOBAL_TEMP_BUFFER_SIZE] = {0};
        snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", RJGLOBAL_DEBUG_FILE_NAME_STR);
        fprintf(stderr, "%s", buffer);
        RJGlobal_Terminate(EXIT_FAILURE, buffer);
    }
#endif

    char messageBuffer[RJGLOBAL_TEMP_BUFFER_SIZE * 4] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    char finalBuffer[RJGLOBAL_TEMP_BUFFER_SIZE * 5] = {0};

    snprintf(finalBuffer, sizeof(finalBuffer), "[%s:%03ld] : [%s] : [%s:%d:%s] :\n%s\n",
             timeBuffer, tempSpec.tv_nsec / 1000000, header, file, line, function, messageBuffer);

    fprintf(RJGLOBAL_DEBUG_FILE, "[%s:%03ld] : [%s] : [%s:%d:%s] :\n%s\n", timeBuffer, tempSpec.tv_nsec / 1000000, header, file, line, function, messageBuffer);

#if RJGLOBAL_DEBUG_FLUSH_AFTER_LOG
    fflush(RJGLOBAL_DEBUG_FILE);
#endif

#if RJGLOBAL_DEBUG_SAFE_LOGGING
    fclose(RJGLOBAL_DEBUG_FILE);
#endif

    if (terminate)
    {
        RJGlobal_Terminate(EXIT_FAILURE, finalBuffer);
    }
}

const char *RJGlobal_GetExecutablePath()
{
    if (RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH == NULL)
    {
        char buffer[RJGLOBAL_TEMP_BUFFER_SIZE];
        RJGlobal_GetExePath(buffer, sizeof(buffer));

        RJGlobal_Size currentIndex = (RJGlobal_Size)RJGlobal_StringLength(buffer);

        while (buffer[--currentIndex] != RJGLOBAL_PATH_DELIMETER_CHAR)
        {
            buffer[currentIndex] = '\0';
        }
        currentIndex++;

        RJGlobal_DebugAssertAllocationCheck(char, RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH, currentIndex + 1);

        RJGlobal_MemoryCopy(RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH, currentIndex, buffer);
        RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH[currentIndex] = '\0';

        RJGlobal_DebugInfo("Executable path detected : '%s'", RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH);
    }

    return RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH;
}

void RJGlobal_Run(int argc, char **argv)
{
    if (RJGLOBAL_SETUP_CALLBACK != NULL)
    {
        RJGLOBAL_SETUP_CALLBACK(argc, argv);
    }

    struct timespec currentTime = {0, 0};
    struct timespec lastTime = {0, 0};
    struct timespec elapsedTime = {0, 0};
    float DT = 0.0f;

    timespec_get(&lastTime, TIME_UTC);

    while (RJGLOBAL_LOOP_CALLBACK != NULL)
    {
        timespec_get(&currentTime, TIME_UTC);

        elapsedTime.tv_sec = currentTime.tv_sec - lastTime.tv_sec;
        elapsedTime.tv_nsec = currentTime.tv_nsec - lastTime.tv_nsec;

        if (elapsedTime.tv_nsec < 0)
        {
            elapsedTime.tv_sec -= 1;
            elapsedTime.tv_nsec += 1000000000;
        }

        DT = (float)elapsedTime.tv_sec + (float)elapsedTime.tv_nsec / 1000000000.0f;

        RJGLOBAL_LOOP_CALLBACK(DT);

        lastTime = currentTime;
    }

    RJGlobal_Terminate(EXIT_SUCCESS, "Main loop has ended normally.");
}

void RJGlobal_Terminate(int exitCode, char *message)
{
    if (RJGLOBAL_TERMINATE_BYPASS_CLEANUP)
    {
        fprintf(stdout, "\nTerminating application with exit code: %d\nExit message : \n%s\n\n", exitCode, message);
        exit(exitCode);
    }

    if (RJGLOBAL_TERMINATE_CALLBACK != NULL)
    {
        RJGLOBAL_TERMINATE_CALLBACK(exitCode, message);
    }

#if !RJGLOBAL_DEBUG_SAFE_LOGGING
    if (RJGLOBAL_DEBUG_FILE != NULL)
    {
        fflush(RJGLOBAL_DEBUG_FILE);
        fclose(RJGLOBAL_DEBUG_FILE);
        RJGLOBAL_DEBUG_FILE = NULL;
    }
#endif

    if (RJGLOBAL_DEBUG_FILE_NAME_STR != NULL)
    {
        free(RJGLOBAL_DEBUG_FILE_NAME_STR);
        RJGLOBAL_DEBUG_FILE_NAME_STR = NULL;
    }

    if (RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH != NULL)
    {
        free(RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH);
        RJGLOBAL_GLOBAL_EXECUTABLE_DIRECTORY_PATH = NULL;
    }

    fprintf(stdout, "\nTerminating application with exit code: %d\nExit message : \n%s\n\n", exitCode, message);
    exit(exitCode);
}

void RJGlobal_SetSetupCallback(RJGlobal_VoidFunIntCharPtrPtr setupCallback)
{
    RJGLOBAL_SETUP_CALLBACK = setupCallback;
}

void RJGlobal_SetLoopCallback(RJGlobal_VoidFunFloat loopCallback)
{
    RJGLOBAL_LOOP_CALLBACK = loopCallback;
}

void RJGlobal_SetTerminateCallback(RJGlobal_VoidFunIntCharPtr terminateCallback)
{
    RJGLOBAL_TERMINATE_CALLBACK = terminateCallback;
}
