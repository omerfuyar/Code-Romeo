#include "systems/Physics.h"

#include "utilities/Maths.h"
#include "utilities/ListArray.h"

#define PHYSICS_FLAG_ACTIVE (1 << 0)
#define PHYSICS_FLAG_STATIC (1 << 1)

#pragma region Source Only

struct PHYSICS_MAIN_SCENE
{
    struct PHYSICS_DATA
    {
        RJGlobal_Size capacity;
        RJGlobal_Size count;
        ListArray freeIndices; // RJGlobal_Index
    } data;

    struct PHYSICS_PROPERTIES
    {
        float drag;
        float gravity;
        float elasticity;
    } properties;

    struct PHYSICS_COMPONENTS
    {
        RJGlobal_Index *entities;

        Vector3 *velocities;
        Vector3 *colliderSizes;
        float *masses;
        uint8_t *flags;

        Vector3 *positionReferences;
    } components;
} PMS = {0};

#define pmsGetEntity(component) (PMS.components.entities[component])

#define pmsGetVelocity(component) (PMS.components.velocities[component])
#define pmsGetColliderSize(component) (PMS.components.colliderSizes[component])
#define pmsGetMass(component) (PMS.components.masses[component])
#define pmsGetFlag(component) (PMS.components.flags[component])

#define pmsGetPositionReference(component) (PMS.components.positionReferences[pmsGetEntity(component)])

#define pmsIsActive(component) (pmsGetFlag(component) & PHYSICS_FLAG_ACTIVE)
#define pmsSetActive(component, isActive) (pmsGetFlag(component) = isActive ? (pmsGetFlag(component) | PHYSICS_FLAG_ACTIVE) : (pmsGetFlag(component) & ~PHYSICS_FLAG_ACTIVE))

#define pmsIsStatic(component) (pmsGetFlag(component) & PHYSICS_FLAG_STATIC)
#define pmsSetStatic(component, isStatic) (pmsGetFlag(component) = isStatic ? (pmsGetFlag(component) | PHYSICS_FLAG_STATIC) : (pmsGetFlag(component) & ~PHYSICS_FLAG_STATIC))

#define pmsAssertComponent(component) RJGlobal_DebugAssert(component < PMS.data.count + PMS.data.freeIndices.count && pmsGetEntity(component) != RJGLOBAL_INDEX_INVALID && pmsIsActive(component), "Physics component %u either exceeds maximum possible index %u, invalid or inactive.", component, PMS.data.count + PMS.data.freeIndices.count)

/// @brief Resolve a collision between a static and dynamic physics component.
/// @param staticComponent Static physics component.
/// @param dynamicComponent Dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
void PhysicsScene_ResolveStaticVsDynamic(PhysicsComponent staticComponent, PhysicsComponent dynamicComponent, Vector3 overlap)
{
    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        pmsGetPositionReference(dynamicComponent).x +=
            pmsGetPositionReference(dynamicComponent).x < pmsGetPositionReference(staticComponent).x
                ? -overlap.x
                : overlap.x;

        pmsGetVelocity(dynamicComponent).x *= -PMS.properties.elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        pmsGetPositionReference(dynamicComponent).y +=
            pmsGetPositionReference(dynamicComponent).y < pmsGetPositionReference(staticComponent).y
                ? -overlap.y
                : overlap.y;

        pmsGetVelocity(dynamicComponent).y *= -PMS.properties.elasticity;
    }
    else
    {
        pmsGetPositionReference(dynamicComponent).z +=
            pmsGetPositionReference(dynamicComponent).z < pmsGetPositionReference(staticComponent).z
                ? -overlap.z
                : overlap.z;

        pmsGetVelocity(dynamicComponent).z *= -PMS.properties.elasticity;
    }
}

