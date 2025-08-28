#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/Vectors.h"

#include "cglm/mat4.h"

#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 3

#define RENDERER_VBO_POSITION_BINDING 0
#define RENDERER_VBO_COLOR_BINDING 1

#define RENDERER_UBO_MATRICES_BINDING 0

#define RENDERER_CAMERA_NEAR_CLIP_PLANE 0.1f
#define RENDERER_CAMERA_FAR_CLIP_PLANE 100.0f
#define RENDERER_CAMERA_ORTHOGRAPHIC_SIZE_MULTIPLIER 1000.0f

#define RENDERER_SCENE_MAX_OBJECT_COUNT 1000

extern float RENDERER_DELTA_TIME;

#pragma region typedefs

typedef unsigned int RendererShaderHandle;
typedef unsigned int RendererShaderProgramHandle;
typedef unsigned int RendererTextureHandle;
typedef unsigned int RendererUniformHandle;

typedef unsigned int RendererVAOHandle;
typedef unsigned int RendererVBOHandle;
typedef unsigned int RendererIBOHandle;
typedef unsigned int RendererUBOHandle;

typedef unsigned int RendererMeshIndex;

/// @brief Represents the transformation (position, rotation, scale) of an object in 3D space.
typedef struct RendererObjectTransform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
} RendererObjectTransform;

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS
/// @brief Represents a primal vertex in 3D space.
typedef struct RendererMeshVertex
{
    Vector3 position;
    Vector4 color;
} RendererMeshVertex;

/// @brief A model mesh structure that holds all necessary data to represent a 3D mesh.
typedef struct RendererMesh
{
    size_t id;
    ListArray vertices; // RendererMeshVertex
    ListArray indices;  // RendererMeshIndex
} RendererMesh;

/// @brief The camera object for the renderer.
typedef struct RendererCamera
{
    String name;
    bool isPerspective;
    float fov;
    float orthographicSize;
    RendererObjectTransform transform;
    mat4 viewMatrix;
    mat4 projectionMatrix;
} RendererCamera;

/// @brief A scene of render objects of the same format that share the same vertex array object (VAO) and vertex buffer object (VBO). The scene is resizable but object's vertices or indices are not because it holds one big mesh for all objects. Scene is only updatable all at once.
typedef struct RendererScene
{
    String name;
    RendererMesh mesh;
    RendererCamera *camera;
    ListArray modelMatrices; // mat4

    RendererVAOHandle vao;
    RendererVBOHandle vboModelVertices;
    RendererIBOHandle iboModelIndices;
    RendererUBOHandle uboModelMatrices; //! updated per frame. Contains projection and view matrices for camera and all other model matrices. Must match with uniform block in the vertex shader.

    RendererUniformHandle matricesBlockHandle;
} RendererScene;

/// @brief A render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the scene. Must be used with RendererScene. Not updatable on it's own.
typedef struct RendererObject
{
    String name;
    RendererObjectTransform transform;
    RendererScene *scene;

    size_t vertexOffsetInScene;
    size_t vertexCountInScene;

    size_t indexOffsetInScene;
    size_t indexCountInScene;

    size_t matrixOffsetInScene;
} RendererObject;

// typedef enum RendererShaderType
//{
//     MeshVertex,
//     Fragment,
//     Compute,
//     Geometry
// } RendererShaderType;
//
// typedef struct RendererShader
//{
//     RendererShaderType type;
//     RendererShaderHandle shader;
// } RendererShader;

#pragma endregion typedefs

#pragma region Renderer

/// @brief Initializer for renderer. Initializes OpenGL and GLFW. Sets options. Creates main window. Should be called before any renderer function.
/// @param title Window title.
/// @param windowSize Window size.
/// @param vertexShaderSource Source code of the main vertex shader.
/// @param fragmentShaderSource Source code of the main fragment shader.
/// @param mainCamera Pointer to the main camera object.
/// @param vSync Whether to enable vertical synchronization.
void Renderer_CreateContext(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource, bool vSync);

