#include "tools/Renderer.h"
#include "tools/Resources.h"

#include "utilities/Timer.h"

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

GLFWwindow *RENDERER_MAIN_WINDOW = NULL;
Vector2Int RENDERER_MAIN_WINDOW_SIZE = {0};

float RENDERER_DELTA_TIME = 0.0f;
Timer RENDERER_TIMER = {0};
String RENDERER_MAIN_WINDOW_TITLE = {0};
RendererShaderProgramHandle RENDERER_MAIN_SHADER_PROGRAM = 0;

typedef struct RendererMaterial
{
    String name;
    Color ambientColor;
    Color diffuseColor;
    Color specularColor;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
    RendererTextureHandle diffuseMapHandle;
} RendererMaterial;

typedef struct RendererMesh
{
    ListArray vertices; // RendererMeshVertex
    ListArray indices;  // RendererMeshIndex
    RendererMaterial *material;
} RendererMesh;

void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    glViewport(0, 0, width, height);
}

#pragma region Renderer Material

RendererMaterial *RendererMaterial_Create(String name)
{
    RendererMaterial *material = (RendererMaterial *)malloc(sizeof(RendererMaterial));
    DebugAssertNullPointerCheck(material);

    material->name = name;

    return material;
}

void RendererMaterial_Destroy(RendererMaterial *material)
{
    material->ambientColor = NewColor(-1.0f, -1.0f, -1.0f, -1.0f);
    material->diffuseColor = NewColor(-1.0f, -1.0f, -1.0f, -1.0f);
    glDeleteTextures(1, &material->diffuseMapHandle);
    material->diffuseMapHandle = 0;
    material->dissolve = -1.0f;
    material->illuminationModel = -1;
    String_Destroy(&material->name);
    material->refractionIndex = -1.0f;
    material->specularColor = NewColor(-1.0f, -1.0f, -1.0f, -1.0f);
    material->specularExponent = -1.0f;

    free(material);

    material = NULL;
}

#pragma endregion Renderer Material

#pragma region Renderer Mesh

RendererMesh *RendererMesh_CreateEmpty(size_t initialVertexCapacity, size_t initialIndexCapacity)
{
    RendererMesh *mesh = malloc(sizeof(RendererMesh));
    DebugAssertNullPointerCheck(mesh);

    mesh->vertices = ListArray_Create("Renderer Mesh Vertices", sizeof(RendererMeshVertex), initialVertexCapacity);
    mesh->indices = ListArray_Create("Renderer Mesh Indices", sizeof(RendererMeshIndex), initialIndexCapacity);

    return mesh;
}

void RendererMesh_Destroy(RendererMesh *mesh)
{
    DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->vertices);
    ListArray_Destroy(&mesh->indices);

    mesh->material = NULL;

    free(mesh);
    mesh = NULL;

    DebugInfo("Mesh destroyed with %zu vertices and %zu indices", mesh->vertices.count, mesh->indices.count);
}

#pragma endregion Renderer Mesh

#pragma endregion Source Only

#pragma region Renderer

void Renderer_CreateContext(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync, bool fullScreen)
{
    DebugAssert(glfwInit(), "Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, RENDERER_OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, RENDERER_OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DebugInfo("GLFW initialized successfully.");

    RENDERER_MAIN_WINDOW = glfwCreateWindow(windowSize.x, windowSize.y, title.characters, NULL, NULL);
    DebugAssertNullPointerCheck(RENDERER_MAIN_WINDOW);

    glfwMakeContextCurrent(RENDERER_MAIN_WINDOW);

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

    RENDERER_TIMER = Timer_Create("Renderer Timer");

    RENDERER_MAIN_WINDOW_TITLE = title;

    DebugInfo("Renderer initialized successfully.");
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
}

void Renderer_StartRendering()
{
    Timer_Start(&RENDERER_TIMER);

    if (glfwWindowShouldClose(RENDERER_MAIN_WINDOW))
    {
        DebugInfo("Main window close input received");
        Terminate(EXIT_SUCCESS, "Main window close input received");
    }

    glClearColor(RENDERER_OPENGL_CLEAR_COLOR);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);
}

