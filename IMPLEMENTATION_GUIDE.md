# Implementation Guide: Handle-Based Component System

This guide provides detailed implementation steps for migrating from pointer-based to handle-based component management.

## Table of Contents
1. [Core Infrastructure](#core-infrastructure)
2. [Step-by-Step Implementation](#step-by-step-implementation)
3. [Code Examples](#code-examples)
4. [Migration Path](#migration-path)
5. [Testing Strategy](#testing-strategy)

## Core Infrastructure

### 1. Define Handle Types

Create a new header file: `include/utilities/ComponentHandle.h`

```c
#pragma once

#include "RJGlobal.h"

/// @brief Opaque handle to a component. Use system-specific functions to access.
typedef struct ComponentHandle
{
    uint32_t index;      // Index into component pool
    uint32_t generation; // Generation counter for validation
} ComponentHandle;

/// @brief Invalid/null handle constant
#define COMPONENT_HANDLE_INVALID ((ComponentHandle){UINT32_MAX, UINT32_MAX})

/// @brief Check if handle is valid (not null)
#define ComponentHandle_IsNull(handle) \
    ((handle).index == UINT32_MAX && (handle).generation == UINT32_MAX)

/// @brief Compare two handles for equality
#define ComponentHandle_Equals(a, b) \
    ((a).index == (b).index && (a).generation == (b).generation)
```

### 2. Create Component Pool System

Create `include/utilities/ComponentPool.h`:

```c
#pragma once

#include "RJGlobal.h"
#include "utilities/ListArray.h"
#include "utilities/ComponentHandle.h"

/// @brief A slot in the component pool
typedef struct ComponentSlot
{
    void *data;          // Pointer to component data
    uint32_t generation; // Generation counter, incremented on reuse
    bool isAlive;        // Whether this slot is currently in use
} ComponentSlot;

/// @brief Pool for managing components with handle-based access
typedef struct ComponentPool
{
    ListArray slots;     // ComponentSlot array
    ListArray freeList;  // uint32_t array of free slot indices
    size_t componentSize;
    char *componentTypeName;
} ComponentPool;

/// @brief Create a new component pool
/// @param componentTypeName Name of component type for debugging
/// @param componentSize Size of component in bytes
/// @param initialCapacity Initial number of slots to allocate
/// @return Initialized component pool
ComponentPool ComponentPool_Create(const char *componentTypeName, size_t componentSize, size_t initialCapacity);

/// @brief Destroy a component pool and all its components
/// @param pool Pool to destroy
void ComponentPool_Destroy(ComponentPool *pool);

/// @brief Allocate a new component and return its handle
/// @param pool Pool to allocate from
/// @return Handle to the new component
ComponentHandle ComponentPool_Allocate(ComponentPool *pool);

/// @brief Free a component by its handle
/// @param pool Pool to free from
/// @param handle Handle of component to free
void ComponentPool_Free(ComponentPool *pool, ComponentHandle handle);

/// @brief Get pointer to component data from handle
/// @param pool Pool containing the component
/// @param handle Handle to resolve
/// @return Pointer to component data, or NULL if handle is invalid
void *ComponentPool_Get(const ComponentPool *pool, ComponentHandle handle);

/// @brief Check if a handle is valid
/// @param pool Pool to check against
/// @param handle Handle to validate
/// @return True if handle is valid and component is alive
bool ComponentPool_IsValid(const ComponentPool *pool, ComponentHandle handle);

/// @brief Get number of alive components in pool
/// @param pool Pool to query
/// @return Number of alive components
size_t ComponentPool_GetAliveCount(const ComponentPool *pool);

/// @brief Iterate over all alive components
/// @param pool Pool to iterate
/// @param callback Function to call for each alive component
/// @param userData User data to pass to callback
typedef void (*ComponentPool_IterateCallback)(ComponentHandle handle, void *component, void *userData);
void ComponentPool_Iterate(const ComponentPool *pool, ComponentPool_IterateCallback callback, void *userData);
```

Create `src/utilities/ComponentPool.c`:

```c
#include "utilities/ComponentPool.h"

ComponentPool ComponentPool_Create(const char *componentTypeName, size_t componentSize, size_t initialCapacity)
{
    ComponentPool pool;
    pool.componentSize = componentSize;
    
    size_t nameLen = strlen(componentTypeName);
    pool.componentTypeName = (char *)malloc(nameLen + 1);
    RJGlobal_DebugAssertNullPointerCheck(pool.componentTypeName);
    strcpy(pool.componentTypeName, componentTypeName);
    
    pool.slots = ListArray_Create("ComponentSlot", sizeof(ComponentSlot), initialCapacity);
    pool.freeList = ListArray_Create("uint32_t", sizeof(uint32_t), initialCapacity);
    
    RJGlobal_DebugInfo("ComponentPool for '%s' created with capacity %zu", componentTypeName, initialCapacity);
    return pool;
}

void ComponentPool_Destroy(ComponentPool *pool)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    
    // Free all component data
    for (size_t i = 0; i < pool->slots.count; i++)
    {
        ComponentSlot *slot = (ComponentSlot *)ListArray_Get(&pool->slots, i);
        if (slot->data != NULL)
        {
            free(slot->data);
            slot->data = NULL;
        }
    }
    
    ListArray_Destroy(&pool->slots);
    ListArray_Destroy(&pool->freeList);
    
    free(pool->componentTypeName);
    pool->componentTypeName = NULL;
    
    RJGlobal_DebugInfo("ComponentPool destroyed");
}

ComponentHandle ComponentPool_Allocate(ComponentPool *pool)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    
    ComponentHandle handle;
    ComponentSlot *slot;
    
    // Try to reuse a free slot
    if (pool->freeList.count > 0)
    {
        // Get the index value before it's removed
        uint32_t *freeIndex = (uint32_t *)ListArray_Get(&pool->freeList, pool->freeList.count - 1);
        handle.index = *freeIndex;
        ListArray_Pop(&pool->freeList);  // Remove after reading
        
        slot = (ComponentSlot *)ListArray_Get(&pool->slots, handle.index);
        
        // Increment generation to invalidate old handles
        slot->generation++;
        handle.generation = slot->generation;
    }
    else
    {
        // Allocate new slot
        handle.index = (uint32_t)pool->slots.count;
        handle.generation = 0;
        
        ComponentSlot newSlot = {0};
        newSlot.generation = 0;
        newSlot.isAlive = false;
        newSlot.data = NULL;
        
        slot = (ComponentSlot *)ListArray_Add(&pool->slots, &newSlot);
    }
    
    // Allocate component data
    if (slot->data == NULL)
    {
        slot->data = malloc(pool->componentSize);
        RJGlobal_DebugAssertNullPointerCheck(slot->data);
    }
    
    // Clear component data
    memset(slot->data, 0, pool->componentSize);
    slot->isAlive = true;
    
    return handle;
}

void ComponentPool_Free(ComponentPool *pool, ComponentHandle handle)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    
    if (!ComponentPool_IsValid(pool, handle))
    {
        RJGlobal_DebugWarning("Attempted to free invalid component handle (index=%u, gen=%u)",
                              handle.index, handle.generation);
        return;
    }
    
    ComponentSlot *slot = (ComponentSlot *)ListArray_Get(&pool->slots, handle.index);
    slot->isAlive = false;
    
    // Add to free list for reuse
    ListArray_Add(&pool->freeList, &handle.index);
}

void *ComponentPool_Get(const ComponentPool *pool, ComponentHandle handle)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    
    if (!ComponentPool_IsValid(pool, handle))
    {
        return NULL;
    }
    
    ComponentSlot *slot = (ComponentSlot *)ListArray_Get(&pool->slots, handle.index);
    return slot->data;
}

bool ComponentPool_IsValid(const ComponentPool *pool, ComponentHandle handle)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    
    if (handle.index >= pool->slots.count)
    {
        return false;
    }
    
    ComponentSlot *slot = (ComponentSlot *)ListArray_Get(&pool->slots, handle.index);
    return slot->isAlive && slot->generation == handle.generation;
}

size_t ComponentPool_GetAliveCount(const ComponentPool *pool)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    return pool->slots.count - pool->freeList.count;
}

void ComponentPool_Iterate(const ComponentPool *pool, ComponentPool_IterateCallback callback, void *userData)
{
    RJGlobal_DebugAssertNullPointerCheck(pool);
    RJGlobal_DebugAssertNullPointerCheck(callback);
    
    for (size_t i = 0; i < pool->slots.count; i++)
    {
        ComponentSlot *slot = (ComponentSlot *)ListArray_Get(&pool->slots, i);
        if (slot->isAlive)
        {
            ComponentHandle handle = {(uint32_t)i, slot->generation};
            callback(handle, slot->data, userData);
        }
    }
}
```

## Step-by-Step Implementation

### Phase 1: Add Component Pool to Renderer System

#### Step 1.1: Update RendererBatch Structure

In `include/systems/Renderer.h`, modify the RendererBatch structure:

```c
/// @brief A batch of render components that use the same model.
typedef struct RendererBatch
{
    RendererModel *model;
    ComponentPool componentPool;  // CHANGED: from ListArray components
    ListArray objectMatrices; // Renderer_Matrix4, stays the same for now
    
    RendererScene *scene;
    size_t batchOffsetInScene;
} RendererBatch;
```

#### Step 1.2: Update RendererComponent Structure

```c
/// @brief A render object that shares its vertex array object (VAO) and vertex buffer object (VBO) 
/// with other objects in the scene.
typedef struct RendererComponent
{
    Vector3 *positionReference;
    Vector3 *rotationReference;
    Vector3 *scaleReference;
    
    RendererBatch *batch;
    size_t componentOffsetInBatch;  // Still needed for objectMatrices index
} RendererComponent;
```

#### Step 1.3: Update API Functions

Change function signatures in `include/systems/Renderer.h`:

```c
// OLD:
// RendererComponent *RendererBatch_CreateComponent(RendererBatch *batch, ...);
// void RendererBatch_DestroyComponent(RendererComponent *component);

// NEW:
/// @brief Creates a component in the specified batch.
/// @param batch The batch to create the component in.
/// @return Handle to the newly created component.
ComponentHandle RendererBatch_CreateComponent(RendererBatch *batch, 
                                               Vector3 *positionReference, 
                                               Vector3 *rotationReference, 
                                               Vector3 *scaleReference);

/// @brief Destroys a component and frees its resources.
/// @param batch The batch containing the component.
/// @param handle Handle to the component to destroy.
void RendererBatch_DestroyComponent(RendererBatch *batch, ComponentHandle handle);

/// @brief Get component data from handle (for internal use)
/// @param batch The batch containing the component.
/// @param handle Handle to resolve.
/// @return Pointer to component data, or NULL if invalid.
RendererComponent *RendererBatch_GetComponent(const RendererBatch *batch, ComponentHandle handle);

/// @brief Set position reference for a component
/// @param batch The batch containing the component.
/// @param handle Handle to the component.
/// @param positionReference New position reference.
void RendererComponent_SetPositionReference(RendererBatch *batch, ComponentHandle handle, Vector3 *positionReference);

/// @brief Set rotation reference for a component
/// @param batch The batch containing the component.
/// @param handle Handle to the component.
/// @param rotationReference New rotation reference.
void RendererComponent_SetRotationReference(RendererBatch *batch, ComponentHandle handle, Vector3 *rotationReference);

/// @brief Set scale reference for a component
/// @param batch The batch containing the component.
/// @param handle Handle to the component.
/// @param scaleReference New scale reference.
void RendererComponent_SetScaleReference(RendererBatch *batch, ComponentHandle handle, Vector3 *scaleReference);
```

#### Step 1.4: Implement New Functions

**Note:** This Phase 1 implementation solves the component handle invalidation problem but still has the matrix shifting issue. This is intentional - Phase 2 addresses the matrix problem separately. The key insight is that component handles remain valid even when matrices shift.

In `src/systems/Renderer.c`:

```c
ComponentHandle RendererBatch_CreateComponent(RendererBatch *batch,
                                               Vector3 *positionReference,
                                               Vector3 *rotationReference,
                                               Vector3 *scaleReference)
{
    RJGlobal_DebugAssertNullPointerCheck(batch);
    
    // Allocate component from pool
    ComponentHandle handle = ComponentPool_Allocate(&batch->componentPool);
    
    // Get component data
    RendererComponent *component = (RendererComponent *)ComponentPool_Get(&batch->componentPool, handle);
    RJGlobal_DebugAssertNullPointerCheck(component);
    
    // Initialize component
    component->positionReference = positionReference;
    component->rotationReference = rotationReference;
    component->scaleReference = scaleReference;
    component->batch = batch;
    component->componentOffsetInBatch = batch->objectMatrices.count;
    
    // Allocate matrix slot
    Renderer_Matrix4 temp;
    ListArray_Add(&batch->objectMatrices, &temp);
    
    return handle;
}

void RendererBatch_DestroyComponent(RendererBatch *batch, ComponentHandle handle)
{
    RJGlobal_DebugAssertNullPointerCheck(batch);
    
    RendererComponent *component = (RendererComponent *)ComponentPool_Get(&batch->componentPool, handle);
    if (component == NULL)
    {
        RJGlobal_DebugWarning("Attempted to destroy invalid renderer component");
        return;
    }
    
    // Remove from object matrices
    // NOTE: This still has the shifting problem! See "Phase 2: Advanced" section for solutions.
    // For now, this is a simplified example showing the handle system basics.
    ListArray_RemoveAtIndex(&batch->objectMatrices, component->componentOffsetInBatch);
    
    // Update offsets for all components after this one
    ComponentPool_Iterate(&batch->componentPool, RendererBatch_UpdateOffsetsCallback, 
                         (void*)(uintptr_t)component->componentOffsetInBatch);
    
    // Free component from pool
    ComponentPool_Free(&batch->componentPool, handle);
}

// Callback to update offsets after component destruction
void RendererBatch_UpdateOffsetsCallback(ComponentHandle handle, void *componentData, void *userData)
{
    size_t destroyedOffset = (size_t)(uintptr_t)userData;
    RendererComponent *comp = (RendererComponent *)componentData;
    
    if (comp->componentOffsetInBatch > destroyedOffset) {
        comp->componentOffsetInBatch--;
    }
}

RendererComponent *RendererBatch_GetComponent(const RendererBatch *batch, ComponentHandle handle)
{
    RJGlobal_DebugAssertNullPointerCheck(batch);
    return (RendererComponent *)ComponentPool_Get(&batch->componentPool, handle);
}

void RendererComponent_SetPositionReference(RendererBatch *batch, ComponentHandle handle, Vector3 *positionReference)
{
    RendererComponent *component = RendererBatch_GetComponent(batch, handle);
    if (component != NULL)
    {
        component->positionReference = positionReference;
    }
}

// Similar implementations for SetRotationReference and SetScaleReference...
```

### Phase 2: Advanced - Remove Matrix Shifting Problem

The objectMatrices array still shifts on removal. Here are two solutions:

#### Solution A: Use ComponentPool for Matrices Too

```c
typedef struct MatrixSlot
{
    Renderer_Matrix4 matrix;
    ComponentHandle ownerHandle;  // Which component owns this matrix
    bool isAlive;
} MatrixSlot;

typedef struct RendererBatch
{
    RendererModel *model;
    ComponentPool componentPool;
    ListArray matrixSlots;  // MatrixSlot array, indexed by component handle
    
    RendererScene *scene;
    size_t batchOffsetInScene;
} RendererBatch;
```

Then matrices are indexed by component handle, not offset!

#### Solution B: Compact Matrix Array Periodically

Keep tracking alive matrices and compact only when needed:

```c
void RendererBatch_CompactMatrices(RendererBatch *batch)
{
    ListArray newMatrices = ListArray_Create("Renderer_Matrix4", 
                                             sizeof(Renderer_Matrix4), 
                                             batch->objectMatrices.count);
    
    // Iterate through all alive components and rebuild matrix array
    ComponentPool_Iterate(&batch->componentPool, 
                         RendererBatch_RebuildMatrixCallback, 
                         &newMatrices);
    
    ListArray_Destroy(&batch->objectMatrices);
    batch->objectMatrices = newMatrices;
}
```

### Phase 3: Apply to Physics System

Follow the same pattern for PhysicsComponent:

```c
typedef struct PhysicsScene
{
    String name;
    ComponentPool componentPool;  // CHANGED from ListArray components
    
    float drag;
    float gravity;
    float elasticity;
} PhysicsScene;

// Update API:
ComponentHandle PhysicsScene_CreateComponent(PhysicsScene *scene, ...);
void PhysicsScene_DestroyComponent(PhysicsScene *scene, ComponentHandle handle);
PhysicsComponent *PhysicsScene_GetComponent(const PhysicsScene *scene, ComponentHandle handle);
```

### Phase 4: Apply to Audio System

Same pattern for AudioComponent and AudioListenerComponent.

## Code Examples

### Example 1: Creating and Using Components (New API)

```c
// Create scene and batch
RendererScene *scene = RendererScene_CreateEmpty(scv("myScene"), 16);
RendererBatch *batch = RendererScene_CreateBatch(scene, model, 32);

// Create game object with transform
typedef struct GameObject {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    ComponentHandle renderComponent;
    ComponentHandle physicsComponent;
} GameObject;

GameObject player = {0};
player.position = (Vector3){0, 0, 0};
player.rotation = (Vector3){0, 0, 0};
player.scale = (Vector3){1, 1, 1};

// Create components using handles
player.renderComponent = RendererBatch_CreateComponent(batch, 
                                                        &player.position,
                                                        &player.rotation,
                                                        &player.scale);

player.physicsComponent = PhysicsScene_CreateComponent(physicsScene,
                                                        &player.position,
                                                        (Vector3){1, 2, 1},
                                                        70.0f,
                                                        false);

// Later, access component safely
RendererComponent *renderComp = RendererBatch_GetComponent(batch, player.renderComponent);
if (renderComp != NULL)
{
    // Safe to use
}

// Destroy component - other components remain valid!
PhysicsScene_DestroyComponent(physicsScene, player.physicsComponent);

// renderComponent handle is still valid and usable
```

### Example 2: Iterating Over Components

```c
void UpdateAllRenderComponents(RendererBatch *batch)
{
    ComponentPool_Iterate(&batch->componentPool, UpdateRenderComponentCallback, batch);
}

void UpdateRenderComponentCallback(ComponentHandle handle, void *componentData, void *userData)
{
    RendererComponent *component = (RendererComponent *)componentData;
    RendererBatch *batch = (RendererBatch *)userData;
    
    // Update component...
}
```

## Migration Path

### For Users of the Framework

1. **Update component storage:**
   ```c
   // OLD:
   RendererComponent *myComponent;
   
   // NEW:
   ComponentHandle myComponent;
   ```

2. **Update creation:**
   ```c
   // OLD:
   myComponent = RendererBatch_CreateComponent(batch, &pos, &rot, &scale);
   myComponent->positionReference = &newPos;
   
   // NEW:
   myComponent = RendererBatch_CreateComponent(batch, &pos, &rot, &scale);
   RendererComponent_SetPositionReference(batch, myComponent, &newPos);
   ```

3. **Update access:**
   ```c
   // OLD:
   myComponent->positionReference = &pos;
   
   // NEW:
   RendererComponent *comp = RendererBatch_GetComponent(batch, myComponent);
   if (comp != NULL) {
       comp->positionReference = &pos;
   }
   ```

## Testing Strategy

### Unit Tests

```c
void test_ComponentPool_CreateDestroy()
{
    ComponentPool pool = ComponentPool_Create("TestComponent", sizeof(int), 4);
    
    ComponentHandle h1 = ComponentPool_Allocate(&pool);
    assert(ComponentPool_IsValid(&pool, h1));
    
    ComponentPool_Free(&pool, h1);
    assert(!ComponentPool_IsValid(&pool, h1));
    
    ComponentPool_Destroy(&pool);
}

void test_ComponentPool_GenerationCheck()
{
    ComponentPool pool = ComponentPool_Create("TestComponent", sizeof(int), 4);
    
    ComponentHandle h1 = ComponentPool_Allocate(&pool);
    ComponentHandle old_h1 = h1;  // Save old handle
    
    ComponentPool_Free(&pool, h1);
    
    // Allocate again - should reuse slot with new generation
    ComponentHandle h2 = ComponentPool_Allocate(&pool);
    
    // Old handle should be invalid
    assert(!ComponentPool_IsValid(&pool, old_h1));
    
    // New handle should be valid
    assert(ComponentPool_IsValid(&pool, h2));
    
    ComponentPool_Destroy(&pool);
}
```

### Integration Tests

Test full component lifecycle with actual renderer/physics scenes.

### Performance Tests

Benchmark handle resolution overhead:
```c
void benchmark_HandleResolution()
{
    ComponentPool pool = ComponentPool_Create("Benchmark", sizeof(int), 10000);
    
    // Allocate many components
    ComponentHandle handles[10000];
    for (int i = 0; i < 10000; i++) {
        handles[i] = ComponentPool_Allocate(&pool);
    }
    
    // Time resolution
    clock_t start = clock();
    for (int i = 0; i < 1000000; i++) {
        void *comp = ComponentPool_Get(&pool, handles[i % 10000]);
        (void)comp;
    }
    clock_t end = clock();
    
    printf("1M handle resolutions: %f seconds\n", 
           (double)(end - start) / CLOCKS_PER_SEC);
    
    ComponentPool_Destroy(&pool);
}
```

## Conclusion

This guide provides a complete implementation path for migrating to handle-based components. The approach:

- ✅ Solves the dangling pointer problem completely
- ✅ Maintains good performance
- ✅ Allows incremental migration
- ✅ Provides clear API
- ✅ Enables future optimizations

Start with Phase 1 for one system (Renderer), test thoroughly, then apply to other systems.
