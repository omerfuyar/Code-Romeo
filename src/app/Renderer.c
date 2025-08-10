#include "app/Renderer.h"
#include "app/Resources.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define OPENGL_DRAW_TYPE GL_STATIC_DRAW

GLFWwindow *MAIN_WINDOW = NULL;

RendererShaderProgramHandle MAIN_SHADER_PROGRAM = 0;

void MAIN_WINDOW_RESIZE_CALLBACK(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    glViewport(0, 0, width, height);
}

#pragma region Renderer Object Transform

void ObjectTransform_SetPosition(RendererObjectTransform *transform, Vector3 position)
{
    DebugAssertNullPointerCheck(transform);
    transform->position = position;
}

void ObjectTransform_SetRotation(RendererObjectTransform *transform, Vector3 rotation)
{
    DebugAssertNullPointerCheck(transform);
    transform->rotation = rotation;
}

void ObjectTransform_SetScale(RendererObjectTransform *transform, Vector3 scale)
{
    DebugAssertNullPointerCheck(transform);
    transform->scale = scale;
}

#pragma endregion Renderer Object Transform

#pragma region Renderer Mesh

RendererMesh RendererMesh_Create(String objFileSource)
{
    RendererMesh mesh;

    size_t lineCount = 0;
    String lines[RESOURCE_FILE_MAX_LINE_COUNT];
    String_Tokenize(objFileSource, scl("\n"), &lineCount, lines, sizeof(lines));

    mesh.vertices = ListArray_Create(sizeof(RendererMeshVertex), lineCount / 4);
    mesh.indices = ListArray_Create(sizeof(RendererMeshIndex), lineCount / 4);

    for (size_t i = 0; i < lineCount; i++)
    {
        size_t tokenCount = 0;
        String tokens[RESOURCE_FILE_LINE_MAX_TOKEN_COUNT];
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

void RendererMesh_Destroy(RendererMesh *mesh)
{
    DebugAssertNullPointerCheck(mesh);

    ListArray_Destroy(&mesh->vertices);
    ListArray_Destroy(&mesh->indices);
}

#pragma endregion Renderer Mesh

#pragma region Renderer Dynamic Object

RendererDynamicObject RendererDynamicObject_Create(String name, RendererMesh mesh)
{
    RendererDynamicObject object;

    object.name = name;
    object.transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    object.mesh = mesh;

    glGenVertexArrays(1, &object.vao);
    glGenBuffers(1, &object.vbo);
    glGenBuffers(1, &object.ibo);

    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ibo);

    size_t offset = 0;

    glVertexAttribPointer(VERTEX_MEMBER_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_POSITION_INDEX);
    offset += sizeof(Vector3);

    glVertexAttribPointer(VERTEX_MEMBER_COLOR_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_COLOR_INDEX);
    offset += sizeof(Vector4);

    //! ... other attributes in vertex

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RendererDynamicObject_Update(object);

    return object;
}

void RendererDynamicObject_Destroy(RendererDynamicObject *object)
{
    DebugAssertNullPointerCheck(object);

    object->transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    String_Destroy(&object->name);

    glDeleteBuffers(1, &object->vbo);
    glDeleteBuffers(1, &object->ibo);
    glDeleteVertexArrays(1, &object->vao);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RendererMesh_Destroy(&object->mesh);

    object = NULL;
}

void RendererDynamicObject_Update(RendererDynamicObject object)
{
    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ibo);

    glBufferData(GL_ARRAY_BUFFER, object.mesh.vertices.sizeOfItem * object.mesh.vertices.count, object.mesh.vertices.data, OPENGL_DRAW_TYPE);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.mesh.indices.sizeOfItem * object.mesh.indices.count, object.mesh.indices.data, OPENGL_DRAW_TYPE);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

#pragma endregion Renderer Dynamic Object

#pragma region Renderer Batch

RendererBatch RendererBatch_Create(String name, size_t initialVertexCapacity, size_t initialIndexCapacity)
{
    RendererBatch batch;

    batch.name = name;
    batch.mesh.vertices = ListArray_Create(sizeof(RendererMeshVertex), initialVertexCapacity);
    batch.mesh.indices = ListArray_Create(sizeof(RendererMeshIndex), initialIndexCapacity);

    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glGenBuffers(1, &batch.ibo);

    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ibo);

    size_t offset = 0;

    glVertexAttribPointer(VERTEX_MEMBER_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_POSITION_INDEX);
    offset += sizeof(Vector3);

    glVertexAttribPointer(VERTEX_MEMBER_COLOR_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(RendererMeshVertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_COLOR_INDEX);
    offset += sizeof(Vector4);

    //! ... other attributes in vertex

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RendererBatch_Update(batch);

    return batch;
}

void RendererBatch_Destroy(RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);

    String_Destroy(&batch->name);

    glDeleteVertexArrays(1, &batch->vao);
    glDeleteBuffers(1, &batch->vbo);
    glDeleteBuffers(1, &batch->ibo);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RendererMesh_Destroy(&batch->mesh);

    batch = NULL;
}

