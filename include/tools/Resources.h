#pragma once

#include "Global.h"
#include "utilities/String.h"

#define RESOURCE_FILE_LINE_MAX_TOKEN_COUNT 64
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 512
#define RESOURCE_FILE_MAX_LINE_COUNT 2048

#if PLATFORM_WINDOWS
#define PATH_DELIMETER "\\"
#elif PLATFORM_UNIX
#define PATH_DELIMETER "/"
#endif

#define RESOURCE_PATH "resources" PATH_DELIMETER

typedef struct Resource
{
    String title;
    String path;
    String data;
} Resource;

/// @brief Initializer for resources. Initializes file path and other settings. Should be called before any resource function.
void Resource_Initialize();

/// @brief Creates a new resource. Looks for a resources folder in executable directory.
/// @param title The title of the resource.
/// @param path The file path of the resource in resources folder.
/// @return The created resource.
Resource Resource_Create(String title, String path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void Resource_Destroy(Resource *resource);

/// @brief Gets the file directory of the current executable
/// @return A reference string which includes the executable path. Does not necessary to destroy later.
String Resource_GetExePath();
