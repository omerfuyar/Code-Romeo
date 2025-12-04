#include "systems/Physics.h"

#include "utilities/Maths.h"
#include "utilities/ListArray.h"

#define PHYSICS_FLAG_STATIC (1 << 0)

#pragma region Source Only

struct PHYSICS_MAIN_SCENE
{
    float drag;
    float gravity;
    float elasticity;

    RJGlobal_Size capacity;
    RJGlobal_Size count;
    ListArray freeIndices; // RJGlobal_Index

    RJGlobal_Index *entities;

    Vector3 *velocities;
    Vector3 *colliderSizes;
    float *masses;
    uint8_t *flags;

    Vector3 *positionReferences;
} PMS = {0};

#define pmsGetEntity(component) (PMS.entities[component])

#define pmsGetVelocity(component) (PMS.velocities[component])
#define pmsGetColliderSize(component) (PMS.colliderSizes[component])
#define pmsGetMass(component) (PMS.masses[component])
#define pmsGetFlag(component) (PMS.flags[component])

#define pmsGetPositionReference(component) (PMS.positionReferences[pmsGetEntity(component)])

#define pmsIsStatic(component) (PMS.flags[component] & PHYSICS_FLAG_STATIC)
#define pmsSetStatic(component, isStatic) (pmsGetFlag(component) = isStatic ? (pmsGetFlag(component) | PHYSICS_FLAG_STATIC) : (pmsGetFlag(component) & ~PHYSICS_FLAG_STATIC))

#define pmsAssertComponent(component) RJGlobal_DebugAssert(component < PMS.capacity, "Physics component %u exceeds maximum possible index %u.", component, PMS.count + PMS.freeIndices.count)

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

        pmsGetVelocity(dynamicComponent).x *= -PMS.elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        pmsGetPositionReference(dynamicComponent).y +=
            pmsGetPositionReference(dynamicComponent).y < pmsGetPositionReference(staticComponent).y
                ? -overlap.y
                : overlap.y;

        pmsGetVelocity(dynamicComponent).y *= -PMS.elasticity;
    }
    else
    {
        pmsGetPositionReference(dynamicComponent).z +=
            pmsGetPositionReference(dynamicComponent).z < pmsGetPositionReference(staticComponent).z
                ? -overlap.z
                : overlap.z;

        pmsGetVelocity(dynamicComponent).z *= -PMS.elasticity;
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
    float onePlusElasticity = 1.0f + PMS.elasticity;

    pmsGetVelocity(firstComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(firstComponent),
                              pmsGetMass(firstComponent) - PMS.elasticity * pmsGetMass(secondComponent)),
                Vector3_Scale(
                    pmsGetVelocity(secondComponent),
                    onePlusElasticity * pmsGetMass(secondComponent))),
            oneOverMassSum);

    pmsGetVelocity(secondComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(secondComponent),
                              pmsGetMass(secondComponent) - PMS.elasticity * pmsGetMass(firstComponent)),
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

    PMS.drag = drag;
    PMS.gravity = gravity;
    PMS.elasticity = elasticity;

    PMS.capacity = componentCapacity;
    PMS.count = 0;
    PMS.freeIndices = ListArray_Create("RJGlobal_Index", sizeof(RJGlobal_Index), PHYSICS_INITIAL_FREE_INDEX_ARRAY_SIZE);

    PMS.entities = (RJGlobal_Index *)malloc(sizeof(RJGlobal_Index) * PMS.capacity);

    PMS.velocities = (Vector3 *)malloc(sizeof(Vector3) * PMS.capacity);
    PMS.colliderSizes = (Vector3 *)malloc(sizeof(Vector3) * PMS.capacity);
    PMS.masses = (float *)malloc(sizeof(float) * PMS.capacity);
    PMS.flags = (uint8_t *)malloc(sizeof(uint8_t) * PMS.capacity / 8 + 1);

    PMS.positionReferences = positionReferences;

    RJGlobal_MemorySet(PMS.entities, sizeof(RJGlobal_Index) * PMS.capacity, 0);
    RJGlobal_MemorySet(PMS.velocities, sizeof(Vector3) * PMS.capacity, 0);
    RJGlobal_MemorySet(PMS.colliderSizes, sizeof(Vector3) * PMS.capacity, 0);
    RJGlobal_MemorySet(PMS.masses, sizeof(float) * PMS.capacity, 0);
    RJGlobal_MemorySet(PMS.flags, sizeof(uint8_t) * PMS.capacity / 8 + 1, 0);

    RJGlobal_DebugInfo("Physics initialized with capacity %u.", PMS.capacity);
}

void Physics_Terminate()
{
    free(PMS.entities);
    free(PMS.velocities);
    free(PMS.colliderSizes);
    free(PMS.masses);
    free(PMS.flags);

    RJGlobal_DebugInfo("Physics terminated successfully.");
}

void Physics_ConfigurePositionReferences(Vector3 *positionReferences, RJGlobal_Size limiting)
{
    RJGlobal_DebugAssertNullPointerCheck(positionReferences);
    RJGlobal_DebugAssert(limiting > PMS.count, "Limiting must be greater than current physics component count.");

    PMS.positionReferences = positionReferences;
    PMS.capacity = limiting;

    PMS.entities = (RJGlobal_Index *)realloc(PMS.entities, sizeof(RJGlobal_Index) * PMS.capacity);
    PMS.velocities = (Vector3 *)realloc(PMS.velocities, sizeof(Vector3) * PMS.capacity);
    PMS.colliderSizes = (Vector3 *)realloc(PMS.colliderSizes, sizeof(Vector3) * PMS.capacity);
    PMS.masses = (float *)realloc(PMS.masses, sizeof(float) * PMS.capacity);
    PMS.flags = (uint8_t *)realloc(PMS.flags, sizeof(uint8_t) * PMS.capacity / 8 + 1);

    RJGlobal_DebugInfo("Physics position references reconfigured with new limiting %u.", PMS.capacity);
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
    for (RJGlobal_Size component = 0; component < PMS.count; component++)
    {
        if (pmsIsStatic(component))
        {
            return;
        }

        pmsGetVelocity(component) = Vector3_Add(pmsGetVelocity(component), Vector3_New(0.0f, PMS.gravity * deltaTime, 0.0f));
        pmsGetVelocity(component) = Vector3_Scale(pmsGetVelocity(component), 1.0f - PMS.drag);
        pmsGetPositionReference(component) = Vector3_Add(pmsGetPositionReference(component), Vector3_Scale(pmsGetVelocity(component), deltaTime));
    }
}

void Physics_ResolveCollisions()
{
    for (RJGlobal_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (RJGlobal_Size firstComponent = 0; firstComponent < PMS.count; firstComponent++)
        {
            for (RJGlobal_Size secondComponent = firstComponent + 1; secondComponent < PMS.count; secondComponent++)
            {
                PhysicsScene_ResolveCollision(firstComponent, secondComponent);
            }
        }
    }
}

PhysicsComponent Physics_ComponentCreate(RJGlobal_Size entity, Vector3 colliderSize, float mass, bool isStatic)
{
    PhysicsComponent newComponent = RJGLOBAL_INDEX_INVALID;

    newComponent = PMS.freeIndices.count != 0 ? *((RJGlobal_Index *)ListArray_Pop(&PMS.freeIndices)) : PMS.count;

    pmsGetEntity(newComponent) = entity;
    pmsGetColliderSize(newComponent) = colliderSize;
    pmsGetMass(newComponent) = mass;
    pmsSetStatic(newComponent, isStatic);

    PMS.count++;

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

    ListArray_Add(&PMS.freeIndices, &component);

    PMS.count--;
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