/// @brief Terminator for renderer.
void Renderer_Terminate();

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

/// @brief Sets the matrix with transform values.
/// @param transform Transform to reference. (pointer for performance)
/// @param matrix Matrix to edit.
void RendererObjectTransform_ToModelMatrix(RendererObjectTransform *transform, mat4 *matrix);

#pragma endregion Renderer Object Transform

#pragma region Renderer Mesh

/// @brief Creates a mesh from an OBJ file source.
/// @param objFileSource Source code of the OBJ file.
/// @return Created mesh with vertices and indices.
RendererMesh RendererMesh_CreateOBJ(String objFileSource);

/// @brief Creates an empty mesh with no vertices or indices.
/// @param initialVertexCapacity Initial capacity for the vertex array.
/// @param initialIndexCapacity Initial capacity for the index array.
/// @return Created empty mesh.
RendererMesh RendererMesh_CreateEmpty(size_t initialVertexCapacity, size_t initialIndexCapacity);

/// @brief Creates a copy of the given mesh with its own memory.
/// @param mesh Mesh to copy.
/// @return Copied new mesh.
RendererMesh RendererMesh_Copy(RendererMesh mesh);

/// @brief Destroyer function for renderer mesh.
/// @param mesh Mesh to destroy.
void RendererMesh_Destroy(RendererMesh *mesh);

#pragma endregion Renderer Mesh

#pragma region Renderer Camera

/// @brief Creates a camera object to control the view.
/// @param name Name of the camera.
/// @param fov Field of view of the camera.
/// @return Created camera object.
RendererCamera RendererCamera_Create(String name);

/// @brief Destroys a camera object.
/// @param camera Camera object to destroy.
void RendererCamera_Destroy(RendererCamera *camera);

/// @brief Configure the renderer camera.
/// @param camera Camera to configure
/// @param isPerspective Set the camera perspective or orthographic.
/// @param value fov if perspective, orthographicSize if orthographic.
void RendererCamera_Configure(RendererCamera *camera, bool isPerspective, float value);

/// @brief Updates the camera's properties to render.
/// @param camera Camera to update
void RendererCamera_Update(RendererCamera *camera);

#pragma endregion Renderer Camera

#pragma region Renderer Scene

/// @brief Creates a scene of render objects. A vertex allocator for objects.
/// @param name Name of the scene.
/// @param camera The main camera of the scene.
/// @param initialObjectCapacity Initial capacity for the model matrix array
/// @param initialVertexCapacity Initial capacity for the vertex array.
/// @param initialIndexCapacity Initial capacity for the index array.
/// @return Created scene of render objects.
RendererScene RendererScene_Create(String name, RendererCamera *camera, size_t initialObjectCapacity, size_t initialVertexCapacity, size_t initialIndexCapacity);

/// @brief Destroyer function for object scene
/// @param scene Scene to destroy
void RendererScene_Destroy(RendererScene *scene);

/// @brief Sets the main camera for the renderer.
/// @param scene Scene to set main camera.
/// @param camera Camera object to set as the main camera.
void RendererScene_SetMainCamera(RendererScene *scene, RendererCamera *camera);

/// @brief Update function for renderer scene. Updates its data to OpenGL context.
/// @param scene Scene to update data.
void RendererScene_Update(RendererScene *scene);

#pragma endregion Renderer Scene

#pragma region Renderer Object

/// @brief Creates a render object that shares its VAO and VBO with other objects in the scene.
/// @param name Name of the render object.
/// @param scene Pointer to the scene that the object belongs to.
/// @param mesh Mesh data of the object.
/// @return Created render object.
RendererObject RendererObject_Create(String name, RendererScene *scene, RendererMesh mesh);

/// @brief Destroyer function for render object
/// @param object Object to destroy
void RendererObject_Destroy(RendererObject *object);

/// @brief Updater function for object transform, logic etc.
/// @param object Object to update. (pointer for performance)
void RendererObject_Update(RendererObject *object);

#pragma endregion Renderer Object
