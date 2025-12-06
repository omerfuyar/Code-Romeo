# ECS-Style Internal Architecture Guide

## Overview

This guide addresses implementing an **ECS-style internal architecture** where:
- Systems are updated internally in a known order
- Components are created from fixed-capacity internal scenes
- Transforms (positions/rotations) are stored in centralized arrays
- Maximum cache-friendliness is desired

## Core Architecture

### Central Transform Storage

```c
/// @brief Global transform system storing ALL positions/rotations
typedef struct TransformSystem {
    ListArray positions;   // Vector3 array - NOT pointers
    ListArray rotations;   // Vector3 array
    ListArray scales;      // Vector3 array
    ListArray active;      // Bitfield - which transforms are in use
    
    size_t capacity;       // Maximum transforms (set on init)
    size_t count;          // Current used transforms
    size_t firstFree;      // Free list head
} TransformSystem;

// Global instance (internal)
static TransformSystem g_transforms;

/// @brief Initialize transform system with fixed capacity
void TransformSystem_Initialize(size_t maxTransforms) {
    g_transforms.positions = ListArray_Create("Vector3", sizeof(Vector3), maxTransforms);
    g_transforms.rotations = ListArray_Create("Vector3", sizeof(Vector3), maxTransforms);
    g_transforms.scales = ListArray_Create("Vector3", sizeof(Vector3), maxTransforms);
    g_transforms.active = ListArray_Create("bool", sizeof(bool), maxTransforms);
    g_transforms.capacity = maxTransforms;
    g_transforms.count = 0;
    g_transforms.firstFree = 0;
}

/// @brief Allocate a transform slot, returns entity ID
uint32_t TransformSystem_Allocate() {
    if (g_transforms.count >= g_transforms.capacity) {
        RJGlobal_DebugError("Transform system capacity exceeded!");
        return UINT32_MAX;
    }
    
    // Simple allocation - just append
    uint32_t entityId = g_transforms.count++;
    
    // Initialize to defaults
    Vector3 defaultPos = {0, 0, 0};
    Vector3 defaultRot = {0, 0, 0};
    Vector3 defaultScale = {1, 1, 1};
    bool active = true;
    
    ListArray_Set(&g_transforms.positions, entityId, &defaultPos);
    ListArray_Set(&g_transforms.rotations, entityId, &defaultRot);
    ListArray_Set(&g_transforms.scales, entityId, &defaultScale);
    ListArray_Set(&g_transforms.active, entityId, &active);
    
    return entityId;
}
```

### Internal Renderer Scene (Fixed Capacity)

```c
/// @brief Internal renderer scene with fixed capacity
typedef struct RendererSceneInternal {
    // Component data (SoA layout for cache-friendliness)
    struct {
        ListArray entityIds;        // uint32_t - references to TransformSystem
        ListArray modelIndices;     // uint32_t - which model to use
        ListArray materialIndices;  // uint32_t - which material
        ListArray flags;            // uint32_t - rendering flags (visible, cast shadow, etc.)
    } components;
    
    // Pre-allocated OpenGL resources
    RendererVAOHandle vao;
    RendererUBOHandle uboMatrices;
    
    // Fixed capacity
    size_t capacity;
    size_t count;
    
    // Models and materials pools
    ListArray models;     // RendererModel*
    ListArray materials;  // RendererMaterial*
} RendererSceneInternal;

// Global instance
static RendererSceneInternal g_rendererScene;

/// @brief Initialize internal renderer scene
void RendererScene_InitializeInternal(size_t maxComponents) {
    g_rendererScene.capacity = maxComponents;
    g_rendererScene.count = 0;
    
    // Allocate component arrays (Structure of Arrays)
    g_rendererScene.components.entityIds = 
        ListArray_Create("uint32_t", sizeof(uint32_t), maxComponents);
    g_rendererScene.components.modelIndices = 
        ListArray_Create("uint32_t", sizeof(uint32_t), maxComponents);
    g_rendererScene.components.materialIndices = 
        ListArray_Create("uint32_t", sizeof(uint32_t), maxComponents);
    g_rendererScene.components.flags = 
        ListArray_Create("uint32_t", sizeof(uint32_t), maxComponents);
    
    // Initialize OpenGL resources
    glGenVertexArrays(1, &g_rendererScene.vao);
    glGenBuffers(1, &g_rendererScene.uboMatrices);
    
    // Pre-allocate UBO for all matrices
    glBindBuffer(GL_UNIFORM_BUFFER, g_rendererScene.uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 
                 maxComponents * sizeof(Renderer_Matrix4), 
                 NULL, 
                 GL_DYNAMIC_DRAW);
}
```

