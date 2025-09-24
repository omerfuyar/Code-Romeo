#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/Vectors.h"

#define CONTEXT_OPENGL_VERSION_MAJOR 3
#define CONTEXT_OPENGL_VERSION_MINOR 3

typedef struct ContextWindow
{
    String title;
    Vector2Int size;
    void *handle;
    bool vSync;
    bool fullScreen;
} ContextWindow;

typedef void (*FunVoidptrIntIntToVoid)(void *, int, int);

ContextWindow *Context_Initialize();

void Context_Configure(StringView title, Vector2Int windowSize, bool vSync, bool fullScreen, FunVoidptrIntIntToVoid resizeCallback);

void Context_ConfigureTitle(StringView title);

void Context_ConfigureSize(Vector2Int size);

void Context_ConfigureVSync(bool vSync);

void Context_ConfigureFullScreen(bool fullScreen);

void Context_ConfigureResizeCallback(FunVoidptrIntIntToVoid callback);

void Context_Terminate();
