#pragma once

#include "Global.h"

#include "utilities/String.h"
#include "utilities/Vectors.h"

#define RESOURCE_FILE_LINE_MAX_TOKEN_COUNT 64
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 512
#define RESOURCE_FILE_MAX_LINE_COUNT 32768

#define RESOURCE_PATH "resources" PATH_DELIMETER_STR

typedef struct Resource
{
    String title;
    String path;
    String data;
} Resource;

typedef unsigned int ResoureImageHandle;

typedef struct ResoureImage
{
    String title;
    String path;
    Vector2Int size;
    ResoureImageHandle handle;
    int channels;
} ResourceImage;

/// @brief Creates a new resource.
/// @param title The title of the resource.
/// @param path The file path of the resource in resources folder.
/// @return Pointer to the created resource.
Resource *Resource_Create(String title, String path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void Resource_Destroy(Resource *resource);

/// @brief Creates a new resource image and assigns its handle to OpenGL texture. Looks for a resources folder in executable directory.
/// @param title The title of the resource image.
/// @param path The file path of the resource image in resources folder.
/// @return Pointer to the created resource image.
ResourceImage *ResourceImage_Create(String title, String path);

/// @brief Destroys a resource image.
/// @param resourceImage The resource image to destroy.
void ResourceImage_Destroy(ResourceImage *resourceImage);
