#include "systems/Renderer.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#pragma region Source Only

#define RENDERER_OPENGL_DRAW_TYPE GL_DYNAMIC_DRAW

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

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (RENDERER DEBUG INITIALIZE)
typedef struct RendererDebugVertex
{
    Vector3 vertexPosition;
    Color vertexColor;
} RendererDebugVertex;

/// @brief Texture object for the renderer.
typedef struct RendererTexture
{
    String name;
    size_t index;
    void *data;
    Vector2Int size;
    int channels;
    RendererTextureHandle handle;
} RendererTexture;

ContextWindow *RENDERER_MAIN_WINDOW = NULL;
RendererShaderProgramHandle RENDERER_MAIN_SHADER_PROGRAM = 0;
ListArray RENDERER_MAIN_TEXTURES = {0}; // RendererTexture

RendererVAOHandle RENDERER_DEBUG_VAO = 0;
RendererVBOHandle RENDERER_DEBUG_VBO = 0;
ListArray RENDERER_DEBUG_VERTICES = {0}; // RendererDebugVertex
RendererShaderProgramHandle RENDERER_DEBUG_SHADER_PROGRAM = 0;
RendererUniformLocationHandle RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX = 0;
RendererUniformLocationHandle RENDERER_DEBUG_UNIFORM_VIEW_MATRIX = 0;

/// @brief
/// @param window
/// @param width
/// @param height
void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(void *window, int width, int height)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    RENDERER_MAIN_WINDOW->size.x = width;
    RENDERER_MAIN_WINDOW->size.y = height;

    if (RENDERER_MAIN_WINDOW->fullScreen)
    {
        glfwGetFramebufferSize(RENDERER_MAIN_WINDOW->handle, &RENDERER_MAIN_WINDOW->size.x, &RENDERER_MAIN_WINDOW->size.y);
    }

    glViewport(0, 0, RENDERER_MAIN_WINDOW->size.x, RENDERER_MAIN_WINDOW->size.y);
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
    for (size_t i = 0; i < RENDERER_MAIN_TEXTURES.count; i++)
    {
        RendererTexture *currentTexture = (RendererTexture *)ListArray_Get(&RENDERER_MAIN_TEXTURES, i);

        if (String_Compare(scv(currentTexture->name), name) == 0)
        {
            RJGlobal_DebugInfo("Texture '%.*s' found in texture pool, reusing it.", (int)name.length, name.characters);
            return currentTexture;
        }
    }

    RJGlobal_DebugAssertNullPointerCheck(data);

    RendererTexture *texture = (RendererTexture *)ListArray_Add(&RENDERER_MAIN_TEXTURES, NULL);
    texture->index = RENDERER_MAIN_TEXTURES.count - 1;
    texture->size = size;
    texture->channels = channels;
    texture->name = scc(name);

    size_t dataSize = (size_t)(size.x * size.y * channels);

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

void RendererTexture_Destroy(RendererTexture *texture)
{
    RJGlobal_DebugAssertNullPointerCheck(texture);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, texture->name.length), texture->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, texture->name.length)] = '\0';

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

RendererMesh *RendererMesh_CreateEmpty(size_t initialIndexCapacity, RendererMaterial *material)
{
    RendererMesh *mesh = malloc(sizeof(RendererMesh));
    RJGlobal_DebugAssertNullPointerCheck(mesh);

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

#pragma endregion Source Only

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window, size_t initialTextureCapacity)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    RENDERER_MAIN_WINDOW = window;
    RENDERER_MAIN_TEXTURES = ListArray_Create("Renderer Texture", sizeof(RendererTexture), initialTextureCapacity);

    RJGlobal_DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    Context_ConfigureLogCallback(RENDERER_MAIN_WINDOW_LOG_CALLBACK);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    RJGlobal_DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate()
{
    for (size_t i = 0; i < RENDERER_MAIN_TEXTURES.count; i++)
    {
        RendererTexture *texture = (RendererTexture *)ListArray_Get(&RENDERER_MAIN_TEXTURES, i);

        RendererTexture_Destroy(texture);
    }

    if (RENDERER_MAIN_TEXTURES.data != NULL)
    {
        ListArray_Destroy(&RENDERER_MAIN_TEXTURES);
    }

    if (RENDERER_MAIN_SHADER_PROGRAM != 0)
    {
        glDeleteProgram(RENDERER_MAIN_SHADER_PROGRAM);
    }

    RENDERER_MAIN_SHADER_PROGRAM = 0;

    RJGlobal_DebugInfo("Renderer terminated successfully.");
}

