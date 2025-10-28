# Code-Romeo

* This framework library project is a cross platform project to try out some new rendering concepts, techniques and design patterns in the C programming language.
* I once tried out making a game engine (framework actually) in C++ but it was too complex and I wasn't knowing what I was doing. So I decided to try something similar in C. (I also don't like C++. I find it pretty powerful but too complex for my taste. Its also more fun to make your own stuff in C.)
* Guide to use the framework explained below. Framework runs the callback functions with needed parameters in the correct time.
* ReadMe sections for MacOS may not be right because I dont have a mac to test on it.
* An example of the usage can be found in my 'Code-Juliett' repository.

## To clone the repository
``` shell
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

## Dependencies

### Note

* Library dependencies of the project is included and handled all by cmake. So user is not responsible for getting packages for runtime. You just need to install depended packages, if have any, and clone the repo with submodules.
* GLFW and CGLM are subdirectories (which cloned with the clone command at the top) are compiled statically and linked to the main application. Other dependencies are just files and they may or may not be edited by me 😁

### `C23` standard suggested

### `CMake` for build system

### `Ninja` for the build system generator

### `OpenGL and GLAD` for OpenGL rendering (included in the project)

### `GLFW` for media and context management (included in the project)

### `CGLM` for math library (included in the project)

### `STB` for resource loading (included in the project)

### Commands to install all

#### Linux
``` shell
sudo apt install cmake ninja-build  libwayland-dev libxkbcommon-dev wayland-protocols libglfw3-dev libx11-dev libxrandr-dev libxinerama-dev  libxcursor-dev libxi-dev pkgconf
```

#### MacOS
``` shell
brew install cmake ninja
```

#### Windows
``` powershell
winget install -e --id Ninja-build.Ninja 
winget install -e --id Kitware.CMake
```

## Build

* Project supports clang/clang-cl, msvc and gcc compilers. At least these are the ones I tested.

``` shell
mkdir build
cd build
cmake -S .. -G Ninja -DCMAKE_C_COMPILER=<COMPILER> -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### For development
* Sanitizers may be enabled by adding commands "-DENABLE_ASAN=ON -DENABLE_UBSAN=ON" to the configuration command .

* If running on Windows with sanitizers, you will need to copy sanitizer runtime to the .exe directory. That command might work for you (change the name, architecture and paths, run after build):
``` powershell
$asanDll = Get-ChildItem -Path "path\to\LLVM" -Recurse -Filter "clang_rt.asan_dynamic-x86_64.dll" | Select-Object -First 1
Copy-Item $asanDll.FullName -Destination "path\to\exe"
```
