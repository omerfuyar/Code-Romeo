#include "systems/Input.h"
#include "GLFW/glfw3.h"

#pragma region Source Only

//! BE CAREFUL
#define INPUT_KEY_NUMBERS_COUNT 10
#define INPUT_KEY_ALPHABETS_COUNT 26
#define INPUT_KEY_SPECIALS_COUNT 10
#define INPUT_KEY_FUNCTIONS_COUNT 12
#define INPUT_KEY_CONTROLS_COUNT 8
#define INPUT_KEY_MOUSE_BUTTONS_COUNT 8

InputState INPUT_KEY_SPACE = {0};
InputState INPUT_KEY_NUMBERS[INPUT_KEY_NUMBERS_COUNT] = {0};
InputState INPUT_KEY_ALPHABETS[INPUT_KEY_ALPHABETS_COUNT] = {0};
InputState INPUT_KEY_SPECIALS[INPUT_KEY_SPECIALS_COUNT] = {0};
InputState INPUT_KEY_FUNCTIONS[INPUT_KEY_FUNCTIONS_COUNT] = {0};
InputState INPUT_KEY_CONTROLS[INPUT_KEY_CONTROLS_COUNT] = {0};
InputState INPUT_KEY_MOUSE_BUTTONS[INPUT_KEY_MOUSE_BUTTONS_COUNT] = {0};

ContextWindow *INPUT_MAIN_WINDOW = NULL;
float INPUT_MOUSE_SCROLL = 0.0f;
Vector2Int INPUT_MOUSE_POSITION = {0};
Vector2Int INPUT_PREVIOUS_MOUSE_POSITION = {0};

/// @brief
/// @param window
/// @param key
/// @param scanCode
/// @param action
/// @param mods
void INPUT_KEY_CALLBACK(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
    (void)window;
    (void)scanCode;
    (void)mods;

    // RJGlobal_DebugInfo("Key %d action %s", key, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : action == GLFW_REPEAT ? "repeated" : "unknown");

    if (key >= InputKeyCode_Num0 && key <= InputKeyCode_Num9)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_NUMBERS[key - InputKeyCode_Num0] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_NUMBERS[key - InputKeyCode_Num0] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else if (key >= InputKeyCode_A && key <= InputKeyCode_Z)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_ALPHABETS[key - InputKeyCode_A] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_ALPHABETS[key - InputKeyCode_A] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else if (key >= InputKeyCode_Escape && key <= InputKeyCode_UpArrow)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_SPECIALS[key - InputKeyCode_Escape] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_SPECIALS[key - InputKeyCode_Escape] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else if (key >= InputKeyCode_F1 && key <= InputKeyCode_F12)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_FUNCTIONS[key - InputKeyCode_F1] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_FUNCTIONS[key - InputKeyCode_F1] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else if (key >= InputKeyCode_LeftShift && key <= InputKeyCode_RightSuper)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_CONTROLS[key - InputKeyCode_LeftShift] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_CONTROLS[key - InputKeyCode_LeftShift] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else if (key == InputKeyCode_Space)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_SPACE = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_SPACE = InputState_Up;
            break;
        default:
            break;
        }
    }
    else
    {
        RJGlobal_DebugWarning("Unhandled key input: %d", key);
    }
}

/// @brief
/// @param window
/// @param positionX
/// @param positionY
void INPUT_MOUSE_POSITION_CALLBACK(GLFWwindow *window, double positionX, double positionY)
{
    (void)window;

    // RJGlobal_DebugInfo("Mouse position updated to (%.2f, %.2f)", positionX, positionY);

    INPUT_MOUSE_POSITION.x = (int)positionX;
    INPUT_MOUSE_POSITION.y = (int)positionY;
}

