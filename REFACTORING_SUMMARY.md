# Refactoring Summary: Matrix Calculation Moved to Shaders

## Overview

This document summarizes the changes made to the Code-Romeo renderer system to move matrix calculation from the CPU to the GPU (vertex shader).

## What Changed

### Before
- **CPU-Side Matrix Calculation**: The renderer calculated 4x4 transformation matrices on the CPU for each object
- **UBO Content**: Pre-calculated `mat4` model matrices were sent to the GPU
- **UBO Name**: `modelMatrices`
- **Data Structure**: `Resource_Matrix4 *objectMatrices` array

### After  
- **GPU-Side Matrix Calculation**: Transformation matrices are now calculated in the vertex shader
- **UBO Content**: Position, rotation, and scale vectors are sent to the GPU
- **UBO Name**: `objectTransforms`
- **Data Structure**: `ObjectTransform *objectTransforms` array

## Code Changes

### New Structure: ObjectTransform

```c
typedef struct ObjectTransform
{
    alignas(16) Vector3 position;  // 16 bytes (12 used + 4 padding)
    alignas(16) Vector3 rotation;  // 16 bytes (12 used + 4 padding)
    alignas(16) Vector3 scale;     // 16 bytes (12 used + 4 padding)
} ObjectTransform;  // Total: 48 bytes per object
```

**Note**: The `alignas(16)` directive ensures proper alignment for GPU std140 layout, where vec3 types are aligned to 16-byte boundaries.

### Modified Files

1. **src/systems/Renderer.c**
   - Added `ObjectTransform` structure
   - Changed `RENDERER_BATCH.components.objectMatrices` to `objectTransforms`
   - Updated `Renderer_Update()` to copy transform data instead of calculating matrices
   - Updated `Renderer_Render()` to upload `ObjectTransform` data to UBO
   - Updated batch creation/destruction to allocate `ObjectTransform` arrays
   - Changed UBO block name from `"modelMatrices"` to `"objectTransforms"`

### Shader Changes Required

Users of this framework need to update their vertex shaders to:

1. Change the UBO declaration from:
   ```glsl
   layout(std140) uniform modelMatrices
   {
       mat4 models[256];
   };
   ```

2. To:
   ```glsl
   layout(std140) uniform objectTransforms
   {
       vec4 transforms[256 * 3]; // pos, rot, scale for 256 objects
   };
   ```

3. Add matrix calculation functions (see `shader_examples/vertex_shader.vert`)

4. Calculate the model matrix in the vertex shader:
   ```glsl
   void main()
   {
       int baseIndex = gl_InstanceID * 3;
       vec3 position = transforms[baseIndex + 0].xyz;
       vec3 rotation = transforms[baseIndex + 1].xyz;
       vec3 scale = transforms[baseIndex + 2].xyz;
       
       mat4 modelMatrix = translationMatrix(position) * 
                         rotationMatrix(rotation) * 
                         scaleMatrix(scale);
       
       gl_Position = camProjectionMatrix * camViewMatrix * 
                    modelMatrix * vec4(vertexPosition, 1.0);
       // ...
   }
   ```

## Benefits

1. **Reduced CPU Load**: Eliminates matrix multiplication on the CPU
2. **Better GPU Utilization**: Leverages parallel processing power of the GPU
3. **Smaller Data Transfer**: Sending 3 vec3s (48 bytes) vs 1 mat4 (64 bytes) per object
4. **Simpler CPU Code**: No need for matrix math libraries on CPU for object transforms
5. **Cache Efficiency**: Smaller data structures can improve cache utilization

## Migration Guide

If you have existing code using this renderer:

1. **Update Your Shaders**: 
   - Use the example shaders in `shader_examples/` as a reference
   - Change UBO block name from `modelMatrices` to `objectTransforms`
   - Add matrix calculation code to vertex shader

2. **No C Code Changes Required**:
   - The API remains the same
   - Transform data is still set via position/rotation/scale references
   - Batch creation and rendering work identically

3. **Testing**:
   - Verify that objects render in the same positions/orientations as before
   - Check performance improvements (should see reduced CPU usage)

## Performance Considerations

- **GPU Overhead**: Modern GPUs are optimized for these calculations
- **Rotation Order**: The shader uses X→Y→Z rotation order (same as CPU implementation)
- **Inactive Objects**: Objects with zero scale won't be visible (set when inactive)
- **Max Objects**: Still limited to 256 objects per batch (RENDERER_BATCH_MAX_OBJECT_COUNT)

## Technical Details

### Memory Layout (std140)

The std140 layout rule requires vec3 to be aligned as if it were a vec4 (16 bytes). This is why we use `alignas(16)` in the C structure:

```
Offset  | Data
--------|------------------
0-11    | position.xyz
12-15   | padding
16-27   | rotation.xyz
28-31   | padding  
32-43   | scale.xyz
44-47   | padding
```

### Shader Access Pattern

```glsl
// For instance ID = N:
transforms[N * 3 + 0].xyz  // position
transforms[N * 3 + 1].xyz  // rotation
transforms[N * 3 + 2].xyz  // scale
```

## Example Usage

```c
// Initialize renderer
Renderer_Initialize(&window, 16);

// Configure shaders (use new vertex shader with matrix calculation)
Renderer_ConfigureShaders("shader_examples/vertex_shader.vert", 
                         "shader_examples/fragment_shader.frag");

// Rest of the code remains unchanged
RendererBatch batch = Renderer_BatchCreate("model.mdl", ...);
// ...
Renderer_Update();
Renderer_Render();
```

## Files Modified

- `src/systems/Renderer.c` - Core renderer implementation changes

## Files Added

- `shader_examples/vertex_shader.vert` - Example vertex shader with matrix calculation
- `shader_examples/fragment_shader.frag` - Example fragment shader
- `shader_examples/README.md` - Shader documentation
- `REFACTORING_SUMMARY.md` - This file

## Compatibility

- **OpenGL Version**: Requires OpenGL 3.3+ (same as before)
- **GLSL Version**: Example shaders use GLSL 330 core
- **Alignment**: Uses C11 `alignas` for proper GPU data alignment
