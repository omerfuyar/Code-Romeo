#include "systems/Renderer.h"

#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"
#include "utilities/ListArray.h"
#include "utilities/HashMap.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#include "cglm/cglm.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(pop)
#endif

#define RENDERER_OPENGL_DRAW_TYPE GL_DYNAMIC_DRAW

#pragma region Source Only

#pragma region typedefs

#define RendererDebug_CheckError()                                                  \
    do                                                                              \
    {                                                                               \
        GLenum glError = glGetError();                                              \
        RJGlobal_DebugAssert(glError == GL_NO_ERROR, "OpenGL error : %d", glError); \
    } while (0)

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (SCENE CREATE)
typedef struct RendererMeshVertex
{
    Vector3 vertexPosition;
    Vector3 vertexNormal;
    Vector2 vertexUV;
} RendererMeshVertex;

/// @brief Index of a mesh within a model.
typedef uint32_t RendererMeshIndex;

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (RENDERER DEBUG INITIALIZE)
typedef struct RendererDebugVertex
{
    Vector3 vertexPosition;
    Color vertexColor;
} RendererDebugVertex;

/// @brief 4x4 matrix type for renderer.
typedef struct Renderer_Matrix4
{
    alignas(16) float m[4][4];
} Renderer_Matrix4;

/// @brief Handle for a shader program object.
typedef uint32_t RendererShaderProgramHandle;
/// @brief Handle for a texture object.
typedef uint32_t RendererTextureHandle;

/// @brief Handle for a uniform location in a shader program.
typedef int32_t RendererUniformLocationHandle;
/// @brief Handle for a uniform block in a shader program.
typedef uint32_t RendererUniformBlockHandle;

/// @brief Handle for a Vertex Array Object.
typedef uint32_t RendererVAOHandle;
/// @brief Handle for a Vertex Buffer Object.
typedef uint32_t RendererVBOHandle;
/// @brief Handle for an Index Buffer Object.
typedef uint32_t RendererIBOHandle;
/// @brief Handle for a Uniform Buffer Object.
typedef uint32_t RendererUBOHandle;

/// @brief Texture object for the renderer.
typedef struct RendererTexture
{
    RJGlobal_Size index;
    RendererTextureHandle handle;
    void *data;
    Vector2Int size;
    int channels;
} RendererTexture;

/// @brief A material that holds the rendering properties of a mesh.
typedef struct RendererMaterial
{
    String name; // maybe change with unique id later

    Vector3 ambientColor;
    Vector3 diffuseColor;
    Vector3 specularColor;
    Vector3 emissiveColor;
    RendererTexture *diffuseMap;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
} RendererMaterial;

// todo maybe store textures and mate

typedef struct RendererMesh
{
    RendererMaterial *material;
    ListArray indices; // RendererMeshIndex
} RendererMesh;

typedef struct RendererModel
{
    ListArray vertices; // RendererMeshVertex
    ListLinked meshes;  // RendererMesh
} RendererModel;

/// @brief A batch of render components that use the same model.
typedef struct RENDERER_BATCH
{
    RendererModel *model;

    struct RENDERER_DATA
    {
        RJGlobal_Size capacity;
        RJGlobal_Size count;
        ListArray freeIndices; // RJGlobal_Index
    } data;

    struct RENDERER_COMPONENTS
    {
        RJGlobal_Index *entities;

        Renderer_Matrix4 *objectMatrices;

        Vector3 *positionReferences;
        Vector3 *rotationReferences;
        Vector3 *scaleReferences;

        uint8_t *flags;
    } components;
} RENDERER_BATCH;

#pragma endregion typedefs

struct RENDERER_MAIN_SCENE
{
    ContextWindow *window;

    struct RENDERER_DATA
    {
        RJGlobal_Size capacity;
        RJGlobal_Size count;
        ListArray freeIndices; // RJGlobal_Index

        RENDERER_BATCH *batches; // RENDERER_BATCH
    } data;

    struct RENDERER_CAMERA
    {
        Vector3 *positionReference;
        Vector3 *rotationReference;

        Renderer_Matrix4 projectionMatrix;
        Renderer_Matrix4 viewMatrix;

        float *sizeReference; // fov if perspective, orthographic size if orthographic
        float *nearClipPlaneReference;
        float *farClipPlaneReference;
        bool *isPerspectiveReference;
    } camera;

    struct RENDERER_SHADER
    {
        RendererShaderProgramHandle programHandle;

        ListLinked textures; // RendererTexture

        RendererVAOHandle vao;
        RendererVBOHandle vboModelVertices;
        RendererIBOHandle iboModelIndices;
        RendererUBOHandle uboObjectMatrices;

        // HashMap uniforms; // RendererUniformLocationHandle
        RendererUniformLocationHandle camProjectionMatrix;
        RendererUniformLocationHandle camViewMatrix;
        RendererUniformLocationHandle camPosition;
        RendererUniformLocationHandle camRotation;
        RendererUniformLocationHandle camSize;
        RendererUniformLocationHandle camIsPerspective;

        RendererUniformLocationHandle matAmbientColor;
        RendererUniformLocationHandle matDiffuseColor;
        RendererUniformLocationHandle matSpecularColor;
        RendererUniformLocationHandle matEmissiveColor;
        RendererUniformLocationHandle matSpecularExponent;
        RendererUniformLocationHandle matDissolve;
        RendererUniformLocationHandle matDiffuseMap;
        RendererUniformLocationHandle matHasDiffuseMap;

        RendererUniformBlockHandle objectMatricesHandle;
    } shader;

    struct RENDERER_DEBUG_SHADER
    {
        RendererShaderProgramHandle programHandle;

        ListArray vertices; // RendererDebugVertex

        RendererVAOHandle vao;
        RendererVBOHandle vbo;

        RendererUniformLocationHandle camProjectionMatrix;
        RendererUniformLocationHandle camViewMatrix;
    } debugShader;
} RMS = {0};

