#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/ListArray.h"
#include "utilities/ListArrayDynamic.h"
#include "utilities/Vectors.h"

typedef unsigned int RendererVAO;
typedef unsigned int RendererVBO;
typedef unsigned int RendererShader;
typedef unsigned int RendererShaderProgram;
typedef unsigned int RendererTexture;

typedef struct Vertex
{
    Vector3 position;
    Vector4 color;
} Vertex;

/// @brief A dynamic render object that have its own vertex array object (VAO) and vertex buffer object (VBO)
typedef struct RendererDynamicObject
{
    String name;
    Vector3 position;
    ListArray vertices;
    RendererVAO vao;
    RendererVBO vbo;
} RendererDynamicObject;

/// @brief A static render object that shares its vertex array object (VAO) and vertex buffer object (VBO) with other objects in the batch. Must be used with RendererBatch.
typedef struct RendererStaticObject
{
    String name;
    Vector3 position;
    Vertex *vertices;
    const size_t vertexCount;
} RendererStaticObject;

/// @brief A batch of static render objects that share the same vertex array object (VAO) and vertex buffer object (VBO). The batch is resizable but static object's vertices are not.
typedef struct RendererBatch
{
    String name;
    ListArrayDynamic staticObjects;
    RendererVAO vao;
    RendererVBO vbo;
} RendererBatch;

void Renderer_RenderDynamicObject(RendererDynamicObject object);

void Renderer_RenderDynamicObjects(RendererDynamicObject *objects, size_t objectCount);

void Renderer_RenderBatch(RendererBatch objects);

RendererDynamicObject *RendererDynamicObject_Create(String name, Vertex *vertices, size_t vertexCount);

RendererStaticObject *RendererStaticObject_Create(Vertex *vertices, size_t vertexCount);

RendererBatch *RendererBatch_Create(RendererStaticObject object, size_t batchSize);
