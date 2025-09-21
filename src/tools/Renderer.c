#include "tools/Renderer.h"
#include "tools/Resources.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#pragma region Source Only

#define OPENGL_DRAW_TYPE GL_STATIC_DRAW

#define DebugCheckRenderer()                                               \
    do                                                                     \
    {                                                                      \
        GLenum glError = glGetError();                                     \
        DebugAssert(glError == GL_NO_ERROR, "OpenGL error : %d", glError); \
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

typedef struct RendererTexture
{
    size_t index;
    ResourceImage *image;
    RendererTextureHandle handle;
} RendererTexture;

typedef struct RendererMaterial
{
    String name;
    Vector3 ambientColor;
    Vector3 diffuseColor;
    Vector3 specularColor;
    RendererTexture diffuseMap;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
} RendererMaterial;

typedef struct RendererMesh
{
    ListArray indices; // RendererMeshIndex
    RendererMaterial *material;
} RendererMesh;

ContextWindow *RENDERER_MAIN_WINDOW = NULL;
RendererShaderProgramHandle RENDERER_MAIN_SHADER_PROGRAM = 0;
RendererTexture RENDERER_MAIN_TEXTURES[RENDERER_TEXTURE_MAX_COUNT] = {0};
size_t RENDERER_MAIN_TEXTURES_COUNT = 0;

RendererVAOHandle RENDERER_DEBUG_VAO = 0;
RendererVBOHandle RENDERER_DEBUG_VBO = 0;
ListArray RENDERER_DEBUG_VERTICES = {0}; // RendererDebugVertex
RendererShaderProgramHandle RENDERER_DEBUG_SHADER_PROGRAM = 0;
RendererUniformLocationHandle RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX = 0;
RendererUniformLocationHandle RENDERER_DEBUG_UNIFORM_VIEW_MATRIX = 0;

void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(void *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);

    RENDERER_MAIN_WINDOW->size.x = width;
    RENDERER_MAIN_WINDOW->size.y = height;

    glViewport(0, 0, RENDERER_MAIN_WINDOW->size.x, RENDERER_MAIN_WINDOW->size.y);
}

void TransformToModelMatrix(Vector3 *position, Vector3 *rotation, Vector3 *scale, mat4 *matrix)
{
    DebugAssertNullPointerCheck(matrix);

    glm_mat4_identity((vec4 *)matrix);

    glm_translate((vec4 *)matrix, (vec3){position->x, position->y, position->z});

    glm_rotate((vec4 *)matrix, rotation->x, (vec3){1, 0, 0});
    glm_rotate((vec4 *)matrix, rotation->y, (vec3){0, 1, 0});
    glm_rotate((vec4 *)matrix, rotation->z, (vec3){0, 0, 1});

    glm_scale((vec4 *)matrix, (vec3){scale->x, scale->y, scale->z});
}

#pragma region Renderer Texture

RendererTexture RendererTexture_Create(String name, String path)
{
    for (size_t i = 0; i < RENDERER_TEXTURE_MAX_COUNT; i++)
    {
        if (RENDERER_MAIN_TEXTURES[i].image != NULL && String_Compare(RENDERER_MAIN_TEXTURES[i].image->name, name) == 0)
        {
            return RENDERER_MAIN_TEXTURES[i];
        }
    }

    RendererTexture texture = {0};
    texture.image = ResourceImage_Create(name, path);

    glGenTextures(1, &texture.handle);
    glBindTexture(GL_TEXTURE_2D, texture.handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = texture.image->channels == 4 ? GL_RGBA : (texture.image->channels == 3 ? GL_RGB : (texture.image->channels == 2 ? GL_RG : GL_RED));

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, texture.image->size.x, texture.image->size.y, 0, format, GL_UNSIGNED_BYTE, texture.image->data);
    // glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    texture.index = RENDERER_MAIN_TEXTURES_COUNT++;
    RENDERER_MAIN_TEXTURES[texture.index] = texture;

    return texture;
}

