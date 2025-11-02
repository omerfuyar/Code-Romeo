#pragma once

#include "RJGlobal.h"

#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/Vector.h"

#include "tools/Context.h"

#define RENDERER_OPENGL_CLEAR_COLOR 0.0f, 0.0f, 0.0f, 0.0f
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

#define RENDERER_MODEL_MAX_CHILD_MESH_COUNT 128
#define RENDERER_MODEL_LINE_MAX_TOKEN_COUNT 64
#define RENDERER_MODEL_MAX_MESH_COUNT 16

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256 //! MUST MATCH WITH VERTEX SHADER
#define RENDERER_BATCH_INITIAL_CAPACITY 16

#pragma region typedefs

/// @brief 4x4 matrix type.
typedef float Matrix4[4][4];

/// @brief Handle for a shader object.
typedef unsigned int RendererShaderHandle;
/// @brief Handle for a shader program object.
typedef unsigned int RendererShaderProgramHandle;
/// @brief Handle for a texture object.
typedef unsigned int RendererTextureHandle;

/// @brief Handle for a uniform location in a shader program.
typedef int RendererUniformLocationHandle;
/// @brief Handle for a uniform block in a shader program.
typedef unsigned int RendererUniformBlockHandle;

/// @brief Handle for a Vertex Array Object.
typedef unsigned int RendererVAOHandle;
/// @brief Handle for a Vertex Buffer Object.
typedef unsigned int RendererVBOHandle;
/// @brief Handle for an Index Buffer Object.
typedef unsigned int RendererIBOHandle;
/// @brief Handle for a Uniform Buffer Object.
typedef unsigned int RendererUBOHandle;

/// @brief Index of a mesh within a model.
typedef unsigned int RendererMeshIndex;

/// @brief A texture that holds image data for rendering.
typedef struct RendererTexture RendererTexture;

/// @brief A material that holds the rendering properties of a mesh.
typedef struct RendererMaterial
{
    String name;
    Vector3 ambientColor;
    Vector3 diffuseColor;
    Vector3 specularColor;
    Vector3 emissiveColor;
    RendererTexture *diffuseMap;
    float specularExponent;
    float refractionIndex;
    float dissolve;
    int illuminationModel;
} RendererMaterial;

/// @brief A mesh that holds indices to used vertices inside a model and a material.
typedef struct RendererMesh
{
    ListArray indices; // RendererMeshIndex
    RendererMaterial *material;
} RendererMesh;

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

    Matrix4 projectionMatrix;
    Matrix4 viewMatrix;

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
    RendererUniformLocationHandle matEmissiveColor;
    RendererUniformLocationHandle matSpecularExponent;
    RendererUniformLocationHandle matDissolve;
    RendererUniformLocationHandle matDiffuseMap;
    RendererUniformLocationHandle matHasDiffuseMap;
    RendererUniformBlockHandle objectMatricesHandle;
} RendererScene;

