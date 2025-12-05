#include "tools/Resource.h"
#include "utilities/Timer.h"
#include "utilities/Maths.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(pop)
#endif

#pragma region ResourceText

ResourceText *ResourceText_Create(StringView file)
{

    ResourceText *resource = NULL;
    RJGlobal_DebugAssertAllocationCheck(ResourceText, resource, 1);

    resource->file = scc(file);

    // RJGlobal_Size pathCount = 0;
    // String pathBuffer[RJGLOBAL_TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(RJGLOBAL_PATH_DELIMETER_STR), &pathCount, pathBuffer, RJGLOBAL_TEMP_BUFFER_SIZE / 32);
    //
    // for (RJGlobal_Size i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    RJGlobal_Size lineCount = 0;
    int character = 0;

    FILE *fileHandle = NULL;
    RJGlobal_DebugAssertFileOpenCheck(fileHandle, fullPath.characters, "r");
    while ((character = fgetc(fileHandle)) != EOF)
    {
        if (character == '\n')
        {
            lineCount++;
        }
    }
    fclose(fileHandle);

    resource->lineCount = lineCount;

    // on heap because the buffer is too large for stack
    char *dataBuffer = NULL;
    RJGlobal_DebugAssertAllocationCheck(char, dataBuffer, resource->lineCount *RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    char *lineBuffer = NULL;
    RJGlobal_DebugAssertAllocationCheck(char, lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    RJGlobal_Size dataIndex = 0;

    RJGlobal_DebugAssertFileOpenCheck(fileHandle, fullPath.characters, "r");
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, fileHandle))
    {
        RJGlobal_Size lineLength = RJGlobal_StringLength(lineBuffer);
        RJGlobal_MemoryCopy(dataBuffer + dataIndex, lineLength, lineBuffer);
        dataIndex += lineLength;
    }
    fclose(fileHandle);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopySafe(dataBuffer, dataIndex);

    free(dataBuffer);
    free(lineBuffer);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource '%s' loaded.", resource->file.characters);

    return resource;
}

void ResourceText_Destroy(ResourceText *resource)
{
    RJGlobal_DebugAssertNullPointerCheck(resource);
    RJGlobal_DebugAssertNullPointerCheck(resource->file.characters);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(resource->file, tempTitle);

    String_Destroy(&resource->file);
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

ResourceImage *ResourceImage_Create(StringView file)
{
    ResourceImage *resourceImage = NULL;
    RJGlobal_DebugAssertAllocationCheck(ResourceImage, resourceImage, 1);

    resourceImage->file = scc(file);

    String fullPath = scc(resourceImage->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 4);
    RJGlobal_DebugAssertNullPointerCheck(resourceImage->data);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource Image '%s' loaded.", resourceImage->file.characters);

    return resourceImage;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    RJGlobal_DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(resourceImage->file, tempTitle);

    String_Destroy(&resourceImage->file);
    resourceImage->channels = 0;
    resourceImage->size = Vector2Int_New(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    RJGlobal_DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceImage
