#pragma once

#include "RJGlobal.h"

#include "utilities/Vector.h"

/// @brief Number of iterations to perform when resolving collisions in a physics scene.
#define PHYSICS_COLLISION_RESOLVE_ITERATIONS 2

/// @brief Capacity of free indices array of the physics system.
#define PHYSICS_INITIAL_FREE_INDEX_ARRAY_SIZE 4

#pragma region Typedefs

/// @brief Represents a component that can interact with the physics system.
typedef RJGlobal_Size PhysicsComponent;

#pragma endregion Typedefs

/// @brief Creates a new physics scene. Try keeping entities that have physics component in sequence for best cpu cache performance.
/// @param componentCapacity The initial capacity for physics components.
/// @param positionReferences A pointer to the start of position vector references for all objects.
/// @param drag The drag to be applied to components (0-1).
/// @param gravity The gravity to be applied to components.
/// @param elasticity The elasticity to be applied to components (0-1).
/// @return A pointer to the newly created physics scene.
void Physics_Initialize(RJGlobal_Size componentCapacity, Vector3 *positionReferences, float drag, float gravity, float elasticity);

/// @brief Destroys a physics scene and all its components.
void Physics_Terminate();

/// @brief Changes the position references for all physics components in the scene.
/// @param positionReferences A pointer to the start of the new position vector references.
/// @param newCapacity The maximum number of position vectors available.
void Physics_ConfigureReferences(Vector3 *positionReferences, RJGlobal_Size newCapacity);

/// @brief Checks for collision between two AABB colliders.
/// @param component1 The first component.
/// @param component2 The second component.
/// @param overlapRet If not NULL, will be filled with the overlap vector on each axis.
/// @return True if the components are colliding, false otherwise.
bool Physics_IsColliding(PhysicsComponent component1, PhysicsComponent component2, Vector3 *overlapRet);

/// @brief Updates the positions of all non-static components in the scene based on their velocity, gravity, and drag.
/// @param deltaTime The time elapsed since the last frame.
void Physics_UpdateComponents(float deltaTime);

/// @brief Detects and resolves collisions between components in the scene.
void Physics_ResolveCollisions();

/// @brief Creates a new physics component and adds it to the scene.
/// @param entity The entity associated with the physics component.
/// @param colliderSize The size of the AABB collider.
/// @param mass The mass of the object.
/// @param isStatic Whether the object is static (unmovable).
/// @return A pointer to the newly created physics component.
PhysicsComponent Physics_ComponentCreate(RJGlobal_Size entity, Vector3 colliderSize, float mass, bool isStatic);

/// @brief Destroys a physics component and removes it from its scene.
/// @param component The component to destroy.
void Physics_ComponentDestroy(PhysicsComponent component);

/// @brief Updates the physics component to current frame.
/// @param component The component to update.
/// @param deltaTime The time elapsed since the last frame.
void Physics_ComponentUpdate(PhysicsComponent component, float deltaTime);

/// @brief Gets the velocity of a physics component.
/// @param component The component to query.
/// @return The velocity of the component.
Vector3 Physics_ComponentGetVelocity(PhysicsComponent component);

/// @brief Sets the velocity of a physics component.
/// @param component The component to update.
/// @param newVelocity The new velocity to set.
void Physics_ComponentSetVelocity(PhysicsComponent component, Vector3 newVelocity);

/// @brief Gets the collider size of a physics component.
/// @param component The component to query.
/// @return The collider size of the component.
Vector3 Physics_ComponentGetColliderSize(PhysicsComponent component);

/// @brief Sets the collider size of a physics component.
/// @param component The component to update.
/// @param newColliderSize The new collider size to set.
void Physics_ComponentSetColliderSize(PhysicsComponent component, Vector3 newColliderSize);

/// @brief Gets the mass of a physics component.
/// @param component The component to query.
/// @return The mass of the component.
float Physics_ComponentGetMass(PhysicsComponent component);

/// @brief Sets the mass of a physics component.
/// @param component The component to update.
/// @param newMass The new mass to set.
void Physics_ComponentSetMass(PhysicsComponent component, float newMass);

/// @brief Checks if a physics component is active.
/// @param component The component to query.
/// @return True if the component is active, false otherwise.
bool Physics_ComponentIsActive(PhysicsComponent component);

/// @brief Sets whether a physics component is active.
/// @param component The component to update.
/// @param newIsActive The new active state to set.
void Physics_ComponentSetActive(PhysicsComponent component, bool newIsActive);

/// @brief Checks if a physics component is static.
/// @param component The component to query.
/// @return True if the component is static, false otherwise.
bool Physics_ComponentIsStatic(PhysicsComponent component);

/// @brief Sets whether a physics component is static.
/// @param component The component to update.
/// @param newIsStatic The new static state to set.
void Physics_ComponentSetStatic(PhysicsComponent component, bool newIsStatic);
