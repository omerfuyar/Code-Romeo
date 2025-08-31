#version 330 core

in vec2 verUv;

uniform sampler2D matDiffuseMap;

out vec4 FragColor;

void main()
{
	FragColor = texture(matDiffuseMap, verUv);
}
