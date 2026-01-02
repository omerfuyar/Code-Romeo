#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"

#include "glad/glad.h"

#if RJ_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJ_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "cglm/cglm.h"

#if RJ_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJ_COMPILER_MSVC
#pragma warning(pop)
#endif

#pragma region Source Only

ListLinked RESOURCE_MODEL_POOL = {0};
ListLinked RESOURCE_MATERIAL_POOL = {0};
ListLinked RESOURCE_TEXTURE_POOL = {0};

#pragma region ResourceTexture

static RJ_Result ResourceTexture_GetByNameOrCreate(ResourceTexture **retTexture, StringView name, StringView filePathInResources)
{
    for (RJ_Size textureIndex = 0; textureIndex < RESOURCE_TEXTURE_POOL.count; textureIndex++)
    {
        ResourceTexture *texture = (ResourceTexture *)ListLinked_Get(&RESOURCE_TEXTURE_POOL, textureIndex);

        if (String_AreSame(scv(texture->name), name))
        {
            *retTexture = texture;
            return RJ_OK;
        }
    }

    ResourceTexture *texture = (ResourceTexture *)ListLinked_Add(&RESOURCE_TEXTURE_POOL, NULL);
    *retTexture = texture;

    texture->index = RESOURCE_TEXTURE_POOL.count - 1;
    texture->name = scc(name);

    String tempFilePath = scc(filePathInResources);
    String_ConcatEnd(&tempFilePath, scv(texture->name));

    RJ_Result result = ResourceImage_Create(&texture->image, scv(tempFilePath));
    String_Destroy(&tempFilePath);
    if (result != RJ_OK)
    {
        ListLinked_RemoveAtIndex(&RESOURCE_TEXTURE_POOL, texture->index);
        String_Destroy(&texture->name);
        RJ_DebugWarning("Failed to create resource image for texture '%s'.", texture->name.characters);
        return result;
    }

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = 0;

    switch (texture->image->channels)
    {
    case 4:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 2:
        format = GL_RG;
        break;
    case 1:
        format = GL_RED;
        break;
    default:
        RJ_DebugError("Unsupported number of channels (%d) for texture '%s'.", texture->image->channels, texture->name.characters);
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, texture->image->size.x, texture->image->size.y, 0, format, GL_UNSIGNED_BYTE, texture->image->data);
    glBindTexture(GL_TEXTURE_2D, 0);

    RJ_DebugInfo("Texture '%s' created successfully.", texture->name.characters);

    return RJ_OK;
}

