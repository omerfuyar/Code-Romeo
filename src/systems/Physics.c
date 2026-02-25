#include "systems/Physics.h"

#include "tools/Entity.h"

#include "utilities/Maths.h"
#include "utilities/ListArray.h"

#pragma region Source Only

#define PHYSICS_FLAG_STATIC (1 << 0)
#define PHYSICS_SEPARATION_EPSILON 0.001f

struct PHYSICS
{
    struct PHYSICS_PROPERTIES
    {
        float drag;
        float gravity;
        float elasticity;
    } properties;

    struct PHYSICS_DATA
    {
        RJ_Size capacity;
        RJ_Size count;

        Entity *entityToCompMap;
        Entity *compToEntityMap;

        Vector3 *velocities;
        Vector3 *colliderSizes;
        float *masses;
        uint8_t *flags;
    } data;
} PHYSICS = {0};

#define rEntity(component) (PHYSICS.data.compToEntityMap[component])
#define rComponent(entity) (PHYSICS.data.entityToCompMap[entity])

#define pVelocity(component) (PHYSICS.data.velocities[component])
#define pColliderSize(component) (PHYSICS.data.colliderSizes[component])
#define pMass(component) (PHYSICS.data.masses[component])
#define pFlag(component) (PHYSICS.data.flags[component])

#define pIsStatic(component) (pFlag(component) & PHYSICS_FLAG_STATIC)
#define pSetStatic(component, isStatic) (pFlag(component) = ((isStatic) ? (pFlag(component) | PHYSICS_FLAG_STATIC) : (pFlag(component) & (uint8_t)~PHYSICS_FLAG_STATIC)))

#define pAssertComponent(entity) RJ_DebugAssert((entity) != RJ_INDEX_INVALID &&                                                              \
                                                    rComponent(entity) != RJ_INDEX_INVALID &&                                                \
                                                    rEntity(rComponent(entity)) == entity &&                                                 \
                                                    rComponent(entity) < PHYSICS.data.count,                                                 \
                                                "Physics component %u or Entity %u either exceeds maximum possible index %u or is invalid.", \
                                                rComponent(entity), entity, PHYSICS.data.count)

/// @brief Resolve a collision between a static and dynamic physics component.
/// @param staticComponent Static physics component.
/// @param dynamicComponent Dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveStaticVsDynamic(Entity staticComponent, Entity dynamicComponent, Vector3 overlap)
{
    Vector3 staticPos = Entity_GetPosition(rEntity(staticComponent));
    Vector3 dynamicPos = Entity_GetPosition(rEntity(dynamicComponent));

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        dynamicPos.x += dynamicPos.x < staticPos.x
                            ? -(overlap.x + PHYSICS_SEPARATION_EPSILON)
                            : (overlap.x + PHYSICS_SEPARATION_EPSILON);

        pVelocity(dynamicComponent).x *= -PHYSICS.properties.elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        dynamicPos.y += dynamicPos.y < staticPos.y
                            ? -(overlap.y + PHYSICS_SEPARATION_EPSILON)
                            : (overlap.y + PHYSICS_SEPARATION_EPSILON);

        pVelocity(dynamicComponent).y *= -PHYSICS.properties.elasticity;
    }
    else
    {
        dynamicPos.z += dynamicPos.z < staticPos.z
                            ? -(overlap.z + PHYSICS_SEPARATION_EPSILON)
                            : (overlap.z + PHYSICS_SEPARATION_EPSILON);

        pVelocity(dynamicComponent).z *= -PHYSICS.properties.elasticity;
    }

    Entity_SetPosition(rEntity(dynamicComponent), dynamicPos);
}

