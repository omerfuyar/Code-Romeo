#include "systems/Audio.h"

#include "utilities/ListArray.h"

#include "tools/Resource.h"

#define MINIAUDIO_IMPLEMENTATION

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#if RJGLOBAL_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJGLOBAL_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJGLOBAL_COMPILER_MSVC
#pragma warning(pop)
#endif

#pragma region Source Only

ma_engine AUDIO_MAIN_ENGINE = {0};

#pragma endregion Source Only

#pragma region Audio

void Audio_Initialize(size_t initialSoundCapacity)
{
    ma_result result = ma_engine_init(NULL, &AUDIO_MAIN_ENGINE);
    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to initialize miniaudio : %zu", result);
}

void Audio_Terminate()
{
    ma_engine_uninit(&AUDIO_MAIN_ENGINE);
}

#pragma endregion Audio

#pragma region AudioClip

AudioClip *AudioClip_Create(StringView audioFile)
{
    AudioClip *clip = (AudioClip *)malloc(sizeof(ma_sound));

    String fullPath = scc(audioFile);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    // todo ma_sound use case is different
    ma_result result = ma_sound_init_from_file(&AUDIO_MAIN_ENGINE, fullPath.characters, 0, NULL, NULL, &clip->data);
    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to create AudioClip '%s' : %d", fullPath.characters, result);

    String_Destroy(&fullPath);

    return clip;
}

void AudioClip_Destroy(AudioClip *clip)
{
    RJGlobal_DebugAssertNullPointerCheck(clip);

    String_Destroy(&clip->name);

    ma_sound_uninit(&clip->data);
    clip->data = NULL;

    free(clip);
}

#pragma endregion AudioClip

#pragma region AudioScene

AudioScene *AudioScene_Create(StringView name, size_t initialComponentCapacity)
{
}

void AudioScene_Destroy(AudioScene *scene)
{
}

AudioComponent *AudioScene_CreateComponent()
{
}

void AudioScene_DestroyComponent(AudioComponent *component)
{
}

#pragma endregion AudioScene
