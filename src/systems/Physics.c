#include "systems/Physics.h"

#include "tools/Entity.h"

#include "utilities/Maths.h"
#include "utilities/ListArray.h"

#pragma region Source Only

#define PHYSICS_FLAG_STATIC (1 << 0)
#define PHYSICS_SEPARATION_EPSILON 0.001f

struct PHYSICS
{
    struct PHYSICS_INFO
    {
        RJ_Size capacity;
        RJ_Size count;
        ListArray freeIndices; // RJ_Size
    } info;

    struct PHYSICS_PROPERTIES
    {
        float drag;
        float gravity;
        float elasticity;
    } properties;

    struct PHYSICS_DATA
    {
        Entity *entityMap;
        Vector3 *velocities;
        Vector3 *colliderSizes;
        float *masses;
        uint8_t *flags;
    } data;
} PHYSICS = {0};

#define pEntity(component) (PHYSICS.data.entityMap[component])

#define pVelocity(component) (PHYSICS.data.velocities[component])
#define pColliderSize(component) (PHYSICS.data.colliderSizes[component])
#define pMass(component) (PHYSICS.data.masses[component])
#define pFlag(component) (PHYSICS.data.flags[component])

#define pIsStatic(component) (pFlag(component) & PHYSICS_FLAG_STATIC)
#define pSetStatic(component, isStatic) (pFlag(component) = ((isStatic) ? (pFlag(component) | PHYSICS_FLAG_STATIC) : (pFlag(component) & (uint8_t)~PHYSICS_FLAG_STATIC)))

#define pAssertComponent(component) RJ_DebugAssert((component) < PHYSICS.info.count + PHYSICS.info.freeIndices.count && pEntity(component) != RJ_INDEX_INVALID, "Physics component %u either exceeds maximum possible index %u or is invalid.", (component), PHYSICS.info.count + PHYSICS.info.freeIndices.count)

/// @brief Resolve a collision between a static and dynamic physics component.
/// @param staticComponent Static physics component.
/// @param dynamicComponent Dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveStaticVsDynamic(PhysicsComponent staticComponent, PhysicsComponent dynamicComponent, Vector3 overlap)
{
    Vector3 staticPos = Entity_GetPosition(pEntity(staticComponent));
    Vector3 dynamicPos = Entity_GetPosition(pEntity(dynamicComponent));

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

    Entity_SetPosition(pEntity(dynamicComponent), dynamicPos);
}

/// @brief Resolve a collision between two dynamic physics components.
/// @param firstComponent First dynamic physics component.
/// @param secondComponent Second dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveDynamicVsDynamic(PhysicsComponent firstComponent, PhysicsComponent secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / pMass(firstComponent) + 1.0f / pMass(secondComponent);
    float move1 = 0.0f;
    float move2 = 0.0f;

    Vector3 firstPos = Entity_GetPosition(pEntity(firstComponent));
    Vector3 secondPos = Entity_GetPosition(pEntity(secondComponent));

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

    Entity_SetPosition(pEntity(firstComponent), firstPos);
    Entity_SetPosition(pEntity(secondComponent), secondPos);

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
static void PhysicsScene_ResolveCollision(PhysicsComponent firstComponent, PhysicsComponent secondComponent)
{
    Vector3 overlap;

    if (!Physics_IsColliding(firstComponent, secondComponent, &overlap))
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
    PHYSICS.info.capacity = initialComponentCapacity;
    PHYSICS.info.count = 0;
    ListArray_Create(&PHYSICS.info.freeIndices, "RJ_Size", sizeof(RJ_Size), ENTITY_INITIAL_FREE_INDEX_ARRAY_SIZE);

    PHYSICS.properties.drag = drag;
    PHYSICS.properties.gravity = gravity;
    PHYSICS.properties.elasticity = elasticity;

    RJ_ReturnAllocate(RJ_Size, PHYSICS.data.entityMap, PHYSICS.info.capacity,
                      ListArray_Destroy(&PHYSICS.info.freeIndices););

    RJ_ReturnAllocate(Vector3, PHYSICS.data.velocities, PHYSICS.info.capacity,
                      ListArray_Destroy(&PHYSICS.info.freeIndices);
                      free(PHYSICS.data.entityMap););

    RJ_ReturnAllocate(Vector3, PHYSICS.data.colliderSizes, PHYSICS.info.capacity,
                      ListArray_Destroy(&PHYSICS.info.freeIndices);
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.entityMap););

    RJ_ReturnAllocate(float, PHYSICS.data.masses, PHYSICS.info.capacity,
                      ListArray_Destroy(&PHYSICS.info.freeIndices);
                      free(PHYSICS.data.colliderSizes);
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.entityMap););

    RJ_ReturnAllocate(uint8_t, PHYSICS.data.flags, PHYSICS.info.capacity,
                      ListArray_Destroy(&PHYSICS.info.freeIndices);
                      free(PHYSICS.data.masses);
                      free(PHYSICS.data.colliderSizes);
                      free(PHYSICS.data.velocities);
                      free(PHYSICS.data.entityMap););

    memset(PHYSICS.data.entityMap, 0xff, sizeof(RJ_Size) * initialComponentCapacity);

    RJ_DebugInfo("Physics initialized with component capacity %u.", PHYSICS.info.capacity);

    return RJ_OK;
}