/// @brief
/// @param window
/// @param button
/// @param action
/// @param mods
void INPUT_MOUSE_BUTTON_CALLBACK(GLFWwindow *window, int button, int action, int mods)
{
    (void)window;
    (void)mods;

    // RJGlobal_DebugInfo("Mouse button %d action %s", button, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : "unknown");

    if (button >= InputMouseButtonCode_Left && button <= InputMouseButtonCode_Fn5)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_MOUSE_BUTTONS[button] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_MOUSE_BUTTONS[button] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else
    {
        RJGlobal_DebugWarning("Unhandled mouse button input: %d", button);
    }
}

/// @brief
/// @param window
/// @param offsetX
/// @param offsetY
void INPUT_MOUSE_SCROLL_CALLBACK(GLFWwindow *window, double offsetX, double offsetY)
{
    (void)window;
    (void)offsetX;

    // RJGlobal_DebugInfo("Mouse scrolled with offset (%.2f, %.2f)", offsetX, offsetY);

    INPUT_MOUSE_SCROLL = (float)offsetY;
}

#pragma endregion Source Only

void Input_Initialize(ContextWindow *window)
{
    RJGlobal_DebugAssertNullPointerCheck(window);

    INPUT_MAIN_WINDOW = window;

    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_STICKY_KEYS, GLFW_FALSE);
    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_STICKY_MOUSE_BUTTONS, GLFW_FALSE);
    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    glfwSetKeyCallback(INPUT_MAIN_WINDOW->handle, INPUT_KEY_CALLBACK);
    glfwSetCursorPosCallback(INPUT_MAIN_WINDOW->handle, INPUT_MOUSE_POSITION_CALLBACK);
    glfwSetMouseButtonCallback(INPUT_MAIN_WINDOW->handle, INPUT_MOUSE_BUTTON_CALLBACK);
    glfwSetScrollCallback(INPUT_MAIN_WINDOW->handle, INPUT_MOUSE_SCROLL_CALLBACK);

    RJGlobal_DebugInfo("Input system initialized successfully");
}

void Input_ConfigureMouseMode(InputMouseMode mode)
{
    RJGlobal_DebugAssertNullPointerCheck(INPUT_MAIN_WINDOW);

    glfwSetInputMode(INPUT_MAIN_WINDOW->handle, GLFW_CURSOR, (int)mode);
}

