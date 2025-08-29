#version 330 core

#define RENDERER_BATCH_MAX_OBJECT_COUNT 256 // 16 KB = 16,384 Bytes = 256 * 64

layout (location = 0) in vec3 aPos;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

layout(std140) uniform modelMatrices {
    mat4 models[RENDERER_BATCH_MAX_OBJECT_COUNT];
};

out vec4 vertexColor;

void main()
{
    gl_Position = projectionMatrix * viewMatrix * models[gl_InstanceID] * vec4(aPos, 1.0);
    vertexColor = vec4(aPos, 1.0);
}