/// @brief A batch of render components that use the same model.
typedef struct RendererBatch
{
    RendererModel *model;
    ListArray components;     // RendererComponent
    ListArray objectMatrices; // Matrix4, data must be continuous and only matrices because it's directly sent to UBO

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
/// @param window Pointer to the context window to use as the main window.
/// @param initialTextureCapacity Initial capacity for the texture array.
/// @return A pointer to the created context / window.
void Renderer_Initialize(ContextWindow *window, size_t initialTextureCapacity);

/// @brief Terminator for renderer.
void Renderer_Terminate();

/// @brief Configures the main shaders for the renderer.
/// @param vertexShaderSource Source code of the main vertex shader.
/// @param fragmentShaderSource Source code of the main fragment shader.
void Renderer_ConfigureShaders(StringView vertexShaderSource, StringView fragmentShaderSource);

/// @brief Should be called before using rendering functions.
void Renderer_StartRendering();

/// @brief Should be called after using rendering functions.
void Renderer_FinishRendering();

/// @brief Renders a scene of objects.
/// @param scene The scene of objects to render.
void Renderer_RenderScene(RendererScene *scene);

#pragma endregion Renderer

#pragma region Renderer Debug

/// @brief Initialize function for renderer debug functions. Should be called after the Renderer_Initialize function.
/// @param vertexShaderSource The source file for debug vertex shader.
/// @param fragmentShaderSource The source file for debug fragment shader.
/// @param initialVertexCapacity The initial capacity for the vertex buffer.
void RendererDebug_Initialize(StringView vertexShaderSource, StringView fragmentShaderSource, size_t initialVertexCapacity);

/// @brief Terminator for renderer debug functions.
void RendererDebug_Terminate();

/// @brief Should be called before using debug rendering functions, and after main renderer's rendering functions.
void RendererDebug_StartRendering();

/// @brief Should be called before Renderer_FinishRendering to draw all debug shapes.
/// @param camProjectionMatrix The projection matrix of the camera.
/// @param camViewMatrix The view matrix of the camera.
void RendererDebug_FinishRendering(Matrix4 *camProjectionMatrix, Matrix4 *camViewMatrix);

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

#pragma region Renderer Material

/// @brief Creates materials from a material file (prefer .mat). File can contain multiple materials but textures of them will be ignored. Use RendererMaterial_CreateFromFileTextured and to create a material with texture.
/// @param matFileData Source code of the material file.
/// @param matFileLineCount Number of lines in the material file source.
/// @return Created material list type of the list is RendererMaterial*.
ListArray RendererMaterial_CreateFromFile(StringView matFileData, size_t matFileLineCount);

/// @brief Creates materials from a material file (prefer .mat) with the argument texture. It copies the texture data into OpenGL so the original data can be freed after this function.
/// @param matFileData Source code of the material file.
/// @param matFileLineCount Number of lines in the material file source.
/// @param textureName Name of the texture to use for the material. If the name is found in the internal texture pool, it will use that texture. Ignored if cannot find and textureData is NULL.
/// @param textureData Pointer to the texture data to use for the material.
/// @param textureSize Size of the texture.
/// @param textureChannels Number of channels in the texture.
/// @return Created material list type of the list is RendererMaterial*.
ListArray RendererMaterial_CreateFromFileTextured(StringView matFileData, size_t matFileLineCount, StringView textureName, const void *textureData, Vector2Int textureSize, int textureChannels);

/// @brief Destroyer function for renderer material.
/// @param material Material to destroy.
void RendererMaterial_Destroy(RendererMaterial *material);

#pragma endregion Renderer Material

#pragma region Renderer Model

// todo fix docs
/// @brief Creates a model from an MDL file source. The .obj and its other files (like .mtl) must be in the same directory. Only supports models with triangular faces. doesn't support objects with normal maps but without UVs (x//x signature).
/// @param mdlFileData Source code of the OBJ file.
/// @param mdlFileLineCount Number of lines in the OBJ file source.
/// @param materialPool Pointer to a list array of material pointer pointers (RendererMaterial **) to use for the model.
/// @param positionOffset Position offset to freely adjust final model position.
/// @param rotationOffset Rotation offset to freely adjust final model rotation.
/// @param scaleOffset Scale offset to freely adjust final model scale.
/// @return Created model with vertices and indices.
RendererModel *RendererModel_Create(StringView mdlFileData, size_t mdlFileLineCount, const ListArray *materialPool, Vector3 positionOffset, Vector3 rotationOffset, Vector3 scaleOffset);

// todo fix docs
/// @brief Creates a model from an MDL file source. The .obj and its other files (like .mtl) must be in the same directory. Only supports models with triangular faces. doesn't support objects with normal maps but without UVs (x//x signature).
/// @param mdlFileData Source code of the OBJ file.
/// @param mdlFileLineCount Number of lines in the OBJ file source.
/// @param materialPool Pointer to a list array of material pointer pointers (RendererMaterial **) to use for the model.
/// @return Created model with vertices and indices.
ListArray RendererModel_CreateFromFile(StringView mdlFileData, size_t mdlFileLineCount, const ListArray *materialPool);

/// @brief Destroyer function for renderer model.
/// @param model Model to destroy.
void RendererModel_Destroy(RendererModel *model);

#pragma endregion Renderer Model

#pragma region Renderer Scene

/// @brief Creates a scene of render objects that will be managed together.
/// @param name Name of the scene.
/// @param initialBatchCapacity The initial capacity of the batch list in scene.
/// @return Created scene of render objects.
RendererScene *RendererScene_CreateEmpty(StringView name, size_t initialBatchCapacity);

// todo fix docs
/// @brief
/// @param scnFileData
/// @param scnFileLineCount
/// @param modelPool
/// @param objectReferences
/// @param transformOffsetInObject
/// @param totalObjectSize
/// @param objectCount
/// @return
RendererScene *RendererScene_CreateFromFile(StringView scnFileData, size_t scnFileLineCount, const ListArray *modelPool, void *objectReferences, size_t transformOffsetInObject, size_t totalObjectSize, size_t objectCount);

/// @brief Destroyer function for object scene
/// @param scene Scene to destroy
void RendererScene_Destroy(RendererScene *scene);

/// @brief Sets the main camera for the renderer.
/// @param scene Scene to set main camera.
/// @param camera Camera object to set as the main camera.
void RendererScene_SetMainCamera(RendererScene *scene, RendererCameraComponent *camera);

/// @brief Creates a renderer batch to store components that use the same model.
/// @param scene Pointer to the scene that the batch will belong to.
/// @param model Pointer to the model that the components in the batch will use.
/// @param initialComponentCapacity The initial capacity for components inside the batch.
/// @return The created batch.
RendererBatch *RendererScene_CreateBatch(RendererScene *scene, RendererModel *model, size_t initialComponentCapacity);

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
void RendererBatch_DestroyComponent(RendererComponent *component); // todo not logical

#pragma endregion Renderer Batch

#pragma region Renderer Camera

/// @brief Creates a camera object to control the view. Changes the scene.
/// @param scene Scene to attach camera.
/// @return Created camera object.
RendererCameraComponent *RendererCameraComponent_Create(Vector3 *positionReference, Vector3 *rotationReference);

/// @brief Destroys a camera object.
/// @param camera Camera object to destroy.
void RendererCameraComponent_Destroy(RendererCameraComponent *camera);

#pragma endregion Renderer Camera