void RendererTexture_Destroy(RendererTexture *texture)
{
    DebugAssertNullPointerCheck(texture);

    RENDERER_MAIN_TEXTURES[texture->index] = (RendererTexture){0};
    RENDERER_MAIN_TEXTURES_COUNT--;

    ResourceImage_Destroy(texture->image);
    texture->image = NULL;

    glDeleteTextures(1, &texture->handle);
    texture->handle = 0;

    for (size_t i = texture->index; i < RENDERER_MAIN_TEXTURES_COUNT; i++)
    {
        RENDERER_MAIN_TEXTURES[i] = RENDERER_MAIN_TEXTURES[i + 1];
        RENDERER_MAIN_TEXTURES[i].index = i;
    }
}

#pragma endregion Renderer Texture

#pragma region Renderer Material

RendererMaterial *RendererMaterial_Create(String name)
{
    RendererMaterial *material = (RendererMaterial *)malloc(sizeof(RendererMaterial));
    DebugAssertNullPointerCheck(material);

    material->name = scc(name);
    material->ambientColor = NewVector3(-1.0f, -1.0f, -1.0f);
    material->diffuseColor = NewVector3(-1.0f, -1.0f, -1.0f);
    material->diffuseMap = (RendererTexture){0};
    material->dissolve = -1.0f;
    material->illuminationModel = -1;
    material->refractionIndex = -1.0f;
    material->specularColor = NewVector3(-1.0f, -1.0f, -1.0f);
    material->specularExponent = -1.0f;

    return material;
}

void RendererMaterial_Destroy(RendererMaterial *material)
{
    material->ambientColor = NewVector3(-1.0f, -1.0f, -1.0f);
    material->diffuseColor = NewVector3(-1.0f, -1.0f, -1.0f);
    glDeleteTextures(1, &material->diffuseMap.handle);
    material->dissolve = -1.0f;
    material->illuminationModel = -1;
    RendererTexture_Destroy(&material->diffuseMap);
    String_Destroy(&material->name);
    material->refractionIndex = -1.0f;
    material->specularColor = NewVector3(-1.0f, -1.0f, -1.0f);
    material->specularExponent = -1.0f;

    free(material);

    material = NULL;
}

#pragma endregion Renderer Material

#pragma region Renderer Mesh

RendererMesh *RendererMesh_CreateEmpty(size_t initialIndexCapacity, RendererMaterial *material)
{
    RendererMesh *mesh = malloc(sizeof(RendererMesh));
    DebugAssertNullPointerCheck(mesh);

    mesh->indices = ListArray_Create("Renderer Mesh Indices", sizeof(RendererMeshIndex), initialIndexCapacity);
    mesh->material = material;

    return mesh;
}

void RendererMesh_Destroy(RendererMesh *mesh)
{
    DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->indices);

    mesh->material = NULL;

    free(mesh);
    mesh = NULL;
}

#pragma endregion Renderer Mesh

