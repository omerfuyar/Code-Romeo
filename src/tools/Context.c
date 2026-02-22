#include "tools/Context.h"

#include "GLFW/glfw3.h"

#pragma region Source Only

ContextWindow *CONTEXT_MAIN_WINDOW_REF = NULL;

static void CONTEXT_ERROR_CALLBACK(int error, const char *description)
{
    RJ_DebugError(error, "Context get error code '%d' : \n'%s'", error, description);
}

#pragma endregion Source Only

RJ_ResultWarn Context_Initialize(ContextWindow *retContext)
{
    RJ_DebugAssertNullPointerCheck(retContext);

    glfwSetErrorCallback(CONTEXT_ERROR_CALLBACK);

    if (!glfwInit())
    {
        RJ_DebugWarning("Failed to initialize GLFW.");
        return RJ_ERROR_DEPENDENCY;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, CONTEXT_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, CONTEXT_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    retContext->handle = glfwCreateWindow(1080, 720, "Romeo Window", NULL, NULL);
    const char *errorLog = NULL;

    if (retContext->handle == NULL)
    {
        RJ_DebugWarning("Failed to create GLFW window (%d):\n%s", glfwGetError(&errorLog), errorLog);
        return RJ_ERROR_DEPENDENCY;
    }

    glfwMakeContextCurrent(retContext->handle);

    CONTEXT_MAIN_WINDOW_REF = retContext;
    Context_Configure(scv(CONTEXT_MAIN_WINDOW_REF->title),
                      CONTEXT_MAIN_WINDOW_REF->size,
                      CONTEXT_MAIN_WINDOW_REF->vSync,
                      CONTEXT_MAIN_WINDOW_REF->fullScreen,
                      CONTEXT_MAIN_WINDOW_REF->resizeCallback);

    RJ_DebugInfo("Main window created successfully.");

    return RJ_OK;
}

void Context_Terminate(void)
{
    glfwDestroyWindow(CONTEXT_MAIN_WINDOW_REF->handle);
    glfwTerminate();

    CONTEXT_MAIN_WINDOW_REF = NULL;
    RJ_DebugInfo("Context terminated successfully.");
}

bool Context_IsInitialized(void)
{
    return CONTEXT_MAIN_WINDOW_REF != NULL;
}

bool Context_Update(void)
{
    glfwPollEvents();

    return !glfwWindowShouldClose(CONTEXT_MAIN_WINDOW_REF->handle);
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
    String_Change(&CONTEXT_MAIN_WINDOW_REF->title, title);

    glfwSetWindowTitle(CONTEXT_MAIN_WINDOW_REF->handle, CONTEXT_MAIN_WINDOW_REF->title.characters);
}

void Context_ConfigureSize(Vector2Int size)
{
    CONTEXT_MAIN_WINDOW_REF->size = size;

    glfwSetWindowSize(CONTEXT_MAIN_WINDOW_REF->handle, CONTEXT_MAIN_WINDOW_REF->size.x, CONTEXT_MAIN_WINDOW_REF->size.y);
}

void Context_ConfigureVSync(bool vSync)
{
    CONTEXT_MAIN_WINDOW_REF->vSync = vSync;

    glfwSwapInterval(CONTEXT_MAIN_WINDOW_REF->vSync);
}

void Context_ConfigureFullScreen(bool fullScreen)
{
    CONTEXT_MAIN_WINDOW_REF->fullScreen = fullScreen;

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (fullScreen)
    {
        glfwSetWindowMonitor(CONTEXT_MAIN_WINDOW_REF->handle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(CONTEXT_MAIN_WINDOW_REF->handle, NULL, 100, 100, CONTEXT_MAIN_WINDOW_REF->size.x, CONTEXT_MAIN_WINDOW_REF->size.y, 0);
    }
}

void Context_ConfigureResizeCallback(Context_VoidFunVoidptrIntInt resizeCallback)
{
    CONTEXT_MAIN_WINDOW_REF->resizeCallback = resizeCallback;

    glfwSetFramebufferSizeCallback(CONTEXT_MAIN_WINDOW_REF->handle, (GLFWframebuffersizefun)resizeCallback);
}

Context_VoidptrFunCcharptr Context_GetDynamicSymbolLoader(void)
{
    return (Context_VoidptrFunCcharptr)glfwGetProcAddress;
}

void Context_SwapBuffers(void)
{
    glfwSwapBuffers(CONTEXT_MAIN_WINDOW_REF->handle);
}
