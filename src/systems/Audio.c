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
    AudioListener listener;

    struct AUDIO_DATA
    {
        RJ_Size capacity;
        RJ_Size count;

        Entity *entityToCompMap; // sparse, accessing the component from entity, to access internal component data etc.
        Entity *compToEntityMap; // dense, accessing entity from component, access entity data like positions etc.

        ma_sound *sounds; // dense, indexed by component, the actual miniaudio sound objects
    } data;
} AUDIO = {0};

#define aComponent(entity) (AUDIO.data.entityToCompMap[entity])
#define aEntity(component) (AUDIO.data.compToEntityMap[component])

#define aSound(component) (AUDIO.data.sounds[component])

#define aAssertEntity(entity) RJ_DebugAssert((entity) != RJ_INDEX_INVALID &&                                                            \
                                                 aComponent(entity) != RJ_INDEX_INVALID &&                                              \
                                                 aEntity(aComponent(entity)) == entity &&                                               \
                                                 aComponent(entity) < AUDIO.data.count,                                                 \
                                             "Audio component %u or Entity %u either exceeds maximum possible index %u or is invalid.", \
                                             aComponent(entity), entity, AUDIO.data.count)

#pragma endregion Source Only

RJ_ResultWarn Audio_Initialize(RJ_Size initialComponentCapacity)
{
    ma_result result = ma_engine_init(NULL, &AUDIO.engine);

    if (result != MA_SUCCESS)
    {
        RJ_DebugWarning("Failed to initialize miniaudio : %zu", result);
        return RJ_ERROR_DEPENDENCY;
    }

    AUDIO.data.capacity = initialComponentCapacity;
    AUDIO.data.count = 0;

    RJ_Size entityCapacity = 0;
    Entity_GetInternalData(&entityCapacity, NULL);

    RJ_ReturnAllocate(RJ_Size, AUDIO.data.entityToCompMap, entityCapacity,
                      ma_engine_uninit(&AUDIO.engine);

                      AUDIO.data.capacity = 0;
                      AUDIO.data.count = 0;);

    RJ_ReturnAllocate(Entity, AUDIO.data.compToEntityMap, initialComponentCapacity,
                      ma_engine_uninit(&AUDIO.engine);
                      free(AUDIO.data.entityToCompMap);

                      AUDIO.data.capacity = 0;
                      AUDIO.data.count = 0;);

    RJ_ReturnAllocate(ma_sound, AUDIO.data.sounds, initialComponentCapacity,
                      ma_engine_uninit(&AUDIO.engine);
                      free(AUDIO.data.entityToCompMap);
                      free(AUDIO.data.compToEntityMap);

                      AUDIO.data.capacity = 0;
                      AUDIO.data.count = 0;);

    memset(AUDIO.data.entityToCompMap, 0xff, sizeof(Entity) * entityCapacity);
    memset(AUDIO.data.compToEntityMap, 0xff, sizeof(Entity) * initialComponentCapacity);

    RJ_DebugInfo("Audio system initialized with component capacity %u.", initialComponentCapacity);
    return RJ_OK;
}

void Audio_Terminate(void)
{
    for (RJ_Size component = AUDIO.data.count; component > 0; component--)
    {
        ma_sound_uninit(&AUDIO.data.sounds[component - 1]);
    }

    ma_engine_uninit(&AUDIO.engine);

    free(AUDIO.data.entityToCompMap);
    free(AUDIO.data.compToEntityMap);
    free(AUDIO.data.sounds);

    memset(&AUDIO, 0, sizeof(AUDIO));

    RJ_DebugInfo("Audio system terminated successfully.");
}

bool Audio_IsInitialized(void)
{
    return AUDIO.data.capacity > 0;
}

const AudioListener *Audio_GetListenerData(void)
{
    return &AUDIO.listener;
}

void Audio_SetListenerData(const AudioListener *listenerData)
{
    AUDIO.listener = *listenerData;
}