#pragma endregion Source Only

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window)
{
    DebugAssertNullPointerCheck(window);

    RENDERER_MAIN_WINDOW = window;

    DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");
    DebugInfo("GLAD initialized successfully.");

    Context_ConfigureResizeCallback(RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate()
{
    if (RENDERER_MAIN_SHADER_PROGRAM != 0)
    {
        glDeleteProgram(RENDERER_MAIN_SHADER_PROGRAM);
    }
}

void Renderer_ConfigureShaders(String vertexShaderSource, String fragmentShaderSource)
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

    DebugAssert(glslHasCompiled != GL_FALSE, "Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Fragment shader compiled successfully.");

    RENDERER_MAIN_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_MAIN_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_MAIN_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER_MAIN_SHADER_PROGRAM, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DebugInfo("Shader program linked and created successfully.");
}

void Renderer_StartRendering()
{
    if (glfwWindowShouldClose(RENDERER_MAIN_WINDOW->handle))
    {
        DebugInfo("Main window close input received");
        Global_Terminate(EXIT_SUCCESS, "Main window close input received");
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
    DebugAssertNullPointerCheck(scene);

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
        RendererBatch *batch = (RendererBatch *)ListArray_Get(scene->batches, i);
        RendererMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(batch->objectMatrices.sizeOfItem * batch->objectMatrices.count),
                     batch->objectMatrices.data,
                     OPENGL_DRAW_TYPE);

        glBufferData(GL_ARRAY_BUFFER,
                     (long long)(batch->model->vertices.sizeOfItem * batch->model->vertices.count),
                     batch->model->vertices.data,
                     OPENGL_DRAW_TYPE);

        for (size_t j = 0; j < batch->model->meshes.count; j++)
        {
            RendererMesh *mesh = *(RendererMesh **)ListArray_Get(batch->model->meshes, j);

            if (mesh->material != previousMaterial)
            {
                glUniform3fv(scene->matAmbientColor, 1, (GLfloat *)&mesh->material->ambientColor);
                glUniform3fv(scene->matDiffuseColor, 1, (GLfloat *)&mesh->material->diffuseColor);
                glUniform3fv(scene->matSpecularColor, 1, (GLfloat *)&mesh->material->specularColor);
                glUniform1f(scene->matSpecularExponent, mesh->material->specularExponent);
                glUniform1f(scene->matDissolve, mesh->material->dissolve);

                // Texture binding
                if (mesh->material->diffuseMap.handle != 0)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh->material->diffuseMap.handle);
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
                         OPENGL_DRAW_TYPE);

            glDrawElementsInstanced(GL_TRIANGLES,
                                    mesh->indices.count,
                                    GL_UNSIGNED_INT,
                                    0,
                                    batch->objectMatrices.count);
        }
    }

    // DebugCheckRenderer();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // DebugInfo("Scene '%s' rendered", scene.name.characters);
}

#pragma endregion Renderer

#pragma region Renderer Debug

void RendererDebug_Initialize(String vertexShaderSource, String fragmentShaderSource, size_t initialVertexCapacity)
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

    DebugAssert(glslHasCompiled != GL_FALSE, "Debug Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Debug Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Debug Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Debug Fragment shader compiled successfully.");

    RENDERER_DEBUG_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_DEBUG_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_DEBUG_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_DEBUG_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_DEBUG_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER_DEBUG_SHADER_PROGRAM, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Debug Shader program linking failed. Logs:\n%s", glslInfoLog);

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

    DebugInfo("Debug Renderer initialized successfully.");
}

void RendererDebug_Terminate()
{
    glDeleteBuffers(1, &RENDERER_DEBUG_VBO);
    glDeleteVertexArrays(1, &RENDERER_DEBUG_VAO);

    if (RENDERER_DEBUG_SHADER_PROGRAM != 0)
    {
        glDeleteProgram(RENDERER_DEBUG_SHADER_PROGRAM);
    }

    RENDERER_DEBUG_VAO = 0;
    RENDERER_DEBUG_VBO = 0;
    RENDERER_DEBUG_SHADER_PROGRAM = 0;
    RENDERER_DEBUG_UNIFORM_PROJECTION_MATRIX = 0;
    RENDERER_DEBUG_UNIFORM_VIEW_MATRIX = 0;

    DebugInfo("Debug Renderer terminated successfully.");
}

void RendererDebug_StartRendering()
{
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);
}

void RendererDebug_FinishRendering(mat4 *camProjectionMatrix, mat4 *camViewMatrix)
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
    glBufferData(GL_ARRAY_BUFFER, RENDERER_DEBUG_VERTICES.sizeOfItem * RENDERER_DEBUG_VERTICES.count, RENDERER_DEBUG_VERTICES.data, OPENGL_DRAW_TYPE);

    glDrawArrays(GL_LINES, 0, RENDERER_DEBUG_VERTICES.count);

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

#pragma region Renderer Model

