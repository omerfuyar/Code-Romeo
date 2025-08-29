#include "tools/Resources.h"
#include "utilities/Timer.h"

#if PLATFORM_WINDOWS
#include <Windows.h>
#define GetExePath(buffer, bufferSize) GetModuleFileName(NULL, buffer, bufferSize)
#elif PLATFORM_UNIX
#include <unistd.h>
#define GetExePath(buffer, bufferSize) readlink("/proc/self/exe", buffer, bufferSize)
#endif

String EXECUTABLE_DIRECTORY_PATH = {0};

void Resource_Initialize()
{
    char buffer[TEMP_BUFFER_SIZE * 2];
    GetExePath(buffer, sizeof(buffer));

    String tempString = String_CreateReference(buffer, strlen(buffer));
    String pathDelimeter = scl(PATH_DELIMETER);

    String pathBuffer[16];
    size_t pathCount = 0;
    String_Tokenize(tempString, pathDelimeter, &pathCount, pathBuffer, 16);

    EXECUTABLE_DIRECTORY_PATH = String_CreateCopy(pathBuffer[0].characters, pathBuffer[0].length);

    if (PLATFORM_UNIX)
    {
        String_ConcatBegin(&EXECUTABLE_DIRECTORY_PATH, pathDelimeter);
    }

    for (size_t i = 1; i < pathCount - 1; i++)
    {
        String_ConcatEnd(&EXECUTABLE_DIRECTORY_PATH, pathDelimeter);
        String_ConcatEnd(&EXECUTABLE_DIRECTORY_PATH, pathBuffer[i]);
    }

    String_ConcatEnd(&EXECUTABLE_DIRECTORY_PATH, pathDelimeter);

    DebugInfo("Executable path detected : '%s'", EXECUTABLE_DIRECTORY_PATH.characters);
}

Resource Resource_Create(String title, String path)
{
    Resource resource;
    resource.title = title;
    resource.path = String_CreateCopy(path.characters, path.length);
    String_ConcatBegin(&resource.path, scl(RESOURCE_PATH));
    String_ConcatBegin(&resource.path, EXECUTABLE_DIRECTORY_PATH);

    FILE *file = NULL;
    FileOpen(file, resource.path.characters, "r");
    DebugAssert(file != NULL, "File open failed for %s", resource.path.characters);

    char *dataBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT);
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';

    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        StringConcat(dataBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT, lineBuffer);
    }
    Timer_Stop(&timer);
    DebugInfo("Resource '%s' loaded in %f seconds.", resource.title.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

    fclose(file);

    resource.data = String_CreateCopy(dataBuffer, strlen(dataBuffer));

    free(dataBuffer);
    free(lineBuffer);

    return resource;
}

void Resource_Destroy(Resource *resource)
{
    DebugAssertNullPointerCheck(resource);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, TEMP_BUFFER_SIZE, resource->title.characters);

    String_Destroy(&resource->title);
    String_Destroy(&resource->path);
    String_Destroy(&resource->data);

    DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}

String Resource_GetExePath()
{
    return String_CreateReference(EXECUTABLE_DIRECTORY_PATH.characters, EXECUTABLE_DIRECTORY_PATH.length);
}
