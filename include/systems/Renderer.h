#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

#include "tools/Context.h"

/// @brief Capacity of free indices array of the renderer system.
#define RENDERER_INITIAL_FREE_INDEX_ARRAY_SIZE 4

#define RENDERER_FLAG_ACTIVE (1 << 0)

#define RENDERER_OPENGL_CLEAR_COLOR 0.3f, 0.3f, 0.3f, 1.0f
#define RENDERER_OPENGL_INFO_LOG_BUFFER 4096

#define RENDERER_VBO_POSITION_BINDING 0
#define RENDERER_VBO_NORMAL_BINDING 1
#define RENDERER_VBO_UV_BINDING 2

#define RENDERER_UBO_MATRICES_BINDING 0

#define RENDERER_DEBUG_VBO_POSITION_BINDING 0
#define RENDERER_DEBUG_VBO_COLOR_BINDING 1

#define RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE true
#define RENDERER_CAMERA_DEFAULT_FOV 90.0f
#define RENDERER_CAMERA_DEFAULT_ORTHOGRAPHIC_SIZE 10.0f
#define RENDERER_CAMERA_DEFAULT_NEAR_CLIP_PLANE 0.01f
#define RENDERER_CAMERA_DEFAULT_FAR_CLIP_PLANE 1000.0f
#define RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER 1000.0f

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256 //! MUST MATCH WITH VERTEX SHADER
#define RENDERER_BATCH_INITIAL_CAPACITY 16

#pragma region typedefs

/// @brief Represents a component that can interact with the renderer system.
typedef RJGlobal_Index RendererComponent;

/// @brief Represents a batch of objects that share the same model for rendering.
typedef RJGlobal_Index RendererBatch;

#pragma endregion typedefs

#pragma region Renderer

void Renderer_Initialize(ContextWindow *window, RJGlobal_Size initialBatchCapacity);

/// @brief Terminator for renderer.
void Renderer_Terminate();

void Renderer_ConfigureShaders(StringView vertexShaderFile, StringView fragmentShaderFile);

void Renderer_ConfigureCamera(Vector3 *positionReference, Vector3 *rotationReference, float *sizeReference, float *nearClipPlaneReference, float *farClipPlaneReference, bool *isPerspectiveReference);

void Renderer_Resize(RJGlobal_Size newBatchCapacity);
// void Renderer_Update(){}

void Renderer_Render();

RendererBatch Renderer_BatchCreate(StringView mdlFile, RJGlobal_Size initialCapacity, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences);

void Renderer_BatchDestroy(RendererBatch batch);

void Renderer_BatchConfigureReferences(RendererBatch batch, Vector3 *positionReferences, Vector3 *rotationReferences, Vector3 *scaleReferences, RJGlobal_Size newComponentCapacity);

RendererComponent Renderer_ComponentCreate(RJGlobal_Index entity, RendererBatch batch);

void Renderer_ComponentDestroy(RendererBatch batch, RendererComponent component);

#pragma endregion Renderer

#pragma region Renderer Debug

/// @brief Initialize function for renderer debug functions. Should be called after the Renderer_Initialize function.
/// @param vertexShaderFile The source file for debug vertex shader.
/// @param fragmentShaderFile The source file for debug fragment shader.
/// @param initialVertexCapacity The initial capacity for the vertex buffer.
void RendererDebug_Initialize(StringView vertexShaderFile, StringView fragmentShaderFile, RJGlobal_Size initialVertexCapacity);

/// @brief Terminator for renderer debug functions.
void RendererDebug_Terminate();

/// @brief Should be called before using debug rendering functions, and after main renderer's rendering functions.
void RendererDebug_StartRendering();

/// @brief Should be called before Renderer_FinishRendering to draw all debug shapes.
/// @param camProjectionMatrix The projection matrix of the camera.
/// @param camViewMatrix The view matrix of the camera.
void RendererDebug_FinishRendering(const Renderer_Matrix4 *camProjectionMatrix, const Renderer_Matrix4 *camViewMatrix);

/// @brief Draws a line in 3D space for debugging purposes.
/// @param start The starting point of the line.
/// @param end The ending point of the line.
/// @param color The color of the line.
void RendererDebug_DrawLine(Vector3 start, Vector3 end, Color color);

/// @brief Draws the wireframe of a box in 3D space for debugging purposes.
/// @param position The center position of the box.
/// @param size The size of the box on each axis.
/// @param color The color of the lines.
void RendererDebug_DrawBoxLines(Vector3 position, Vector3 size, Color color);

#pragma endregion Renderer Debug
