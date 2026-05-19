#include "systems/Renderer.h"

#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"
#include "utilities/ListArray.h"
#include "utilities/HashMap.h"

#include "glad/glad.h"
#include "cglm/cglm.h"

#define RENDERER_OPENGL_DRAW_TYPE GL_DYNAMIC_DRAW

#pragma region Source Only

#pragma region typedefs

/// @brief Handle for a shader program object.
typedef uint32_t RendererShaderProgramHandle;
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

typedef struct RendererEntityPair
{
    RendererBatch batch;
    Entity component;
} RendererEntityPair;

/// @brief A batch of render components that use the same model.
typedef struct RENDERER_BATCH
{
    ResourceModel *model;

    struct RENDERER_BATCH_COMPONENTS
    {
        RJ_Size capacity;
        RJ_Size count;

        Entity *compToEntityMap;

        Matrix4 *objectMatrices;
    } data;
} RENDERER_BATCH;

#pragma endregion typedefs

// todo lights

struct RENDERER
{
    const ContextWindow *window;

    struct RENDERER_DATA
    {
        RJ_Size capacity;
        RJ_Size count;

        RendererEntityPair *entityToPairMap;

        RENDERER_BATCH *batches;
        // todo maybe change with linked list for dynamic resizing?
    } data;

    struct RENDERER_CAMERA
    {
        RendererCamera *reference;
        Matrix4 projectionMatrix;
        Matrix4 viewMatrix;
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

        RendererUniformLocationHandle matBaseColorFactor;
        RendererUniformLocationHandle matMetallicFactor;
        RendererUniformLocationHandle matRoughnessFactor;
        RendererUniformLocationHandle matEmissiveFactor;

        RendererUniformLocationHandle matHasBaseColorMap;
        RendererUniformLocationHandle matBaseColorMap;
        RendererUniformLocationHandle matHasMetallicRoughnessMap;
        RendererUniformLocationHandle matMetallicRoughnessMap;

        RendererUniformBlockHandle objectMatricesHandle;
    } shader;
} RENDERER = {0};

#define rBatch(batch) (RENDERER.data.batches[batch])
#define rPair(entity) (RENDERER.data.entityToPairMap[entity])
#define rEntity(pair) (rBatch((pair).batch).data.compToEntityMap[(pair).component])

#define rObjectMatrix(pair) (rBatch((pair).batch).data.objectMatrices[(pair).component])

#define rAssertBatch(batch) RJ_DebugAssert((batch) != RJ_INDEX_INVALID &&                       \
                                               (batch) < RENDERER.data.count,                   \
                                           "Renderer batch %u exceeds maximum batch count %u.", \
                                           (batch), RENDERER.data.count)

#define rAssertEntity(entity) RJ_DebugAssert((entity) != RJ_INDEX_INVALID &&                                                               \
                                                 rPair(entity).component != RJ_INDEX_INVALID &&                                            \
                                                 rEntity(rPair(entity)) == entity &&                                                       \
                                                 rPair(entity).component < rBatch(rPair(entity).batch).data.count,                         \
                                             "Renderer component %u or Entity %u either exceeds maximum possible index %u or is invalid.", \
                                             rPair(entity).component, entity, rBatch(rPair(entity).batch).data.count)

