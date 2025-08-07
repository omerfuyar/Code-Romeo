#include "Global.h"
#include "app/Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// --- Shader Source Code ---
// The Vertex Shader is responsible for processing individual vertices.
// It takes vertex attributes (like position and color) as input.
// Its main job is to output the final position of the vertex in clip space (gl_Position).
static const char *vertexShaderSource =
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
static const char *fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 ourColor;\n" // Input color from vertex shader
    "void main()\n"
    "{\n"
    "   FragColor = vec4(ourColor, 1.0f);\n"
    "}\n\0";

// --- Main Application ---

// Callback function for when the window is resized
void FrameBufferSizeCallback(GLFWwindow *window, int width, int height)
{
    DebugAssertNullPointerCheck(window);
    glViewport(0, 0, width, height);
}

int main()
{
    DebugInfo("Application starting.");

    // 1. Initialize GLFW
    if (!glfwInit())
    {
        DebugError("Failed to initialize GLFW");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create a Window
    GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL test", NULL, NULL);
    if (window == NULL)
    {
        DebugError("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FrameBufferSizeCallback);

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        DebugError("Failed to initialize GLAD");
        return -1;
    }

    // --- Graphics Pipeline Setup ---

    // 4. Compile Shaders
    // Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // (Error checking for compilation is omitted for brevity but is crucial in real projects)

    // Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // 5. Link Shaders into a Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // (Error checking for linking is also omitted for brevity)
    glDeleteShader(vertexShader); // Shaders are now in the program, so we can delete them.
    glDeleteShader(fragmentShader);

    // 6. Set up Vertex Data and Buffers
    // A triangle with position and color data for each vertex.
    float vertices[] = {
        // positions         // colors
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left, red
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom right, green
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f    // top, blue
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind the Vertex Array Object (VAO) first. It will store the VBO and attribute configurations.
    glBindVertexArray(VAO);

    // Bind the Vertex Buffer Object (VBO) and copy our vertex data into it.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 7. Configure Vertex Attributes
    // Tell OpenGL how to interpret the vertex data.
    // Position attribute (aPos in shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // Color attribute (aColor in shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VBO and VAO (optional, but good practice)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Render Loop ---
    while (!glfwWindowShouldClose(window))
    {
        // Clear the screen with a color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 8. Draw the triangle
        glUseProgram(shaderProgram);      // Activate the shader program
        glBindVertexArray(VAO);           // Bind the VAO that contains our triangle's data
        glDrawArrays(GL_TRIANGLES, 0, 3); // Draw the triangle

        // Swap the front and back buffers to display the rendered image
        glfwSwapBuffers(window);
        // Check for and process events like keyboard input or window closing
        glfwPollEvents();
    }

    // 9. Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    DebugInfo("Application terminated successfully.");
    return 0;
}