RendererModel *RendererModel_CreateOBJ(String name, String objFileSource, size_t objFileSourceLineCount, String objFilePath, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset)
{
    Timer modelTimer = Timer_Create("Model Import Timer");
    Timer_Start(&modelTimer);

    mat4 offsetMatrix;
    TransformToModelMatrix(&positionOffset, &rotationOffset, &scaleOffset, &offsetMatrix);

    size_t meshCount = 0;

    size_t totalVertexPositionCount = 0;
    size_t totalVertexNormalCount = 0;
    size_t totalVertexUvCount = 0;

    String *lines = (String *)malloc(objFileSourceLineCount * sizeof(String));
    DebugAssertNullPointerCheck(lines);

    size_t lineTokenCount = 0;
    String lineTokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};

    String strNewline = scl("\n");
    String strSpace = scl(" ");
    String strV = scl("v");
    String strF = scl("f");
    String strVT = scl("vt");
    String strVN = scl("vn");
    // String strO = scl("o");
    // String strG = scl("g");
    String strMTLLIB = scl("mtllib");
    String strUSEMTL = scl("usemtl");

    ListArray materials = {0}; // RendererMaterial*

    String_Tokenize(objFileSource, strNewline, &objFileSourceLineCount, lines, objFileSourceLineCount);

    for (size_t i = 0; i < objFileSourceLineCount; i++) // count and create materials
    {
        String_Tokenize(lines[i], strSpace, &lineTokenCount, lineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        String firstToken = lineTokens[0];

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
        else if (String_Compare(firstToken, strUSEMTL) == 0) // new object
        {
            meshCount++;
        }
        else if (String_Compare(firstToken, strMTLLIB) == 0) // .mtl file
        {
            Resource *mtlFileResource = Resource_Create(lineTokens[1], objFilePath);

            size_t materialCount = 0;

            size_t mtlLineCount = 0;
            String *mtlLines = (String *)malloc(mtlFileResource->lineCount * sizeof(String));
            DebugAssertNullPointerCheck(mtlLines);

            size_t mtlLineTokenCount = 0;
            String mtlLineTokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};

            String strNEWMTL = scl("newmtl");
            String strNS = scl("Ns");
            String strKA = scl("Ka");
            String strKD = scl("Kd");
            String strKS = scl("Ks");
            String strNI = scl("Ni");
            String strD = scl("d");
            String strILLNUM = scl("illum");
            String strMAP_KD = scl("map_Kd");
            // String strMAP_BUMP = scl("map_bump");

            String_Tokenize(mtlFileResource->data, strNewline, &mtlLineCount, mtlLines, mtlFileResource->lineCount);

            for (size_t j = 0; j < mtlLineCount; j++) // count
            {
                String_Tokenize(mtlLines[j], strSpace, &mtlLineTokenCount, mtlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

                String mtlFirstToken = mtlLineTokens[0];

                if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
                {
                    materialCount++;
                }
            }

            materials = ListArray_Create("Renderer Material Pointer", sizeof(RendererMaterial *), materialCount);
            RendererMaterial *currentMaterial = NULL;

            for (size_t j = 0; j < mtlLineCount; j++) // compute
            {
                String_Tokenize(mtlLines[j], strSpace, &mtlLineTokenCount, mtlLineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

                String mtlFirstToken = mtlLineTokens[0];

                if (String_Compare(mtlFirstToken, strNEWMTL) == 0)
                {
                    currentMaterial = RendererMaterial_Create(mtlLineTokens[1]);
                    ListArray_Add(&materials, &currentMaterial);
                }
                else if (String_Compare(mtlFirstToken, strNS) == 0)
                {
                    currentMaterial->specularExponent = String_ToFloat(mtlLineTokens[1]);
                }
                else if (String_Compare(mtlFirstToken, strKA) == 0)
                {
                    currentMaterial->ambientColor =
                        NewVector3(String_ToFloat(mtlLineTokens[1]),
                                   String_ToFloat(mtlLineTokens[2]),
                                   String_ToFloat(mtlLineTokens[3]));
                }
                else if (String_Compare(mtlFirstToken, strKD) == 0)
                {
                    currentMaterial->diffuseColor =
                        NewVector3(String_ToFloat(mtlLineTokens[1]),
                                   String_ToFloat(mtlLineTokens[2]),
                                   String_ToFloat(mtlLineTokens[3]));
                }
                else if (String_Compare(mtlFirstToken, strKS) == 0)
                {
                    currentMaterial->specularColor =
                        NewVector3(String_ToFloat(mtlLineTokens[1]),
                                   String_ToFloat(mtlLineTokens[2]),
                                   String_ToFloat(mtlLineTokens[3]));
                }
                else if (String_Compare(mtlFirstToken, strNI) == 0)
                {
                    currentMaterial->refractionIndex = String_ToFloat(mtlLineTokens[1]);
                }
                else if (String_Compare(mtlFirstToken, strD) == 0)
                {
                    currentMaterial->dissolve = String_ToFloat(mtlLineTokens[1]);
                }
                else if (String_Compare(mtlFirstToken, strILLNUM) == 0)
                {
                    currentMaterial->illuminationModel = String_ToInt(mtlLineTokens[1]);
                }
                else if (String_Compare(mtlFirstToken, strMAP_KD) == 0)
                {
                    currentMaterial->diffuseMap = RendererTexture_Create(scc(mtlLineTokens[1]), objFilePath); // try find in existing textures
                }
            }

            Resource_Destroy(mtlFileResource);

            free(mtlLines);
        }
    }

    size_t *faceCounts = (size_t *)malloc(meshCount * sizeof(size_t));
    MemorySet(faceCounts, 0, meshCount * sizeof(size_t));
    size_t tempMeshIndex = 0;
    for (size_t i = 0; i < objFileSourceLineCount; i++) // count and create materials
    {
        String_Tokenize(lines[i], strSpace, &lineTokenCount, lineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        String firstToken = lineTokens[0];
        if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            faceCounts[tempMeshIndex - 1] += lineTokenCount == 4 ? 1 : 2;
        }
        if (String_Compare(firstToken, strUSEMTL) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            tempMeshIndex++;
        }
    }

    RendererModel *model = RendererModel_CreateEmpty(name, meshCount, totalVertexPositionCount);
    ListArray globalVertexNormalPool = ListArray_Create("Vector3", sizeof(Vector3), totalVertexNormalCount); // Vector3
    ListArray globalVertexUvPool = ListArray_Create("Vector2", sizeof(Vector2), totalVertexUvCount);         // Vector3

    for (size_t i = 0; i < objFileSourceLineCount; i++) // create global pools
    {
        String_Tokenize(lines[i], strSpace, &lineTokenCount, lineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        String firstToken = lineTokens[0];

        if (String_Compare(firstToken, strV) == 0) // v -7.579129 4.591946 4.850700
        {
            vec3 vertexPosition;

            glm_mat4_mulv3((vec4 *)&offsetMatrix,
                           (vec3){String_ToFloat(lineTokens[1]),
                                  String_ToFloat(lineTokens[2]),
                                  String_ToFloat(lineTokens[3])},
                           0.0f,
                           (float *)&vertexPosition);

            RendererMeshVertex createdVertex;
            createdVertex.vertexPosition = NewVector3(vertexPosition[0], vertexPosition[1], vertexPosition[2]);

            ListArray_Add(&model->vertices, &createdVertex);
        }
        else if (String_Compare(firstToken, strVT) == 0) // vt 0.073128 0.431854
        {
            Vector2 vertexUv =
                NewVector2(String_ToFloat(lineTokens[1]),
                           String_ToFloat(lineTokens[2]));

            ListArray_Add(&globalVertexUvPool, &vertexUv);
        }
        else if (String_Compare(firstToken, strVN) == 0) // vn -0.0233 0.1253 -0.9918
        {
            vec3 vertexNormal;

            glm_mat4_mulv3((vec4 *)&offsetMatrix,
                           (vec3){String_ToFloat(lineTokens[1]),
                                  String_ToFloat(lineTokens[2]),
                                  String_ToFloat(lineTokens[3])},
                           0.0f,
                           (float *)&vertexNormal);

            RendererMeshVertex createdVertex;
            createdVertex.vertexPosition = NewVector3(vertexNormal[0], vertexNormal[1], vertexNormal[2]);

            ListArray_Add(&globalVertexNormalPool, &vertexNormal);
        }
    }

    RendererMesh *currentMesh = NULL;

    for (size_t i = 0; i < objFileSourceLineCount; i++) // add data to model
    {
        String_Tokenize(lines[i], scl(" "), &lineTokenCount, lineTokens, RESOURCE_FILE_LINE_MAX_TOKEN_COUNT);

        String firstToken = lineTokens[0];

        if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            if (lineTokenCount == 4)
            {
                for (size_t j = 1; j < lineTokenCount; j++)
                {
                    String faceData[3];
                    size_t faceDataCount;
                    String_Tokenize(lineTokens[j], scl("/"), &faceDataCount, faceData, 3);

                    int createdVertexIndex = String_ToInt(faceData[0]);
                    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

                    RendererMeshVertex *vertex = (RendererMeshVertex *)ListArray_Get(model->vertices, (size_t)vertexIndex);

                    if (faceDataCount > 1 && faceData[1].length != 0)
                    {
                        int createdUIndex = String_ToInt(faceData[1]);
                        unsigned int uvIndex = createdUIndex < 0 ? (unsigned int)globalVertexUvPool.count + (unsigned int)createdUIndex : (unsigned int)createdUIndex - 1;

                        vertex->vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (size_t)uvIndex);
                    }

                    if (faceDataCount > 2 && faceData[2].length != 0)
                    {
                        int createdNormalIndex = String_ToInt(faceData[2]);
                        unsigned int normalIndex = createdNormalIndex < 0 ? (unsigned int)globalVertexNormalPool.count + (unsigned int)createdNormalIndex : (unsigned int)createdNormalIndex - 1;

                        vertex->vertexNormal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (size_t)normalIndex);
                    }

                    ListArray_Add(&currentMesh->indices, &vertexIndex);
                }
            }
            else if (lineTokenCount == 5)
            {
                for (size_t j = 1; j < lineTokenCount; j++)
                {
                    String faceData[3];
                    size_t faceDataCount;
                    String_Tokenize(lineTokens[j], scl("/"), &faceDataCount, faceData, 3);

                    int createdVertexIndex = String_ToInt(faceData[0]);
                    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

                    RendererMeshVertex *vertex = (RendererMeshVertex *)ListArray_Get(model->vertices, (size_t)vertexIndex);

                    if (faceDataCount > 1 && faceData[1].length != 0)
                    {
                        int createdUIndex = String_ToInt(faceData[1]);
                        unsigned int uvIndex = createdUIndex < 0 ? (unsigned int)globalVertexUvPool.count + (unsigned int)createdUIndex : (unsigned int)createdUIndex - 1;

                        vertex->vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (size_t)uvIndex);
                    }

                    if (faceDataCount > 2 && faceData[2].length != 0)
                    {
                        int createdNormalIndex = String_ToInt(faceData[2]);
                        unsigned int normalIndex = createdNormalIndex < 0 ? (unsigned int)globalVertexNormalPool.count + (unsigned int)createdNormalIndex : (unsigned int)createdNormalIndex - 1;

                        vertex->vertexNormal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (size_t)normalIndex);
                    }

                    ListArray_Add(&currentMesh->indices, &vertexIndex);
                }

                // wrap for square faces, add 0, add 2
                for (size_t j = 1; j < 4; j += 2)
                {
                    String faceData[3];
                    size_t faceDataCount;
                    String_Tokenize(lineTokens[j], scl("/"), &faceDataCount, faceData, 3);

                    int createdVertexIndex = String_ToInt(faceData[0]);
                    unsigned int vertexIndex = createdVertexIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdVertexIndex : (unsigned int)createdVertexIndex - 1;

                    ListArray_Add(&currentMesh->indices, &vertexIndex);
                }
            }
        }
        else if (String_Compare(firstToken, strUSEMTL) == 0) // use material and create object
        {
            DebugAssertNullPointerCheck(materials.data);

            for (size_t j = 0; j < materials.count; j++)
            {
                RendererMaterial *material = *(RendererMaterial **)ListArray_Get(materials, j);

                if (String_Compare(material->name, lineTokens[1]) == 0)
                {
                    currentMesh = RendererMesh_CreateEmpty(faceCounts[model->meshes.count] * 3, material);
                    ListArray_Add(&model->meshes, &currentMesh);
                    break;
                }
            }
        }
    }

    ListArray_Destroy(&globalVertexNormalPool);
    ListArray_Destroy(&globalVertexUvPool);

    ListArray_Destroy(&materials);

    free(lines);
    free(faceCounts);

    Timer_Stop(&modelTimer);

    DebugInfo("Renderer Model '%s' imported successfully with %zu child meshes in %f seconds.", model->name.characters, model->meshes.count, (double)Timer_GetElapsedNanoseconds(modelTimer) / 1000000000.0);

    return model;
}

