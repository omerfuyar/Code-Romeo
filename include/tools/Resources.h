#pragma once

#include "Global.h"

#include "utilities/String.h"
#include "utilities/Vectors.h"

#define RESOURCE_FILE_LINE_MAX_TOKEN_COUNT 64
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 256

#define RESOURCE_PATH "resources" PATH_DELIMETER_STR

typedef struct Resource
{
    String name;
    String path;
    String data;
    size_t lineCount;
} Resource;

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
Resource *Resource_Create(String name, String path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void Resource_Destroy(Resource *resource);

/// @brief Creates a new resource image. Looks for a resources folder in executable directory.
/// @param name The name of the resource image. (e.g. "texture.png")
/// @param path The file path of the resource image in resources folder. Excluding file name. (e.g. "images/").
/// @return Pointer to the created resource image.
ResourceImage *ResourceImage_Create(String name, String path);

/// @brief Destroys a resource image.
/// @param resourceImage The resource image to destroy.
void ResourceImage_Destroy(ResourceImage *resourceImage);
