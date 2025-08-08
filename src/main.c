#include "Global.h"
#include "app/Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// --- Shader Source Code ---
// The Vertex Shader is responsible for processing individual vertices.
// It takes vertex attributes (like position and color) as input.
// Its main job is to output the final position of the vertex in clip space (gl_Position).
static char *vertexShaderSource =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"   // Vertex position attribute
    "layout (location = 1) in vec3 aColor;\n" // Vertex color attribute
    "out vec3 ourColor;\n"                    // Output color to fragment shader
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "}\0";

// The Fragment Shader is responsible for calculating the final color of each pixel (fragment).
// It receives data from the vertex shader (like ourColor).
static char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 ourColor;\n" // Input color from vertex shader
    "void main()\n"
    "{\n"
    "   FragColor = vec4(ourColor, 1.0f);\n"
    "}\n\0";

int main()
{
    Renderer_Initialize(String_CreateCopy("test win"), NewVector2Int(720, 540), String_CreateCopy(vertexShaderSource), String_CreateCopy(fragmentShaderSource));

    Vertex vertices[] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    RendererDynamicObject myObj = RendererDynamicObject_Create(String_CreateCopy("myObj"), vertices, 3);

    while (true)
    {
        Renderer_StartRendering();

        Renderer_RenderDynamicObject(myObj);

        Renderer_FinishRendering();
    }

    DebugInfo("Application terminated successfully.");
    return 0;
}
