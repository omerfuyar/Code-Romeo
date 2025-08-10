#include "app/Resources.h"
#include "utilities/Timer.h"

Resource Resource_Create(String title, String path)
{
    Resource resource;

    resource.title = title;
    resource.path = String_CreateCopy(path.characters, path.length);
    String_ConcatBegin(&resource.path, scl(RESOURCE_DIRECTORY));

    FILE *file = NULL;
    FileOpen(file, resource.path.characters, "r");
    DebugAssert(file != NULL, "File open failed for %s", resource.path.characters);

    char *dataBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT);
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';

    Timer timer = Timer_Create(scl("Resource Loading"));
    Timer_Start(&timer);
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        StringConcat(dataBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT, lineBuffer);
    }
    Timer_Stop(&timer);
    DebugInfo("Resource '%s' loaded in %f seconds.", resource.title.characters, (double)Timer_GetElapsedNanoseconds(&timer) / 1000000000.0);

    fclose(file);

    resource.data = String_CreateCopy(dataBuffer, strlen(dataBuffer));

    free(dataBuffer);
    free(lineBuffer);

    return resource;
}

void Resource_Destroy(Resource *resource)
{
    DebugAssertNullPointerCheck(resource);

    char tempTitle[TEMP_TITLE_BUFFER_SIZE];
    StringCopy(tempTitle, TEMP_TITLE_BUFFER_SIZE, resource->title.characters);
    String_Destroy(&resource->title);

    String_Destroy(&resource->path);

    String_Destroy(&resource->data);

    DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}
