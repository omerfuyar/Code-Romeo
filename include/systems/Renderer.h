#pragma once

#include "RJGlobal.h"

#include "tools/Entity.h"
#include "tools/Context.h"

#include "utilities/String.h"
#include "utilities/Vector.h"

#pragma region typedefs

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

/// @brief Represents a batch of objects that share the same model for rendering.
typedef RJ_Size RendererBatch;

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

#pragma endregion typedefs

#pragma region Renderer

/// @brief Initializes the renderer system.
/// @param window The context window to render to.
/// @param initialBatchCapacity The initial capacity for renderer batches.
/// @return RJ_OK on success, RJ_ERROR_DEPENDENCY if glad or GLFW fails or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Renderer_Initialize(const ContextWindow *window, RJ_Size initialBatchCapacity);

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

/// @brief Configures the camera parameters for the renderer.
/// @param camera The address of the camera data struct to use for rendering.
void Renderer_SetCamera(RendererCamera *camera);

/// @brief Gets the camera used for rendering.
/// @param retCamera Camera pointer reference to store returned data.
/// @return RJ_OK on success,
RJ_ResultWarn Renderer_GetCamera(RendererCamera **retCamera);

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

/// @param mdlFile The file path of the model.
/// @param retBatch The handle to the created renderer batch.
/// @param transformOffset The offset to apply to the model's transform.
/// @param initialComponentCapacity The initial capacity for components in the batch.
/// @return RJ_OK on success, RJ_ERROR_ALLOCATION if internal allocation fails, RJ_ERROR_FILE if model loading fails.
RJ_ResultWarn Renderer_BatchCreate(RendererBatch *retBatch, StringView mdlFile, const Vector3 *transformOffset, RJ_Size initialComponentCapacity);

/// @brief Destroys a renderer batch.
/// @param batch The handle to the renderer batch to destroy.
void Renderer_BatchDestroy(RendererBatch batch);

/// @brief Configures the references for a renderer batch.
/// @param batch The handle to the renderer batch.
/// @param newComponentCapacity The new capacity for components in the batch.
/// @return RJ_OK on success, RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Renderer_BatchConfigure(RendererBatch batch, RJ_Size newComponentCapacity);

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

#pragma region Renderer Debug

/// @brief Initialize function for renderer debug functions. Should be called after the Renderer_Initialize function.
/// @param vertexShaderFile The source file for debug vertex shader.
/// @param fragmentShaderFile The source file for debug fragment shader.
/// @param initialVertexCapacity The initial capacity for the vertex buffer.
/// @return RJ_OK on success, RJ_ERROR_FILE if internal file read fails, RJ_ERROR_ALLOCATION if internal allocation fails, RJ_ERROR_DEPENDENCY if opengl fails.
RJ_ResultWarn RendererDebug_Initialize(StringView vertexShaderFile, StringView fragmentShaderFile, RJ_Size initialVertexCapacity);

/// @brief Terminator for renderer debug functions.
void RendererDebug_Terminate(void);

/// @brief Should be called before using debug rendering functions, and after main renderer's rendering functions.
void RendererDebug_StartRendering(void);

/// @brief Should be called before Renderer_FinishRendering to draw all debug shapes.
void RendererDebug_FinishRendering(void);

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
