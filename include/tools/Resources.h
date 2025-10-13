#pragma once

#include "Global.h"

#include "utilities/String.h"
#include "utilities/Vectors.h"

#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 256

#define RESOURCE_PATH "resources" PATH_DELIMETER_STR

typedef struct ResourceText
{
    String name;
    String path;
    String data;
    size_t lineCount;
} ResourceText;

typedef unsigned int ResourceImageHandle;

typedef struct ResoureImage
{
    String name;
    String path;
    void *data;
    Vector2Int size;
    int channels;
} ResourceImage;

/// @brief Creates a new resource.
/// @param name The name of the resource file. (e.g. "vertex.glsl")
/// @param path The file path of the resource in resources folder. Excluding file name. (e.g. "shaders/").
/// @return Pointer to the created resource.
ResourceText *ResourceText_Create(StringView name, StringView path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void ResourceText_Destroy(ResourceText *resource);

/// @brief Creates a new resource image. Looks for a resources folder in executable directory.
/// @param name The name of the resource image. (e.g. "texture.png")
/// @param path The file path of the resource image in resources folder. Excluding file name. (e.g. "images/").
/// @return Pointer to the created resource image.
ResourceImage *ResourceImage_Create(StringView name, StringView path);

/// @brief Destroys a resource image.
/// @param resourceImage The resource image to destroy.
void ResourceImage_Destroy(ResourceImage *resourceImage);