RendererModel *RendererModel_CreateEmpty(String name, size_t initialMeshCapacity, size_t initialVertexCapacity)
{
    RendererModel *model = malloc(sizeof(RendererModel));
    DebugAssertNullPointerCheck(model);

    model->name = scc(name);
    model->vertices = ListArray_Create("Renderer Mesh Vertex", sizeof(RendererMeshVertex), initialVertexCapacity);
    model->meshes = ListArray_Create("Renderer Mesh Pointer", sizeof(RendererMesh *), initialMeshCapacity);

    return model;
}

void RendererModel_Destroy(RendererModel *model)
{
    DebugAssertNullPointerCheck(model);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, model->name.length + 1), model->name.characters);

    String_Destroy(&model->name);

    ListArray_Destroy(&model->vertices);

    for (size_t i = model->meshes.count - 1; i >= 0; i--)
    {
        RendererMesh_Destroy(*(RendererMesh **)ListArray_Get(model->meshes, i));
    }

    ListArray_Destroy(&model->meshes);

    free(model);
    model = NULL;

    DebugInfo("Renderer Model '%s' destroyed.", tempTitle);
}

#pragma endregion Renderer Model

#pragma region Renderer Scene

RendererScene *RendererScene_Create(String name, size_t initialBatchCapacity)
{
    RendererScene *scene = malloc(sizeof(RendererScene));
    DebugAssertNullPointerCheck(scene);

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

    DebugInfo("Renderer Scene '%s' created.", scene->name.characters);

    return scene;
}