#define rmsBatch(batch) (RMS.data.batches[batch])
#define rmsEntity(batch, component) (rmsBatch(batch).components.entities[component])

#define rmsObjectMatrix(batch, component) (rmsBatch(batch).components.objectMatrices[component])

#define rmsPositionReference(batch, component) (rmsBatch(batch).components.positionReferences[component])
#define rmsRotationReference(batch, component) (rmsBatch(batch).components.rotationReferences[component])
#define rmsScaleReference(batch, component) (rmsBatch(batch).components.scaleReferences[component])

#define rmsFlag(batch, component) (rmsBatch(batch).components.flags[component])

#define rmsIsActive(batch, component) (rmsFlag(batch, component) & RENDERER_FLAG_ACTIVE)
#define rmsSetActive(batch, component, isActive) (rmsFlag(batch, component) = isActive ? (rmsFlag(batch, component) | RENDERER_FLAG_ACTIVE) : (rmsFlag(batch, component) & ~RENDERER_FLAG_ACTIVE))

#define rmsAssertBatch(batch) RJGlobal_DebugAssert(batch < RMS.data.count + RMS.data.freeIndices.count, "Renderer batch %u exceeds maximum batch count %u.", batch, RMS.data.count)
#define rmsAssertComponent(batch, component) RJGlobal_DebugAssert(component < rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count && rmsEntity(batch, component) != RJGLOBAL_INDEX_INVALID && rmsIsActive(batch, component), "Renderer component %u either exceeds maximum possible index %u, invalid or inactive.", component, rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count)

/// @brief
/// @param window
/// @param width
/// @param height
void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(void *window, int width, int height)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    RMS.window->size.x = width;
    RMS.window->size.y = height;

    if (RMS.window->fullScreen)
    {
        glfwGetFramebufferSize(RMS.window->handle, &RMS.window->size.x, &RMS.window->size.y);
    }

    glViewport(0, 0, RMS.window->size.x, RMS.window->size.y);
}

/// @brief
/// @param source
/// @param type
/// @param id
/// @param severity
/// @param length
/// @param message
/// @param userParam
void RENDERER_MAIN_WINDOW_LOG_CALLBACK(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;

    if (type == GL_DEBUG_TYPE_ERROR)
    {
        RJGlobal_DebugError("OpenGL Error :\ntype : 0x%x\nseverity : 0x%x\nmessage : \n%s\n",
                            type, severity, message);
    }
    else
    {
        RJGlobal_DebugInfo("OpenGL Log :\ntype : 0x%x\nseverity : 0x%x\nmessage : \n%s\n",
                           type, severity, message);
    }
}

/// @brief
/// @param matrix
/// @param position
/// @param rotation
/// @param scale
void TRANSFORM_TO_MODEL_MATRIX(Renderer_Matrix4 *matrix, const Vector3 *position, const Vector3 *rotation, const Vector3 *scale)
{
    RJGlobal_DebugAssertNullPointerCheck(matrix);

    glm_mat4_identity((vec4 *)matrix);

    glm_translate((vec4 *)matrix, (float *)&(vec3){position->x, position->y, position->z});

    glm_rotate((vec4 *)matrix, rotation->x, (float *)&(vec3){1, 0, 0});
    glm_rotate((vec4 *)matrix, rotation->y, (float *)&(vec3){0, 1, 0});
    glm_rotate((vec4 *)matrix, rotation->z, (float *)&(vec3){0, 0, 1});

    glm_scale((vec4 *)matrix, (float *)&(vec3){scale->x, scale->y, scale->z});
}

#pragma region Renderer Texture

RendererTexture *RendererTexture_CreateOrGet(StringView name, const void *data, Vector2Int size, int channels)
{
    for (RJGlobal_Size i = 0; i < RMS.shader.textures.count; i++)
    {
        RendererTexture *currentTexture = (RendererTexture *)ListArray_Get(&RMS.shader.textures, i);

        if (String_Compare(scv(currentTexture->name), name) == 0)
        {
            RJGlobal_DebugInfo("Texture '%.*s' found in texture pool, reusing it.", (int)name.length, name.characters);
            return currentTexture;
        }
    }

    RJGlobal_DebugAssertNullPointerCheck(data);

    RendererTexture *texture = (RendererTexture *)ListArray_Add(&RMS.shader.textures, NULL); // todo fix
    texture->index = RMS.shader.textures.count - 1;
    texture->size = size;
    texture->channels = channels;
    texture->name = scc(name);

    RJGlobal_Size dataSize = (RJGlobal_Size)(size.x * size.y * channels);

    RJGlobal_DebugAssertAllocationCheck(char, texture->data, dataSize);
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

void RendererTexture_Destroy(RendererTexture *texture)
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

#pragma endregion Renderer Texture

#pragma region Renderer Mesh

RendererMesh *RendererMesh_CreateEmpty(RJGlobal_Size initialIndexCapacity, RendererMaterial *material)
{
    RendererMesh *mesh = NULL;
    RJGlobal_DebugAssertAllocationCheck(RendererMesh, mesh, 1);

    mesh->indices = ListArray_Create("Renderer Mesh Indices", sizeof(RendererMeshIndex), initialIndexCapacity);
    mesh->material = material;

    return mesh;
}

void RendererMesh_Destroy(RendererMesh *mesh)
{
    RJGlobal_DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->indices);

    mesh->material = NULL;

    free(mesh);
    mesh = NULL;
}

#pragma endregion Renderer Mesh

#pragma region Renderer Material