void Renderer_ConfigureShaders(StringView vertexShader, StringView fragmentShader)
{
    if (RENDERER_MAIN_SHADER_PROGRAM != 0)
    {
        glDeleteProgram(RENDERER_MAIN_SHADER_PROGRAM);
    }

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&vertexShaderSource.characters, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Fragment shader compiled successfully.");

    RENDERER_MAIN_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_MAIN_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_MAIN_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER_MAIN_SHADER_PROGRAM, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RJGlobal_DebugInfo("Shader program linked and created successfully.");
}

void Renderer_StartRendering()
{
    if (glfwWindowShouldClose(RENDERER_MAIN_WINDOW->handle))
    {
        RJGlobal_DebugInfo("Main window close input received");
        RJGlobal_Terminate(EXIT_SUCCESS, "Main window close input received");
    }

    glClearColor(RENDERER_OPENGL_CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);
}

void Renderer_FinishRendering()
{
    glfwSwapBuffers(RENDERER_MAIN_WINDOW->handle);
}

void Renderer_RenderScene(RendererScene *scene)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    glBindVertexArray(scene->vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene->vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->uboObjectMatrices);

    glUniformMatrix4fv(scene->camera->scene->camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&scene->camera->projectionMatrix);
    glUniformMatrix4fv(scene->camera->scene->camViewMatrix, 1, GL_FALSE, (GLfloat *)&scene->camera->viewMatrix);
    glUniform3fv(scene->camera->scene->camPosition, 1, (GLfloat *)scene->camera->positionReference);
    glUniform3fv(scene->camera->scene->camRotation, 1, (GLfloat *)scene->camera->rotationReference);

    for (size_t i = 0; i < scene->batches.count; i++)
    {
        RendererBatch *batch = (RendererBatch *)ListArray_Get(&scene->batches, i);
        RendererMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(batch->objectMatrices.sizeOfItem * batch->objectMatrices.count),
                     batch->objectMatrices.data,
                     RENDERER_OPENGL_DRAW_TYPE);

        glBufferData(GL_ARRAY_BUFFER,
                     (long long)(batch->model->vertices.sizeOfItem * batch->model->vertices.count),
                     batch->model->vertices.data,
                     RENDERER_OPENGL_DRAW_TYPE);

        for (size_t j = 0; j < batch->model->meshes.count; j++)
        {
            RendererMesh *mesh = *(RendererMesh **)ListArray_Get(&batch->model->meshes, j);

            if (mesh->material != previousMaterial)
            {
                glUniform3fv(scene->matAmbientColor, 1, (GLfloat *)&mesh->material->ambientColor);
                glUniform3fv(scene->matDiffuseColor, 1, (GLfloat *)&mesh->material->diffuseColor);
                glUniform3fv(scene->matSpecularColor, 1, (GLfloat *)&mesh->material->specularColor);
                glUniform3fv(scene->matEmissiveColor, 1, (GLfloat *)&mesh->material->emissiveColor);
                glUniform1f(scene->matSpecularExponent, mesh->material->specularExponent);
                glUniform1f(scene->matDissolve, mesh->material->dissolve);

                // Texture binding
                if (mesh->material->diffuseMap != NULL)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->material->diffuseMap->handle);
                    glUniform1i(scene->matDiffuseMap, 0);
                    glUniform1i(scene->matHasDiffuseMap, 1);
                }
                else
                {
                    glUniform1i(scene->matHasDiffuseMap, 0);
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
                                    (GLsizei)batch->objectMatrices.count);
        }
    }

    // RendererDebug_CheckError();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // RJGlobal_DebugInfo("Scene '%s' rendered", scene.name.characters);
}

#pragma endregion Renderer

#pragma region Renderer Debug