## Cache-Friendly System Updates

### Question: Do Position References Hurt Cache Performance?

**Answer: It depends on the access pattern!**

#### Scenario A: Direct Array Access (BEST)

```c
// Components store entity IDs, transforms are in arrays
// This is OPTIMAL for cache

void PhysicsSystem_Update(float dt) {
    // Get direct pointers to arrays
    uint32_t* entityIds = (uint32_t*)g_physicsScene.components.entityIds.data;
    Vector3* velocities = (Vector3*)g_physicsScene.components.velocities.data;
    Vector3* positions = (Vector3*)g_transforms.positions.data;
    Vector3* rotations = (Vector3*)g_transforms.rotations.data;
    uint32_t* flags = (uint32_t*)g_physicsScene.components.flags.data;
    
    float gravity = g_physicsScene.gravity;
    float drag = g_physicsScene.drag;
    
    // PERFECT cache usage - all arrays accessed sequentially
    for (size_t i = 0; i < g_physicsScene.count; i++) {
        if (flags[i] & PHYSICS_FLAG_STATIC) continue;
        
        uint32_t entityId = entityIds[i];
        
        // Update velocity (local array - cache hit)
        velocities[i].y += gravity * dt;
        velocities[i] *= drag;
        
        // Update position (indexed access - ONE cache miss per iteration)
        positions[entityId] += velocities[i] * dt;
        
        // If updates are sparse, this is still good
        // If updates are dense (most components active), this is excellent
    }
}
```

**Cache Analysis:**
- Physics arrays: Sequential access = perfect prefetch
- Transform array: Indexed access = 1 cache miss per component
- **Total: ~1 cache miss per component** (very good!)

#### Scenario B: Pointer References (WORSE)

```c
// Components store pointers to transforms
// This is SUBOPTIMAL

typedef struct PhysicsComponent {
    Vector3 velocity;
    Vector3* positionPtr;  // Pointer to somewhere in g_transforms
    // ...
} PhysicsComponent;

void PhysicsSystem_Update_WithPointers(float dt) {
    PhysicsComponent* comps = (PhysicsComponent*)g_physicsScene.components.data;
    
    for (size_t i = 0; i < g_physicsScene.count; i++) {
        if (comps[i].flags & PHYSICS_FLAG_STATIC) continue;
        
        // Update velocity (good - contiguous)
        comps[i].velocity.y += g_physicsScene.gravity * dt;
        
        // Update position (bad - pointer chase)
        *comps[i].positionPtr += comps[i].velocity * dt;
        
        // Each pointer dereference is unpredictable for prefetcher
    }
}
```

**Cache Analysis:**
- Component array: Sequential access = good
- Position pointers: Random access = 1 cache miss per component
- **Total: ~1 cache miss per component** (same as entity ID!)

**Conclusion:** Entity IDs vs pointers have **similar cache performance** (both ~1 miss), but entity IDs are:
- ✅ Safer (can validate)
- ✅ Smaller (4 bytes vs 8 bytes)
- ✅ Better for serialization

### Optimal Update Order

Since you control update order internally, arrange for maximum cache reuse:

```c
/// @brief Main update loop with optimal cache usage
void Framework_Update(float deltaTime) {
    // 1. Input system (updates input state - small, cache hot)
    InputSystem_Update();
    
    // 2. Physics system (reads/writes transforms)
    PhysicsSystem_Update(deltaTime);
    // After this: transform positions are HOT in cache
    
    // 3. Renderer system (reads transforms - still in cache!)
    RendererSystem_Update(deltaTime);
    // Optimal: reads positions that physics just wrote
    
    // 4. Audio system (reads transforms - may need reload)
    AudioSystem_Update(deltaTime);
    
    // Order matters for cache reuse!
}
```

### Multi-Level Cache Optimization