ListArray RendererMaterial_CreateFromFile(StringView matFile)
{
    ResourceText *rscMaterial = ResourceText_Create(matFile);

    RJGlobal_Size materialCount = 0;

    RJGlobal_Size mtlLineCount = 0;
    StringView *mtlLines = NULL;

    RJGlobal_DebugAssertAllocationCheck(StringView, mtlLines, rscMaterial->lineCount);

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

    String_Tokenize(scv(rscMaterial->data), strNewline, &mtlLineCount, mtlLines, rscMaterial->lineCount);

    for (RJGlobal_Size j = 0; j < mtlLineCount; j++) // count
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            materialCount++;
        }
    }

    ListArray materials = ListArray_Create("Renderer Material Pointer", sizeof(RendererMaterial *), materialCount);
    RendererMaterial *currentMaterial = NULL;

    for (RJGlobal_Size j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            RJGlobal_DebugAssertAllocationCheck(RendererMaterial, currentMaterial, 1);

            ListArray_Add(&materials, &currentMaterial);

            currentMaterial->name = scc(mtlLineTokens[1]);
            currentMaterial->diffuseMap = NULL;
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
    ResourceText_Destroy(rscMaterial);

    return materials;
}

ListArray RendererMaterial_CreateFromFileTextured(StringView matFileData, RJGlobal_Size matFileLineCount, StringView textureName, const void *textureData, Vector2Int textureSize, int textureChannels)
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

    ListArray materials = ListArray_Create("Renderer Material Pointer", sizeof(RendererMaterial *), materialCount);
    RendererMaterial *currentMaterial = NULL;

    for (RJGlobal_Size j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            RJGlobal_DebugAssertAllocationCheck(RendererMaterial, currentMaterial, 1);

            ListArray_Add(&materials, &currentMaterial);

            currentMaterial->name = scc(mtlLineTokens[1]);
            currentMaterial->diffuseMap = RendererTexture_CreateOrGet(textureName, textureData, textureSize, textureChannels);
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

void RendererMaterial_Destroy(RendererMaterial *material)
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

#pragma endregion Renderer Material

#pragma region Renderer Model

void ProcessFaceVertex(StringView faceComponent, RendererModel *model, RendererMesh *currentMesh, ListArray *globalVertexUvPool, ListArray *globalVertexNormalPool)
{
    StringView faceData[4]; // v/vt/vn/w
    RJGlobal_Size faceDataCount;
    String_Tokenize(faceComponent, scl("/"), &faceDataCount, faceData, 4);

    int createdVertexIndex = String_ToInt(faceData[0]);
    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

    RendererMeshVertex *vertex = (RendererMeshVertex *)ListArray_Get(&model->vertices, (RJGlobal_Size)vertexIndex);

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

RendererModel *RendererModel_CreateEmpty(StringView name, RJGlobal_Size initialMeshCapacity, RJGlobal_Size initialVertexCapacity)
{
    RendererModel *model = NULL;
    RJGlobal_DebugAssertAllocationCheck(RendererModel, model, 1);

    model->name = scc(name);
    model->vertices = ListArray_Create("Renderer Mesh Vertex", sizeof(RendererMeshVertex), initialVertexCapacity);
    model->meshes = ListArray_Create("Renderer Mesh Pointer", sizeof(RendererMesh *), initialMeshCapacity);

    return model;
}

RendererModel *RendererModel_Create(StringView mdlFile, const ListArray *materialPool, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset)
{
    Renderer_Matrix4 offsetMatrix = {0};
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

    RendererModel *model = RendererModel_CreateEmpty(scv(modelName), meshCount, totalVertexPositionCount);
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

            RendererMeshVertex createdVertex;
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

    RendererMesh *currentMesh = NULL;
    RendererMaterial *currentMaterial = NULL;

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
                RendererMaterial *material = *(RendererMaterial **)ListArray_Get(materialPool, j);

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
            currentMesh = RendererMesh_CreateEmpty(faceCounts[model->meshes.count] * 3, currentMaterial);
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

    RJGlobal_DebugInfo("Renderer Model '%s' imported successfully with %u child meshes.", model->name.characters, model->meshes.count);

    return model;
}

ListArray RendererModel_CreateFromFile(StringView mdlFile, const ListArray *materialPool)
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

    ListArray models = ListArray_Create("Renderer Model Pointer", sizeof(RendererModel *), modelCount);
    RendererModel *currentModel = NULL;

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

    RendererMaterial *currentMaterial = NULL;
    RendererMesh *currentMesh = NULL;
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
            RendererMeshVertex createdVertex;
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
                RendererMaterial *material = *(RendererMaterial **)ListArray_Get(materialPool, j);

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
            currentModel = RendererModel_CreateEmpty(scv(modelNames[currentModelIndex - 1]), meshCounts[currentModelIndex - 1], vertexPositionCounts[currentModelIndex - 1]);
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
            currentMesh = RendererMesh_CreateEmpty(faceCounts[currentModelIndex - 1][currentMeshIndex - 1] * 3, currentMaterial);
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

void RendererModel_Destroy(RendererModel *model)
{
    RJGlobal_DebugAssertNullPointerCheck(model);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    scb(model->name, tempTitle);

    String_Destroy(&model->name);

    ListArray_Destroy(&model->vertices);

    for (RJGlobal_Size i = model->meshes.count - 1; i < model->meshes.count; i--)
    {
        RendererMesh_Destroy(*(RendererMesh **)ListArray_Get(&model->meshes, i));
    }

    ListArray_Destroy(&model->meshes);

    free(model);
    model = NULL;

    RJGlobal_DebugInfo("Renderer Model '%s' destroyed.", tempTitle);
}

#pragma endregion Renderer Model

#pragma endregion Source Only

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window, RJGlobal_Size initialBatchCapacity)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    RMS.window = window;
    RJGlobal_DebugAssertAllocationCheck(RENDERER_BATCH, RMS.data.batches, initialBatchCapacity);

    RMS.data.capacity = initialBatchCapacity;
    RMS.data.count = 0;
    RMS.data.freeIndices = ListArray_Create("Renderer Free Indices", sizeof(RJGlobal_Index), initialBatchCapacity);

    RJGlobal_DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    Context_ConfigureLogCallback(RENDERER_MAIN_WINDOW_LOG_CALLBACK);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RMS.shader.programHandle = glCreateProgram();

    RMS.shader.textures = ListLinked_Create("Renderer Texture", sizeof(RendererTexture));

    glGenVertexArrays(1, &RMS.shader.vao);
    glGenBuffers(1, &RMS.shader.vboModelVertices);
    glGenBuffers(1, &RMS.shader.iboModelIndices);
    glGenBuffers(1, &RMS.shader.uboObjectMatrices);

    glBindVertexArray(RMS.shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.shader.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RMS.shader.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, RMS.shader.uboObjectMatrices);

    size_t offset = 0;

    glVertexAttribPointer(RENDERER_VBO_POSITION_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_POSITION_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_VBO_NORMAL_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_NORMAL_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_VBO_UV_BINDING, 2, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_UV_BINDING);
    offset += sizeof(Vector2);

    //! ... other attributes in vertex

    RMS.shader.camProjectionMatrix = glGetUniformLocation(RMS.shader.programHandle, "camProjectionMatrix");
    RMS.shader.camViewMatrix = glGetUniformLocation(RMS.shader.programHandle, "camViewMatrix");
    RMS.shader.camPosition = glGetUniformLocation(RMS.shader.programHandle, "camPosition");
    RMS.shader.camRotation = glGetUniformLocation(RMS.shader.programHandle, "camRotation");
    RMS.shader.camSize = glGetUniformLocation(RMS.shader.programHandle, "camSize");
    RMS.shader.camIsPerspective = glGetUniformLocation(RMS.shader.programHandle, "camIsPerspective");

    RMS.shader.matAmbientColor = glGetUniformLocation(RMS.shader.programHandle, "matAmbientColor");
    RMS.shader.matDiffuseColor = glGetUniformLocation(RMS.shader.programHandle, "matDiffuseColor");
    RMS.shader.matSpecularColor = glGetUniformLocation(RMS.shader.programHandle, "matSpecularColor");
    RMS.shader.matEmissiveColor = glGetUniformLocation(RMS.shader.programHandle, "matEmissiveColor");
    RMS.shader.matSpecularExponent = glGetUniformLocation(RMS.shader.programHandle, "matSpecularExponent");
    RMS.shader.matDissolve = glGetUniformLocation(RMS.shader.programHandle, "matDissolve");
    RMS.shader.matDiffuseMap = glGetUniformLocation(RMS.shader.programHandle, "matDiffuseMap");
    RMS.shader.matHasDiffuseMap = glGetUniformLocation(RMS.shader.programHandle, "matHasDiffuseMap");

    RMS.shader.objectMatricesHandle = glGetUniformBlockIndex(RMS.shader.programHandle, "modelMatrices");
    glUniformBlockBinding(RMS.shader.programHandle, RMS.shader.objectMatricesHandle, RENDERER_UBO_MATRICES_BINDING);
    glBindBufferBase(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, RMS.shader.uboObjectMatrices);
    // glBindBufferRange(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices, 0, RENDERER_SCENE_MAX_OBJECT_COUNT);

    RJGlobal_DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate()
{
    RMS.window = NULL;

    for (RJGlobal_Size batch = 0; batch < RMS.data.count; batch++)
    {
        Renderer_BatchDestroy(batch);
    }

    free(RMS.data.batches);

    RMS.data.capacity = 0;
    RMS.data.count = 0;
    ListArray_Destroy(&RMS.data.freeIndices);

    RMS.camera.positionReference = NULL;
    RMS.camera.rotationReference = NULL;
    RMS.camera.sizeReference = NULL;
    RMS.camera.nearClipPlaneReference = NULL;
    RMS.camera.farClipPlaneReference = NULL;
    RMS.camera.isPerspectiveReference = NULL;

    if (RMS.shader.programHandle != 0)
    {
        glDeleteProgram(RMS.shader.programHandle);
    }

    RMS.shader.programHandle = 0;

    for (RJGlobal_Size i = 0; i < RMS.shader.textures.count; i++)
    {
        RendererTexture_Destroy((RendererTexture *)ListLinked_RemoveHead(&RMS.shader.textures));
    }

    ListLinked_Destroy(&RMS.shader.textures);

    glDeleteVertexArrays(1, &RMS.shader.vao);
    glDeleteBuffers(1, &RMS.shader.vboModelVertices);
    glDeleteBuffers(1, &RMS.shader.iboModelIndices);
    glDeleteBuffers(1, &RMS.shader.uboObjectMatrices);

    RMS.shader.vao = 0;
    RMS.shader.vboModelVertices = 0;
    RMS.shader.iboModelIndices = 0;
    RMS.shader.uboObjectMatrices = 0;

    RMS.shader.camProjectionMatrix = 0;
    RMS.shader.camViewMatrix = 0;
    RMS.shader.camPosition = 0;
    RMS.shader.camRotation = 0;
    RMS.shader.camSize = 0;
    RMS.shader.camIsPerspective = 0;

    RMS.shader.matAmbientColor = 0;
    RMS.shader.matDiffuseColor = 0;
    RMS.shader.matSpecularColor = 0;
    RMS.shader.matEmissiveColor = 0;
    RMS.shader.matSpecularExponent = 0;
    RMS.shader.matDissolve = 0;
    RMS.shader.matDiffuseMap = 0;
    RMS.shader.matHasDiffuseMap = 0;

    RMS.shader.objectMatricesHandle = 0;

    RJGlobal_DebugInfo("Renderer terminated successfully.");
}

void Renderer_ConfigureShaders(StringView vertexShaderFile, StringView fragmentShaderFile)
{
    RJGlobal_DebugAssert(RMS.shader.programHandle != 0, "Initialize the renderer before configuring shaders.");

    ResourceText *rscVertexShader = ResourceText_Create(vertexShaderFile);
    ResourceText *rscFragmentShader = ResourceText_Create(fragmentShaderFile);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&rscVertexShader->data.characters, NULL);
    glCompileShader(vertexShader);

    ResourceText_Destroy(rscVertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&rscFragmentShader->data.characters, NULL);
    glCompileShader(fragmentShader);

    ResourceText_Destroy(rscFragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Fragment shader compiled successfully.");

    glAttachShader(RMS.shader.programHandle, vertexShader);
    glAttachShader(RMS.shader.programHandle, fragmentShader);
    glLinkProgram(RMS.shader.programHandle);

    glGetProgramiv(RMS.shader.programHandle, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RMS.shader.programHandle, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RJGlobal_DebugInfo("Shader program linked and created successfully.");
}

void Renderer_ConfigureCamera(Vector3 *positionReference, Vector3 *rotationReference, float *sizeReference, float *nearClipPlaneReference, float *farClipPlaneReference, bool *isPerspectiveReference)
{
    RMS.camera.positionReference = positionReference;
    RMS.camera.rotationReference = rotationReference;
    RMS.camera.sizeReference = sizeReference;
    RMS.camera.nearClipPlaneReference = nearClipPlaneReference;
    RMS.camera.farClipPlaneReference = farClipPlaneReference;
    RMS.camera.isPerspectiveReference = isPerspectiveReference;
}

/*
RendererScene *RendererScene_CreateFromFile(StringView scnFile, const ListArray *modelPool, void *objectReferences, RJGlobal_Size transformOffsetInObject, RJGlobal_Size totalObjectSize, RJGlobal_Size objectCount)
{
    RJGlobal_DebugAssertNullPointerCheck(modelPool);
    RJGlobal_DebugAssertNullPointerCheck(objectReferences);

    RendererScene *scene = NULL;
    RendererBatch *currentBatch = NULL;
    RendererComponent *currentComponent = NULL;
    RJGlobal_Size totalObjectIndex = 0;

    ResourceText *rscScene = ResourceText_Create(scnFile);

    RJGlobal_Size scnLineCount = 0;
    StringView *scnLines = (StringView *)malloc(rscScene->lineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(scnLines);
    RJGlobal_MemorySet(scnLines, rscScene->lineCount * sizeof(StringView), 0);

    RJGlobal_Size scnLineTokenCount = 0;
    StringView scnLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strP = scl("p");
    StringView strR = scl("r");
    StringView strS = scl("s");
    StringView strNEWSCN = scl("newscn");
    StringView strUSEMDL = scl("usemdl");

    String_Tokenize(scv(rscScene->data), strNewline, &scnLineCount, scnLines, rscScene->lineCount);
    for (RJGlobal_Size i = 0; i < scnLineCount; i++)
    {
        String_Tokenize(scnLines[i], strSpace, &scnLineTokenCount, scnLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scnLineTokens[0];

        if (String_Compare(firstToken, strP) == 0)
        {
            RJGlobal_DebugAssert(totalObjectIndex < objectCount, "Object reference count %u is not enough for the imported scene file", objectCount);
            currentComponent = RendererBatch_CreateComponent(currentBatch,
                                                             (Vector3 *)((char *)objectReferences + (totalObjectSize * totalObjectIndex) + transformOffsetInObject + 0 * sizeof(Vector3)),
                                                             (Vector3 *)((char *)objectReferences + (totalObjectSize * totalObjectIndex) + transformOffsetInObject + 1 * sizeof(Vector3)),
                                                             (Vector3 *)((char *)objectReferences + (totalObjectSize * totalObjectIndex) + transformOffsetInObject + 2 * sizeof(Vector3)));

            *currentComponent->positionReference = Vector3_New(String_ToFloat(scnLineTokens[1]), String_ToFloat(scnLineTokens[2]), String_ToFloat(scnLineTokens[3]));
        }
        else if (String_Compare(firstToken, strR) == 0)
        {
            *currentComponent->rotationReference = Vector3_New(String_ToFloat(scnLineTokens[1]), String_ToFloat(scnLineTokens[2]), String_ToFloat(scnLineTokens[3]));
        }
        else if (String_Compare(firstToken, strS) == 0)
        {
            *currentComponent->scaleReference = Vector3_New(String_ToFloat(scnLineTokens[1]), String_ToFloat(scnLineTokens[2]), String_ToFloat(scnLineTokens[3]));
            totalObjectIndex++;
        }
        else if (String_Compare(firstToken, strNEWSCN) == 0)
        {
            scene = RendererScene_CreateEmpty(scnLineTokens[1], modelPool->count);
        }
        else if (String_Compare(firstToken, strUSEMDL) == 0)
        {
            bool modelFound = false;

            for (RJGlobal_Size j = 0; j < modelPool->count; j++)
            {
                RendererModel *model = *(RendererModel **)ListArray_Get(modelPool, j);

                // RJGlobal_DebugInfo("Comparing model '%.*s' with '%.*s'...", model->name.length, model->name.characters, lineTokens[1].length, lineTokens[1].characters);

                if (String_Compare(scv(model->name), scv(scnLineTokens[1])) == 0)
                {
                    modelFound = true;
                    currentBatch = RendererScene_CreateBatch(scene, model, scnLineTokenCount > 2 ? (RJGlobal_Size)String_ToInt(scv(scnLineTokens[2])) : RENDERER_BATCH_INITIAL_CAPACITY);
                    break;
                }
            }

            RJGlobal_DebugAssert(modelFound, "Model '%.*s' not found in model pool when trying to assign it to mesh in model '%.*s'.", scnLineTokens[1].length, scnLineTokens[1].characters, currentBatch->model->name.length, currentBatch->model->name.characters);
        }
    }

    free(scnLines);
    ResourceText_Destroy(rscScene);

    RJGlobal_DebugInfo("Scene %s imported successfully.", scene->name.characters);

    return scene;
}
*/

// todo move this to shader, compute in gpu
void Renderer_Update()
{
    glm_mat4_identity((vec4 *)&RMS.camera.projectionMatrix);
    glm_mat4_identity((vec4 *)&RMS.camera.viewMatrix);

    Vector3 direction = Vector3_Normalized(Vector3_New(Maths_Cos(RMS.camera.rotationReference->x) * Maths_Cos(RMS.camera.rotationReference->y),
                                                       Maths_Sin(RMS.camera.rotationReference->x),
                                                       Maths_Cos(RMS.camera.rotationReference->x) * Maths_Sin(RMS.camera.rotationReference->y)));

    Vector3 center = Vector3_Add(*RMS.camera.positionReference, Vector3_Normalized(direction));

    glm_lookat((float *)RMS.camera.positionReference, (float *)&center, (float *)&(vec3){0, 1, 0}, (vec4 *)&RMS.camera.viewMatrix);
    if (*RMS.camera.isPerspectiveReference)
    {
        glm_perspective(Maths_DegToRad(*RMS.camera.sizeReference),
                        (float)RMS.window->size.x / (float)RMS.window->size.y,
                        *RMS.camera.nearClipPlaneReference,
                        *RMS.camera.farClipPlaneReference,
                        (vec4 *)&RMS.camera.projectionMatrix);
    }
    else
    {
        float sizeX = (float)RMS.window->size.x * *RMS.camera.sizeReference / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)RMS.window->size.y * *RMS.camera.sizeReference / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;

        glm_ortho(-sizeX, sizeX,
                  -sizeY, sizeY,
                  *RMS.camera.nearClipPlaneReference,
                  *RMS.camera.farClipPlaneReference,
                  (vec4 *)&RMS.camera.projectionMatrix);
    }

    for (RJGlobal_Size batch = 0; batch < RMS.data.count; batch++)
    {
        for (RJGlobal_Size component = 0; component < rmsBatch(batch).data.count; component++)
        {
            if (!rmsIsActive(batch, component))
            {
                continue;
            }

            TRANSFORM_TO_MODEL_MATRIX(
                &rmsObjectMatrix(batch, component),
                &rmsPositionReference(batch, component),
                &rmsRotationReference(batch, component),
                &rmsScaleReference(batch, component));
        }
    }
}

void Renderer_Resize(RJGlobal_Size newBatchCapacity)
{
    RJGlobal_DebugAssert(newBatchCapacity > RMS.data.count, "New batch capacity %u is must be greater than current batch count %u.", newBatchCapacity, RMS.data.count);

    RENDERER_BATCH *newBatches = NULL;
    RJGlobal_DebugAssertAllocationCheck(RENDERER_BATCH, newBatches, newBatchCapacity);

    for (RJGlobal_Size i = 0; i < RMS.data.count; i++)
    {
        newBatches[i] = rmsBatch(i);
    }

    free(RMS.data.batches);
    RMS.data.batches = newBatches;

    RMS.data.capacity = newBatchCapacity;

    RJGlobal_DebugInfo("Renderer resized to new batch capacity of %u successfully.", newBatchCapacity);
}

void Renderer_Render()
{
    if (glfwWindowShouldClose(RMS.window->handle))
    {
        RJGlobal_DebugInfo("Main window close input received");
        RJGlobal_Terminate(EXIT_SUCCESS, "Main window close input received");
    }

    glClearColor(RENDERER_OPENGL_CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RMS.shader.programHandle);

    glBindVertexArray(RMS.shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.shader.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RMS.shader.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, RMS.shader.uboObjectMatrices);

    glUniformMatrix4fv(RMS.shader.camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&RMS.camera.projectionMatrix);
    glUniformMatrix4fv(RMS.shader.camViewMatrix, 1, GL_FALSE, (GLfloat *)&RMS.camera.viewMatrix);
    glUniform3fv(RMS.shader.camPosition, 1, (GLfloat *)RMS.camera.positionReference);
    glUniform3fv(RMS.shader.camRotation, 1, (GLfloat *)RMS.camera.rotationReference);
    glUniform1f(RMS.shader.camSize, *RMS.camera.sizeReference);
    glUniform1i(RMS.shader.camIsPerspective, *RMS.camera.isPerspectiveReference);

    for (RJGlobal_Size batch = 0; batch < RMS.data.count; batch++)
    {
        RendererMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(sizeof(Vector3) * rmsBatch(batch).data.count),
                     rmsBatch(batch).components.positionReferences,
                     RENDERER_OPENGL_DRAW_TYPE);

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(sizeof(Vector3) * rmsBatch(batch).data.count),
                     rmsBatch(batch).components.rotationReferences,
                     RENDERER_OPENGL_DRAW_TYPE);
        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(sizeof(Vector3) * rmsBatch(batch).data.count),
                     rmsBatch(batch).components.scaleReferences,
                     RENDERER_OPENGL_DRAW_TYPE);

        glBufferData(GL_ARRAY_BUFFER,
                     (long long)(rmsBatch(batch).model->vertices.sizeOfItem * rmsBatch(batch).model->vertices.count),
                     rmsBatch(batch).model->vertices.data,
                     RENDERER_OPENGL_DRAW_TYPE);

        for (RJGlobal_Size j = 0; j < rmsBatch(batch).model->meshes.count; j++)
        {
            RendererMesh *mesh = (RendererMesh *)ListArray_Get(&rmsBatch(batch).model->meshes, j);

            if (mesh->material != previousMaterial)
            {
                glUniform3fv(RMS.shader.matAmbientColor, 1, (GLfloat *)&mesh->material->ambientColor);
                glUniform3fv(RMS.shader.matDiffuseColor, 1, (GLfloat *)&mesh->material->diffuseColor);
                glUniform3fv(RMS.shader.matSpecularColor, 1, (GLfloat *)&mesh->material->specularColor);
                glUniform3fv(RMS.shader.matEmissiveColor, 1, (GLfloat *)&mesh->material->emissiveColor);
                glUniform1f(RMS.shader.matSpecularExponent, mesh->material->specularExponent);
                glUniform1f(RMS.shader.matDissolve, mesh->material->dissolve);

                // Texture binding
                if (mesh->material->diffuseMap != NULL)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->material->diffuseMap->handle);
                    glUniform1i(RMS.shader.matDiffuseMap, 0);
                    glUniform1i(RMS.shader.matHasDiffuseMap, 1);
                }
                else
                {
                    glUniform1i(RMS.shader.matHasDiffuseMap, 0);
                }

                previousMaterial = mesh->material;
            }

            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         (long long)(mesh->indices.sizeOfItem * mesh->indices.count),
                         mesh->indices.data,
                         RENDERER_OPENGL_DRAW_TYPE);

            glDrawElementsInstanced(GL_TRIANGLES,
                                    (GLsizei)mesh->indices.count,
                                    GL_UNSIGNED_INT,
                                    0,
                                    (GLsizei)rmsBatch(batch).data.count);
        }
    }

    // RendererDebug_CheckError();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glfwSwapBuffers(RMS.window->handle);

    // RJGlobal_DebugInfo("Scene '%s' rendered", scene.name.characters);
}