void Input_Update()
{
    INPUT_PREVIOUS_MOUSE_POSITION = INPUT_MOUSE_POSITION;
    INPUT_MOUSE_SCROLL = 0.0f;

    switch (INPUT_KEY_SPACE)
    {
    case InputState_Up:
        INPUT_KEY_SPACE = InputState_Released;
        break;
    case InputState_Down:
        INPUT_KEY_SPACE = InputState_Pressed;
        break;
    default:
        break;
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_NUMBERS_COUNT; i++)
    {
        switch (INPUT_KEY_NUMBERS[i])
        {
        case InputState_Up:
            INPUT_KEY_NUMBERS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_NUMBERS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_ALPHABETS_COUNT; i++)
    {
        switch (INPUT_KEY_ALPHABETS[i])
        {
        case InputState_Up:
            INPUT_KEY_ALPHABETS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_ALPHABETS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_SPECIALS_COUNT; i++)
    {
        switch (INPUT_KEY_SPECIALS[i])
        {
        case InputState_Up:
            INPUT_KEY_SPECIALS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_SPECIALS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_FUNCTIONS_COUNT; i++)
    {
        switch (INPUT_KEY_FUNCTIONS[i])
        {
        case InputState_Up:
            INPUT_KEY_FUNCTIONS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_FUNCTIONS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_CONTROLS_COUNT; i++)
    {
        switch (INPUT_KEY_CONTROLS[i])
        {
        case InputState_Up:
            INPUT_KEY_CONTROLS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_CONTROLS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }

    for (RJGlobal_Size i = 0; i < INPUT_KEY_MOUSE_BUTTONS_COUNT; i++)
    {
        switch (INPUT_KEY_MOUSE_BUTTONS[i])
        {
        case InputState_Up:
            INPUT_KEY_MOUSE_BUTTONS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_KEY_MOUSE_BUTTONS[i] = InputState_Pressed;
            break;
        default:
            break;
        }
    }
}

bool Input_GetKey(InputKeyCode key, InputState state)
{
    return (Input_GetKeyState(key) & state) != 0;
}

bool Input_GetMouseButton(InputMouseButtonCode button, InputState state)
{
    return (Input_GetMouseButtonState(button) & state) != 0;
}

InputState Input_GetKeyState(InputKeyCode key)
{
    if (key >= InputKeyCode_A && key <= InputKeyCode_Z)
    {
        return INPUT_KEY_ALPHABETS[key - InputKeyCode_A];
    }
    else if (key >= InputKeyCode_Num0 && key <= InputKeyCode_Num9)
    {
        return INPUT_KEY_NUMBERS[key - InputKeyCode_Num0];
    }
    else if (key >= InputKeyCode_Escape && key <= InputKeyCode_UpArrow)
    {
        return INPUT_KEY_SPECIALS[key - InputKeyCode_Escape];
    }
    else if (key >= InputKeyCode_LeftShift && key <= InputKeyCode_RightSuper)
    {
        return INPUT_KEY_CONTROLS[key - InputKeyCode_LeftShift];
    }
    else if (key >= InputKeyCode_F1 && key <= InputKeyCode_F12)
    {
        return INPUT_KEY_FUNCTIONS[key - InputKeyCode_F1];
    }
    else if (key == InputKeyCode_Space)
    {
        return INPUT_KEY_SPACE;
    }
    else
    {
        RJGlobal_DebugWarning("Unhandled key input: %d", key);
        return InputState_Released;
    }
}

InputState Input_GetMouseButtonState(InputMouseButtonCode button)
{
    if (button >= InputMouseButtonCode_Left && button <= InputMouseButtonCode_Fn5)
    {
        return INPUT_KEY_MOUSE_BUTTONS[button];
    }
    else
    {
        RJGlobal_DebugWarning("Unhandled mouse button input: %d", button);
        return InputState_Released;
    }
}

float Input_GetMouseScroll()
{
    return INPUT_MOUSE_SCROLL;
}

Vector2Int Input_GetMousePosition()
{
    return INPUT_MOUSE_POSITION;
}

Vector2Int Input_GetMousePositionDelta()
{
    return Vector2Int_Add(INPUT_MOUSE_POSITION, Vector2Int_Scale(INPUT_PREVIOUS_MOUSE_POSITION, -1.0f));
}

Vector3 Input_GetMovementVector()
{
    Vector3 input = {0};

    if (Input_GetKey(InputKeyCode_W, InputState_Down | InputState_Pressed) ||
        Input_GetKey(InputKeyCode_UpArrow, InputState_Down | InputState_Pressed))
    {
        input.y += 1.0f;
    }

    if (Input_GetKey(InputKeyCode_S, InputState_Down | InputState_Pressed) ||
        Input_GetKey(InputKeyCode_DownArrow, InputState_Down | InputState_Pressed))
    {
        input.y -= 1.0f;
    }

    if (Input_GetKey(InputKeyCode_D, InputState_Down | InputState_Pressed) ||
        Input_GetKey(InputKeyCode_RightArrow, InputState_Down | InputState_Pressed))
    {
        input.x += 1.0f;
    }

    if (Input_GetKey(InputKeyCode_A, InputState_Down | InputState_Pressed) ||
        Input_GetKey(InputKeyCode_LeftArrow, InputState_Down | InputState_Pressed))
    {
        input.x -= 1.0f;
    }

    if (Input_GetKey(InputKeyCode_Space, InputState_Down | InputState_Pressed))
    {
        input.z += 1.0f;
    }

    if (Input_GetKey(InputKeyCode_LeftControl, InputState_Down | InputState_Pressed) ||
        Input_GetKey(InputKeyCode_RightControl, InputState_Down | InputState_Pressed))
    {
        input.z -= 1.0f;
    }

    return Vector3_Normalized(input);
}
