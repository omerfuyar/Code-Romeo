#include "systems/Renderer.h"

#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"
#include "utilities/ListArray.h"
#include "utilities/HashMap.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#if RJ_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJ_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#include "cglm/cglm.h"

#if RJ_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJ_COMPILER_MSVC
#pragma warning(pop)
#endif

#define RENDERER_OPENGL_DRAW_TYPE GL_DYNAMIC_DRAW
#define RENDERER_FLAG_ACTIVE (1 << 0)

#pragma region Source Only

#pragma region typedefs

#define RendererDebug_CheckError()                                            \
    do                                                                        \
    {                                                                         \
        GLenum glError = glGetError();                                        \
        RJ_DebugAssert(glError == GL_NO_ERROR, "OpenGL error : %d", glError); \
    } while (0)

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (RENDERER DEBUG INITIALIZE)
typedef struct RendererDebugVertex
{
    Vector3 vertexPosition;
    Color vertexColor;
} RendererDebugVertex;

/// @brief Handle for a shader program object.
typedef uint32_t RendererShaderProgramHandle;
/// @brief Handle for a texture object.
typedef uint32_t ResourceTextureHandle;

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

/// @brief A batch of render components that use the same model.
typedef struct RENDERER_BATCH
{
    struct RENDERER_BATCH_DATA
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size

        ResourceModel *model;
    } data;

    struct RENDERER_COMPONENTS
    {
        RJ_Size *entities;

        Resource_Matrix4 *objectMatrices;

        Vector3 *positionReferences;
        Vector3 *rotationReferences;
        Vector3 *scaleReferences;

        uint8_t *flags;
    } components;
} RENDERER_BATCH;

#pragma endregion typedefs

bool RENDERER_INITIALIZED = false;

struct RENDERER_MAIN_SCENE
{
    ContextWindow *window;

    struct RENDERER_SCENE_DATA
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size

