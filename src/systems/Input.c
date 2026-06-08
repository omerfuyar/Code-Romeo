#include "systems/Input.h"

#include "tools/Context.h"

#include "GLFW/glfw3.h"

#pragma region Source Only

#define INPUT_KEY_NUMBERS_COUNT (InputKeyCode_Num9 - InputKeyCode_Num0 + 1)
#define INPUT_KEY_ALPHABETS_COUNT (InputKeyCode_Z - InputKeyCode_A + 1)
#define INPUT_KEY_SPECIALS_COUNT (InputKeyCode_UpArrow - InputKeyCode_Escape + 1)
#define INPUT_KEY_FUNCTIONS_COUNT (InputKeyCode_F12 - InputKeyCode_F1 + 1)
#define INPUT_KEY_CONTROLS_COUNT (InputKeyCode_RightSuper - InputKeyCode_LeftShift + 1)
#define INPUT_MOUSE_BUTTONS_COUNT (InputMouseButtonCode_F5 - InputMouseButtonCode_Left + 1)

bool INPUT_IS_INITIALIZED = {0};

InputState INPUT_KEY_SPACE = {0};
InputState INPUT_KEY_NUMBERS[INPUT_KEY_NUMBERS_COUNT] = {0};
InputState INPUT_KEY_ALPHABETS[INPUT_KEY_ALPHABETS_COUNT] = {0};
InputState INPUT_KEY_SPECIALS[INPUT_KEY_SPECIALS_COUNT] = {0};
InputState INPUT_KEY_FUNCTIONS[INPUT_KEY_FUNCTIONS_COUNT] = {0};
InputState INPUT_KEY_CONTROLS[INPUT_KEY_CONTROLS_COUNT] = {0};
InputState INPUT_MOUSE_BUTTONS[INPUT_MOUSE_BUTTONS_COUNT] = {0};

float INPUT_MOUSE_SCROLL = {0};
Vector2Int INPUT_MOUSE_POSITION = {0};
Vector2Int INPUT_MOUSE_PREVIOUS_POSITION = {0};

static void INPUT_KEY_CALLBACK(GLFWwindow *window, int key, int scanCode, int action, int mods)
{
    (void)window;
    (void)scanCode;
    (void)mods;

    // RJ_DebugInfo("Key %d action %s", key, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : action == GLFW_REPEAT ? "repeated" : "unknown");

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
        RJ_DebugWarning("Unhandled key input: %d", key);
    }
}

static void INPUT_MOUSE_POSITION_CALLBACK(GLFWwindow *window, double positionX, double positionY)
{
    (void)window;

    // RJ_DebugInfo("Mouse position updated to (%.2f, %.2f)", positionX, positionY);

    INPUT_MOUSE_POSITION.x = (int)positionX;
    INPUT_MOUSE_POSITION.y = (int)positionY;
}

static void INPUT_MOUSE_BUTTON_CALLBACK(GLFWwindow *window, int button, int action, int mods)
{
    (void)window;
    (void)mods;

    // RJ_DebugInfo("Mouse button %d action %s", button, action == GLFW_PRESS ? "pressed" : action == GLFW_RELEASE ? "released" : "unknown");

    if (button >= InputMouseButtonCode_Left && button <= InputMouseButtonCode_F5)
    {
        switch (action)
        {
        case GLFW_PRESS:
            INPUT_MOUSE_BUTTONS[button] = InputState_Down;
            break;
        case GLFW_RELEASE:
            INPUT_MOUSE_BUTTONS[button] = InputState_Up;
            break;
        default:
            break;
        }
    }
    else
    {
        RJ_DebugWarning("Unhandled mouse button input: %d", button);
    }
}

static void INPUT_MOUSE_SCROLL_CALLBACK(GLFWwindow *window, double offsetX, double offsetY)
{
    (void)window;
    (void)offsetX;

    // RJ_DebugInfo("Mouse scrolled with offset (%.2f, %.2f)", offsetX, offsetY);

    INPUT_MOUSE_SCROLL = (float)offsetY;
}