/// @brief Resolve a collision between two dynamic physics components.
/// @param firstComponent First dynamic physics component.
/// @param secondComponent Second dynamic physics component.
/// @param overlap Overlap vector indicating the penetration depth.
void PhysicsScene_ResolveDynamicVsDynamic(PhysicsComponent firstComponent, PhysicsComponent secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / pmsGetMass(firstComponent) + 1.0f / pmsGetMass(secondComponent);
    float move1 = 0.0f;
    float move2 = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / pmsGetMass(firstComponent)) / totalInvMass * overlap.x;
        move2 = (1.0f / pmsGetMass(secondComponent)) / totalInvMass * overlap.x;

        if (pmsGetPositionReference(firstComponent).x < pmsGetPositionReference(secondComponent).x)
        {
            pmsGetPositionReference(firstComponent).x -= move1;
            pmsGetPositionReference(secondComponent).x += move2;
        }
        else
        {
            pmsGetPositionReference(firstComponent).x += move1;
            pmsGetPositionReference(secondComponent).x -= move2;
        }
    }
    else if (overlap.y < overlap.z)
    {
        move1 = (1.0f / pmsGetMass(firstComponent)) / totalInvMass * overlap.y;
        move2 = (1.0f / pmsGetMass(secondComponent)) / totalInvMass * overlap.y;

        if (pmsGetPositionReference(firstComponent).y < pmsGetPositionReference(secondComponent).y)
        {
            pmsGetPositionReference(firstComponent).y -= move1;
            pmsGetPositionReference(secondComponent).y += move2;
        }
        else
        {
            pmsGetPositionReference(firstComponent).y += move1;
            pmsGetPositionReference(secondComponent).y -= move2;
        }
    }
    else
    {
        move1 = (1.0f / pmsGetMass(firstComponent)) / totalInvMass * overlap.z;
        move2 = (1.0f / pmsGetMass(secondComponent)) / totalInvMass * overlap.z;

        if (pmsGetPositionReference(firstComponent).z < pmsGetPositionReference(secondComponent).z)
        {
            pmsGetPositionReference(firstComponent).z -= move1;
            pmsGetPositionReference(secondComponent).z += move2;
        }
        else
        {
            pmsGetPositionReference(firstComponent).z += move1;
            pmsGetPositionReference(secondComponent).z -= move2;
        }
    }

    // v1' = ( (m1 - e*m2)*v1 + (1+e)*m2*v2 ) / (m1+m2)
    // v2' = ( (m2 - e*m1)*v2 + (1+e)*m1*v1 ) / (m1+m2)

    Vector3 tempVelocity1 = pmsGetVelocity(firstComponent);
    float oneOverMassSum = 1.0f / (pmsGetMass(firstComponent) + pmsGetMass(secondComponent));
    float onePlusElasticity = 1.0f + PMS.properties.elasticity;

    pmsGetVelocity(firstComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(firstComponent),
                              pmsGetMass(firstComponent) - PMS.properties.elasticity * pmsGetMass(secondComponent)),
                Vector3_Scale(
                    pmsGetVelocity(secondComponent),
                    onePlusElasticity * pmsGetMass(secondComponent))),
            oneOverMassSum);

    pmsGetVelocity(secondComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(secondComponent),
                              pmsGetMass(secondComponent) - PMS.properties.elasticity * pmsGetMass(firstComponent)),
                Vector3_Scale(
                    tempVelocity1,
                    onePlusElasticity * pmsGetMass(firstComponent))),
            oneOverMassSum);
}

/// @brief Resolve a collision between two physics components. Which resolution method to use is determined by whether components are static or dynamic.
/// @param firstComponent First physics component.
/// @param secondComponent Second physics component.
void PhysicsScene_ResolveCollision(PhysicsComponent firstComponent, PhysicsComponent secondComponent)
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

void Physics_Initialize(RJGlobal_Size componentCapacity, Vector3 *positionReferences, float drag, float gravity, float elasticity)
{
    RJGlobal_DebugAssertNullPointerCheck(positionReferences);

    PMS.data.capacity = componentCapacity;
    PMS.data.count = 0;
    PMS.data.freeIndices = ListArray_Create("RJGlobal_Index", sizeof(RJGlobal_Index), PHYSICS_INITIAL_FREE_INDEX_ARRAY_SIZE);

    PMS.properties.drag = drag;
    PMS.properties.gravity = gravity;
    PMS.properties.elasticity = elasticity;

    PMS.components.entities = (RJGlobal_Index *)malloc(sizeof(RJGlobal_Index) * PMS.data.capacity);

    PMS.components.velocities = (Vector3 *)malloc(sizeof(Vector3) * PMS.data.capacity);
    PMS.components.colliderSizes = (Vector3 *)malloc(sizeof(Vector3) * PMS.data.capacity);
    PMS.components.masses = (float *)malloc(sizeof(float) * PMS.data.capacity);
    PMS.components.flags = (uint8_t *)malloc(sizeof(uint8_t) * PMS.data.capacity);

    PMS.components.positionReferences = positionReferences;

    RJGlobal_MemorySet(PMS.components.entities, sizeof(RJGlobal_Index) * PMS.data.capacity, 0);
    RJGlobal_MemorySet(PMS.components.velocities, sizeof(Vector3) * PMS.data.capacity, 0);
    RJGlobal_MemorySet(PMS.components.colliderSizes, sizeof(Vector3) * PMS.data.capacity, 0);
    RJGlobal_MemorySet(PMS.components.masses, sizeof(float) * PMS.data.capacity, 0);
    RJGlobal_MemorySet(PMS.components.flags, sizeof(uint8_t) * PMS.data.capacity, 0);

    RJGlobal_DebugInfo("Physics initialized with capacity %u.", PMS.data.capacity);
}

void Physics_Terminate()
{
    free(PMS.components.entities);
    free(PMS.components.velocities);
    free(PMS.components.colliderSizes);
    free(PMS.components.masses);
    free(PMS.components.flags);

    ListArray_Destroy(&PMS.data.freeIndices);

    PMS = (struct PHYSICS_MAIN_SCENE){0};

    RJGlobal_DebugInfo("Physics terminated successfully.");
}

