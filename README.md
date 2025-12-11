# Code-Romeo

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()

A cross-platform C rendering framework exploring modern rendering concepts, techniques, and design patterns.

## 📚 Documentation

- **[Getting Started Guide](GETTING_STARTED.md)** - Your first steps with Code-Romeo
- **[Contributing Guidelines](CONTRIBUTING.md)** - Development guidelines and architecture
- **[Project Analysis](PROJECT_ANALYSIS.md)** - Comprehensive analysis and recommendations
- **[Security Policy](SECURITY.md)** - Security practices and reporting
- **[Examples](https://github.com/omerfuyar/Code-Juliett)** - Real-world usage in Code-Juliett repository

## ✨ Features

* **Modular Design**: Pick and choose components you need
* **Cross-Platform**: Windows, Linux, and macOS support
* **Layered Architecture**: Clean separation of concerns
* **Well Documented**: Inline documentation for all public APIs
* **OpenGL 3.3**: Modern graphics rendering
* **Free & Open Source**: Apache 2.0 License

## 🚀 Quick Start

### Clone with Submodules

**Important:** Use `--recurse-submodules` to include dependencies:

```shell
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

If you already cloned without submodules:
```shell
git submodule update --init --recursive
```

### Build with CMake (Recommended)

```shell
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The library will be at `build/libCodeRomeo.a`

### Alternative: Build with shuild

```shell
clang shuild.c -o ShuildRomeo -O3
./ShuildRomeo clang r all
```

## 📋 Dependencies

### System Dependencies

#### Linux (Ubuntu/Debian)
```shell
sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev xvfb
```

#### macOS
```shell
# Install Xcode Command Line Tools
xcode-select --install
```

#### Windows
No additional dependencies needed - Visual Studio or MinGW includes everything.

### Included Dependencies

All library dependencies are included as submodules:
- **GLFW** - Window and context management
- **CGLM** - OpenGL Mathematics (GLM)
- **GLAD** - OpenGL loader
- **stb_image** - Image loading
- **miniaudio** - Audio playback

## 🏗️ Architecture

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

**Rule**: Each layer only depends on layers below it.

## 📖 Example Usage

```c
#include "RJGlobal.h"
#include "tools/Context.h"
#include "systems/Input.h"

ContextWindow *window = NULL;

void setup(int argc, char **argv) {
    Context_Initialize();
    window = Context_WindowCreate(scl("My App"), 800, 600, false, true);
    Input_Initialize(window);
}

void loop(float deltaTime) {
    Input_Update();
    
    if (Input_GetKeyState(InputKeyCode_Escape) == InputState_Pressed) {
        RJGlobal_SetLoopCallback(NULL);
    }
    
    Context_WindowSwapBuffers(window);
    Context_PollEvents();
}

void terminate(int exitCode, char *message) {
    Input_Terminate();
    Context_WindowDestroy(window);
    Context_Terminate();
}

int main(int argc, char **argv) {
    RJGlobal_SetSetupCallback(setup);
    RJGlobal_SetLoopCallback(loop);
    RJGlobal_SetTerminateCallback(terminate);
    RJGlobal_Run(argc, argv);
    return 0;
}
```

For more examples, see [Code-Juliett](https://github.com/omerfuyar/Code-Juliett).

## 🧪 Testing

Build with tests:
```shell
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest --output-on-failure
```

Enable sanitizers for debugging:
```shell
cmake .. -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build .
```

## 🤝 Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Development guidelines
- Naming conventions
- Documentation standards
- Commit message format

## ⚠️ Platform Notes

- **Windows**: Fully tested and supported
- **Linux**: Fully tested and supported
- **macOS**: Supported but less tested (contributions welcome!)

## 📄 License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

## 🔗 Links

- **Examples**: [Code-Juliett](https://github.com/omerfuyar/Code-Juliett)
- **Build System**: [shuild](https://github.com/omerfuyar/shuild)
- **Issues**: [GitHub Issues](https://github.com/omerfuyar/Code-Romeo/issues)

---

**Note**: This project is under active development. See [PROJECT_ANALYSIS.md](PROJECT_ANALYSIS.md) for known issues and planned improvements.
