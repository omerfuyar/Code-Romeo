# Code-Romeo

* This project is a cross platform project to try out some new rendering libraries, techniques and design patterns in the C programming language.
* I once tried out making a game engine in C++ but it was too complex and I wasn't knowing what I was doing. So I decided to try something similar in C. (I also don't like C++. I find it pretty powerful but too complex for my taste. Its also more fun to make your own stuff in C.)
* This project do not have a structure like my previous projects (Core and App, Engine and Game). Instead I have focused on mostly doing one thing, just a game or whatever it is.
* ReadMe sections for MacOS may not be right because I dont have a mac to test on it.

## To clone the repository
``` bash
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

## Dependencies

### Note

* Library dependencies of the project is included and handled all by cmake. So user is not responsible for getting packages for runtime.
* GLFW and CGLM are subdirectories (which cloned with the clone command at the top) are compiled statically and linked to the main application. Other dependencies are just files and they may or may not be edited by me 😁

### `C23` standard

### `CMake` for build system

### `Ninja` for the build system generator

### `Clang` for the preferred compiler

* You can select your compiler as you want but I am using clang and it works best for this project. I don't even test other compilers.

### `OpenGL and GLAD` for OpenGL rendering (included in the project)

### `GLFW` for media and OpenGL context management (included in the project)

### `CGLM` for math library (included in the project)

### `STB` for resource loading (included in the project)

### Commands to install all

#### Linux
``` bash
sudo apt install cmake ninja-build  libwayland-dev libxkbcommon-dev wayland-protocols libglfw3-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev clang pkgconf
```

#### MacOS
``` bash
brew install cmake ninja clang
```

#### Windows
``` powershell
winget install -e --id Ninja-build.Ninja 
winget install -e --id Kitware.CMake
winget install -e --id LLVM.LLVM
```

## Build

### Linux and MacOS
``` bash
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows
``` powershell
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

#### Note
* If running on Windows with sanitizers, you will need to copy runtime to the .exe directory (change the name, architecture and paths):
``` powershell
$asanDll = Get-ChildItem -Path "path\to\LLVM" -Recurse -Filter "clang_rt.asan_dynamic-x86_64.dll" | Select-Object -First 1
Copy-Item $asanDll.FullName -Destination "path\to\exe"
```