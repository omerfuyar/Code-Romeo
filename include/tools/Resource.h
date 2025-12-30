#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

/// @brief Maximum character count for a single line in a resource file.
#define RESOURCE_FILE_LINE_MAX_CHAR_COUNT 256
/// @brief Maximum token count for a single line in a resource file.
#define RESOURCE_FILE_LINE_MAX_TOKEN_COUNT 8
/// @brief Path to the resources folder relative to the executable.
#define RESOURCE_PATH "resources/"

#pragma region Typedefs

/// @brief Resource representation for text files.
typedef struct ResourceText
{
    String file;
    String data;
    RJ_Size lineCount;
} ResourceText;

/// @brief Resource representation for image files.
typedef struct ResourceImage
{
    String file;

    void *data;
    Vector2Int size;
    int channels;
} ResourceImage;

#pragma region Helpers

/// @brief Should not be used by user.
typedef struct Resource_Matrix4
{
    alignas(16) float m[4][4];
} Resource_Matrix4;

/// @brief Should not be used by user.
typedef RJ_Size ResourceTextureHandle;

/// @brief Should not be used by user.
typedef struct ResourceTexture
{
    String name;
    RJ_Size index;

    ResourceTextureHandle handle;
    ResourceImage *image;
} ResourceTexture;

/// @brief Should not be used by user.
typedef struct ResourceMaterial
{
    String name;
    RJ_Size index;

    Vector3 ambientColor;
    Vector3 diffuseColor;
    Vector3 specularColor;
    Vector3 emissiveColor;
    ResourceTexture *diffuseMap;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
} ResourceMaterial;

/// @brief Should not be used by user.
typedef RJ_Size ResourceMeshIndex;

/// @brief Should not be used by user.
typedef struct ResourceMeshVertex
{
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
} ResourceMeshVertex;

/// @brief Should not be used by user.
typedef struct ResourceMesh
{
    ResourceMaterial *material;
    ListArray indices; // ResourceMeshIndex
} ResourceMesh;

#pragma endregion Helpers

/// @brief Resource representation for 3D models.
typedef struct ResourceModel
{
    String file;
    RJ_Size index;

    ListArray vertices; // ResourceMeshVertex
    ListArray meshes;   // ResourceMesh
} ResourceModel;

#pragma endregion Typedefs

#pragma region ResourceText

/// @brief Creates a new resource.
/// @param file The file path of the resource in resources folder. Including file name. (e.g. "shaders/vertex.glsl").
/// @return Pointer to the created resource.
ResourceText *ResourceText_Create(StringView file);

/// @brief Destroys a resource.
/// @param resource The resource to destroy.
void ResourceText_Destroy(ResourceText *resource);

#pragma endregion ResourceText

#pragma region ResourceImage

/// @brief Creates a new resource image. Looks for a resources folder in executable directory.
/// @param file The file path of the resource image in resources folder. Including file name. (e.g. "images/texture.png").
/// @return Pointer to the created resource image.
ResourceImage *ResourceImage_Create(StringView file);

/// @brief Destroys a resource image.
/// @param resourceImage The resource image to destroy.
void ResourceImage_Destroy(ResourceImage *resourceImage);

#pragma endregion ResourceImage

#pragma region ResourceModel

/// @brief Creates a new resource model. Looks for a resources folder in executable directory.
/// @param fileName The file path of the resource model in resources folder. Including file name. (e.g. "models/model.mdl").
/// @param transformOffset Transform offset to apply to the model's vertices. Should be in position, rotation, and scale order. Leave NULL if not needed.
/// @return Pointer to the created resource model.
ResourceModel *ResourceModel_GetOrCreate(StringView fileName, Vector3 *transformOffset);

/// @brief Destroys a resource model and frees all associated memory.
/// @param model The resource model to destroy.
void ResourceModel_Destroy(ResourceModel *model);

#pragma endregion ResourceModel