void Physics_ConfigurePositionReferences(Vector3 *positionReferences, RJGlobal_Size limiting)
{
    RJGlobal_DebugAssertNullPointerCheck(positionReferences);
    RJGlobal_DebugAssert(limiting > PMS.data.count, "Limiting must be greater than current physics component count.");

    PMS.components.positionReferences = positionReferences;
    PMS.data.capacity = limiting;

    PMS.components.entities = (RJGlobal_Index *)realloc(PMS.components.entities, sizeof(RJGlobal_Index) * PMS.data.capacity);
    PMS.components.velocities = (Vector3 *)realloc(PMS.components.velocities, sizeof(Vector3) * PMS.data.capacity);
    PMS.components.colliderSizes = (Vector3 *)realloc(PMS.components.colliderSizes, sizeof(Vector3) * PMS.data.capacity);
    PMS.components.masses = (float *)realloc(PMS.components.masses, sizeof(float) * PMS.data.capacity);
    PMS.components.flags = (uint8_t *)realloc(PMS.components.flags, sizeof(uint8_t) * PMS.data.capacity);

    RJGlobal_DebugInfo("Physics position references reconfigured with new limiting %u.", PMS.data.capacity);
}

bool Physics_IsColliding(PhysicsComponent component1, PhysicsComponent component2, Vector3 *overlapRet)
{
    Vector3 position1 = pmsGetPositionReference(component1);
    Vector3 position2 = pmsGetPositionReference(component2);

    Vector3 colliderSize1 = pmsGetColliderSize(component1);
    Vector3 colliderSize2 = pmsGetColliderSize(component2);

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
    for (RJGlobal_Size component = 0; component < PMS.data.count; component++)
    {
        if (!pmsIsActive(component))
        {
            continue;
        }

        if (pmsIsStatic(component))
        {
            continue;
        }

        pmsGetVelocity(component) = Vector3_Add(pmsGetVelocity(component), Vector3_New(0.0f, PMS.properties.gravity * deltaTime, 0.0f));
        pmsGetVelocity(component) = Vector3_Scale(pmsGetVelocity(component), 1.0f - PMS.properties.drag);
        pmsGetPositionReference(component) = Vector3_Add(pmsGetPositionReference(component), Vector3_Scale(pmsGetVelocity(component), deltaTime));
    }
}

void Physics_ResolveCollisions()
{
    for (RJGlobal_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (RJGlobal_Size firstComponent = 0; firstComponent < PMS.data.count; firstComponent++)
        {
            if (!pmsIsActive(firstComponent))
            {
                continue;
            }

            for (RJGlobal_Size secondComponent = firstComponent + 1; secondComponent < PMS.data.count; secondComponent++)
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

PhysicsComponent Physics_ComponentCreate(RJGlobal_Index entity, Vector3 colliderSize, float mass, bool isStatic)
{
    PhysicsComponent newComponent = RJGLOBAL_INDEX_INVALID;

    newComponent = PMS.data.freeIndices.count != 0 ? *((RJGlobal_Index *)ListArray_Pop(&PMS.data.freeIndices)) : PMS.data.count;

    pmsGetEntity(newComponent) = entity;
    pmsGetColliderSize(newComponent) = colliderSize;
    pmsGetMass(newComponent) = mass;
    pmsSetStatic(newComponent, isStatic);

    PMS.data.count++;

    return newComponent;
}

void Physics_ComponentDestroy(PhysicsComponent component)
{
    pmsAssertComponent(component);

    pmsGetEntity(component) = RJGLOBAL_INDEX_INVALID;

    pmsGetVelocity(component) = Vector3_Zero;
    pmsGetColliderSize(component) = Vector3_Zero;
    pmsGetMass(component) = 0.0f;
    pmsGetFlag(component) = false;

    ListArray_Add(&PMS.data.freeIndices, &component);

    PMS.data.count--;
}

Vector3 Physics_ComponentGetVelocity(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsGetVelocity(component);
}

void Physics_ComponentSetVelocity(PhysicsComponent component, Vector3 newVelocity)
{
    pmsAssertComponent(component);
    pmsGetVelocity(component) = newVelocity;
}

Vector3 Physics_ComponentGetColliderSize(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsGetColliderSize(component);
}

void Physics_ComponentSetColliderSize(PhysicsComponent component, Vector3 newColliderSize)
{
    pmsAssertComponent(component);
    pmsGetColliderSize(component) = newColliderSize;
}

float Physics_ComponentGetMass(PhysicsComponent component)
{
    pmsAssertComponent(component);
    return pmsGetMass(component);
}

void Physics_ComponentSetMass(PhysicsComponent component, float newMass)
{
    pmsAssertComponent(component);
    pmsGetMass(component) = newMass;
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
