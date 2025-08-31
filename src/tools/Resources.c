#include "tools/Resources.h"
#include "utilities/Timer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Resource *Resource_Create(String title, String path)
{
    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);

    Resource *resource = (Resource *)malloc(sizeof(Resource));
    DebugAssertNullPointerCheck(resource);

    resource->title = title;
    resource->path = String_CreateCopy(path.characters);
    String_ConcatBegin(&resource->path, scl(RESOURCE_PATH));
    String_ConcatBegin(&resource->path, scl(GetExecutablePath()));

    // malloc because the buffer is too large for stack
    char *dataBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT * RESOURCE_FILE_MAX_LINE_COUNT);
    DebugAssertNullPointerCheck(dataBuffer);
    char *lineBuffer = (char *)malloc(RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    DebugAssertNullPointerCheck(lineBuffer);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    size_t dataIndex = 0;

    FILE *file = NULL;
    DebugAssertFileOpenCheck(file, resource->path.characters, "r");

    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, file))
    {
        size_t lineLength = strlen(lineBuffer);
        MemoryCopy(dataBuffer + dataIndex, lineLength * sizeof(char), lineBuffer);
        dataIndex += lineLength;
    }
    fclose(file);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopy(dataBuffer);

    free(dataBuffer);
    free(lineBuffer);

    Timer_Stop(&timer);

    DebugInfo("Resource '%s' loaded in %f seconds.", resource->title.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

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

    free(resource);
    resource = NULL;

    DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}

ResourceImage *ResourceImage_Create(String title, String path)
{
    Timer timer = Timer_Create("Resource Loading");
    Timer_Start(&timer);

    ResourceImage *resourceImage = (ResourceImage *)malloc(sizeof(ResourceImage));
    DebugAssertNullPointerCheck(resourceImage);

    resourceImage->title = title;
    resourceImage->path = String_CreateCopy(path.characters);
    String_ConcatBegin(&resourceImage->path, scl(RESOURCE_PATH));
    String_ConcatBegin(&resourceImage->path, scl(GetExecutablePath()));

    // stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(resourceImage->path.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 0);
    DebugAssertNullPointerCheck(data);

    glGenTextures(1, &resourceImage->handle);
    glBindTexture(GL_TEXTURE_2D, resourceImage->handle);

    // todo
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = GL_RGB;
    if (resourceImage->channels == 1)
        format = GL_RED;
    else if (resourceImage->channels == 3)
        format = GL_RGB;
    else if (resourceImage->channels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, resourceImage->size.x, resourceImage->size.y, 0, format, GL_UNSIGNED_BYTE, data);
    // glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data); // Free the image memory

    Timer_Stop(&timer);

    DebugInfo("Resource Image '%s' loaded in %f seconds.", resourceImage->title.characters, (double)Timer_GetElapsedNanoseconds(timer) / 1000000000.0);

    return resourceImage;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, TEMP_BUFFER_SIZE, resourceImage->title.characters);

    String_Destroy(&resourceImage->title);
    String_Destroy(&resourceImage->path);
    resourceImage->channels = 0;
    resourceImage->size = NewVector2Int(0, 0);

    glDeleteTextures(1, &resourceImage->handle);
    resourceImage->handle = 0;

    free(resourceImage);
    resourceImage = NULL;

    DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}