void Renderer_FinishRendering()
{
    glfwSwapBuffers(RENDERER_MAIN_WINDOW);
    glfwPollEvents();

    // DebugInfo("Rendering finished");
    Timer_Stop(&RENDERER_TIMER);
    RENDERER_DELTA_TIME = (float)Timer_GetElapsedNanoseconds(RENDERER_TIMER) / 1000000000.0f;

    char titleBuffer[TEMP_BUFFER_SIZE];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s | FPS: %d | Frame Time: %f ms", RENDERER_MAIN_WINDOW_TITLE.characters, (int)(1 / RENDERER_DELTA_TIME), RENDERER_DELTA_TIME * 1000);
    glfwSetWindowTitle(RENDERER_MAIN_WINDOW, titleBuffer);

    // DebugInfo("Rendering finished : FPS: %d | Frame Time: %f ms", (int)(1 / RENDERER_DELTA_TIME), RENDERER_DELTA_TIME * 1000);
}

void Renderer_RenderScene(RendererScene *scene)
{
    glBindVertexArray(scene->vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene->vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->uboObjectMatrices);

    glUniformMatrix4fv(scene->camProjectionMatrix, 1, GL_FALSE, (GLfloat *)&scene->camera->projectionMatrix);
    glUniformMatrix4fv(scene->camViewMatrix, 1, GL_FALSE, (GLfloat *)&scene->camera->viewMatrix);

    for (size_t i = 0; i < scene->batches.count; i++)
    {
        RendererBatch *batch = *(RendererBatch **)ListArray_Get(scene->batches, i);
        RendererMaterial *previousMaterial = NULL;

        glBufferData(GL_UNIFORM_BUFFER,
                     (long long)(batch->objectMatrices.sizeOfItem * batch->objectMatrices.count),
                     batch->objectMatrices.data,
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
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh->material->diffuseMapHandle);
                glUniform1i(scene->matDiffuseMap, 0);

                previousMaterial = mesh->material;
            }

            glBufferData(GL_ARRAY_BUFFER,
                         (long long)(mesh->vertices.sizeOfItem * mesh->vertices.count),
                         mesh->vertices.data,
                         OPENGL_DRAW_TYPE);

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

#pragma region Renderer Object Transform

void RendererObjectTransform_SetPosition(RendererTransform *transform, Vector3 position)
{
    DebugAssertNullPointerCheck(transform);

    transform->position = position;
}

void RendererObjectTransform_SetRotation(RendererTransform *transform, Vector3 rotation)
{
    DebugAssertNullPointerCheck(transform);

    transform->rotation = rotation;
}

void RendererObjectTransform_SetScale(RendererTransform *transform, Vector3 scale)
{
    DebugAssertNullPointerCheck(transform);

    transform->scale = scale;
}

void RendererObjectTransform_AddPosition(RendererTransform *transform, Vector3 position)
{
    DebugAssertNullPointerCheck(transform);

    transform->position = Vector3_Add(transform->position, position);
}

void RendererObjectTransform_AddRotation(RendererTransform *transform, Vector3 rotation)
{
    DebugAssertNullPointerCheck(transform);

    transform->rotation = Vector3_Add(transform->rotation, rotation);
}

void RendererObjectTransform_MultiplyScale(RendererTransform *transform, Vector3 scale)
{
    DebugAssertNullPointerCheck(transform);

    transform->scale.x *= scale.x;
    transform->scale.y *= scale.y;
    transform->scale.z *= scale.z;
}

void RendererObjectTransform_ToModelMatrix(RendererTransform *transform, mat4 *matrix)
{
    DebugAssertNullPointerCheck(transform);
    DebugAssertNullPointerCheck(matrix);

    glm_mat4_identity((vec4 *)matrix);

    glm_translate((vec4 *)matrix, (vec3){transform->position.x, transform->position.y, transform->position.z});

    glm_rotate((vec4 *)matrix, transform->rotation.x, (vec3){1, 0, 0});
    glm_rotate((vec4 *)matrix, transform->rotation.y, (vec3){0, 1, 0});
    glm_rotate((vec4 *)matrix, transform->rotation.z, (vec3){0, 0, 1});

    glm_scale((vec4 *)matrix, (vec3){transform->scale.x, transform->scale.y, transform->scale.z});
}

#pragma endregion Renderer Object Transform

#pragma region Renderer Model

RendererModel *RendererModel_CreateOBJ(String name, String objFileSource, String objFilePath, RendererTransform modelOffset)
{
    Timer modelTimer = Timer_Create("Model Import Timer");
    Timer_Start(&modelTimer);

    ListArray materials = {0}; // RendererMaterial*

    mat4 offsetMatrix;
    vec3 finalVector;
    RendererObjectTransform_ToModelMatrix(&modelOffset, &offsetMatrix);

    size_t meshCount = 0;
    size_t meshIndex = 0;

    size_t vertexCounts[RENDERER_MODEL_MAX_CHILD_MESH_COUNT] = {0};
    size_t vertexNormalCounts[RENDERER_MODEL_MAX_CHILD_MESH_COUNT] = {0};
    size_t vertexUvCounts[RENDERER_MODEL_MAX_CHILD_MESH_COUNT] = {0};
    size_t faceCounts[RENDERER_MODEL_MAX_CHILD_MESH_COUNT] = {0};

    size_t lineCount = 0;
    String *lines = (String *)malloc(RESOURCE_FILE_MAX_LINE_COUNT * sizeof(String));

    size_t lineTokenCount = 0;
    String lineTokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};

    String strNewline = scl("\n");
    String strSpace = scl(" ");
    String strV = scl("v");
    String strF = scl("f");
    String strVT = scl("vt");
    String strVN = scl("vn");
    String strO = scl("o");
    String strG = scl("g");
    String strMTLLIB = scl("mtllib");
    String strUSEMTL = scl("usemtl");

    String_Tokenize(objFileSource, strNewline, &lineCount, lines, RESOURCE_FILE_MAX_LINE_COUNT * sizeof(String));
    for (size_t i = 0; i < lineCount; i++) // count
    {
        String_Tokenize(lines[i], strSpace, &lineTokenCount, lineTokens, sizeof(lineTokens));

        String firstToken = lineTokens[0];

        if (String_Compare(firstToken, strV) == 0) // v -7.579129 4.591946 4.850700
        {
            vertexCounts[meshCount - 1]++;
        }
        else if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            faceCounts[meshCount - 1]++;
        }
        else if (String_Compare(firstToken, strVT) == 0) // vt 0.073128 0.431854
        {
            vertexUvCounts[meshCount - 1]++;
        }
        else if (String_Compare(firstToken, strVN) == 0) // vn -0.0233 0.1253 -0.9918
        {
            vertexNormalCounts[meshCount - 1]++;
        }
        else if (String_Compare(firstToken, strO) == 0) // new object
        {
            meshCount++;
        }
        else if (String_Compare(firstToken, strMTLLIB) == 0) // .mtl file
        {
            char nameBuffer[TEMP_BUFFER_SIZE] = {0};
            snprintf(nameBuffer, sizeof(nameBuffer), "%s Material File", name.characters);

            String tempStr = String_CreateCopy(objFilePath.characters);
            String_ConcatEnd(&tempStr, lineTokens[1]);

            Resource *mtlFileResource = Resource_Create(scl(nameBuffer), tempStr);
            String_Destroy(&tempStr);

            size_t materialCount = 0;

            size_t mtlLineCount = 0;
            String mtlLines[RESOURCE_FILE_MAX_LINE_COUNT] = {0};

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

            String_Tokenize(mtlFileResource->data, strNewline, &mtlLineCount, mtlLines, sizeof(mtlLines));

            for (size_t j = 0; j < mtlLineCount; j++) // count
            {
                String_Tokenize(mtlLines[j], strSpace, &mtlLineTokenCount, mtlLineTokens, sizeof(mtlLineTokens));

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
                String_Tokenize(lines[i], strSpace, &mtlLineTokenCount, mtlLineTokens, sizeof(mtlLineTokens));

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
                        NewColor(String_ToFloat(mtlLineTokens[1]),
                                 String_ToFloat(mtlLineTokens[2]),
                                 String_ToFloat(mtlLineTokens[3]),
                                 1.0f);
                }
                else if (String_Compare(mtlFirstToken, strKD) == 0)
                {
                    currentMaterial->diffuseColor =
                        NewColor(String_ToFloat(mtlLineTokens[1]),
                                 String_ToFloat(mtlLineTokens[2]),
                                 String_ToFloat(mtlLineTokens[3]),
                                 1.0f);
                }
                else if (String_Compare(mtlFirstToken, strKS) == 0)
                {
                    currentMaterial->specularColor =
                        NewColor(String_ToFloat(mtlLineTokens[1]),
                                 String_ToFloat(mtlLineTokens[2]),
                                 String_ToFloat(mtlLineTokens[3]),
                                 1.0f);
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
                    char imageNameBuffer[TEMP_BUFFER_SIZE] = {0};
                    snprintf(imageNameBuffer, sizeof(imageNameBuffer), "%s : %s", name.characters, lineTokens[1].characters);

                    String imageTempStr = String_CreateCopy(objFilePath.characters);
                    String_ConcatEnd(&imageTempStr, lineTokens[1]);

                    ResourceImage *image = ResourceImage_Create(scl(imageNameBuffer), imageTempStr);
                    String_Destroy(&imageTempStr);

                    currentMaterial->diffuseMapHandle = image->handle;
                }
            }

            Resource_Destroy(mtlFileResource);
        }
    }

    RendererModel *model = RendererModel_CreateEmpty(name, meshCount);
    RendererMesh *currentMesh = NULL;
    RendererMaterial *currentMaterial = NULL;

    ListArray currentVertexNormalPool = ListArray_Create("Vector3", sizeof(Vector3), 1); // Vector3
    ListArray currentVertexUvPool = ListArray_Create("Vector2", sizeof(Vector2), 1);     // Vector2

    for (size_t i = 0; i < lineCount; i++) // compute
    {
        String_Tokenize(lines[i], scl(" "), &lineTokenCount, lineTokens, sizeof(lineTokens));

        String firstToken = lineTokens[0];

        if (String_Compare(firstToken, strV) == 0) // v -7.579129 4.591946 4.850700
        {
            glm_mat4_mulv3((vec4 *)&offsetMatrix,
                           (vec3){String_ToFloat(lineTokens[1]),
                                  String_ToFloat(lineTokens[2]),
                                  String_ToFloat(lineTokens[3])},
                           0.0f,
                           (float *)&finalVector);

            RendererMeshVertex createdVertex;
            createdVertex.vertexPosition = NewVector3(finalVector[0], finalVector[1], finalVector[2]);

            ListArray_Add(&currentMesh->vertices, &createdVertex);
        }
        else if (String_Compare(firstToken, strF) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            for (size_t j = 1; j < lineTokenCount; j++)
            {
                String faceData[3];
                size_t faceDataCount;
                String_Tokenize(lineTokens[j], scl("/"), &faceDataCount, faceData, sizeof(faceData));

                if ((size_t)String_ToInt(faceData[0]) - 1 >= currentMesh->vertices.count)
                {
                    // todo DebugWarning("Incorrect .obj file format");
                    continue;
                }

                RendererMeshVertex currentVertex = *(RendererMeshVertex *)ListArray_Get(currentMesh->vertices, (size_t)String_ToInt(faceData[0]) - 1);

                if (faceDataCount == 2) // 15/16
                {
                    if ((size_t)String_ToInt(faceData[1]) - 1 >= currentVertexUvPool.count)
                    {
                        // todo DebugWarning("Incorrect .obj file format");
                        continue;
                    }
                    currentVertex.vertexUV = *(Vector2 *)ListArray_Get(currentVertexUvPool, (size_t)String_ToInt(faceData[1]) - 1);
                }
                else if (faceDataCount == 3) // 15/16/17
                {
                    currentVertex.vertexUV = *(Vector2 *)ListArray_Get(currentVertexUvPool, (size_t)String_ToInt(faceData[1]) - 1);
                    currentVertex.vertexNormal = *(Vector3 *)ListArray_Get(currentVertexNormalPool, (size_t)String_ToInt(faceData[2]) - 1);
                }

                unsigned int createdIndex = (unsigned int)String_ToInt(faceData[0]) - 1;

                ListArray_Add(&currentMesh->indices, &createdIndex);
            }
        }
        else if (String_Compare(firstToken, strVT) == 0) // vt 0.073128 0.431854
        {
            Vector2 vertexUv =
                NewVector2(String_ToFloat(lineTokens[1]),
                           String_ToFloat(lineTokens[2]));

            ListArray_Add(&currentVertexUvPool, &vertexUv);
        }
        else if (String_Compare(firstToken, strVN) == 0) // vn -0.0233 0.1253 -0.9918
        {
            Vector3 vertexNormal =
                NewVector3(String_ToFloat(lineTokens[1]),
                           String_ToFloat(lineTokens[2]),
                           String_ToFloat(lineTokens[3]));

            // todo maybe matrix mult here too

            ListArray_Add(&currentVertexNormalPool, &vertexNormal);
        }
        else if (String_Compare(firstToken, strO) == 0) // new mesh object
        {
            ListArray_Clear(&currentVertexNormalPool);
            ListArray_Clear(&currentVertexUvPool);

            ListArray_Resize(&currentVertexNormalPool, vertexNormalCounts[meshIndex]);
            ListArray_Resize(&currentVertexUvPool, vertexUvCounts[meshIndex]);

            currentMesh = RendererMesh_CreateEmpty(vertexCounts[meshIndex], faceCounts[meshIndex] * 3);
            ListArray_Add(&model->meshes, &currentMesh);

            currentMesh->material = currentMaterial;
            meshIndex++;
        }
        else if (String_Compare(firstToken, strG) == 0) // new mesh object group
        {
        }
        else if (String_Compare(firstToken, strUSEMTL) == 0) // use material
        {
            for (size_t j = 0; j < materials.count; j++)
            {
                RendererMaterial *tempMaterial = *(RendererMaterial **)ListArray_Get(materials, j);
                if (String_Compare(tempMaterial->name, lineTokens[1]) == 0)
                {
                    currentMaterial = tempMaterial;
                    currentMesh->material = tempMaterial;
                    break;
                }
            }
        }
    }

    ListArray_Destroy(&currentVertexNormalPool);
    ListArray_Destroy(&currentVertexUvPool);

    free(lines);

    Timer_Stop(&modelTimer);

    DebugInfo("Renderer Model '%s' imported successfully with %zu child meshes in %f seconds.", model->name, model->meshes.count, (double)Timer_GetElapsedNanoseconds(modelTimer) / 1000000000.0);

    return model;
}

