#pragma once

#include "RJGlobal.h"

#include "tools/Entity.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

typedef struct AudioListener
{
    Vector3 position;
    Vector3 rotation;

    //? float range;
    //? float sensitivity;
} AudioListener;

/// @brief Initialize the audio system with the specified component capacity.
/// @param initialComponentCapacity The initial capacity for audio components.
/// @return RJ_OK / RJ_ERROR_DEPENDENCY / RJ_ERROR_ALLOCATION
RJ_ResultWarn Audio_Initialize(RJ_Size initialComponentCapacity);

/// @brief Terminate and free the necessary resources for audio system.
void Audio_Terminate(void);

/// @brief Gets the internal listener information.
/// @return Pointer to the internal listener data, safe to read, should not be written.
const AudioListener *Audio_GetListenerData(void);

void Audio_SetListenerData(const AudioListener *listenerData);

/// @brief Reconfigure the audio system's position references and component capacity.
/// @param newComponentCapacity The new capacity for audio components.
//! RJ_ResultWarn Audio_Resize(RJ_Size newComponentCapacity);

/// @brief Update the audio system. Should be called every frame.
void Audio_Update(void);

/// @brief Creates an audio component and associates it with an entity.
/// @param entity The entity to associate the component with.
/// @param audioFile The audio file to be used by the component.
/// @return RJ_OK / RJ_ERROR_DEPENDENCY
RJ_ResultWarn Audio_ComponentCreate(Entity entity, StringView audioFile);
// todo maybe add sound files to resource and make reference counted resources

/// @brief Destroys an audio component and frees its resources.
/// @param entity Component to destroy.
void Audio_ComponentDestroy(Entity entity);

/// @brief Rewinds the audio by moving its cursor.
/// @param entity Component to rewind.
/// @param interval Between 0 and 1. 0 is the start and 1 is the end. Clamps to 0-1 internally.
void Audio_ComponentRewind(Entity entity, float interval);

/// @brief Gets the audio of the component currently playing or not.
/// @param entity Component to check.
/// @return True if the audio is playing, false otherwise.
bool Audio_ComponentIsPlaying(Entity entity);

/// @brief Sets the playing state of the audio component.
/// @param entity Component to play or stop.
/// @param play Plays the audio if true, pause otherwise.
void Audio_ComponentSetPlaying(Entity entity, bool play);

/// @brief Checks if the component is set to loop the audio.
/// @param entity Component to check.
/// @return True if the component is set to loop the audio, false otherwise.
bool Audio_ComponentIsLooping(Entity entity);

/// @brief Configures the component to loop its audio or not.
/// @param entity Component to loop or unloop.
/// @param loop Loops the audio if true, unloop otherwise.
void Audio_ComponentSetLooping(Entity entity, bool loop);
