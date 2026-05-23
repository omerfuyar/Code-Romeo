#pragma once

#include "RJGlobal.h"

#include "utilities/Vector.h"

#include "tools/Context.h"

/// @brief Key codes for keyboard.
typedef enum InputKeyCode
{
    InputKeyCode_Space = 32,

    InputKeyCode_Num0 = 48,
    InputKeyCode_Num1 = 49,
    InputKeyCode_Num2 = 50,
    InputKeyCode_Num3 = 51,
    InputKeyCode_Num4 = 52,
    InputKeyCode_Num5 = 53,
    InputKeyCode_Num6 = 54,
    InputKeyCode_Num7 = 55,
    InputKeyCode_Num8 = 56,
    InputKeyCode_Num9 = 57,

    InputKeyCode_A = 65,
    InputKeyCode_B = 66,
    InputKeyCode_C = 67,
    InputKeyCode_D = 68,
    InputKeyCode_E = 69,
    InputKeyCode_F = 70,
    InputKeyCode_G = 71,
    InputKeyCode_H = 72,
    InputKeyCode_I = 73,
    InputKeyCode_J = 74,
    InputKeyCode_K = 75,
    InputKeyCode_L = 76,
    InputKeyCode_M = 77,
    InputKeyCode_N = 78,
    InputKeyCode_O = 79,
    InputKeyCode_P = 80,
    InputKeyCode_Q = 81,
    InputKeyCode_R = 82,
    InputKeyCode_S = 83,
    InputKeyCode_T = 84,
    InputKeyCode_U = 85,
    InputKeyCode_V = 86,
    InputKeyCode_W = 87,
    InputKeyCode_X = 88,
    InputKeyCode_Y = 89,
    InputKeyCode_Z = 90,

    InputKeyCode_Escape = 256,
    InputKeyCode_Enter = 257,
    InputKeyCode_Tab = 258,
    InputKeyCode_BackSpace = 259,
    InputKeyCode_Insert = 260,
    InputKeyCode_Delete = 261,
    InputKeyCode_RightArrow = 262,
    InputKeyCode_LeftArrow = 263,
    InputKeyCode_DownArrow = 264,
    InputKeyCode_UpArrow = 265,

    InputKeyCode_F1 = 290,
    InputKeyCode_F2 = 291,
    InputKeyCode_F3 = 292,
    InputKeyCode_F4 = 293,
    InputKeyCode_F5 = 294,
    InputKeyCode_F6 = 295,
    InputKeyCode_F7 = 296,
    InputKeyCode_F8 = 297,
    InputKeyCode_F9 = 298,
    InputKeyCode_F10 = 299,
    InputKeyCode_F11 = 300,
    InputKeyCode_F12 = 301,

    InputKeyCode_LeftShift = 340,
    InputKeyCode_LeftControl = 341,
    InputKeyCode_LeftAlt = 342,
    InputKeyCode_LeftSuper = 343,
    InputKeyCode_RightShift = 344,
    InputKeyCode_RightControl = 345,
    InputKeyCode_RightAlt = 346,
    InputKeyCode_RightSuper = 347
} InputKeyCode;

/// @brief Button codes for mouse.
typedef enum InputMouseButtonCode
{
    InputMouseButtonCode_Left = 0,
    InputMouseButtonCode_Right = 1,
    InputMouseButtonCode_Middle = 2,

    InputMouseButtonCode_F1 = 3,
    InputMouseButtonCode_F2 = 4,
    InputMouseButtonCode_F3 = 5,
    InputMouseButtonCode_F4 = 6,
    InputMouseButtonCode_F5 = 7,
} InputMouseButtonCode;

// todo controller/gamepad support

/// @brief States of a key/button. Can be used with flags in parameters. (eg. InputState_Released | InputState_Up)
typedef enum InputState
{
    InputState_Released = (1 << 0), // The key/button is released
    InputState_Down = (1 << 1),     // The key/button is pressed just in the last frame
    InputState_Pressed = (1 << 2),  // The key/button is pressed
    InputState_Up = (1 << 3)        // The key/button is released just in the last frame
} InputState;

/// @brief Mouse cursor modes for the input system.
typedef enum InputCursorMode
{
    InputCursorMode_Normal = 0x00034001,  // Makes the cursor visible and behaving normally.
    InputCursorMode_Hidden = 0x00034002,  // Makes the cursor invisible when it is over the content area of the window but does not restrict the cursor from leaving.
    InputCursorMode_Captured = 0x00034003 // Hides and grabs the cursor, providing virtual and unlimited cursor movement.
} InputCursorMode;

/// @brief Initialization function for the input system.
void Input_Initialize(void);

/// @brief Termination function for the input system.
void Input_Terminate(void);

/// @brief The input system is initialized or not.
/// @return True if the system is initialized previously and not terminated, false otherwise.
bool Input_IsInitialized(void);

/// @brief Configures the mouse cursor mode for the context window.
/// @param mode Mode to set.
void Input_ConfigureCursorMode(InputCursorMode mode);

/// @brief Updates the state of all input devices (keyboard and mouse) for the current frame
/// todo @note This function should be called once per frame before any input queries before Context update
void Input_Update(void);

/// @brief Checks if a keyboard key is in one of the given state parameter. Parameter can be passed with operator "|" to check more than one state (eg. InputState_Released | InputState_Up).
/// @param key The key code to check.
/// @param state The input state to compare. (Down, Pressed, Up, Released)
/// @return True if the key is in the specified state, false otherwise.
bool Input_GetKey(InputKeyCode key, InputState state);

/// @brief Checks if a mouse button is in one of the given state parameter. Parameter can be passed with operator "|" to check more than one state (eg. InputState_Released | InputState_Up).
/// @param button The mouse button code to check.
/// @param state The input state to compare. (Down, Pressed, Up, Released)
/// @return True if the mouse button is in the specified state, false otherwise.
bool Input_GetMouseButton(InputMouseButtonCode button, InputState state);

/// @brief Gets the current state of a keyboard key
/// @param key The key code to check
/// @return The current state of the key (Down, Pressed, Up, Released)
InputState Input_GetKeyState(InputKeyCode key);

/// @brief Gets the current state of a mouse button
/// @param button The mouse button code to check
/// @return The current state of the mouse button (Down, Pressed, Up, Released)
InputState Input_GetMouseButtonState(InputMouseButtonCode button);

/// @brief Gets the current mouse wheel scroll delta, how much the mouse scroll scrolled last frame.
/// @return A float value representing the scroll amount, positive for scrolling up and negative for scrolling down.
float Input_GetMouseScroll(void);

/// @brief Gets the current mouse cursor position in screen coordinates.
/// @return Coordinates of the mouse cursor. Relative to the top-left corner of the window.
Vector2Int Input_GetMousePosition(void);

/// @brief Gets the change in mouse position since the last frame.
/// @return Delta of the mouse movement. Relative to the top-left corner of the window.
Vector2Int Input_GetMousePositionDelta(void);

/// @brief Gets the movement vector of the user. X axis is D-A / RightArrow-LeftArrow, Y axis is W-S / UpArrow-DownArrow, Z axis is Space-(LeftControl/RightControl).
/// @return The built vector from user input. Final vector is normalized;
Vector3 Input_GetMovementVector(void);