void RendererDebug_Initialize(StringView vertexShaderSource, StringView fragmentShaderSource, size_t initialVertexCapacity)
{
    RENDERER_DEBUG_VERTICES = ListArray_Create("Renderer Debug Vertex", sizeof(RendererDebugVertex), initialVertexCapacity);

    glGenVertexArrays(1, &RENDERER_DEBUG_VAO);
    glGenBuffers(1, &RENDERER_DEBUG_VBO);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&vertexShaderSource.characters, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Debug Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    RJGlobal_DebugInfo("Debug Fragment shader compiled successfully.");

    RENDERER_DEBUG_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_DEBUG_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_DEBUG_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_DEBUG_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_DEBUG_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER_DEBUG_SHADER_PROGRAM, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    RJGlobal_DebugAssert(glslHasCompiled != GL_FALSE, "Debug Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX = glGetUniformLocation(RENDERER_DEBUG_SHADER_PROGRAM, "camProjectionMatrix");
    RENDERER_DEBUG_UNIFORM_VIEW_MATRIX = glGetUniformLocation(RENDERER_DEBUG_SHADER_PROGRAM, "camViewMatrix");

    glBindVertexArray(RENDERER_DEBUG_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, RENDERER_DEBUG_VBO);

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
    if (RENDERER_DEBUG_VBO != 0)
    {
        glDeleteBuffers(1, &RENDERER_DEBUG_VBO);
    }

    if (RENDERER_DEBUG_VAO != 0)
    {
        glDeleteVertexArrays(1, &RENDERER_DEBUG_VAO);
    }

    if (RENDERER_DEBUG_SHADER_PROGRAM != 0)
    {
        glDeleteProgram(RENDERER_DEBUG_SHADER_PROGRAM);
    }

    ListArray_Destroy(&RENDERER_DEBUG_VERTICES);

    RENDERER_DEBUG_VAO = 0;
    RENDERER_DEBUG_VBO = 0;
    RENDERER_DEBUG_SHADER_PROGRAM = 0;
    RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX = 0;
    RENDERER_DEBUG_UNIFORM_VIEW_MATRIX = 0;

    RJGlobal_DebugInfo("Debug Renderer terminated successfully.");
}

void RendererDebug_StartRendering()
{
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);
}

void RendererDebug_FinishRendering(const Renderer_Matrix4 *camProjectionMatrix, const Renderer_Matrix4 *camViewMatrix)
{
    if (RENDERER_DEBUG_VERTICES.count == 0)
    {
        return;
    }

    glUseProgram(RENDERER_DEBUG_SHADER_PROGRAM);
    glUniformMatrix4fv(RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX, 1, GL_FALSE, (GLfloat *)camProjectionMatrix);
    glUniformMatrix4fv(RENDERER_DEBUG_UNIFORM_VIEW_MATRIX, 1, GL_FALSE, (GLfloat *)camViewMatrix);

    glBindVertexArray(RENDERER_DEBUG_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, RENDERER_DEBUG_VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(RENDERER_DEBUG_VERTICES.sizeOfItem * RENDERER_DEBUG_VERTICES.count), RENDERER_DEBUG_VERTICES.data, RENDERER_OPENGL_DRAW_TYPE);

    glDrawArrays(GL_LINES, 0, (GLsizei)RENDERER_DEBUG_VERTICES.count);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ListArray_Clear(&RENDERER_DEBUG_VERTICES);
}

