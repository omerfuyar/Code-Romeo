# Migration Guide: From Pointer-Based to Handle-Based Components

This guide helps you migrate existing code that uses the old pointer-based component API to the new handle-based API.

## Overview of Changes

### Core Concept Change

**Before (Pointers):**
- Components returned as `Component*` pointers
- Direct access to component fields
- Pointers become invalid when other components destroyed

**After (Handles):**
- Components identified by `ComponentHandle` 
- Access through getter functions
- Handles remain valid regardless of other operations

## Step-by-Step Migration

### Step 1: Update Data Structures

#### Old Code
```c
typedef struct Player {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    RendererComponent *renderComponent;
    PhysicsComponent *physicsComponent;
} Player;
```

#### New Code
```c
typedef struct Player {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    ComponentHandle renderComponent;  // Changed from pointer to handle
    ComponentHandle physicsComponent; // Changed from pointer to handle
} Player;
```

### Step 2: Update Component Creation

#### Old Code
```c
Player *CreatePlayer(RendererBatch *batch, PhysicsScene *physics) {
    Player *player = malloc(sizeof(Player));
    
    player->position = (Vector3){0, 0, 0};
    player->rotation = (Vector3){0, 0, 0};
    player->scale = (Vector3){1, 1, 1};
    
    // Old API: Returns pointer
    player->renderComponent = RendererBatch_CreateComponent(
        batch,
        &player->position,
        &player->rotation,
        &player->scale
    );
    
    player->physicsComponent = PhysicsScene_CreateComponent(
        physics,
        &player->position,
        (Vector3){1, 2, 1},
        70.0f,
        false
    );
    
    return player;
}
```

#### New Code
```c
Player *CreatePlayer(RendererBatch *batch, PhysicsScene *physics) {
    Player *player = malloc(sizeof(Player));
    
    player->position = (Vector3){0, 0, 0};
    player->rotation = (Vector3){0, 0, 0};
    player->scale = (Vector3){1, 1, 1};
    
    // New API: Returns handle
    player->renderComponent = RendererBatch_CreateComponent(
        batch,
        &player->position,
        &player->rotation,
        &player->scale
    );
    
    player->physicsComponent = PhysicsScene_CreateComponent(
        physics,
        &player->position,
        (Vector3){1, 2, 1},
        70.0f,
        false
    );
    
    return player;
}
```

**Note:** Component creation code looks nearly identical! Just the return type changed.

### Step 3: Update Component Access

#### Old Code - Direct Field Access
```c
void UpdatePlayer(Player *player, Vector3 newPosition) {
    // Direct access to component fields
    player->renderComponent->positionReference = &newPosition;
    player->physicsComponent->velocity = (Vector3){1, 0, 0};
}
```

#### New Code - Via Getter and Setter Functions

**Option A: Use setter functions (Recommended for simple updates)**
```c
void UpdatePlayer(Player *player, RendererBatch *batch, PhysicsScene *physics, Vector3 newPosition) {
    // Use setter functions
    RendererComponent_SetPositionReference(batch, player->renderComponent, &newPosition);
    
    PhysicsComponent *physicsComp = PhysicsScene_GetComponent(physics, player->physicsComponent);
    if (physicsComp != NULL) {
        physicsComp->velocity = (Vector3){1, 0, 0};
    }
}
```

**Option B: Get pointer for batch operations**
```c
void UpdatePlayer(Player *player, RendererBatch *batch, PhysicsScene *physics, Vector3 newPosition) {
    // Get component pointers for batch operations
    RendererComponent *renderComp = RendererBatch_GetComponent(batch, player->renderComponent);
    PhysicsComponent *physicsComp = PhysicsScene_GetComponent(physics, player->physicsComponent);
    
    // Check validity and use
    if (renderComp != NULL) {
        renderComp->positionReference = &newPosition;
    }
    
    if (physicsComp != NULL) {
        physicsComp->velocity = (Vector3){1, 0, 0};
    }
}
```

### Step 4: Update Component Destruction

#### Old Code
```c
void DestroyPlayer(Player *player) {
    // Old API: Pass component pointer
    RendererBatch_DestroyComponent(player->renderComponent);
    PhysicsScene_DestroyComponent(player->physicsComponent);
    
    free(player);
}
```