void RendererScene_Destroy(RendererScene *scene)
{
    DebugAssertNullPointerCheck(scene);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, scene->name.length + 1), scene->name.characters);

    String_Destroy(&scene->name);
    scene->camera = NULL;

    for (size_t i = scene->batches.count - 1; i >= 0; i--)
    {
        RendererBatch *batch = (RendererBatch *)ListArray_Get(scene->batches, i);
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

    DebugInfo("Renderer Scene '%s' destroyed.", tempTitle);
}

void RendererScene_SetMainCamera(RendererScene *scene, RendererCameraComponent *camera)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(camera);

    scene->camera = camera;
    camera->scene = scene;
}

RendererBatch *RendererScene_CreateBatch(RendererScene *scene, String name, RendererModel *model, size_t initialObjectCapacity)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(model);

    RendererBatch batch = {0};

    batch.name = scc(name);
    batch.model = model;
    batch.scene = scene;
    batch.batchOffsetInScene = scene->batches.count;
    batch.objectMatrices = ListArray_Create("Matrix 4x4", sizeof(mat4), initialObjectCapacity);
    batch.components = ListArray_Create("Renderer Component", sizeof(RendererComponent), initialObjectCapacity);

    DebugInfo("Renderer Batch '%s' created.", batch.name.characters);

    return (RendererBatch *)ListArray_Add(&scene->batches, &batch);
}

