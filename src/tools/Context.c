#include "tools/Context.h"

#include "GLFW/glfw3.h"

#pragma region Source Only

bool CONTEXT_INITIALIZED = false;
ContextWindow CONTEXT_MAIN_WINDOW = {0};
Context_VoidFunVoidptrIntInt CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK = NULL;
Context_VoidFunUintUintUintUintIntCcharptrCvoidptr CONTEXT_MAIN_WINDOW_LOG_CALLBACK = NULL;

static void CONTEXT_ERROR_CALLBACK(int error, const char *description)
{
    RJ_DebugError("Context get error code '%d' : \n'%s'", error, description);
}

#pragma endregion Source Only

RJ_Result Context_Initialize(ContextWindow **retContext)
{
    glfwSetErrorCallback(CONTEXT_ERROR_CALLBACK);

    if (!glfwInit())
    {
        RJ_DebugWarning("Failed to initialize GLFW.");
        return RJ_ERROR_DEPENDENCY;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    CONTEXT_MAIN_WINDOW.handle = glfwCreateWindow(1080, 720, "", NULL, NULL);
    const char *errorLog = NULL;

    if (CONTEXT_MAIN_WINDOW.handle == NULL)
    {
        RJ_DebugWarning("Failed to create GLFW window (%d):\n%s", glfwGetError(&errorLog), errorLog);
        return RJ_ERROR_DEPENDENCY;
    }

    glfwMakeContextCurrent(CONTEXT_MAIN_WINDOW.handle);

    CONTEXT_INITIALIZED = true;
    RJ_DebugInfo("Main window created successfully.");

    *retContext = &CONTEXT_MAIN_WINDOW;
    return RJ_OK;
}

void Context_Terminate(void)
{
    glfwDestroyWindow(CONTEXT_MAIN_WINDOW.handle);
    glfwTerminate();

    CONTEXT_INITIALIZED = false;
    RJ_DebugInfo("Context terminated successfully.");
}

bool Context_IsInitialized(void)
{
    return CONTEXT_INITIALIZED;
}

void Context_Update(void)
{
    glfwPollEvents();

    if (glfwWindowShouldClose(CONTEXT_MAIN_WINDOW.handle))
    {
        RJ_DebugInfo("Main window close input received");
        RJ_Terminate(EXIT_SUCCESS, "Main window close input received");
    }
}

void Context_Configure(StringView title, Vector2Int windowSize, bool vSync, bool fullScreen, Context_VoidFunVoidptrIntInt resizeCallback)
{
    Context_ConfigureTitle(title);
    Context_ConfigureResizeCallback(resizeCallback);
    Context_ConfigureSize(windowSize);
    Context_ConfigureFullScreen(fullScreen);
    Context_ConfigureVSync(vSync);
}

void Context_ConfigureTitle(StringView title)
{
    String_Change(&CONTEXT_MAIN_WINDOW.title, title);

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
        RJ_DebugWarning("The context resize callback function is NULL. Skipped without calling");
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

void Context_ConfigureResizeCallback(Context_VoidFunVoidptrIntInt callback)
{
    CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK = callback;

    glfwSetFramebufferSizeCallback(CONTEXT_MAIN_WINDOW.handle, (GLFWframebuffersizefun)CONTEXT_MAIN_WINDOW_RESIZE_CALLBACK);

    // Context_ConfigureSize(CONTEXT_MAIN_WINDOW.size);
}