RendererModel *RendererModel_CreateEmpty(String name, size_t initialMeshCapacity)
{
    RendererModel *model = malloc(sizeof(RendererModel));
    DebugAssertNullPointerCheck(model);

    model->name = name;
    model->meshes = ListArray_Create("Renderer Mesh Pointer", sizeof(RendererMesh *), initialMeshCapacity);

    return model;
}

void RendererModel_Destroy(RendererModel *model)
{
    DebugAssertNullPointerCheck(model);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, sizeof(tempTitle), model->name.characters);

    String_Destroy(&model->name);

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

    scene->name = name;
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

    scene->matAmbientColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matAmbientColor");
    scene->matDiffuseColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDiffuseColor");
    scene->matSpecularColor = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matSpecularColor");
    scene->matSpecularExponent = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matSpecularExponent");
    scene->matDissolve = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDissolve");
    scene->matDiffuseMap = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "matDiffuseMap");

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
        RendererBatch_Destroy(batch);
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

#pragma endregion Renderer Scene

#pragma region Renderer Batch

RendererBatch *RendererBatch_Create(String name, RendererScene *scene, RendererModel *model, size_t initialObjectCapacity)
{
    RendererBatch *batch = malloc(sizeof(RendererBatch));
    DebugAssertNullPointerCheck(batch);

    batch->name = name;
    batch->model = model;
    batch->scene = scene;
    batch->objectMatrices = ListArray_Create("Matrix 4x4", sizeof(mat4), initialObjectCapacity);

    ListArray_Add(&scene->batches, &batch);

    DebugInfo("Renderer Batch '%s' created.", batch->name.characters);

    return batch;
}