void RendererScene_DestroyBatch(RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, batch->name.length + 1), batch->name.characters);

    for (size_t i = batch->batchOffsetInScene; i < batch->scene->batches.count - batch->batchOffsetInScene; i++)
    {
        RendererBatch *nextBatch = (RendererBatch *)ListArray_Get(batch->scene->batches, i);
        nextBatch->batchOffsetInScene--;
    }

    ListArray_RemoveAtIndex(&batch->scene->batches, batch->batchOffsetInScene);

    String_Destroy(&batch->name);
    batch->model = NULL;

    ListArray_Destroy(&batch->objectMatrices);
    ListArray_Destroy(&batch->components);

    DebugInfo("Renderer Batch '%s' destroyed.", tempTitle);
}

void RendererScene_Update(RendererScene *scene)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(scene->camera);

    glm_mat4_identity((vec4 *)&scene->camera->viewMatrix);
    glm_mat4_identity((vec4 *)&scene->camera->projectionMatrix);

    Vector3 direction = Vector3_Normalized(NewVector3(Cos(scene->camera->rotationReference->x) * Cos(scene->camera->rotationReference->y),
                                                      Sin(scene->camera->rotationReference->x),
                                                      Cos(scene->camera->rotationReference->x) * Sin(scene->camera->rotationReference->y)));

    Vector3 center = Vector3_Add(*scene->camera->positionReference, Vector3_Normalized(direction));

    glm_lookat((float *)scene->camera->positionReference, (float *)&center, (vec3){0, 1, 0}, (vec4 *)&scene->camera->viewMatrix);

    if (scene->camera->isPerspective)
    {
        glm_perspective(DegToRad(scene->camera->size),
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
        RendererBatch *batch = (RendererBatch *)ListArray_Get(scene->batches, i);
        for (size_t j = 0; j < batch->objectMatrices.count; j++)
        {
            RendererComponent *component = (RendererComponent *)ListArray_Get(batch->components, j);
            TransformToModelMatrix(component->positionReference,
                                   component->rotationReference,
                                   component->scaleReference,
                                   (mat4 *)ListArray_Get(batch->objectMatrices, j));
        }
    }
}