/// @brief
/// @param window
/// @param width
/// @param height
static void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(void *window, int width, int height)
{
    RJ_DebugAssertNullPointerCheck(window);

    // RMS.window->size.x = width;
    // RMS.window->size.y = height;

    // if (RMS.window->fullScreen)
    // {
    //     glfwGetFramebufferSize(RMS.window->handle, &RMS.window->size.x, &RMS.window->size.y);
    // }

    glViewport(0, 0, width, height);
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
        RJ_DebugError(RJ_ERROR_DEPENDENCY, "OpenGL Error :\ntype : 0x%x\nseverity : 0x%x\nmessage : \n%s\n",
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

RJ_ResultWarn Renderer_Initialize(const ContextWindow *window, RJ_Size initialBatchCapacity)
{
    RJ_DebugAssertNullPointerCheck(window);

    RENDERER.window = window;

    RJ_ReturnAllocate(RENDERER_BATCH, RENDERER.data.entityToPairMap, initialBatchCapacity);
    RJ_ReturnAllocate(RENDERER_BATCH, RENDERER.data.batches, initialBatchCapacity);

    RENDERER.data.capacity = initialBatchCapacity;
    RENDERER.data.count = 0;

    RJ_DebugAssert(gladLoadGLLoader((GLADloadproc)Context_GetDynamicSymbolLoader()), "Failed to initialize GLAD");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    glDebugMessageCallback((GLDEBUGPROC)RENDERER_MAIN_WINDOW_LOG_CALLBACK, NULL);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RENDERER.shader.programHandle = glCreateProgram();

    glGenVertexArrays(1, &RENDERER.shader.vao);
    glGenBuffers(1, &RENDERER.shader.vboModelVertices);
    glGenBuffers(1, &RENDERER.shader.iboModelIndices);
    glGenBuffers(1, &RENDERER.shader.uboObjectMatrices);

    glBindVertexArray(RENDERER.shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RENDERER.shader.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RENDERER.shader.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, RENDERER.shader.uboObjectMatrices);

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

    RJ_DebugInfo("Renderer initialized successfully.");
    return RJ_OK;
}

void Renderer_Terminate(void)
{
    RENDERER.window = NULL;

    for (RJ_Size batch = RENDERER.data.count; batch > 0; batch--)
    {
        Renderer_BatchDestroy(batch - 1);
    }

    free(RENDERER.data.entityToPairMap);
    RENDERER.data.entityToPairMap = NULL;

    free(RENDERER.data.batches);
    RENDERER.data.batches = NULL;

    RENDERER.data.capacity = 0;
    RENDERER.data.count = 0;

    RENDERER.camera.reference = NULL;

    if (RENDERER.shader.programHandle != 0)
    {
        glDeleteProgram(RENDERER.shader.programHandle);
    }

    RENDERER.shader.programHandle = 0;

    glDeleteVertexArrays(1, &RENDERER.shader.vao);
    glDeleteBuffers(1, &RENDERER.shader.vboModelVertices);
    glDeleteBuffers(1, &RENDERER.shader.iboModelIndices);
    glDeleteBuffers(1, &RENDERER.shader.uboObjectMatrices);

    RENDERER.shader.vao = 0;
    RENDERER.shader.vboModelVertices = 0;
    RENDERER.shader.iboModelIndices = 0;
    RENDERER.shader.uboObjectMatrices = 0;

    RENDERER.shader.camProjectionMatrix = 0;
    RENDERER.shader.camViewMatrix = 0;
    RENDERER.shader.camPosition = 0;
    RENDERER.shader.camRotation = 0;
    RENDERER.shader.camSize = 0;
    RENDERER.shader.camIsPerspective = 0;

    RENDERER.shader.matBaseColorFactor = 0;
    RENDERER.shader.matMetallicFactor = 0;
    RENDERER.shader.matRoughnessFactor = 0;
    RENDERER.shader.matEmissiveFactor = 0;
    RENDERER.shader.matBaseColorMap = 0;
    RENDERER.shader.matHasBaseColorMap = 0;
    RENDERER.shader.matMetallicRoughnessMap = 0;
    RENDERER.shader.matHasMetallicRoughnessMap = 0;

    RENDERER.shader.objectMatricesHandle = 0;

    RJ_DebugInfo("Renderer terminated successfully.");
}

bool Renderer_IsInitialized(void)
{
    return RENDERER.window != NULL;
}

RJ_ResultWarn Renderer_ConfigureShaders(StringView vertexShaderFile, StringView fragmentShaderFile)
{
    RJ_DebugAssert(RENDERER.shader.programHandle != 0, "Initialize the renderer before configuring shaders.");

    ResourceText *rscVertexShader = NULL;
    RJ_Result result = ResourceText_Create(&rscVertexShader, vertexShaderFile);
    if (result != RJ_OK)
    {
        RJ_DebugWarning("Failed to create renderer vertex shader from file '%s'.", vertexShaderFile.characters);
        return result;
    }

    ResourceText *rscFragmentShader = NULL;
    result = ResourceText_Create(&rscFragmentShader, fragmentShaderFile);
    if (result != RJ_OK)
    {
        ResourceText_Destroy(rscVertexShader);
        RJ_DebugWarning("Failed to create renderer fragment shader from file '%s'.", fragmentShaderFile.characters);
        return result;
    }

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

    glAttachShader(RENDERER.shader.programHandle, vertexShader);
    glAttachShader(RENDERER.shader.programHandle, fragmentShader);
    glLinkProgram(RENDERER.shader.programHandle);

    glGetProgramiv(RENDERER.shader.programHandle, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER.shader.programHandle, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJ_DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RENDERER.shader.camProjectionMatrix = glGetUniformLocation(RENDERER.shader.programHandle, "camProjectionMatrix");
    RENDERER.shader.camViewMatrix = glGetUniformLocation(RENDERER.shader.programHandle, "camViewMatrix");
    RENDERER.shader.camPosition = glGetUniformLocation(RENDERER.shader.programHandle, "camPosition");
    RENDERER.shader.camRotation = glGetUniformLocation(RENDERER.shader.programHandle, "camRotation");
    RENDERER.shader.camSize = glGetUniformLocation(RENDERER.shader.programHandle, "camSize");
    RENDERER.shader.camIsPerspective = glGetUniformLocation(RENDERER.shader.programHandle, "camIsPerspective");

    RENDERER.shader.matBaseColorFactor = glGetUniformLocation(RENDERER.shader.programHandle, "matBaseColorFactor");
    RENDERER.shader.matMetallicFactor = glGetUniformLocation(RENDERER.shader.programHandle, "matMetallicFactor");
    RENDERER.shader.matRoughnessFactor = glGetUniformLocation(RENDERER.shader.programHandle, "matRoughnessFactor");
    RENDERER.shader.matEmissiveFactor = glGetUniformLocation(RENDERER.shader.programHandle, "matEmissiveFactor");
    RENDERER.shader.matBaseColorMap = glGetUniformLocation(RENDERER.shader.programHandle, "matBaseColorMap");
    RENDERER.shader.matHasBaseColorMap = glGetUniformLocation(RENDERER.shader.programHandle, "matHasBaseColorMap");
    RENDERER.shader.matMetallicRoughnessMap = glGetUniformLocation(RENDERER.shader.programHandle, "matMetallicRoughnessMap");
    RENDERER.shader.matHasMetallicRoughnessMap = glGetUniformLocation(RENDERER.shader.programHandle, "matHasMetallicRoughnessMap");

    RENDERER.shader.objectMatricesHandle = glGetUniformBlockIndex(RENDERER.shader.programHandle, "modelMatrices");
    glUniformBlockBinding(RENDERER.shader.programHandle, RENDERER.shader.objectMatricesHandle, RENDERER_UBO_MATRICES_BINDING);
    glBindBufferBase(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, RENDERER.shader.uboObjectMatrices);
    // glBindBufferRange(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices, 0, RENDERER_SCENE_MAX_OBJECT_COUNT);

    RJ_DebugInfo("Shader program linked and created successfully.");
    return RJ_OK;
}

void Renderer_SetCamera(RendererCamera *camera)
{
    RJ_DebugAssertNullPointerCheck(camera);

    RENDERER.camera.reference = camera;
}

RJ_ResultWarn Renderer_GetCamera(RendererCamera **retCamera)
{
    RJ_DebugAssertNullPointerCheck(retCamera);

    if (RENDERER.camera.reference == NULL)
    {
        *retCamera = NULL;
        RJ_DebugWarning("Internal renderer camera is NULL");
        return RJ_ERROR_INTERNAL;
    }

    *retCamera = RENDERER.camera.reference;
    return RJ_OK;
}

Vector3 Renderer_ScreenToWorldSpace(Vector2Int screenPosition, float depth)
{
    Vector3 nearWindowCoords = Vector3_New(
        (float)screenPosition.x,
        (float)(RENDERER.window->size.y - screenPosition.y),
        0.0f);

    Vector3 farWindowCoords = Vector3_New(
        (float)screenPosition.x,
        (float)(RENDERER.window->size.y - screenPosition.y),
        1.0f);

    Vector4 viewport = Vector4_New(0, 0, (float)RENDERER.window->size.x, (float)RENDERER.window->size.y);

    mat4 tempMatrix;
    glm_mat4_mul((vec4 *)&RENDERER.camera.projectionMatrix, (vec4 *)&RENDERER.camera.viewMatrix, tempMatrix);

    Vector3 nearPoint = Vector3_Zero;
    Vector3 farPoint = Vector3_Zero;
    glm_unproject((float *)&nearWindowCoords, (vec4 *)tempMatrix, (float *)&viewport, (float *)&nearPoint);
    glm_unproject((float *)&farWindowCoords, (vec4 *)tempMatrix, (float *)&viewport, (float *)&farPoint);

    Vector3 rayDirection = Vector3_Normalized(Vector3_Sum(farPoint, Vector3_Scale(nearPoint, -1.0f)));

    if (RENDERER.camera.reference->isPerspective)
    {
        return Vector3_Sum(RENDERER.camera.reference->position, Vector3_Scale(rayDirection, depth));
    }
    else
    {
        return Vector3_Sum(Vector3_Sum(nearPoint, Vector3_New(0.0f, 0.0f, -(RENDERER.camera.reference->nearClipPlane))), Vector3_Scale(rayDirection, depth));
    }
}

RJ_ResultWarn Renderer_Resize(RJ_Size newBatchCapacity)
{
    RJ_DebugAssert(newBatchCapacity > RENDERER.data.count, "New batch capacity %u is must be greater than current batch count %u.", newBatchCapacity, RENDERER.data.count);

    RJ_ReturnAllocate(RENDERER_BATCH, RENDERER.data.entityToPairMap, newBatchCapacity);
    RJ_ReturnAllocate(RENDERER_BATCH, RENDERER.data.batches, newBatchCapacity);

    RENDERER.data.capacity = newBatchCapacity;

    RJ_DebugInfo("Renderer resized to new batch capacity of %u successfully.", newBatchCapacity);
    return RJ_OK;
}

// todo move this to shader, compute in gpu
void Renderer_Update(void)
{
    glm_mat4_identity((vec4 *)&RENDERER.camera.projectionMatrix);
    glm_mat4_identity((vec4 *)&RENDERER.camera.viewMatrix);

    Vector3 direction = Vector3_Normalized(Vector3_New(Maths_Cos(RENDERER.camera.reference->rotation.x) * Maths_Cos(RENDERER.camera.reference->rotation.y),
                                                       Maths_Sin(RENDERER.camera.reference->rotation.x),
                                                       Maths_Cos(RENDERER.camera.reference->rotation.x) * Maths_Sin(RENDERER.camera.reference->rotation.y)));

    Vector3 center = Vector3_Sum(RENDERER.camera.reference->position, Vector3_Normalized(direction));

    glm_lookat((float *)&RENDERER.camera.reference->position, (float *)&center, (float *)&(vec3){0, 1, 0}, (vec4 *)&RENDERER.camera.viewMatrix);
    if (RENDERER.camera.reference->isPerspective)
    {
        glm_perspective(Maths_DegToRad(RENDERER.camera.reference->size),
                        (float)RENDERER.window->size.x / (float)RENDERER.window->size.y,
                        RENDERER.camera.reference->nearClipPlane,
                        RENDERER.camera.reference->farClipPlane,
                        (vec4 *)&RENDERER.camera.projectionMatrix);
    }
    else
    {
        float sizeX = (float)RENDERER.window->size.x * RENDERER.camera.reference->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)RENDERER.window->size.y * RENDERER.camera.reference->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;

        glm_ortho(-sizeX, sizeX,
                  -sizeY, sizeY,
                  RENDERER.camera.reference->nearClipPlane,
                  RENDERER.camera.reference->farClipPlane,
                  (vec4 *)&RENDERER.camera.projectionMatrix);
    }

    for (RJ_Size batch = 0; batch < RENDERER.data.count; batch++)
    {
        for (RJ_Size component = 0; component < rBatch(batch).data.count; component++)
        {
            RendererEntityPair pair = {batch, component};
            // todo send transform data to gpu instead of calculating here

            glm_mat4_identity((vec4 *)&rObjectMatrix(pair));

            Vector3 componentPos = Entity_GetPosition(rEntity(pair));
            glm_translate((vec4 *)&rObjectMatrix(pair), (float *)&(vec3){componentPos.x, componentPos.y, componentPos.z});

            Vector3 componentRot = Entity_GetRotation(rEntity(pair));
            glm_rotate((vec4 *)&rObjectMatrix(pair), componentRot.x, (float *)&(vec3){1, 0, 0});
            glm_rotate((vec4 *)&rObjectMatrix(pair), componentRot.y, (float *)&(vec3){0, 1, 0});
            glm_rotate((vec4 *)&rObjectMatrix(pair), componentRot.z, (float *)&(vec3){0, 0, 1});

            Vector3 componentScl = Entity_GetScale(rEntity(pair));
            glm_scale((vec4 *)&rObjectMatrix(pair), (float *)&(vec3){componentScl.x, componentScl.y, componentScl.z});
        }
    }
}

void Renderer_Render(void)
{
    glClearColor(RENDERER_OPENGL_CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RENDERER.shader.programHandle);

    glBindVertexArray(RENDERER.shader.vao);
    glBindBuffer(GL_ARRAY_BUFFER, RENDERER.shader.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RENDERER.shader.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, RENDERER.shader.uboObjectMatrices);

    glUniformMatrix4fv(RENDERER.shader.camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&RENDERER.camera.projectionMatrix);
    glUniformMatrix4fv(RENDERER.shader.camViewMatrix, 1, GL_FALSE, (GLfloat *)&RENDERER.camera.viewMatrix);
    glUniform3fv(RENDERER.shader.camPosition, 1, (GLfloat *)&RENDERER.camera.reference->position);
    glUniform3fv(RENDERER.shader.camRotation, 1, (GLfloat *)&RENDERER.camera.reference->rotation);
    glUniform1f(RENDERER.shader.camSize, RENDERER.camera.reference->size);
    glUniform1i(RENDERER.shader.camIsPerspective, RENDERER.camera.reference->isPerspective);

    ResourceModel *previousModel = NULL;

    for (RJ_Size batch = 0; batch < RENDERER.data.count; batch++)
    {
        glBufferData(GL_UNIFORM_BUFFER,
                     (GLsizeiptr)(sizeof(Matrix4) * rBatch(batch).data.count),
                     rBatch(batch).data.objectMatrices,
                     RENDERER_OPENGL_DRAW_TYPE); // todo send transforms

        if (previousModel != rBatch(batch).model)
        {
            // todo dont upload static model data every frame upload on batch creation
            // todo  move buffer objects to batch struct
            glBufferData(GL_ARRAY_BUFFER,
                         (GLsizeiptr)(rBatch(batch).model->vertices.sizeOfItem * rBatch(batch).model->vertices.count),
                         rBatch(batch).model->vertices.data,
                         RENDERER_OPENGL_DRAW_TYPE);

            previousModel = rBatch(batch).model;
        }

        ResourceMaterial *previousMaterial = NULL;

        for (RJ_Size j = 0; j < rBatch(batch).model->meshes.count; j++)
        {
            ResourceMesh *mesh = (ResourceMesh *)ListArray_Get(&rBatch(batch).model->meshes, j);

            if (mesh->material != previousMaterial)
            {
                glUniform4fv(RENDERER.shader.matBaseColorFactor, 1, (GLfloat *)&mesh->material->baseColorFactor);
                glUniform1f(RENDERER.shader.matMetallicFactor, mesh->material->metallicFactor);
                glUniform1f(RENDERER.shader.matRoughnessFactor, mesh->material->roughnessFactor);
                glUniform3fv(RENDERER.shader.matEmissiveFactor, 1, (GLfloat *)&mesh->material->emissiveFactor);

                if (mesh->material->baseColorMap != NULL)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->material->baseColorMap->handle);
                    glUniform1i(RENDERER.shader.matBaseColorMap, 0);
                    glUniform1i(RENDERER.shader.matHasBaseColorMap, 1);
                }
                else
                {
                    glUniform1i(RENDERER.shader.matHasBaseColorMap, 0);
                }

                if (mesh->material->metallicRoughnessMap != NULL)
                {
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mesh->material->metallicRoughnessMap->handle);
                    glUniform1i(RENDERER.shader.matMetallicRoughnessMap, 1);
                    glUniform1i(RENDERER.shader.matHasMetallicRoughnessMap, 1);
                }
                else
                {
                    glUniform1i(RENDERER.shader.matHasMetallicRoughnessMap, 0);
                }

                previousMaterial = mesh->material;
            }

            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         (GLsizeiptr)(mesh->indices.sizeOfItem * mesh->indices.count),
                         mesh->indices.data,
                         RENDERER_OPENGL_DRAW_TYPE);

            glDrawElementsInstanced(GL_TRIANGLES,
                                    (GLsizei)mesh->indices.count,
                                    GL_UNSIGNED_INT,
                                    0,
                                    (GLsizei)rBatch(batch).data.count);
        }
    }

    Context_SwapBuffers();
}

