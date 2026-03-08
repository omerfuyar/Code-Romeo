#define SHUC_NO_RUN_LOG
#define SHUC_MAX_COMMAND_BUFFER_SIZE 8192
#define SHUC_SHORT_LOG
#define SHUC_ENABLE_INCREMENTAL
#define SHUILD_IMPLEMENTATION
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
        SHU_LogError(1, "Usage is <compiler> <r/d/dsa/dst/dsu/dsm> [dynamic] [clean]");
    }

    char isDebug = 0;
	char isClean = 0;
	char isDynamic = 0;

	const char *const compilerStr = argv[1];
	const char *const buildOptStr = argv[2];
	const char *buildName = NULL;

    if(strcmp(buildOptStr, "r") == 0)
    {
        isDebug = 0;
		buildName = "release";
    }
    else if(strcmp(buildOptStr, "d") == 0)
    {
    	isDebug = DEBUG_REGULAR;
		buildName = "debug";
    }
    else if(strcmp(buildOptStr, "dsa") == 0)
    {
		isDebug = DEBUG_SANITIZE_ADDRESS;
		buildName = "debug_sanitize_address";
    }
    else if(strcmp(buildOptStr, "dst") == 0)
    {
		isDebug = DEBUG_SANITIZE_THREAD;
		buildName = "debug_sanitize_thread";
    }
    else if(strcmp(buildOptStr, "dsu") == 0)
    {
		isDebug = DEBUG_SANITIZE_UNDEFINED;
		buildName = "debug_sanitize_undefined";
    }
    else if(strcmp(buildOptStr, "dsm") == 0)
    {
		isDebug = DEBUG_SANITIZE_MEMORY;
		buildName = "debug_sanitize_memory";
    }
    else
    {
        SHU_LogError(1, "Unknown argument '%s', Specify debug or release build with second parameter <r/d/dsa/dst/dsu/dsm>.", buildOptStr);
    }

	if(isDebug > DEBUG_REGULAR && strcmp(compilerStr, "clang") != 0)
	{
		SHU_LogError(1, "Sanitizers can only be used with Clang compiler.");
	}

    for(int i = 3; i < argc; i++)
    {
		const char *const optionalArg = argv[i];

		if(isClean == 0 && strcmp(optionalArg, "clean") == 0)
		{
			isClean = 1;
		}
		else if(isDynamic == 0 && strcmp(optionalArg, "dynamic") == 0)
		{
			isDynamic = 1;
		}
		else
		{
			SHU_LogError(1, "Unknown argument '%s', try [dynamic] [clean].", optionalArg);
		}
    }

    SHU_CompilerTryConfigure(compilerStr);
    SHU_UtilAutomate(argc, argv);

	char strBuffer[SHUC_MAX_STRING_SIZE] = {0};
	snprintf(strBuffer, SHUC_MAX_STRING_SIZE, ".shu/%s/", buildName);
    SHU_CacheConfigure(strBuffer);

    if (isClean)
    {
        SHU_LogWarning("Performing clean build...");
        SHU_CacheClearAll();
    }

 	if (isDebug > 0 && SHU_CompilerGetIdentifier() == SHUM_COMPILER_CLANG)
    {
    	SHU_CompilerAddFlags("-fno-omit-frame-pointer");
    }

    switch(isDebug)
    {
    case DEBUG_SANITIZE_ADDRESS:
    	SHU_CompilerAddFlags("-fsanitize=address,leak");
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

    SHU_CompilerAddFlags(SHUM_FLAGS_STANDARD_C99 " -w");
    SHU_CompilerAddFlags(isDebug ? SHUM_FLAGS_DEBUG SHUM_FLAGS_OPTIMIZATION_DEBUG : SHUM_FLAGS_OPTIMIZATION_HIGH);
	SHU_CompilerAddFlags(isDynamic ? "" : " -DCGLM_STATIC");

#if SHUM_HOST_PLATFORM == SHUM_PLATFORM_WINDOWS
    SHU_CompilerAddFlags("-D_GLFW_WIN32");
#elif SHUM_HOST_PLATFORM == SHUM_PLATFORM_LINUX
    SHU_CompilerAddFlags("-D_GLFW_X11 -D_XOPEN_SOURCE=700 -D_GNU_SOURCE");
#endif

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo Dependencies"), compilerStr, isDebug);

	snprintf(strBuffer, SHUC_MAX_STRING_SIZE, "build/%s/", buildName);

    SHU_ModuleBegin("cglm", "dependencies/cglm/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/");
    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("glfw", "dependencies/glfw/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/");
    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("glad", "dependencies/glad/");
    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddSourceFile("src/glad.c");
    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("miniaudio", "dependencies/miniaudio/");
    SHU_ModuleAddSourceFile("miniaudio.c");
    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    SHU_ModuleBegin("stb", "dependencies/stb/");
    SHU_ModuleAddSourceFile("stb.c");
    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    SHU_CompilerAddFlags(SHUM_FLAGS_STANDARD_C2X);

    if (isDebug > 0)
    {
        SHU_CompilerAddFlags(SHUM_FLAGS_WARNING_ERROR SHUM_FLAGS_WARNING_HIGH);
        SHU_CompilerAddFlags("-Wno-format-nonliteral -Wno-unused-function");

        if(SHU_CompilerGetIdentifier() == SHUM_COMPILER_CLANG)
        {
        	SHU_CompilerAddFlags("-Wno-gnu-zero-variadic-macro-arguments");
        }
    }

    ShowBuildConfig(SHUM_COLOR_BLUE("Romeo"), compilerStr, isDebug);

    SHU_ModuleBegin("Code-Romeo", NULL);

    SHU_ModuleAddIncludeDirectory("include/");
    SHU_ModuleAddIncludeDirectory("dependencies/");
    SHU_ModuleAddIncludeDirectory("dependencies/cglm/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glad/include/");
    SHU_ModuleAddIncludeDirectory("dependencies/glfw/include/");

    SHU_ModuleAddSourceFile("src/");

    SHU_ModuleCompile(strBuffer, isDynamic ? SHUM_MODULE_LIBRARY_DYNAMIC : SHUM_MODULE_LIBRARY_STATIC);

    return 0;
}
