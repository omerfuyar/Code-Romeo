#include "Global.h"

#include "tools/Resources.h"

#if PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_UNIX
#include <unistd.h>
#endif

FILE *DEBUG_FILE = NULL;
String DEBUG_FILE_NAME_STR = {0};

void DebugLog(const char *header, const char *file, int line, const char *function, const char *format, ...)
{
    struct timespec tempSpec = {.tv_nsec = 0, .tv_sec = 0};
    struct tm timer = {.tm_sec = 0, .tm_min = 0, .tm_hour = 0, .tm_mday = 0, .tm_mon = 0, .tm_year = 0, .tm_wday = 0, .tm_yday = 0, .tm_isdst = 0};
    char buffer[16];

    timespec_get(&tempSpec, TIME_UTC);
    LocalTime(&tempSpec.tv_sec, &timer);
    strftime(buffer, sizeof(buffer), DEBUG_TIME_FORMAT, &timer);

    if (DEBUG_FILE == NULL)
    {
        DEBUG_FILE_NAME_STR = String_CreateCopy(Resource_GetExePath().characters, Resource_GetExePath().length);
        String_ConcatEnd(&DEBUG_FILE_NAME_STR, scl(DEBUG_FILE_NAME));

        remove(DEBUG_FILE_NAME_STR.characters);

        if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR.characters, "a"))
        {
            fprintf(stderr, "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR.characters);
            Terminate(-1); // todo error codes
        }

        fprintf(DEBUG_FILE, "[%s:%03ld] : [INFO] :\nLog file created successfully.\n", buffer, tempSpec.tv_nsec / 1000000);
    }
    else if (!FileOpen(DEBUG_FILE, DEBUG_FILE_NAME_STR.characters, "a"))
    {
        fprintf(stderr, "Failed to open debug file: %s\n", DEBUG_FILE_NAME_STR.characters);
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
    fprintf(stdout, "Terminating application with exit code: %d\n", exitCode);

    exit(exitCode);
}
