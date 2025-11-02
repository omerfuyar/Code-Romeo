#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/Vector.h"

#define CONTEXT_VERSION_MAJOR 3
#define CONTEXT_VERSION_MINOR 3

#pragma region Typedefs

typedef struct ContextWindow
{
    String title;
    Vector2Int size;
    void *handle;
    bool vSync;
    bool fullScreen;
} ContextWindow;

typedef void (*VoidFunVoidptrIntInt)(void *, int, int);

typedef void (*VoidFunUintUintUintUintIntCcharptrCvoidptr)(unsigned int, unsigned int, unsigned int, unsigned int, int, const char *, const void *);

#pragma endregion Typedefs

/// @brief Initialize the context system and create the main window
/// @return Pointer to the main window context structure
ContextWindow *Context_Initialize();

/// @brief Configure all window properties at once
/// @param title Window title text
/// @param windowSize Window dimensions in pixels
/// @param vSync Enable/disable vertical synchronization
/// @param fullScreen Enable/disable fullscreen mode
/// @param resizeCallback Function to call when window is resized
void Context_Configure(StringView title, Vector2Int windowSize, bool vSync, bool fullScreen, VoidFunVoidptrIntInt resizeCallback);

/// @brief Set the window title
/// @param title New window title text
void Context_ConfigureTitle(StringView title);

/// @brief Set the window size and trigger resize callback
/// @param size New window dimensions in pixels
void Context_ConfigureSize(Vector2Int size);

/// @brief Enable or disable vertical synchronization
/// @param vSync True to enable VSync, false to disable
void Context_ConfigureVSync(bool vSync);

/// @brief Switch between windowed and fullscreen mode
/// @param fullScreen True for fullscreen, false for windowed
void Context_ConfigureFullScreen(bool fullScreen);

/// @brief Set the window resize callback function
/// @param callback Function to call when window is resized, or NULL to remove callback
void Context_ConfigureResizeCallback(VoidFunVoidptrIntInt callback);

/// @brief Set the window log callback function
/// @param callback Function to call when window log events occur, or NULL to remove callback
void Context_ConfigureLogCallback(VoidFunUintUintUintUintIntCcharptrCvoidptr callback);

/// @brief Clean up and terminate the context system
void Context_Terminate();