#### New Code
```c
void DestroyPlayer(Player *player, RendererBatch *batch, PhysicsScene *physics) {
    // New API: Pass batch/scene and handle
    RendererBatch_DestroyComponent(batch, player->renderComponent);
    PhysicsScene_DestroyComponent(physics, player->physicsComponent);
    
    free(player);
}
```

**Note:** You now need to pass the batch/scene to the destroy function.

### Step 5: Update Component Queries

#### Old Code
```c
bool IsPlayerGrounded(Player *player) {
    // Direct access to physics component
    return player->physicsComponent->velocity.y == 0.0f;
}
```

#### New Code
```c
bool IsPlayerGrounded(Player *player, PhysicsScene *physics) {
    // Get component first
    PhysicsComponent *comp = PhysicsScene_GetComponent(physics, player->physicsComponent);
    
    if (comp == NULL) {
        return false; // Component was destroyed or invalid
    }
    
    return comp->velocity.y == 0.0f;
}
```

## Common Patterns

### Pattern 1: Keeping Context

Instead of passing batch/scene to every function, keep them in a context:

```c
typedef struct GameContext {
    RendererBatch *mainBatch;
    PhysicsScene *mainPhysics;
    AudioScene *mainAudio;
} GameContext;

typedef struct Player {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    ComponentHandle renderComponent;
    ComponentHandle physicsComponent;
    ComponentHandle audioComponent;
    
    GameContext *context;  // Keep reference to context
} Player;

// Now functions only need player
void UpdatePlayer(Player *player, Vector3 newPosition) {
    PhysicsComponent *comp = PhysicsScene_GetComponent(
        player->context->mainPhysics, 
        player->physicsComponent
    );
    
    if (comp != NULL) {
        comp->velocity = (Vector3){1, 0, 0};
    }
}
```

### Pattern 2: Caching Component Pointers

For performance-critical code, you can cache pointers within a single frame:

```c
void UpdatePlayerFrame(Player *player, GameContext *ctx) {
    // Get pointers once at start of frame
    RendererComponent *render = RendererBatch_GetComponent(
        ctx->mainBatch, player->renderComponent);
    PhysicsComponent *physics = PhysicsScene_GetComponent(
        ctx->mainPhysics, player->physicsComponent);
    
    // Check validity
    if (render == NULL || physics == NULL) {
        return; // Component was destroyed
    }
    
    // Now use pointers for multiple operations
    for (int i = 0; i < 100; i++) {
        // No need to resolve handle each time
        physics->velocity.x += 0.1f;
        render->positionReference = &player->position;
    }
    
    // Don't store these pointers beyond this function!
}
```

**Warning:** Only cache pointers within a single function/frame. Never store them in structs.

### Pattern 3: Checking Component Validity

```c
bool HasValidComponents(Player *player, GameContext *ctx) {
    if (ComponentHandle_IsNull(player->renderComponent)) {
        return false;
    }
    
    if (ComponentHandle_IsNull(player->physicsComponent)) {
        return false;
    }
    
    // Check if handles still resolve to valid components
    if (RendererBatch_GetComponent(ctx->mainBatch, player->renderComponent) == NULL) {
        return false;
    }
    
    if (PhysicsScene_GetComponent(ctx->mainPhysics, player->physicsComponent) == NULL) {
        return false;
    }
    
    return true;
}
```

### Pattern 4: Optional Components

```c
typedef struct GameObject {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    ComponentHandle renderComponent;  
    ComponentHandle physicsComponent; // Optional
    ComponentHandle audioComponent;   // Optional
} GameObject;

GameObject *CreateStaticObject(RendererBatch *batch) {
    GameObject *obj = malloc(sizeof(GameObject));
    
    obj->position = (Vector3){0, 0, 0};
    obj->rotation = (Vector3){0, 0, 0};
    obj->scale = (Vector3){1, 1, 1};
    
    obj->renderComponent = RendererBatch_CreateComponent(
        batch, &obj->position, &obj->rotation, &obj->scale);
    
    // No physics for static object
    obj->physicsComponent = COMPONENT_HANDLE_INVALID;
    obj->audioComponent = COMPONENT_HANDLE_INVALID;
    
    return obj;
}

void UpdateGameObject(GameObject *obj, GameContext *ctx) {
    // Always has render
    RendererComponent *render = RendererBatch_GetComponent(
        ctx->mainBatch, obj->renderComponent);
    
    // Physics is optional
    if (!ComponentHandle_IsNull(obj->physicsComponent)) {
        PhysicsComponent *physics = PhysicsScene_GetComponent(
            ctx->mainPhysics, obj->physicsComponent);
        if (physics != NULL) {
            // Update physics
        }
    }
}
```

