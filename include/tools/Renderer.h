#pragma once

#include "Global.h"

#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/Vectors.h"

#include "wrappers/Context.h"

#include <cglm/mat4.h>

#define RENDERER_OPENGL_CLEAR_COLOR 0.1f, 0.1f, 0.1f, 1.0f
#define RENDERER_OPENGL_INFO_LOG_BUFFER 4096

#define RENDERER_VBO_POSITION_BINDING 0
#define RENDERER_VBO_NORMAL_BINDING 1
#define RENDERER_VBO_UV_BINDING 2

#define RENDERER_UBO_MATRICES_BINDING 0

#define RENDERER_CAMERA_DEFAULT_IS_PERSPECTIVE true
#define RENDERER_CAMERA_DEFAULT_FOV 90.0f
#define RENDERER_CAMERA_DEFAULT_ORTHOGRAPHIC_SIZE 10.0f
#define RENDERER_CAMERA_DEFAULT_NEAR_CLIP_PLANE 0.1f
#define RENDERER_CAMERA_DEFAULT_FAR_CLIP_PLANE 100.0f
#define RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER 1000.0f

#define RENDERER_MODEL_MAX_CHILD_MESH_COUNT 128

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256 //! MUST MATCH WITH VERTEX SHADER

#define RENDERER_TEXTURE_MAX_COUNT 16

#pragma region typedefs

typedef unsigned int RendererShaderHandle;
typedef unsigned int RendererShaderProgramHandle;
typedef unsigned int RendererTextureHandle;

typedef int RendererUniformLocationHandle;
typedef unsigned int RendererUniformBlockHandle;

typedef unsigned int RendererVAOHandle;
typedef unsigned int RendererVBOHandle;
typedef unsigned int RendererIBOHandle;
typedef unsigned int RendererUBOHandle;

typedef unsigned int RendererMeshIndex;

/// @brief A complete model that holds multiple mesh data inside
typedef struct RendererModel
{
    String name;
    ListArray vertices; // RendererMeshVertex, data must be continuous
    ListArray meshes;   // RendererMesh*
} RendererModel;

/// @brief A scene of render objects of the same format that share the same vertex array object (VAO) and vertex buffer object (VBO). The scene is resizable but object's vertices or indices are not because it holds one big mesh for all objects. Scene is only updatable all at once.
typedef struct RendererScene RendererScene;

/// @brief The camera object for the renderer.
typedef struct RendererCameraComponent
{
    RendererScene *scene;

    Vector3 *positionReference;
    Vector3 *rotationReference;

    mat4 projectionMatrix;
    mat4 viewMatrix;

    float size; // fov if perspective, orthographic size if orthographic
    float nearClipPlane;
    float farClipPlane;
    bool isPerspective;
} RendererCameraComponent;

/// @brief A scene of render objects of the same format that share the same vertex array object (VAO) and vertex buffer object (VBO). The scene is resizable but object's vertices or indices are not because it holds one big mesh for all objects. Scene is only updatable all at once.
typedef struct RendererScene
{
    String name;
    RendererCameraComponent *camera;
    ListArray batches; // RendererBatch

    RendererVAOHandle vao;
    RendererVBOHandle vboModelVertices;
    RendererIBOHandle iboModelIndices;
    RendererUBOHandle uboObjectMatrices;

    RendererUniformLocationHandle camProjectionMatrix;
    RendererUniformLocationHandle camViewMatrix;
    RendererUniformLocationHandle camPosition;
    RendererUniformLocationHandle camRotation;
    RendererUniformLocationHandle matAmbientColor;
    RendererUniformLocationHandle matDiffuseColor;
    RendererUniformLocationHandle matSpecularColor;
    RendererUniformLocationHandle matSpecularExponent;
    RendererUniformLocationHandle matDissolve;
    RendererUniformLocationHandle matDiffuseMap;
    RendererUniformLocationHandle matHasDiffuseMap;
    RendererUniformBlockHandle objectMatricesHandle;
} RendererScene;

typedef struct RendererBatch
{
    String name;
    RendererModel *model;
    ListArray components;     // RendererComponent
    ListArray objectMatrices; // mat4, data must be continuous

    RendererScene *scene;
    size_t batchOffsetInScene;
} RendererBatch;

/// @brief A render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the scene. Must be used with RendererScene. Not updatable on it's own.
typedef struct RendererComponent
{
    Vector3 *positionReference;
    Vector3 *rotationReference;
    Vector3 *scaleReference;

    RendererBatch *batch;
    size_t componentOffsetInBatch;
} RendererComponent;

#pragma endregion typedefs

#pragma region Renderer

/// @brief Initializer for renderer. Initializes OpenGL and GLFW. Sets options. Creates main window. Should be called before any renderer function.
/// @param title Window title.
/// @param windowSize Window size.
/// @param vertexShaderSource Source code of the main vertex shader.
/// @param fragmentShaderSource Source code of the main fragment shader.
/// @param vSync Whether to enable vertical synchronization.
/// @param fullScreen Whether the app will start in full screen mode or not.
/// @return A pointer to the created context / window.
void Renderer_Initialize(ContextWindow *window);