static void ResourceTexture_Destroy(ResourceTexture *texture)
{
    RJ_DebugAssertNullPointerCheck(texture);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(texture->name, tempTitle, sizeof(tempTitle));

    String_Destroy(&texture->name);
    ResourceImage_Destroy(texture->image);
    glDeleteTextures(1, &texture->handle);
    ListLinked_RemoveAtIndex(&RESOURCE_TEXTURE_POOL, texture->index);

    RJ_DebugInfo("Resource Texture '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceTexture

#pragma region ResourceMaterial

static RJ_Result ResourceMaterial_GetByName(ResourceMaterial **retMaterial, StringView name)
{
    for (RJ_Size materialIndex = 0; materialIndex < RESOURCE_MATERIAL_POOL.count; materialIndex++)
    {
        ResourceMaterial *material = (ResourceMaterial *)ListLinked_Get(&RESOURCE_MATERIAL_POOL, materialIndex);

        if (String_AreSame(scv(material->name), name))
        {
            *retMaterial = material;
            return RJ_OK;
        }
    }

    // RJ_DebugWarning("Resource Material '%.*s' not found in the global material pool.", (int)name.length, name.characters);
    return RJ_ERROR_NOT_FOUND;
}

static RJ_Result ResourceMaterial_AddFromFileIfNew(StringView matFile, StringView resourcePathInResources)
{
    String tempFilePath = scc(resourcePathInResources);
    String_ConcatEnd(&tempFilePath, matFile);

    ResourceText *matFileResource = NULL;
    RJ_Result result = ResourceText_Create(&matFileResource, scv(tempFilePath));
    String_Destroy(&tempFilePath);

    if (result != RJ_OK)
    {
        RJ_DebugWarning("Failed to create resource text for material file '%s'.", tempFilePath.characters);
        return result;
    }

    RJ_Size matLineCount = 0;

    StringView *matLines = NULL;
    RJ_ReturnAllocate(StringView, matLines, matFileResource->lineCount,
                      ResourceText_Destroy(matFileResource););

    RJ_Size matLineTokenCount = 0;
    StringView matLineTokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};

    ResourceMaterial *currentMaterial = NULL;

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strNEWMTL = scl("newmtl"); // new material
    StringView strNS = scl("Ns");         // specular exponent
    StringView strKA = scl("Ka");         // ambient color
    StringView strKD = scl("Kd");         // diffuse color
    StringView strKS = scl("Ks");         // specular color
    StringView strKE = scl("Ke");         // emissive color
    StringView strNI = scl("Ni");         // refraction index
    StringView strD = scl("d");           // dissolve
    StringView strILLNUM = scl("illum");  // illumination model
    StringView strMAP_KD = scl("map_Kd"); // diffuse map

    String_Tokenize(scv(matFileResource->data), strNewline, &matLineCount, matLines, matFileResource->lineCount);

    for (RJ_Size matLine = 0; matLine < matLineCount; matLine++)
    {
        String_Tokenize(matLines[matLine], strSpace, &matLineTokenCount, matLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        StringView matFirstToken = matLineTokens[0];

        if (String_AreSame(matFirstToken, strNEWMTL))
        {
            result = ResourceMaterial_GetByName(&currentMaterial, matLineTokens[1]);
            if (result == RJ_ERROR_NOT_FOUND)
            {
                currentMaterial = (ResourceMaterial *)ListLinked_Add(&RESOURCE_MATERIAL_POOL, NULL);
                currentMaterial->index = RESOURCE_MATERIAL_POOL.count - 1;
                currentMaterial->name = scc(matLineTokens[1]);

                RJ_DebugInfo("Resource Material '%s' created successfully.", currentMaterial->name.characters);
            }
        }
        else if (String_AreSame(matFirstToken, strNS))
        {
            currentMaterial->specularExponent = String_ToFloat(matLineTokens[1]);
        }
        else if (String_AreSame(matFirstToken, strKA))
        {
            currentMaterial->ambientColor =
                Vector3_New(String_ToFloat(matLineTokens[1]),
                            String_ToFloat(matLineTokens[2]),
                            String_ToFloat(matLineTokens[3]));
        }
        else if (String_AreSame(matFirstToken, strKD))
        {
            currentMaterial->diffuseColor =
                Vector3_New(String_ToFloat(matLineTokens[1]),
                            String_ToFloat(matLineTokens[2]),
                            String_ToFloat(matLineTokens[3]));
        }
        else if (String_AreSame(matFirstToken, strKS))
        {
            currentMaterial->specularColor =
                Vector3_New(String_ToFloat(matLineTokens[1]),
                            String_ToFloat(matLineTokens[2]),
                            String_ToFloat(matLineTokens[3]));
        }
        else if (String_AreSame(matFirstToken, strKE))
        {
            currentMaterial->emissiveColor =
                Vector3_New(String_ToFloat(matLineTokens[1]),
                            String_ToFloat(matLineTokens[2]),
                            String_ToFloat(matLineTokens[3]));
        }
        else if (String_AreSame(matFirstToken, strNI))
        {
            currentMaterial->refractionIndex = String_ToFloat(matLineTokens[1]);
        }
        else if (String_AreSame(matFirstToken, strD))
        {
            currentMaterial->dissolve = String_ToFloat(matLineTokens[1]);
        }
        else if (String_AreSame(matFirstToken, strILLNUM))
        {
            currentMaterial->illuminationModel = String_ToInt(matLineTokens[1]);
        }
        else if (String_AreSame(matFirstToken, strMAP_KD))
        {
            result = ResourceTexture_GetByNameOrCreate(&currentMaterial->diffuseMap, matLineTokens[1], resourcePathInResources);
            if (result != RJ_OK)
            {
                // todo maybe set a default texture
                RJ_DebugWarning("Failed to load diffuse map '%.*s' for material '%s'.",
                                (int)matLineTokens[1].length,
                                matLineTokens[1].characters,
                                currentMaterial->name.characters);
                return result;
            }
        }
    }

    free(matLines);
    ResourceText_Destroy(matFileResource);

    return RJ_OK;
}

static void ResourceMaterial_Destroy(ResourceMaterial *material)
{
    RJ_DebugAssertNullPointerCheck(material);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(material->name, tempTitle, sizeof(tempTitle));

    ResourceTexture_Destroy(material->diffuseMap);
    String_Destroy(&material->name);
    ListLinked_RemoveAtIndex(&RESOURCE_MATERIAL_POOL, material->index);

    RJ_DebugInfo("Resource Material '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceMaterial

#pragma region ResourceMesh

static RJ_Result ResourceMesh_Create(ResourceMesh **retMesh, ResourceModel *model, RJ_Size indexCount, ResourceMaterial *material)
{
    ResourceMesh *mesh = (ResourceMesh *)ListArray_Add(&model->meshes, NULL);

    RJ_Result result = ListArray_Create(&mesh->indices, "Resource Mesh Indices", sizeof(ResourceMeshIndex), indexCount);
    if (result != RJ_OK) // todo make this for all ListArray_Create calls or not
    {
        ListArray_Pop(&model->meshes);
        RJ_DebugWarning("Failed to create indices list for sub-mesh of model '%s'.", model->file.characters);
        return RJ_ERROR_ALLOCATION;
    }

    mesh->material = material;

    *retMesh = mesh;
    return RJ_OK;
}

static void ResourceMesh_Destroy(ResourceMesh *mesh)
{
    RJ_DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->indices);
    mesh->material = NULL;
}

#pragma endregion ResourceMesh

#pragma endregion Source Only

#pragma region ResourceText

RJ_Result ResourceText_Create(ResourceText **retResource, StringView file)
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
#if RJ_PLATFORM == RJ_PLATFORM_WINDOWS
    String_Replace(&fullPath, scv("\\"), scv("/"));
#endif

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

RJ_Result ResourceImage_Create(ResourceImage **retResourceImage, StringView file)
{
    ResourceImage *resourceImage = *retResourceImage;

    RJ_ReturnAllocate(ResourceImage, resourceImage, 1);

    *retResourceImage = resourceImage;

    resourceImage->file = scc(file);

    String fullPath = scc(resourceImage->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
#if RJ_PLATFORM == RJ_PLATFORM_WINDOWS
    String_Replace(&fullPath, scv("\\"), scv("/"));
#endif

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

static void ProcessFaceVertex(StringView faceToken, ResourceModel *model, ResourceMesh *currentMesh, ListArray *globalVertexUvPool, ListArray *globalVertexNormalPool)
{
    StringView faceData[3] = {0}; // v/vt/vn
    RJ_Size faceDataCount;
    String_Tokenize(faceToken, scl("/"), &faceDataCount, faceData, 3);

    RJ_DebugAssert(faceDataCount == 3, "Face vertex data '%.*s' is invalid. Expected format 'v/vt/vn'.", (int)faceToken.length, faceToken.characters);

    int createdPositionIndex = String_ToInt(faceData[0]);
    RJ_Size positionIndex = createdPositionIndex < 0 ? (RJ_Size)((int)model->vertices.count + createdPositionIndex) : (RJ_Size)createdPositionIndex - 1;

    ResourceMeshVertex *vertex = (ResourceMeshVertex *)ListArray_Get(&model->vertices, (RJ_Size)positionIndex);

    int createdUVIndex = String_ToInt(faceData[1]);
    RJ_Size uvIndex = createdUVIndex < 0 ? (RJ_Size)((int)globalVertexUvPool->count + createdUVIndex) : (RJ_Size)createdUVIndex - 1;
    vertex->uv = *(Vector2 *)ListArray_Get(globalVertexUvPool, (RJ_Size)uvIndex);

    int createdNormalIndex = String_ToInt(faceData[2]);
    RJ_Size normalIndex = createdNormalIndex < 0 ? (RJ_Size)((int)globalVertexNormalPool->count + createdNormalIndex) : (RJ_Size)createdNormalIndex - 1;
    vertex->normal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (RJ_Size)normalIndex);

    ListArray_Add(&currentMesh->indices, &positionIndex);
}

RJ_Result ResourceModel_GetOrCreate(ResourceModel **retResourceModel, StringView fileName, Vector3 *transformOffset)
{
    if (RESOURCE_TEXTURE_POOL.head == NULL && RESOURCE_MATERIAL_POOL.head == NULL)
    {
        RESOURCE_MODEL_POOL = ListLinked_Create("Resource Model", sizeof(ResourceModel));
        RESOURCE_TEXTURE_POOL = ListLinked_Create("Resource Texture", sizeof(ResourceTexture));
        RESOURCE_MATERIAL_POOL = ListLinked_Create("Resource Material", sizeof(ResourceMaterial));
    }

    for (RJ_Size modelIndex = 0; modelIndex < RESOURCE_MODEL_POOL.count; modelIndex++)
    {
        ResourceModel *model = (ResourceModel *)ListLinked_Get(&RESOURCE_MODEL_POOL, modelIndex);

        if (String_AreSame(scv(model->file), fileName))
        {
            *retResourceModel = model;
            return RJ_OK;
        }
    }

    ResourceModel *model = (ResourceModel *)ListLinked_Add(&RESOURCE_MODEL_POOL, NULL);

    *retResourceModel = model;

    model->file = scc(fileName);
    model->index = RESOURCE_MODEL_POOL.count - 1;

    Resource_Matrix4 offsetMatrix = {0};

    if (transformOffset != NULL)
    {
        Vector3 *positionOffset = transformOffset;
        Vector3 *rotationOffset = transformOffset + 1;
        Vector3 *scaleOffset = transformOffset + 2;

        glm_mat4_identity((vec4 *)offsetMatrix.m);

        glm_translate((vec4 *)offsetMatrix.m, (float *)&(vec3){positionOffset->x, positionOffset->y, positionOffset->z});

        glm_rotate((vec4 *)offsetMatrix.m, rotationOffset->x, (float *)&(vec3){1, 0, 0});
        glm_rotate((vec4 *)offsetMatrix.m, rotationOffset->y, (float *)&(vec3){0, 1, 0});
        glm_rotate((vec4 *)offsetMatrix.m, rotationOffset->z, (float *)&(vec3){0, 0, 1});

        glm_scale((vec4 *)offsetMatrix.m, (float *)&(vec3){scaleOffset->x, scaleOffset->y, scaleOffset->z});
    }

    RJ_Size meshCount = 0;
    RJ_Size totalVertexPositionCount = 0;
    RJ_Size totalVertexNormalCount = 0;
    RJ_Size totalVertexUvCount = 0;

    ResourceText *mdlFileResource = NULL;
    RJ_Result result = ResourceText_Create(&mdlFileResource, fileName);
    if (result != RJ_OK)
    {
        RJ_DebugWarning("Failed to create resource text for model file '%.*s'.", (int)fileName.length, fileName.characters);
        return result;
    }

    RJ_Size mdlLineCount = 0;
    StringView *mdlLines = NULL;

    RJ_ReturnAllocate(StringView, mdlLines, mdlFileResource->lineCount,
                      ResourceText_Destroy(mdlFileResource);
                      free(model););

    RJ_Size mdlLineTokenCount = 0;
    StringView mdlLineTokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strMAT = scl("mat");       // material file
    StringView strV = scl("v");           // vertex position
    StringView strVT = scl("vt");         // vertex uv
    StringView strVN = scl("vn");         // vertex normal
    StringView strF = scl("f");           // face
    StringView strO = scl("o");           // mesh
    StringView strUSEMTL = scl("usemtl"); // usemtl

    String_Tokenize(scv(mdlFileResource->data), strNewline, &mdlLineCount, mdlLines, mdlFileResource->lineCount);

    for (RJ_Size i = 0; i < mdlLineCount; i++) // count
    {
        String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_AreSame(firstToken, strV))
        {
            totalVertexPositionCount++;
        }
        else if (String_AreSame(firstToken, strVT))
        {
            totalVertexUvCount++;
        }
        else if (String_AreSame(firstToken, strVN))
        {
            totalVertexNormalCount++;
        }
        else if (String_AreSame(firstToken, strO))
        {
            meshCount++;
        }
    }

    RJ_Size *triangleCounts = NULL;

    RJ_ReturnAllocate(RJ_Size, triangleCounts, meshCount,
                      ResourceText_Destroy(mdlFileResource);
                      free(mdlLines);
                      free(model););

    RJ_Size tempMeshIndex = 0;
    for (RJ_Size i = 0; i < mdlLineCount && tempMeshIndex < meshCount + 1; i++) // count faces per mesh
    {
        String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_AreSame(firstToken, strF))
        {
            triangleCounts[tempMeshIndex - 1] += mdlLineTokenCount - 3;
        }
        else if (String_AreSame(firstToken, strO))
        {
            tempMeshIndex++;
        }
        else if (String_AreSame(firstToken, strMAT))
        {
            String tempFilePath = scc(fileName);

            while (tempFilePath.length > 0 && tempFilePath.characters[tempFilePath.length - 1] != (RJ_PLATFORM == RJ_PLATFORM_WINDOWS ? '\\' : '/'))
            {
                tempFilePath.length--;
            }

            RJ_DebugAssert(tempFilePath.length > 0, "Resource model file path '%s' is empty.", fileName.characters);

            result = ResourceMaterial_AddFromFileIfNew(mdlLineTokens[1], scv(tempFilePath));
            if (result != RJ_OK)
            {
                // todo maybe set a default material
                RJ_DebugWarning("Failed to load material file '%.*s' for resource model '%s'.",
                                (int)mdlLineTokens[1].length,
                                mdlLineTokens[1].characters,
                                fileName.characters);
                return result;
            }

            String_Destroy(&tempFilePath);
        }
    }

    ListArray_Create(&model->vertices, "Resource Model Vertices", sizeof(ResourceMeshVertex), totalVertexPositionCount);
    ListArray_Create(&model->meshes, "Resource Model Meshes", sizeof(ResourceMesh), meshCount);

    ListArray globalVertexNormalPool = {0};

    if (totalVertexNormalCount != 0)
    {
        ListArray_Create(&globalVertexNormalPool, "Vector3", sizeof(Vector3), totalVertexNormalCount);
    }

    ListArray globalVertexUvPool = {0};

    if (totalVertexUvCount != 0)
    {
        ListArray_Create(&globalVertexUvPool, "Vector2", sizeof(Vector2), totalVertexUvCount);
    }

    for (RJ_Size i = 0; i < mdlLineCount; i++) // create global pools
    {
        String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_AreSame(firstToken, strV)) // v -7.579129 4.591946 4.850700
        {
            vec3 vertexPosition;

            if (transformOffset != NULL)
            {
                glm_mat4_mulv3((vec4 *)&offsetMatrix,
                               (float *)&(vec3){String_ToFloat(mdlLineTokens[1]),
                                                String_ToFloat(mdlLineTokens[2]),
                                                String_ToFloat(mdlLineTokens[3])},
                               0.0f,
                               (float *)&vertexPosition);
            }
            else
            {
                vertexPosition[0] = String_ToFloat(mdlLineTokens[1]);
                vertexPosition[1] = String_ToFloat(mdlLineTokens[2]);
                vertexPosition[2] = String_ToFloat(mdlLineTokens[3]);
            }

            ResourceMeshVertex createdVertex = {0};
            createdVertex.position = Vector3_New(vertexPosition[0], vertexPosition[1], vertexPosition[2]);

            ListArray_Add(&model->vertices, &createdVertex);
        }
        else if (String_AreSame(firstToken, strVT)) // vt 0.073128 0.431854
        {
            Vector2 vertexUv =
                Vector2_New(String_ToFloat(mdlLineTokens[1]),
                            String_ToFloat(mdlLineTokens[2]));

            ListArray_Add(&globalVertexUvPool, &vertexUv);
        }
        else if (String_AreSame(firstToken, strVN)) // vn -0.0233 0.1253 -0.9918
        {
            vec3 vertexNormal;

            if (transformOffset != NULL)
            {
                glm_mat4_mulv3((vec4 *)&offsetMatrix,
                               (float *)&(vec3){String_ToFloat(mdlLineTokens[1]),
                                                String_ToFloat(mdlLineTokens[2]),
                                                String_ToFloat(mdlLineTokens[3])},
                               0.0f,
                               (float *)&vertexNormal);
            }
            else
            {
                vertexNormal[0] = String_ToFloat(mdlLineTokens[1]);
                vertexNormal[1] = String_ToFloat(mdlLineTokens[2]);
                vertexNormal[2] = String_ToFloat(mdlLineTokens[3]);
            }

            ListArray_Add(&globalVertexNormalPool, &vertexNormal);
        }
    }

    ResourceMesh *currentMesh = NULL;
    ResourceMaterial *currentMaterial = NULL;

    for (RJ_Size i = 0; i < mdlLineCount; i++) // create meshes and fill them with indices
    {
        String_Tokenize(mdlLines[i], scl(" "), &mdlLineTokenCount, mdlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_AreSame(firstToken, strF))
        {
            for (RJ_Size j = 3; j < mdlLineTokenCount; j++)
            {
                ProcessFaceVertex(mdlLineTokens[1], model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(mdlLineTokens[j - 1], model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(mdlLineTokens[j], model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
            }
        }
        else if (String_AreSame(firstToken, strO))
        {
            result = ResourceMesh_Create(&currentMesh, model, triangleCounts[model->meshes.count] * 3, currentMaterial);
            if (result != RJ_OK)
            {
                RJ_DebugWarning("Failed to create mesh '%.*s' for model '%s'.",
                                (int)mdlLineTokens[1].length,
                                mdlLineTokens[1].characters,
                                fileName.characters);
                return result;
            }
        }
        else if (String_AreSame(firstToken, strUSEMTL)) // use material and create object
        {
            result = ResourceMaterial_GetByName(&currentMaterial, mdlLineTokens[1]);

            if (result != RJ_OK)
            {
                // todo maybe set a default material
                RJ_DebugWarning("Material '%.*s' not found for model '%s'.",
                                (int)mdlLineTokens[1].length,
                                mdlLineTokens[1].characters,
                                fileName.characters);
                return result;
            }
        }
    }

    if (globalVertexNormalPool.data != NULL)
    {
        ListArray_Destroy(&globalVertexNormalPool);
    }

    if (globalVertexUvPool.data != NULL)
    {
        ListArray_Destroy(&globalVertexUvPool);
    }

    free(mdlLines);
    free(triangleCounts);
    ResourceText_Destroy(mdlFileResource);

    RJ_DebugInfo("Resource Model '%s' loaded successfully with %u meshes.", fileName.characters, model->meshes.count);

    return RJ_OK;
}

void ResourceModel_Destroy(ResourceModel *model)
{
    RJ_DebugAssertNullPointerCheck(model);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(model->file, tempTitle, sizeof(tempTitle));

    String_Destroy(&model->file);

    ListArray_Destroy(&model->vertices);

    for (RJ_Size i = model->meshes.count - 1; i > 0; i--)
    {
        ResourceMesh *mesh = (ResourceMesh *)ListArray_Get(&model->meshes, i);
        ResourceMesh_Destroy(mesh);
    }

    ListArray_Destroy(&model->meshes);

    ListLinked_RemoveAtIndex(&RESOURCE_MODEL_POOL, model->index);

    RJ_DebugInfo("Resource Model '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceModel
