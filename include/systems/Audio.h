#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

/// @brief Scene to build a audio functionality.
typedef struct AudioScene AudioScene;

/// @brief Listener Component to hold necessary data to listen sound in a scene.
typedef struct AudioListenerComponent
{
    AudioScene *scene;

    Vector3 *positionReference;
    Vector3 *rotationReference;
} AudioListenerComponent;

/// @brief Scene to build a audio functionality.
typedef struct AudioScene
{
    String name;
    AudioListenerComponent *listener;
    ListArray components; // AudioComponent
} AudioScene;

/// @brief Audio component to hold the necessary data to play and store a sound.
typedef struct AudioComponent
{
    AudioScene *scene;

    Vector3 *positionReference;

    size_t componentOffsetInScene;
    void *data;
} AudioComponent;

#pragma region Audio

/// @brief Initialize the necessary resources for audio system.
void Audio_Initialize();

/// @brief Terminate and free the necessary resources for audio system.
void Audio_Terminate();

#pragma endregion Audio

#pragma region AudioScene

AudioScene *AudioScene_Create(StringView name, size_t initialComponentCapacity);

void AudioScene_Destroy(AudioScene *scene);

void AudioScene_SetMainListener(AudioScene *scene, AudioListenerComponent *listener);

AudioComponent *AudioScene_CreateComponent(AudioScene *scene, StringView file, Vector3 *positionReference);

AudioListenerComponent *AudioScene_CreateListenerComponent(AudioScene *scene, Vector3 *positionReference, Vector3 *rotationReference);

#pragma endregion AudioScene

#pragma region AudioComponent

/// @brief Update the component to its 3D space.
/// @param component Component to update.
void AudioComponent_Update(AudioComponent *component);

/// @brief Play the components audio.
/// @param component Component to play the audio
void AudioComponent_Play(AudioComponent *component);

/// @brief Pause the components audio.
/// @param component Component to pause the audio.
void AudioComponent_Pause(AudioComponent *component);

/// @brief Is the audio of the component currently playing or not.
/// @param component Component to check.
/// @return True if the audio is playing, false if not.
bool AudioComponent_IsPlaying(AudioComponent *component);

/// @brief Rewinds the audio by moving its cursor.
/// @param component Component to rewind.
/// @param interval Between 0 and 1. 0 is the start and 1 is the end. Clamps to 0-1 internally.
void AudioComponent_Rewind(AudioComponent *component, float interval);

/// @brief Configures the component to loop the audio not.
/// @param component Component to configure
/// @param loop Loop or not.
void AudioComponent_SetLooping(AudioComponent *component, bool loop);

#pragma endregion AudioComponent

#pragma region AudioListenerComponent

/// @brief Update the listener to its 3D space.
/// @param listener Listener to update.
void AudioListenerComponent_Update(AudioListenerComponent *listener);

#pragma endregion AudioListenerComponent