/// @brief Resolve a collision between two dynamic physics components.
/// @param firstComponent First dynamic physics component.
/// @param secondComponent Second dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveDynamicVsDynamic(Entity firstComponent, Entity secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / pMass(firstComponent) + 1.0f / pMass(secondComponent);
    float move1 = 0.0f;
    float move2 = 0.0f;

    Vector3 firstPos = Entity_GetPosition(rEntity(firstComponent));
    Vector3 secondPos = Entity_GetPosition(rEntity(secondComponent));

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / pMass(firstComponent)) / totalInvMass * overlap.x;
        move2 = (1.0f / pMass(secondComponent)) / totalInvMass * overlap.x;

        if (firstPos.x < secondPos.x)
        {
            firstPos.x -= move1;
            secondPos.x += move2;
        }
        else
        {
            firstPos.x += move1;
            secondPos.x -= move2;
        }
    }
    else if (overlap.y < overlap.z)
    {
        move1 = (1.0f / pMass(firstComponent)) / totalInvMass * overlap.y;
        move2 = (1.0f / pMass(secondComponent)) / totalInvMass * overlap.y;

        if (firstPos.y < secondPos.y)
        {
            firstPos.y -= move1;
            secondPos.y += move2;
        }
        else
        {
            firstPos.y += move1;
            secondPos.y -= move2;
        }
    }
    else
    {
        move1 = (1.0f / pMass(firstComponent)) / totalInvMass * overlap.z;
        move2 = (1.0f / pMass(secondComponent)) / totalInvMass * overlap.z;

        if (firstPos.z < secondPos.z)
        {
            firstPos.z -= move1;
            secondPos.z += move2;
        }
        else
        {
            firstPos.z += move1;
            secondPos.z -= move2;
        }
    }

    Entity_SetPosition(rEntity(firstComponent), firstPos);
    Entity_SetPosition(rEntity(secondComponent), secondPos);

    // v1' = ( (m1 - e*m2)*v1 + (1+e)*m2*v2 ) / (m1+m2)
    // v2' = ( (m2 - e*m1)*v2 + (1+e)*m1*v1 ) / (m1+m2)

    float m1 = pMass(firstComponent);
    float m2 = pMass(secondComponent);
    float oneOverMassSum = 1.0f / (pMass(firstComponent) + pMass(secondComponent));

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        float v1 = pVelocity(firstComponent).x;
        float v2 = pVelocity(secondComponent).x;
        pVelocity(firstComponent).x = ((m1 - PHYSICS.properties.elasticity * m2) * v1 + (1.0f + PHYSICS.properties.elasticity) * m2 * v2) * oneOverMassSum;
        pVelocity(secondComponent).x = ((m2 - PHYSICS.properties.elasticity * m1) * v2 + (1.0f + PHYSICS.properties.elasticity) * m1 * v1) * oneOverMassSum;
    }
    else if (overlap.y < overlap.z)
    {
        float v1 = pVelocity(firstComponent).y;
        float v2 = pVelocity(secondComponent).y;
        pVelocity(firstComponent).y = ((m1 - PHYSICS.properties.elasticity * m2) * v1 + (1.0f + PHYSICS.properties.elasticity) * m2 * v2) * oneOverMassSum;
        pVelocity(secondComponent).y = ((m2 - PHYSICS.properties.elasticity * m1) * v2 + (1.0f + PHYSICS.properties.elasticity) * m1 * v1) * oneOverMassSum;
    }
    else
    {
        float v1 = pVelocity(firstComponent).z;
        float v2 = pVelocity(secondComponent).z;
        pVelocity(firstComponent).z = ((m1 - PHYSICS.properties.elasticity * m2) * v1 + (1.0f + PHYSICS.properties.elasticity) * m2 * v2) * oneOverMassSum;
        pVelocity(secondComponent).z = ((m2 - PHYSICS.properties.elasticity * m1) * v2 + (1.0f + PHYSICS.properties.elasticity) * m1 * v1) * oneOverMassSum;
    }
}

/// @brief Resolve a collision between two physics components. Which resolution method to use is determined by whether components are static or dynamic.
/// @param firstComponent First physics component.
/// @param secondComponent Second physics component.
static void PhysicsScene_ResolveCollision(Entity firstComponent, Entity secondComponent)
{
    Vector3 overlap;

    if (!Physics_IsColliding(rEntity(firstComponent), rEntity(secondComponent), &overlap))
    {
        return;
    }

    if (pIsStatic(firstComponent))
    {
        PhysicsScene_ResolveStaticVsDynamic(firstComponent, secondComponent, overlap);
    }
    else if (pIsStatic(secondComponent))
    {
        PhysicsScene_ResolveStaticVsDynamic(secondComponent, firstComponent, overlap);
    }
    else
    {
        PhysicsScene_ResolveDynamicVsDynamic(firstComponent, secondComponent, overlap);
    }
}

#pragma endregion Source Only

