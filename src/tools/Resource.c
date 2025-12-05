#include "tools/Resource.h"
#include "utilities/Timer.h"
#include "utilities/Maths.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(pop)
#endif

#pragma region ResourceText

ResourceText *ResourceText_Create(StringView file)
{

    ResourceText *resource = NULL;
    RJGlobal_DebugAssertAllocationCheck(ResourceText, resource, 1);

    resource->file = scc(file);

    // RJGlobal_Size pathCount = 0;
    // String pathBuffer[RJGLOBAL_TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(RJGLOBAL_PATH_DELIMETER_STR), &pathCount, pathBuffer, RJGLOBAL_TEMP_BUFFER_SIZE / 32);
    //
    // for (RJGlobal_Size i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    RJGlobal_Size lineCount = 0;
    int character = 0;

    FILE *fileHandle = NULL;
    RJGlobal_DebugAssertFileOpenCheck(fileHandle, fullPath.characters, "r");
    while ((character = fgetc(fileHandle)) != EOF)
    {
        if (character == '\n')
        {
            lineCount++;
        }
    }
    fclose(fileHandle);

    resource->lineCount = lineCount;

    // on heap because the buffer is too large for stack
    char *dataBuffer = NULL;
    RJGlobal_DebugAssertAllocationCheck(char, dataBuffer, resource->lineCount *RESOURCE_FILE_LINE_MAX_CHAR_COUNT);
    char *lineBuffer = NULL;
    RJGlobal_DebugAssertAllocationCheck(char, lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT);

    dataBuffer[0] = '\0';
    lineBuffer[0] = '\0';
    RJGlobal_Size dataIndex = 0;

    RJGlobal_DebugAssertFileOpenCheck(fileHandle, fullPath.characters, "r");
    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, fileHandle))
    {
        RJGlobal_Size lineLength = RJGlobal_StringLength(lineBuffer);
        RJGlobal_MemoryCopy(dataBuffer + dataIndex, lineLength, lineBuffer);
        dataIndex += lineLength;
    }
    fclose(fileHandle);

    dataBuffer[dataIndex] = '\0';

    resource->data = String_CreateCopySafe(dataBuffer, dataIndex);

    free(dataBuffer);
    free(lineBuffer);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource '%s' loaded.", resource->file.characters);

    return resource;
}

