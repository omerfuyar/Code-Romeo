#version 330 core

layout (location = 0) in vec3 aPos;   // Vertex position attribute
layout (location = 1) in vec3 aColor; // Vertex color attribute

out vec3 ourColor;                 // Output color to fragment shader

void main()
{
   gl_Position = vec4(aPos, 1.0);
   ourColor = aColor;
}