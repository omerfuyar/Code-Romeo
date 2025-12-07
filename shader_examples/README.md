# Shader Examples for Refactored Renderer

## Overview

This directory contains example shader files that demonstrate the new rendering system where transformation matrices are calculated in the vertex shader instead of on the CPU.

## Key Changes

### Previous System (CPU-side)
- Model matrices were calculated on the CPU using position, rotation, and scale
- Pre-calculated 4x4 matrices were sent to GPU via Uniform Buffer Object (UBO)
- UBO name: `modelMatrices` containing `mat4` arrays

### New System (GPU-side)
- Position, rotation, and scale are sent separately to the GPU
- Model matrices are calculated in the vertex shader
- UBO name: `objectTransforms` containing position, rotation, scale vectors

## UBO Structure

The uniform buffer object now contains transform data structured as follows:

```glsl
layout(std140) uniform objectTransforms
{
    // Each object has 3 vec4s (48 bytes total per object):
    // - vec4[0].xyz = position (w unused, padding)
    // - vec4[1].xyz = rotation (w unused, padding)  
    // - vec4[2].xyz = scale (w unused, padding)
    vec4 transforms[256 * 3]; // 256 objects max
};
```

### C Structure (ObjectTransform)
```c
typedef struct ObjectTransform
{
    alignas(16) Vector3 position;  // 16 bytes (12 used + 4 padding)
    alignas(16) Vector3 rotation;  // 16 bytes (12 used + 4 padding)
    alignas(16) Vector3 scale;     // 16 bytes (12 used + 4 padding)
} ObjectTransform;  // Total: 48 bytes
```

## Matrix Calculation in Vertex Shader

The vertex shader now calculates the model matrix using these steps:

1. Extract transform data for the current instance using `gl_InstanceID`
2. Build individual transformation matrices:
   - Translation matrix from position
   - Rotation matrix from Euler angles (X → Y → Z order)
   - Scale matrix from scale vector
3. Combine matrices: `modelMatrix = translate * rotate * scale`
4. Apply to vertex position

**Important**: GLSL matrices use column-major order. The mat4 constructor takes values column by column, not row by row. All matrix constructors in the example shader are properly formatted for column-major layout.

## Usage

To use these shaders in your application:

1. Load the shaders using `Renderer_ConfigureShaders()`:
   ```c
   Renderer_ConfigureShaders("shader_examples/vertex_shader.vert", 
                            "shader_examples/fragment_shader.frag");
   ```

2. The renderer system will automatically:
   - Upload transform data to the UBO each frame
   - Use instanced rendering to draw all objects in a batch

## Benefits

- Reduces CPU workload (no matrix multiplication on CPU)
- Leverages GPU parallel processing power
- Simplifies data transfer to GPU
- Potentially better cache utilization with smaller data structures

## Notes

- The maximum number of objects per batch is defined by `RENDERER_BATCH_MAX_OBJECT_COUNT` (256)
- Rotation order matches the CPU implementation (X, Y, Z)
- Alignment is critical for UBO data (vec3 aligned to 16 bytes in std140 layout)
