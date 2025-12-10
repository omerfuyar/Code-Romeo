#define SHUILD_IMPLEMENTATION
#include "../dependencies/shuild/shuild.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        goto usageError;
    }

    SHU_CompilerTryConfigure(argv[1]);
    SHU_Automate(argc, argv);

    return 0;

usageError:
    SHU_LogError(1, "Usage is <compiler>");
}