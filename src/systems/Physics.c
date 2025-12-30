#include "systems/Physics.h"

#include "utilities/Maths.h"
#include "utilities/ListArray.h"

#define PHYSICS_FLAG_ACTIVE (1 << 0)
#define PHYSICS_FLAG_STATIC (1 << 1)
#define PHYSICS_SEPARATION_EPSILON 0.001f

#pragma region Source Only

struct PHYSICS_MAIN_SCENE
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
        ListArray freeIndices; // RJ_Size

        RJ_Size *entities;

        Vector3 *velocities;
        Vector3 *colliderSizes;
        float *masses;
        uint8_t *flags;

        Vector3 *positionReferences;
    } data;
} PMS = {0};

#define pmsEntity(component) (PMS.data.entities[component])

#define pmsVelocity(component) (PMS.data.velocities[component])
#define pmsColliderSize(component) (PMS.data.colliderSizes[component])
#define pmsMass(component) (PMS.data.masses[component])
#define pmsFlag(component) (PMS.data.flags[component])

#define pmsPositionReference(component) (PMS.data.positionReferences[pmsEntity(component)])

#define pmsIsActive(component) (pmsFlag(component) & PHYSICS_FLAG_ACTIVE)
#define pmsSetActive(component, isActive) (pmsFlag(component) = isActive ? (pmsFlag(component) | PHYSICS_FLAG_ACTIVE) : (pmsFlag(component) & ~PHYSICS_FLAG_ACTIVE))

#define pmsIsStatic(component) (pmsFlag(component) & PHYSICS_FLAG_STATIC)
#define pmsSetStatic(component, isStatic) (pmsFlag(component) = isStatic ? (pmsFlag(component) | PHYSICS_FLAG_STATIC) : (pmsFlag(component) & ~PHYSICS_FLAG_STATIC))

#define pmsAssertComponent(component) RJ_DebugAssert(component < PMS.data.count + PMS.data.freeIndices.count && pmsEntity(component) != RJ_INDEX_INVALID && pmsIsActive(component), "Physics component %u either exceeds maximum possible index %u, invalid or inactive.", component, PMS.data.count + PMS.data.freeIndices.count)

/// @brief Resolve a collision between a static and dynamic physics component.
/// @param staticComponent Static physics component.
/// @param dynamicComponent Dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveStaticVsDynamic(PhysicsComponent staticComponent, PhysicsComponent dynamicComponent, Vector3 overlap)
{
    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        pmsPositionReference(dynamicComponent).x +=
            pmsPositionReference(dynamicComponent).x < pmsPositionReference(staticComponent).x
                ? -(overlap.x + PHYSICS_SEPARATION_EPSILON)
                : (overlap.x + PHYSICS_SEPARATION_EPSILON);

        pmsVelocity(dynamicComponent).x *= -PMS.properties.elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        pmsPositionReference(dynamicComponent).y +=
            pmsPositionReference(dynamicComponent).y < pmsPositionReference(staticComponent).y
                ? -(overlap.y + PHYSICS_SEPARATION_EPSILON)
                : (overlap.y + PHYSICS_SEPARATION_EPSILON);

        pmsVelocity(dynamicComponent).y *= -PMS.properties.elasticity;
    }
    else
    {
        pmsPositionReference(dynamicComponent).z +=
            pmsPositionReference(dynamicComponent).z < pmsPositionReference(staticComponent).z
                ? -(overlap.z + PHYSICS_SEPARATION_EPSILON)
                : (overlap.z + PHYSICS_SEPARATION_EPSILON);

        pmsVelocity(dynamicComponent).z *= -PMS.properties.elasticity;
    }
}

