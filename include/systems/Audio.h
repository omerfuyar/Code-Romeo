#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

// typedef uint32_t AudioComponent;

/// @brief Capacity of free indices array of the audio system.
#define AUDIO_INITIAL_FREE_INDEX_ARRAY_SIZE 4

#pragma region Typedefs

/// @brief Represents a component that can interact with the audio system.
typedef RJ_Size AudioComponent;

#pragma endregion Typedefs

/// @brief Initialize the audio system with the specified component capacity.
/// @param initialComponentCapacity The initial capacity for audio components.
/// @param positionReferences Reference to the array of position vectors for audio components.
/// @return RJ_OK on success or RJ_ERROR_DEPENDENCY if miniaudio fails or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Audio_Initialize(RJ_Size initialComponentCapacity, Vector3 *positionReferences);

/// @brief Terminate and free the necessary resources for audio system.
void Audio_Terminate(void);

/// @brief Configure the audio listener's position and rotation references.
/// @param positionReference Reference to the listener's position.
/// @param rotationReference Reference to the listener's rotation.
void Audio_ConfigureListener(Vector3 *positionReference, Vector3 *rotationReference);

/// @brief Reconfigure the audio system's position references and component capacity.
/// @param positionReferences Reference to the array of position vectors for audio components.
/// @param newComponentCapacity The new capacity for audio components.
RJ_Result Audio_ConfigureReferences(Vector3 *positionReferences, RJ_Size newComponentCapacity);

/// @brief Update the audio system.
void Audio_Update(void);

/// @brief Creates an audio component and associates it with an entity.
/// @param retComponent Pointer to store the created AudioComponent.
/// @param entity The entity to associate the component with.
/// @param audioFile The audio file to be used by the component.
/// @param positionReference Reference to the position of the audio component.
/// @return RJ_OK on success or RJ_ERROR_DEPENDENCY if miniaudio fails.
RJ_ResultWarn Audio_ComponentCreate(AudioComponent *retComponent, RJ_Size entity, StringView audioFile);

/// @brief Destroys an audio component and frees its resources.
/// @param component Component to destroy.
void Audio_ComponentDestroy(AudioComponent component);

/// @brief Checks if the audio component is currently active.
/// @param component Component to check.
/// @return True if the component is active, false otherwise.
bool Audio_ComponentIsActive(AudioComponent component);

/// @brief Sets the active state of the audio component.
/// @param component Component to configure.
/// @param isActive True to activate the component, false to deactivate.
void Audio_ComponentSetActive(AudioComponent component, bool isActive);

/// @brief Is the audio of the component currently playing or not.
/// @param component Component to check.
/// @return True if the audio is playing, false if not.
bool Audio_ComponentIsPlaying(AudioComponent component);

/// @brief Sets the playing state of the audio component.
/// @param component Component to check.
/// @param play Play the audio if true, pause if false.
void Audio_ComponentSetPlaying(AudioComponent component, bool play);

/// @brief Rewinds the audio by moving its cursor.
/// @param batch Audio batch the component is in.
/// @param component Component to rewind.
/// @param interval Between 0 and 1. 0 is the start and 1 is the end. Clamps to 0-1 internally.
void Audio_ComponentRewind(AudioComponent component, float interval);

/// @brief Checks if the component is set to loop the audio.
/// @param component Component to check.
/// @return True if the component is set to loop the audio, false otherwise.
bool Audio_ComponentIsLooping(AudioComponent component);

/// @brief Configures the component to loop the audio not.
/// @param component Component to configure
/// @param loop Loop or not.
void Audio_ComponentSetLooping(AudioComponent component, bool loop);
