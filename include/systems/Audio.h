#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"
#include "utilities/ListArray.h"

/// @brief
typedef struct AudioClip
{
    String name;
    void *data;
} AudioClip;

typedef struct AudioScene AudioScene;

typedef struct AudioListenerComponent
{
    AudioScene *scene;

    Vector3 *positionReference;
    Vector3 *rotationReference;
} AudioListenerComponent;

typedef struct AudioScene
{
    String name;
    AudioListenerComponent *listener;
    ListArray components; // AudioComponent
} AudioScene;

/// @brief source
typedef struct AudioComponent
{
    AudioClip *clip;
    AudioScene *scene;
    size_t componentOffsetInScene;
} AudioComponent;

#pragma region Audio

void Audio_Initialize(size_t initialSoundCapacity);

void Audio_Terminate();

#pragma endregion Audio

#pragma region AudioClip

AudioClip *AudiClip_Create(StringView audioFile);

void AudioClip_Destroy(AudioClip *clip);

#pragma endregion AudioClip

#pragma region AudioScene

AudioScene *AudioScene_Create(StringView name, size_t initialComponentCapacity);

void AudioScene_Destroy(AudioScene *scene);

AudioComponent *AudioScene_CreateComponent();

void AudioScene_DestroyComponent(AudioComponent *component);

#pragma endregion AudioScene