```c
/// @brief Physics update with cache-aware batching
void PhysicsSystem_Update(float dt) {
    // Get array pointers once
    uint32_t* entityIds = g_physicsScene.components.entityIds.data;
    Vector3* velocities = g_physicsScene.components.velocities.data;
    Vector3* positions = g_transforms.positions.data;
    uint32_t* flags = g_physicsScene.components.flags.data;
    
    // Pre-cache global values
    float gravity = g_physicsScene.gravity;
    float drag = g_physicsScene.drag;
    
    // Process in batches for L1 cache (typically 32KB)
    // Assume: 1 component = 32 bytes of hot data
    // L1 can hold ~1000 components
    const size_t BATCH_SIZE = 256; // Conservative for L1d
    
    for (size_t batch = 0; batch < g_physicsScene.count; batch += BATCH_SIZE) {
        size_t end = (batch + BATCH_SIZE < g_physicsScene.count) 
                     ? batch + BATCH_SIZE 
                     : g_physicsScene.count;
        
        // All data for this batch should fit in L1
        for (size_t i = batch; i < end; i++) {
            if (flags[i] & PHYSICS_FLAG_STATIC) continue;
            
            uint32_t entityId = entityIds[i];
            
            velocities[i].y += gravity * dt;
            velocities[i] *= drag;
            positions[entityId] += velocities[i] * dt;
        }
        
        // Batch boundary - natural place for prefetch
    }
}
```

## Complete ECS-Style Architecture

### Full Example

