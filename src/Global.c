#include "Global.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#define GetExePath(buffer, bufferSize) GetModuleFileName(NULL, buffer, bufferSize)
#elif PLATFORM_UNIX
#include <unistd.h>
#define GetExePath(buffer, bufferSize) readlink("/proc/self/exe", buffer, bufferSize)
#endif

FILE *DEBUG_FILE = NULL;
char *DEBUG_FILE_NAME_STR = NULL;
char *EXECUTABLE_DIRECTORY_PATH = NULL;

void DebugLog(bool terminate, const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {.tv_nsec = 0, .tv_sec = 0};
    struct tm timer = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 0, .tm_mon = 0, .tm_year = 0, .tm_wday = 0, .tm_yday = 0, .tm_isdst = 0};
    char timeBuffer[TEMP_BUFFER_SIZE / 4];

    timespec_get(&tempSpec, TIME_UTC);
    LocalTime(&tempSpec.tv_sec, &timer);
    strftime(timeBuffer, sizeof(timeBuffer), DEBUG_TIME_FORMAT, &timer);

    if (DEBUG_FILE == NULL)
    {
        DEBUG_FILE_NAME_STR = (char *)malloc(strlen(GetExecutablePath()) + strlen(DEBUG_FILE_NAME) + 1);

        MemoryCopy(DEBUG_FILE_NAME_STR, strlen(EXECUTABLE_DIRECTORY_PATH) * sizeof(char), EXECUTABLE_DIRECTORY_PATH);
        MemoryCopy(DEBUG_FILE_NAME_STR + strlen(EXECUTABLE_DIRECTORY_PATH), strlen(DEBUG_FILE_NAME) * sizeof(char), DEBUG_FILE_NAME);

        DEBUG_FILE_NAME_STR[strlen(EXECUTABLE_DIRECTORY_PATH) + strlen(DEBUG_FILE_NAME)] = '\0';

        remove(DEBUG_FILE_NAME_STR);

        if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR, "a"))
        {
            char buffer[TEMP_BUFFER_SIZE] = {0};
            snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR);
            fprintf(stderr, "%s", buffer);
            Terminate(-1, buffer);
        }

        fprintf(DEBUG_FILE, "\n[%s:%03ld] : [INFO] :\nLog file created successfully.", timeBuffer, tempSpec.tv_nsec / 1000000);
    }
    else if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR, "a"))
    {
        char buffer[TEMP_BUFFER_SIZE] = {0};
        snprintf(buffer, sizeof(buffer), "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR);
        fprintf(stderr, "%s", buffer);
        Terminate(-1, buffer);
    }

    char messageBuffer[TEMP_BUFFER_SIZE * 4] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
    va_end(args);

    char finalBuffer[TEMP_BUFFER_SIZE * 5] = {0};

    snprintf(finalBuffer, sizeof(finalBuffer), "[%s:%03ld] : [%s] : [%s:%d:%s] :\n %s\n",
             timeBuffer, tempSpec.tv_nsec / 1000000, header, file, line, function, messageBuffer);

    fprintf(DEBUG_FILE, "%s", finalBuffer);

    if (DEBUG_FLUSH_AFTER_LOG)
    {
        fflush(DEBUG_FILE);
    }

    fclose(DEBUG_FILE);

    if (terminate)
    {
        Terminate(-1, finalBuffer);
    }
}

void Terminate(int exitCode, char *message)
{
    fprintf(stdout, "Terminating application with exit code: %d\nExit message : %s\n", exitCode, message);

    exit(exitCode);
}

char *GetExecutablePath()
{
    if (EXECUTABLE_DIRECTORY_PATH == NULL)
    {
        char buffer[TEMP_BUFFER_SIZE];
        GetExePath(buffer, sizeof(buffer));

        size_t currentIndex = strlen(buffer);

        while (buffer[--currentIndex] != PATH_DELIMETER_CHAR)
        {
            buffer[currentIndex] = '\0';
        }
        currentIndex++;

        EXECUTABLE_DIRECTORY_PATH = (char *)malloc(currentIndex + 1);
        DebugAssertNullPointerCheck(EXECUTABLE_DIRECTORY_PATH);

        MemoryCopy(EXECUTABLE_DIRECTORY_PATH, currentIndex * sizeof(char), buffer);
        EXECUTABLE_DIRECTORY_PATH[currentIndex] = '\0';

        DebugInfo("Executable path detected : '%s'", EXECUTABLE_DIRECTORY_PATH);
    }

    return EXECUTABLE_DIRECTORY_PATH;
}
