#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

/// @brief
typedef struct AudioClip AudioClip;

#pragma region Audio

/// @brief
/// @param initialSoundCapacity
void Audio_Initialize(size_t initialSoundCapacity);

/// @brief
void Audio_Terminate();

// void Audio_ConfigureListener(const Vector3 *positionReference, const Vector3 *rotationReference, float maxDistance, float rolloffFactor);

/// @brief
/// @param clip
void Audio_PlayClip(AudioClip *clip /*, const Vector3 *position*/);

/// @brief
/// @param clip
void Audio_StopClip(AudioClip *clip);

#pragma endregion Audio

#pragma region AudioClip

/// @brief Creates an audio clip from a file.
/// @param audioFile File name of the audio clip to play. Relative to resources folder in executable directory.
/// @return Newly created AudioClip pointer. All data is internally managed.
AudioClip *AudioClip_Create(StringView audioFile);

/// @brief
/// @param clip
void AudioClip_Destroy(AudioClip *clip);

#pragma endregion AudioClip