void RendererBatch_Destroy(RendererBatch *batch)
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

#pragma endregion Renderer Batch

#pragma region Renderer Camera

RendererCamera *RendererCamera_Create(String name, RendererScene *scene)
{
    RendererCamera *camera = malloc(sizeof(RendererCamera));
    DebugAssertNullPointerCheck(camera);

    camera->name = name;
    camera->transform = RendererObjectTransformDefault;
    camera->scene = scene;
    scene->camera = camera;

    camera->isPerspective = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE;
    camera->size = RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE ? RENDERER_CAMERA_DEFAULT_FOV : RENDERER_CAMERA_DEFAULT_ORTHOGRAPHIC_SIZE;

    camera->nearClipPlane = RENDERER_CAMERA_DEFAULT_NEAR_CLIP_PLANE;
    camera->farClipPlane = RENDERER_CAMERA_DEFAULT_FAR_CLIP_PLANE;

    glm_mat4_identity((vec4 *)&camera->projectionMatrix);
    glm_mat4_identity((vec4 *)&camera->viewMatrix);

    DebugInfo("Renderer Camera '%s' created.", camera->name.characters);

    return camera;
}

void RendererCamera_Destroy(RendererCamera *camera)
{
    DebugAssertNullPointerCheck(camera);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, sizeof(tempTitle), camera->name.characters);

    String_Destroy(&camera->name);
    camera->transform = RendererObjectTransformDefault;

    camera->size = -1.0f;

    glm_mat4_identity((vec4 *)&camera->projectionMatrix);
    glm_mat4_identity((vec4 *)&camera->viewMatrix);

    free(camera);
    camera = NULL;

    DebugInfo("Renderer Camera '%s' Object destroyed", tempTitle);
}

