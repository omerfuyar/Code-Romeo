#include "RJGlobal.h"

#if RJ_PLATFORM == RJ_PLATFORM_WINDOWS
#include <windows.h>
#define RJ_GetExePath(buffer, bufferSize) GetModuleFileName(NULL, buffer, bufferSize)

#elif RJ_PLATFORM == RJ_PLATFORM_UNIX
#include <unistd.h>
#define RJ_GetExePath(buffer, bufferSize) readlink("/proc/self/exe", buffer, bufferSize)

#elif RJ_PLATFORM == RJ_PLATFORM_MACOS
#include <mach-o/dyld.h>
#define RJ_GetExePath(buffer, bufferSize) _NSGetExecutablePath(buffer, &size)

#endif

// #pragma message("Build info: " RJ_PLATFORM " | " RJ_COMPILER_NAME " | " RJ_ARCHITECTURE)

#pragma region Source Only

FILE *RJ_DEBUG_FILE = NULL;
char RJ_DEBUG_FILE_NAME_STR[RJ_TEMP_BUFFER_SIZE * 2] = {0};
char RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH[RJ_TEMP_BUFFER_SIZE * 2] = {0};

RJ_VoidFunIntCharPtrPtr RJ_SETUP_CALLBACK = NULL;
RJ_VoidFunFloat RJ_LOOP_CALLBACK = NULL;
RJ_VoidFunIntCharPtr RJ_TERMINATE_CALLBACK = NULL;

#pragma endregion Source Only

RJ_Result RJ_Log(RJ_Result terminate, const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {0};
    struct tm timer = {0};
    char timeBuffer[RJ_TEMP_BUFFER_SIZE / 4] = {0};

    timespec_get(&tempSpec, TIME_UTC);
    timer = *localtime(&tempSpec.tv_sec);
    strftime(timeBuffer, sizeof(timeBuffer), RJ_DEBUG_TIME_FORMAT, &timer);

    if (RJ_DEBUG_FILE == NULL)
    {
        memcpy(RJ_DEBUG_FILE_NAME_STR, RJ_GetExecutablePath(), strlen(RJ_GetExecutablePath()));
        memcpy(RJ_DEBUG_FILE_NAME_STR + strlen(RJ_GetExecutablePath()), RJ_DEBUG_FILE_NAME, strlen(RJ_DEBUG_FILE_NAME));

        RJ_DEBUG_FILE_NAME_STR[strlen(RJ_GetExecutablePath()) + strlen(RJ_DEBUG_FILE_NAME)] = '\0';

        remove(RJ_DEBUG_FILE_NAME_STR);

        RJ_ReturnFileOpen(RJ_DEBUG_FILE, RJ_DEBUG_FILE_NAME_STR, "w+");

        fprintf(RJ_DEBUG_FILE, "[%s:%03ld] : [INFO] :\nLog file created successfully.\n", timeBuffer, tempSpec.tv_nsec / 1000000);
    }
#if RJ_DEBUG_SAFE_LOGGING
    else
    {
        RJ_ReturnFileOpen(RJ_DEBUG_FILE, RJ_DEBUG_FILE_NAME_STR, "a");
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

    fprintf(RJ_DEBUG_FILE, "%s", finalBuffer);

#if RJ_DEBUG_SAFE_LOGGING
    fclose(RJ_DEBUG_FILE);
#endif

    if (terminate)
    {
        RJ_Terminate(EXIT_FAILURE, finalBuffer);
    }

    return RJ_OK;
}

const char *RJ_GetExecutablePath(void)
{
    if (RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH[0] == '\0')
    {
        RJ_GetExePath(RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH, sizeof(RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH));

        RJ_Size currentIndex = (RJ_Size)strlen(RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH);

        while (RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH[--currentIndex] != RJ_PATH_SEPARATOR)
        {
            RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH[currentIndex] = '\0';
        }
        currentIndex++;

        RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH[currentIndex] = '\0';

        RJ_DebugInfo("Executable path detected : '%s'", RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH);
    }

    return RJ_GLOBAL_EXECUTABLE_DIRECTORY_PATH;
}

void RJ_Run(int argc, char **argv)
{
    if (RJ_SETUP_CALLBACK != NULL)
    {
        RJ_SETUP_CALLBACK(argc, argv);
    }

    struct timespec currentTime = {0, 0};
    struct timespec lastTime = {0, 0};
    struct timespec elapsedTime = {0, 0};
    float DT = 0.0f;

    timespec_get(&lastTime, TIME_UTC);

    while (RJ_LOOP_CALLBACK != NULL)
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

        RJ_LOOP_CALLBACK(DT);

        lastTime = currentTime;
    }

    RJ_Terminate(EXIT_SUCCESS, "Main loop has ended normally.");
}

void RJ_Terminate(int exitCode, char *message)
{
    if (RJ_TERMINATE_CALLBACK != NULL)
    {
        RJ_TERMINATE_CALLBACK(exitCode, message);
    }

#if !RJ_DEBUG_SAFE_LOGGING
    if (RJ_DEBUG_FILE != NULL)
    {
        fflush(RJ_DEBUG_FILE);
        fclose(RJ_DEBUG_FILE);
        RJ_DEBUG_FILE = NULL;
    }
#endif

    fprintf(stdout, "\nTerminating application with exit code: %d\nExit message : \n%s\n\n", exitCode, message);
    exit(exitCode);
}

void RJ_SetSetupCallback(RJ_VoidFunIntCharPtrPtr setupCallback)
{
    RJ_SETUP_CALLBACK = setupCallback;
}

void RJ_SetLoopCallback(RJ_VoidFunFloat loopCallback)
{
    RJ_LOOP_CALLBACK = loopCallback;
}

void RJ_SetTerminateCallback(RJ_VoidFunIntCharPtr terminateCallback)
{
    RJ_TERMINATE_CALLBACK = terminateCallback;
}