RJ_ResultWarn Renderer_BatchCreate(RendererBatch *retBatch, StringView modelFile, RJ_Size initialComponentCapacity)
{
    RJ_DebugAssert(RENDERER.data.count < RENDERER.data.capacity, "Maximum renderer batch capacity of %u reached.", RENDERER.data.capacity); // todo expand capacity

    RendererBatch newBatch = RENDERER.data.count;

    RJ_Result result = ResourceModel_Create(&rBatch(newBatch).model, modelFile);
    if (result != RJ_OK)
    {
        RJ_DebugWarning("Failed to create renderer batch for model file '%s'.", modelFile.characters);
        return result;
    }

    rBatch(newBatch).data.capacity = initialComponentCapacity;
    rBatch(newBatch).data.count = 0;

    RJ_ReturnAllocate(RJ_Size, rBatch(newBatch).data.compToEntityMap, initialComponentCapacity);

    RJ_ReturnAllocate(Matrix4, rBatch(newBatch).data.objectMatrices, initialComponentCapacity,
                      free(rBatch(newBatch).data.compToEntityMap););

    memset(rBatch(newBatch).data.compToEntityMap, 0xff, sizeof(RJ_Size) * initialComponentCapacity);

    RENDERER.data.count++;

    *retBatch = newBatch;
    return RJ_OK;
}