## Complete Example: Before and After

### Before (Old Pointer-Based API)

```c
#include "systems/Renderer.h"
#include "systems/Physics.h"

typedef struct Enemy {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    RendererComponent *renderComponent;
    PhysicsComponent *physicsComponent;
    
    float health;
    bool isAlive;
} Enemy;

Enemy *CreateEnemy(RendererBatch *batch, PhysicsScene *physics, Vector3 spawnPos) {
    Enemy *enemy = malloc(sizeof(Enemy));
    
    enemy->position = spawnPos;
    enemy->rotation = (Vector3){0, 0, 0};
    enemy->scale = (Vector3){1, 1, 1};
    enemy->health = 100.0f;
    enemy->isAlive = true;
    
    enemy->renderComponent = RendererBatch_CreateComponent(
        batch, &enemy->position, &enemy->rotation, &enemy->scale);
    
    enemy->physicsComponent = PhysicsScene_CreateComponent(
        physics, &enemy->position, (Vector3){1, 2, 1}, 50.0f, false);
    
    return enemy;
}

void UpdateEnemy(Enemy *enemy, float deltaTime) {
    if (!enemy->isAlive) return;
    
    // Direct access - ERROR PRONE!
    enemy->physicsComponent->velocity.x = 2.0f;
    
    // If another enemy was destroyed before this one,
    // these pointers might be INVALID!
}

void DamageEnemy(Enemy *enemy, float damage) {
    enemy->health -= damage;
    if (enemy->health <= 0) {
        enemy->isAlive = false;
    }
}

void DestroyEnemy(Enemy *enemy) {
    if (enemy->renderComponent != NULL) {
        RendererBatch_DestroyComponent(enemy->renderComponent);
    }
    if (enemy->physicsComponent != NULL) {
        PhysicsScene_DestroyComponent(enemy->physicsComponent);
    }
    free(enemy);
}

// Main game loop
void GameLoop(RendererBatch *batch, PhysicsScene *physics) {
    Enemy *enemies[10];
    int enemyCount = 0;
    
    // Spawn enemies
    for (int i = 0; i < 10; i++) {
        enemies[i] = CreateEnemy(batch, physics, (Vector3){i * 2.0f, 0, 0});
        enemyCount++;
    }
    
    // Update
    for (int i = 0; i < enemyCount; i++) {
        UpdateEnemy(enemies[i], 0.016f);
    }
    
    // Destroy first enemy
    DestroyEnemy(enemies[0]);
    
    // BUG: Now enemies[1-9] have INVALID component pointers!
    for (int i = 1; i < enemyCount; i++) {
        UpdateEnemy(enemies[i], 0.016f); // CRASH!
    }
}
```

### After (New Handle-Based API)

