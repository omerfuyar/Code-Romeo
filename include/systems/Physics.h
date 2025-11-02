#pragma once

#include "RJGlobal.h"

#include "utilities/Vector.h"
#include "utilities/String.h"
#include "utilities/ListArray.h"

/// @brief Number of iterations to perform when resolving collisions in a physics scene.
#define PHYSICS_COLLISION_RESOLVE_ITERATIONS 2

#pragma region Typedefs

/// @brief Represents a physics scene containing components and global physics properties.
typedef struct PhysicsScene
{
    String name;
    ListArray components; // PhysicsComponent

    float drag;
    float gravity;
    float elasticity;
} PhysicsScene;

/// @brief Represents a component that can interact with the physics scene.
typedef struct PhysicsComponent
{
    Vector3 velocity;
    Vector3 colliderSize;
    Vector3 *positionReference;

    PhysicsScene *scene;
    size_t componentOffsetInScene;

    float mass;
    bool isStatic;
} PhysicsComponent;

#pragma endregion Typedefs

#pragma region Physics

/// @brief Checks for collision between two AABB colliders.
/// @param component1 The first component.
/// @param component2 The second component.
/// @param overlapRet If not NULL, will be filled with the overlap vector on each axis.
/// @return True if the components are colliding, false otherwise.
bool Physics_IsColliding(const PhysicsComponent *component1, const PhysicsComponent *component2, Vector3 *overlapRet);

#pragma endregion Physics

#pragma region Physics Scene

/// @brief Creates a new physics scene.
/// @param name The name of the scene.
/// @param initialColliderCapacity The initial capacity for physics components.
/// @param drag The drag to be applied to components (0-1).
/// @param gravity The gravity to be applied to components.
/// @param elasticity The elasticity to be applied to components (0-1).
/// @return A pointer to the newly created physics scene.
PhysicsScene *PhysicsScene_Create(StringView name, size_t initialColliderCapacity, float drag, float gravity, float elasticity);

/// @brief Destroys a physics scene and all its components.
/// @param scene The scene to destroy.
void PhysicsScene_Destroy(PhysicsScene *scene);

/// @brief Updates the positions of all non-static components in the scene based on their velocity, gravity, and drag.
/// @param scene The scene to update.
/// @param deltaTime The time elapsed since the last frame.
void PhysicsScene_UpdateComponents(const PhysicsScene *scene, float deltaTime);

/// @brief Detects and resolves collisions between components in the scene.
/// @param scene The scene to process.
void PhysicsScene_ResolveCollisions(const PhysicsScene *scene);

/// @brief Creates a new physics component and adds it to the scene.
/// @param scene The scene to add the component to.
/// @param positionReference A pointer to the position vector of the object.
/// @param colliderSize The size of the AABB collider.
/// @param mass The mass of the object.
/// @param isStatic Whether the object is static (unmovable).
/// @return A pointer to the newly created physics component.
PhysicsComponent *PhysicsScene_CreateComponent(PhysicsScene *scene, Vector3 *positionReference, Vector3 colliderSize, float mass, bool isStatic);

/// @brief Destroys a physics component and removes it from its scene.
/// @param component The component to destroy.
void PhysicsScene_DestroyComponent(PhysicsComponent *component);

#pragma endregion Physics Scene

#pragma region Physics Component

void PhysicsComponent_Update(PhysicsComponent *component, float deltaTime);

void PhysicsComponent_Configure(PhysicsComponent *component, Vector3 newColliderSize, float newMass, bool newIsStatic);

#pragma endregion Physics Component
