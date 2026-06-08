#include "tools/Context.h"

#include "GLFW/glfw3.h"

#pragma region Source Only

ContextWindow CONTEXT = {0};

static void CONTEXT_ERROR_CALLBACK(int error, const char *description)
{
    RJ_DebugError(error, "Context get error code '%d' : \n'%s'", error, description);
}

#pragma endregion Source Only

RJ_ResultWarn Context_Initialize(void)
{
    glfwSetErrorCallback(CONTEXT_ERROR_CALLBACK);

    if (!glfwInit())
    {
        RJ_DebugWarning("Failed to initialize GLFW.");
        return RJ_ERROR_DEPENDENCY;
    }

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    CONTEXT = ContextWindow_Default;
    CONTEXT.title = scc(ContextWindow_Default.title);

    CONTEXT.handle = glfwCreateWindow(CONTEXT.size.x,
                                      CONTEXT.size.y,
                                      CONTEXT.title.characters,
                                      NULL, NULL);
    const char *errorLog = NULL;

    if (CONTEXT.handle == NULL)
    {
        RJ_DebugWarning("Failed to create GLFW window (%d):\n%s", glfwGetError(&errorLog), errorLog);
        return RJ_ERROR_DEPENDENCY;
    }

    glfwMakeContextCurrent(CONTEXT.handle);

    Context_Configure(scv(CONTEXT.title),
                      CONTEXT.size,
                      CONTEXT.vSync,
                      CONTEXT.fullScreen,
                      CONTEXT.resizeCallback);

    RJ_DebugInfo("Main window created successfully.");

    return RJ_OK;
}

void Context_Terminate(void)
{
    glfwDestroyWindow(CONTEXT.handle);
    glfwTerminate();
    String_Destroy(&CONTEXT.title);

    memset(&CONTEXT, 0, sizeof(CONTEXT));
    RJ_DebugInfo("Context terminated successfully.");
}

bool Context_IsInitialized(void)
{
    return CONTEXT.handle != NULL;
}

const ContextWindow *Context_GetInternalData(void)
{
    return &CONTEXT;
}

bool Context_Update(void)
{
    glfwPollEvents();

    return !glfwWindowShouldClose(CONTEXT.handle);
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
    String_Change(&CONTEXT.title, title);

    glfwSetWindowTitle(CONTEXT.handle, CONTEXT.title.characters);
}

void Context_ConfigureSize(Vector2Int size)
{
    CONTEXT.size = size;

    glfwSetWindowSize(CONTEXT.handle, CONTEXT.size.x, CONTEXT.size.y);
}

void Context_ConfigureVSync(bool vSync)
{
    CONTEXT.vSync = vSync;

    glfwSwapInterval(CONTEXT.vSync);
}

void Context_ConfigureFullScreen(bool fullScreen)
{
    CONTEXT.fullScreen = fullScreen;

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (fullScreen)
    {
        glfwSetWindowMonitor(CONTEXT.handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(CONTEXT.handle, NULL, 100, 100, CONTEXT.size.x, CONTEXT.size.y, 0);
    }
}

void Context_ConfigureResizeCallback(Context_VoidFunVoidptrIntInt resizeCallback)
{
    CONTEXT.resizeCallback = resizeCallback;

    glfwSetFramebufferSizeCallback(CONTEXT.handle, (GLFWframebuffersizefun)resizeCallback);
}

Context_VoidptrFunCcharptr Context_GetDynamicSymbolLoader(void)
{
    return (Context_VoidptrFunCcharptr)glfwGetProcAddress;
}

void Context_SwapBuffers(void)
{
    glfwSwapBuffers(CONTEXT.handle);
}
