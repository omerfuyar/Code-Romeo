#include "tools/Resources.h"
#include "utilities/Timer.h"

Resource Resource_Create(String title, String path)
{
    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);

    Resource resource;
    resource.title = title;
    resource.path = String_CreateCopy(path.characters);
    String_ConcatBegin(&resource.path, scl(RESOURCE_PATH));
    String_ConcatBegin(&resource.path, String_CreateReference(GetExecutablePath()));

    // malloc because the buffer is too large for stack
    char *dataBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT);
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    size_t dataIndex = 0;

    FILE *file = NULL;
    DebugAssert(FileOpen(file, resource.path.characters, "r"), "File open failed for %s", resource.path.characters);

    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        size_t lineLength = strlen(lineBuffer);
        MemoryCopy(dataBuffer + dataIndex, lineLength * sizeof(char), lineBuffer);
        dataIndex += lineLength;
    }
    fclose(file);

    dataBuffer[dataIndex] = '\0';

    resource.data = String_CreateCopy(dataBuffer);

    free(dataBuffer);
    free(lineBuffer);

    Timer_Stop(&timer);

    DebugInfo("Resource '%s' loaded in %f seconds.", resource.title.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

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