#pragma endregion Renderer Scene

#pragma region Renderer Batch

RendererComponent *RendererBatch_CreateComponent(RendererBatch *batch, Vector3 *positionReference, Vector3 *rotationReference, Vector3 *scaleReference)
{
    DebugAssertNullPointerCheck(batch);
    //    DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererComponent component = {0};

    component.positionReference = positionReference;
    component.rotationReference = rotationReference;
    component.scaleReference = scaleReference;
    component.batch = batch;
    component.componentOffsetInBatch = batch->objectMatrices.count;

    mat4 temp; // dummy, just to allocate the space in list
    ListArray_Add(&batch->objectMatrices, &temp);

    // DebugInfo("Renderer Component created.");

    return (RendererComponent *)ListArray_Add(&component.batch->components, &component);
}

void RendererBatch_DestroyComponent(RendererComponent *component)
{
    DebugAssertNullPointerCheck(component);

    for (size_t i = component->componentOffsetInBatch; i < component->batch->components.count - component->componentOffsetInBatch; i++)
    {
        RendererComponent *nextComponent = (RendererComponent *)ListArray_Get(component->batch->components, i);
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
    DebugAssertNullPointerCheck(camera);

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
    DebugAssertNullPointerCheck(camera);

    camera->size = -1.0f;

    free(camera);
    camera = NULL;
}

void RendererCameraComponent_Configure(RendererCameraComponent *camera, bool isPerspective, float value, float nearClipPlane, float farClipPlane)
{
    DebugAssertNullPointerCheck(camera);

    camera->isPerspective = isPerspective;
    camera->size = value;
    camera->nearClipPlane = nearClipPlane;
    camera->farClipPlane = farClipPlane;
}

#pragma endregion Renderer Camera