RJ_ResultWarn Physics_Initialize(RJ_Size initialComponentCapacity, float drag, float gravity, float elasticity)
{
    PHYSICS.data.capacity = initialComponentCapacity;
    PHYSICS.data.count = 0;

    PHYSICS.properties.drag = drag;
    PHYSICS.properties.gravity = gravity;
    PHYSICS.properties.elasticity = elasticity;

    RJ_ReturnAllocate(Entity, PHYSICS.data.entityToCompMap, PHYSICS.data.capacity);

    RJ_ReturnAllocate(Entity, PHYSICS.data.compToEntityMap, PHYSICS.data.capacity,
                      free(PHYSICS.data.entityToCompMap););

    RJ_ReturnAllocate(Vector3, PHYSICS.data.velocities, PHYSICS.data.capacity,
                      free(PHYSICS.data.compToEntityMap);
                      free(PHYSICS.data.entityToCompMap););

    RJ_ReturnAllocate(Vector3, PHYSICS.data.colliderSizes, PHYSICS.data.capacity,
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.compToEntityMap);
                      free(PHYSICS.data.entityToCompMap););

    RJ_ReturnAllocate(float, PHYSICS.data.masses, PHYSICS.data.capacity,
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.colliderSizes);
                      free(PHYSICS.data.compToEntityMap);
                      free(PHYSICS.data.entityToCompMap););

    RJ_ReturnAllocate(uint8_t, PHYSICS.data.flags, PHYSICS.data.capacity,
                      free(PHYSICS.data.masses);
                      free(PHYSICS.data.colliderSizes);
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.compToEntityMap);
                      free(PHYSICS.data.entityToCompMap););

    memset(PHYSICS.data.entityToCompMap, 0xff, sizeof(Entity) * initialComponentCapacity);
    memset(PHYSICS.data.compToEntityMap, 0xff, sizeof(Entity) * initialComponentCapacity);

    RJ_DebugInfo("Physics initialized with component capacity %u.", PHYSICS.data.capacity);

    return RJ_OK;
}

void Physics_Terminate(void)
{
    PHYSICS.data.capacity = 0;
    PHYSICS.data.count = 0;

    PHYSICS.properties.drag = 0.0f;
    PHYSICS.properties.gravity = 0.0f;
    PHYSICS.properties.elasticity = 0.0f;

    free(PHYSICS.data.entityToCompMap);
    free(PHYSICS.data.compToEntityMap);
    free(PHYSICS.data.velocities);
    free(PHYSICS.data.colliderSizes);
    free(PHYSICS.data.masses);
    free(PHYSICS.data.flags);

    PHYSICS.data.entityToCompMap = NULL;
    PHYSICS.data.compToEntityMap = NULL;
    PHYSICS.data.velocities = NULL;
    PHYSICS.data.colliderSizes = NULL;
    PHYSICS.data.masses = NULL;
    PHYSICS.data.flags = NULL;

    RJ_DebugInfo("Physics terminated successfully.");
}

RJ_ResultWarn Physics_Configure(RJ_Size newCapacity)
{
    RJ_DebugAssert(newCapacity > PHYSICS.data.count, "New component capacity must be greater than current physics component count.");

    PHYSICS.data.capacity = newCapacity;

    RJ_ReturnReallocate(Entity, PHYSICS.data.entityToCompMap, PHYSICS.data.capacity);

    RJ_ReturnReallocate(Entity, PHYSICS.data.compToEntityMap, PHYSICS.data.capacity,
                        free(PHYSICS.data.entityToCompMap););

    RJ_ReturnReallocate(Vector3, PHYSICS.data.velocities, PHYSICS.data.capacity,
                        free(PHYSICS.data.compToEntityMap);
                        free(PHYSICS.data.entityToCompMap););

    RJ_ReturnReallocate(Vector3, PHYSICS.data.colliderSizes, PHYSICS.data.capacity,
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.compToEntityMap);
                        free(PHYSICS.data.entityToCompMap););

    RJ_ReturnReallocate(float, PHYSICS.data.masses, PHYSICS.data.capacity,
                        free(PHYSICS.data.colliderSizes);
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.compToEntityMap);
                        free(PHYSICS.data.entityToCompMap););

    RJ_ReturnReallocate(uint8_t, PHYSICS.data.flags, PHYSICS.data.capacity,
                        free(PHYSICS.data.masses);
                        free(PHYSICS.data.colliderSizes);
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.compToEntityMap);
                        free(PHYSICS.data.entityToCompMap););

    RJ_DebugInfo("Physics position references reconfigured with new capacity %u.", PHYSICS.data.capacity);
    return RJ_OK;
}

