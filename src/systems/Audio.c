#include "systems/Audio.h"

#include "utilities/ListArray.h"
#include "utilities/Maths.h"

#include "tools/Resource.h"

#define MINIAUDIO_IMPLEMENTATION

#if RJ_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#elif RJ_COMPILER_MSVC
#pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#if RJ_COMPILER_CLANG
#pragma clang diagnostic pop
#elif RJ_COMPILER_GCC
#pragma GCC diagnostic pop
#elif RJ_COMPILER_MSVC
#pragma warning(pop)
#endif

#define AUDIO_FLAG_ACTIVE (1 << 0)

#pragma region Source Only

struct AUDIO_MAIN_SCENE
{
    ma_engine engine;

    struct AUDIO_SCENE_DATA
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size
    } data;

    struct AUDIO_COMPONENTS
    {
        RJ_Size *entities;

        ma_sound *sounds;
        Vector3 *positionReferences;

        uint8_t *flags;
    } components;

    struct AUDIO_LISTENER
    {
        Vector3 *positionReference;
        Vector3 *rotationReference;
    } listener;
} AMS = {0};

#define amsEntity(component) (AMS.components.entities[component])
#define amsSound(component) (AMS.components.sounds[component])
#define amsPositionReference(component) (AMS.components.positionReferences[amsEntity(component)])
#define amsFlag(component) (AMS.components.flags[component])

#define amsIsActive(component) (amsFlag(component) & AUDIO_FLAG_ACTIVE)
#define amsSetActive(component, isActive) (amsFlag(component) = isActive ? (amsFlag(component) | AUDIO_FLAG_ACTIVE) : (amsFlag(component) & ~AUDIO_FLAG_ACTIVE))

#define amsAssertComponent(component) RJ_DebugAssert(component < AMS.data.count + AMS.data.freeIndices.count && amsEntity(component) != RJ_INDEX_INVALID && amsIsActive(component), "Audio component %u either exceeds maximum possible index %u, invalid or inactive.", component, AMS.data.count + AMS.data.freeIndices.count)

#pragma endregion Source Only

RJ_Result Audio_Initialize(RJ_Size initialComponentCapacity, Vector3 *positionReferences)
{
    RJ_DebugAssertNullPointerCheck(positionReferences);

    ma_result result = ma_engine_init(NULL, &AMS.engine);

    if (result != MA_SUCCESS)
    {
        RJ_DebugWarning("Failed to initialize miniaudio : %zu", result);
        return RJ_ERROR_DEPENDENCY;
    }

    AMS.data.capacity = initialComponentCapacity;
    AMS.data.count = 0;

    ListArray_Create(&AMS.data.freeIndices, "Audio Free Indices", sizeof(RJ_Size), AUDIO_INITIAL_FREE_INDEX_ARRAY_SIZE);

    AMS.components.positionReferences = positionReferences;

    if (!RJ_Allocate(RJ_Size, AMS.components.entities, initialComponentCapacity) == false ||
        !RJ_Allocate(ma_sound, AMS.components.sounds, initialComponentCapacity) == false ||
        !RJ_Allocate(uint8_t, AMS.components.flags, initialComponentCapacity) == false)
    {
        ma_engine_uninit(&AMS.engine);
        ListArray_Destroy(&AMS.data.freeIndices);

        AMS.data.capacity = 0;
        AMS.data.count = 0;
        AMS.components.positionReferences = NULL;

        RJ_DebugWarning("Failed to allocate data for audio module with size %zu.", initialComponentCapacity * (sizeof(RJ_Size) + sizeof(ma_sound) + sizeof(uint8_t)));
        return RJ_ERROR_ALLOCATION;
    }

    RJ_DebugInfo("Audio system initialized with component capacity %u.", initialComponentCapacity);
    return RJ_OK;
}

void Audio_Terminate(void)
{
    ma_engine_uninit(&AMS.engine);

    AMS.data.capacity = 0;
    AMS.data.count = 0;
    ListArray_Destroy(&AMS.data.freeIndices);

    free(AMS.components.entities);
    free(AMS.components.sounds);
    free(AMS.components.flags);

    AMS.components.entities = NULL;
    AMS.components.sounds = NULL;
    AMS.components.flags = NULL;
    AMS.components.positionReferences = NULL;

    AMS.listener.positionReference = NULL;
    AMS.listener.rotationReference = NULL;

    RJ_DebugInfo("Audio system terminated successfully.");
}

void Audio_ConfigureListener(Vector3 *positionReference, Vector3 *rotationReference)
{
    RJ_DebugAssertNullPointerCheck(positionReference);
    RJ_DebugAssertNullPointerCheck(rotationReference);

    AMS.listener.positionReference = positionReference;
    AMS.listener.rotationReference = rotationReference;
}

