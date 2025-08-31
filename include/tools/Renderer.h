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

extern float RENDERER_DELTA_TIME;

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
typedef struct RendererScene RendererScene;

/// @brief Represents the transformation (position, rotation, scale) of an object in 3D space.
typedef struct RendererObjectTransform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
} RendererObjectTransform;

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS (SCENE CREATE)
/// @brief Represents a primal vertex in 3D space.
typedef struct RendererMeshVertex
{
    Vector3 vertexPosition;
    Vector3 vertexNormal;
    Vector2 vertexUV;
} RendererMeshVertex;

typedef struct RendererMaterial
{
    String name;
    Color ambientColor;
    Color diffuseColor;
    Color specularColor;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
    RendererTextureHandle diffuseMapHandle;
} RendererMaterial;

/// @brief A model mesh structure that holds all necessary data to represent a 3D mesh.
typedef struct RendererMesh
{
    ListArray vertices; // RendererMeshVertex
    ListArray indices;  // RendererMeshIndex
    RendererMaterial *material;
} RendererMesh;

/// @brief A complete model that holds multiple mesh data inside
typedef struct RendererModel
{
    String name;
    ListArray meshes; // RendererMesh*
} RendererModel;

/// @brief The camera object for the renderer.
typedef struct RendererCamera
{
    String name;
    RendererObjectTransform transform;
    RendererScene *scene;

    bool isPerspective;
    float size; // fov if perspective, orthographic size if orthographic
    float nearClipPlane;
    float farClipPlane;

    mat4 projectionMatrix;
    mat4 viewMatrix;
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
    RendererUniformLocationHandle matAmbientColor;
    RendererUniformLocationHandle matDiffuseColor;
    RendererUniformLocationHandle matSpecularColor;
    RendererUniformLocationHandle matSpecularExponent;
    RendererUniformLocationHandle matDissolve;
    RendererUniformLocationHandle matDiffuseMap;
    RendererUniformBlockHandle objectMatricesHandle;
} RendererScene;

typedef struct RendererBatch
{
    String name;
    RendererModel *model;
    RendererScene *scene;

    ListArray objectMatrices; // mat4, data should be continuous

    size_t batchOffsetInScene;
} RendererBatch;

/// @brief A render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the scene. Must be used with RendererScene. Not updatable on it's own.
typedef struct RendererObject
{
    String name;
    RendererObjectTransform transform;
    RendererBatch *batch;
    size_t matrixOffsetInBatch;
} RendererObject;

#pragma endregion typedefs

#pragma region Renderer

/// @brief Initializer for renderer. Initializes OpenGL and GLFW. Sets options. Creates main window. Should be called before any renderer function.
/// @param title Window title.
/// @param windowSize Window size.
/// @param vertexShaderSource Source code of the main vertex shader.
/// @param fragmentShaderSource Source code of the main fragment shader.
/// @param vSync Whether to enable vertical synchronization.
/// @param fullScreen Whether the app will start in full screen mode or not.
void Renderer_CreateContext(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync, bool fullScreen);

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
void Renderer_FinishRendering();

/// @brief Renders a scene of objects.
/// @param scene The scene of objects to render.
void Renderer_RenderScene(RendererScene *scene);

#pragma endregion Renderer

#pragma region Renderer Object Transform

/// @brief Sets the position of the object transform.
/// @param transform Pointer to the object transform.
/// @param position New position for the object.
void RendererObjectTransform_SetPosition(RendererObjectTransform *transform, Vector3 position);

/// @brief Sets the rotation of the object transform.
/// @param transform Pointer to the object transform.
/// @param rotation New rotation for the object.
void RendererObjectTransform_SetRotation(RendererObjectTransform *transform, Vector3 rotation);

/// @brief Sets the scale of the object transform.
/// @param transform Pointer to the object transform.
/// @param scale New scale for the object.
void RendererObjectTransform_SetScale(RendererObjectTransform *transform, Vector3 scale);

