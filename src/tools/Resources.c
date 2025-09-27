#include "tools/Resources.h"
#include "utilities/Timer.h"
#include "utilities/Maths.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

ResourceText *ResourceText_Create(StringView name, StringView relativePath)
{
    ResourceText *resource = (ResourceText *)malloc(sizeof(ResourceText));
    DebugAssertNullPointerCheck(resource);

    resource->name = scc(name);
    resource->path = scc(relativePath);

    // size_t pathCount = 0;
    // String pathBuffer[TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(PATH_DELIMETER_STR), &pathCount, pathBuffer, TEMP_BUFFER_SIZE / 32);
    //
    // for (size_t i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->path);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(Global_GetExecutablePath()));
    String_ConcatEnd(&fullPath, scv(resource->name));

    size_t lineCount = 0;
    int character = 0;

    FILE *file = NULL;
    DebugAssertFileOpenCheck(file, fullPath.characters, "r");
    while ((character = fgetc(file)) != EOF)
    {
        if (character == '\n')
        {
            lineCount++;
        }
    }
    fclose(file);

    resource->lineCount = lineCount;

    // malloc because the buffer is too large for stack
    char *dataBuffer = (char *)malloc(resource->lineCount * RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    size_t dataIndex = 0;

    DebugAssertFileOpenCheck(file, fullPath.characters, "r");
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        size_t lineLength = strlen(lineBuffer);
        MemoryCopy(dataBuffer + dataIndex, lineLength, lineBuffer);
        dataIndex += lineLength;
    }
    fclose(file);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopyS(dataBuffer, dataIndex);

    free(dataBuffer);
    free(lineBuffer);

    String_Destroy(&fullPath);

    DebugInfo("Resource '%s' loaded.", resource->name.characters);

    return resource;
}

void ResourceText_Destroy(ResourceText *resource)
{
    DebugAssertNullPointerCheck(resource);
    DebugAssertNullPointerCheck(resource->name.characters);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, resource->name.length + 1), resource->name.characters);

    String_Destroy(&resource->name);
    String_Destroy(&resource->path);
    String_Destroy(&resource->data);
    resource->lineCount = 0;

    if (resource != NULL)
    {
        free(resource);
        resource = NULL;
    }

    DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}

ResourceImage *ResourceImage_Create(StringView title, StringView path)
{
    ResourceImage *resourceImage = (ResourceImage *)malloc(sizeof(ResourceImage));
    DebugAssertNullPointerCheck(resourceImage);

    resourceImage->name = scc(title);
    resourceImage->path = scc(path);

    String fullPath = scc(resourceImage->path);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(Global_GetExecutablePath()));
    String_ConcatEnd(&fullPath, scv(resourceImage->name));

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 0);
    DebugAssertNullPointerCheck(resourceImage->data);

    String_Destroy(&fullPath);

    DebugInfo("Resource Image '%s' loaded.", resourceImage->name.characters);

    return resourceImage;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, resourceImage->name.length + 1), resourceImage->name.characters);

    String_Destroy(&resourceImage->name);
    String_Destroy(&resourceImage->path);
    resourceImage->channels = 0;
    resourceImage->size = NewVector2Int(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}