RendererBatch Renderer_BatchCreate(StringView mdlFile, RJGlobal_Size initialCapacity, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences)
{
    RJGlobal_DebugAssert(RMS.data.count + RMS.data.freeIndices.count < RMS.data.capacity, "Maximum renderer batch capacity of %u reached.", RMS.data.capacity); // todo expand capacity

    RendererBatch newBatch = RJGLOBAL_INDEX_INVALID;

    newBatch = RMS.data.freeIndices.count != 0 ? *((RJGlobal_Index *)ListArray_Pop(&RMS.data.freeIndices)) : RMS.data.count;

    rmsBatch(newBatch).model = NULL; // todo

    rmsBatch(newBatch).data.capacity = initialCapacity;
    rmsBatch(newBatch).data.count = 0;
    rmsBatch(newBatch).data.freeIndices = ListArray_Create("RJGlobal_Index", sizeof(RJGlobal_Index), RENDERER_INITIAL_FREE_INDEX_ARRAY_SIZE);

    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Index, rmsBatch(newBatch).components.entities, initialCapacity);

    RJGlobal_DebugAssertAllocationCheck(Renderer_Matrix4, rmsBatch(newBatch).components.objectMatrices, initialCapacity);

    rmsBatch(newBatch).components.positionReferences = positionReferences;
    rmsBatch(newBatch).components.rotationReferences = rotationReferences;
    rmsBatch(newBatch).components.scaleReferences = scaleReferences;

    RJGlobal_DebugAssertAllocationCheck(uint8_t, rmsBatch(newBatch).components.flags, initialCapacity);

    RMS.data.count++;

    return newBatch;
}

