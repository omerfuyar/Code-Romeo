#define SHUC_NO_RUN_LOG
#define SHUC_MAX_COMMAND_BUFFER_SIZE 8192
#define SHUILD_IMPLEMENTATION
#include "shuild.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        goto usageError;
    }

    char isDebug = -1;
    char *buildTypeString = NULL;

    if (strcmp(argv[2], "d") == 0)
    {
        buildTypeString = "Debug";
        isDebug = 1;
    }
    else if (strcmp(argv[2], "r") == 0)
    {
        buildTypeString = "Release";
        isDebug = 0;
    }
    else
    {
        return 2;
    }

    SHU_CompilerTryConfigure(argv[1]);
    SHU_Automate(argc, argv);

    SHU_CompilerSetFlags("-w -DCGLM_STATIC");
#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#endif

    char *arcOutputDirectory = isDebug ? "build/debug/" : "build/release/";

    char *compilerFlags = NULL;

    if (argc > 3)
    {
        if (strcmp(argv[1], "clang-cl") == 0 || strcmp(argv[1], "cl") == 0)
        {
            compilerFlags = "/O2 /DNDEBUG";
        }
        else if (strcmp(argv[1], "clang") == 0 || strcmp(argv[1], "gcc") == 0)
        {
            compilerFlags = "-O3 -DNDEBUG";
        }
        else
        {
            return 3;
        }

        SHU_CompilerAddFlags(compilerFlags);

        SHU_ModuleBegin("cglm");
        SHU_ModuleAddIncludeDirectory("dependencies/cglm/include/");
        SHU_ModuleAddSourceDirectory("dependencies/cglm/src/");
        SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

        SHU_ModuleBegin("glfw");
        SHU_ModuleAddIncludeDirectory("dependencies/glfw/include/");
        SHU_ModuleAddSourceDirectory("dependencies/glfw/src/");
        SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);
    }

    if (isDebug)
    {
        if (strcmp(argv[1], "clang-cl") == 0 || strcmp(argv[1], "cl") == 0)
        {
            compilerFlags = "/Zi /Od /W4 /permissive- /GS /WX /wd4324";
        }
        else if (strcmp(argv[1], "clang") == 0 || strcmp(argv[1], "gcc") == 0)
        {
            compilerFlags = "-g -O0 -Wall -Werror -Wextra -Wshadow -Wpedantic -Wconversion -Wnull-dereference -Wunused-result -Wno-strict-prototypes -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-value -fstack-protector-strong";
        }
        else
        {
            return 3;
        }
    }
    else
    {
        if (strcmp(argv[1], "clang-cl") == 0 || strcmp(argv[1], "cl") == 0)
        {
            compilerFlags = "/O2 /DNDEBUG";
        }
        else if (strcmp(argv[1], "clang") == 0 || strcmp(argv[1], "gcc") == 0)
        {
            compilerFlags = "-O3 -DNDEBUG";
        }
        else
        {
            return 3;
        }
    }

    SHU_CompilerSetFlags(compilerFlags);

    SHU_CompilerAddFlags("-DCGLM_STATIC");
#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#endif

    SHU_Log(0, SHUM_COLOR_BLUE("Romeo"), "Build info : %s", buildTypeString);
    SHU_Log(0, SHUM_COLOR_BLUE("Romeo"), "Compiler info : %s", argv[1]);
    SHU_Log(0, SHUM_COLOR_BLUE("Romeo"), "Compile options : %s", compilerFlags);

    SHU_ModuleBegin("Code-Romeo");

    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glfw/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/cglm/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glad/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/stb/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/miniaudio/include/");

    SHU_ModuleAddSourceDirectory("src/");
    SHU_ModuleAddSourcefile("dependencies/glad/src/glad.c");

    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    return 0;

usageError:
    SHU_LogInfo("Usage is <compiler> <d/r> [all]");
    return 1;
}