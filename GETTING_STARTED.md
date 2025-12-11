# Getting Started with Code-Romeo

This guide will help you get started with Code-Romeo, a cross-platform C rendering framework.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Your First Program](#your-first-program)
4. [Basic Concepts](#basic-concepts)
5. [Next Steps](#next-steps)
6. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### System Requirements

- **C Compiler**: clang, gcc, or MSVC
- **CMake**: 3.15 or later (recommended) OR compatible C compiler for shuild
- **Git**: For cloning the repository

### Platform-Specific Dependencies

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y \
    libwayland-dev \
    libxkbcommon-dev \
    wayland-protocols \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev \
    xvfb
```

#### Linux (Fedora)
```bash
sudo dnf install -y \
    wayland-devel \
    libxkbcommon-devel \
    wayland-protocols-devel \
    libX11-devel \
    libXrandr-devel \
    libXinerama-devel \
    libXcursor-devel \
    libXi-devel \
    mesa-libGL-devel
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# OpenGL is included with macOS
# No additional dependencies needed
```

#### Windows
```bash
# No additional dependencies needed
# Visual Studio or MinGW includes required libraries
```

---

## Installation

### 1. Clone the Repository

**IMPORTANT**: Use `--recurse-submodules` to clone dependencies:

```bash
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

If you already cloned without submodules:
```bash
git submodule update --init --recursive
```

### 2. Build with CMake (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure (Debug build)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Or for Release build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# The library will be in build/libCodeRomeo.a
```

### 3. Alternative: Build with shuild

```bash
# Compile the build script
clang shuild.c -o ShuildRomeo -O3

# Build the library (Debug)
./ShuildRomeo clang d all

# Or for Release
./ShuildRomeo clang r all

# The library will be in build/debug/ or build/release/
```

---

## Your First Program

Let's create a minimal program that opens a window using Code-Romeo.

### Step 1: Create Your Project

```bash
mkdir MyRomeoApp
cd MyRomeoApp
```

### Step 2: Create main.c

```c
#include "RJGlobal.h"
#include "tools/Context.h"
#include "systems/Input.h"

// Window handle
ContextWindow *window = NULL;

// Setup callback - called once at startup
void setup(int argc, char **argv)
{
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    RJGlobal_DebugInfo("Application starting...");
    
    // Initialize context (window management)
    Context_Initialize();
    
    // Create a window
    window = Context_WindowCreate(
        scl("My First Code-Romeo App"),  // Title
        800,                              // Width
        600,                              // Height
        false,                            // Not fullscreen
        true                              // Resizable
    );
    
    // Initialize input system
    Input_Initialize(window);
    
    RJGlobal_DebugInfo("Window created successfully!");
}

// Main loop callback - called every frame
void loop(float deltaTime)
{
    // Update input state
    Input_Update();
    
    // Check if user wants to close window
    if (Context_WindowShouldClose(window) || 
        Input_GetKeyState(InputKeyCode_Escape) == InputState_Pressed)
    {
        // Stop the main loop by setting callback to NULL
        RJGlobal_SetLoopCallback(NULL);
        return;
    }
    
    // Swap buffers and poll events
    Context_WindowSwapBuffers(window);
    Context_PollEvents();
    
    // Print FPS occasionally
    static float timeAccum = 0.0f;
    timeAccum += deltaTime;
    if (timeAccum >= 1.0f)
    {
        float fps = 1.0f / deltaTime;
        RJGlobal_DebugInfo("FPS: %.2f", fps);
        timeAccum = 0.0f;
    }
}

// Terminate callback - called when application exits
void terminate(int exitCode, char *message)
{
    (void)exitCode;  // Unused
    (void)message;   // Unused
    
    RJGlobal_DebugInfo("Cleaning up...");
    
    // Terminate systems in reverse order
    Input_Terminate();
    
    if (window != NULL)
    {
        Context_WindowDestroy(window);
    }
    
    Context_Terminate();
}

int main(int argc, char **argv)
{
    // Register callbacks
    RJGlobal_SetSetupCallback(setup);
    RJGlobal_SetLoopCallback(loop);
    RJGlobal_SetTerminateCallback(terminate);
    
    // Run the application
    RJGlobal_Run(argc, argv);
    
    return 0;
}
```

### Step 3: Create CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyRomeoApp C)

# Point to Code-Romeo
set(CODE_ROMEO_DIR "${CMAKE_SOURCE_DIR}/../Code-Romeo")

# Add Code-Romeo
add_subdirectory(${CODE_ROMEO_DIR} code-romeo)

# Create executable
add_executable(MyRomeoApp main.c)

# Link with Code-Romeo
target_link_libraries(MyRomeoApp PRIVATE CodeRomeo)

# Copy executable to root for easy running
add_custom_command(TARGET MyRomeoApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:MyRomeoApp>
    ${CMAKE_SOURCE_DIR}/MyRomeoApp${CMAKE_EXECUTABLE_SUFFIX}
)
```

### Step 4: Build and Run

```bash
mkdir build
cd build
cmake ..
cmake --build .
cd ..
./MyRomeoApp
```

You should see:
- A window titled "My First Code-Romeo App"
- FPS information in the console
- The window closes when you press ESC or close button
- A debug.log file with detailed information

---

## Basic Concepts

### Layer Architecture

Code-Romeo follows a strict layered architecture:

```
┌─────────────────────────────┐
│     Your Application         │
├─────────────────────────────┤
│  Systems (Renderer, Input)   │
├─────────────────────────────┤
│  Tools (Context, Resource)   │
├─────────────────────────────┤
│  Utilities (String, Vector)  │
├─────────────────────────────┤
│       RJGlobal              │
└─────────────────────────────┘
```

**Rule**: Each layer can only depend on layers below it.

### Key Modules

1. **RJGlobal**: Core utilities, platform detection, logging
2. **Utilities**: Data structures, math, strings
3. **Tools**: Context (window/input), Resource loading
4. **Systems**: High-level features (Renderer, Physics, Audio, Input)

### Naming Conventions

- **Types**: `PascalCase` with module prefix (e.g., `RendererBatch`)
- **Functions**: `Module_PascalCase` (e.g., `Context_Initialize`)
- **Enums**: `Type_PascalCase` (e.g., `InputKeyCode_Space`)
- **Variables**: `camelCase` (e.g., `window`, `deltaTime`)

### Initialization Pattern

Most modules follow this pattern:

```c
// 1. Initialize the module
Module_Initialize(/* parameters */);

// 2. Create objects from the module
ModuleObject *obj = Module_ObjectCreate(/* parameters */);

// 3. Use the object
Module_ObjectDoSomething(obj, /* parameters */);

// 4. Destroy the object when done
Module_ObjectDestroy(obj);

// 5. Terminate the module when application exits
Module_Terminate();
```

### Memory Management

- **Strings**: `String` owns memory, `StringView` doesn't
- **Create/Destroy**: Always destroy what you create
- **Order**: Destroy in reverse order of creation

### Debug Logging

Code-Romeo has built-in logging:

```c
RJGlobal_DebugInfo("Info message");
RJGlobal_DebugWarning("Warning message");
RJGlobal_DebugError("Error message");  // May terminate in debug builds
RJGlobal_DebugAssert(condition, "Assertion failed");
```

Logs go to `debug.log` in the executable directory.

---

## Next Steps

### Examples to Try

1. **Window with Input**
   - Already shown above
   - Try different input functions from `Input.h`

2. **Basic Rendering**
   - See Code-Juliett repository for renderer examples
   - Initialize `Renderer_Initialize(window, batchCapacity)`
   - Configure shaders and camera
   - Create batches and render

3. **Resource Loading**
   - Load models, shaders, textures
   - Use `Resource.h` functions

4. **Physics**
   - Add physics simulation
   - Use `Physics.h` module

### Recommended Reading

1. **CONTRIBUTING.md**: Development guidelines and architecture details
2. **Header files in include/**: All functions are documented
3. **Code-Juliett repository**: Real-world usage examples

### Advanced Topics

- Custom data structures (ListArray, HashMap, ListLinked)
- Vector math operations
- Timer and time management
- Audio playback
- Physics simulation
- Advanced rendering techniques

---

## Troubleshooting

### "Submodules not initialized"

**Problem**: Build fails with missing headers

**Solution**:
```bash
git submodule update --init --recursive
```

### "Cannot find OpenGL libraries" (Linux)

**Problem**: Missing system dependencies

**Solution**: Install dependencies from Prerequisites section

### Build fails with shuild

**Problem**: shuild.c won't compile

**Solution**: Use CMake instead:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Window doesn't appear

**Possible causes**:
1. Graphics drivers not installed
2. Running on headless system
3. Context initialization failed (check debug.log)

**Solution**: Check debug.log for detailed error messages

### "debug.log permission denied"

**Problem**: Can't write to executable directory

**Solution**: 
- Run from a directory where you have write permissions
- Or modify `RJGLOBAL_DEBUG_FILE_NAME` in RJGlobal.h

### Performance issues

**Solution**:
- Build in Release mode (`-DCMAKE_BUILD_TYPE=Release`)
- Disable debug logging (will be optimized out in Release)
- Check VSync settings

### macOS-specific issues

**Note**: Some macOS features are untested by the author.

**If you encounter issues**:
1. Report them on GitHub
2. Check for platform-specific code in RJGlobal.c
3. Test with latest macOS and Xcode

---

## Getting Help

- **Issues**: Open a GitHub issue for bugs or questions
- **Discussions**: Use GitHub discussions for general questions
- **Examples**: See Code-Juliett repository
- **Documentation**: Read header files - they're well documented!

---

**Last Updated**: December 11, 2025

**Next**: Check out [CONTRIBUTING.md](CONTRIBUTING.md) for development guidelines or browse the [include/](include/) directory for API documentation.
