#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/ListArrayDynamic.h"
#include "utilities/Vectors.h"

#define OPENGL_VERSION_MAJOR 3
#define OPENGL_VERSION_MINOR 3

#pragma region typedefs

typedef unsigned int RendererVAOHandle;
typedef unsigned int RendererVBOHandle;
typedef unsigned int RendererShaderHandle;
typedef unsigned int RendererShaderProgramHandle;
typedef unsigned int RendererTextureHandle;

/// @brief Represents the transformation (position, rotation, scale) of an object in 3D space.
typedef struct ObjectTransform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
} ObjectTransform;

//! LAYOUT OF MEMBERS IN THE STRUCT MUST MATCH THE OPENGL ATTRIBUTE LAYOUT IN SHADER AND ATTRIBUTE SETUPS
/// @brief Represents a primal vertex in 3D space.
typedef struct Vertex
{
    Vector3 position;
    Vector4 color;
} Vertex;

#define VERTEX_MEMBER_POSITION_INDEX 0
#define VERTEX_MEMBER_COLOR_INDEX 1

/// @brief A dynamic render object that have its own vertex array object (VAO) and vertex buffer object (VBO)
typedef struct RendererDynamicObject
{
    String name;
    ObjectTransform transform;
    ListArray vertices;
    RendererVAOHandle vao;
    RendererVBOHandle vbo;
} RendererDynamicObject;

/// @brief A batch of static render objects of the same format that share the same vertex array object (VAO) and vertex buffer object (VBO). The batch is resizable but static object's vertices are not.
typedef struct RendererBatch
{
    String name;
    ListArray vertices;
    RendererVAOHandle vao;
    RendererVBOHandle vbo;
} RendererBatch;

/// @brief A static render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the batch. Must be used with RendererBatch.
typedef struct RendererStaticObject
{
    String name;
    ObjectTransform transform;
    RendererBatch *batch;
    size_t vertexOffsetInBatch;
    size_t vertexCountInBatch;
} RendererStaticObject;

// typedef enum RendererShaderType
//{
//     Vertex,
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

#pragma region Object Transform

/// @brief Sets the position of the object transform.
/// @param transform Pointer to the object transform.
/// @param position New position for the object.
void ObjectTransform_SetPosition(ObjectTransform *transform, Vector3 position);

/// @brief Sets the rotation of the object transform.
/// @param transform Pointer to the object transform.
/// @param rotation New rotation for the object.
void ObjectTransform_SetRotation(ObjectTransform *transform, Vector3 rotation);

/// @brief Sets the scale of the object transform.
/// @param transform Pointer to the object transform.
/// @param scale New scale for the object.
void ObjectTransform_SetScale(ObjectTransform *transform, Vector3 scale);

#pragma endregion Object Transform

#pragma region Renderer Dynamic Object

/// @brief Creates a dynamic render object with its own VAO and VBO.
/// @param name Name of the dynamic render object.
/// @param vertices Array of vertices for the dynamic render object.
/// @param vertexCount Number of vertices in the array.
/// @return Created dynamic render object.
RendererDynamicObject RendererDynamicObject_Create(String name, Vertex *vertices, size_t vertexCount);

/// @brief Destroyer function for dynamic render object
/// @param object Object to destroy
void RendererDynamicObject_Destroy(RendererDynamicObject *object);

/// @brief Update function for dynamic renderer object. Updates its data to OpenGL context.
/// @param object Object to update data.
void RendererDynamicObject_Update(RendererDynamicObject object);

#pragma endregion Renderer Dynamic Object

#pragma region Renderer Batch

/// @brief Creates a batch of static render objects. A vertex allocator for static objects.
/// @param name Name of the batch.
/// @param initialVertexCapacity Initial capacity for the vertex array.
/// @return Created batch of static render objects.
RendererBatch RendererBatch_Create(String name, size_t initialVertexCapacity);

/// @brief Destroyer function for object batch
/// @param batch Batch to destroy
void RendererBatch_Destroy(RendererBatch *batch);

/// @brief Update function for renderer batch. Updates its data to OpenGL context.
/// @param batch Batch to update data.
void RendererBatch_Update(RendererBatch batch);

#pragma endregion Renderer Batch

#pragma region Renderer Static Object

/// @brief Creates a static render object that shares its VAO and VBO with other objects in the batch.
/// @param name Name of the static render object.
/// @param batch Pointer to the batch that the static object belongs to.
/// @param vertices Array of vertices for the static render object.
/// @param vertexCount Number of vertices in the array.
/// @return Created static render object.
RendererStaticObject RendererStaticObject_Create(String name, RendererBatch *batch, Vertex *vertices, size_t vertexCount);

/// @brief Destroyer function for static render object
/// @param object Object to destroy
void RendererStaticObject_Destroy(RendererStaticObject *object);

#pragma endregion Renderer Static Object

#pragma region Renderer

/// @brief Initializer for renderer. Initializes OpenGL and GLFW. Sets options. Creates main window.
/// @param title Window title.
/// @param windowSize Window size.
/// @param vertexShaderSource Source code of the main vertex shader.
/// @param fragmentShaderSource Source code of the main fragment shader.
void Renderer_Initialize(String title, Vector2Int windowSize, String vertexShaderSource, String fragmentShaderSource);

/// @brief Terminator for renderer.
void Renderer_Terminate();

/// @brief Should be called before using rendering functions.
void Renderer_StartRendering();

/// @brief Should be called after using rendering functions.
void Renderer_FinishRendering();

/// @brief Renders a dynamic object.
/// @param object The dynamic object to render.
void Renderer_RenderDynamicObject(RendererDynamicObject object);

/// @brief Renders a batch of static objects.
/// @param batch The batch of static objects to render.
void Renderer_RenderBatch(RendererBatch batch);

#pragma endregion Renderer
