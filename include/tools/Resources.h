#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

/// @brief Maximum character count for a single line in a resource file.
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 256
/// @brief Path to the resources folder relative to the executable.
#define RESOURCE_PATH "resources" RJGLOBAL_PATH_DELIMETER_STR

#pragma region Typedefs

/// @brief Resource representation for text files.
typedef struct ResourceText
{
    String name;
    String path;
    String data;
    size_t lineCount;
} ResourceText;

/// @brief Resource representation for image files.
typedef struct ResourceImage
{
    String name;
    String path;
    void *data;
    Vector2Int size;
    int channels;
} ResourceImage;

#pragma endregion Typedefs

#pragma region ResourceText

/// @brief Creates a new resource.
/// @param name The name of the resource file. (e.g. "vertex.glsl")
/// @param path The file path of the resource in resources folder. Excluding file name. (e.g. "shaders/").
/// @return Pointer to the created resource.
ResourceText *ResourceText_Create(StringView name, StringView path);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void ResourceText_Destroy(ResourceText *resource);

#pragma endregion ResourceText

#pragma region ResourceImage

/// @brief Creates a new resource image. Looks for a resources folder in executable directory.
/// @param name The name of the resource image. (e.g. "texture.png")
/// @param path The file path of the resource image in resources folder. Excluding file name. (e.g. "images/").
/// @return Pointer to the created resource image.
ResourceImage *ResourceImage_Create(StringView name, StringView path);

/// @brief Destroys a resource image.
/// @param resourceImage The resource image to destroy.
void ResourceImage_Destroy(ResourceImage *resourceImage);

#pragma endregion ResourceImage