```c
//============================================================================
// Transform System (Central)
//============================================================================

typedef struct TransformSystem {
    ListArray positions;   // Vector3
    ListArray rotations;   // Vector3  
    ListArray scales;      // Vector3
    ListArray dirty;       // bool - needs matrix recalc
    
    size_t capacity;
    size_t count;
} TransformSystem;

static TransformSystem g_transforms;

void TransformSystem_Initialize(size_t capacity) {
    g_transforms.positions = ListArray_Create("Vector3", sizeof(Vector3), capacity);
    g_transforms.rotations = ListArray_Create("Vector3", sizeof(Vector3), capacity);
    g_transforms.scales = ListArray_Create("Vector3", sizeof(Vector3), capacity);
    g_transforms.dirty = ListArray_Create("bool", sizeof(bool), capacity);
    g_transforms.capacity = capacity;
    g_transforms.count = 0;
}

uint32_t TransformSystem_Allocate() {
    uint32_t id = g_transforms.count++;
    Vector3 zero = {0, 0, 0};
    Vector3 one = {1, 1, 1};
    bool dirty = true;
    
    ListArray_Set(&g_transforms.positions, id, &zero);
    ListArray_Set(&g_transforms.rotations, id, &zero);
    ListArray_Set(&g_transforms.scales, id, &one);
    ListArray_Set(&g_transforms.dirty, id, &dirty);
    
    return id;
}

void TransformSystem_SetPosition(uint32_t entityId, Vector3 position) {
    ListArray_Set(&g_transforms.positions, entityId, &position);
    bool dirty = true;
    ListArray_Set(&g_transforms.dirty, entityId, &dirty);
}

Vector3 TransformSystem_GetPosition(uint32_t entityId) {
    return *(Vector3*)ListArray_Get(&g_transforms.positions, entityId);
}

//============================================================================
// Physics System (Internal)
//============================================================================

#define PHYSICS_FLAG_STATIC    (1 << 0)
#define PHYSICS_FLAG_KINEMATIC (1 << 1)

typedef struct PhysicsSystemInternal {
    struct {
        ListArray entityIds;     // uint32_t
        ListArray velocities;    // Vector3
        ListArray colliderSizes; // Vector3
        ListArray masses;        // float
        ListArray flags;         // uint32_t
    } components;
    
    size_t capacity;
    size_t count;
    
    float gravity;
    float drag;
} PhysicsSystemInternal;

static PhysicsSystemInternal g_physics;

void PhysicsSystem_Initialize(size_t capacity, float gravity, float drag) {
    g_physics.capacity = capacity;
    g_physics.count = 0;
    g_physics.gravity = gravity;
    g_physics.drag = drag;
    
    g_physics.components.entityIds = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
    g_physics.components.velocities = 
        ListArray_Create("Vector3", sizeof(Vector3), capacity);
    g_physics.components.colliderSizes = 
        ListArray_Create("Vector3", sizeof(Vector3), capacity);
    g_physics.components.masses = 
        ListArray_Create("float", sizeof(float), capacity);
    g_physics.components.flags = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
}

uint32_t PhysicsSystem_CreateComponent(uint32_t entityId, Vector3 colliderSize, 
                                       float mass, bool isStatic) {
    if (g_physics.count >= g_physics.capacity) {
        return UINT32_MAX;
    }
    
    uint32_t componentId = g_physics.count++;
    
    Vector3 zeroVel = {0, 0, 0};
    uint32_t flags = isStatic ? PHYSICS_FLAG_STATIC : 0;
    
    ListArray_Set(&g_physics.components.entityIds, componentId, &entityId);
    ListArray_Set(&g_physics.components.velocities, componentId, &zeroVel);
    ListArray_Set(&g_physics.components.colliderSizes, componentId, &colliderSize);
    ListArray_Set(&g_physics.components.masses, componentId, &mass);
    ListArray_Set(&g_physics.components.flags, componentId, &flags);
    
    return componentId;
}

void PhysicsSystem_Update(float dt) {
    // Cache-friendly update with entity ID references
    uint32_t* entityIds = g_physics.components.entityIds.data;
    Vector3* velocities = g_physics.components.velocities.data;
    uint32_t* flags = g_physics.components.flags.data;
    Vector3* positions = g_transforms.positions.data;
    
    float gravity = g_physics.gravity;
    float drag = g_physics.drag;
    
    for (size_t i = 0; i < g_physics.count; i++) {
        if (flags[i] & PHYSICS_FLAG_STATIC) continue;
        
        uint32_t entityId = entityIds[i];
        
        // Update velocity
        velocities[i].y += gravity * dt;
        velocities[i] *= drag;
        
        // Update position in transform system
        positions[entityId] += velocities[i] * dt;
    }
}

//============================================================================
// Renderer System (Internal)
//============================================================================

typedef struct RendererSystemInternal {
    struct {
        ListArray entityIds;        // uint32_t
        ListArray modelIndices;     // uint32_t
        ListArray materialIndices;  // uint32_t
        ListArray visibilityFlags;  // uint32_t
    } components;
    
    ListArray models;     // RendererModel*
    ListArray materials;  // RendererMaterial*
    ListArray matrices;   // Renderer_Matrix4 - computed from transforms
    
    size_t capacity;
    size_t count;
    
    RendererVAOHandle vao;
    RendererUBOHandle ubo;
} RendererSystemInternal;

static RendererSystemInternal g_renderer;

void RendererSystem_Initialize(size_t capacity) {
    g_renderer.capacity = capacity;
    g_renderer.count = 0;
    
    g_renderer.components.entityIds = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
    g_renderer.components.modelIndices = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
    g_renderer.components.materialIndices = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
    g_renderer.components.visibilityFlags = 
        ListArray_Create("uint32_t", sizeof(uint32_t), capacity);
    
    g_renderer.matrices = 
        ListArray_Create("Renderer_Matrix4", sizeof(Renderer_Matrix4), capacity);
    
    g_renderer.models = ListArray_Create("RendererModel*", sizeof(RendererModel*), 16);
    g_renderer.materials = ListArray_Create("RendererMaterial*", sizeof(RendererMaterial*), 16);
    
    // OpenGL setup
    glGenVertexArrays(1, &g_renderer.vao);
    glGenBuffers(1, &g_renderer.ubo);
}

uint32_t RendererSystem_CreateComponent(uint32_t entityId, uint32_t modelIndex, 
                                        uint32_t materialIndex) {
    if (g_renderer.count >= g_renderer.capacity) {
        return UINT32_MAX;
    }
    
    uint32_t componentId = g_renderer.count++;
    uint32_t visible = 1;
    
    ListArray_Set(&g_renderer.components.entityIds, componentId, &entityId);
    ListArray_Set(&g_renderer.components.modelIndices, componentId, &modelIndex);
    ListArray_Set(&g_renderer.components.materialIndices, componentId, &materialIndex);
    ListArray_Set(&g_renderer.components.visibilityFlags, componentId, &visible);
    
    return componentId;
}

void RendererSystem_Update(float dt) {
    // Update matrices from transforms (cache-friendly)
    uint32_t* entityIds = g_renderer.components.entityIds.data;
    Renderer_Matrix4* matrices = g_renderer.matrices.data;
    
    Vector3* positions = g_transforms.positions.data;
    Vector3* rotations = g_transforms.rotations.data;
    Vector3* scales = g_transforms.scales.data;
    bool* dirty = g_transforms.dirty.data;
    
    // Only recalculate dirty matrices
    for (size_t i = 0; i < g_renderer.count; i++) {
        uint32_t entityId = entityIds[i];
        
        if (!dirty[entityId]) continue;
        
        // Transform is still hot in cache from PhysicsSystem_Update!
        TRANSFORM_TO_MODEL_MATRIX(&matrices[i], 
                                  &positions[entityId],
                                  &rotations[entityId],
                                  &scales[entityId]);
        
        dirty[entityId] = false;
    }
    
    // Upload to GPU
    glBindBuffer(GL_UNIFORM_BUFFER, g_renderer.ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 
                    g_renderer.count * sizeof(Renderer_Matrix4),
                    matrices);
}

//============================================================================
// Public API (Simple and Clean)
//============================================================================

/// @brief Initialize framework with fixed capacities
void Framework_Initialize(size_t maxEntities) {
    TransformSystem_Initialize(maxEntities);
    PhysicsSystem_Initialize(maxEntities, -9.8f, 0.98f);
    RendererSystem_Initialize(maxEntities);
    AudioSystem_Initialize(maxEntities);
}

/// @brief Create a game object (returns entity ID)
uint32_t GameObject_Create(Vector3 position) {
    uint32_t entityId = TransformSystem_Allocate();
    TransformSystem_SetPosition(entityId, position);
    return entityId;
}

/// @brief Add renderer component to entity
void GameObject_AddRenderer(uint32_t entityId, uint32_t modelIndex, 
                           uint32_t materialIndex) {
    RendererSystem_CreateComponent(entityId, modelIndex, materialIndex);
}

/// @brief Add physics component to entity
void GameObject_AddPhysics(uint32_t entityId, Vector3 colliderSize, 
                          float mass, bool isStatic) {
    PhysicsSystem_CreateComponent(entityId, colliderSize, mass, isStatic);
}

/// @brief Main update loop (internally optimized)
void Framework_Update(float deltaTime) {
    InputSystem_Update();
    PhysicsSystem_Update(deltaTime);   // Writes to transforms
    RendererSystem_Update(deltaTime);  // Reads from transforms (cache hot!)
    AudioSystem_Update(deltaTime);
}
```

