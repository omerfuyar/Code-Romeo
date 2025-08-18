#version 330 core

#define RENDERER_SCENE_MAX_OBJECT_COUNT 1000

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

layout (std140) uniform matrices
{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrices[RENDERER_SCENE_MAX_OBJECT_COUNT];
};

out vec4 vertexColor;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * modelMatrices[gl_InstanceID] * vec4(aPos, 1.0);
    vertexColor = aColor;
}