```c
#include "systems/Renderer.h"
#include "systems/Physics.h"
#include "utilities/ComponentHandle.h"

typedef struct GameContext {
    RendererBatch *mainBatch;
    PhysicsScene *mainPhysics;
} GameContext;

typedef struct Enemy {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    ComponentHandle renderComponent;  // Changed to handle
    ComponentHandle physicsComponent; // Changed to handle
    
    float health;
    bool isAlive;
    
    GameContext *context;  // Reference to context
} Enemy;

Enemy *CreateEnemy(GameContext *ctx, Vector3 spawnPos) {
    Enemy *enemy = malloc(sizeof(Enemy));
    
    enemy->position = spawnPos;
    enemy->rotation = (Vector3){0, 0, 0};
    enemy->scale = (Vector3){1, 1, 1};
    enemy->health = 100.0f;
    enemy->isAlive = true;
    enemy->context = ctx;
    
    enemy->renderComponent = RendererBatch_CreateComponent(
        ctx->mainBatch, &enemy->position, &enemy->rotation, &enemy->scale);
    
    enemy->physicsComponent = PhysicsScene_CreateComponent(
        ctx->mainPhysics, &enemy->position, (Vector3){1, 2, 1}, 50.0f, false);
    
    return enemy;
}

void UpdateEnemy(Enemy *enemy, float deltaTime) {
    if (!enemy->isAlive) return;
    
    // Resolve handle to component
    PhysicsComponent *physics = PhysicsScene_GetComponent(
        enemy->context->mainPhysics, enemy->physicsComponent);
    
    // Check validity - handle might be invalid if component was destroyed
    if (physics == NULL) {
        enemy->isAlive = false;
        return;
    }
    
    // Safe to use!
    physics->velocity.x = 2.0f;
}

void DamageEnemy(Enemy *enemy, float damage) {
    enemy->health -= damage;
    if (enemy->health <= 0) {
        enemy->isAlive = false;
    }
}

void DestroyEnemy(Enemy *enemy) {
    if (!ComponentHandle_IsNull(enemy->renderComponent)) {
        RendererBatch_DestroyComponent(
            enemy->context->mainBatch, enemy->renderComponent);
    }
    if (!ComponentHandle_IsNull(enemy->physicsComponent)) {
        PhysicsScene_DestroyComponent(
            enemy->context->mainPhysics, enemy->physicsComponent);
    }
    free(enemy);
}

// Main game loop
void GameLoop(GameContext *ctx) {
    Enemy *enemies[10];
    int enemyCount = 0;
    
    // Spawn enemies
    for (int i = 0; i < 10; i++) {
        enemies[i] = CreateEnemy(ctx, (Vector3){i * 2.0f, 0, 0});
        enemyCount++;
    }
    
    // Update
    for (int i = 0; i < enemyCount; i++) {
        UpdateEnemy(enemies[i], 0.016f);
    }
    
    // Destroy first enemy
    DestroyEnemy(enemies[0]);
    
    // SAFE: enemies[1-9] still have VALID handles!
    for (int i = 1; i < enemyCount; i++) {
        UpdateEnemy(enemies[i], 0.016f); // Works perfectly!
    }
}
```

## Performance Tips

1. **Batch Operations:** If updating many components, get all pointers at once
2. **Cache Context:** Store batch/scene references in your game objects
3. **Validate Once:** Check handle validity at start of function, not repeatedly
4. **Short-lived Pointers:** It's fine to get component pointers for local use

## Common Mistakes to Avoid

❌ **Don't store component pointers long-term**
```c
// BAD
typedef struct Player {
    RendererComponent *render; // Don't do this!
} Player;
```

✅ **Do store component handles**
```c
// GOOD
typedef struct Player {
    ComponentHandle renderComponent; // Do this!
} Player;
```

❌ **Don't assume handles are always valid**
```c
// BAD
PhysicsComponent *comp = PhysicsScene_GetComponent(physics, handle);
comp->velocity = ...; // Might crash if comp is NULL!
```

✅ **Do check for NULL**
```c
// GOOD
PhysicsComponent *comp = PhysicsScene_GetComponent(physics, handle);
if (comp != NULL) {
    comp->velocity = ...;
}
```

❌ **Don't pass batch/scene everywhere**
```c
// BAD - too many parameters
void UpdatePlayer(Player *p, RendererBatch *rb, PhysicsScene *ps, AudioScene *as);
```

✅ **Do use a context structure**
```c
// GOOD - single context parameter
typedef struct GameContext {
    RendererBatch *batch;
    PhysicsScene *physics;
    AudioScene *audio;
} GameContext;

void UpdatePlayer(Player *p, GameContext *ctx);
```

## Summary Checklist

- [ ] Replace all `Component*` with `ComponentHandle` in structs
- [ ] Update component creation (handle return type)
- [ ] Update component access (use getters)
- [ ] Update component destruction (pass batch/scene)
- [ ] Add NULL checks after getting component pointers
- [ ] Consider adding a context structure
- [ ] Never store component pointers in structs
- [ ] Test thoroughly, especially after destroying components

## Questions?

If you encounter issues during migration:
1. Check that you're using `GetComponent` instead of direct access
2. Verify you're checking for NULL after `GetComponent`
3. Make sure you're not storing component pointers
4. Ensure you're passing the correct batch/scene to functions

The new API is safer but requires slightly more code. The trade-off is worth it for the elimination of dangling pointer bugs!
