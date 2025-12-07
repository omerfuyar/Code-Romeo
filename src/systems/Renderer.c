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

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL UNIFORM BLOCK LAYOUT IN VERTEX SHADER
typedef struct ObjectTransform
{
    alignas(16) Vector3 position;
    alignas(16) Vector3 rotation;
    alignas(16) Vector3 scale;
} ObjectTransform;

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
        RJGlobal_Size capacity;
        RJGlobal_Size count;
        ListArray freeIndices; // RJGlobal_Size

        ResourceModel *model;
    } data;

    struct RENDERER_COMPONENTS
    {
        RJGlobal_Size *entities;

        ObjectTransform *objectTransforms;

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

    struct RENDERER_SCENE_DATA
    {
        RJGlobal_Size capacity;
        RJGlobal_Size count;
        ListArray freeIndices; // RJGlobal_Size

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

#define rmsObjectTransform(batch, component) (rmsBatch(batch).components.objectTransforms[component])

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

#pragma endregion Source Only

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window, RJGlobal_Size initialBatchCapacity)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    RMS.window = window;
    RJGlobal_DebugAssertAllocationCheck(RENDERER_BATCH, RMS.data.batches, initialBatchCapacity);

    RMS.data.capacity = initialBatchCapacity;
    RMS.data.count = 0;
    RMS.data.freeIndices = ListArray_Create("Renderer Free Indices", sizeof(RJGlobal_Size), initialBatchCapacity);

    RJGlobal_DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    Context_ConfigureLogCallback(RENDERER_MAIN_WINDOW_LOG_CALLBACK);

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

    RMS.shader.objectMatricesHandle = glGetUniformBlockIndex(RMS.shader.programHandle, "objectTransforms");
    glUniformBlockBinding(RMS.shader.programHandle, RMS.shader.objectMatricesHandle, RENDERER_UBO_MATRICES_BINDING);
    glBindBufferBase(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, RMS.shader.uboObjectMatrices);
    // glBindBufferRange(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices, 0, RENDERER_SCENE_MAX_OBJECT_COUNT);

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

    RJGlobal_DebugInfo("Camera: Pos=(%.2f,%.2f,%.2f) Rot=(%.2f,%.2f,%.2f)",
                       RMS.camera.positionReference->x, RMS.camera.positionReference->y, RMS.camera.positionReference->z,
                       RMS.camera.rotationReference->x, RMS.camera.rotationReference->y, RMS.camera.rotationReference->z);
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

// Matrices calculated in vertex shader now
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

    // Copy transform data to UBO structure for all objects
    // Note: Instanced rendering draws all objects in the batch, including inactive ones.
    // Inactive objects are handled by setting scale to zero, making them not visible.
    for (RJGlobal_Size batch = 0; batch < RMS.data.count; batch++)
    {
        for (RJGlobal_Size component = 0; component < rmsBatch(batch).data.count; component++)
        {
            if (!rmsIsActive(batch, component))
            {
                // Inactive objects: set scale to zero to make them invisible
                // (Previous implementation left inactive objects at identity matrix)
                rmsObjectTransform(batch, component).position = Vector3_New(0, 0, 0);
                rmsObjectTransform(batch, component).rotation = Vector3_New(0, 0, 0);
                rmsObjectTransform(batch, component).scale = Vector3_New(0, 0, 0);
                continue;
            }

            // Copy transform data from references to UBO structure
            rmsObjectTransform(batch, component).position = rmsPositionReference(batch, component);
            rmsObjectTransform(batch, component).rotation = rmsRotationReference(batch, component);
            rmsObjectTransform(batch, component).scale = rmsScaleReference(batch, component);
        }
    }
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
        ResourceMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(sizeof(ObjectTransform) * rmsBatch(batch).data.count),
                     rmsBatch(batch).components.objectTransforms,
                     RENDERER_OPENGL_DRAW_TYPE);

        glBufferData(GL_ARRAY_BUFFER,
                     (long long)(rmsBatch(batch).data.model->vertices.sizeOfItem * rmsBatch(batch).data.model->vertices.count),
                     rmsBatch(batch).data.model->vertices.data,
                     RENDERER_OPENGL_DRAW_TYPE);

        for (RJGlobal_Size j = 0; j < rmsBatch(batch).data.model->meshes.count; j++)
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
}

RendererBatch Renderer_BatchCreate(StringView mdlFile, Vector3 *transformOffset, RJGlobal_Size initialComponentCapacity, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences)
{
    RJGlobal_DebugAssert(RMS.data.count + RMS.data.freeIndices.count < RMS.data.capacity, "Maximum renderer batch capacity of %u reached.", RMS.data.capacity); // todo expand capacity

    RendererBatch newBatch = RJGLOBAL_INDEX_INVALID;

    newBatch = RMS.data.freeIndices.count != 0 ? *((RJGlobal_Size *)ListArray_Pop(&RMS.data.freeIndices)) : RMS.data.count;

    rmsBatch(newBatch).data.model = ResourceModel_GetOrCreate(mdlFile, transformOffset);

    rmsBatch(newBatch).data.capacity = initialComponentCapacity;
    rmsBatch(newBatch).data.count = 0;
    rmsBatch(newBatch).data.freeIndices = ListArray_Create("RJGlobal_Size", sizeof(RJGlobal_Size), RENDERER_INITIAL_FREE_INDEX_ARRAY_SIZE);

    RJGlobal_DebugAssertAllocationCheck(RJGlobal_Size, rmsBatch(newBatch).components.entities, initialComponentCapacity);

    RJGlobal_DebugAssertAllocationCheck(ObjectTransform, rmsBatch(newBatch).components.objectTransforms, initialComponentCapacity);

    rmsBatch(newBatch).components.positionReferences = positionReferences;
    rmsBatch(newBatch).components.rotationReferences = rotationReferences;
    rmsBatch(newBatch).components.scaleReferences = scaleReferences;

    RJGlobal_DebugAssertAllocationCheck(uint8_t, rmsBatch(newBatch).components.flags, initialComponentCapacity);

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
    free(rmsBatch(batch).components.objectTransforms);
    free(rmsBatch(batch).components.flags);

    rmsBatch(batch).components.entities = NULL;
    rmsBatch(batch).components.objectTransforms = NULL;
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

    RJGlobal_DebugAssertReallocationCheck(RJGlobal_Size, rmsBatch(batch).components.entities, newComponentCapacity);

    RJGlobal_DebugAssertReallocationCheck(ObjectTransform, rmsBatch(batch).components.objectTransforms, newComponentCapacity);

    rmsBatch(batch).components.positionReferences = positionReferences;
    rmsBatch(batch).components.rotationReferences = rotationReferences;
    rmsBatch(batch).components.scaleReferences = scaleReferences;

    RJGlobal_DebugAssertReallocationCheck(uint8_t, rmsBatch(batch).components.flags, newComponentCapacity);
}

RendererComponent Renderer_ComponentCreate(RJGlobal_Size entity, RendererBatch batch)
{
    rmsAssertBatch(batch);
    RJGlobal_DebugAssert(rmsBatch(batch).data.count + rmsBatch(batch).data.freeIndices.count < rmsBatch(batch).data.capacity, "Maximum renderer batch %u component capacity of %u reached.", batch, rmsBatch(batch).data.capacity); // todo expand capacity
    //    RJGlobal_DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererComponent newComponent = rmsBatch(batch).data.freeIndices.count != 0 ? *((RJGlobal_Size *)ListArray_Pop(&rmsBatch(batch).data.freeIndices)) : rmsBatch(batch).data.count;

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

void RendererDebug_FinishRendering()
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

#pragma endregion RendererDebug
