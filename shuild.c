#define SHUC_NO_RUN_LOG
#define SHUC_MAX_COMMAND_BUFFER_SIZE 8192
#define SHUC_ENABLE_INCREMENTAL
#define SHUILD_IMPLEMENTATION
#include "dependencies/shuild/shuild.h"

void ShowBuildConfig(const char *header, const char *compiler, char isDebug)
{
    SHU_Log(0, header, "Build info : %s", isDebug ? "Debug" : "Release");
    SHU_Log(0, header, "Compiler info : %s", compiler);

    char flagBuffer[2048] = {0};
    SHU_CompilerGetFlags(flagBuffer, sizeof(flagBuffer));

    SHU_Log(0, header, "Compile options : %s", flagBuffer);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        SHU_LogError(1, "Usage is <compiler> <d/r> [clean]");
    }

    char isDebug = -1;

    if (strcmp(argv[2], "d") == 0)
    {
        isDebug = 1;
    }
    else if (strcmp(argv[2], "r") == 0)
    {
        isDebug = 0;
    }
    else
    {
        return 2;
    }

    SHU_CompilerTryConfigure(argv[1]);
    SHU_UtilAutomate(argc, argv);

    SHU_CompilerSetFlags("-w -DCGLM_STATIC");
#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#elif SHUM_HOST_PLATFORM == SHUM_PLATFORM_LINUX
    SHU_CompilerAddFlags("-D_GLFW_X11");
#endif

    char *arcOutputDirectory = isDebug ? "build/debug/" : "build/release/";

    if (argc > 3)
    {
        SHU_LogWarning("Performing clean build...");
        SHU_CacheClearAll();
    }

    SHU_CompilerAddFlags(SHUM_FLAGS_OPTIMIZATION_HIGH);

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo Dependencies"), argv[1], isDebug);

    SHU_ModuleBegin("cglm", "dependencies/cglm/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceDirectory("src/");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("glfw", "dependencies/glfw/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceDirectory("src/");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("glad", "dependencies/glad/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/glad.c");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("miniaudio", "dependencies/miniaudio/");
    SHU_ModuleAddSourceFile("miniaudio.c");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("stb", "dependencies/stb/");
    SHU_ModuleAddSourceFile("stb.c");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_CompilerClearFlags();

    if (isDebug)
    {
        SHU_CompilerAddFlags(SHUM_FLAGS_DEBUG);
        SHU_CompilerAddFlags(SHUM_FLAGS_WARNING_HIGH SHUM_FLAGS_WARNING_ERROR);
        SHU_CompilerAddFlags("-Wno-unused-function -Wno-gnu-zero-variadic-macro-arguments -Wno-format-nonliteral -Wno-language-extension-token");
    }
    else
    {
        SHU_CompilerAddFlags(SHUM_FLAGS_OPTIMIZATION_HIGH);
    }

    SHU_CompilerAddFlags("-DCGLM_STATIC");
#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#elif SHUM_HOST_PLATFORM == SHUM_PLATFORM_LINUX
    SHU_CompilerAddFlags("-D_GLFW_X11");
#endif

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo"), argv[1], isDebug);

    SHU_ModuleBegin("Code-Romeo", "");

    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddIncludeDirectory("dependencies/");
    SHU_ModuleAddIncludeDirectory("dependencies/cglm/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glad/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glfw/include/");

    SHU_ModuleAddSourceDirectory("src/");

    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    return 0;

usageError:
    SHU_LogError(1, "Usage is <compiler> <d/r> [all] [clean]");
}