void RendererDebug_DrawLine(Vector3 start, Vector3 end, Color color)
{
    RendererDebugVertex startVertex = (RendererDebugVertex){.vertexColor = color, .vertexPosition = start};
    RendererDebugVertex endVertex = (RendererDebugVertex){.vertexColor = color, .vertexPosition = end};

    ListArray_Add(&RENDERER_DEBUG_VERTICES, &startVertex);
    ListArray_Add(&RENDERER_DEBUG_VERTICES, &endVertex);
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

#pragma region Renderer Material

ListArray RendererMaterial_CreateFromFile(StringView matFileData, size_t matFileLineCount)
{
    size_t materialCount = 0;

    size_t mtlLineCount = 0;
    StringView *mtlLines = (StringView *)malloc(matFileLineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(mtlLines);
    RJGlobal_MemorySet(mtlLines, matFileLineCount * sizeof(StringView), 0);

    size_t mtlLineTokenCount = 0;
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
    for (size_t j = 0; j < mtlLineCount; j++) // count
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

    for (size_t j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            currentMaterial = (RendererMaterial *)malloc(sizeof(RendererMaterial));
            RJGlobal_DebugAssertNullPointerCheck(currentMaterial);

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

    return materials;
}

ListArray RendererMaterial_CreateFromFileTextured(StringView matFileData, size_t matFileLineCount, StringView textureName, const void *textureData, Vector2Int textureSize, int textureChannels)
{
    size_t materialCount = 0;

    size_t mtlLineCount = 0;
    StringView *mtlLines = (StringView *)malloc(matFileLineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(mtlLines);
    RJGlobal_MemorySet(mtlLines, matFileLineCount * sizeof(StringView), 0);

    size_t mtlLineTokenCount = 0;
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
    for (size_t j = 0; j < mtlLineCount; j++)
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

    for (size_t j = 0; j < mtlLineCount; j++)
    {
        String_Tokenize(scv(mtlLines[j]), strSpace, &mtlLineTokenCount, mtlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView mtlFirstToken = scv(mtlLineTokens[0]);

        if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
        {
            currentMaterial = (RendererMaterial *)malloc(sizeof(RendererMaterial));
            RJGlobal_DebugAssertNullPointerCheck(currentMaterial);

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
    size_t faceDataCount;
    String_Tokenize(faceComponent, scl("/"), &faceDataCount, faceData, 4);

    int createdVertexIndex = String_ToInt(faceData[0]);
    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

    RendererMeshVertex *vertex = (RendererMeshVertex *)ListArray_Get(&model->vertices, (size_t)vertexIndex);

    if (faceDataCount > 1 && faceData[1].length != 0)
    {
        int createdUIndex = String_ToInt(faceData[1]);
        unsigned int uvIndex = createdUIndex < 0 ? (unsigned int)globalVertexUvPool->count + (unsigned int)createdUIndex : (unsigned int)createdUIndex - 1;
        vertex->vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (size_t)uvIndex);
    }

    if (faceDataCount > 2 && faceData[2].length != 0)
    {
        int createdNormalIndex = String_ToInt(faceData[2]);
        unsigned int normalIndex = createdNormalIndex < 0 ? (unsigned int)globalVertexNormalPool->count + (unsigned int)createdNormalIndex : (unsigned int)createdNormalIndex - 1;
        vertex->vertexNormal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (size_t)normalIndex);
    }

    ListArray_Add(&currentMesh->indices, &vertexIndex);
}

RendererModel *RendererModel_CreateEmpty(StringView name, size_t initialMeshCapacity, size_t initialVertexCapacity)
{
    RendererModel *model = malloc(sizeof(RendererModel));
    RJGlobal_DebugAssertNullPointerCheck(model);

    model->name = scc(name);
    model->vertices = ListArray_Create("Renderer Mesh Vertex", sizeof(RendererMeshVertex), initialVertexCapacity);
    model->meshes = ListArray_Create("Renderer Mesh Pointer", sizeof(RendererMesh *), initialMeshCapacity);

    return model;
}

RendererModel *RendererModel_Create(StringView mdlFileData, size_t mdlFileLineCount, const ListArray *materialPool, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset)
{
    Renderer_Matrix4 offsetMatrix = {0};
    TRANSFORM_TO_MODEL_MATRIX(&offsetMatrix, &positionOffset, &rotationOffset, &scaleOffset);

    size_t meshCount = 0;
    String modelName = {0};

    size_t totalVertexPositionCount = 0;
    size_t totalVertexNormalCount = 0;
    size_t totalVertexUvCount = 0;

    StringView *mdlLines = (StringView *)malloc(mdlFileLineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(mdlLines);
    RJGlobal_MemorySet(mdlLines, mdlFileLineCount * sizeof(StringView), 0);

    size_t lineTokenCount = 0;
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

    String_Tokenize(mdlFileData, strNewline, &mdlFileLineCount, mdlLines, mdlFileLineCount);

    for (size_t i = 0; i < mdlFileLineCount; i++) // count and create materials
    {
        String_Tokenize(mdlLines[i], strSpace, &lineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

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

    size_t *faceCounts = (size_t *)malloc(meshCount * sizeof(size_t));
    RJGlobal_DebugAssertNullPointerCheck(faceCounts);
    RJGlobal_MemorySet(faceCounts, meshCount * sizeof(size_t), 0);

    {
        size_t tempMeshIndex = 0;
        for (size_t i = 0; i < mdlFileLineCount && tempMeshIndex <= meshCount; i++) // count faces
        {
            String_Tokenize(scv(mdlLines[i]), strSpace, &lineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

            StringView firstToken = scv(mdlLineTokens[0]);

            if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
            {
                faceCounts[tempMeshIndex - 1] += lineTokenCount == 4 ? 1 : 2;
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

    for (size_t i = 0; i < mdlFileLineCount; i++) // create global pools
    {
        String_Tokenize(scv(mdlLines[i]), strSpace, &lineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

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

    for (size_t i = 0; i < mdlFileLineCount; i++) // add data to model
    {
        String_Tokenize(scv(mdlLines[i]), scl(" "), &lineTokenCount, mdlLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scv(mdlLineTokens[0]);

        if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            if (lineTokenCount == 4) // 3 vertex face (triangle)
            {
                ProcessFaceVertex(scv(mdlLineTokens[1]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[2]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
                ProcessFaceVertex(scv(mdlLineTokens[3]), model, currentMesh, &globalVertexUvPool, &globalVertexNormalPool);
            }
            else if (lineTokenCount == 5) // 4 vertex face (quad), triangulate it
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

            for (size_t j = 0; j < materialPool->count; j++)
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

    RJGlobal_DebugInfo("Renderer Model '%s' imported successfully with %zu child meshes.", model->name.characters, model->meshes.count);

    return model;
}

ListArray RendererModel_CreateFromFile(StringView mdlFileData, size_t mdlFileLineCount, const ListArray *materialPool)
{
    size_t modelCount = 0;

    StringView *mdlLines = (StringView *)malloc(mdlFileLineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(mdlLines);
    RJGlobal_MemorySet(mdlLines, mdlFileLineCount * sizeof(StringView), 0);

    size_t mdlLineTokenCount = 0;
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

    String_Tokenize(mdlFileData, strNewline, &mdlFileLineCount, mdlLines, mdlFileLineCount);
    for (size_t j = 0; j < mdlFileLineCount; j++) // count models
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

    size_t *vertexPositionCounts = (size_t *)malloc(modelCount * sizeof(size_t));
    RJGlobal_DebugAssertNullPointerCheck(vertexPositionCounts);
    RJGlobal_MemorySet(vertexPositionCounts, modelCount * sizeof(size_t), 0);

    size_t *vertexUvCounts = (size_t *)malloc(modelCount * sizeof(size_t));
    RJGlobal_DebugAssertNullPointerCheck(vertexUvCounts);
    RJGlobal_MemorySet(vertexUvCounts, modelCount * sizeof(size_t), 0);

    size_t *vertexNormalCounts = (size_t *)malloc(modelCount * sizeof(size_t));
    RJGlobal_DebugAssertNullPointerCheck(vertexNormalCounts);
    RJGlobal_MemorySet(vertexNormalCounts, modelCount * sizeof(size_t), 0);

    size_t *meshCounts = (size_t *)malloc(modelCount * sizeof(size_t));
    RJGlobal_DebugAssertNullPointerCheck(meshCounts);
    RJGlobal_MemorySet(meshCounts, modelCount * sizeof(size_t), 0);

    String *modelNames = (String *)malloc(modelCount * sizeof(String));
    RJGlobal_DebugAssertNullPointerCheck(modelNames);
    RJGlobal_MemorySet(modelNames, modelCount * sizeof(String), 0);

    {
        size_t tempModelIndex = 0;
        for (size_t i = 0; i < mdlFileLineCount; i++)
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

    size_t **faceCounts = (size_t **)malloc(modelCount * sizeof(size_t *));
    RJGlobal_DebugAssertNullPointerCheck(faceCounts);
    RJGlobal_MemorySet(faceCounts, modelCount * sizeof(size_t *), 0);

    {
        size_t tempModelIndex = 0;
        size_t tempMeshIndex = 0;

        for (size_t i = 0; i < mdlFileLineCount; i++)
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
                faceCounts[tempModelIndex - 1] = (size_t *)malloc(meshCounts[tempModelIndex - 1] * sizeof(size_t));
                RJGlobal_DebugAssertNullPointerCheck(faceCounts[tempModelIndex - 1]);
                RJGlobal_MemorySet(faceCounts[tempModelIndex - 1], meshCounts[tempModelIndex - 1] * sizeof(size_t), 0);
            }
        }
    }

    RendererMaterial *currentMaterial = NULL;
    RendererMesh *currentMesh = NULL;
    size_t currentModelIndex = 0;
    size_t currentMeshIndex = 0;
    ListArray currentVertexUvPool = {0};
    ListArray currentVertexNormalPool = {0};

    for (size_t i = 0; i < mdlFileLineCount; i++)
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

            for (size_t j = 0; j < materialPool->count; j++)
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

    for (size_t i = 0; i < modelCount; i++)
    {
        free(faceCounts[i]);
    }

    free(faceCounts);

    return models;
}

void RendererModel_Destroy(RendererModel *model)
{
    RJGlobal_DebugAssertNullPointerCheck(model);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, model->name.length), model->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, model->name.length)] = '\0';

    String_Destroy(&model->name);

    ListArray_Destroy(&model->vertices);

    for (size_t i = model->meshes.count - 1; i < model->meshes.count; i--)
    {
        RendererMesh_Destroy(*(RendererMesh **)ListArray_Get(&model->meshes, i));
    }

    ListArray_Destroy(&model->meshes);

    free(model);
    model = NULL;

    RJGlobal_DebugInfo("Renderer Model '%s' destroyed.", tempTitle);
}

#pragma endregion Renderer Model

#pragma region Renderer Scene

RendererScene *RendererScene_CreateEmpty(StringView name, size_t initialBatchCapacity)
{
    RendererScene *scene = malloc(sizeof(RendererScene));
    RJGlobal_DebugAssertNullPointerCheck(scene);

    scene->name = scc(name);
    scene->camera = NULL;
    scene->batches = ListArray_Create("Renderer Batch", sizeof(RendererBatch), initialBatchCapacity);

    glGenVertexArrays(1, &scene->vao);
    glGenBuffers(1, &scene->vboModelVertices);
    glGenBuffers(1, &scene->iboModelIndices);
    glGenBuffers(1, &scene->uboObjectMatrices);

    glBindVertexArray(scene->vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene->vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->uboObjectMatrices);

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

    scene->camProjectionMatrix = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "camProjectionMatrix");
    scene->camViewMatrix = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "camViewMatrix");
    scene->camPosition = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "camPosition");
    scene->camRotation = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "camRotation");

    scene->matAmbientColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matAmbientColor");
    scene->matDiffuseColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDiffuseColor");
    scene->matSpecularColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matSpecularColor");
    scene->matEmissiveColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matEmissiveColor");
    scene->matSpecularExponent = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matSpecularExponent");
    scene->matDissolve = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDissolve");
    scene->matDiffuseMap = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDiffuseMap");
    scene->matHasDiffuseMap = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matHasDiffuseMap");

    scene->objectMatricesHandle = glGetUniformBlockIndex(RENDERER_MAIN_SHADER_PROGRAM, "modelMatrices");
    glUniformBlockBinding(RENDERER_MAIN_SHADER_PROGRAM, scene->objectMatricesHandle, RENDERER_UBO_MATRICES_BINDING);
    glBindBufferBase(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene->uboObjectMatrices);
    // glBindBufferRange(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices, 0, RENDERER_SCENE_MAX_OBJECT_COUNT);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindVertexArray(0);

    RJGlobal_DebugInfo("Renderer Scene '%s' created.", scene->name.characters);

    return scene;
}

RendererScene *RendererScene_CreateFromFile(StringView scnFileData, size_t scnFileLineCount, const ListArray *modelPool, void *objectReferences, size_t transformOffsetInObject, size_t totalObjectSize, size_t objectCount)
{
    RJGlobal_DebugAssertNullPointerCheck(modelPool);
    RJGlobal_DebugAssertNullPointerCheck(objectReferences);

    RendererScene *scene = NULL;
    RendererBatch *currentBatch = NULL;
    RendererComponent *currentComponent = NULL;
    size_t totalObjectIndex = 0;

    StringView *scnLines = (StringView *)malloc(scnFileLineCount * sizeof(StringView));
    RJGlobal_DebugAssertNullPointerCheck(scnLines);
    RJGlobal_MemorySet(scnLines, scnFileLineCount * sizeof(StringView), 0);

    size_t scnLineTokenCount = 0;
    StringView scnLineTokens[RENDERER_MODEL_LINE_MAX_TOKEN_COUNT] = {0};

    StringView strNewline = scl("\n");
    StringView strSpace = scl(" ");
    StringView strP = scl("p");
    StringView strR = scl("r");
    StringView strS = scl("s");
    StringView strNEWSCN = scl("newscn");
    StringView strUSEMDL = scl("usemdl");

    String_Tokenize(scnFileData, strNewline, &scnFileLineCount, scnLines, scnFileLineCount);

    for (size_t i = 0; i < scnFileLineCount; i++)
    {
        String_Tokenize(scnLines[i], strSpace, &scnLineTokenCount, scnLineTokens, RENDERER_MODEL_LINE_MAX_TOKEN_COUNT);

        StringView firstToken = scnLineTokens[0];

        if (String_Compare(firstToken, strP) == 0)
        {
            RJGlobal_DebugAssert(totalObjectIndex < objectCount, "Object reference count %zu is not enough for the imported scene file", objectCount);
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

            for (size_t j = 0; j < modelPool->count; j++)
            {
                RendererModel *model = *(RendererModel **)ListArray_Get(modelPool, j);

                // RJGlobal_DebugInfo("Comparing model '%.*s' with '%.*s'...", model->name.length, model->name.characters, lineTokens[1].length, lineTokens[1].characters);

                if (String_Compare(scv(model->name), scv(scnLineTokens[1])) == 0)
                {
                    modelFound = true;
                    currentBatch = RendererScene_CreateBatch(scene, model, scnLineTokenCount > 2 ? (size_t)String_ToInt(scv(scnLineTokens[2])) : RENDERER_BATCH_INITIAL_CAPACITY);
                    break;
                }
            }

            RJGlobal_DebugAssert(modelFound, "Model '%.*s' not found in model pool when trying to assign it to mesh in model '%.*s'.", scnLineTokens[1].length, scnLineTokens[1].characters, currentBatch->model->name.length, currentBatch->model->name.characters);
        }
    }

    free(scnLines);

    RJGlobal_DebugInfo("Scene %s imported successfully.", scene->name.characters);

    return scene;
}

void RendererScene_Destroy(RendererScene *scene)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, scene->name.length), scene->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, scene->name.length)] = '\0';

    String_Destroy(&scene->name);
    scene->camera = NULL;

    for (size_t i = scene->batches.count - 1; i < scene->batches.count; i--)
    {
        RendererBatch *batch = (RendererBatch *)ListArray_Get(&scene->batches, i);
        RendererScene_DestroyBatch(batch);
    }

    ListArray_Destroy(&scene->batches);

    glDeleteVertexArrays(1, &scene->vao);
    glDeleteBuffers(1, &scene->vboModelVertices);
    glDeleteBuffers(1, &scene->iboModelIndices);

    scene->vao = 0;
    scene->vboModelVertices = 0;
    scene->iboModelIndices = 0;

    scene->camProjectionMatrix = 0;
    scene->camViewMatrix = 0;
    scene->camPosition = 0;
    scene->camRotation = 0;
    scene->objectMatricesHandle = 0;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    free(scene);
    scene = NULL;

    RJGlobal_DebugInfo("Renderer Scene '%s' destroyed.", tempTitle);
}

void RendererScene_SetMainCamera(RendererScene *scene, RendererCameraComponent *camera)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);
    RJGlobal_DebugAssertNullPointerCheck(camera);

    scene->camera = camera;
    camera->scene = scene;
}

RendererBatch *RendererScene_CreateBatch(RendererScene *scene, RendererModel *model, size_t initialObjectCapacity)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);
    RJGlobal_DebugAssertNullPointerCheck(model);

    RendererBatch batch = {0};

    batch.model = model;
    batch.scene = scene;
    batch.batchOffsetInScene = scene->batches.count;
    batch.objectMatrices = ListArray_Create("Matrix 4x4", sizeof(Renderer_Matrix4), initialObjectCapacity);
    batch.components = ListArray_Create("Renderer Component", sizeof(RendererComponent), initialObjectCapacity);

    return (RendererBatch *)ListArray_Add(&scene->batches, &batch);
}

void RendererScene_DestroyBatch(RendererBatch *batch)
{
    RJGlobal_DebugAssertNullPointerCheck(batch);

    // todo ! be may be wrong
    for (size_t i = batch->batchOffsetInScene + 1; i < batch->scene->batches.count - batch->batchOffsetInScene; i++)
    {
        RendererBatch *nextBatch = (RendererBatch *)ListArray_Get(&batch->scene->batches, i);
        nextBatch->batchOffsetInScene--;
    }

    batch->model = NULL;

    ListArray_Destroy(&batch->objectMatrices);
    ListArray_Destroy(&batch->components);

    ListArray_RemoveAtIndex(&batch->scene->batches, batch->batchOffsetInScene);
}

void RendererScene_Update(RendererScene *scene)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);
    RJGlobal_DebugAssertNullPointerCheck(scene->camera);

    glm_mat4_identity((vec4 *)&scene->camera->viewMatrix);
    glm_mat4_identity((vec4 *)&scene->camera->projectionMatrix);

    Vector3 direction = Vector3_Normalized(Vector3_New(Maths_Cos(scene->camera->rotationReference->x) * Maths_Cos(scene->camera->rotationReference->y),
                                                       Maths_Sin(scene->camera->rotationReference->x),
                                                       Maths_Cos(scene->camera->rotationReference->x) * Maths_Sin(scene->camera->rotationReference->y)));

    Vector3 center = Vector3_Add(*scene->camera->positionReference, Vector3_Normalized(direction));

    glm_lookat((float *)scene->camera->positionReference, (float *)&center, (float *)&(vec3){0, 1, 0}, (vec4 *)&scene->camera->viewMatrix);

    if (scene->camera->isPerspective)
    {
        glm_perspective(Maths_DegToRad(scene->camera->size),
                        (float)RENDERER_MAIN_WINDOW->size.x / (float)RENDERER_MAIN_WINDOW->size.y,
                        scene->camera->nearClipPlane,
                        scene->camera->farClipPlane,
                        (vec4 *)&scene->camera->projectionMatrix);
    }
    else
    {
        float sizeX = (float)RENDERER_MAIN_WINDOW->size.x * scene->camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)RENDERER_MAIN_WINDOW->size.y * scene->camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;

        glm_ortho(-sizeX, sizeX,
                  -sizeY, sizeY,
                  scene->camera->nearClipPlane,
                  scene->camera->farClipPlane,
                  (vec4 *)&scene->camera->projectionMatrix);
    }

    for (size_t i = 0; i < scene->batches.count; i++)
    {
        RendererBatch *batch = (RendererBatch *)ListArray_Get(&scene->batches, i);
        for (size_t j = 0; j < batch->objectMatrices.count; j++)
        {
            RendererComponent *component = (RendererComponent *)ListArray_Get(&batch->components, j);
            TRANSFORM_TO_MODEL_MATRIX(
                (Renderer_Matrix4 *)ListArray_Get(&batch->objectMatrices, j),
                component->positionReference,
                component->rotationReference,
                component->scaleReference);
        }
    }
}

