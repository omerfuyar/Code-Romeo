# Code-Romeo

* This project is a cross platform project to try out some new rendering libraries, techniques and design patterns in the C programming language.
* I once tried out making a game engine in C++ but it was too complex and I wasn't knowing what I was doing. So I decided to try something similar in C. (I also don't like C++. I find it pretty powerful but too complex for my taste. Its also more fun to make your own stuff in C.)
* This project do not have a structure like my previous projects (Core and App, Engine and Game). Instead I have focused on mostly doing one thing, just a game or whatever it is.
* ReadMe sections for MacOS may not be right because I dont have a mac to test on it.

## Dependencies

### `C23` standard

### `CMake` for build system

### `Ninja` for the build system generator

### `Clang` for the preferred compiler

### `OpenGL and GLAD` for OpenGL rendering (included in the project)

### `GLFW` for media and OpenGL context management (included in the project)

### `CGLM` for math library (included in the project)

### `STB` for resource loading (included in the project)

### Commands to install all

#### Linux
``` bash
sudo apt install cmake clang ninja-build libwayland-dev libxkbcommon-dev wayland-protocols libglfw3-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

#### MacOS
``` bash
brew install cmake clang ninja
```

### Windows
Download and install `CMake` from the [official site](https://cmake.org/download/) (in binary distributions section)

Download and install `LLVM` from the [official site](https://releases.llvm.org/download.html) (Enable the option "Add LLVM to system PATH" during installation)

``` powershell
winget install -e --id Ninja-build.Ninja
```

## Build

### To clone the repository
``` bash
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

### Linux and MacOS
``` bash
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows
``` powershell
mkdir build
cd build
cmake .. -G "Ninja"
cmake --build .
```

# Note to myself
* glfw and cglm is included in the project with `add_subdirectory()` function in cmake file.
* glad and stb is included in the project by adding the src and include directories to cmake.
* do not forget to add include and src directories to cmake and vscode/ide settings.
* only glad requires manual compilation for src files. other dependencies are handled by CMake.