/// @brief Adds the given position of the object to transform.
/// @param transform Pointer to the object transform.
/// @param position Position to add to the object.
void RendererObjectTransform_AddPosition(RendererObjectTransform *transform, Vector3 position);

/// @brief Adds the given rotation of the object to transform.
/// @param transform Pointer to the object transform.
/// @param rotation Rotation to add to the object.
void RendererObjectTransform_AddRotation(RendererObjectTransform *transform, Vector3 rotation);

/// @brief Multiplies the scale of the transform with given scale.
/// @param transform Pointer to the object transform.
/// @param scale Scale to multiply the object.
void RendererObjectTransform_MultiplyScale(RendererObjectTransform *transform, Vector3 scale);

/// @brief Sets the matrix with transform values.
/// @param transform Transform to reference. (pointer for performance)
/// @param matrix Matrix to edit.
void RendererObjectTransform_ToModelMatrix(RendererObjectTransform *transform, mat4 *matrix);

#pragma endregion Renderer Object Transform

#pragma region Renderer Mesh

/// @brief Creates an empty mesh with no vertices or indices.
/// @param initialVertexCapacity Initial capacity for the vertex array.
/// @param initialIndexCapacity Initial capacity for the index array.
/// @return Created empty mesh.
RendererMesh *RendererMesh_CreateEmpty(size_t initialVertexCapacity, size_t initialIndexCapacity);

/// @brief Destroyer function for renderer mesh.
/// @param mesh Mesh to destroy.
void RendererMesh_Destroy(RendererMesh *mesh);

#pragma endregion Renderer Mesh

#pragma region Renderer Model

/// @brief Creates a mesh from an OBJ file source. The .obj and its other files (like .mtl) must be in the same directory.
/// @param name Name of the model to create.
/// @param objFileSource Source code of the OBJ file.
/// @param objFilePath The resources-relative path of the OBJ file.
/// @return Created mesh with vertices and indices.
RendererModel *RendererModel_CreateOBJ(String name, String objFileSource, String objFilePath);

/// @brief
/// @param initialMeshCapacity
/// @return
RendererModel *RendererModel_CreateEmpty(size_t initialMeshCapacity);

/// @brief
/// @param model
void RendererModel_Destroy(RendererModel *model);

#pragma endregion Renderer Model

#pragma region Renderer Scene

/// @brief Creates a scene of render objects. A vertex allocator for objects.
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

#pragma endregion Renderer Scene

#pragma region Renderer Batch

/// @brief Creates renderer batch to store objects that are using the same mesh. Changes the scene.
/// @param name Name of the batch.
/// @param scene Pointer to the scene that the batch is belong to.
/// @param model Pointer to the model that the batch is using.
/// @param initialObjectCapacity The initial capacity for objects inside batch.
/// @return The created batch.
RendererBatch *RendererBatch_Create(String name, RendererScene *scene, RendererModel *model, size_t initialObjectCapacity);

/// @brief Destroys the renderer batch and frees its resources.
/// @param batch Batch to destroy.
void RendererBatch_Destroy(RendererBatch *batch);

#pragma endregion Renderer Batch

#pragma region Renderer Camera

/// @brief Creates a camera object to control the view. Changes the scene.
/// @param name Name of the camera.
/// @param scene Scene to attach camera.
/// @return Created camera object.
RendererCamera *RendererCamera_Create(String name, RendererScene *scene);

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
void RendererCamera_Update(RendererCamera *camera);

#pragma endregion Renderer Camera

#pragma region Renderer Object

/// @brief Creates a render object that shares its VAO and VBO with other objects in the scene. Changes the batch.
/// @param name Name of the render object.
/// @param batch Pointer to the batch that the object belongs to.
/// @return Created render object.
RendererObject *RendererObject_Create(String name, RendererBatch *batch);

/// @brief Destroyer function for render object
/// @param object Object to destroy
void RendererObject_Destroy(RendererObject *object);

/// @brief Updater function for object transform, logic etc.
/// @param object Object to update. (pointer for performance)
void RendererObject_Update(RendererObject *object);

#pragma endregion Renderer Object