/// @brief Resolve a collision between two dynamic physics components.
/// @param firstComponent First dynamic physics component.
/// @param secondComponent Second dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
static void PhysicsScene_ResolveDynamicVsDynamic(PhysicsComponent firstComponent, PhysicsComponent secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / pmsMass(firstComponent) + 1.0f / pmsMass(secondComponent);
    float move1 = 0.0f;
    float move2 = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / pmsMass(firstComponent)) / totalInvMass * overlap.x;
        move2 = (1.0f / pmsMass(secondComponent)) / totalInvMass * overlap.x;

        if (pmsPositionReference(firstComponent).x < pmsPositionReference(secondComponent).x)
        {
            pmsPositionReference(firstComponent).x -= move1;
            pmsPositionReference(secondComponent).x += move2;
        }
        else
        {
            pmsPositionReference(firstComponent).x += move1;
            pmsPositionReference(secondComponent).x -= move2;
        }
    }
    else if (overlap.y < overlap.z)
    {
        move1 = (1.0f / pmsMass(firstComponent)) / totalInvMass * overlap.y;
        move2 = (1.0f / pmsMass(secondComponent)) / totalInvMass * overlap.y;

        if (pmsPositionReference(firstComponent).y < pmsPositionReference(secondComponent).y)
        {
            pmsPositionReference(firstComponent).y -= move1;
            pmsPositionReference(secondComponent).y += move2;
        }
        else
        {
            pmsPositionReference(firstComponent).y += move1;
            pmsPositionReference(secondComponent).y -= move2;
        }
    }
    else
    {
        move1 = (1.0f / pmsMass(firstComponent)) / totalInvMass * overlap.z;
        move2 = (1.0f / pmsMass(secondComponent)) / totalInvMass * overlap.z;

        if (pmsPositionReference(firstComponent).z < pmsPositionReference(secondComponent).z)
        {
            pmsPositionReference(firstComponent).z -= move1;
            pmsPositionReference(secondComponent).z += move2;
        }
        else
        {
            pmsPositionReference(firstComponent).z += move1;
            pmsPositionReference(secondComponent).z -= move2;
        }
    }

    // v1' = ( (m1 - e*m2)*v1 + (1+e)*m2*v2 ) / (m1+m2)
    // v2' = ( (m2 - e*m1)*v2 + (1+e)*m1*v1 ) / (m1+m2)

    Vector3 tempVelocity1 = pmsVelocity(firstComponent);
    float oneOverMassSum = 1.0f / (pmsMass(firstComponent) + pmsMass(secondComponent));
    float onePlusElasticity = 1.0f + PMS.properties.elasticity;

    pmsVelocity(firstComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsVelocity(firstComponent),
                              pmsMass(firstComponent) - PMS.properties.elasticity * pmsMass(secondComponent)),
                Vector3_Scale(
                    pmsVelocity(secondComponent),
                    onePlusElasticity * pmsMass(secondComponent))),
            oneOverMassSum);

    pmsVelocity(secondComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsVelocity(secondComponent),
                              pmsMass(secondComponent) - PMS.properties.elasticity * pmsMass(firstComponent)),
                Vector3_Scale(
                    tempVelocity1,
                    onePlusElasticity * pmsMass(firstComponent))),
            oneOverMassSum);
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

    if (pmsIsStatic(firstComponent))
    {
        PhysicsScene_ResolveStaticVsDynamic(firstComponent, secondComponent, overlap);
    }
    else if (pmsIsStatic(secondComponent))
    {
        PhysicsScene_ResolveStaticVsDynamic(secondComponent, firstComponent, overlap);
    }
    else
    {
        PhysicsScene_ResolveDynamicVsDynamic(firstComponent, secondComponent, overlap);
    }
}

#pragma endregion Source Only

void Physics_Initialize(RJ_Size componentCapacity, Vector3 *positionReferences, float drag, float gravity, float elasticity)
{
    RJ_DebugAssertNullPointerCheck(positionReferences);

    PMS.data.capacity = componentCapacity;
    PMS.data.count = 0;
    PMS.data.freeIndices = ListArray_Create("RJ_Size", sizeof(RJ_Size), PHYSICS_INITIAL_FREE_INDEX_ARRAY_SIZE);

    PMS.properties.drag = drag;
    PMS.properties.gravity = gravity;
    PMS.properties.elasticity = elasticity;

    RJ_DebugAssertAllocationCheck(RJ_Size, PMS.data.entities, PMS.data.capacity);

    RJ_DebugAssertAllocationCheck(Vector3, PMS.data.velocities, PMS.data.capacity);

    RJ_DebugAssertAllocationCheck(Vector3, PMS.data.colliderSizes, PMS.data.capacity);
    RJ_DebugAssertAllocationCheck(float, PMS.data.masses, PMS.data.capacity);
    RJ_DebugAssertAllocationCheck(uint8_t, PMS.data.flags, PMS.data.capacity);

    PMS.data.positionReferences = positionReferences;

    RJ_DebugInfo("Physics initialized with component capacity %u.", PMS.data.capacity);
}

void Physics_Terminate(void)
{
    PMS.data.capacity = 0;
    PMS.data.count = 0;
    ListArray_Destroy(&PMS.data.freeIndices);

    PMS.properties.drag = 0.0f;
    PMS.properties.gravity = 0.0f;
    PMS.properties.elasticity = 0.0f;

    free(PMS.data.entities);
    free(PMS.data.velocities);
    free(PMS.data.colliderSizes);
    free(PMS.data.masses);
    free(PMS.data.flags);

    PMS.data.entities = NULL;
    PMS.data.velocities = NULL;
    PMS.data.colliderSizes = NULL;
    PMS.data.masses = NULL;
    PMS.data.flags = NULL;

    RJ_DebugInfo("Physics terminated successfully.");
}