void Audio_ConfigureReferences(Vector3 *positionReferences, RJ_Size newComponentCapacity)
{
    RJ_DebugAssertNullPointerCheck(positionReferences);
    RJ_DebugAssert(newComponentCapacity > AMS.data.count, "New component capacity must be greater than current audio component count");

    AMS.components.positionReferences = positionReferences;
    AMS.data.capacity = newComponentCapacity;

    if (!RJ_Reallocate(RJ_Size, AMS.components.entities, AMS.data.capacity) ||
        !RJ_Reallocate(ma_sound, AMS.components.sounds, AMS.data.capacity) ||
        !RJ_Reallocate(uint8_t, AMS.components.flags, AMS.data.capacity))
    {
        RJ_DebugWarning("Failed to reallocate data for audio module with size %zu.", AMS.data.capacity * (sizeof(RJ_Size) + sizeof(ma_sound) + sizeof(uint8_t)));
        return RJ_ERROR_ALLOCATION;
    }

    RJ_DebugInfo("Audio position references reconfigured with new capacity %u.", AMS.data.capacity);
}

void Audio_Update(void)
{
    for (RJ_Size component = 0; component < AMS.data.count; component++)
    {
        if (!amsIsActive(component))
        {
            continue;
        }

        ma_sound_set_position((ma_sound *)&amsSound(component), amsPositionReference(component).x, amsPositionReference(component).y, amsPositionReference(component).z);
    }

    ma_engine_listener_set_position(&AMS.engine, 0, AMS.listener.positionReference->x, AMS.listener.positionReference->y, AMS.listener.positionReference->z);
    ma_engine_listener_set_direction(&AMS.engine, 0, AMS.listener.rotationReference->x, AMS.listener.rotationReference->y, AMS.listener.rotationReference->z); // todo forward rotation
}

RJ_Result Audio_ComponentCreate(AudioComponent *retComponent, RJ_Size entity, StringView audioFile)
{
    RJ_DebugAssert(AMS.data.count + AMS.data.freeIndices.count < AMS.data.capacity, "Maximum audio component capacity of %u reached.", AMS.data.capacity);

    AudioComponent newComponent = AMS.data.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&AMS.data.freeIndices)) : AMS.data.count;

    amsEntity(newComponent) = entity;
    amsSetActive(newComponent, true);

    *retComponent = newComponent;

    String fullPath = scc(audioFile);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));

    ma_result result = ma_sound_init_from_file(
        &AMS.engine,
        fullPath.characters,
        0,
        NULL,
        NULL,
        (ma_sound *)&amsSound(newComponent));

    if (result != MA_SUCCESS)
    {
        ListArray_Add(&AMS.data.freeIndices, &newComponent);
        amsEntity(newComponent) = RJ_INDEX_INVALID;
        amsFlag(newComponent) = false;
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    AMS.data.count++;

    return RJ_OK;
}

void Audio_ComponentDestroy(AudioComponent component)
{
    amsAssertComponent(component);

    ListArray_Add(&AMS.data.freeIndices, &component);

    amsEntity(component) = RJ_INDEX_INVALID;
    ma_sound_uninit((ma_sound *)&amsSound(component));
    amsFlag(component) = false;

    AMS.data.count--;
}

bool Audio_ComponentIsActive(AudioComponent component)
{
    amsAssertComponent(component);

    return amsIsActive(component);
}

void Audio_ComponentSetActive(AudioComponent component, bool isActive)
{
    amsAssertComponent(component);

    amsSetActive(component, isActive);
}

bool Audio_ComponentIsPlaying(AudioComponent component)
{
    amsAssertComponent(component);

    return ma_sound_is_playing((ma_sound *)&amsSound(component));
}

void Audio_ComponentSetPlaying(AudioComponent component, bool play)
{
    amsAssertComponent(component);

    if (play)
    {
        ma_sound_start((ma_sound *)&amsSound(component));
    }
    else
    {
        ma_sound_stop((ma_sound *)&amsSound(component));
    }
}

void Audio_ComponentRewind(AudioComponent component, float interval)
{
    amsAssertComponent(component);

    ma_uint64 totalFrames = 0;

    ma_sound_get_length_in_pcm_frames((ma_sound *)&amsSound(component), &totalFrames);

    ma_uint64 targetFrame = (ma_uint64)(totalFrames * Maths_Clamp(interval, 0.0f, 1.0f));

    ma_sound_seek_to_pcm_frame((ma_sound *)&amsSound(component), targetFrame);
}

bool Audio_ComponentIsLooping(AudioComponent component)
{
    amsAssertComponent(component);

    return ma_sound_is_looping((ma_sound *)&amsSound(component));
}

void Audio_ComponentSetLooping(AudioComponent component, bool loop)
{
    amsAssertComponent(component);

    ma_sound_set_looping((ma_sound *)&amsSound(component), loop);
}
