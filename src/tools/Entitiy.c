#include "tools/Entity.h"

#include "utilities/ListArray.h"

#pragma region SourceOnly

#define ENTITY_FLAG_ACTIVE (1 << 0)

struct ENTITY
{
    struct ENTITY_DATA
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size
        // todo use another data structure like stack or queue

        Vector3 *positions;
        Vector3 *rotations;
        Vector3 *scales;
        uint8_t *flags;
    } data;
} ENTITY = {0};

#define ePosition(entity) (ENTITY.data.positions[entity])
#define eRotation(entity) (ENTITY.data.rotations[entity])
#define eScale(entity) (ENTITY.data.scales[entity])
#define eFlag(entity) (ENTITY.data.flags[entity])

#define eIsActive(entity) (eFlag(entity) & ENTITY_FLAG_ACTIVE)
#define eSetActive(entity, isActive) (eFlag(entity) = ((isActive) ? (eFlag(entity) | ENTITY_FLAG_ACTIVE) : (eFlag(entity) & (uint8_t)~ENTITY_FLAG_ACTIVE)))

#define eAssertEntity(entity) RJ_DebugAssert((entity) < ENTITY.data.count + ENTITY.data.freeIndices.count && entity != RJ_INDEX_INVALID && eIsActive(entity), "Entity %u either exceeds maximum possible index %u, invalid or inactive.", (entity), ENTITY.data.count + ENTITY.data.freeIndices.count)

#pragma endregion SourceOnly

RJ_ResultWarn Entity_Initialize(RJ_Size initialEntityCapacity)
{
    ENTITY.data.capacity = initialEntityCapacity;
    ENTITY.data.count = 0;

    ListArray_Create(&ENTITY.data.freeIndices, "Entity Free Indices", sizeof(RJ_Size), ENTITY_INITIAL_FREE_INDEX_ARRAY_SIZE);

    RJ_ReturnAllocate(Vector3, ENTITY.data.positions, initialEntityCapacity,
                      ListArray_Destroy(&ENTITY.data.freeIndices);
                      ENTITY.data.capacity = 0;);

    RJ_ReturnAllocate(Vector3, ENTITY.data.rotations, initialEntityCapacity,
                      ListArray_Destroy(&ENTITY.data.freeIndices);
                      free(ENTITY.data.positions);
                      ENTITY.data.positions = NULL;
                      ENTITY.data.capacity = 0;);

    RJ_ReturnAllocate(Vector3, ENTITY.data.scales, initialEntityCapacity,
                      ListArray_Destroy(&ENTITY.data.freeIndices);
                      free(ENTITY.data.positions);
                      free(ENTITY.data.rotations);
                      ENTITY.data.positions = NULL;
                      ENTITY.data.rotations = NULL;
                      ENTITY.data.capacity = 0;);

    RJ_ReturnAllocate(uint8_t, ENTITY.data.flags, initialEntityCapacity,
                      ListArray_Destroy(&ENTITY.data.freeIndices);
                      free(ENTITY.data.positions);
                      free(ENTITY.data.rotations);
                      free(ENTITY.data.scales);
                      ENTITY.data.positions = NULL;
                      ENTITY.data.rotations = NULL;
                      ENTITY.data.scales = NULL;
                      ENTITY.data.capacity = 0;);

    RJ_DebugInfo("Entity data initialized with component capacity %u.", initialEntityCapacity);
    return RJ_OK;
}

void Entity_Terminate()
{
    ENTITY.data.capacity = 0;
    ENTITY.data.count = 0;
    ListArray_Destroy(&ENTITY.data.freeIndices);

    free(ENTITY.data.positions);
    free(ENTITY.data.rotations);
    free(ENTITY.data.scales);

    ENTITY.data.positions = NULL;
    ENTITY.data.rotations = NULL;
    ENTITY.data.scales = NULL;

    RJ_DebugInfo("Entity data terminated successfully.");
}

Entity Entity_Create(Vector3 position, Vector3 rotation, Vector3 scale)
{
    RJ_DebugAssert(ENTITY.data.count + ENTITY.data.freeIndices.count < ENTITY.data.capacity, "Maximum Entity capacity of %u reached.", ENTITY.data.capacity);

    Entity newEntity = ENTITY.data.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&ENTITY.data.freeIndices)) : ENTITY.data.count;

    ePosition(newEntity) = position;
    eRotation(newEntity) = rotation;
    eScale(newEntity) = scale;
    eSetActive(newEntity, true);

    ENTITY.data.count++;

    return newEntity;
}

void Entity_Destroy(Entity entity)
{
    eAssertEntity(entity);

    eSetActive(entity, false);
    ListArray_Add(&ENTITY.data.freeIndices, &entity);

    ENTITY.data.count--;
}

Vector3 Entity_GetPosition(Entity entity)
{
    eAssertEntity(entity);
    return ePosition(entity);
}

Vector3 Entity_GetRotation(Entity entity)
{
    eAssertEntity(entity);
    return eRotation(entity);
}

Vector3 Entity_GetScale(Entity entity)
{
    eAssertEntity(entity);
    return eScale(entity);
}

void Entity_SetPosition(Entity entity, Vector3 position)
{
    eAssertEntity(entity);
    ePosition(entity) = position;
}

void Entity_SetRotation(Entity entity, Vector3 rotation)
{
    eAssertEntity(entity);
    eRotation(entity) = rotation;
}

void Entity_SetScale(Entity entity, Vector3 scale)
{
    eAssertEntity(entity);
    eScale(entity) = scale;
}
