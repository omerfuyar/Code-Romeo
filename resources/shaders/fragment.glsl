#version 330 core

out vec4 FragColor;

in vec3 ourColor; // Input color from vertex shader

void main()
{
   FragColor = vec4(ourColor, 1.0f);
}