# Code-Romeo

* This framework library project is a cross platform project to try out some new rendering concepts, techniques and design patterns in the C programming language.
* Guide to install the framework explained above.
* Usage and examples of the framework can be found in my [Code-Juliett](https://github.com/omerfuyar/Code-Juliett) repository.
* Because I do not have any Mac, I am not able to test anything on MacOS. So some parts of the software may not be working correctly.
* To see the development guidelines and other details please check [CONTRIBUTING.md](./CONTRIBUTING.md).

## To clone the repository
``` shell
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

## Dependencies

* Library dependencies of the project is included and handled all by cmake. So user is not responsible for getting packages for runtime. You just need to install depended packages, if have any, and clone the repo with submodules.
* GLFW and CGLM are subdirectories (which cloned with the clone command at the top) are compiled statically and linked to the main application. Other dependencies are just files and they may or may not be edited by me 😁

### `CMake` for build system

### `Ninja` for the build system generator

### `OpenGL 3.3` for rendering

### Commands to install all

#### Linux
``` shell
sudo apt install cmake ninja-build libwayland-dev libxkbcommon-dev wayland-protocols libx11-dev libxrandr-dev libxinerama-dev  libxcursor-dev libxi-dev libgl1-mesa-dev pkgconf
```

#### MacOS
``` shell
brew install cmake ninja
```

#### Windows
``` powershell
winget install -e --id Kitware.CMake
winget install -e --id Ninja-build.Ninja 
```

## Build

* Project supports clang/clang-cl, msvc and gcc compilers. Clang is prioritized all the time because it is much easier to use as a developer and produces higher performance binary especially compared to msvc.

``` shell
mkdir build
cd build
cmake -S .. -G Ninja -DCMAKE_C_COMPILER=<COMPILER> -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
