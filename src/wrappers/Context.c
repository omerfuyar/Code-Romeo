#include "wrappers/Context.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

ContextWindow CONTEXT_MAIN_WINDOW = {0};
FunVoidptrIntIntToVoid CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK = NULL;

ContextWindow *Context_Initialize()
{
    DebugAssert(glfwInit(), "Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, CONTEXT_OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, CONTEXT_OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DebugInfo("ContextManager initialized successfully.");

    CONTEXT_MAIN_WINDOW.handle = glfwCreateWindow(1080, 720, "", NULL, NULL);
    DebugAssertNullPointerCheck(CONTEXT_MAIN_WINDOW.handle);

    glfwMakeContextCurrent(CONTEXT_MAIN_WINDOW.handle);

    DebugInfo("Main window created successfully.");

    return &CONTEXT_MAIN_WINDOW;
}

void Context_Configure(String title, Vector2Int windowSize, bool vSync, bool fullScreen, FunVoidptrIntIntToVoid resizeCallback)
{
    Context_ConfigureTitle(title);
    Context_ConfigureResizeCallback(resizeCallback);
    Context_ConfigureSize(windowSize);
    Context_ConfigureVSync(vSync);
    Context_ConfigureFullScreen(fullScreen);
}

void Context_ConfigureTitle(String title)
{
    String_Destroy(&CONTEXT_MAIN_WINDOW.title);
    CONTEXT_MAIN_WINDOW.title = scc(title);

    glfwSetWindowTitle(CONTEXT_MAIN_WINDOW.handle, CONTEXT_MAIN_WINDOW.title.characters);
}
void Context_ConfigureSize(Vector2Int size)
{
    CONTEXT_MAIN_WINDOW.size = size;

    if (CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK != NULL)
    {
        CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK(CONTEXT_MAIN_WINDOW.handle, CONTEXT_MAIN_WINDOW.size.x, CONTEXT_MAIN_WINDOW.size.y);
    }
    else
    {
        DebugWarning("The context resize callback function is NULL. Skipped without calling");
    }
}

void Context_ConfigureVSync(bool vSync)
{
    CONTEXT_MAIN_WINDOW.vSync = vSync;

    glfwSwapInterval(CONTEXT_MAIN_WINDOW.vSync);
}

void Context_ConfigureFullScreen(bool fullScreen)
{
    CONTEXT_MAIN_WINDOW.fullScreen = fullScreen;

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (fullScreen)
    {
        glfwSetWindowMonitor(CONTEXT_MAIN_WINDOW.handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(CONTEXT_MAIN_WINDOW.handle, NULL, 100, 100, CONTEXT_MAIN_WINDOW.size.x, CONTEXT_MAIN_WINDOW.size.y, 0);
    }
}

void Context_ConfigureResizeCallback(FunVoidptrIntIntToVoid callback)
{
    CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK = callback;

    glfwSetFramebufferSizeCallback(CONTEXT_MAIN_WINDOW.handle, (GLFWframebuffersizefun)CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK);

    Context_ConfigureSize(CONTEXT_MAIN_WINDOW.size);
}

void Context_Terminate()
{
    glfwDestroyWindow(CONTEXT_MAIN_WINDOW.handle);
    glfwTerminate();
}