/// @brief Terminator for renderer.
void Renderer_Terminate();

/// @brief
/// @param vertexShaderSource
/// @param fragmentShaderSource
void Renderer_ConfigureShaders(String vertexShaderSource, String fragmentShaderSource);

/// @brief Should be called before using rendering functions.
void Renderer_StartRendering();

/// @brief Should be called after using rendering functions.
void Renderer_FinishRendering();

/// @brief Renders a scene of objects.
/// @param scene The scene of objects to render.
void Renderer_RenderScene(RendererScene *scene);

#pragma endregion Renderer

#pragma region Renderer Model

/// @brief Creates a model from an OBJ file source. The .obj and its other files (like .mtl) must be in the same directory. Only supports models with triangular faces. doesn't support objects with normal maps but without UVs (x//x signature).
/// @param name Name of the model to create.
/// @param objFileSource Source code of the OBJ file.
/// @param objFileSourceLineCount Number of lines in the OBJ file source.
/// @param objFilePath The resources-relative path of the OBJ file.
/// @param positionOffset Position offset to freely adjust final model position.
/// @param rotationOffset Rotation offset to freely adjust final model rotation.
/// @param scaleOffset Scale offset to freely adjust final model scale.
/// @return Created model with vertices and indices.
RendererModel *RendererModel_CreateOBJ(String name, String objFileSource, size_t objFileSourceLineCount, String objFilePath, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset);

/// @brief Creates an empty model with no meshes.
/// @param name Name of the model to create.
/// @param initialMeshCapacity Initial capacity for the mesh array.
/// @param initialVertexCapacity Initial capacity for the vertex array.
/// @return Created empty model.
RendererModel *RendererModel_CreateEmpty(String name, size_t initialMeshCapacity, size_t initialVertexCapacity);

/// @brief Destroyer function for renderer model.
/// @param model Model to destroy.
void RendererModel_Destroy(RendererModel *model);

#pragma endregion Renderer Model

#pragma region Renderer Scene

/// @brief Creates a scene of render objects that will be managed together.
/// @param name Name of the scene.
/// @param initialBatchCapacity The initial capacity of the batch list in scene.
/// @return Created scene of render objects.
RendererScene *RendererScene_Create(String name, size_t initialBatchCapacity);

/// @brief Destroyer function for object scene
/// @param scene Scene to destroy
void RendererScene_Destroy(RendererScene *scene);

/// @brief Sets the main camera for the renderer.
/// @param scene Scene to set main camera.
/// @param camera Camera object to set as the main camera.
void RendererScene_SetMainCamera(RendererScene *scene, RendererCameraComponent *camera);

/// @brief Creates a renderer batch to store components that use the same model.
/// @param scene Pointer to the scene that the batch will belong to.
/// @param name Name of the batch.
/// @param model Pointer to the model that the components in the batch will use.
/// @param initialComponentCapacity The initial capacity for components inside the batch.
/// @return The created batch.
RendererBatch *RendererScene_CreateBatch(RendererScene *scene, String name, RendererModel *model, size_t initialComponentCapacity);

/// @brief Destroys the renderer batch and frees its resources.
/// @param batch Batch to destroy.
void RendererScene_DestroyBatch(RendererBatch *batch);

/// @brief Updates the renderer scene.
/// @param scene Scene to update.
void RendererScene_Update(RendererScene *scene);

#pragma endregion Renderer Scene

#pragma region Renderer Batch

/// @brief Creates a component in the specified batch. The total number of components in a batch cannot exceed RENDERER_BATCH_MAX_OBJECT_COUNT.
/// @param batch The batch to create the component in.
/// @return A newly created component.
RendererComponent *RendererBatch_CreateComponent(RendererBatch *batch, Vector3 *positionReference, Vector3 *rotationReference, Vector3 *scaleReference);

/// @brief Destroys a component and frees its resources.
/// @param component The component to destroy.
void RendererBatch_DestroyComponent(RendererComponent *component);

#pragma endregion Renderer Batch

#pragma region Renderer Camera

/// @brief Creates a camera object to control the view. Changes the scene.
/// @param scene Scene to attach camera.
/// @return Created camera object.
RendererCameraComponent *RendererCameraComponent_Create(Vector3 *positionReference, Vector3 *rotationReference);

/// @brief Destroys a camera object.
/// @param camera Camera object to destroy.
void RendererCameraComponent_Destroy(RendererCameraComponent *camera);

/// @brief Configure the renderer camera.
/// @param camera Camera to configure
/// @param isPerspective Set the camera perspective or orthographic.
/// @param value fov if perspective, orthographicSize if orthographic.
/// @param nearClipPlane Near clipping plane for the camera.
/// @param farClipPlane Far clipping plane for the camera.
void RendererCameraComponent_Configure(RendererCameraComponent *camera, bool isPerspective, float value, float nearClipPlane, float farClipPlane);

#pragma endregion Renderer Camera