void Physics_ConfigureReferences(Vector3 *positionReferences, RJ_Size newCapacity)
{
    RJ_DebugAssertNullPointerCheck(positionReferences);
    RJ_DebugAssert(newCapacity > PMS.data.count, "New component capacity must be greater than current physics component count.");

    PMS.data.positionReferences = positionReferences;
    PMS.data.capacity = newCapacity;

    RJ_DebugAssertReallocationCheck(RJ_Size, PMS.data.entities, PMS.data.capacity);
    RJ_DebugAssertReallocationCheck(Vector3, PMS.data.velocities, PMS.data.capacity);
    RJ_DebugAssertReallocationCheck(Vector3, PMS.data.colliderSizes, PMS.data.capacity);
    RJ_DebugAssertReallocationCheck(float, PMS.data.masses, PMS.data.capacity);
    RJ_DebugAssertReallocationCheck(uint8_t, PMS.data.flags, PMS.data.capacity);

    RJ_DebugInfo("Physics position references reconfigured with new capacity %u.", PMS.data.capacity);
}

bool Physics_IsColliding(PhysicsComponent component1, PhysicsComponent component2, Vector3 *overlapRet)
{
    Vector3 position1 = pmsPositionReference(component1);
    Vector3 position2 = pmsPositionReference(component2);

    Vector3 colliderSize1 = pmsColliderSize(component1);
    Vector3 colliderSize2 = pmsColliderSize(component2);

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
    for (RJ_Size component = 0; component < PMS.data.count; component++)
    {
        if (!pmsIsActive(component))
        {
            continue;
        }

        if (pmsIsStatic(component))
        {
            continue;
        }

        pmsVelocity(component) = Vector3_Add(pmsVelocity(component), Vector3_New(0.0f, PMS.properties.gravity * deltaTime, 0.0f));
        pmsVelocity(component) = Vector3_Scale(pmsVelocity(component), 1.0f - PMS.properties.drag);
        pmsPositionReference(component) = Vector3_Add(pmsPositionReference(component), Vector3_Scale(pmsVelocity(component), deltaTime));
    }
}

void Physics_ResolveCollisions(void)
{
    for (RJ_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (RJ_Size firstComponent = 0; firstComponent < PMS.data.count; firstComponent++)
        {
            if (!pmsIsActive(firstComponent))
            {
                continue;
            }

            for (RJ_Size secondComponent = firstComponent + 1; secondComponent < PMS.data.count; secondComponent++)
            {
                if (!pmsIsActive(secondComponent))
                {
                    continue;
                }

                PhysicsScene_ResolveCollision(firstComponent, secondComponent);
            }
        }
    }
}

PhysicsComponent Physics_ComponentCreate(RJ_Size entity, Vector3 colliderSize, float mass, bool isStatic)
{
    RJ_DebugAssert(PMS.data.count + PMS.data.freeIndices.count < PMS.data.capacity, "Maximum physics component capacity of %u reached.", PMS.data.capacity);

    PhysicsComponent newComponent = PMS.data.freeIndices.count != 0 ? *((RJ_Size *)ListArray_Pop(&PMS.data.freeIndices)) : PMS.data.count;

    pmsEntity(newComponent) = entity;
    pmsColliderSize(newComponent) = colliderSize;
    pmsMass(newComponent) = mass;
    pmsSetActive(newComponent, true);
    pmsSetStatic(newComponent, isStatic);

    PMS.data.count++;

    return newComponent;
}

void Physics_ComponentDestroy(PhysicsComponent component)
{
    pmsAssertComponent(component);

    pmsEntity(component) = RJ_INDEX_INVALID;

    pmsVelocity(component) = Vector3_Zero;
    pmsColliderSize(component) = Vector3_Zero;
    pmsMass(component) = 0.0f;
    pmsFlag(component) = false;

    ListArray_Add(&PMS.data.freeIndices, &component);

    PMS.data.count--;
}

Vector3 Physics_ComponentGetVelocity(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsVelocity(component);
}

void Physics_ComponentSetVelocity(PhysicsComponent component, Vector3 newVelocity)
{
    pmsAssertComponent(component);
    pmsVelocity(component) = newVelocity;
}

Vector3 Physics_ComponentGetColliderSize(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsColliderSize(component);
}

void Physics_ComponentSetColliderSize(PhysicsComponent component, Vector3 newColliderSize)
{
    pmsAssertComponent(component);
    pmsColliderSize(component) = newColliderSize;
}

float Physics_ComponentGetMass(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsMass(component);
}

void Physics_ComponentSetMass(PhysicsComponent component, float newMass)
{
    pmsAssertComponent(component);
    pmsMass(component) = newMass;
}

bool Physics_ComponentIsActive(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsIsActive(component);
}

void Physics_ComponentSetActive(PhysicsComponent component, bool newIsActive)
{
    pmsAssertComponent(component);
    pmsSetActive(component, newIsActive);
}

bool Physics_ComponentIsStatic(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsIsStatic(component);
}

void Physics_ComponentSetStatic(PhysicsComponent component, bool newIsStatic)
{
    pmsAssertComponent(component);
    pmsSetStatic(component, newIsStatic);
}