void RendererCamera_Configure(RendererCamera *camera, bool isPerspective, float value, float nearClipPlane, float farClipPlane)
{
    DebugAssertNullPointerCheck(camera);

    camera->isPerspective = isPerspective;
    camera->size = value;
    camera->nearClipPlane = nearClipPlane;
    camera->farClipPlane = farClipPlane;
}

void RendererCamera_Update(RendererCamera *camera)
{
    DebugAssertNullPointerCheck(camera);

    glm_mat4_identity((vec4 *)&camera->viewMatrix);

    Vector3 direction = NewVector3(cosf(camera->transform.rotation.x) * cosf(camera->transform.rotation.y),
                                   sinf(camera->transform.rotation.x),
                                   cosf(camera->transform.rotation.x) * sinf(camera->transform.rotation.y));

    direction = Vector3_Normalized(direction);

    Vector3 center = Vector3_Add(camera->transform.position, Vector3_Normalized(direction));

    glm_lookat((float *)&camera->transform.position, (float *)&center, (vec3){0, 1, 0}, (vec4 *)&camera->viewMatrix);

    glm_mat4_identity((vec4 *)&camera->projectionMatrix);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (camera->isPerspective)
    {
        glm_perspective(camera->size, (float)(mode->width / mode->height), camera->nearClipPlane, camera->farClipPlane, (vec4 *)&camera->projectionMatrix);
    }
    else
    {
        float sizeX = (float)mode->width * camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)mode->height * camera->size / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        glm_ortho(-sizeX, sizeX, -sizeY, sizeY, camera->nearClipPlane, camera->farClipPlane, (vec4 *)&camera->projectionMatrix);
    }

    // DebugInfo("Renderer Camera '%s' updated", camera->name.characters);
}

