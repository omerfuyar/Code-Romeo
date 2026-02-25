#pragma once

#include "RJGlobal.h"

#include "tools/Entity.h"

#include "utilities/Vector.h"

#pragma region Typedefs

/// @brief Number of iterations to perform when resolving collisions in a physics scene.
#define PHYSICS_COLLISION_RESOLVE_ITERATIONS 2

#pragma endregion Typedefs

/// @brief Creates a new physics scene. Try keeping entities that have physics component in sequence for best cpu cache performance.
/// @param initialComponentCapacity The initial capacity for physics components.
/// @param drag The drag to be applied to components (0-1).
/// @param gravity The gravity to be applied to components.
/// @param elasticity The elasticity to be applied to components (0-1).
/// @return RJ_OK on success, or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Physics_Initialize(RJ_Size initialComponentCapacity, float drag, float gravity, float elasticity);

/// @brief Destroys a physics scene and all its components.
void Physics_Terminate(void);

/// @brief Changes the position references for all physics components in the scene.
/// @param newCapacity The maximum number of position vectors available.
/// @return RJ_OK on success, or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Physics_Configure(RJ_Size newCapacity);

/// @brief Checks for collision between two AABB colliders.
/// @param component1 The first component.
/// @param component2 The second component.
/// @param overlapRet If not NULL, will be filled with the overlap vector on each axis.
/// @return True if the components are colliding, false otherwise.
bool Physics_IsColliding(Entity entity1, Entity entity2, Vector3 *overlapRet);

/// @brief Updates the positions of all non-static components in the scene based on their velocity, gravity, and drag.
/// @param deltaTime The time elapsed since the last frame.
void Physics_UpdateComponents(float deltaTime);

/// @brief Detects and resolves collisions between components in the scene.
void Physics_ResolveCollisions(void);

/// @brief Creates a new physics component and adds it to the scene.
/// @param entity The entity associated with the physics component.
/// @param colliderSize The size of the AABB collider.
/// @param mass The mass of the object.
/// @param isStatic Whether the object is static (unmovable).
/// @return A pointer to the newly created physics component.
void Physics_ComponentCreate(RJ_Size entity, Vector3 colliderSize, float mass, bool isStatic);

/// @brief Destroys a physics component and removes it from its scene.
/// @param component The component to destroy.
void Physics_ComponentDestroy(Entity entity);

/// @brief Gets the velocity of a physics component.
/// @param component The component to query.
/// @return The velocity of the component.
Vector3 Physics_ComponentGetVelocity(Entity entity);

/// @brief Sets the velocity of a physics component.
/// @param component The component to update.
/// @param newVelocity The new velocity to set.
void Physics_ComponentSetVelocity(Entity entity, Vector3 newVelocity);

/// @brief Gets the collider size of a physics component.
/// @param component The component to query.
/// @return The collider size of the component.
Vector3 Physics_ComponentGetColliderSize(Entity entity);

/// @brief Sets the collider size of a physics component.
/// @param component The component to update.
/// @param newColliderSize The new collider size to set.
void Physics_ComponentSetColliderSize(Entity entity, Vector3 newColliderSize);

/// @brief Gets the mass of a physics component.
/// @param component The component to query.
/// @return The mass of the component.
float Physics_ComponentGetMass(Entity entity);

/// @brief Sets the mass of a physics component.
/// @param component The component to update.
/// @param newMass The new mass to set.
void Physics_ComponentSetMass(Entity entity, float newMass);

/// @brief Checks if a physics component is static.
/// @param component The component to query.
/// @return True if the component is static, false otherwise.
bool Physics_ComponentIsStatic(Entity entity);

/// @brief Sets whether a physics component is static.
/// @param component The component to update.
/// @param newIsStatic The new static state to set.
void Physics_ComponentSetStatic(Entity component, bool newIsStatic);