void RendererBatch_Update(RendererBatch batch)
{
    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ibo);

    glBufferData(GL_ARRAY_BUFFER, batch.mesh.vertices.sizeOfItem * batch.mesh.vertices.count, batch.mesh.vertices.data, OPENGL_DRAW_TYPE);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.mesh.indices.sizeOfItem * batch.mesh.indices.count, batch.mesh.indices.data, OPENGL_DRAW_TYPE);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

#pragma endregion Renderer Batch

#pragma region Renderer Static Object

RendererStaticObject RendererStaticObject_Create(String name, RendererBatch *batch, RendererMesh mesh)
{
    DebugAssertNullPointerCheck(batch);

    RendererStaticObject object;

    object.name = name;
    object.transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    object.batch = batch;
    object.vertexCountInBatch = mesh.vertices.count;
    object.vertexOffsetInBatch = object.batch->mesh.vertices.count;
    object.indexCountInBatch = mesh.indices.count;
    object.indexOffsetInBatch = object.batch->mesh.indices.count;

    ListArray_AddRange(&object.batch->mesh.vertices, mesh.vertices.data, mesh.vertices.count);
    ListArray_AddRange(&object.batch->mesh.indices, mesh.indices.data, mesh.indices.count);

    return object;
}

void RendererStaticObject_Destroy(RendererStaticObject *object)
{
    DebugAssertNullPointerCheck(object);

    object->transform = (RendererObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    String_Destroy(&object->name);

    ListArray_RemoveRange(&object->batch->mesh.vertices, object->vertexOffsetInBatch, object->vertexCountInBatch);
    ListArray_RemoveRange(&object->batch->mesh.indices, object->indexOffsetInBatch, object->indexCountInBatch);

    object->batch = NULL;
    object->vertexCountInBatch = 0;
    object->vertexOffsetInBatch = 0;
    object->indexCountInBatch = 0;
    object->indexOffsetInBatch = 0;

    object = NULL;
}

#pragma endregion Renderer Static Object

#pragma region Renderer

void Renderer_Initialize(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource)
{
    DebugAssert(glfwInit(), "Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DebugInfo("GLFW initialized successfully.");

    MAIN_WINDOW = glfwCreateWindow(windowSize.x, windowSize.y, title.characters, NULL, NULL);
    DebugAssertNullPointerCheck(MAIN_WINDOW);

    glfwMakeContextCurrent(MAIN_WINDOW);
    glfwSetFramebufferSizeCallback(MAIN_WINDOW, MAIN_WINDOW_RESIZE_CALLBACK);
    DebugInfo("Main window created successfully.");

    DebugAssert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");
    DebugInfo("GLAD initialized successfully.");

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint glslHasCompiled;
    char glslInfoLog[1024];

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

    MAIN_SHADER_PROGRAM = glCreateProgram();
    glAttachShader(MAIN_SHADER_PROGRAM, vertexShader);
    glAttachShader(MAIN_SHADER_PROGRAM, fragmentShader);
    glLinkProgram(MAIN_SHADER_PROGRAM);

    glGetProgramiv(MAIN_SHADER_PROGRAM, GL_LINK_STATUS, &glslHasCompiled);
    if (glslHasCompiled == GL_FALSE)
    {
        glGetProgramInfoLog(MAIN_SHADER_PROGRAM, 1024, NULL, glslInfoLog);
        DebugError("Shader program linking failed: %s", glslInfoLog);
    }
    DebugInfo("Shader program linked successfully.");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    DebugInfo("Renderer initialized successfully.");
}

void Renderer_Terminate()
{
    glDeleteProgram(MAIN_SHADER_PROGRAM);
    glfwDestroyWindow(MAIN_WINDOW);
    glfwTerminate();
}

void Renderer_StartRendering()
{
    if (glfwWindowShouldClose(MAIN_WINDOW))
    {
        DebugInfo("Main window close input received");
        Terminate(0);
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(MAIN_SHADER_PROGRAM);

    // DebugInfo("Rendering started");
}

void Renderer_FinishRendering()
{
    glfwSwapBuffers(MAIN_WINDOW);
    glfwPollEvents();

    // DebugInfo("Rendering finished");
}

void Renderer_RenderDynamicObject(RendererDynamicObject object)
{
    glBindVertexArray(object.vao);

    glDrawElements(GL_TRIANGLES, object.mesh.indices.count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    // DebugInfo("Dynamic object %s rendered", object.name.characters);
}

void Renderer_RenderBatch(RendererBatch batch)
{
    glBindVertexArray(batch.vao);

    glDrawElements(GL_TRIANGLES, batch.mesh.indices.count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    // DebugInfo("Batch %s rendered", batch.name.characters);
}

#pragma endregion Renderer