void Renderer_BatchDestroy(RendererBatch batch)
{
    rmsAssertBatch(batch);

    rmsBatch(batch).model = NULL;

    rmsBatch(batch).data.capacity = 0;
    rmsBatch(batch).data.count = 0;
    ListArray_Destroy(&rmsBatch(batch).data.freeIndices);

    free(rmsBatch(batch).components.entities);
    free(rmsBatch(batch).components.objectMatrices);
    free(rmsBatch(batch).components.flags);

    rmsBatch(batch).components.entities = NULL;
    rmsBatch(batch).components.objectMatrices = NULL;
    rmsBatch(batch).components.flags = NULL;

    RMS.data.count--;
}

void Renderer_BatchConfigureReferences(RendererBatch batch, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences, RJGlobal_Size newComponentCapacity)
{
    rmsAssertBatch(batch);
    RJGlobal_DebugAssertNullPointerCheck(positionReferences);
    RJGlobal_DebugAssertNullPointerCheck(rotationReferences);
    RJGlobal_DebugAssertNullPointerCheck(scaleReferences);
    RJGlobal_DebugAssert(newComponentCapacity > rmsBatch(batch).data.count, "New component capacity %u must be greater than current physics component count %u.", newComponentCapacity, rmsBatch(batch).data.count);

    rmsBatch(batch).data.capacity = newComponentCapacity;

    rmsBatch(batch).components.positionReferences = positionReferences;
    rmsBatch(batch).components.rotationReferences = rotationReferences;
    rmsBatch(batch).components.scaleReferences = scaleReferences;

    RJGlobal_DebugAssertReallocationCheck(RJGlobal_Index, rmsBatch(batch).components.entities, newComponentCapacity);

    RJGlobal_DebugAssertReallocationCheck(Renderer_Matrix4, rmsBatch(batch).components.objectMatrices, newComponentCapacity);

    rmsBatch(batch).components.positionReferences = positionReferences;
    rmsBatch(batch).components.rotationReferences = rotationReferences;
    rmsBatch(batch).components.scaleReferences = scaleReferences;

    RJGlobal_DebugAssertReallocationCheck(uint8_t, rmsBatch(batch).components.flags, newComponentCapacity);
}

