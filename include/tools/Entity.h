#pragma once

#include "RJGlobal.h"

#include "utilities/Vector.h"

#pragma region Typedefs

/// @brief Capacity of free indices array of the entity array and other component systems.
#define ENTITY_INITIAL_FREE_INDEX_ARRAY_SIZE 4

/// @brief Entity type used for all of the component systems.
typedef RJ_Size Entity;

#pragma endregion Typedefs

/// @brief Initialize the entity data with the specified capacity.
/// @param initialEntityCapacity The initial capacity for entities.
/// @return RJ_OK on success or RJ_ERROR_ALLOCATION if internal allocation fails.
RJ_ResultWarn Entity_Initialize(RJ_Size initialEntityCapacity);

/// @brief Terminate and free the necessary resources for entity datas.
void Entity_Terminate(void);

/// @brief
/// @param position
/// @param rotation
/// @param scale
/// @return
Entity Entity_Create(Vector3 position, Vector3 rotation, Vector3 scale);

/// @brief
/// @param entity
void Entity_Destroy(Entity entity);

// todo add resize and callbacks

/// @brief
/// @param entity
/// @return
Vector3 Entity_GetPosition(Entity entity);

/// @brief
/// @param entity
/// @return
Vector3 Entity_GetRotation(Entity entity);

/// @brief
/// @param entity
/// @return
Vector3 Entity_GetScale(Entity entity);

/// @brief
/// @param entity
/// @param position
void Entity_SetPosition(Entity entity, Vector3 position);

/// @brief
/// @param entity
/// @param position
void Entity_SetRotation(Entity entity, Vector3 position);

/// @brief
/// @param entity
/// @param position
void Entity_SetScale(Entity entity, Vector3 position);