## Cache Performance Analysis

### Memory Layout

```
Transform System (Central):
┌─────────────────────────────────────────┐
│ positions[0...N] | rotations[0...N]    │
│ scales[0...N]    | dirty[0...N]        │
└─────────────────────────────────────────┘
All contiguous, perfectly cacheable

Physics System:
┌─────────────────────────────────────────┐
│ entityIds[0...M] | velocities[0...M]   │
│ masses[0...M]    | flags[0...M]        │
└─────────────────────────────────────────┘
All contiguous, perfectly cacheable

Renderer System:
┌─────────────────────────────────────────┐
│ entityIds[0...K] | modelIndices[0...K] │
│ matrices[0...K]  | flags[0...K]        │
└─────────────────────────────────────────┘
All contiguous, perfectly cacheable
```

### Cache Miss Analysis

**Per-component cache misses during update:**

1. **Physics Update:**
   - entityIds array: 0 misses (sequential)
   - velocities array: 0 misses (sequential)
   - flags array: 0 misses (sequential)
   - positions array: ~1 miss per component (indexed)
   - **Total: ~1 cache miss per component**

2. **Renderer Update:**
   - entityIds array: 0 misses (sequential)
   - positions/rotations/scales: ~0 misses (just written by physics, still in cache!)
   - matrices array: 0 misses (sequential write)
   - **Total: ~0 cache misses per component!**

**Conclusion: Using entity IDs with centralized transforms is EXCELLENT for cache performance!**

## Comparison: Entity IDs vs Pointers

| Aspect | Entity IDs | Pointers |
|--------|-----------|----------|
| **Size** | 4 bytes | 8 bytes |
| **Cache misses** | ~1 per update | ~1 per update |
| **Validation** | Can check bounds | Cannot validate |
| **Serialization** | Easy (just ID) | Hard (relocate) |
| **Indirection** | Array index | Pointer dereference |
| **Safety** | Bounds-checkable | Can dangle |
| **Verdict** | ✅ **Better** | ⚠️ Acceptable |

## Recommendation

**For your ECS-style internal architecture:**

✅ **Use entity IDs, NOT pointers**

```c
// Components reference transforms by entity ID
typedef struct PhysicsComponent {
    uint32_t entityId;  // 4 bytes - index into TransformSystem
    Vector3 velocity;
    Vector3 colliderSize;
    uint32_t flags;
} PhysicsComponent;

// Transform system stores actual data
static struct {
    Vector3* positions;  // Array indexed by entity ID
    Vector3* rotations;
    Vector3* scales;
} g_transforms;
```

**Benefits:**
1. ✅ Same cache performance as pointers (~1 miss per component)
2. ✅ Smaller (4 bytes vs 8 bytes) = more components per cache line
3. ✅ Safer (can validate IDs)
4. ✅ Easier serialization
5. ✅ Better for multi-threading (no pointer sharing issues)
6. ✅ Update order optimization gives near-zero cache misses

**Position references DO NOT significantly hurt performance when:**
- Arrays are accessed sequentially in systems
- Update order is optimized (physics writes, renderer reads immediately)
- Entity IDs are used instead of pointers

The key is **Structure of Arrays (SoA) everywhere** and **known update order**!
