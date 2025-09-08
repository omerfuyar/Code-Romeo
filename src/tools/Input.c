#include "tools/Input.h"
#include <GLFW/glfw3.h>

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

float INPUT_MOUSE_SCROLL = 0.0f;
Vector2Int INPUT_MOUSE_POSITION = {0};
Vector2Int INPUT_PREVIOUS_MOUSE_POSITION = {0};

void INPUT_KEY_CALLBACK(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
    (void)window;
    (void)scanCode;
    (void)mods;

    // DebugInfo("Key %d action %s", key, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : action == GLFW_REPEAT ? "repeated" : "unknown");

    if (key >= InputKeyCode_Num0 && key <= InputKeyCode_Num9)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_NUMBERS[key - InputKeyCode_Num0] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_NUMBERS[key - InputKeyCode_Num0] = InputKeyState_Up;
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
            INPUT_KEY_ALPHABETS[key - InputKeyCode_A] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_ALPHABETS[key - InputKeyCode_A] = InputKeyState_Up;
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
            INPUT_KEY_SPECIALS[key - InputKeyCode_Escape] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_SPECIALS[key - InputKeyCode_Escape] = InputKeyState_Up;
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
            INPUT_KEY_FUNCTIONS[key - InputKeyCode_F1] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_FUNCTIONS[key - InputKeyCode_F1] = InputKeyState_Up;
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
            INPUT_KEY_CONTROLS[key - InputKeyCode_LeftShift] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_CONTROLS[key - InputKeyCode_LeftShift] = InputKeyState_Up;
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
            INPUT_KEY_SPACE = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_SPACE = InputKeyState_Up;
            break;
        default:
            break;
        }
    }
    else
    {
        DebugWarning("Unhandled key input: %d", key);
    }
}

void INPUT_MOUSE_POSITION_CALLBACK(GLFWwindow *window, double positionX, double positionY)
{
    (void)window;

    // DebugInfo("Mouse position updated to (%.2f, %.2f)", positionX, positionY);

    INPUT_MOUSE_POSITION.x = (int)positionX;
    INPUT_MOUSE_POSITION.y = (int)positionY;
}

void INPUT_MOUSE_BUTTON_CALLBACK(GLFWwindow *window, int button, int action, int mods)
{
    (void)window;
    (void)mods;

    // DebugInfo("Mouse button %d action %s", button, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : "unknown");

    if (button >= InputMouseButtonCode_Left && button <= InputMouseButtonCode_Fn5)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_KEY_MOUSE_BUTTONS[button] = InputKeyState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_KEY_MOUSE_BUTTONS[button] = InputKeyState_Up;
            break;
        default:
            break;
        }
    }
    else
    {
        DebugWarning("Unhandled mouse button input: %d", button);
    }
}

void INPUT_MOUSE_SCROLL_CALLBACK(GLFWwindow *window, double offsetX, double offsetY)
{
    (void)window;
    (void)offsetX;

    DebugInfo("Mouse scrolled with offset (%.2f, %.2f)", offsetX, offsetY);

    INPUT_MOUSE_SCROLL = (float)offsetY;
}

void Input_Initialize(void *window)
{
    GLFWwindow *glfwWindow = (GLFWwindow *)window;

    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_FALSE);
    glfwSetInputMode(glfwWindow, GLFW_STICKY_MOUSE_BUTTONS, GLFW_FALSE);
    glfwSetInputMode(glfwWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    glfwSetKeyCallback(glfwWindow, INPUT_KEY_CALLBACK);
    glfwSetCursorPosCallback(glfwWindow, INPUT_MOUSE_POSITION_CALLBACK);
    glfwSetMouseButtonCallback(glfwWindow, INPUT_MOUSE_BUTTON_CALLBACK);
    glfwSetScrollCallback(glfwWindow, INPUT_MOUSE_SCROLL_CALLBACK);

    DebugInfo("Input system initialized successfully");
}

void Input_Update()
{
    INPUT_PREVIOUS_MOUSE_POSITION = INPUT_MOUSE_POSITION;

    switch (INPUT_KEY_SPACE)
    {
    case InputKeyState_Up:
        INPUT_KEY_SPACE = InputKeyState_Released;
        break;
    case InputKeyState_Down:
        INPUT_KEY_SPACE = InputKeyState_Pressed;
        break;
    default:
        break;
    }

    for (size_t i = 0; i < INPUT_KEY_NUMBERS_COUNT; i++)
    {
        switch (INPUT_KEY_NUMBERS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_NUMBERS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_NUMBERS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    for (size_t i = 0; i < INPUT_KEY_ALPHABETS_COUNT; i++)
    {
        switch (INPUT_KEY_ALPHABETS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_ALPHABETS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_ALPHABETS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    for (size_t i = 0; i < INPUT_KEY_SPECIALS_COUNT; i++)
    {
        switch (INPUT_KEY_SPECIALS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_SPECIALS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_SPECIALS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    for (size_t i = 0; i < INPUT_KEY_FUNCTIONS_COUNT; i++)
    {
        switch (INPUT_KEY_FUNCTIONS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_FUNCTIONS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_FUNCTIONS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    for (size_t i = 0; i < INPUT_KEY_CONTROLS_COUNT; i++)
    {
        switch (INPUT_KEY_CONTROLS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_CONTROLS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_CONTROLS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    for (size_t i = 0; i < INPUT_KEY_MOUSE_BUTTONS_COUNT; i++)
    {
        switch (INPUT_KEY_MOUSE_BUTTONS[i])
        {
        case InputKeyState_Up:
            INPUT_KEY_MOUSE_BUTTONS[i] = InputKeyState_Released;
            break;
        case InputKeyState_Down:
            INPUT_KEY_MOUSE_BUTTONS[i] = InputKeyState_Pressed;
            break;
        default:
            break;
        }
    }

    glfwPollEvents();
}

bool Input_GetKey(InputKeyCode key, InputState state)
{
    return Input_GetKeyState(key) == state;
}

bool Input_GetMouseButton(InputMouseButtonCode button, InputState state)
{
    return Input_GetMouseButtonState(button) == state;
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
        DebugWarning("Unhandled key input: %d", key);
        return InputKeyState_Released;
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
        DebugWarning("Unhandled mouse button input: %d", button);
        return InputKeyState_Released;
    }
}

float Input_GetMouseButtonScroll()
{
    return INPUT_MOUSE_SCROLL;
}

Vector2Int GetMousePosition()
{
    return INPUT_MOUSE_POSITION;
}

Vector2Int GetMousePositionDelta()
{
    return Vector2Int_Add(INPUT_MOUSE_POSITION, Vector2Int_Scale(INPUT_PREVIOUS_MOUSE_POSITION, -1.0f));
}
