#include "app/Resources.h"
#include <stdio.h>
#include <string.h>

Resource *Resource_Create(String title, String path)
{
    Resource *resource = (Resource *)malloc(sizeof(Resource));
    DebugAssertNullPointerCheck(resource);

    resource->title = title;
    resource->path = path;

    FILE *file = NULL;
    FileOpen(file, resource->path.characters, "r");
    DebugAssert(file != NULL, "File open failed for %s", resource->path.characters);

    char dataBuffer[RESOURCE_FILE_MAX_LINE_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT];
    char lineBuffer[RESOURCE_FILE_MAX_LINE_COUNT];

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';

    while (fgets(lineBuffer, sizeof(lineBuffer), file))
    {
        StringConcat(dataBuffer, sizeof(lineBuffer), lineBuffer);
    }

    fclose(file);

    resource->dataSize = strlen(dataBuffer) + 1;

    resource->data = (char *)malloc(resource->dataSize);
    DebugAssert(resource->data != NULL, "Memory allocation failed for resource data %s.", resource->path.characters);

    StringCopy(resource->data, resource->dataSize, dataBuffer);

    return resource;
}

void Resource_Destroy(Resource *resource)
{
    DebugAssertNullPointerCheck(resource);

    resource->dataSize = 0;

    String_Destroy(resource->path);

    char tempTitle[TEMP_TITLE_BUFFER_SIZE];
    StringCopy(tempTitle, TEMP_TITLE_BUFFER_SIZE, resource->title.characters);
    String_Destroy(resource->title);

    free(resource->data);
    resource->data = NULL;

    free(resource);
    resource = NULL;

    DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}
