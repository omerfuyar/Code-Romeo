#pragma once

#include "RJGlobal.h"

#include "tools/Entity.h"
#include "tools/Context.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

#pragma region typedefs

// todo put buffer objects to batches and bind them on use

#define RENDERER_OPENGL_CLEAR_COLOR 0.3f, 0.3f, 0.3f, 1.0f
#define RENDERER_OPENGL_INFO_LOG_BUFFER 4096

#define RENDERER_VBO_POSITION_BINDING 0
#define RENDERER_VBO_NORMAL_BINDING 1
#define RENDERER_VBO_UV_BINDING 2

#define RENDERER_UBO_MATRICES_BINDING 0

#define RENDERER_DEBUG_VBO_POSITION_BINDING 0
#define RENDERER_DEBUG_VBO_COLOR_BINDING 1

#define RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER 1000.0f

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256 //! MUST MATCH WITH VERTEX SHADER
#define RENDERER_BATCH_INITIAL_CAPACITY 16

/// @brief Represents a batch of objects that share the same model for rendering.
typedef Entity RendererBatch;

// todo either make systems to use user exposed data and retrieve data from user passed pointers or move data to systems internal memory and use with getter/setters

/// @brief Represents a camera used to render components.
typedef struct RendererCamera
{
    Vector3 position;
    Vector3 rotation;

    float size; // fov if perspective, orthographic size if orthographic
    float nearClipPlane;
    float farClipPlane;
    bool isPerspective;
} RendererCamera;

#define RendererCamera_Default \
    (RendererCamera) { .position = Vector3_Zero, .rotation = Vector3_Zero, .size = 90.0f, .nearClipPlane = 0.01f, .farClipPlane = 1000.0f, .isPerspective = true }

#pragma endregion typedefs

#pragma region Renderer

/// @brief Initializes the renderer system.
/// @param initialBatchCapacity The initial capacity for renderer batches.
/// @return RJ_OK on success, RJ_ERROR_DEPENDENCY if glad or GLFW fails or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Renderer_Initialize(RJ_Size initialBatchCapacity);

/// @brief Terminates the renderer system.
void Renderer_Terminate(void);

/// @brief Get the status of renderer module.
/// @return Renderer module is initialized or not.
bool Renderer_IsInitialized(void);

/// @brief Configures the shaders used by the renderer.
/// @param vertexShaderFile The file path of the vertex shader.
/// @param fragmentShaderFile The file path of the fragment shader.
/// @return RJ_OK on success, RJ_ERROR_FILE if internal file read fails, RJ_ERROR_ALLOCATION if internal allocation fails, RJ_ERROR_DEPENDENCY if opengl fails.
RJ_ResultWarn Renderer_ConfigureShaders(StringView vertexShaderFile, StringView fragmentShaderFile);

const RendererCamera *Renderer_GetCameraData(void);

void Renderer_SetCameraData(const RendererCamera *cameraData);

/// @brief Converts screen coordinates to world coordinates with given depth.
/// @param screenPosition Screen coordinate to convert.
/// @param depth Depth of the cast. How far you want to get from camera
/// @return World space position of the converted coordinate.
Vector3 Renderer_ScreenToWorldSpace(Vector2Int screenPosition, float depth);

/// @brief Resizes the renderer's batch capacity.
/// @param newBatchCapacity The new capacity for renderer batches.
/// @return RJ_OK on success, RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Renderer_Resize(RJ_Size newBatchCapacity);

/// @brief Updates the renderer system. Call before using any renderer function in during the frame.
void Renderer_Update(void);

/// @brief Renders the current frame.
void Renderer_Render(void);

/// @brief Creates a renderer batch.

/// @param modelFile The file path to the .glb or .gltf model file.
/// @param retBatch The handle to the created renderer batch.
/// @param initialComponentCapacity The initial capacity for components in the batch.
/// @return RJ_OK on success, RJ_ERROR_ALLOCATION if internal allocation fails, RJ_ERROR_FILE if model loading fails.
RJ_ResultWarn Renderer_BatchCreate(RendererBatch *retBatch, StringView modelFile, RJ_Size initialComponentCapacity);

/// @brief Destroys a renderer batch.
/// @param batch The handle to the renderer batch to destroy.
void Renderer_BatchDestroy(RendererBatch batch);

// todo maybe remove resizing

/// @brief Configures the references for a renderer batch.
/// @param batch The handle to the renderer batch.
/// @param newComponentCapacity The new capacity for components in the batch.
/// @return RJ_OK on success, RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Renderer_BatchResize(RendererBatch batch, RJ_Size newComponentCapacity);

/// @brief Creates a renderer component within a specified batch.
/// @param entity The entity associated with the renderer component.
/// @param batch The handle to the renderer batch.
/// @return The handle to the created renderer component.
void Renderer_ComponentCreate(RendererBatch batch, Entity entity);

/// @brief Destroys a renderer component within a specified batch.
/// @param batch The handle to the renderer batch.
/// @param component The handle to the renderer component to destroy.
void Renderer_ComponentDestroy(Entity entity);

#pragma endregion Renderer
