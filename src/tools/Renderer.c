#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define OPENGL_DRAW_TYPE GL_STATIC_DRAW

GLFWwindow *RENDERER_MAIN_WINDOW = NULL;

size_t RENDERER_MESH_ID_INDEX = 0;
float RENDERER_DELTA_TIME = 0.0f;
Timer RENDERER_TIMER = {0};
String RENDERER_MAIN_WINDOW_TITLE = {0};
RendererShaderProgramHandle RENDERER_MAIN_SHADER_PROGRAM = 0;

void RENDERER_MAIN_WINDOW_RESIZE_CALLBACK(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    glViewport(0, 0, width, height);
}

#pragma region Renderer

void Renderer_Initialize(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync)
{
    DebugAssert(glfwInit(), "Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DebugInfo("GLFW initialized successfully.");

    RENDERER_MAIN_WINDOW = glfwCreateWindow(windowSize.x, windowSize.y, title.characters, NULL, NULL);
    DebugAssertNullPointerCheck(RENDERER_MAIN_WINDOW);

    glfwMakeContextCurrent(RENDERER_MAIN_WINDOW);

    glfwSetFramebufferSizeCallback(RENDERER_MAIN_WINDOW, RENDERER_MAIN_WINDOW_RESIZE_CALLBACK);
    DebugInfo("Main window created successfully.");

    DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");
    DebugInfo("GLAD initialized successfully.");

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSwapInterval(vSync);

    GLint glslHasCompiled = 0;
    char glslInfoLog[1024] = {0};

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar *const *)&vertexShaderSource.characters, NULL); //!
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &glslHasCompiled);
    if (glslHasCompiled == GL_FALSE)
    {
        glGetShaderInfoLog(vertexShader, 1024, NULL, glslInfoLog);
        DebugError("Vertex shader compilation failed: %s", glslInfoLog);
    }
    DebugInfo("Vertex shader compiled successfully.");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar *const *)&fragmentShaderSource.characters, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &glslHasCompiled);
    if (glslHasCompiled == GL_FALSE)
    {
        glGetShaderInfoLog(fragmentShader, sizeof(glslInfoLog), NULL, glslInfoLog);
        DebugError("Fragment shader compilation failed: %s", glslInfoLog);
    }
    DebugInfo("Fragment shader compiled successfully.");

    RENDERER_MAIN_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, vertexShader);
    glAttachShader(RENDERER_MAIN_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(RENDERER_MAIN_SHADER_PROGRAM);

    glGetProgramiv(RENDERER_MAIN_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    if (glslHasCompiled == GL_FALSE)
    {
        glGetProgramInfoLog(RENDERER_MAIN_SHADER_PROGRAM, 1024, NULL, glslInfoLog);
        DebugError("Shader program linking failed: %s", glslInfoLog);
    }
    DebugInfo("Shader program linked successfully.");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    RENDERER_TIMER = Timer_Create("Renderer Timer");

    RENDERER_MAIN_WINDOW_TITLE = title;

    DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate()
{
    glDeleteProgram(RENDERER_MAIN_SHADER_PROGRAM);
    glfwDestroyWindow(RENDERER_MAIN_WINDOW);
    glfwTerminate();
}

void Renderer_StartRendering()
{
    Timer_Start(&RENDERER_TIMER);

    if (glfwWindowShouldClose(RENDERER_MAIN_WINDOW))
    {
        DebugInfo("Main window close input received");
        Terminate(0);
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(RENDERER_MAIN_SHADER_PROGRAM);

    DebugInfo("Rendering started");
}

void Renderer_FinishRendering()
{
    glfwSwapBuffers(RENDERER_MAIN_WINDOW);
    glfwPollEvents();

    // DebugInfo("Rendering finished");
    Timer_Stop(&RENDERER_TIMER);
    RENDERER_DELTA_TIME = (float)Timer_GetElapsedNanoseconds(RENDERER_TIMER) / 1000000000.0f;

    char titleBuffer[TEMP_BUFFER_SIZE];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s | FPS: %d | Delta Time: %f", RENDERER_MAIN_WINDOW_TITLE.characters, (int)(1 / RENDERER_DELTA_TIME), RENDERER_DELTA_TIME);
    glfwSetWindowTitle(RENDERER_MAIN_WINDOW, titleBuffer);

    DebugInfo("Frame time: %f milliseconds", RENDERER_DELTA_TIME * 1000);
}

void Renderer_RenderScene(RendererScene *scene)
{
    glBindVertexArray(scene->vao);

    // todo find out how to update per model data with buffer or use multiple draw calls
    glDrawElementsInstanced(GL_TRIANGLES, scene->mesh.indices.count, GL_UNSIGNED_INT, scene->mesh.indices.data, scene->modelMatrices.count);

    glBindVertexArray(0);

    // DebugInfo("Scene '%s' rendered", scene.name.characters);
}

#pragma endregion Renderer

#pragma region Renderer Object Transform

void RendererObjectTransform_SetPosition(RendererObjectTransform *transform, Vector3 position)
{
    DebugAssertNullPointerCheck(transform);
    transform->position = position;
}

void RendererObjectTransform_SetRotation(RendererObjectTransform *transform, Vector3 rotation)
{
    DebugAssertNullPointerCheck(transform);
    transform->rotation = rotation;
}

void RendererObjectTransform_SetScale(RendererObjectTransform *transform, Vector3 scale)
{
    DebugAssertNullPointerCheck(transform);
    transform->scale = scale;
}

void RendererObjectTransform_ToModelMatrix(RendererObjectTransform *transform, mat4 *matrix)
{
    glm_mat4_identity((vec4 *)matrix);

    glm_translate((vec4 *)matrix, (vec3){transform->position.x, transform->position.y, transform->position.z});

    glm_rotate((vec4 *)matrix, transform->position.x, (vec3){1, 0, 0});
    glm_rotate((vec4 *)matrix, transform->position.y, (vec3){0, 1, 0});
    glm_rotate((vec4 *)matrix, transform->position.z, (vec3){0, 0, 1});

    glm_scale((vec4 *)matrix, (vec3){transform->scale.x, transform->scale.y, transform->scale.z});
}

#pragma endregion Renderer Object Transform

#pragma region Renderer Mesh

RendererMesh RendererMesh_CreateOBJ(String objFileSource)
{
    RendererMesh mesh = {0};

    mesh.id = ++RENDERER_MESH_ID_INDEX;

    size_t lineCount = 0;
    String lines[RESOURCE_FILE_MAX_LINE_COUNT] = {0};
    String_Tokenize(objFileSource, scl("\n"), &lineCount, lines, sizeof(lines));

    mesh.vertices = ListArray_Create("Renderer Mesh Vertices", sizeof(RendererMeshVertex), lineCount / 4);
    mesh.indices = ListArray_Create("Renderer Mesh Indices", sizeof(RendererMeshIndex), lineCount / 4);

    for (size_t i = 0; i < lineCount; i++)
    {
        size_t tokenCount = 0;
        String tokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT] = {0};
        String_Tokenize(lines[i], scl(" "), &tokenCount, tokens, sizeof(tokens));

        String firstToken = tokens[0];

        if (String_Compare(firstToken, scl("v")) == 0) // v -7.579129 4.591946 4.850700
        {
            RendererMeshVertex vertex = {{String_ToFloat(tokens[1]) / 100,
                                          String_ToFloat(tokens[2]) / 100,
                                          String_ToFloat(tokens[3]) / 100},
                                         {String_ToFloat(tokens[1]) / 10,
                                          String_ToFloat(tokens[2]) / 10,
                                          String_ToFloat(tokens[3]) / 10,
                                          1.0f}};

            ListArray_Add(&mesh.vertices, &vertex);
        }
        else if (String_Compare(firstToken, scl("vt")) == 0) // vt 0.073128 0.431854
        {
            // Process texture coordinate data
        }
        else if (String_Compare(firstToken, scl("vn")) == 0) // vn -0.0233 0.1253 -0.9918
        {
            // Process normal vector data
        }
        else if (String_Compare(firstToken, scl("f")) == 0) // f 15/15/24 102/122/119 116/142/107 67/79/106
        {
            for (int j = 0; j < 3; j++)
            {
                String indices[3]; // 15/15/24
                String_Tokenize(tokens[j + 1], scl("/"), NULL, indices, 3);

                unsigned int index = (unsigned int)String_ToInt(indices[0]);

                ListArray_Add(&mesh.indices, &index);
            }
        }
    }

    DebugInfo("Mesh created with %zu vertices and %zu indices", mesh.vertices.count, mesh.indices.count);

    return mesh;
}

RendererMesh RendererMesh_CreateEmpty(size_t initialVertexCapacity, size_t initialIndexCapacity)
{
    RendererMesh mesh = {0};

    mesh.id = ++RENDERER_MESH_ID_INDEX;

    mesh.vertices = ListArray_Create("Renderer Mesh Vertices", sizeof(RendererMeshVertex), initialVertexCapacity);
    mesh.indices = ListArray_Create("Renderer Mesh Indices", sizeof(RendererMeshIndex), initialIndexCapacity);

    DebugInfo("Empty mesh created with %zu vertex and %zu index capacity", mesh.vertices.capacity, mesh.indices.capacity);

    return mesh;
}

RendererMesh RendererMesh_Copy(RendererMesh mesh)
{
    RendererMesh copiedMesh = {0};

    copiedMesh.id = ++RENDERER_MESH_ID_INDEX;

    copiedMesh.vertices = ListArray_Copy(mesh.vertices);
    copiedMesh.indices = ListArray_Copy(mesh.indices);

    DebugInfo("Renderer mesh copied with %zu vertices and %zu indices", mesh.vertices.count, mesh.indices.count);

    return copiedMesh;
}

void RendererMesh_Destroy(RendererMesh *mesh)
{
    DebugAssertNullPointerCheck(mesh);

    mesh->id = 0;

    ListArray_Destroy(&mesh->vertices);
    ListArray_Destroy(&mesh->indices);

    DebugInfo("Mesh destroyed with %zu vertices and %zu indices", mesh->vertices.count, mesh->indices.count);
}

#pragma endregion Renderer Mesh

#pragma region Renderer Camera Object

RendererCamera RendererCamera_Create(String name)
{
    RendererCamera camera = {0};

    camera.name = name;
    camera.transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};
    camera.isPerspective = true;
    camera.fov = 90.0f;

    RendererCamera_Update(&camera);

    return camera;
}

void RendererCamera_Destroy(RendererCamera *camera)
{
    DebugAssertNullPointerCheck(camera);

    String_Destroy(&camera->name);
    camera->fov = -1.0f;
    camera->transform = (RendererObjectTransform){0};

    DebugInfo("Renderer Camera Object destroyed");
}

void RendererCamera_Configure(RendererCamera *camera, bool isPerspective, float value)
{
    camera->isPerspective = isPerspective;

    if (camera->isPerspective)
    {
        camera->fov = value;
    }
    else
    {
        camera->orthographicSize = value;
    }
}

void RendererCamera_Update(RendererCamera *camera)
{
    glm_mat4_identity((vec4 *)&camera->viewMatrix);

    Vector3 direction = NewVector3(cosf(camera->transform.rotation.x) * cosf(camera->transform.rotation.y),
                                   sinf(camera->transform.rotation.x),
                                   cosf(camera->transform.rotation.x) * sinf(camera->transform.rotation.y));

    Vector3 center = Vector3_Add(camera->transform.position, Vector3_Normalized(direction));

    glm_lookat((float *)&camera->transform.position, (float *)&center, (vec3){0, 1, 0}, (vec4 *)&camera->viewMatrix);

    glm_mat4_identity((vec4 *)&camera->projectionMatrix);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (camera->isPerspective)
    {
        glm_perspective(camera->fov, (float)(mode->width / mode->height), RENDERER_CAMERA_NEAR_CLIP_PLANE, RENDERER_CAMERA_FAR_CLIP_PLANE, (vec4 *)&camera->projectionMatrix);
    }
    else
    {
        float sizeX = (float)mode->width * camera->orthographicSize / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        float sizeY = (float)mode->height * camera->orthographicSize / RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER;
        glm_ortho(-sizeX, sizeX, -sizeY, sizeY, RENDERER_CAMERA_NEAR_CLIP_PLANE, RENDERER_CAMERA_FAR_CLIP_PLANE, (vec4 *)&camera->projectionMatrix);
    }

    DebugInfo("Renderer Camera '%s' updated", camera->name.characters);
}

#pragma endregion Renderer Camera Object

#pragma region Renderer Scene

RendererScene RendererScene_Create(String name, RendererCamera *camera, size_t initialObjectCapacity, size_t initialVertexCapacity, size_t initialIndexCapacity)
{
    RendererScene scene = {0};

    scene.name = name;
    scene.camera = camera;
    scene.mesh = RendererMesh_CreateEmpty(initialVertexCapacity, initialIndexCapacity);
    scene.modelMatrices = ListArray_Create("Matrix 4x4", sizeof(mat4), initialObjectCapacity);

    glGenVertexArrays(1, &scene.vao);
    glGenBuffers(1, &scene.vboModelVertices);
    glGenBuffers(1, &scene.iboModelIndices);
    glGenBuffers(1, &scene.uboModelMatrices);

    glBindVertexArray(scene.vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene.vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene.iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene.uboModelMatrices);

    size_t offset = 0;

    glVertexAttribPointer(RENDERER_VBO_POSITION_BINDING, 3, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_POSITION_BINDING);
    offset += sizeof(Vector3);

    glVertexAttribPointer(RENDERER_VBO_COLOR_BINDING, 4, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(RENDERER_VBO_COLOR_BINDING);
    offset += sizeof(Vector4);

    //! ... other attributes in vertex

    // scene.modelIndexHandle = glGetUniformLocation(RENDERER_MAIN_SHADER_PROGRAM, "modelIndex");
    scene.matricesBlockHandle = glGetUniformBlockIndex(RENDERER_MAIN_SHADER_PROGRAM, "matrices");
    glUniformBlockBinding(RENDERER_MAIN_SHADER_PROGRAM, scene.matricesBlockHandle, RENDERER_UBO_MATRICES_BINDING);
    // glBindBufferBase(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices);
    glBindBufferRange(GL_UNIFORM_BUFFER, RENDERER_UBO_MATRICES_BINDING, scene.uboModelMatrices, 0, RENDERER_SCENE_MAX_OBJECT_COUNT);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindVertexArray(0);

    RendererScene_Update(&scene);

    DebugInfo("Renderer Scene created with name: %s, initial object capacity: %zu, initial vertex capacity: %zu, initial index capacity: %zu",
              scene.name.characters, scene.modelMatrices.capacity, scene.mesh.vertices.capacity, scene.mesh.indices.capacity);

    return scene;
}

void RendererScene_Destroy(RendererScene *scene)
{
    DebugAssertNullPointerCheck(scene);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, TEMP_BUFFER_SIZE, scene->name.characters);

    String_Destroy(&scene->name);

    glDeleteVertexArrays(1, &scene->vao);
    glDeleteBuffers(1, &scene->vboModelVertices);
    glDeleteBuffers(1, &scene->iboModelIndices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RendererMesh_Destroy(&scene->mesh);

    scene = NULL;

    DebugInfo("Renderer Scene destroyed with name: %s, vertices: %zu, indices: %zu", tempTitle, scene->mesh.vertices.count, scene->mesh.indices.count);
}

void RendererScene_SetMainCamera(RendererScene *scene, RendererCamera *camera)
{
    DebugAssertNullPointerCheck(scene);
    DebugAssertNullPointerCheck(camera);

    scene->camera = camera;
}

void RendererScene_Update(RendererScene *scene)
{
    glBindVertexArray(scene->vao);
    glBindBuffer(GL_ARRAY_BUFFER, scene->vboModelVertices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->iboModelIndices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->uboModelMatrices);

    glBufferData(GL_ARRAY_BUFFER, scene->mesh.vertices.sizeOfItem * scene->mesh.vertices.count, scene->mesh.vertices.data, OPENGL_DRAW_TYPE);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, scene->mesh.indices.sizeOfItem * scene->mesh.indices.count, scene->mesh.indices.data, OPENGL_DRAW_TYPE);

    glBufferData(GL_UNIFORM_BUFFER, scene->modelMatrices.sizeOfItem * (2 + scene->modelMatrices.count), scene->modelMatrices.data, OPENGL_DRAW_TYPE);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, scene->modelMatrices.sizeOfItem, &scene->camera->projectionMatrix);                                                         // projection matrix
    glBufferSubData(GL_UNIFORM_BUFFER, scene->modelMatrices.sizeOfItem, scene->modelMatrices.sizeOfItem, &scene->camera->viewMatrix);                                 // view matrix
    glBufferSubData(GL_UNIFORM_BUFFER, scene->modelMatrices.sizeOfItem * 2, scene->modelMatrices.sizeOfItem * scene->modelMatrices.count, scene->modelMatrices.data); // model matrices

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    DebugInfo("Renderer Scene '%s' updated", scene->name.characters);
}

