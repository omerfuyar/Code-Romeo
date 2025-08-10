#pragma once

#include "Global.h"
#include "utilities/String.h"

#define RESOURCE_FILE_LINE_MAX_TOKEN_COUNT 64
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 512
#define RESOURCE_FILE_MAX_LINE_COUNT 2048
#define RESOURCE_DIRECTORY "resources/"

typedef struct Resource
{
    String title;
    String path;
    String data;
} Resource;

/// @brief Creates a new resource. Looks for a resources folder in executable directory.
/// @param title The title of the resource.
/// @param path The file path of the resource in resources folder.
/// @return The created resource.
Resource Resource_Create(String title, String path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void Resource_Destroy(Resource *resource);
