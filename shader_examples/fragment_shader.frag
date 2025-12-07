#version 330 core

// Input from vertex shader
in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragUV;

// Material uniforms
uniform vec3 matAmbientColor;
uniform vec3 matDiffuseColor;
uniform vec3 matSpecularColor;
uniform vec3 matEmissiveColor;
uniform float matSpecularExponent;
uniform float matDissolve;
uniform sampler2D matDiffuseMap;
uniform int matHasDiffuseMap;

// Camera uniforms
uniform vec3 camPosition;

// Output color
out vec4 fragColor;

void main()
{
    // Base color from material or texture
    vec3 baseColor = matDiffuseColor;
    if (matHasDiffuseMap == 1)
    {
        baseColor = texture(matDiffuseMap, fragUV).rgb;
    }
    
    // Simple lighting calculation
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0)); // Fixed light direction
    
    // Ambient
    vec3 ambient = matAmbientColor * baseColor;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * baseColor;
    
    // Specular
    vec3 viewDir = normalize(camPosition - fragPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), matSpecularExponent);
    vec3 specular = spec * matSpecularColor;
    
    // Combine
    vec3 result = ambient + diffuse + specular + matEmissiveColor;
    
    // Apply dissolve (opacity)
    fragColor = vec4(result, matDissolve);
}
