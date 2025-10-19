#include "Global.h"

#if RJ_PLATFORM_WINDOWS
#include <windows.h>
#define GetExePath(buffer, bufferSize) GetModuleFileName(NULL, buffer, bufferSize)
#elif RJ_PLATFORM_UNIX
#include <unistd.h>
#define GetExePath(buffer, bufferSize) readlink("/proc/self/exe", buffer, bufferSize)
#endif

#pragma region Source Only

FILE *DEBUG_FILE = NULL;
char *DEBUG_FILE_NAME_STR = NULL;
char *GLOBAL_EXECUTABLE_DIRECTORY_PATH = NULL;

VoidFunIntCharPtrPtr GLOBAL_SETUP_CALLBACK = NULL;
VoidFunFloat GLOBAL_LOOP_CALLBACK = NULL;
VoidFunIntCharptr GLOBAL_TERMINATE_CALLBACK = NULL;

#pragma endregion Source Only

void Global_DebugLog(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {.tv_nsec = 0, .tv_sec = 0};
    struct tm timer = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 0, .tm_mon = 0, .tm_year = 0, .tm_wday = 0, .tm_yday = 0, .tm_isdst = 0};
    char timeBuffer[RJ_TEMP_BUFFER_SIZE / 4];

    timespec_get(&tempSpec, TIME_UTC);
    LocalTime(&tempSpec.tv_sec, &timer);
    strftime(timeBuffer, sizeof(timeBuffer), RJ_DEBUG_TIME_FORMAT, &timer);

    if (DEBUG_FILE == NULL)
    {
        DEBUG_FILE_NAME_STR = (char *)malloc(strlen(Global_GetExecutablePath()) + strlen(RJ_DEBUG_FILE_NAME) + 1);

        MemoryCopy(DEBUG_FILE_NAME_STR, strlen(GLOBAL_EXECUTABLE_DIRECTORY_PATH), GLOBAL_EXECUTABLE_DIRECTORY_PATH);
        MemoryCopy(DEBUG_FILE_NAME_STR + strlen(GLOBAL_EXECUTABLE_DIRECTORY_PATH), strlen(RJ_DEBUG_FILE_NAME), RJ_DEBUG_FILE_NAME);

        DEBUG_FILE_NAME_STR[strlen(GLOBAL_EXECUTABLE_DIRECTORY_PATH) + strlen(RJ_DEBUG_FILE_NAME)] = '\0';

        remove(DEBUG_FILE_NAME_STR);

        if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR, "a"))
        {
            char buffer[RJ_TEMP_BUFFER_SIZE] = {0};
            snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR);
            fprintf(stderr, "%s", buffer);
            Global_Terminate(EXIT_FAILURE, buffer);
        }

        fprintf(DEBUG_FILE, "[%s:%03ld] : [INFO] :\nLog file created successfully.\n", timeBuffer, tempSpec.tv_nsec / 1000000);
    }
#if RJ_DEBUG_SAFE_LOGGING
    else if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR, "a"))
    {
        char buffer[RJ_TEMP_BUFFER_SIZE] = {0};
        snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR);
        fprintf(stderr, "%s", buffer);
        Global_Terminate(EXIT_FAILURE, buffer);
    }
#endif

    char messageBuffer[RJ_TEMP_BUFFER_SIZE * 4] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    char finalBuffer[RJ_TEMP_BUFFER_SIZE * 5] = {0};

    snprintf(finalBuffer, sizeof(finalBuffer), "[%s:%03ld] : [%s] : [%s:%d:%s] :\n%s\n",
             timeBuffer, tempSpec.tv_nsec / 1000000, header, file, line, function, messageBuffer);

    fprintf(DEBUG_FILE, "[%s:%03ld] : [%s] : [%s:%d:%s] :\n%s\n", timeBuffer, tempSpec.tv_nsec / 1000000, header, file, line, function, messageBuffer);

#if RJ_DEBUG_FLUSH_AFTER_LOG
    fflush(DEBUG_FILE);
#endif

#if RJ_DEBUG_SAFE_LOGGING
    fclose(DEBUG_FILE);
#endif

    if (terminate)
    {
        Global_Terminate(EXIT_FAILURE, finalBuffer);
    }
}

char *Global_GetExecutablePath()
{
    if (GLOBAL_EXECUTABLE_DIRECTORY_PATH == NULL)
    {
        char buffer[RJ_TEMP_BUFFER_SIZE];
        GetExePath(buffer, sizeof(buffer));

        size_t currentIndex = strlen(buffer);

        while (buffer[--currentIndex] != RJ_PATH_DELIMETER_CHAR)
        {
            buffer[currentIndex] = '\0';
        }
        currentIndex++;

        GLOBAL_EXECUTABLE_DIRECTORY_PATH = (char *)malloc(currentIndex + 1);
        DebugAssertNullPointerCheck(GLOBAL_EXECUTABLE_DIRECTORY_PATH);

        MemoryCopy(GLOBAL_EXECUTABLE_DIRECTORY_PATH, currentIndex, buffer);
        GLOBAL_EXECUTABLE_DIRECTORY_PATH[currentIndex] = '\0';

        DebugInfo("Executable path detected : '%s'", GLOBAL_EXECUTABLE_DIRECTORY_PATH);
    }

    return GLOBAL_EXECUTABLE_DIRECTORY_PATH;
}

void Global_Run(int argc, char **argv)
{
    if (GLOBAL_SETUP_CALLBACK != NULL)
    {
        GLOBAL_SETUP_CALLBACK(argc, argv);
    }

    struct timespec currentTime = {0, 0};
    struct timespec lastTime = {0, 0};
    struct timespec elapsedTime = {0, 0};
    float DT = 0.0f;

    timespec_get(&lastTime, TIME_UTC);

    while (GLOBAL_LOOP_CALLBACK != NULL)
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

        GLOBAL_LOOP_CALLBACK(DT);

        lastTime = currentTime;
    }
}

void Global_Terminate(int exitCode, char *message)
{
    if (GLOBAL_TERMINATE_CALLBACK != NULL)
    {
        GLOBAL_TERMINATE_CALLBACK(exitCode, message);
    }

    fprintf(stdout, "\nTerminating application with exit code: %d\nExit message : \n%s\n\n", exitCode, message);

    if (DEBUG_FILE != NULL)
    {
        fprintf(DEBUG_FILE, "\nTerminating application with exit code: %d\nExit message : \n%s\n\n", exitCode, message);
        fflush(DEBUG_FILE);
        fclose(DEBUG_FILE);
        DEBUG_FILE = NULL;
    }

    exit(exitCode);
}

#pragma region Callbacks

void Global_SetSetupCallback(VoidFunIntCharPtrPtr setupCallback)
{
    GLOBAL_SETUP_CALLBACK = setupCallback;
}

void Global_SetLoopCallback(VoidFunFloat loopCallback)
{
    GLOBAL_LOOP_CALLBACK = loopCallback;
}

void Global_SetTerminateCallback(VoidFunIntCharptr terminateCallback)
{
    GLOBAL_TERMINATE_CALLBACK = terminateCallback;
}

#pragma endregion Callbacks
