#version 330 core

in vec2 fragUv;
in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D matDiffuseMap;
uniform bool matHasDiffuseMap;

uniform vec3 matAmbientColor;
uniform vec3 matDiffuseColor;
uniform vec3 matSpecularColor;
uniform float matSpecularExponent;
uniform float matDissolve;

out vec4 FragColor;

void main()
{
    // Normalize the fragment normal
    vec3 normal = normalize(fragNormal);
    
    // Hard-coded light direction (could be passed as uniform)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    
    // Hard-coded camera position (could be passed as uniform)
    vec3 viewDir = normalize(vec3(0.0, 0.0, 0.0) - fragPosition);
    
    // Calculate the reflection direction
    vec3 reflectDir = reflect(-lightDir, normal);
    
    // Calculate ambient component
    vec3 ambient = matAmbientColor * 0.2;
    
    // Calculate diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * matDiffuseColor;
    
    // Calculate specular component
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), matSpecularExponent);
    vec3 specular = spec * matSpecularColor;
    
    // Get base color from texture or material
    vec3 baseColor;
    if (matHasDiffuseMap) {
        baseColor = texture(matDiffuseMap, fragUv).rgb;
    } else {
        baseColor = matDiffuseColor;
    }
    
    // Combine all lighting components
    vec3 result = (ambient + diffuse) * baseColor + specular;
    
    // Apply material opacity
    FragColor = vec4(result, matDissolve);
}
