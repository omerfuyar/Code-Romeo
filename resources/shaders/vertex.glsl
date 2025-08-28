#version 330 core

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrices[RENDERER_BATCH_MAX_OBJECT_COUNT];

out vec4 vertexColor;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * modelMatrices[0] * vec4(aPos, 1.0);
    vertexColor = aColor;
}