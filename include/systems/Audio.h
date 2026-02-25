#pragma once

#include "RJGlobal.h"

#include "tools/Entity.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

/// @brief Initialize the audio system with the specified component capacity.
/// @param initialComponentCapacity The initial capacity for audio components.
/// @return RJ_OK on success or RJ_ERROR_DEPENDENCY if miniaudio fails or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Audio_Initialize(RJ_Size initialComponentCapacity);

/// @brief Terminate and free the necessary resources for audio system.
void Audio_Terminate(void);

/// @brief Configure the audio listener's position and rotation references.
/// @param positionReference Reference to the listener's position.
/// @param rotationReference Reference to the listener's rotation.
void Audio_ConfigureListener(Vector3 *positionReference, Vector3 *rotationReference);

/// @brief Reconfigure the audio system's position references and component capacity.
/// @param newComponentCapacity The new capacity for audio components.
RJ_ResultWarn Audio_Configure(RJ_Size newComponentCapacity);

/// @brief Update the audio system.
void Audio_Update(void);

/// @brief Creates an audio component and associates it with an entity.
/// @param retComponent Pointer to store the created AudioComponent.
/// @param entity The entity to associate the component with.
/// @param audioFile The audio file to be used by the component.
/// @param positionReference Reference to the position of the audio component.
/// @return RJ_OK on success or RJ_ERROR_DEPENDENCY if miniaudio fails.
RJ_ResultWarn Audio_ComponentCreate(Entity entity, StringView audioFile);

/// @brief Destroys an audio component and frees its resources.
/// @param component Component to destroy.
void Audio_ComponentDestroy(Entity component);

/// @brief Is the audio of the component currently playing or not.
/// @param component Component to check.
/// @return True if the audio is playing, false if not.
bool Audio_ComponentIsPlaying(Entity component);

/// @brief Sets the playing state of the audio component.
/// @param component Component to check.
/// @param play Play the audio if true, pause if false.
void Audio_ComponentSetPlaying(Entity component, bool play);

/// @brief Rewinds the audio by moving its cursor.
/// @param batch Audio batch the component is in.
/// @param component Component to rewind.
/// @param interval Between 0 and 1. 0 is the start and 1 is the end. Clamps to 0-1 internally.
void Audio_ComponentRewind(Entity component, float interval);

/// @brief Checks if the component is set to loop the audio.
/// @param component Component to check.
/// @return True if the component is set to loop the audio, false otherwise.
bool Audio_ComponentIsLooping(Entity component);

/// @brief Configures the component to loop the audio not.
/// @param component Component to configure
/// @param loop Loop or not.
void Audio_ComponentSetLooping(Entity component, bool loop);
