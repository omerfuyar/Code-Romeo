#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"

#include "glad/glad.h"
#include "stb/stb_image.h"
#include "cgltf/cgltf.h"
#include "cglm/cglm.h"

#pragma region Source Only

#pragma endregion Source Only

#pragma region ResourceText

RJ_ResultWarn ResourceText_Create(ResourceText **retResource, StringView file)
{
    ResourceText *resource = *retResource;

    RJ_ReturnAllocate(ResourceText, resource, 1);

    *retResource = resource;

    resource->file = scc(file);

    // RJ_Size pathCount = 0;
    // String pathBuffer[RJ_TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(RJ_PATH_DELIMETER_STR), &pathCount, pathBuffer, RJ_TEMP_BUFFER_SIZE / 32);
    //
    // for (RJ_Size i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
    swn(&fullPath);

    RJ_Size lineCount = 0;
    int character = 0;

    FILE *fileHandle = NULL;
    RJ_ReturnFileOpen(fileHandle, fullPath.characters, "r",
                      String_Destroy(&fullPath);
                      free(resource););

    while ((character = fgetc(fileHandle)) != EOF)
    {
        if (character == '\n')
        {
            lineCount++;
        }
    }

    resource->lineCount = lineCount + 1;

    rewind(fileHandle);

    // on heap because the buffer is too large for stack
    char *dataBuffer = NULL;
    RJ_ReturnAllocate(char, dataBuffer, resource->lineCount *RESOURCE_FILE_LINE_MAX_CHAR_COUNT,
                      String_Destroy(&fullPath);
                      free(resource););

    dataBuffer[0] = '\0';

    char lineBuffer[RESOURCE_FILE_LINE_MAX_CHAR_COUNT] = {0};

    RJ_Size dataIndex = 0;

    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, fileHandle))
    {
        RJ_Size lineLength = (RJ_Size)strlen(lineBuffer);
        memcpy(dataBuffer + dataIndex, lineBuffer, lineLength);
        dataIndex += lineLength;
    }
    fclose(fileHandle);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopySafe(dataBuffer, dataIndex);

    free(dataBuffer);

    String_Destroy(&fullPath);

    RJ_DebugInfo("Resource text '%s' loaded successfully.", resource->file.characters);

    return RJ_OK;
}

void ResourceText_Destroy(ResourceText *resource)
{
    RJ_DebugAssertNullPointerCheck(resource);
    RJ_DebugAssertNullPointerCheck(resource->file.characters);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(resource->file, tempTitle, sizeof(tempTitle));

    String_Destroy(&resource->file);
    String_Destroy(&resource->data);
    resource->lineCount = 0;

    if (resource != NULL)
    {
        free(resource);
        resource = NULL;
    }

    RJ_DebugInfo("Resource Text '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceText

#pragma region ResourceImage

RJ_ResultWarn ResourceImage_Create(ResourceImage **retResourceImage, StringView file)
{
    ResourceImage *resourceImage = *retResourceImage;

    RJ_ReturnAllocate(ResourceImage, resourceImage, 1);

    *retResourceImage = resourceImage;

    resourceImage->file = scc(file);

    String fullPath = scc(resourceImage->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
    swn(&fullPath);

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 4);
    if (resourceImage->data == NULL)
    {
        String_Destroy(&fullPath);
        free(resourceImage);
        RJ_DebugWarning("Failed to load image data from file '%s'. STB Error: %s", fullPath.characters, stbi_failure_reason());
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    RJ_DebugInfo("Resource Image '%s' loaded successfully.", resourceImage->file.characters);

    return RJ_OK;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    RJ_DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(resourceImage->file, tempTitle, sizeof(tempTitle));

    String_Destroy(&resourceImage->file);
    resourceImage->channels = 0;
    resourceImage->size = Vector2Int_New(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    RJ_DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceImage

#pragma region ResourceModel

RJ_ResultWarn ResourceModel_Create(ResourceModel **retResourceModel, StringView fileName)
{
}

void ResourceModel_Destroy(ResourceModel *model)
{
}

#pragma endregion ResourceModel