void Physics_Terminate(void)
{
    PHYSICS.info.capacity = 0;
    PHYSICS.info.count = 0;
    ListArray_Destroy(&PHYSICS.info.freeIndices);

    PHYSICS.properties.drag = 0.0f;
    PHYSICS.properties.gravity = 0.0f;
    PHYSICS.properties.elasticity = 0.0f;

    free(PHYSICS.data.entityMap);
    free(PHYSICS.data.velocities);
    free(PHYSICS.data.colliderSizes);
    free(PHYSICS.data.masses);
    free(PHYSICS.data.flags);

    PHYSICS.data.entityMap = NULL;
    PHYSICS.data.velocities = NULL;
    PHYSICS.data.colliderSizes = NULL;
    PHYSICS.data.masses = NULL;
    PHYSICS.data.flags = NULL;

    RJ_DebugInfo("Physics terminated successfully.");
}

RJ_ResultWarn Physics_Configure(RJ_Size newCapacity)
{
    RJ_DebugAssert(newCapacity > PHYSICS.info.count, "New component capacity must be greater than current physics component count.");

    PHYSICS.info.capacity = newCapacity;

    RJ_ReturnReallocate(RJ_Size, PHYSICS.data.entityMap, PHYSICS.info.capacity);
    RJ_ReturnReallocate(Vector3, PHYSICS.data.velocities, PHYSICS.info.capacity,
                        free(PHYSICS.data.entityMap););
    RJ_ReturnReallocate(Vector3, PHYSICS.data.colliderSizes, PHYSICS.info.capacity,
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.entityMap););
    RJ_ReturnReallocate(float, PHYSICS.data.masses, PHYSICS.info.capacity,
                        free(PHYSICS.data.colliderSizes);
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.entityMap););
    RJ_ReturnReallocate(uint8_t, PHYSICS.data.flags, PHYSICS.info.capacity,
                        free(PHYSICS.data.masses);
                        free(PHYSICS.data.colliderSizes);
                        free(PHYSICS.data.velocities);
                        free(PHYSICS.data.entityMap););

    RJ_DebugInfo("Physics position references reconfigured with new capacity %u.", PHYSICS.info.capacity);
    return RJ_OK;
}

bool Physics_IsColliding(PhysicsComponent component1, PhysicsComponent component2, Vector3 *overlapRet)
{
    Vector3 position1 = Entity_GetPosition(pEntity(component1));
    Vector3 position2 = Entity_GetPosition(pEntity(component2));

    Vector3 colliderSize1 = pColliderSize(component1);
    Vector3 colliderSize2 = pColliderSize(component2);

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
    for (RJ_Size component = 0; component < PHYSICS.info.count; component++)
    {
        if (pIsStatic(component))
        {
            continue;
        }

        pVelocity(component) = Vector3_Sum(pVelocity(component), Vector3_New(0.0f, PHYSICS.properties.gravity * deltaTime, 0.0f));
        pVelocity(component) = Vector3_Scale(pVelocity(component), 1.0f - PHYSICS.properties.drag);
        Entity_SetPosition(pEntity(component), Vector3_Sum(Entity_GetPosition(pEntity(component)), Vector3_Scale(pVelocity(component), deltaTime)));
    }
}

void Physics_ResolveCollisions(void)
{
    for (RJ_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (RJ_Size firstComponent = 0; firstComponent < PHYSICS.info.count; firstComponent++)
        {
            for (RJ_Size secondComponent = firstComponent + 1; secondComponent < PHYSICS.info.count; secondComponent++)
            {
                PhysicsScene_ResolveCollision(firstComponent, secondComponent);
            }
        }
    }
}

PhysicsComponent Physics_ComponentCreate(RJ_Size entity, Vector3 colliderSize, float mass, bool isStatic)
{
    RJ_DebugAssert(PHYSICS.info.count + PHYSICS.info.freeIndices.count < PHYSICS.info.capacity, "Maximum physics component capacity of %u reached.", PHYSICS.info.capacity);

    PhysicsComponent newComponent = PHYSICS.info.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&PHYSICS.info.freeIndices)) : PHYSICS.info.count;

    pEntity(newComponent) = entity;
    pColliderSize(newComponent) = colliderSize;
    pMass(newComponent) = mass;
    pSetStatic(newComponent, isStatic);

    PHYSICS.info.count++;

    return newComponent;
}

void Physics_ComponentDestroy(PhysicsComponent component)
{
    pAssertComponent(component);

    pEntity(component) = RJ_INDEX_INVALID;

    pVelocity(component) = Vector3_Zero;
    pColliderSize(component) = Vector3_Zero;
    pMass(component) = 0.0f;
    pFlag(component) = false;

    ListArray_Add(&PHYSICS.info.freeIndices, &component);

    PHYSICS.info.count--;
}

Vector3 Physics_ComponentGetVelocity(PhysicsComponent component)
{
    pAssertComponent(component);
    return pVelocity(component);
}

void Physics_ComponentSetVelocity(PhysicsComponent component, Vector3 newVelocity)
{
    pAssertComponent(component);
    pVelocity(component) = newVelocity;
}

Vector3 Physics_ComponentGetColliderSize(PhysicsComponent component)
{
    pAssertComponent(component);
    return pColliderSize(component);
}

void Physics_ComponentSetColliderSize(PhysicsComponent component, Vector3 newColliderSize)
{
    pAssertComponent(component);
    pColliderSize(component) = newColliderSize;
}

float Physics_ComponentGetMass(PhysicsComponent component)
{
    pAssertComponent(component);
    return pMass(component);
}

void Physics_ComponentSetMass(PhysicsComponent component, float newMass)
{
    pAssertComponent(component);
    pMass(component) = newMass;
}

bool Physics_ComponentIsStatic(PhysicsComponent component)
{
    pAssertComponent(component);
    return pIsStatic(component);
}

void Physics_ComponentSetStatic(PhysicsComponent component, bool newIsStatic)
{
    pAssertComponent(component);
    pSetStatic(component, newIsStatic);
}
