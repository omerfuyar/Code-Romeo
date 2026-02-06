# Code-Romeo

* This framework library project is a cross platform project to try out some new rendering concepts, techniques and design patterns in the C programming language.
* Usage and examples of the framework can be found in my [Code-Juliett](https://github.com/omerfuyar/Code-Juliett) repository.
* Because I do not have any Mac, I am not able to test anything on MacOS. So some parts of the software may not be working correctly.
* To see the development guidelines and other details please check [CONTRIBUTING.md](./CONTRIBUTING.md).

## To clone the repository
``` shell
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo
```

## Dependencies

* Library dependencies of the project is included and handled all by the project itself. So user is not responsible for getting packages manually. You just need to install depended packages, clone the repo with submodules and compile & run the build source which will be explained in build section.
* GLFW and CGLM are subdirectories (which cloned with the clone command at the top) are compiled statically and linked to the main application. Other dependencies are just files and they may or may not be edited by me 😁

### `OpenGL 3.3` for rendering

### Linux commands to install dependencies
``` shell
# For Debian/Ubuntu
sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev xvfb

# For Arch
sudo pacman -S libxkbcommon wayland wayland-protocols libx11 libxrandr libxinerama libxcursor libxi mesa xorg-server-xvfb
```

## Build

* Project uses my own [build library](https://github.com/omerfuyar/shuild) to build all sub dependencies. User just needs to compile the ShuildRomeo.c to build static libraries into build/release/. I will use clang for examples but you can use your own (clang/gcc/msvc).

``` shell
clang shuild.c -o shuild -O3
./shuild clang r all
```
