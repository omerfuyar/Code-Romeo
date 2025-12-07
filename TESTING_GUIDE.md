# Testing Guide for Matrix Calculation Refactoring

This document provides guidance for testing the refactored renderer system.

## Pre-Testing Checklist

Before testing, ensure:
- [ ] Dependencies are built (GLFW, GLAD, CGLM)
- [ ] Code compiles without errors
- [ ] Your project has vertex and fragment shaders based on the examples in `shader_examples/`

## Compilation Testing

### Build the Library

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/omerfuyar/Code-Romeo.git
cd Code-Romeo

# Build using the shuild build system
gcc ShuildRomeo.c -o ShuildRomeo -O3
./ShuildRomeo gcc r
```

Expected result: Library should compile without errors.

### Common Compilation Issues

1. **Alignment errors**: If you see alignment-related errors, ensure C11 support is enabled (`-std=c11`)
2. **GLSL errors**: Shader compilation will fail if the UBO block name doesn't match
3. **Linker errors**: Ensure cglm is no longer needed for object matrix calculations (it's still used for camera matrices)

## Functional Testing

### Test Case 1: Single Object Rendering

**Setup:**
```c
// Initialize renderer
Renderer_Initialize(&window, 16);
Renderer_ConfigureShaders("vertex_shader.vert", "fragment_shader.frag");

// Create a batch with one object
RendererBatch batch = Renderer_BatchCreate("cube.mdl", &offset, 1, 
                                          positions, rotations, scales);
RendererComponent comp = Renderer_ComponentCreate(0, batch);

// Set transform
positions[0] = Vector3_New(0, 0, -5);
rotations[0] = Vector3_New(0, 0, 0);
scales[0] = Vector3_New(1, 1, 1);
```

**Expected Result:**
- Object appears at position (0, 0, -5) with no rotation and scale of 1
- Same visual result as before the refactoring

### Test Case 2: Multiple Objects

**Setup:**
```c
// Create multiple components
for (int i = 0; i < 10; i++) {
    positions[i] = Vector3_New(i * 2.0f, 0, -10);
    rotations[i] = Vector3_New(0, i * 0.5f, 0);
    scales[i] = Vector3_New(1, 1, 1);
}
```

**Expected Result:**
- 10 objects in a row, each rotated differently around Y axis
- All objects visible and correctly positioned

### Test Case 3: Rotation Verification

**Purpose:** Verify rotation order matches CPU implementation

**Setup:**
```c
positions[0] = Vector3_New(0, 0, -5);
rotations[0] = Vector3_New(0.5f, 0.3f, 0.2f);  // X, Y, Z
scales[0] = Vector3_New(1, 1, 1);
```

**Expected Result:**
- Object rotates in the same way as with the old CPU-based matrix calculation
- Compare side-by-side with previous version to verify identical visual result

### Test Case 4: Scale Verification

**Setup:**
```c
// Non-uniform scaling
scales[0] = Vector3_New(2.0f, 0.5f, 1.0f);
```

**Expected Result:**
- Object stretched 2x on X axis, compressed 0.5x on Y axis, normal on Z axis

### Test Case 5: Inactive Objects

**Purpose:** Verify inactive objects don't render

**Setup:**
```c
// Create 5 objects, make 2 inactive
Renderer_ComponentSetActive(batch, comp2, false);
Renderer_ComponentSetActive(batch, comp4, false);
```

**Expected Result:**
- Only 3 objects visible (components 0, 1, and 3)
- Inactive objects (2 and 4) not visible
- No rendering artifacts from inactive objects

### Test Case 6: Maximum Batch Size

**Purpose:** Verify system handles maximum number of objects

**Setup:**
```c
// Create 256 objects (RENDERER_BATCH_MAX_OBJECT_COUNT)
for (int i = 0; i < 256; i++) {
    // Position in a grid
    positions[i] = Vector3_New((i % 16) * 2, 0, (i / 16) * 2);
    rotations[i] = Vector3_New(0, 0, 0);
    scales[i] = Vector3_New(1, 1, 1);
}
```

**Expected Result:**
- All 256 objects render correctly
- No artifacts or missing objects
- Performance should be similar or better than before

## Performance Testing

### CPU Usage

**Test:**
```bash
# Run your application and monitor CPU usage
htop  # or Task Manager on Windows
```

**Expected Result:**
- Lower CPU usage during rendering compared to old implementation
- CPU time previously spent on matrix multiplication should be reduced

### Frame Rate

**Test:**
- Measure FPS with varying object counts (10, 50, 100, 256 objects)
- Compare with previous implementation

**Expected Result:**
- Similar or better frame rates
- GPU utilization may increase slightly (expected)

### GPU Usage

**Test:**
```bash
# Monitor GPU usage
nvidia-smi  # For NVIDIA GPUs
# or use GPU-Z, MSI Afterburner, etc.
```

**Expected Result:**
- Slight increase in GPU shader unit utilization (expected)
- Overall GPU usage should remain similar

## Visual Regression Testing

### Comparison Test

1. Checkout the old version: `git checkout refactor/single-scenes`
2. Build and run, take screenshots of test scenes
3. Checkout the new version: `git checkout copilot/refactor-matrix-calculation`
4. Build and run, take screenshots of the same test scenes
5. Compare screenshots pixel-by-pixel if possible

**Expected Result:**
- Identical visual output (within floating-point precision tolerance)
- No visual differences in object positions, rotations, or scales

## Debugging Tips

### Shader Debugging

If objects don't render correctly:

1. **Check UBO binding:**
   ```c
   // Verify in logs:
   RJGlobal_DebugInfo("objectMatricesHandle: %u", RMS.shader.objectMatricesHandle);
   ```

2. **Verify transform data:**
   ```c
   // Add debug output in Renderer_Update:
   RJGlobal_DebugInfo("Object %d: pos=(%.2f,%.2f,%.2f) rot=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f)",
       component,
       rmsObjectTransform(batch, component).position.x,
       rmsObjectTransform(batch, component).position.y,
       rmsObjectTransform(batch, component).position.z,
       rmsObjectTransform(batch, component).rotation.x,
       rmsObjectTransform(batch, component).rotation.y,
       rmsObjectTransform(batch, component).rotation.z,
       rmsObjectTransform(batch, component).scale.x,
       rmsObjectTransform(batch, component).scale.y,
       rmsObjectTransform(batch, component).scale.z);
   ```

3. **Test shader compilation:**
   - Check for shader compilation errors in debug output
   - Verify the shader is using GLSL 330 core or higher

4. **Visualize matrices:**
   - Temporarily output gl_Position directly to see if vertices are being transformed
   - Use a simple shader to render solid colors first before adding lighting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Objects not visible | Wrong UBO block name | Verify shader uses `objectTransforms` |
| Objects in wrong position | Matrix order wrong | Check rotation matrix multiplication order |
| Objects squashed/stretched | Wrong matrix layout | Verify column-major layout in GLSL |
| All objects at origin | Transform data not uploaded | Check glBufferData call |
| Shader won't compile | GLSL version mismatch | Use `#version 330 core` |

