#version 330 core

in vec2 fragUv;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D matDiffuseMap;
uniform bool matHasDiffuseMap;

out vec4 FragColor;

void main()
{
	FragColor = matHasDiffuseMap ? texture(matDiffuseMap, fragUv) : vec4(fragPosition, 1.0);
}