void ResourceText_Destroy(ResourceText *resource)
{
    RJGlobal_DebugAssertNullPointerCheck(resource);
    RJGlobal_DebugAssertNullPointerCheck(resource->file.characters);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(resource->file, tempTitle);

    String_Destroy(&resource->file);
    String_Destroy(&resource->data);
    resource->lineCount = 0;

    if (resource != NULL)
    {
        free(resource);
        resource = NULL;
    }

    RJGlobal_DebugInfo("Resource '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceText

#pragma region ResourceImage

ResourceImage *ResourceImage_Create(StringView file)
{
    ResourceImage *resourceImage = NULL;
    RJGlobal_DebugAssertAllocationCheck(ResourceImage, resourceImage, 1);

    resourceImage->file = scc(file);

    String fullPath = scc(resourceImage->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    stbi_set_flip_vertically_on_load(true);
    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 4);
    RJGlobal_DebugAssertNullPointerCheck(resourceImage->data);

    String_Destroy(&fullPath);

    RJGlobal_DebugInfo("Resource Image '%s' loaded.", resourceImage->file.characters);

    return resourceImage;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    RJGlobal_DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(resourceImage->file, tempTitle);

    String_Destroy(&resourceImage->file);
    resourceImage->channels = 0;
    resourceImage->size = Vector2Int_New(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    RJGlobal_DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceImage

#pragma region ResourceModel

#pragma region ResourceTexture

ResourceTexture *ResourceTexture_CreateOrGet(StringView name, const void *data, Vector2Int size, int channels)
{
    for (RJGlobal_Size i = 0; i < RMS.shader.textures.count; i++)
    {
        ResourceTexture *currentTexture = (ResourceTexture *)ListArray_Get(&RMS.shader.textures, i);

        if (String_Compare(scv(currentTexture->name), name) == 0)
        {
            RJGlobal_DebugInfo("Texture '%.*s' found in texture pool, reusing it.", (int)name.length, name.characters);
            return currentTexture;
        }
    }

    RJGlobal_DebugAssertNullPointerCheck(data);

    ResourceTexture *texture = (ResourceTexture *)ListArray_Add(&RMS.shader.textures, NULL); // todo fix
    texture->index = RMS.shader.textures.count - 1;
    texture->size = size;
    texture->channels = channels;
    texture->name = scc(name);

    RJGlobal_Size dataSize = (RJGlobal_Size)(size.x * size.y * channels);

    texture->data = malloc(dataSize);
    RJGlobal_DebugAssertNullPointerCheck(texture->data);
    RJGlobal_MemoryCopy(texture->data, dataSize, data);

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = 0;

    switch (texture->channels)
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
        RJGlobal_DebugError("Unsupported number of channels (%d) for texture '%.*s'.", texture->channels, (int)name.length, name.characters);
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, texture->size.x, texture->size.y, 0, format, GL_UNSIGNED_BYTE, texture->data);
    // glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    RJGlobal_DebugInfo("Texture '%.*s' created successfully.", (int)name.length, name.characters);

    return texture;
}

void ResourceTexture_Destroy(ResourceTexture *texture)
{
    RJGlobal_DebugAssertNullPointerCheck(texture);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(texture->name, tempTitle);

    String_Destroy(&texture->name);
    free(texture->data);
    texture->data = NULL;
    texture->size = Vector2Int_NewN(0);
    texture->channels = 0;

    glDeleteTextures(1, &texture->handle);
    texture->handle = 0;

    RJGlobal_DebugInfo("Texture '%s' destroyed successfully.", tempTitle);
}

#pragma endregion Resource Texture

#pragma region ResourceMaterial

ListArray ResourceMaterial_Create(StringView matFile)
{
    RJGlobal_Size materialCount = 0;

    RJGlobal_Size mtlLineCount = 0;

    StringView *mtlLines = NULL;
    RJGlobal_DebugAssertAllocationCheck(StringView, mtlLines, matFileLineCount);

    RJGlobal_Size mtlLineTokenCount = 0;
    StringView mtlLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strNEWMTL = scl("newmtl");
    StringView strNS = scl("Ns");
    StringView strKA = scl("Ka");
    StringView strKE = scl("Ke");
    StringView strKD = scl("Kd");
    StringView strKS = scl("Ks");
    StringView strNI = scl("Ni");
    StringView strD = scl("d");
    StringView strILLNUM = scl("illum");

    String_Tokenize(matFileData, strNewline, &mtlLineCount, mtlLines, matFileLineCount);
    for (RJGlobal_Size j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            materialCount++;
        }
    }

    ListArray materials = ListArray_Create("Resource Material Pointer", sizeof(ResourceMaterial *), materialCount);
    ResourceMaterial *currentMaterial = NULL;

    for (RJGlobal_Size j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            RJGlobal_DebugAssertAllocationCheck(ResourceMaterial, currentMaterial, 1);

            ListArray_Add(&materials, &currentMaterial);

            currentMaterial->name = scc(mtlLineTokens[1]);
            currentMaterial->diffuseMap = ResourceTexture_CreateOrGet(textureName, textureData, textureSize, textureChannels);
        }
        else if (String_Compare(mtlFirstToken, strNS) == 0)
        {
            currentMaterial->specularExponent = String_ToFloat(scv(mtlLineTokens[1]));
        }
        else if (String_Compare(mtlFirstToken, strKA) == 0)
        {
            currentMaterial->ambientColor =
                Vector3_New(String_ToFloat(scv(mtlLineTokens[1])),
                            String_ToFloat(scv(mtlLineTokens[2])),
                            String_ToFloat(scv(mtlLineTokens[3])));
        }
        else if (String_Compare(mtlFirstToken, strKE) == 0)
        {
            currentMaterial->emissiveColor =
                Vector3_New(String_ToFloat(scv(mtlLineTokens[1])),
                            String_ToFloat(scv(mtlLineTokens[2])),
                            String_ToFloat(scv(mtlLineTokens[3])));
        }
        else if (String_Compare(mtlFirstToken, strKD) == 0)
        {
            currentMaterial->diffuseColor =
                Vector3_New(String_ToFloat(scv(mtlLineTokens[1])),
                            String_ToFloat(scv(mtlLineTokens[2])),
                            String_ToFloat(scv(mtlLineTokens[3])));
        }
        else if (String_Compare(mtlFirstToken, strKS) == 0)
        {
            currentMaterial->specularColor =
                Vector3_New(String_ToFloat(scv(mtlLineTokens[1])),
                            String_ToFloat(scv(mtlLineTokens[2])),
                            String_ToFloat(scv(mtlLineTokens[3])));
        }
        else if (String_Compare(mtlFirstToken, strNI) == 0)
        {
            currentMaterial->refractionIndex = String_ToFloat(scv(mtlLineTokens[1]));
        }
        else if (String_Compare(mtlFirstToken, strD) == 0)
        {
            currentMaterial->dissolve = String_ToFloat(scv(mtlLineTokens[1]));
        }
        else if (String_Compare(mtlFirstToken, strILLNUM) == 0)
        {
            currentMaterial->illuminationModel = String_ToInt(scv(mtlLineTokens[1]));
        }
    }

    free(mtlLines);

    return materials;
}

void ResourceMaterial_Destroy(ResourceMaterial *material)
{
    material->ambientColor = Vector3_New(-1.0f, -1.0f, -1.0f);
    material->diffuseColor = Vector3_New(-1.0f, -1.0f, -1.0f);
    material->dissolve = -1.0f;
    material->illuminationModel = -1;
    String_Destroy(&material->name);
    material->refractionIndex = -1.0f;
    material->specularColor = Vector3_New(-1.0f, -1.0f, -1.0f);
    material->specularExponent = -1.0f;

    free(material);

    material = NULL;
}

#pragma endregion ResourceMaterial

#pragma region ResourceMesh

ResourceMesh *ResourceMesh_Create(ResourceModel *model, RJGlobal_Size initialIndexCapacity, ResourceMaterial *material)
{
    ResourceMesh *mesh = (ResourceMesh *)ListLinked_Add(&model->meshes, NULL);

    mesh->indices = ListArray_Create("Resource Mesh Indices", sizeof(ResourceMeshIndex), initialIndexCapacity);
    mesh->material = material;

    return mesh;
}

void ResourceMesh_Destroy(ResourceMesh *mesh)
{
    RJGlobal_DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->indices);
    mesh->material = NULL;
}