        RENDERER_BATCH *batches; // RENDERER_BATCH
        // todo maybe change with linked list for dynamic resizing?
    } data;

    struct RENDERER_CAMERA
    {
        Vector3 *positionReference;
        Vector3 *rotationReference;

        Resource_Matrix4 projectionMatrix;
        Resource_Matrix4 viewMatrix;

        float *sizeReference; // fov if perspective, orthographic size if orthographic
        float *nearClipPlaneReference;
        float *farClipPlaneReference;
        bool *isPerspectiveReference;
    } camera;

    struct RENDERER_SHADER
    {
        RendererShaderProgramHandle programHandle;

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
#define rmsPositionReference(batch, component) (rmsBatch(batch).components.positionReferences[rmsEntity(batch, component)])
#define rmsRotationReference(batch, component) (rmsBatch(batch).components.rotationReferences[rmsEntity(batch, component)])
#define rmsScaleReference(batch, component) (rmsBatch(batch).components.scaleReferences[rmsEntity(batch, component)])
#define rmsFlag(batch, component) (rmsBatch(batch).components.flags[component])

#define rmsIsActive(batch, component) (rmsFlag(batch, component) & RENDERER_FLAG_ACTIVE)
#define rmsSetActive(batch, component, isActive) (rmsFlag(batch, component) = isActive ? (rmsFlag(batch, component) | RENDERER_FLAG_ACTIVE) : (rmsFlag(batch, component) & ~RENDERER_FLAG_ACTIVE))

#define rmsAssertBatch(batch) RJ_DebugAssert(batch < RMS.data.count + RMS.data.freeIndices.count, "Renderer batch %u exceeds maximum batch count %u.", batch, RMS.data.count)
#define rmsAssertComponent(batch, component) RJ_DebugAssert(component < rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count && rmsEntity(batch, component) != RJ_INDEX_INVALID && rmsIsActive(batch, component), "Renderer component %u either exceeds maximum possible index %u, invalid or inactive.", component, rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count)

/// @brief
/// @param window
/// @param width
/// @param height
static void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(void *window, int width, int height)
{
    RJ_DebugAssertNullPointerCheck(window);

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
static void RENDERER_MAIN_WINDOW_LOG_CALLBACK(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;

    if (type == GL_DEBUG_TYPE_ERROR)
    {
        RJ_DebugError("OpenGL Error :\ntype : 0x%x\nseverity : 0x%x\nmessage : \n%s\n",
                      type, severity, message);
    }
    else
    {
        RJ_DebugInfo("OpenGL Log :\ntype : 0x%x\nseverity : 0x%x\nmessage : \n%s\n",
                     type, severity, message);
    }
}

#pragma endregion Source Only

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window, RJ_Size initialBatchCapacity)
{
    RJ_DebugAssertNullPointerCheck(window);

    RMS.window = window;

    RJ_DebugAssertAllocationCheck(RENDERER_BATCH, RMS.data.batches, initialBatchCapacity);

    RMS.data.capacity = initialBatchCapacity;
    RMS.data.count = 0;
    ListArray_Create(&RMS.data.freeIndices, "Renderer Free Indices", sizeof(RJ_Size), RENDERER_INITIAL_FREE_INDEX_ARRAY_SIZE);

    // todo only glfw function in this file, make a wrapper in context
    RJ_DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    glDebugMessageCallback((GLDEBUGPROC)RENDERER_MAIN_WINDOW_LOG_CALLBACK, NULL);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RMS.shader.programHandle = glCreateProgram();

    glGenVertexArrays(1, &RMS.shader.vao);
    glGenBuffers(1, &RMS.shader.vboModelVertices);
    glGenBuffers(1, &RMS.shader.iboModelIndices);
    glGenBuffers(1, &RMS.shader.uboObjectMatrices);

    glBindVertexArray(RMS.shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.shader.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RMS.shader.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, RMS.shader.uboObjectMatrices);

    size_t offset = 0;

    glVertexAttribPointer(RENDERER_VBO_POSITION_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(ResourceMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_POSITION_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_VBO_NORMAL_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(ResourceMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_NORMAL_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_VBO_UV_BINDING, 2, GL_FLOAT, GL_FALSE, sizeof(ResourceMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_UV_BINDING);
    offset += sizeof(Vector2);

    //! ... other attributes in vertex

    RENDERER_INITIALIZED = true;
    RJ_DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate(void)
{
    RMS.window = NULL;

    for (RJ_Size batch = 0; batch < RMS.data.count; batch++)
    {
        Renderer_BatchDestroy(batch);
    }

    free(RMS.data.batches);
    RMS.data.batches = NULL;

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

    RENDERER_INITIALIZED = false;
    RJ_DebugInfo("Renderer terminated successfully.");
}

bool Renderer_IsInitialized(void)
{
    return RENDERER_INITIALIZED;
}

void Renderer_ConfigureShaders(StringView vertexShaderFile, StringView fragmentShaderFile)
{
    RJ_DebugAssert(RMS.shader.programHandle != 0, "Initialize the renderer before configuring shaders.");

    ResourceText *rscVertexShader = NULL;
    ResourceText_Create(&rscVertexShader, vertexShaderFile);
    // todo check return value in systems
    ResourceText *rscFragmentShader = NULL;
    ResourceText_Create(&rscFragmentShader, fragmentShaderFile);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&rscVertexShader->data.characters, NULL);
    glCompileShader(vertexShader);

    ResourceText_Destroy(rscVertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJ_DebugInfo("Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&rscFragmentShader->data.characters, NULL);
    glCompileShader(fragmentShader);

    ResourceText_Destroy(rscFragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJ_DebugInfo("Fragment shader compiled successfully.");

    glAttachShader(RMS.shader.programHandle, vertexShader);
    glAttachShader(RMS.shader.programHandle, fragmentShader);
    glLinkProgram(RMS.shader.programHandle);

    glGetProgramiv(RMS.shader.programHandle, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RMS.shader.programHandle, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

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

    RJ_DebugInfo("Shader program linked and created successfully.");
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

Vector3 Renderer_ScreenToWorldSpace(Vector2Int screenPosition, float depth)
{
    Vector3 nearWindowCoords = Vector3_New(
        (float)screenPosition.x,
        (float)(RMS.window->size.y - screenPosition.y),
        0.0f);

    Vector3 farWindowCoords = Vector3_New(
        (float)screenPosition.x,
        (float)(RMS.window->size.y - screenPosition.y),
        1.0f);

    Vector4 viewport = Vector4_New(0, 0, (float)RMS.window->size.x, (float)RMS.window->size.y);

    mat4 tempMatrix;
    glm_mat4_mul((vec4 *)&RMS.camera.projectionMatrix, (vec4 *)&RMS.camera.viewMatrix, tempMatrix);

    Vector3 nearPoint = Vector3_Zero;
    Vector3 farPoint = Vector3_Zero;
    glm_unproject((float *)&nearWindowCoords, (vec4 *)tempMatrix, (float *)&viewport, (float *)&nearPoint);
    glm_unproject((float *)&farWindowCoords, (vec4 *)tempMatrix, (float *)&viewport, (float *)&farPoint);

    Vector3 rayDirection = Vector3_Normalized(Vector3_Sum(farPoint, Vector3_Scale(nearPoint, -1.0f)));

    if (*RMS.camera.isPerspectiveReference)
    {
        return Vector3_Sum(*RMS.camera.positionReference, Vector3_Scale(rayDirection, depth));
    }
    else
    {
        return Vector3_Sum(Vector3_Sum(nearPoint, Vector3_New(0.0f, 0.0f, -(*RMS.camera.nearClipPlaneReference))), Vector3_Scale(rayDirection, depth));
    }
}

void Renderer_Resize(RJ_Size newBatchCapacity)
{
    RJ_DebugAssert(newBatchCapacity > RMS.data.count, "New batch capacity %u is must be greater than current batch count %u.", newBatchCapacity, RMS.data.count);

    RENDERER_BATCH *newBatches = NULL;
    RJ_DebugAssertAllocationCheck(RENDERER_BATCH, newBatches, newBatchCapacity);

    for (RJ_Size i = 0; i < RMS.data.count; i++)
    {
        newBatches[i] = rmsBatch(i);
    }

    free(RMS.data.batches);
    RMS.data.batches = newBatches;

    RMS.data.capacity = newBatchCapacity;

    RJ_DebugInfo("Renderer resized to new batch capacity of %u successfully.", newBatchCapacity);
}

/*
RendererScene *RendererScene_CreateFromFile(StringView scnFile, const ListArray *modelPool, void *objectReferences, RJ_Size transformOffsetInObject, RJ_Size totalObjectSize, RJ_Size objectCount)
{
    RJ_DebugAssertNullPointerCheck(modelPool);
    RJ_DebugAssertNullPointerCheck(objectReferences);

    RendererScene *scene = NULL;
    RendererBatch *currentBatch = NULL;
    RendererComponent *currentComponent = NULL;
    RJ_Size totalObjectIndex = 0;

    ResourceText *rscScene = ResourceText_Create(scnFile);

    RJ_Size scnLineCount = 0;
    StringView *scnLines = (StringView *)malloc(rscScene->lineCount * sizeof(StringView));
    RJ_DebugAssertNullPointerCheck(scnLines);
    memset(scnLines, 0, rscScene->lineCount * sizeof(StringView));

    RJ_Size scnLineTokenCount = 0;
    StringView scnLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strP = scl("p");
    StringView strR = scl("r");
    StringView strS = scl("s");
    StringView strNEWSCN = scl("newscn");
    StringView strUSEMDL = scl("usemdl");

    String_Tokenize(scv(rscScene->data), strNewline, &scnLineCount, scnLines, rscScene->lineCount);
    for (RJ_Size i = 0; i < scnLineCount; i++)
    {
        String_Tokenize(scnLines[i], strSpace, &scnLineTokenCount, scnLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scnLineTokens[0];

        if (String_Compare(firstToken, strP) == 0)
        {
            RJ_DebugAssert(totalObjectIndex < objectCount, "Object reference count %u is not enough for the imported scene file", objectCount);
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

            for (RJ_Size j = 0; j < modelPool->count; j++)
            {
                RendererModel *model = *(RendererModel **)ListArray_Get(modelPool, j);

                // RJ_DebugInfo("Comparing model '%.*s' with '%.*s'...", model->name.length, model->name.characters, lineTokens[1].length, lineTokens[1].characters);

                if (String_Compare(scv(model->name), scv(scnLineTokens[1])) == 0)
                {
                    modelFound = true;
                    currentBatch = RendererScene_CreateBatch(scene, model, scnLineTokenCount > 2 ? (RJ_Size)String_ToInt(scv(scnLineTokens[2])) : RENDERER_BATCH_INITIAL_CAPACITY);
                    break;
                }
            }

            RJ_DebugAssert(modelFound, "Model '%.*s' not found in model pool when trying to assign it to mesh in model '%.*s'.", scnLineTokens[1].length, scnLineTokens[1].characters, currentBatch->model->name.length, currentBatch->model->name.characters);
        }
    }

    free(scnLines);
    ResourceText_Destroy(rscScene);

    RJ_DebugInfo("Scene %s imported successfully.", scene->name.characters);

    return scene;
}
*/

// todo move this to shader, compute in gpu
void Renderer_Update(void)
{
    glm_mat4_identity((vec4 *)&RMS.camera.projectionMatrix);
    glm_mat4_identity((vec4 *)&RMS.camera.viewMatrix);

    Vector3 direction = Vector3_Normalized(Vector3_New(Maths_Cos(RMS.camera.rotationReference->x) * Maths_Cos(RMS.camera.rotationReference->y),
                                                       Maths_Sin(RMS.camera.rotationReference->x),
                                                       Maths_Cos(RMS.camera.rotationReference->x) * Maths_Sin(RMS.camera.rotationReference->y)));

    Vector3 center = Vector3_Sum(*RMS.camera.positionReference, Vector3_Normalized(direction));

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

    for (RJ_Size batch = 0; batch < RMS.data.count; batch++)
    {
        for (RJ_Size component = 0; component < rmsBatch(batch).data.count; component++)
        {
            // todo send transform data to gpu instead of calculating here

            glm_mat4_identity((vec4 *)&rmsObjectMatrix(batch, component));

            if (!rmsIsActive(batch, component))
            {
                continue;
            }

            glm_translate((vec4 *)&rmsObjectMatrix(batch, component), (float *)&(vec3){(&rmsPositionReference(batch, component))->x, (&rmsPositionReference(batch, component))->y, (&rmsPositionReference(batch, component))->z});

            glm_rotate((vec4 *)&rmsObjectMatrix(batch, component), (&rmsRotationReference(batch, component))->x, (float *)&(vec3){1, 0, 0});
            glm_rotate((vec4 *)&rmsObjectMatrix(batch, component), (&rmsRotationReference(batch, component))->y, (float *)&(vec3){0, 1, 0});
            glm_rotate((vec4 *)&rmsObjectMatrix(batch, component), (&rmsRotationReference(batch, component))->z, (float *)&(vec3){0, 0, 1});

            glm_scale((vec4 *)&rmsObjectMatrix(batch, component), (float *)&(vec3){(&rmsScaleReference(batch, component))->x, (&rmsScaleReference(batch, component))->y, (&rmsScaleReference(batch, component))->z});
        }
    }
}

void Renderer_Render(void)
{
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

    ResourceModel *previousModel = NULL;

    for (RJ_Size batch = 0; batch < RMS.data.count; batch++)
    {
        ResourceMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(sizeof(Resource_Matrix4) * rmsBatch(batch).data.count),
                     rmsBatch(batch).components.objectMatrices,
                     RENDERER_OPENGL_DRAW_TYPE); // todo send transforms

        if (previousModel != rmsBatch(batch).data.model)
        {
            glBufferData(GL_ARRAY_BUFFER,
                         (long long)(rmsBatch(batch).data.model->vertices.sizeOfItem * rmsBatch(batch).data.model->vertices.count),
                         rmsBatch(batch).data.model->vertices.data,
                         RENDERER_OPENGL_DRAW_TYPE);

            previousModel = rmsBatch(batch).data.model;
        }

        for (RJ_Size j = 0; j < rmsBatch(batch).data.model->meshes.count; j++)
        {
            ResourceMesh *mesh = (ResourceMesh *)ListArray_Get(&rmsBatch(batch).data.model->meshes, j);

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

    glfwSwapBuffers(RMS.window->handle);
    glFinish();
}

RendererBatch Renderer_BatchCreate(StringView mdlFile, Vector3 *transformOffset, RJ_Size initialComponentCapacity, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences)
{
    RJ_DebugAssert(RMS.data.count + RMS.data.freeIndices.count < RMS.data.capacity, "Maximum renderer batch capacity of %u reached.", RMS.data.capacity); // todo expand capacity

    RendererBatch newBatch = RJ_INDEX_INVALID;

    newBatch = RMS.data.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&RMS.data.freeIndices)) : RMS.data.count;

    // todo
    ResourceModel_GetOrCreate(&rmsBatch(newBatch).data.model, mdlFile, transformOffset);

    rmsBatch(newBatch).data.capacity = initialComponentCapacity;
    rmsBatch(newBatch).data.count = 0;
    ListArray_Create(&rmsBatch(newBatch).data.freeIndices, "RJ_Size", sizeof(RJ_Size), RENDERER_INITIAL_FREE_INDEX_ARRAY_SIZE);

    RJ_DebugAssertAllocationCheck(RJ_Size, rmsBatch(newBatch).components.entities, initialComponentCapacity);

    RJ_DebugAssertAllocationCheck(Resource_Matrix4, rmsBatch(newBatch).components.objectMatrices, initialComponentCapacity);

    rmsBatch(newBatch).components.positionReferences = positionReferences;
    rmsBatch(newBatch).components.rotationReferences = rotationReferences;
    rmsBatch(newBatch).components.scaleReferences = scaleReferences;

    RJ_DebugAssertAllocationCheck(uint8_t, rmsBatch(newBatch).components.flags, initialComponentCapacity);

    RMS.data.count++;

    return newBatch;
}

void Renderer_BatchDestroy(RendererBatch batch)
{
    rmsAssertBatch(batch);

    rmsBatch(batch).data.model = NULL;

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

void Renderer_BatchConfigureReferences(RendererBatch batch, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences, RJ_Size newComponentCapacity)
{
    rmsAssertBatch(batch);
    RJ_DebugAssertNullPointerCheck(positionReferences);
    RJ_DebugAssertNullPointerCheck(rotationReferences);
    RJ_DebugAssertNullPointerCheck(scaleReferences);
    RJ_DebugAssert(newComponentCapacity > rmsBatch(batch).data.count, "New component capacity %u must be greater than current physics component count %u.", newComponentCapacity, rmsBatch(batch).data.count);

    rmsBatch(batch).data.capacity = newComponentCapacity;

    rmsBatch(batch).components.positionReferences = positionReferences;
    rmsBatch(batch).components.rotationReferences = rotationReferences;
    rmsBatch(batch).components.scaleReferences = scaleReferences;

    RJ_DebugAssertReallocationCheck(RJ_Size, rmsBatch(batch).components.entities, newComponentCapacity);

    RJ_DebugAssertReallocationCheck(Resource_Matrix4, rmsBatch(batch).components.objectMatrices, newComponentCapacity);

    rmsBatch(batch).components.positionReferences = positionReferences;
    rmsBatch(batch).components.rotationReferences = rotationReferences;
    rmsBatch(batch).components.scaleReferences = scaleReferences;

    RJ_DebugAssertReallocationCheck(uint8_t, rmsBatch(batch).components.flags, newComponentCapacity);
}

RendererComponent Renderer_ComponentCreate(RJ_Size entity, RendererBatch batch)
{
    rmsAssertBatch(batch);
    RJ_DebugAssert(rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count < rmsBatch(batch).data.capacity, "Maximum renderer batch %u component capacity of %u reached.", batch, rmsBatch(batch).data.capacity); // todo expand capacity
    //    RJ_DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererComponent newComponent = rmsBatch(batch).data.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&rmsBatch(batch).data.freeIndices)) : rmsBatch(batch).data.count;

    rmsEntity(batch, newComponent) = entity;
    rmsSetActive(batch, newComponent, true);

    rmsBatch(batch).data.count++;

    return newComponent;
}

void Renderer_ComponentDestroy(RendererBatch batch, RendererComponent component)
{
    rmsAssertBatch(batch);
    rmsAssertComponent(batch, component);

    rmsEntity(batch, component) = RJ_INDEX_INVALID;
    rmsFlag(batch, component) = false;

    ListArray_Add(&rmsBatch(batch).data.freeIndices, &component);

    rmsBatch(batch).data.count--;
}

bool Renderer_ComponentIsActive(RendererBatch batch, RendererComponent component)
{
    rmsAssertBatch(batch);
    rmsAssertComponent(batch, component);

    return rmsIsActive(batch, component);
}

void Renderer_ComponentSetActive(RendererBatch batch, RendererComponent component, bool isActive)
{
    rmsAssertBatch(batch);
    rmsAssertComponent(batch, component);

    rmsSetActive(batch, component, isActive);
}

#pragma endregion Renderer

#pragma region RendererDebug

void RendererDebug_Initialize(StringView vertexShaderFile, StringView fragmentShaderFile, RJ_Size initialVertexCapacity)
{
    ListArray_Create(&RMS.debugShader.vertices, "Renderer Debug Vertex", sizeof(RendererDebugVertex), initialVertexCapacity);

    glGenVertexArrays(1, &RMS.debugShader.vao);
    glGenBuffers(1, &RMS.debugShader.vbo);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    ResourceText *rscVertexShader = NULL;
    ResourceText_Create(&rscVertexShader, vertexShaderFile);
    ResourceText *rscFragmentShader = NULL;
    ResourceText_Create(&rscFragmentShader, fragmentShaderFile);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&rscVertexShader->data.characters, NULL);
    glCompileShader(vertexShader);

    ResourceText_Destroy(rscVertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJ_DebugInfo("Debug Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&rscFragmentShader->data.characters, NULL);
    glCompileShader(fragmentShader);

    ResourceText_Destroy(rscFragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJ_DebugInfo("Debug Fragment shader compiled successfully.");

    RMS.debugShader.programHandle = glCreateProgram();
    glAttachShader(RMS.debugShader.programHandle, vertexShader);
    glAttachShader(RMS.debugShader.programHandle, fragmentShader);
    glLinkProgram(RMS.debugShader.programHandle);

    glGetProgramiv(RMS.debugShader.programHandle, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RMS.debugShader.programHandle, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Shader program linking failed. Logs:\n%s", glslInfoLog);

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

    RJ_DebugInfo("Debug Renderer initialized successfully.");
}

void RendererDebug_Terminate(void)
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

    RJ_DebugInfo("Debug Renderer terminated successfully.");
}

void RendererDebug_StartRendering(void)
{
    glUseProgram(RMS.debugShader.programHandle);
}

void RendererDebug_FinishRendering(void)
{
    if (RMS.debugShader.vertices.count == 0)
    {
        return;
    }

    glUseProgram(RMS.debugShader.programHandle);
    glUniformMatrix4fv(RMS.debugShader.camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&RMS.camera.projectionMatrix);
    glUniformMatrix4fv(RMS.debugShader.camViewMatrix, 1, GL_FALSE, (GLfloat *)&RMS.camera.viewMatrix);

    glBindVertexArray(RMS.debugShader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RMS.debugShader.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(RMS.debugShader.vertices.sizeOfItem * RMS.debugShader.vertices.count), RMS.debugShader.vertices.data, RENDERER_OPENGL_DRAW_TYPE);
    glDrawArrays(GL_LINES, 0, (GLsizei)RMS.debugShader.vertices.count);

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
    Vector3 min = Vector3_Sum(position, Vector3_Scale(halfSize, -1.0f));
    Vector3 max = Vector3_Sum(position, halfSize);

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

#pragma endregion RendererDebug