#pragma endregion Renderer Camera

#pragma region Renderer Object

RendererObject *RendererObject_Create(String name, RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);

    RendererObject *object = (RendererObject *)malloc(sizeof(RendererObject));
    DebugAssertNullPointerCheck(object);

    object->name = name;
    object->transform = RendererObjectTransformDefault;
    object->hitBoxSize = NewVector3(1.0f, 1.0f, 1.0f);
    object->batch = batch;
    object->matrixOffsetInBatch = object->batch->objectMatrices.count;

    mat4 temp; // dummy, just to allocate the space in list
    ListArray_Add(&object->batch->objectMatrices, &temp);

    // DebugInfo("Renderer Object '%s' created.", object->name.characters);

    return object;
}

void RendererObject_Destroy(RendererObject *object)
{
    DebugAssertNullPointerCheck(object);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, sizeof(tempTitle), object->name.characters);

    object->transform = RendererObjectTransformDefault;
    object->hitBoxSize = NewVector3(0.0f, 0.0f, 0.0f);

    String_Destroy(&object->name);

    ListArray_RemoveAtIndex(&object->batch->objectMatrices, object->matrixOffsetInBatch);
    for (size_t i = object->matrixOffsetInBatch; i < object->batch->objectMatrices.count - object->matrixOffsetInBatch; i++)
    {
        RendererObject *nextObject = (RendererObject *)ListArray_Get(object->batch->objectMatrices, i);
        nextObject->matrixOffsetInBatch--;
    }

    free(object);
    object = NULL;

    DebugInfo("Renderer Object '%s' destroyed.", tempTitle);
}

void RendererObject_Update(RendererObject *object)
{
    mat4 *matrix = (mat4 *)ListArray_Get(object->batch->objectMatrices, object->matrixOffsetInBatch);

    RendererObjectTransform_ToModelMatrix(&object->transform, matrix);

    // DebugInfo("Renderer Object '%s' updated", object->name.characters);
}

#pragma endregion Renderer Object