void Renderer_BatchDestroy(RendererBatch batch)
{
    // todo cleanup all components
    rAssertBatch(batch);

    // todo refcount ResourceModel_Destroy(rBatch(batch).model);
    rBatch(batch).model = NULL;

    rBatch(batch).data.capacity = 0;
    rBatch(batch).data.count = 0;

    free(rBatch(batch).data.compToEntityMap);
    free(rBatch(batch).data.objectMatrices);

    rBatch(batch).data.compToEntityMap = NULL;
    rBatch(batch).data.objectMatrices = NULL;

    RENDERER.data.count--;
}

RJ_ResultWarn Renderer_BatchResize(RendererBatch batch, RJ_Size newComponentCapacity)
{
    rAssertBatch(batch);
    RJ_DebugAssert(newComponentCapacity > rBatch(batch).data.count, "New component capacity %u must be greater than current physics component count %u.", newComponentCapacity, rBatch(batch).data.count);

    rBatch(batch).data.capacity = newComponentCapacity;

    RJ_ReturnReallocate(RJ_Size, rBatch(batch).data.compToEntityMap, newComponentCapacity);

    RJ_ReturnReallocate(Matrix4, rBatch(batch).data.objectMatrices, newComponentCapacity,
                        free(rBatch(batch).data.compToEntityMap););

    memset(rBatch(batch).data.compToEntityMap, 0xff, sizeof(RJ_Size) * newComponentCapacity);

    return RJ_OK;
}

void Renderer_ComponentCreate(RendererBatch batch, Entity entity)
{
    rAssertBatch(batch);
    RJ_DebugAssert(rBatch(batch).data.count < rBatch(batch).data.capacity,
                   "Maximum renderer batch %u component capacity of %u reached.", batch, rBatch(batch).data.capacity); // todo expand capacity

    Entity component = rBatch(batch).data.count;

    rEntity(((RendererEntityPair){batch, component})) = entity;

    rBatch(batch).data.count++;
}

void Renderer_ComponentDestroy(Entity entity)
{
    rAssertEntity(entity);

    rEntity(rPair(entity)) = RJ_INDEX_INVALID;

    rBatch(rPair(entity).batch).data.count--;
}

#pragma endregion Renderer
