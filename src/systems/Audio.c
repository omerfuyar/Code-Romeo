#include "systems/Audio.h"

#include "utilities/ListArray.h"

#include "tools/Resource.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#pragma region Source Only

typedef struct AudioClip
{
    size_t index;
    ma_sound data;
} AudioClip;

/*
typedef struct AudioListener
{
    const Vector3 *positionReference;
    const Vector3 *rotationReference;
    float maxDistance;
    float rolloffFactor;
} AudioListener;
*/

ma_engine AUDIO_MAIN_ENGINE;
ListArray AUDIO_MAIN_CLIPS; // AudioClip
// AudioListener AUDIO_MAIN_LISTENER = {0};

#pragma endregion Source Only

#pragma region Audio

void Audio_Initialize(size_t initialSoundCapacity)
{
    ma_result result = ma_engine_init(NULL, &AUDIO_MAIN_ENGINE);
    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to initialize miniaudio : %zu", result);

    AUDIO_MAIN_CLIPS = ListArray_Create("ma_sound", sizeof(AudioClip), initialSoundCapacity);
}

void Audio_Terminate()
{
    ma_engine_uninit(&AUDIO_MAIN_ENGINE);
    ListArray_Destroy(&AUDIO_MAIN_CLIPS);
}

/*
void Audio_ConfigureListener(const Vector3 *positionReference, const Vector3 *rotationReference, float maxDistance, float rolloffFactor)
{
    AUDIO_MAIN_LISTENER.positionReference = positionReference;
    AUDIO_MAIN_LISTENER.rotationReference = rotationReference;
    AUDIO_MAIN_LISTENER.maxDistance = maxDistance;
    AUDIO_MAIN_LISTENER.rolloffFactor = rolloffFactor;
}
*/

void Audio_PlayClip(AudioClip *clip /*, const Vector3 *position*/)
{
    ma_sound_start(&clip->data);
}

#pragma endregion Audio

#pragma region AudioClip

AudioClip *AudioClip_Create(StringView audioFile)
{
    AudioClip *clip = (AudioClip *)ListArray_Add(&AUDIO_MAIN_CLIPS, NULL);
    clip->index = AUDIO_MAIN_CLIPS.count - 1;

    String fullPath = scc(audioFile);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    ma_result result = ma_sound_init_from_file(&AUDIO_MAIN_ENGINE, fullPath.characters, 0, NULL, NULL, &clip->data);
    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to create AudioClip '%s' : %d", fullPath.characters, result);

    String_Destroy(&fullPath);

    return clip;
}

void AudioClip_Destroy(AudioClip *clip)
{
    RJGlobal_DebugAssertNullPointerCheck(clip);

    for (size_t i = clip->index + 1; i < AUDIO_MAIN_CLIPS.count - clip->index; i++)
    {
        AudioClip *nextClip = (AudioClip *)ListArray_Get(&AUDIO_MAIN_CLIPS, i);
        nextClip->index--;
    }

    ma_sound_uninit(&clip->data);
}

#pragma endregion AudioClip