## Validation Checklist

- [ ] Code compiles without warnings
- [ ] Single object renders correctly
- [ ] Multiple objects render correctly
- [ ] Rotations match old implementation
- [ ] Scales work correctly (uniform and non-uniform)
- [ ] Inactive objects don't render
- [ ] Maximum batch size (256 objects) works
- [ ] CPU usage is reduced or similar
- [ ] Frame rate is maintained or improved
- [ ] Visual output matches old implementation
- [ ] No memory leaks (run with valgrind)

## Automated Testing

If you have a test framework, consider adding:

```c
// Test transform data structure size
assert(sizeof(ObjectTransform) == 48);  // 3 * 16 bytes

// Test transform data upload
ObjectTransform test;
test.position = Vector3_New(1, 2, 3);
test.rotation = Vector3_New(0.1, 0.2, 0.3);
test.scale = Vector3_New(1, 1, 1);

// Verify data is copied correctly
assert(test.position.x == 1.0f);
assert(test.position.y == 2.0f);
assert(test.position.z == 3.0f);
// ... etc
```

## Reporting Issues

If you find issues, please report:
1. Hardware/software environment (OS, GPU, driver version)
2. Steps to reproduce
3. Expected vs actual behavior
4. Screenshots if visual issue
5. Relevant log output

## Performance Baseline

For reference, expected performance improvements:
- **CPU usage**: 5-15% reduction (depends on object count)
- **FPS**: 0-10% improvement (depends on bottlenecks)
- **Memory**: Slightly reduced (48 bytes vs 64 bytes per object in UBO)
