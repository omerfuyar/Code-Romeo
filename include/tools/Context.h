#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

/// @brief OpenGL context version major number
#define CONTEXT_VERSION_MAJOR 3
/// @brief OpenGL context version minor number
#define CONTEXT_VERSION_MINOR 3

#pragma region Typedefs

/// @brief Structure representing the main window context
typedef struct ContextWindow
{
    String title;
    Vector2Int size;
    void *handle; // user should not use this variable
    bool vSync;
    bool fullScreen;
} ContextWindow;

/// @brief Function pointer type used for dynamic symbol loading
typedef void *(*Context_VoidptrFunCcharptr)(const char *name);

/// @brief Function pointer type used for window resize callback
typedef void (*Context_VoidFunVoidptrIntInt)(void *, int, int);

/// @brief Function pointer type used for window log callback
typedef void (*Context_VoidFunUintUintUintUintIntCcharptrCvoidptr)(unsigned int, unsigned int, unsigned int, unsigned int, int, const char *, const void *);

#pragma endregion Typedefs

/// @brief Initialize the context system and create the main window
/// @param retContext Pointer to store the address of the main window context structure
/// @return RJ_OK on success, or RJ_ERROR_DEPENDENCY if GLFW fails. Analyze the logs for more information.
RJ_Result Context_Initialize(ContextWindow **retContext);

/// @brief Clean up and terminate the context system
void Context_Terminate(void);

/// @brief Get the status of context module.
/// @return Context module is initialized or not.
bool Context_IsInitialized(void);

/// @brief Updates the window. Should be called before any system module update.
void Context_Update(void);

/// @brief Configure all window properties at once
/// @param title Window title text
/// @param windowSize Window dimensions in pixels
/// @param vSync Enable/disable vertical synchronization
/// @param fullScreen Enable/disable fullscreen mode
/// @param resizeCallback Function to call when window is resized
void Context_Configure(StringView title, Vector2Int windowSize, bool vSync, bool fullScreen, Context_VoidFunVoidptrIntInt resizeCallback);

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
void Context_ConfigureResizeCallback(Context_VoidFunVoidptrIntInt callback);

/// @brief Get the dynamic symbol loader function pointer
/// @return Function pointer to load dynamic symbols by name
Context_VoidptrFunCcharptr Context_GetDynamicSymbolLoader(void);

/// @brief Swap the front and back buffers of the main window
void Context_SwapBuffers(void);