#pragma endregion Renderer Scene

#pragma region Renderer Batch

RendererComponent *RendererBatch_CreateComponent(RendererBatch *batch, Vector3 *positionReference, Vector3 *rotationReference, Vector3 *scaleReference)
{
    RJGlobal_DebugAssertNullPointerCheck(batch);
    RJGlobal_DebugAssertNullPointerCheck(positionReference);
    RJGlobal_DebugAssertNullPointerCheck(rotationReference);
    RJGlobal_DebugAssertNullPointerCheck(scaleReference);
    //    RJGlobal_DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererComponent component = {0};

    component.positionReference = positionReference;
    component.rotationReference = rotationReference;
    component.scaleReference = scaleReference;
    component.batch = batch;
    component.componentOffsetInBatch = batch->objectMatrices.count;

    Renderer_Matrix4 temp; // dummy, just to allocate the space in list
    ListArray_Add(&batch->objectMatrices, &temp);

    // RJGlobal_DebugInfo("Renderer Component created.");

    return (RendererComponent *)ListArray_Add(&component.batch->components, &component);
}

void RendererBatch_DestroyComponent(RendererComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);

    // todo ! be may be wrong
    for (size_t i = component->componentOffsetInBatch + 1; i < component->batch->components.count - component->componentOffsetInBatch; i++)
    {
        RendererComponent *nextComponent = (RendererComponent *)ListArray_Get(&component->batch->components, i);
        nextComponent->componentOffsetInBatch--;
    }

    ListArray_RemoveAtIndex(&component->batch->objectMatrices, component->componentOffsetInBatch);
    ListArray_RemoveAtIndex(&component->batch->components, component->componentOffsetInBatch);
}

#pragma endregion Renderer Batch

#pragma region Renderer Camera

RendererCameraComponent *RendererCameraComponent_Create(Vector3 *positionReference, Vector3 *rotationReference)
{
    RendererCameraComponent *camera = malloc(sizeof(RendererCameraComponent));
    RJGlobal_DebugAssertNullPointerCheck(camera);

    camera->scene = NULL;

    camera->positionReference = positionReference;
    camera->rotationReference = rotationReference;

    camera->isPerspective = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE;
    camera->size = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE ? RENDERER_CAMERA_DEFAULT_FOV : RENDERER_CAMERA_DEFAULT_ORTHOGRAPHIC_SIZE;

    camera->nearClipPlane = RENDERER_CAMERA_DEFAULT_NEAR_CLIP_PLANE;
    camera->farClipPlane = RENDERER_CAMERA_DEFAULT_FAR_CLIP_PLANE;

    return camera;
}

void RendererCameraComponent_Destroy(RendererCameraComponent *camera)
{
    RJGlobal_DebugAssertNullPointerCheck(camera);

    camera->size = -1.0f;

    free(camera);
    camera = NULL;
}

#pragma endregion Renderer Camera