#pragma endregion Source Only

void Input_Initialize()
{
    const ContextWindow *window = Context_GetInternalData();
    // todo add assertions for initialization order

    glfwSetInputMode(window->handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetInputMode(window->handle, GLFW_STICKY_KEYS, GLFW_FALSE);
    glfwSetInputMode(window->handle, GLFW_STICKY_MOUSE_BUTTONS, GLFW_FALSE);
    glfwSetInputMode(window->handle, GLFW_LOCK_KEY_MODS, GLFW_TRUE);
    glfwSetInputMode(window->handle, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    glfwSetKeyCallback(window->handle, INPUT_KEY_CALLBACK);
    glfwSetCursorPosCallback(window->handle, INPUT_MOUSE_POSITION_CALLBACK);
    glfwSetMouseButtonCallback(window->handle, INPUT_MOUSE_BUTTON_CALLBACK);
    glfwSetScrollCallback(window->handle, INPUT_MOUSE_SCROLL_CALLBACK);

    INPUT_IS_INITIALIZED = true;

    RJ_DebugInfo("Input system initialized successfully");
}

void Input_Terminate(void)
{
    const ContextWindow *window = Context_GetInternalData();

    glfwSetKeyCallback(window->handle, NULL);
    glfwSetCursorPosCallback(window->handle, NULL);
    glfwSetMouseButtonCallback(window->handle, NULL);
    glfwSetScrollCallback(window->handle, NULL);

    INPUT_IS_INITIALIZED = false;
}

bool Input_IsInitialized(void)
{
    return INPUT_IS_INITIALIZED;
}

void Input_ConfigureCursorMode(InputCursorMode mode)
{
    glfwSetInputMode(Context_GetInternalData()->handle, GLFW_CURSOR, (int)mode);
}

void Input_Update(void)
{
    INPUT_MOUSE_PREVIOUS_POSITION = INPUT_MOUSE_POSITION;
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

    for (RJ_Size i = 0; i < INPUT_KEY_NUMBERS_COUNT; i++)
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

    for (RJ_Size i = 0; i < INPUT_KEY_ALPHABETS_COUNT; i++)
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

    for (RJ_Size i = 0; i < INPUT_KEY_SPECIALS_COUNT; i++)
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

    for (RJ_Size i = 0; i < INPUT_KEY_FUNCTIONS_COUNT; i++)
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

    for (RJ_Size i = 0; i < INPUT_KEY_CONTROLS_COUNT; i++)
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

    for (RJ_Size i = 0; i < INPUT_MOUSE_BUTTONS_COUNT; i++)
    {
        switch (INPUT_MOUSE_BUTTONS[i])
        {
        case InputState_Up:
            INPUT_MOUSE_BUTTONS[i] = InputState_Released;
            break;
        case InputState_Down:
            INPUT_MOUSE_BUTTONS[i] = InputState_Pressed;
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
        RJ_DebugWarning("Unhandled key input: %d. Returning InputState_Released.", key);
        return InputState_Released;
    }
}

InputState Input_GetMouseButtonState(InputMouseButtonCode button)
{
    if (button >= InputMouseButtonCode_Left && button <= InputMouseButtonCode_F5)
    {
        return INPUT_MOUSE_BUTTONS[button];
    }
    else
    {
        RJ_DebugWarning("Unhandled mouse button input: %d", button);
        return InputState_Released;
    }
}

float Input_GetMouseScroll(void)
{
    return INPUT_MOUSE_SCROLL;
}

Vector2Int Input_GetMousePosition(void)
{
    return INPUT_MOUSE_POSITION;
}

Vector2Int Input_GetMousePositionDelta(void)
{
    return Vector2_Sum(INPUT_MOUSE_POSITION, Vector2_Scale(INPUT_MOUSE_PREVIOUS_POSITION, -1));
}

Vector3 Input_GetMovementVector(void)
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