bool Physics_IsColliding(Entity entity1, Entity entity2, Vector3 *overlapRet)
{
    Vector3 position1 = Entity_GetPosition(entity1);
    Vector3 position2 = Entity_GetPosition(entity2);

    Vector3 colliderSize1 = pColliderSize(rComponent(entity1));
    Vector3 colliderSize2 = pColliderSize(rComponent(entity2));

    float overlapX = Maths_Min(position1.x + colliderSize1.x / 2.0f,
                               position2.x + colliderSize2.x / 2.0f) -
                     Maths_Max(position1.x - colliderSize1.x / 2.0f,
                               position2.x - colliderSize2.x / 2.0f);

    float overlapY = Maths_Min(position1.y + colliderSize1.y / 2.0f,
                               position2.y + colliderSize2.y / 2.0f) -
                     Maths_Max(position1.y - colliderSize1.y / 2.0f,
                               position2.y - colliderSize2.y / 2.0f);

    float overlapZ = Maths_Min(position1.z + colliderSize1.z / 2.0f,
                               position2.z + colliderSize2.z / 2.0f) -
                     Maths_Max(position1.z - colliderSize1.z / 2.0f,
                               position2.z - colliderSize2.z / 2.0f);

    if (overlapRet != NULL)
    {
        *overlapRet = Vector3_New(overlapX, overlapY, overlapZ);
    }

    return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
}

void Physics_UpdateComponents(float deltaTime)
{
    for (Entity component = 0; component < PHYSICS.data.count; component++)
    {
        if (pIsStatic(component))
        {
            continue;
        }

        pVelocity(component) = Vector3_Sum(pVelocity(component), Vector3_New(0.0f, PHYSICS.properties.gravity * deltaTime, 0.0f));
        pVelocity(component) = Vector3_Scale(pVelocity(component), 1.0f - PHYSICS.properties.drag);

        Entity_SetPosition(rEntity(component), Vector3_Sum(Entity_GetPosition(rEntity(component)), Vector3_Scale(pVelocity(component), deltaTime)));
    }
}

void Physics_ResolveCollisions(void)
{
    for (RJ_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (Entity firstComponent = 0; firstComponent < PHYSICS.data.count; firstComponent++)
        {
            for (Entity secondComponent = firstComponent + 1; secondComponent < PHYSICS.data.count; secondComponent++)
            {
                PhysicsScene_ResolveCollision(firstComponent, secondComponent);
            }
        }
    }
}

Entity Physics_ComponentCreate(Entity entity, Vector3 colliderSize, float mass, bool isStatic)
{
    RJ_DebugAssert(PHYSICS.data.count < PHYSICS.data.capacity, "Maximum physics component capacity of %u reached.", PHYSICS.data.capacity);

    Entity component = PHYSICS.data.count;

    rComponent(entity) = component;
    rEntity(component) = entity;

    pColliderSize(component) = colliderSize;
    pMass(component) = mass;
    pSetStatic(component, isStatic);

    PHYSICS.data.count++;

    return component;
}

void Physics_ComponentDestroy(Entity entity)
{
    pAssertComponent(entity);

    pVelocity(rComponent(entity)) = Vector3_Zero;
    pColliderSize(rComponent(entity)) = Vector3_Zero;
    pMass(rComponent(entity)) = 0.0f;
    pFlag(rComponent(entity)) = false;

    rEntity(rComponent(entity)) = RJ_INDEX_INVALID;
    rComponent(entity) = RJ_INDEX_INVALID;

    PHYSICS.data.count--;
}

Vector3 Physics_ComponentGetVelocity(Entity entity)
{
    pAssertComponent(entity);
    return pVelocity(rComponent(entity));
}

void Physics_ComponentSetVelocity(Entity entity, Vector3 newVelocity)
{
    pAssertComponent(entity);
    pVelocity(rComponent(entity)) = newVelocity;
}

Vector3 Physics_ComponentGetColliderSize(Entity entity)
{
    pAssertComponent(entity);
    return pColliderSize(rComponent(entity));
}

void Physics_ComponentSetColliderSize(Entity entity, Vector3 newColliderSize)
{
    pAssertComponent(entity);
    pColliderSize(rComponent(entity)) = newColliderSize;
}

float Physics_ComponentGetMass(Entity entity)
{
    pAssertComponent(entity);
    return pMass(rComponent(entity));
}

void Physics_ComponentSetMass(Entity entity, float newMass)
{
    pAssertComponent(entity);
    pMass(rComponent(entity)) = newMass;
}

bool Physics_ComponentIsStatic(Entity entity)
{
    pAssertComponent(entity);
    return pIsStatic(rComponent(entity));
}

void Physics_ComponentSetStatic(Entity entity, bool newIsStatic)
{
    pAssertComponent(entity);
    pSetStatic(rComponent(entity), newIsStatic);
}
