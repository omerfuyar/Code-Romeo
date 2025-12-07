#version 330 core

// Vertex attributes
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

// Uniform buffer object containing object transforms (position, rotation, scale)
// Must match RENDERER_BATCH_MAX_OBJECT_COUNT (256) in Renderer.h
layout(std140) uniform objectTransforms
{
    // Each ObjectTransform contains:
    // vec3 position (aligned to 16 bytes)
    // vec3 rotation (aligned to 16 bytes) 
    // vec3 scale (aligned to 16 bytes)
    // Total: 48 bytes per transform
    vec4 transforms[256 * 3]; // 256 objects * 3 vec4s per object (pos, rot, scale)
};

// Camera uniforms
uniform mat4 camProjectionMatrix;
uniform mat4 camViewMatrix;
uniform vec3 camPosition;
uniform vec3 camRotation;
uniform float camSize;
uniform int camIsPerspective;

// Output to fragment shader
out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragUV;

// Function to create a rotation matrix from Euler angles (in radians)
mat4 rotationMatrix(vec3 rotation)
{
    float cx = cos(rotation.x);
    float sx = sin(rotation.x);
    float cy = cos(rotation.y);
    float sy = sin(rotation.y);
    float cz = cos(rotation.z);
    float sz = sin(rotation.z);
    
    // Rotation order: X -> Y -> Z (same as CPU implementation)
    mat4 rotX = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, cx, sx, 0.0,
        0.0, -sx, cx, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    mat4 rotY = mat4(
        cy, 0.0, -sy, 0.0,
        0.0, 1.0, 0.0, 0.0,
        sy, 0.0, cy, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    mat4 rotZ = mat4(
        cz, sz, 0.0, 0.0,
        -sz, cz, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
    
    // Apply rotations in X -> Y -> Z order (same as glm_rotate calls in CPU code)
    return rotZ * rotY * rotX;
}

// Function to create a translation matrix
mat4 translationMatrix(vec3 position)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        position.x, position.y, position.z, 1.0
    );
}

// Function to create a scale matrix
mat4 scaleMatrix(vec3 scale)
{
    return mat4(
        scale.x, 0.0, 0.0, 0.0,
        0.0, scale.y, 0.0, 0.0,
        0.0, 0.0, scale.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main()
{
    // Extract transform data for this instance
    int baseIndex = gl_InstanceID * 3;
    vec3 position = transforms[baseIndex + 0].xyz;
    vec3 rotation = transforms[baseIndex + 1].xyz;
    vec3 scale = transforms[baseIndex + 2].xyz;
    
    // Build model matrix from individual transform components
    // Order: Scale -> Rotate -> Translate (same as CPU implementation)
    mat4 modelMatrix = translationMatrix(position) * rotationMatrix(rotation) * scaleMatrix(scale);
    
    // Calculate final position
    vec4 worldPosition = modelMatrix * vec4(vertexPosition, 1.0);
    gl_Position = camProjectionMatrix * camViewMatrix * worldPosition;
    
    // Pass data to fragment shader
    fragPosition = worldPosition.xyz;
    fragNormal = mat3(transpose(inverse(modelMatrix))) * vertexNormal;
    fragUV = vertexUV;
}
