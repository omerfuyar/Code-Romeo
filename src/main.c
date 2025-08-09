#include "Global.h"
#include "app/Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

static char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec4 aColor;\n"
    "out vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "}\0";

static char *fragmentShaderSource =
    "#version 330 core\n"
    "in vec4 ourColor;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = ourColor;\n"
    "}\n\0";

int main()
{
    Renderer_Initialize(String_CreateCopy("test win"), NewVector2Int(720, 540), String_CreateCopy(vertexShaderSource), String_CreateCopy(fragmentShaderSource));

    RendererMeshVertex verticesData[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    RendererMeshIndex indicesData[] = {0, 1, 2, 2, 3, 0};

    ListArray vertices = ListArray_Create(sizeof(RendererMeshVertex), 4);
    ListArray indices = ListArray_Create(sizeof(RendererMeshIndex), 6);

    ListArray_AddRange(&vertices, verticesData, 4);
    ListArray_AddRange(&indices, indicesData, 6);

    RendererMesh mesh = {vertices, indices};

    RendererDynamicObject myObj = RendererDynamicObject_Create(String_CreateCopy("myObj"), mesh);

    while (true)
    {
        Renderer_StartRendering();

        Renderer_RenderDynamicObject(myObj);

        Renderer_FinishRendering();
    }

    return 0;
}
