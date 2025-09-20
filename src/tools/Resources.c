#include "tools/Resources.h"
#include "utilities/Timer.h"
#include "utilities/Maths.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Resource *Resource_Create(String name, String relativePath)
{
    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);

    Resource *resource = (Resource *)malloc(sizeof(Resource));
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
    String_ConcatEnd(&fullPath, resource->name);

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
    char *dataBuffer = (char *)malloc(resource->lineCount * RESOURCE_FILE_LINE_MAX_CHAR_COUNT * sizeof(char));
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT * sizeof(char));
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    size_t dataIndex = 0;

    DebugAssertFileOpenCheck(file, fullPath.characters, "r");
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        size_t lineLength = strlen(lineBuffer);
        MemoryCopy(dataBuffer + dataIndex, lineLength * sizeof(char), lineBuffer);
        dataIndex += lineLength;
    }
    fclose(file);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopyS(dataBuffer, dataIndex);

    free(dataBuffer);
    free(lineBuffer);

    String_Destroy(&fullPath);

    Timer_Stop(&timer);

    DebugInfo("Resource '%s' loaded in %f seconds.", resource->name.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

    return resource;
}

void Resource_Destroy(Resource *resource)
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

ResourceImage *ResourceImage_Create(String title, String path)
{
    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);

    ResourceImage *resourceImage = (ResourceImage *)malloc(sizeof(ResourceImage));
    DebugAssertNullPointerCheck(resourceImage);

    resourceImage->name = scc(title);
    resourceImage->path = scc(path);

    String fullPath = scc(resourceImage->path);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(Global_GetExecutablePath()));
    String_ConcatEnd(&fullPath, resourceImage->name);

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 0);
    DebugAssertNullPointerCheck(resourceImage->data);

    String_Destroy(&fullPath);

    Timer_Stop(&timer);

    DebugInfo("Resource Image '%.*s' loaded in %f seconds.", resourceImage->name.length, resourceImage->name.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

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