/*
!RJ_ResultWarn Audio_Resize(RJ_Size newComponentCapacity)
{
    RJ_DebugAssert(newComponentCapacity > AUDIO.data.count, "New component capacity must be greater than current audio component count");

    RJ_ReturnReallocate(RJ_Size, AUDIO.data.entityToCompMap, AUDIO.data.capacity); // todo proper failing

    RJ_ReturnReallocate(Entity, AUDIO.data.compToEntityMap, AUDIO.data.capacity,
                        free(AUDIO.data.entityToCompMap););

    RJ_ReturnReallocate(ma_sound, AUDIO.data.sounds, AUDIO.data.capacity,
                        free(AUDIO.data.compToEntityMap);
                        free(AUDIO.data.entityToCompMap););

    AUDIO.data.capacity = newComponentCapacity;

    RJ_DebugInfo("Audio position references reconfigured with new capacity %u.", AUDIO.data.capacity);
    return RJ_OK;
}
*/

void Audio_Update(void)
{
    for (Entity component = 0; component < AUDIO.data.count; component++)
    {
        Vector3 componentPos = Entity_GetPosition(aEntity(component));
        ma_sound_set_position(&aSound(component), componentPos.x, componentPos.y, componentPos.z);
    }

    ma_engine_listener_set_position(&AUDIO.engine, 0, AUDIO.listener.position.x, AUDIO.listener.position.y, AUDIO.listener.position.z);
    ma_engine_listener_set_direction(&AUDIO.engine, 0, AUDIO.listener.rotation.x, AUDIO.listener.rotation.y, AUDIO.listener.rotation.z); // todo forward rotation
}

RJ_ResultWarn Audio_ComponentCreate(Entity entity, StringView audioFile)
{
    RJ_DebugAssert(AUDIO.data.count < AUDIO.data.capacity, "Maximum audio component capacity of %u reached.", AUDIO.data.capacity);

    Entity component = AUDIO.data.count;

    aComponent(entity) = component;
    aEntity(component) = entity;

    String fullPath = scc(audioFile);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));

    ma_result result = ma_sound_init_from_file(
        &AUDIO.engine,
        fullPath.characters,
        0,
        NULL,
        NULL,
        &aSound(aComponent(entity)));

    if (result != MA_SUCCESS)
    {
        aComponent(entity) = RJ_INDEX_INVALID;
        aEntity(component) = RJ_INDEX_INVALID;
        RJ_DebugWarning("Failed to create audio component : %zu", result);
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    AUDIO.data.count++;

    return RJ_OK;
}

void Audio_ComponentDestroy(Entity entity)
{
    aAssertEntity(entity);

    ma_sound_uninit(&aSound(aComponent(entity)));

    // todo swap with last component
    aEntity(aComponent(entity)) = RJ_INDEX_INVALID;
    aComponent(entity) = RJ_INDEX_INVALID;

    AUDIO.data.count--;
}

void Audio_ComponentRewind(Entity entity, float interval)
{
    aAssertEntity(entity);

    ma_uint64 totalFrames = 0;

    ma_sound_get_length_in_pcm_frames(&aSound(aComponent(entity)), &totalFrames);

    ma_uint64 targetFrame = (ma_uint64)((float)totalFrames * Maths_Clamp(interval, 0.0f, 1.0f));

    ma_sound_seek_to_pcm_frame(&aSound(aComponent(entity)), targetFrame);
}

bool Audio_ComponentIsPlaying(Entity entity)
{
    aAssertEntity(entity);

    return ma_sound_is_playing(&aSound(aComponent(entity)));
}

void Audio_ComponentSetPlaying(Entity entity, bool play)
{
    aAssertEntity(entity);

    if (play)
    {
        ma_sound_start(&aSound(aComponent(entity)));
    }
    else
    {
        ma_sound_stop(&aSound(aComponent(entity)));
    }
}

bool Audio_ComponentIsLooping(Entity entity)
{
    aAssertEntity(entity);

    return ma_sound_is_looping(&aSound(aComponent(entity)));
}

void Audio_ComponentSetLooping(Entity entity, bool loop)
{
    aAssertEntity(entity);

    ma_sound_set_looping(&aSound(aComponent(entity)), loop);
}
