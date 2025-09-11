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

GLFWwindow *RENDERER_MAIN_WINDOW = NULL;
Vector2Int RENDERER_MAIN_WINDOW_SIZE = {0};

String RENDERER_MAIN_WINDOW_TITLE = {0};
RendererShaderProgramHandle RENDERER_MAIN_SHADER_PROGRAM = 0;

RendererTexture RENDERER_MAIN_TEXTURES[RENDERER_TEXTURE_MAX_COUNT] = {0};
size_t RENDERER_MAIN_TEXTURES_COUNT = 0;

void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    RENDERER_MAIN_WINDOW_SIZE.x = width;
    RENDERER_MAIN_WINDOW_SIZE.y = height;
    glViewport(0, 0, width, height);
}

void TransformToModelMatrix(Vector3 position, Vector3 rotation, Vector3 scale, mat4 *matrix)
{
    DebugAssertNullPointerCheck(matrix);

    glm_mat4_identity((vec4 *)matrix);

    glm_translate((vec4 *)matrix, (vec3){position.x, position.y, position.z});

    glm_rotate((vec4 *)matrix, rotation.x, (vec3){1, 0, 0});
    glm_rotate((vec4 *)matrix, rotation.y, (vec3){0, 1, 0});
    glm_rotate((vec4 *)matrix, rotation.z, (vec3){0, 0, 1});

    glm_scale((vec4 *)matrix, (vec3){scale.x, scale.y, scale.z});
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

void *Renderer_CreateContext(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync, bool fullScreen)
{
    DebugAssert(glfwInit(), "Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, RENDERER_OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, RENDERER_OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DebugInfo("GLFW initialized successfully.");

    RENDERER_MAIN_WINDOW = glfwCreateWindow(windowSize.x, windowSize.y, title.characters, NULL, NULL);
    DebugAssertNullPointerCheck(RENDERER_MAIN_WINDOW);

    glfwMakeContextCurrent(RENDERER_MAIN_WINDOW);

    // GLint maxUniformBlockSize;
    // glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    // DebugCheckRenderer();
    // DebugInfo("Max supported uniform block size in system : %d bytes, %d matrices\n", maxUniformBlockSize, maxUniformBlockSize / 64);

    glfwSetFramebufferSizeCallback(RENDERER_MAIN_WINDOW, RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    DebugInfo("Main window created successfully.");

    DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");
    DebugInfo("GLAD initialized successfully.");

    Renderer_ConfigureContext(windowSize, vSync, fullScreen);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint glslHasCompiled = 0;
    char glslInfoLog[RENDERER_OPENGL_INFO_LOG_BUFFER] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&vertexShaderSource.characters, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(vertexShader, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Vertex shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Vertex shader compiled successfully. Logs:\n%s", glslInfoLog);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Fragment shader compilation failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Fragment shader compiled successfully. Logs:\n%s", glslInfoLog);

    RENDERER_MAIN_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_MAIN_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_MAIN_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    glGetProgramInfoLog(RENDERER_MAIN_SHADER_PROGRAM, RENDERER_OPENGL_INFO_LOG_BUFFER, NULL, glslInfoLog);

    DebugAssert(glslHasCompiled != GL_FALSE, "Shader program linking failed. Logs:\n%s", glslInfoLog);
    DebugInfo("Shader program linked successfully. Logs:\n%s", glslInfoLog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RENDERER_MAIN_WINDOW_TITLE = title;

    DebugInfo("Renderer initialized successfully.");

    return (void *)RENDERER_MAIN_WINDOW;
}

void Renderer_Terminate()
{
    String_Destroy(&RENDERER_MAIN_WINDOW_TITLE);
    glDeleteProgram(RENDERER_MAIN_SHADER_PROGRAM);
    glfwDestroyWindow(RENDERER_MAIN_WINDOW);
    glfwTerminate();
}

void Renderer_ConfigureContext(Vector2Int windowSize, bool vSync, bool fullScreen)
{
    RENDERER_MAIN_WINDOW_SIZE = windowSize;

    glfwSwapInterval(vSync);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (fullScreen)
    {
        glfwSetWindowMonitor(RENDERER_MAIN_WINDOW, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(RENDERER_MAIN_WINDOW, NULL, 100, 100, RENDERER_MAIN_WINDOW_SIZE.x, RENDERER_MAIN_WINDOW_SIZE.y, 0);
    }

    RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(RENDERER_MAIN_WINDOW, RENDERER_MAIN_WINDOW_SIZE.x, RENDERER_MAIN_WINDOW_SIZE.y);
}

void Renderer_StartRendering()
{
    if (glfwWindowShouldClose(RENDERER_MAIN_WINDOW))
    {
        DebugInfo("Main window close input received");
        Terminate(EXIT_SUCCESS, "Main window close input received");
    }

    glClearColor(RENDERER_OPENGL_CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);
}

void Renderer_FinishRendering(float deltaTime)
{
    glfwSwapBuffers(RENDERER_MAIN_WINDOW);

    char titleBuffer[TEMP_BUFFER_SIZE];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s | FPS: %f | Frame Time: %f ms", RENDERER_MAIN_WINDOW_TITLE.characters, 1.0f / deltaTime, deltaTime * 1000);
    glfwSetWindowTitle(RENDERER_MAIN_WINDOW, titleBuffer);
}

void Renderer_RenderScene(RendererScene *scene)
{
    glBindVertexArray(scene->vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene->vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->uboObjectMatrices);

    for (size_t i = 0; i < scene->batches.count; i++)
    {
        RendererBatch *batch = *(RendererBatch **)ListArray_Get(scene->batches, i);
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

    DebugCheckRenderer();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // DebugInfo("Scene '%s' rendered", scene.name.characters);
}

#pragma endregion Renderer

#pragma region Renderer Model

RendererModel *RendererModel_CreateOBJ(String name, String objFileSource, size_t objFileSourceLineCount, String objFilePath, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset)
{
    Timer modelTimer = Timer_Create("Model Import Timer");
    Timer_Start(&modelTimer);

    mat4 offsetMatrix;
    TransformToModelMatrix(positionOffset, rotationOffset, scaleOffset, &offsetMatrix);

    size_t meshCount = 0;

    size_t totalVertexPositionCount = 0;
    size_t totalVertexNormalCount = 0;
    size_t totalVertexUvCount = 0;

    size_t faceCounts[RENDERER_MODEL_MAX_CHILD_MESH_COUNT] = {0};

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
        else if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            faceCounts[meshCount - 1]++;
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
            for (size_t j = 1; j < lineTokenCount; j++)
            {
                String faceData[3];
                size_t faceDataCount;
                String_Tokenize(lineTokens[j], scl("/"), &faceDataCount, faceData, 3);

                RendererMeshVertex vertex = *(RendererMeshVertex *)ListArray_Get(model->vertices, (size_t)String_ToInt(faceData[0]) - 1);

                // 15

                if (faceDataCount == 2) // 15/16
                {
                    vertex.vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (size_t)String_ToInt(faceData[1]) - 1);
                }
                else if (faceDataCount > 2) // 15/16/17 ...
                {
                    vertex.vertexUV = *(Vector2 *)ListArray_Get(globalVertexUvPool, (size_t)String_ToInt(faceData[1]) - 1);
                    vertex.vertexNormal = *(Vector3 *)ListArray_Get(globalVertexNormalPool, (size_t)String_ToInt(faceData[2]) - 1);
                }

                int createdIndex = (int)String_ToInt(faceData[0]);
                unsigned int index = 0;

                index = createdIndex < 0 ? (unsigned int)model->vertices.count + (unsigned int)createdIndex : (unsigned int)createdIndex - 1;

                ListArray_Add(&currentMesh->indices, &index);
            }
        }
        else if (String_Compare(firstToken, strUSEMTL) == 0) // use material and create object
        {
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
    MemoryCopy(tempTitle, sizeof(tempTitle), model->name.characters);

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
    scene->batches = ListArray_Create("Renderer Batch Pointer", sizeof(RendererBatch *), initialBatchCapacity);

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
    MemoryCopy(tempTitle, sizeof(tempTitle), scene->name.characters);

    String_Destroy(&scene->name);
    scene->camera = NULL;

    for (size_t i = scene->batches.count - 1; i >= 0; i--)
    {
        RendererBatch *batch = *(RendererBatch **)ListArray_Get(scene->batches, i);
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

void RendererScene_SetMainCamera(RendererScene *scene, RendererCamera *camera)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(camera);

    scene->camera = camera;
}

RendererBatch *RendererScene_CreateBatch(RendererScene *scene, String name, RendererModel *model, size_t initialObjectCapacity)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(model);

    RendererBatch *batch = malloc(sizeof(RendererBatch));
    DebugAssertNullPointerCheck(batch);

    batch->name = scc(name);
    batch->model = model;
    batch->scene = scene;
    batch->objectMatrices = ListArray_Create("Matrix 4x4", sizeof(mat4), initialObjectCapacity);

    ListArray_Add(&scene->batches, &batch);

    DebugInfo("Renderer Batch '%s' created.", batch->name.characters);

    return batch;
}

void RendererScene_DestroyBatch(RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, sizeof(tempTitle), batch->name.characters);

    ListArray_RemoveAtIndex(&batch->scene->batches, batch->batchOffsetInScene);
    for (size_t i = batch->batchOffsetInScene; i < batch->scene->batches.count - batch->batchOffsetInScene; i++)
    {
        RendererBatch *nextBatch = *(RendererBatch **)ListArray_Get(batch->scene->batches, i);
        nextBatch->batchOffsetInScene--;
    }

    String_Destroy(&batch->name);
    batch->model = NULL;

    ListArray_Destroy(&batch->objectMatrices);

    free(batch);
    batch = NULL;

    DebugInfo("Renderer Batch '%s' destroyed.", tempTitle);
}

#pragma endregion Renderer Scene

#pragma region Renderer Batch

RendererRenderable *RendererBatch_CreateRenderable(RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);
    //    DebugAssert(batch->objectMatrices.count <= RENDERER_BATCH_MAX_OBJECT_COUNT, "Maximum object capacity of batch is reached : %d", RENDERER_BATCH_MAX_OBJECT_COUNT);

    RendererRenderable *object = (RendererRenderable *)malloc(sizeof(RendererRenderable));
    DebugAssertNullPointerCheck(object);

    object->batch = batch;
    object->matrixOffsetInBatch = object->batch->objectMatrices.count;

    mat4 temp; // dummy, just to allocate the space in list
    ListArray_Add(&object->batch->objectMatrices, &temp);

    // DebugInfo("Renderer Object '%s' created.", object->name.characters);

    return object;
}

void RendererBatch_DestroyRenderable(RendererRenderable *object)
{
    DebugAssertNullPointerCheck(object);

    ListArray_RemoveAtIndex(&object->batch->objectMatrices, object->matrixOffsetInBatch);
    for (size_t i = object->matrixOffsetInBatch; i < object->batch->objectMatrices.count - object->matrixOffsetInBatch; i++)
    {
        RendererRenderable *nextObject = (RendererRenderable *)ListArray_Get(object->batch->objectMatrices, i);
        nextObject->matrixOffsetInBatch--;
    }

    free(object);
    object = NULL;
}

#pragma endregion Renderer Batch

#pragma region Renderer Camera

RendererCamera *RendererCamera_Create(RendererScene *scene)
{
    RendererCamera *camera = malloc(sizeof(RendererCamera));
    DebugAssertNullPointerCheck(camera);

    camera->scene = scene;
    scene->camera = camera;

    camera->isPerspective = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE;
    camera->size = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE ? RENDERER_CAMERA_DEFAULT_FOV : RENDERER_CAMERA_DEFAULT_ORTHOGRAPHIC_SIZE;

    camera->nearClipPlane = RENDERER_CAMERA_DEFAULT_NEAR_CLIP_PLANE;
    camera->farClipPlane = RENDERER_CAMERA_DEFAULT_FAR_CLIP_PLANE;

    return camera;
}

void RendererCamera_Destroy(RendererCamera *camera)
{
    DebugAssertNullPointerCheck(camera);

    camera->size = -1.0f;

    free(camera);
    camera = NULL;
}

void RendererCamera_Configure(RendererCamera *camera, bool isPerspective, float value, float nearClipPlane, float farClipPlane)
{
    DebugAssertNullPointerCheck(camera);

    camera->isPerspective = isPerspective;
    camera->size = value;
    camera->nearClipPlane = nearClipPlane;
    camera->farClipPlane = farClipPlane;
}

void RendererCamera_Update(RendererCamera *camera, Vector3 position, Vector3 rotation)
{
    DebugAssertNullPointerCheck(camera);
    DebugAssertNullPointerCheck(camera->scene);

    mat4 viewMatrix = {0};
    mat4 projectionMatrix = {0};

    glm_mat4_identity((vec4 *)&viewMatrix);
    glm_mat4_identity((vec4 *)&projectionMatrix);

    Vector3 direction = Vector3_Normalized(NewVector3(Cos(rotation.x) * Cos(rotation.y),
                                                      Sin(rotation.x),
                                                      Cos(rotation.x) * Sin(rotation.y)));

    Vector3 center = Vector3_Add(position, Vector3_Normalized(direction));

    glm_lookat((float *)&position, (float *)&center, (vec3){0, 1, 0}, (vec4 *)&viewMatrix);

    if (camera->isPerspective)
    {
        glm_perspective(DegToRad(camera->size),
                        (float)RENDERER_MAIN_WINDOW_SIZE.x / (float)RENDERER_MAIN_WINDOW_SIZE.y,
                        camera->nearClipPlane,
                        camera->farClipPlane,
                        (vec4 *)&projectionMatrix);
    }
    else
    {
        float sizeX = (float)RENDERER_MAIN_WINDOW_SIZE.x * camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)RENDERER_MAIN_WINDOW_SIZE.y * camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        glm_ortho(-sizeX, sizeX, -sizeY, sizeY, camera->nearClipPlane, camera->farClipPlane, (vec4 *)&projectionMatrix);
    }

    rotation = Vector3_Scale(rotation, PI_M / 180.0f);

    glUniformMatrix4fv(camera->scene->camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&projectionMatrix);
    glUniformMatrix4fv(camera->scene->camViewMatrix, 1, GL_FALSE, (GLfloat *)&viewMatrix);
    glUniform3fv(camera->scene->camPosition, 1, (GLfloat *)&position);
    glUniform3fv(camera->scene->camRotation, 1, (GLfloat *)&rotation);

    // DebugInfo("Renderer Camera '%s' updated", camera->name.characters);
}

#pragma endregion Renderer Camera

#pragma region Renderer Renderable

void RendererRenderable_Update(RendererRenderable *renderable, Vector3 position, Vector3 rotation, Vector3 scale)
{
    mat4 *matrix = (mat4 *)ListArray_Get(renderable->batch->objectMatrices, renderable->matrixOffsetInBatch);

    TransformToModelMatrix(position, rotation, scale, matrix);
}

#pragma endregion Renderer Renderable