RendererComponent Renderer_ComponentCreate(RJGlobal_Index entity, RendererBatch batch)
{
    rmsAssertBatch(batch);
    RJGlobal_DebugAssert(rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count < rmsBatch(batch).data.capacity, "Maximum renderer batch %u component capacity of %u reached.", batch, rmsBatch(batch).data.capacity); // todo expand capacity
    //    RJGlobal_DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererComponent newComponent = rmsBatch(batch).data.freeIndices.count != 0 ? *((RJGlobal_Index *)ListArray_Pop(&rmsBatch(batch).data.freeIndices)) : rmsBatch(batch).data.count;

    rmsEntity(batch, newComponent) = entity;
    rmsSetActive(batch, newComponent, true);

    rmsBatch(batch).data.count++;

    return newComponent;
}

void Renderer_ComponentDestroy(RendererBatch batch, RendererComponent component)
{
    rmsAssertBatch(batch);
    rmsAssertComponent(batch, component);

    rmsEntity(batch, component) = RJGLOBAL_INDEX_INVALID;
    rmsFlag(batch, component) = false;

    ListArray_Add(&rmsBatch(batch).data.freeIndices, &component);

    rmsBatch(batch).data.count--;
}

#pragma endregion Renderer

#pragma region Renderer Debug

void RendererDebug_Initialize(StringView vertexShaderFile, StringView fragmentShaderFile, RJGlobal_Size initialVertexCapacity)
{
    RMS.debugShader.vertices = ListArray_Create("Renderer Debug Vertex", sizeof(RendererDebugVertex), initialVertexCapacity);

    glGenVertexArrays(1, &RMS.debugShader.vao);
    glGenBuffers(1, &RMS.debugShader.vbo);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    ResourceText *rscVertexShader = ResourceText_Create(vertexShaderFile);
    ResourceText *rscFragmentShader = ResourceText_Create(fragmentShaderFile);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&rscVertexShader->data.characters, NULL);
    glCompileShader(vertexShader);

    ResourceText_Destroy(rscVertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Debug Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&rscFragmentShader->data.characters, NULL);
    glCompileShader(fragmentShader);

    ResourceText_Destroy(rscFragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Debug Fragment shader compiled successfully.");

    RMS.debugShader.programHandle = glCreateProgram();
    glAttachShader(RMS.debugShader.programHandle, vertexShader);
    glAttachShader(RMS.debugShader.programHandle, fragmentShader);
    glLinkProgram(RMS.debugShader.programHandle);

    glGetProgramiv(RMS.debugShader.programHandle, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RMS.debugShader.programHandle, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RMS.debugShader.camProjectionMatrix = glGetUniformLocation(RMS.debugShader.programHandle, "camProjectionMatrix");
    RMS.debugShader.camViewMatrix = glGetUniformLocation(RMS.debugShader.programHandle, "camViewMatrix");

    glBindVertexArray(RMS.debugShader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.debugShader.vbo);

    size_t offset = 0;

    glVertexAttribPointer(RENDERER_DEBUG_VBO_POSITION_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(RendererDebugVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_DEBUG_VBO_POSITION_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_DEBUG_VBO_COLOR_BINDING, 4, GL_FLOAT, GL_FALSE, sizeof(RendererDebugVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_DEBUG_VBO_COLOR_BINDING);
    offset += sizeof(Color);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RJGlobal_DebugInfo("Debug Renderer initialized successfully.");
}

void RendererDebug_Terminate()
{
    if (RMS.debugShader.vbo != 0)
    {
        glDeleteBuffers(1, &RMS.debugShader.vbo);
    }

    if (RMS.debugShader.vao != 0)
    {
        glDeleteVertexArrays(1, &RMS.debugShader.vao);
    }

    if (RMS.debugShader.programHandle != 0)
    {
        glDeleteProgram(RMS.debugShader.programHandle);
    }

    ListArray_Destroy(&RMS.debugShader.vertices);

    RMS.debugShader.vao = 0;
    RMS.debugShader.vbo = 0;
    RMS.debugShader.programHandle = 0;
    RMS.debugShader.camProjectionMatrix = 0;
    RMS.debugShader.camViewMatrix = 0;

    RJGlobal_DebugInfo("Debug Renderer terminated successfully.");
}

void RendererDebug_StartRendering()
{
    glUseProgram(RMS.debugShader.programHandle);
}

void RendererDebug_FinishRendering(const Renderer_Matrix4 *camProjectionMatrix, const Renderer_Matrix4 *camViewMatrix)
{
    if (RMS.debugShader.vertices.count == 0)
    {
        return;
    }

    glUseProgram(RMS.debugShader.programHandle);
    glUniformMatrix4fv(RMS.debugShader.camProjectionMatrix, 1, GL_FALSE, (GLfloat *)camProjectionMatrix);
    glUniformMatrix4fv(RMS.debugShader.camViewMatrix, 1, GL_FALSE, (GLfloat *)camViewMatrix);

    glBindVertexArray(RMS.debugShader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.debugShader.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(RMS.debugShader.vertices.sizeOfItem * RMS.debugShader.vertices.count), RMS.debugShader.vertices.data, RENDERER_OPENGL_DRAW_TYPE);
    glDrawArrays(GL_LINES, 0, (GLsizei)RMS.debugShader.vertices.count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ListArray_Clear(&RMS.debugShader.vertices);
}

void RendererDebug_DrawLine(Vector3 start, Vector3 end, Color color)
{
    RendererDebugVertex startVertex = (RendererDebugVertex){.vertexColor = color, .vertexPosition = start};
    RendererDebugVertex endVertex = (RendererDebugVertex){.vertexColor = color, .vertexPosition = end};

    ListArray_Add(&RMS.debugShader.vertices, &startVertex);
    ListArray_Add(&RMS.debugShader.vertices, &endVertex);
}

void RendererDebug_DrawBoxLines(Vector3 position, Vector3 size, Color color)
{
    Vector3 halfSize = Vector3_Scale(size, 0.5f);
    Vector3 min = Vector3_Add(position, Vector3_Scale(halfSize, -1.0f));
    Vector3 max = Vector3_Add(position, halfSize);

    Vector3 corners[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, max.z},
        {min.x, max.y, max.z}};

    // Bottom face
    RendererDebug_DrawLine(corners[0], corners[1], color);
    RendererDebug_DrawLine(corners[1], corners[2], color);
    RendererDebug_DrawLine(corners[2], corners[3], color);
    RendererDebug_DrawLine(corners[3], corners[0], color);

    // Top face
    RendererDebug_DrawLine(corners[4], corners[5], color);
    RendererDebug_DrawLine(corners[5], corners[6], color);
    RendererDebug_DrawLine(corners[6], corners[7], color);
    RendererDebug_DrawLine(corners[7], corners[4], color);

    // Connecting edges
    RendererDebug_DrawLine(corners[0], corners[4], color);
    RendererDebug_DrawLine(corners[1], corners[5], color);
    RendererDebug_DrawLine(corners[2], corners[6], color);
    RendererDebug_DrawLine(corners[3], corners[7], color);
}

#pragma endregion Renderer Debug
