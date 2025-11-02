#include "tools/Resources.h"
#include "utilities/Timer.h"
#include "utilities/Maths.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#pragma region ResourceText

ResourceText *ResourceText_Create(StringView name, StringView relativePath)
{
    ResourceText *resource = (ResourceText *)malloc(sizeof(ResourceText));
    RJGlobal_DebugAssertNullPointerCheck(resource);

    resource->name = scc(name);
    resource->path = scc(relativePath);

    // size_t pathCount = 0;
    // String pathBuffer[RJGLOBAL_TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(RJGLOBAL_PATH_DELIMETER_STR), &pathCount, pathBuffer, RJGLOBAL_TEMP_BUFFER_SIZE / 32);
    //
    // for (size_t i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->path);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));
    String_ConcatEnd(&fullPath, scv(resource->name));

    size_t lineCount = 0;
    int character = 0;

    FILE *file = NULL;
    RJGlobal_DebugAssertFileOpenCheck(file, fullPath.characters, "r");
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
    RJGlobal_DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    RJGlobal_DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    size_t dataIndex = 0;

    RJGlobal_DebugAssertFileOpenCheck(file, fullPath.characters, "r");
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        size_t lineLength = strlen(lineBuffer);
        RJGlobal_MemoryCopy(dataBuffer + dataIndex, lineLength, lineBuffer);
        dataIndex += lineLength;
    }
    fclose(file);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopySafe(dataBuffer, dataIndex);

    free(dataBuffer);
    free(lineBuffer);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource '%s' loaded.", resource->name.characters);

    return resource;
}

void ResourceText_Destroy(ResourceText *resource)
{
    RJGlobal_DebugAssertNullPointerCheck(resource);
    RJGlobal_DebugAssertNullPointerCheck(resource->name.characters);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, resource->name.length), resource->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, resource->name.length)] = '\0';

    String_Destroy(&resource->name);
    String_Destroy(&resource->path);
    String_Destroy(&resource->data);
    resource->lineCount = 0;

    if (resource != NULL)
    {
        free(resource);
        resource = NULL;
    }

    RJGlobal_DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceText

#pragma region ResourceImage

ResourceImage *ResourceImage_Create(StringView title, StringView path)
{
    ResourceImage *resourceImage = (ResourceImage *)malloc(sizeof(ResourceImage));
    RJGlobal_DebugAssertNullPointerCheck(resourceImage);

    resourceImage->name = scc(title);
    resourceImage->path = scc(path);

    String fullPath = scc(resourceImage->path);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));
    String_ConcatEnd(&fullPath, scv(resourceImage->name));

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 4);
    RJGlobal_DebugAssertNullPointerCheck(resourceImage->data);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource Image '%s' loaded.", resourceImage->name.characters);

    return resourceImage;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    RJGlobal_DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, resourceImage->name.length), resourceImage->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, resourceImage->name.length)] = '\0';

    String_Destroy(&resourceImage->name);
    String_Destroy(&resourceImage->path);
    resourceImage->channels = 0;
    resourceImage->size = Vector2Int_New(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    RJGlobal_DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceImage
