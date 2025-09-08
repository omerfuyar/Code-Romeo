#pragma once

#include "Global.h"

#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/Vectors.h"

#include <cglm/mat4.h>

#define RENDERER_OPENGL_VERSION_MAJOR 3
#define RENDERER_OPENGL_VERSION_MINOR 3
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

#define RENDERER_MODEL_MAX_CHILD_MESH_COUNT 16

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

#define RendererObjectTransformDefault

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (SCENE CREATE)
/// @brief Represents a primal vertex in 3D space.
typedef struct RendererMeshVertex
{
    Vector3 vertexPosition;
    Vector3 vertexNormal;
    Vector2 vertexUV;
} RendererMeshVertex;

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
typedef struct RendererCamera
{
    mat4 projectionMatrix;
    mat4 viewMatrix;

    Vector3 position;
    Vector3 rotation;

    RendererScene *scene;

    float size; // fov if perspective, orthographic size if orthographic
    float nearClipPlane;
    float farClipPlane;
    bool isPerspective;
} RendererCamera;

/// @brief A scene of render objects of the same format that share the same vertex array object (VAO) and vertex buffer object (VBO). The scene is resizable but object's vertices or indices are not because it holds one big mesh for all objects. Scene is only updatable all at once.
typedef struct RendererScene
{
    String name;
    RendererCamera *camera;
    ListArray batches; // RendererBatch*

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
    ListArray objectMatrices; // mat4, data must be continuous

    RendererScene *scene;
    size_t batchOffsetInScene;
} RendererBatch;

/// @brief A render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the scene. Must be used with RendererScene. Not updatable on it's own.
typedef struct RendererRenderable
{
    RendererBatch *batch;
    size_t matrixOffsetInBatch;
} RendererRenderable;

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
void *Renderer_CreateContext(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync, bool fullScreen);

/// @brief Terminator for renderer.
void Renderer_Terminate();

/// @brief Configure the main window of the app.
/// @param windowSize Window size.
/// @param vSync Whether to enable vertical synchronization.
/// @param fullScreen Whether the app will start in full screen mode or not
void Renderer_ConfigureContext(Vector2Int windowSize, bool vSync, bool fullScreen);

/// @brief Should be called before using rendering functions.
void Renderer_StartRendering();

/// @brief Should be called after using rendering functions.
void Renderer_FinishRendering(float deltaTime);

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
void RendererScene_SetMainCamera(RendererScene *scene, RendererCamera *camera);

/// @brief Creates renderer batch to store objects that are using the same mesh. Changes the scene. Max object count is RENDERER_BATCH_MAX_OBJECT_COUNT macro.
/// @param name Name of the batch.
/// @param scene Pointer to the scene that the batch is belong to.
/// @param model Pointer to the model that the batch is using.
/// @param initialObjectCapacity The initial capacity for objects inside batch.
/// @return The created batch.
RendererBatch *RendererScene_CreateBatch(RendererScene *scene, String name, RendererModel *model, size_t initialObjectCapacity);

/// @brief Destroys the renderer batch and frees its resources.
/// @param batch Batch to destroy.
void RendererScene_DestroyBatch(RendererBatch *batch);

#pragma endregion Renderer Scene

#pragma region Renderer Batch

/// @brief Creates a renderable object in the specified batch. Max object count is RENDERER_BATCH_MAX_OBJECT_COUNT macro.
/// @param batch The batch to create the renderable in.
/// @return A newly created renderable object.
RendererRenderable *RendererBatch_CreateRenderable(RendererBatch *batch);

/// @brief Destroys a renderable object and frees its resources.
/// @param object The renderable object to destroy.
void RendererBatch_DestroyRenderable(RendererRenderable *object);

#pragma endregion Renderer Batch

#pragma region Renderer Camera

/// @brief Creates a camera object to control the view. Changes the scene.
/// @param scene Scene to attach camera.
/// @return Created camera object.
RendererCamera *RendererCamera_Create(RendererScene *scene);

/// @brief Destroys a camera object.
/// @param camera Camera object to destroy.
void RendererCamera_Destroy(RendererCamera *camera);

/// @brief Configure the renderer camera.
/// @param camera Camera to configure
/// @param isPerspective Set the camera perspective or orthographic.
/// @param value fov if perspective, orthographicSize if orthographic.
/// @param nearClipPlane Near clipping plane for the camera.
/// @param farClipPlane Far clipping plane for the camera.
void RendererCamera_Configure(RendererCamera *camera, bool isPerspective, float value, float nearClipPlane, float farClipPlane);

/// @brief Updates the camera's properties to render.
/// @param camera Camera to update
/// @param position New position for the camera.
/// @param rotation New rotation for the camera. In degrees.
void RendererCamera_Update(RendererCamera *camera, Vector3 position, Vector3 rotation);

#pragma endregion Renderer Camera

#pragma region Renderer Renderable

/// @brief Updates the renderable object's transform matrix.
/// @param renderable Renderable object to update.
/// @param position New position for the renderable.
/// @param rotation New rotation for the renderable. In degrees.
/// @param scale New scale for the renderable.
void RendererRenderable_Update(RendererRenderable *renderable, Vector3 position, Vector3 rotation, Vector3 scale);

#pragma endregion Renderer Renderable
