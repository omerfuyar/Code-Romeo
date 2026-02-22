#include "systems/Audio.h"

#include "tools/Resource.h"
#include "tools/Entity.h"

#include "utilities/ListArray.h"
#include "utilities/Maths.h"

#include "miniaudio/miniaudio.h"

#pragma region Source Only

struct AUDIO
{
    ma_engine engine;

    struct AUDIO_INFO
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size
        // todo use another data structure like stack or queue
    } info;

    struct AUDIO_DATA
    {
        Entity *entityMap;
        ma_sound *sounds;
    } data;

    struct AUDIO_LISTENER
    {
        Vector3 *positionReference;
        Vector3 *rotationReference;
    } listener;
} AUDIO = {0};

#define amsEntity(component) (AUDIO.data.entityMap[component])
#define amsSound(component) (AUDIO.data.sounds[component])

#define amsAssertComponent(component) RJ_DebugAssert(component < AUDIO.info.count + AUDIO.info.freeIndices.count && amsEntity(component) != RJ_INDEX_INVALID, "Audio component %u either exceeds maximum possible index %u or is invalid.", component, AUDIO.info.count + AUDIO.info.freeIndices.count)

#pragma endregion Source Only

RJ_ResultWarn Audio_Initialize(RJ_Size initialComponentCapacity)
{
    ma_result result = ma_engine_init(NULL, &AUDIO.engine);

    if (result != MA_SUCCESS)
    {
        RJ_DebugWarning("Failed to initialize miniaudio : %zu", result);
        return RJ_ERROR_DEPENDENCY;
    }

    AUDIO.info.capacity = initialComponentCapacity;
    AUDIO.info.count = 0;

    ListArray_Create(&AUDIO.info.freeIndices, "Audio Free Indices", sizeof(RJ_Size), ENTITY_INITIAL_FREE_INDEX_ARRAY_SIZE);

    RJ_ReturnAllocate(RJ_Size, AUDIO.data.entityMap, initialComponentCapacity,
                      ma_engine_uninit(&AUDIO.engine);
                      ListArray_Destroy(&AUDIO.info.freeIndices);

                      AUDIO.info.capacity = 0;
                      AUDIO.info.count = 0;);

    RJ_ReturnAllocate(ma_sound, AUDIO.data.sounds, initialComponentCapacity,
                      ma_engine_uninit(&AUDIO.engine);
                      ListArray_Destroy(&AUDIO.info.freeIndices);

                      AUDIO.info.capacity = 0;
                      AUDIO.info.count = 0;);

    memset(AUDIO.data.entityMap, 0xff, sizeof(RJ_Size) * initialComponentCapacity);

    RJ_DebugInfo("Audio system initialized with component capacity %u.", initialComponentCapacity);
    return RJ_OK;
}

void Audio_Terminate(void)
{
    ma_engine_uninit(&AUDIO.engine);

    AUDIO.info.capacity = 0;
    AUDIO.info.count = 0;
    ListArray_Destroy(&AUDIO.info.freeIndices);

    free(AUDIO.data.entityMap);
    free(AUDIO.data.sounds);

    AUDIO.data.entityMap = NULL;
    AUDIO.data.sounds = NULL;

    AUDIO.listener.positionReference = NULL;
    AUDIO.listener.rotationReference = NULL;

    RJ_DebugInfo("Audio system terminated successfully.");
}

void Audio_ConfigureListener(Vector3 *positionReference, Vector3 *rotationReference)
{
    RJ_DebugAssertNullPointerCheck(positionReference);
    RJ_DebugAssertNullPointerCheck(rotationReference);

    AUDIO.listener.positionReference = positionReference;
    AUDIO.listener.rotationReference = rotationReference;
}

RJ_Result Audio_Configure(RJ_Size newComponentCapacity)
{
    RJ_DebugAssert(newComponentCapacity > AUDIO.info.count, "New component capacity must be greater than current audio component count");

    AUDIO.info.capacity = newComponentCapacity;

    RJ_ReturnReallocate(RJ_Size, AUDIO.data.entityMap, AUDIO.info.capacity);
    RJ_ReturnReallocate(ma_sound, AUDIO.data.sounds, AUDIO.info.capacity);

    RJ_DebugInfo("Audio position references reconfigured with new capacity %u.", AUDIO.info.capacity);
    return RJ_OK;
}

void Audio_Update(void)
{
    for (RJ_Size component = 0; component < AUDIO.info.count; component++)
    {
        Vector3 componentPos = Entity_GetPosition(amsEntity(component));
        ma_sound_set_position((ma_sound *)&amsSound(component), componentPos.x, componentPos.y, componentPos.z);
    }

    ma_engine_listener_set_position(&AUDIO.engine, 0, AUDIO.listener.positionReference->x, AUDIO.listener.positionReference->y, AUDIO.listener.positionReference->z);
    ma_engine_listener_set_direction(&AUDIO.engine, 0, AUDIO.listener.rotationReference->x, AUDIO.listener.rotationReference->y, AUDIO.listener.rotationReference->z); // todo forward rotation
}

RJ_ResultWarn Audio_ComponentCreate(RJ_Size entity, AudioComponent *retComponent, StringView audioFile)
{
    RJ_DebugAssert(AUDIO.info.count + AUDIO.info.freeIndices.count < AUDIO.info.capacity, "Maximum audio component capacity of %u reached.", AUDIO.info.capacity);

    AudioComponent newComponent = AUDIO.info.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&AUDIO.info.freeIndices)) : AUDIO.info.count;

    amsEntity(newComponent) = entity;

    *retComponent = newComponent;

    String fullPath = scc(audioFile);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));

    ma_result result = ma_sound_init_from_file(
        &AUDIO.engine,
        fullPath.characters,
        0,
        NULL,
        NULL,
        (ma_sound *)&amsSound(newComponent));

    if (result != MA_SUCCESS)
    {
        ListArray_Add(&AUDIO.info.freeIndices, &newComponent);
        amsEntity(newComponent) = RJ_INDEX_INVALID;
        RJ_DebugWarning("Failed to create audio component : %zu", result);
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    AUDIO.info.count++;

    return RJ_OK;
}

void Audio_ComponentDestroy(AudioComponent component)
{
    amsAssertComponent(component);

    ListArray_Add(&AUDIO.info.freeIndices, &component);

    amsEntity(component) = RJ_INDEX_INVALID;
    ma_sound_uninit((ma_sound *)&amsSound(component));

    AUDIO.info.count--;
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

    ma_uint64 targetFrame = (ma_uint64)((float)totalFrames * Maths_Clamp(interval, 0.0f, 1.0f));

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