#pragma endregion ResourceMesh

ResourceModel *ResourceModel_Create(StringView fileName)
{
}

void ResourceModel_Destroy(ResourceModel *model)
{
}

void ProcessFaceVertex(StringView faceComponent, ResourceModel *model, ResourceMesh *currentMesh, ListArray *globalVertexUvPool, ListArray *globalVertexNormalPool)
{
    StringView faceData[4]; // v/vt/vn/w
    RJGlobal_Size faceDataCount;
    String_Tokenize(faceComponent, scl("/"), &faceDataCount, faceData, 4);

    int createdVertexIndex = String_ToInt(faceData[0]);
    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

    ResourceMeshVertex *vertex = (ResourceMeshVertex *)ListArray_Get(&model->vertices, (RJGlobal_Size)vertexIndex);

    if (faceDataCount > 1 && faceData[1].length != 0)
    {
        int createdUIndex = String_ToInt(faceData[1]);
        unsigned int uvIndex = createdUIndex < 0 ? (unsigned int)globalVertexUvPool->count + (unsigned int)createdUIndex : (unsigned int)createdUIndex - 1;
        vertex->vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (RJGlobal_Size)uvIndex);
    }

    if (faceDataCount > 2 && faceData[2].length != 0)
    {
        int createdNormalIndex = String_ToInt(faceData[2]);
        unsigned int normalIndex = createdNormalIndex < 0 ? (unsigned int)globalVertexNormalPool->count + (unsigned int)createdNormalIndex : (unsigned int)createdNormalIndex - 1;
        vertex->vertexNormal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (RJGlobal_Size)normalIndex);
    }

    ListArray_Add(&currentMesh->indices, &vertexIndex);
}

