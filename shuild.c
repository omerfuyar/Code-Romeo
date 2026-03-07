#define SHUC_NO_RUN_LOG
#define SHUC_MAX_COMMAND_BUFFER_SIZE 8192
#define SHUC_ENABLE_INCREMENTAL
#define SHUILD_IMPLEMENTATION
#define SHUC_SHORT_LOG
#include "dependencies/shuild/shuild.h"

#define DEBUG_REGULAR 1
#define DEBUG_SANITIZE_ADDRESS 2
#define DEBUG_SANITIZE_THREAD 3
#define DEBUG_SANITIZE_UNDEFINED 4
#define DEBUG_SANITIZE_MEMORY 5

void ShowBuildConfig(const char *header, const char *compiler, char isDebug)
{
    SHU_Log(0, header, "Build info : %s", isDebug ? "Debug" : "Release");
    SHU_Log(0, header, "Compiler info : %s", compiler);

    char flagBuffer[1024] = {0};
    SHU_CompilerGetFlags(flagBuffer, sizeof(flagBuffer));

    SHU_Log(0, header, "Compile options : %s", flagBuffer);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        SHU_LogError(1, "Usage is <compiler> <r/d/sa/st/su/sm> [clean]");
    }

    char isDebug = 0;
	char isClean = 0;

	const char *compilerStr = argv[1];
	const char *buildOptStr = argv[2];

    if(strcmp(buildOptStr, "r") == 0)
    {
        isDebug = 0;
    }
    else if(strcmp(buildOptStr, "d") == 0)
    {
    	isDebug = DEBUG_REGULAR;
    }
    else if(strcmp(buildOptStr, "sa") == 0)
    {
		isDebug = DEBUG_SANITIZE_ADDRESS;
    }
    else if(strcmp(buildOptStr, "st") == 0)
    {
		isDebug = DEBUG_SANITIZE_THREAD;
    }
    else if(strcmp(buildOptStr, "su") == 0)
    {
		isDebug = DEBUG_SANITIZE_UNDEFINED;
    }
    else if(strcmp(buildOptStr, "sm") == 0)
    {
		isDebug = DEBUG_SANITIZE_MEMORY;
    }
    else
    {
        SHU_LogError(1, "Specify debug or release build with second parameter <r/d/sa/st/su/sm>.");
    }

	if(isDebug > DEBUG_REGULAR && strcmp(compilerStr, "clang") != 0)
	{
		SHU_LogError(1, "Sanitizers can only be used with Clang compiler");
	}

    if(argv[3] != NULL && strcmp(argv[3], "clean") == 0)
    {
		isClean = 1;
    }
    else if(argv[3] != NULL)
    {
        SHU_LogError(1, "Specify clean build with thrid parameter [clean].");
    }

    SHU_CompilerTryConfigure(argv[1]);
    SHU_UtilAutomate(argc, argv);

    char *arcOutputDirectory = isDebug ? "build/debug/" : "build/release/";

    SHU_CacheConfigure(isDebug ? ".shu/debug/" : ".shu/release/");

    if (isClean)
    {
        SHU_LogWarning("Performing clean build...");
        SHU_CacheClearAll();
    }

    SHU_CompilerAddFlags(isDebug ? SHUM_FLAGS_DEBUG : SHUM_FLAGS_OPTIMIZATION_HIGH);
    SHU_CompilerAddFlags(SHUM_FLAGS_STANDARD_C99 " -w -DCGLM_STATIC");

#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#elif SHUM_HOST_PLATFORM == SHUM_PLATFORM_LINUX
    SHU_CompilerAddFlags("-D_GLFW_X11 -D_XOPEN_SOURCE=700 -D_GNU_SOURCE");
#endif

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo Dependencies"), argv[1], isDebug);

    SHU_ModuleBegin("cglm", "dependencies/cglm/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/");
    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("glfw", "dependencies/glfw/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/");
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

    SHU_CompilerAddFlags(SHUM_FLAGS_STANDARD_C2X);

    if (isDebug > 0)
    {
        SHU_CompilerAddFlags(SHUM_FLAGS_DEBUG SHUM_FLAGS_WARNING_ERROR);
        SHU_CompilerAddFlags(SHUM_FLAGS_WARNING_HIGH);
        SHU_CompilerAddFlags("-Wno-format-nonliteral -Wno-unused-function");

        if(SHU_CompilerGetIdentifier() == SHUM_COMPILER_CLANG)
        {
        	SHU_CompilerAddFlags("-Wno-gnu-zero-variadic-macro-arguments");
        }
    }
    else
    {
        SHU_CompilerAddFlags(SHUM_FLAGS_OPTIMIZATION_HIGH);
    }

    switch(isDebug)
    {
    case DEBUG_SANITIZE_ADDRESS:
    	SHU_CompilerAddFlags("-fsanitize=address");
        break;

    case DEBUG_SANITIZE_THREAD:
     	SHU_CompilerAddFlags("-fsanitize=thread");
        break;

    case DEBUG_SANITIZE_UNDEFINED:
        SHU_CompilerAddFlags("-fsanitize=undefined");
        break;

    case DEBUG_SANITIZE_MEMORY:
        SHU_CompilerAddFlags("-fsanitize=memory");
        break;
     }

    SHU_CompilerAddFlags("-DCGLM_STATIC");
#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#elif SHUM_HOST_PLATFORM == SHUM_PLATFORM_LINUX
    SHU_CompilerAddFlags("-D_GLFW_X11");
#endif

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo"), argv[1], isDebug);

    SHU_ModuleBegin("Code-Romeo", NULL);

    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddIncludeDirectory("dependencies/");
    SHU_ModuleAddIncludeDirectory("dependencies/cglm/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glad/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glfw/include/");

    SHU_ModuleAddSourceFile("src/");

    SHU_ModuleCompile(arcOutputDirectory, SHUM_MODULE_LIBRARY_STATIC);

    return 0;
}