#pragma endregion Renderer Scene

#pragma region Renderer Object

RendererObject RendererObject_Create(String name, RendererScene *scene, RendererMesh mesh)
{
    DebugAssertNullPointerCheck(scene);

    RendererObject object = {0};

    object.name = name;
    object.transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    object.scene = scene;
    object.vertexCountInScene = mesh.vertices.count;
    object.vertexOffsetInScene = object.scene->mesh.vertices.count;
    object.indexCountInScene = mesh.indices.count;
    object.indexOffsetInScene = object.scene->mesh.indices.count;
    object.matrixOffsetInScene = object.scene->modelMatrices.count;

    ListArray_AddRange(&object.scene->mesh.vertices, mesh.vertices.data, mesh.vertices.count);
    ListArray_AddRange(&object.scene->mesh.indices, mesh.indices.data, mesh.indices.count);
    ListArray_Add(&object.scene->modelMatrices, (mat4){0});

    RendererScene_Update(object.scene);

    DebugInfo("Renderer Object created with name: %s", object.name.characters);

    return object;
}

void RendererObject_Destroy(RendererObject *object)
{
    DebugAssertNullPointerCheck(object);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, sizeof(tempTitle), object->name.characters);

    object->transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    String_Destroy(&object->name);

    ListArray_RemoveRange(&object->scene->mesh.vertices, object->vertexOffsetInScene, object->vertexCountInScene);
    ListArray_RemoveRange(&object->scene->mesh.indices, object->indexOffsetInScene, object->indexCountInScene);

    object->scene = NULL;
    object->vertexCountInScene = 0;
    object->vertexOffsetInScene = 0;
    object->indexCountInScene = 0;
    object->indexOffsetInScene = 0;

    object = NULL;

    DebugInfo("Renderer Dynamic Object destroyed with name: %s", tempTitle);
}

void RendererObject_Update(RendererObject *object)
{
    mat4 *matrix = (mat4 *)ListArray_Get(object->scene->modelMatrices, object->matrixOffsetInScene);
    RendererObjectTransform_ToModelMatrix(&object->transform, matrix);

    DebugInfo("Renderer Object '%s' updated", object->name.characters);
}

#pragma endregion Renderer Object