ResourceModel *ResourceModel_Create(StringView mdlFile, const ListArray *materialPool, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset)
{
    Resource_Matrix4 offsetMatrix = {0};
    TRANSFORM_TO_MODEL_MATRIX(&offsetMatrix, &positionOffset, &rotationOffset, &scaleOffset);

    RJGlobal_Size meshCount = 0;
    String modelName = {0};

    RJGlobal_Size totalVertexPositionCount = 0;
    RJGlobal_Size totalVertexNormalCount = 0;
    RJGlobal_Size totalVertexUvCount = 0;

    ResourceText *rscModel = ResourceText_Create(mdlFile);

    RJGlobal_Size mdlLineCount = 0;
    StringView *mdlLines = NULL;
    RJGlobal_DebugAssertAllocationCheck(StringView, mdlLines, rscModel->lineCount);

    RJGlobal_Size mdlLineTokenCount = 0;
    StringView mdlLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strV = scl("v");
    StringView strF = scl("f");
    StringView strVT = scl("vt");
    StringView strVN = scl("vn");
    StringView strO = scl("o");
    StringView strNEWMDL = scl("newmdl");
    StringView strUSEMTL = scl("usemtl");

    String_Tokenize(scv(rscModel->data), strNewline, &mdlLineCount, mdlLines, rscModel->lineCount);

    for (RJGlobal_Size i = 0; i < mdlLineCount; i++) // count and create materials
    {
        String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_Compare(firstToken, strV) == 0) // v -7.579129 4.591946 4.850700
        {
            totalVertexPositionCount++;
        }
        else if (String_Compare(firstToken, strVT) == 0) // vt 0.073128 0.431854
        {
            totalVertexUvCount++;
        }
        else if (String_Compare(firstToken, strVN) == 0) // vn -0.0233 0.1253 -0.9918
        {
            totalVertexNormalCount++;
        }
        else if (String_Compare(firstToken, strO) == 0)
        {
            meshCount++;
        }
        else if (String_Compare(firstToken, strNEWMDL) == 0)
        {
            modelName = scc(mdlLineTokens[1]);
        }
    }

    RJGlobal_Size *faceCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, faceCounts, meshCount);

    {
        RJGlobal_Size tempMeshIndex = 0;
        for (RJGlobal_Size i = 0; i < mdlLineCount && tempMeshIndex <= meshCount; i++) // count faces
        {
            String_Tokenize(scv(mdlLines[i]), strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

            StringView firstToken = scv(mdlLineTokens[0]);

            if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
            {
                faceCounts[tempMeshIndex - 1] += mdlLineTokenCount == 4 ? 1 : 2;
            }
            else if (String_Compare(firstToken, strO) == 0)
            {
                tempMeshIndex++;
            }
        }
    }

    ResourceModel *model = ResourceModel_CreateEmpty(scv(modelName), meshCount, totalVertexPositionCount);
    String_Destroy(&modelName);

    ListArray globalVertexNormalPool = {0};

    if (totalVertexNormalCount != 0)
    {
        globalVertexNormalPool = ListArray_Create("Vector3", sizeof(Vector3), totalVertexNormalCount);
    }

    ListArray globalVertexUvPool = {0};

    if (totalVertexUvCount != 0)
    {
        globalVertexUvPool = ListArray_Create("Vector2", sizeof(Vector2), totalVertexUvCount);
    }

    for (RJGlobal_Size i = 0; i < mdlLineCount; i++) // create global pools
    {
        String_Tokenize(scv(mdlLines[i]), strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scv(mdlLineTokens[0]);

        if (String_Compare(firstToken, strV) == 0) // v -7.579129 4.591946 4.850700
        {
            vec3 vertexPosition;

            glm_mat4_mulv3((vec4 *)&offsetMatrix,
                           (float *)&(vec3){String_ToFloat(scv(mdlLineTokens[1])),
                                            String_ToFloat(scv(mdlLineTokens[2])),
                                            String_ToFloat(scv(mdlLineTokens[3]))},
                           0.0f,
                           (float *)&vertexPosition);

            ResourceMeshVertex createdVertex;
            createdVertex.vertexPosition = Vector3_New(vertexPosition[0], vertexPosition[1], vertexPosition[2]);

            ListArray_Add(&model->vertices, &createdVertex);
        }
        else if (String_Compare(firstToken, strVT) == 0) // vt 0.073128 0.431854
        {
            Vector2 vertexUv =
                Vector2_New(String_ToFloat(scv(mdlLineTokens[1])),
                            String_ToFloat(scv(mdlLineTokens[2])));

            ListArray_Add(&globalVertexUvPool, &vertexUv);
        }
        else if (String_Compare(firstToken, strVN) == 0) // vn -0.0233 0.1253 -0.9918
        {
            vec3 vertexNormal;

            glm_mat4_mulv3((vec4 *)&offsetMatrix,
                           (float *)&(vec3){String_ToFloat(scv(mdlLineTokens[1])),
                                            String_ToFloat(scv(mdlLineTokens[2])),
                                            String_ToFloat(scv(mdlLineTokens[3]))},
                           0.0f,
                           (float *)&vertexNormal);

            ListArray_Add(&globalVertexNormalPool, &vertexNormal);
        }
    }

    ResourceMesh *currentMesh = NULL;
    ResourceMaterial *currentMaterial = NULL;

    for (RJGlobal_Size i = 0; i < mdlLineCount; i++) // add data to model
    {
        String_Tokenize(scv(mdlLines[i]), scl(" "), &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scv(mdlLineTokens[0]);

        if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            if (mdlLineTokenCount == 4) // 3 vertex face (triangle)
            {
                ProcessFaceVertex(scv(mdlLineTokens[1]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[2]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
            }
            else if (mdlLineTokenCount == 5) // 4 vertex face (quad), triangulate it
            {
                // First triangle: vertices 1, 2, 3
                ProcessFaceVertex(scv(mdlLineTokens[1]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[2]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);

                // Second triangle: vertices 1, 3, 4
                ProcessFaceVertex(scv(mdlLineTokens[1]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[4]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
            }
        }
        else if (String_Compare(firstToken, strUSEMTL) == 0) // use material and create object
        {
            bool materialFound = false;

            for (RJGlobal_Size j = 0; j < materialPool->count; j++)
            {
                ResourceMaterial *material = *(ResourceMaterial **)ListArray_Get(materialPool, j);

                // RJGlobal_DebugInfo("Comparing material '%.*s' with '%.*s'...", material->name.length, material->name.characters, lineTokens[1].length, lineTokens[1].characters);

                if (String_Compare(scv(material->name), scv(mdlLineTokens[1])) == 0)
                {
                    materialFound = true;
                    currentMaterial = material;
                    break;
                }
            }

            RJGlobal_DebugAssert(materialFound, "Material '%.*s' not found in material pool when trying to assign it to mesh in model '%.*s'.", mdlLineTokens[1].length, mdlLineTokens[1].characters, model->name.length, model->name.characters);
        }
        else if (String_Compare(firstToken, strO) == 0)
        {
            currentMesh = ResourceMesh_Create(faceCounts[model->meshes.count] * 3, currentMaterial);
            ListArray_Add(&model->meshes, &currentMesh);
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
    free(faceCounts);
    ResourceText_Destroy(rscModel);

    RJGlobal_DebugInfo("Resource Model '%s' imported successfully with %u child meshes.", model->name.characters, model->meshes.count);

    return model;
}
// todo merge this two
ListArray ResourceModel_CreateFromFile(StringView mdlFile, const ListArray *materialPool)
{
    RJGlobal_Size modelCount = 0;

    ResourceText *rscModel = ResourceText_Create(mdlFile);

    RJGlobal_Size mdlLineCount = 0;
    StringView *mdlLines = NULL;
    RJGlobal_DebugAssertAllocationCheck(StringView, mdlLines, rscModel->lineCount);

    RJGlobal_Size mdlLineTokenCount = 0;
    StringView mdlLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strV = scl("v");
    StringView strF = scl("f");
    StringView strVT = scl("vt");
    StringView strVN = scl("vn");
    StringView strO = scl("o");
    StringView strNEWMDL = scl("newmdl");
    StringView strUSEMTL = scl("usemtl");

    String_Tokenize(scv(rscModel->data), strNewline, &mdlLineCount, mdlLines, rscModel->lineCount);
    for (RJGlobal_Size j = 0; j < mdlLineCount; j++) // count models
    {
        String_Tokenize(scv(mdlLines[j]), strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mdlFirstToken = scv(mdlLineTokens[0]);

        if (String_Compare(mdlFirstToken, strNEWMDL) == 0)
        {
            modelCount++;
        }
    }

    ListArray models = ListArray_Create("Resource Model Pointer", sizeof(ResourceModel *), modelCount);
    ResourceModel *currentModel = NULL;

    RJGlobal_Size *vertexPositionCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, vertexPositionCounts, modelCount);

    RJGlobal_Size *vertexUvCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, vertexUvCounts, modelCount);

    RJGlobal_Size *vertexNormalCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, vertexNormalCounts, modelCount);

    RJGlobal_Size *meshCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, meshCounts, modelCount);

    String *modelNames = NULL;
    RJGlobal_DebugAssertAllocationCheck(String, modelNames, modelCount);

    {
        RJGlobal_Size tempModelIndex = 0;
        for (RJGlobal_Size i = 0; i < mdlLineCount; i++)
        {
            String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

            StringView firstToken = mdlLineTokens[0];

            if (String_Compare(firstToken, strV) == 0)
            {
                vertexPositionCounts[tempModelIndex - 1]++;
            }
            else if (String_Compare(firstToken, strVT) == 0)
            {
                vertexUvCounts[tempModelIndex - 1]++;
            }
            else if (String_Compare(firstToken, strVN) == 0)
            {
                vertexNormalCounts[tempModelIndex - 1]++;
            }
            else if (String_Compare(firstToken, strO) == 0)
            {
                meshCounts[tempModelIndex - 1]++;
            }
            else if (String_Compare(firstToken, strNEWMDL) == 0)
            {
                tempModelIndex++;
                modelNames[tempModelIndex - 1] = scc(mdlLineTokens[1]);
            }
        }
    }

    RJGlobal_Size **faceCounts = NULL;
    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size *, faceCounts, modelCount);

    {
        RJGlobal_Size tempModelIndex = 0;
        RJGlobal_Size tempMeshIndex = 0;

        for (RJGlobal_Size i = 0; i < mdlLineCount; i++)
        {
            String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

            StringView firstToken = mdlLineTokens[0];

            if (String_Compare(firstToken, strF) == 0)
            {
                faceCounts[tempModelIndex - 1][tempMeshIndex - 1] += mdlLineTokenCount == 4 ? 1 : 2;
            }
            else if (String_Compare(firstToken, strO) == 0)
            {
                tempMeshIndex++;
            }
            else if (String_Compare(firstToken, strNEWMDL) == 0)
            {
                tempModelIndex++;
                tempMeshIndex = 0;

                RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, faceCounts[tempModelIndex - 1], meshCounts[tempModelIndex - 1]);
            }
        }
    }

    ResourceMaterial *currentMaterial = NULL;
    ResourceMesh *currentMesh = NULL;
    RJGlobal_Size currentModelIndex = 0;
    RJGlobal_Size currentMeshIndex = 0;
    ListArray currentVertexUvPool = {0};
    ListArray currentVertexNormalPool = {0};

    for (RJGlobal_Size i = 0; i < mdlLineCount; i++)
    {
        String_Tokenize(mdlLines[i], strSpace, &mdlLineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = mdlLineTokens[0];

        if (String_Compare(firstToken, strV) == 0)
        {
            ResourceMeshVertex createdVertex;
            createdVertex.vertexPosition = Vector3_New(String_ToFloat(scv(mdlLineTokens[1])), String_ToFloat(scv(mdlLineTokens[2])), String_ToFloat(scv(mdlLineTokens[3])));

            ListArray_Add(&currentModel->vertices, &createdVertex);
        }
        else if (String_Compare(firstToken, strVT) == 0)
        {
            Vector2 vertexUv =
                Vector2_New(String_ToFloat(scv(mdlLineTokens[1])),
                            String_ToFloat(scv(mdlLineTokens[2])));

            ListArray_Add(&currentVertexUvPool, &vertexUv);
        }
        else if (String_Compare(firstToken, strVN) == 0)
        {
            Vector3 vertexNormal = Vector3_New(String_ToFloat(scv(mdlLineTokens[1])), String_ToFloat(scv(mdlLineTokens[2])), String_ToFloat(scv(mdlLineTokens[3])));

            ListArray_Add(&currentVertexNormalPool, &vertexNormal);
        }
        else if (String_Compare(firstToken, strF) == 0)
        {
            if (mdlLineTokenCount == 4) // 3 vertex face (triangle)
            {
                ProcessFaceVertex(scv(mdlLineTokens[1]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[2]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
            }
            else if (mdlLineTokenCount == 5) // 4 vertex face (quad), triangulate it
            {
                // First triangle: vertices 1, 2, 3
                ProcessFaceVertex(scv(mdlLineTokens[1]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[2]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);

                // Second triangle: vertices 1, 3, 4
                ProcessFaceVertex(scv(mdlLineTokens[1]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[4]), currentModel, currentMesh, &currentVertexUvPool, &currentVertexNormalPool);
            }
        }
        else if (String_Compare(firstToken, strUSEMTL) == 0)
        {
            bool materialFound = false;

            for (RJGlobal_Size j = 0; j < materialPool->count; j++)
            {
                ResourceMaterial *material = *(ResourceMaterial **)ListArray_Get(materialPool, j);

                // RJGlobal_DebugInfo("Comparing material '%.*s' with '%.*s'...", material->name.length, material->name.characters, lineTokens[1].length, lineTokens[1].characters);

                if (String_Compare(scv(material->name), scv(mdlLineTokens[1])) == 0)
                {
                    materialFound = true;
                    currentMaterial = material;
                    break;
                }
            }

            RJGlobal_DebugAssert(materialFound, "Material '%.*s' not found in material pool when trying to assign it to mesh in model '%.*s'.", mdlLineTokens[1].length, mdlLineTokens[1].characters, currentModel->name.length, currentModel->name.characters);
        }
        else if (String_Compare(firstToken, strNEWMDL) == 0)
        {
            currentModelIndex++;
            currentMeshIndex = 0;
            currentModel = ResourceModel_CreateEmpty(scv(modelNames[currentModelIndex - 1]), meshCounts[currentModelIndex - 1], vertexPositionCounts[currentModelIndex - 1]);
            String_Destroy(&modelNames[currentModelIndex - 1]);

            ListArray_Add(&models, &currentModel);

            if (currentVertexUvPool.data != NULL)
            {
                ListArray_Destroy(&currentVertexUvPool);
            }

            if (vertexUvCounts[currentModelIndex - 1] != 0)
            {
                currentVertexUvPool = ListArray_Create("Vector2", sizeof(Vector2), vertexUvCounts[currentModelIndex - 1]);
            }
            else
            {
                currentVertexUvPool.data = NULL;
            }

            if (currentVertexNormalPool.data != NULL)
            {
                ListArray_Destroy(&currentVertexNormalPool);
            }

            if (vertexNormalCounts[currentModelIndex - 1] != 0)
            {
                currentVertexNormalPool = ListArray_Create("Vector3", sizeof(Vector3), vertexNormalCounts[currentModelIndex - 1]);
            }
            else
            {
                currentVertexNormalPool.data = NULL;
            }
        }
        else if (String_Compare(firstToken, strO) == 0)
        {
            currentMeshIndex++;
            currentMesh = ResourceMesh_Create(faceCounts[currentModelIndex - 1][currentMeshIndex - 1] * 3, currentMaterial);
            ListArray_Add(&currentModel->meshes, &currentMesh);
        }
    }

    free(vertexPositionCounts);
    free(vertexUvCounts);
    free(vertexNormalCounts);
    free(meshCounts);
    free(modelNames);
    free(mdlLines);

    for (RJGlobal_Size i = 0; i < modelCount; i++)
    {
        free(faceCounts[i]);
    }

    free(faceCounts);
    ResourceText_Destroy(rscModel);

    return models;
}

void ResourceModel_Destroy(ResourceModel *model)
{
    RJGlobal_DebugAssertNullPointerCheck(model);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(model->name, tempTitle);

    String_Destroy(&model->name);

    ListArray_Destroy(&model->vertices);

    for (RJGlobal_Size i = 0; i < model->meshes.count; i++)
    {
        ResourceMesh *mesh = (ResourceMesh *)ListLinked_Get(&model->meshes, 0);
        ResourceMesh_Destroy(mesh);
        ListLinked_RemoveAtIndex(&model->meshes, 0);
    }

    ListArray_Destroy(&model->meshes);

    free(model);

    RJGlobal_DebugInfo("Resource Model '%s' destroyed.", tempTitle);
}

#pragma endregion ResourceModel
