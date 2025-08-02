#include <stdio.h>
#include <time.h>
#include "Global.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_UNIX
#include <unistd.h>
#endif

FILE *DEBUG_FILE = NULL;

void DebugLog(const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {0, 0}; //! don't change
    struct tm timer = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    char buffer[16];

    timespec_get(&tempSpec, TIME_UTC);
    localtime_s(&timer, &tempSpec.tv_sec);
    strftime(buffer, sizeof(buffer), DEBUG_TIME_FORMAT, &timer);

    if (DEBUG_FILE == NULL)
    {
        remove(DEBUG_FILE_NAME);

        if (fopen_s(&DEBUG_FILE, DEBUG_FILE_NAME, "a") != 0)
        {
            fprintf(stderr, "Failed to open debug file: %s\n", DEBUG_FILE_NAME);
            Terminate(-1); // todo error codes
        }

        fprintf(DEBUG_FILE, "[%s:%03ld] : [INFO] :\nLog file created successfully.\n", buffer, tempSpec.tv_nsec / 1000000);
    }
    else if (fopen_s(&DEBUG_FILE, DEBUG_FILE_NAME, "a") != 0)
    {
        fprintf(stderr, "Failed to open debug file: %s\n", DEBUG_FILE_NAME);
        Terminate(-1); // todo error codes
    }

    va_list args;
    va_start(args, format);
    fprintf(DEBUG_FILE, "[%s:%03ld] : [%s] : [%s:%d:%s] :\n",
            buffer, tempSpec.tv_nsec / 1000000, header, file, line, function);
    vfprintf(DEBUG_FILE, format, args);
    fprintf(DEBUG_FILE, "\n");
    va_end(args);

    if (DEBUG_FLUSH_AFTER_LOG)
    {
        fflush(DEBUG_FILE);
    }

    fclose(DEBUG_FILE);
}

void Terminate(int exitCode)
{
    DebugInfo("App terminated with exit code %d.", exitCode);

    exit(exitCode);
}
