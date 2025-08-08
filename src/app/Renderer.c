#include "app/Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

GLFWwindow *MAIN_WINDOW = NULL;

RendererShaderProgramHandle MAIN_SHADER_PROGRAM = 0;

void MAIN_WINDOW_RESIZE_CALLBACK(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    glViewport(0, 0, width, height);
}

#pragma region Object Transform

void ObjectTransform_SetPosition(ObjectTransform *transform, Vector3 position)
{
    DebugAssertNullPointerCheck(transform);
    transform->position = position;
}

void ObjectTransform_SetRotation(ObjectTransform *transform, Vector3 rotation)
{
    DebugAssertNullPointerCheck(transform);
    transform->rotation = rotation;
}

void ObjectTransform_SetScale(ObjectTransform *transform, Vector3 scale)
{
    DebugAssertNullPointerCheck(transform);
    transform->scale = scale;
}

#pragma endregion Object Transform

#pragma region Renderer Dynamic Object

RendererDynamicObject RendererDynamicObject_Create(String name, Vertex *vertices, size_t vertexCount)
{
    DebugAssertNullPointerCheck(vertices);
    RendererDynamicObject object;

    object.name = name;
    object.transform = (ObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    object.vertices = ListArray_Create(sizeof(Vertex), vertexCount);
    ListArray_AddRange(&object.vertices, &vertices, vertexCount);

    glGenVertexArrays(1, &object.vao);
    glGenBuffers(1, &object.vbo);

    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);

    size_t offset = 0;

    glVertexAttribPointer(VERTEX_MEMBER_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_POSITION_INDEX);
    offset += sizeof(Vector3);

    glVertexAttribPointer(VERTEX_MEMBER_COLOR_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offset);
    glEnableVertexAttribArray(VERTEX_MEMBER_COLOR_INDEX);
    offset += sizeof(Vector4);

    //! ... other attributes in vertex

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return object;
}

void RendererDynamicObject_Destroy(RendererDynamicObject *object)
{
    DebugAssertNullPointerCheck(object);

    String_Destroy(&object->name);

    glDeleteBuffers(1, &object->vbo);
    glDeleteVertexArrays(1, &object->vao);

    ListArray_Destroy(&object->vertices);
}

void RendererDynamicObject_Update(RendererDynamicObject object)
{
    glBindVertexArray(object.vao);
    glBindBuffer(GL_ARRAY_BUFFER, object.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(object.vertices), &object.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

#pragma endregion Renderer Dynamic Object

#pragma region Renderer Batch

RendererBatch RendererBatch_Create(String name, size_t initialVertexCapacity)
{
    RendererBatch batch;

    batch.name = name;
    batch.vertices = ListArray_Create(sizeof(Vertex), initialVertexCapacity);

    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);

    return batch;
}

void RendererBatch_Destroy(RendererBatch *batch)
{
    DebugAssertNullPointerCheck(batch);

    String_Destroy(&batch->name);

    glDeleteVertexArrays(1, &batch->vao);
    glDeleteBuffers(1, &batch->vbo);

    ListArray_Destroy(&batch->vertices);
}

void RendererBatch_Update(RendererBatch batch)
{
    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(batch.vertices), &batch.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

#pragma endregion Renderer Batch

#pragma region Renderer Static Object

RendererStaticObject RendererStaticObject_Create(String name, RendererBatch *batch, Vertex *vertices, size_t vertexCount)
{
    DebugAssertNullPointerCheck(batch);
    DebugAssertNullPointerCheck(vertices);

    RendererStaticObject object;

    object.name = name;
    object.transform = (ObjectTransform){NewVector3(0, 0, 0), NewVector3(0, 0, 0), NewVector3(1, 1, 1)};

    object.batch = batch;
    object.vertexCountInBatch = vertexCount;
    object.vertexOffsetInBatch = object.batch->vertices.count;
    ListArray_AddRange(&object.batch->vertices, vertices, vertexCount);

    return object;
}

void RendererStaticObject_Destroy(RendererStaticObject *object)
{
    DebugAssertNullPointerCheck(object);

    String_Destroy(&object->name);

    ListArray_RemoveRange(&object->batch->vertices, object->vertexOffsetInBatch, object->vertexCountInBatch);

    object->vertexCountInBatch = 0;
    object->vertexOffsetInBatch = 0;
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
    glShaderSource(vertexShader, 1, (const GLchar *const *)&vertexShaderSource.characters, NULL);
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
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(MAIN_SHADER_PROGRAM);

    DebugInfo("Rendering started");
}

void Renderer_FinishRendering()
{
    glfwSwapBuffers(MAIN_WINDOW);
    glfwPollEvents();

    DebugInfo("Rendering finished");
}

void Renderer_RenderDynamicObject(RendererDynamicObject object)
{
    glBindVertexArray(object.vao);
    glDrawArrays(GL_TRIANGLES, 0, object.vertices.count);

    DebugInfo("Dynamic object %s rendered", object.name.characters);
}

void Renderer_RenderBatch(RendererBatch batch)
{
    glBindVertexArray(batch.vao);
    glDrawArrays(GL_TRIANGLES, 0, batch.vertices.count);
}

#pragma endregion Renderer
