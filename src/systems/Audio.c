#include "systems/Audio.h"

#include "utilities/ListArray.h"
#include "utilities/Maths.h"

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

void Audio_Initialize()
{
    ma_result result = ma_engine_init(NULL, &AUDIO_MAIN_ENGINE);
    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to initialize miniaudio : %zu", result);
}

void Audio_Terminate()
{
    ma_engine_uninit(&AUDIO_MAIN_ENGINE);
}

#pragma endregion Audio

#pragma region AudioScene

AudioScene *AudioScene_Create(StringView name, RJGlobal_Size initialComponentCapacity)
{
    AudioScene *scene = NULL;
    RJGlobal_DebugAssertAllocationCheck(AudioScene, scene, 1);

    scene->name = scc(name);
    scene->listener = NULL;
    scene->components = ListArray_Create("AudioComponent", sizeof(AudioComponent), initialComponentCapacity);

    return scene;
}

void AudioScene_Destroy(AudioScene *scene)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    String_Destroy(&scene->name);
    ListArray_Destroy(&scene->components);

    free(scene);
}

void AudioScene_SetMainListener(AudioScene *scene, AudioListenerComponent *listener)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);
    RJGlobal_DebugAssertNullPointerCheck(listener);

    scene->listener = listener;
}

AudioComponent *AudioScene_CreateComponent(AudioScene *scene, StringView file, Vector3 *positionReference)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);
    RJGlobal_DebugAssertNullPointerCheck(positionReference);

    AudioComponent component = {0};
    component.scene = scene;
    component.componentOffsetInScene = scene->components.count;
    component.positionReference = positionReference;

    String fullPath = scc(file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJGlobal_GetExecutablePath()));

    RJGlobal_DebugAssertAllocationCheck(ma_sound, component.data, 1);

    ma_result result = ma_sound_init_from_file(
        &AUDIO_MAIN_ENGINE,
        fullPath.characters,
        0,
        NULL,
        NULL,
        (ma_sound *)component.data);

    RJGlobal_DebugAssert(result == MA_SUCCESS, "Failed to create AudioComponent for '%s' : %d", fullPath.characters, result);

    String_Destroy(&fullPath);

    return (AudioComponent *)ListArray_Add(&scene->components, &component);
}

AudioListenerComponent *AudioScene_CreateListenerComponent(AudioScene *scene, Vector3 *positionReference, Vector3 *rotationReference)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    AudioListenerComponent *listener = NULL;
    RJGlobal_DebugAssertAllocationCheck(AudioListenerComponent, listener, 1);

    listener->positionReference = positionReference;
    listener->rotationReference = rotationReference;
    listener->scene = scene;
    scene->listener = listener;

    return listener;
}

#pragma endregion AudioScene

#pragma region AudioComponent

void AudioComponent_Update(AudioComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);
    ma_sound_set_position((ma_sound *)component->data, component->positionReference->x, component->positionReference->y, component->positionReference->z);
}

void AudioComponent_Play(AudioComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);
    ma_sound_start((ma_sound *)component->data);
}

void AudioComponent_Pause(AudioComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);
    ma_sound_stop((ma_sound *)component->data);
}

bool AudioComponent_IsPlaying(AudioComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);
    return ma_sound_is_playing((ma_sound *)component->data);
}

void AudioComponent_Rewind(AudioComponent *component, float interval)
{
    RJGlobal_DebugAssertNullPointerCheck(component);

    ma_uint64 totalFrames = 0;

    ma_sound_get_length_in_pcm_frames((ma_sound *)component->data, &totalFrames);

    ma_uint64 targetFrame = (ma_uint64)(totalFrames * Maths_Clamp(interval, 0.0f, 1.0f));

    ma_sound_seek_to_pcm_frame((ma_sound *)component->data, targetFrame);
}

void AudioComponent_SetLooping(AudioComponent *component, bool loop)
{
    RJGlobal_DebugAssertNullPointerCheck(component);
    ma_sound_set_looping((ma_sound *)component->data, loop);
}

#pragma endregion AudioComponent

#pragma region AudioListenerComponent

void AudioListenerComponent_Update(AudioListenerComponent *listener)
{
    RJGlobal_DebugAssertNullPointerCheck(listener);

    ma_engine_listener_set_position(&AUDIO_MAIN_ENGINE, 0, listener->positionReference->x, listener->positionReference->y, listener->positionReference->z);
    ma_engine_listener_set_direction(&AUDIO_MAIN_ENGINE, 0, listener->rotationReference->x, listener->rotationReference->y, listener->rotationReference->z); // todo forward rotation
}

#pragma endregion AudioListenerComponent
