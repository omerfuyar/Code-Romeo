#include "systems/Physics.h"

#include "utilities/Maths.h"

#define PHYSICS_FLAG_STATIC (1 << 0)

#pragma region Source Only

struct PHYSICS_SCENE
{
    bool initialized;

    float drag;
    float gravity;
    float elasticity;

    RJGlobal_Size capacity;
    RJGlobal_Size count;
    RJGlobal_Size firstFree;

    Vector3 *velocities;
    Vector3 *colliderSizes;
    float *masses;
    RJGlobal_Index *positionIndices;
    uint8_t *flags;

    Vector3 *positionReferences;
} PHYSICS_MAIN_SCENE = {0};

#define pmsGetVelocity(component) (PHYSICS_MAIN_SCENE.velocities[component])
#define pmsGetColliderSize(component) (PHYSICS_MAIN_SCENE.colliderSizes[component])
#define pmsGetMass(component) (PHYSICS_MAIN_SCENE.masses[component])
#define pmsGetPositionIndex(component) (PHYSICS_MAIN_SCENE.positionIndices[component])
#define pmsGetPositionReference(component) (PHYSICS_MAIN_SCENE.positionReferences[pmsGetPositionIndex(component)])
#define pmsGetFlag(component) (PHYSICS_MAIN_SCENE.flags[component])
#define pmsIsStatic(component) (PHYSICS_MAIN_SCENE.flags[component] & PHYSICS_FLAG_STATIC)

/// @brief
/// @param staticComponent
/// @param dynamicComponent
/// @param overlap
void PhysicsScene_ResolveStaticVsDynamic(PhysicsComponent staticComponent, PhysicsComponent dynamicComponent, Vector3 overlap)
{
    Vector3 dynamicComponentPosition = pmsGetPositionReference(dynamicComponent);
    Vector3 staticComponentPosition = pmsGetPositionReference(staticComponent);

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        pmsGetPositionReference(dynamicComponent).x += dynamicComponentPosition.x < staticComponentPosition.x ? overlap.x : -overlap.x;

        pmsGetVelocity(dynamicComponent).x = -pmsGetVelocity(dynamicComponent).x * PHYSICS_MAIN_SCENE.elasticity;
    }
    else if (overlap.y < overlap.z)
    {

        pmsGetPositionReference(dynamicComponent).y += dynamicComponentPosition.y < staticComponentPosition.y ? overlap.y : -overlap.y;

        pmsGetVelocity(dynamicComponent).y = -pmsGetVelocity(dynamicComponent).y * PHYSICS_MAIN_SCENE.elasticity;
    }
    else
    {
        pmsGetPositionReference(dynamicComponent).z += dynamicComponentPosition.z < staticComponentPosition.z ? overlap.z : -overlap.z;

        pmsGetVelocity(dynamicComponent).z = -pmsGetVelocity(dynamicComponent).z * PHYSICS_MAIN_SCENE.elasticity;
    }
}

/// @brief
/// @param firstComponent
/// @param secondComponent
/// @param overlap
void PhysicsScene_ResolveDynamicVsDynamic(PhysicsComponent firstComponent, PhysicsComponent secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / pmsGetMass(firstComponent) + 1.0f / pmsGetMass(secondComponent);
    float move1 = 0.0f;
    float move2 = 0.0f;

    Vector3 firstComponentPosition = pmsGetPositionReference(firstComponent);
    Vector3 secondComponentPosition = pmsGetPositionReference(secondComponent);

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / pmsGetMass(firstComponent)) / totalInvMass * overlap.x;
        move2 = (1.0f / pmsGetMass(secondComponent)) / totalInvMass * overlap.x;

        if (firstComponentPosition.x < secondComponentPosition.x)
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

        if (firstComponentPosition.y < secondComponentPosition.y)
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

        if (firstComponentPosition.z < secondComponentPosition.z)
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
    float onePlusElasticity = 1.0f + PHYSICS_MAIN_SCENE.elasticity;

    pmsGetVelocity(firstComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(firstComponent),
                              pmsGetMass(firstComponent) - PHYSICS_MAIN_SCENE.elasticity * pmsGetMass(secondComponent)),
                Vector3_Scale(
                    pmsGetVelocity(secondComponent),
                    onePlusElasticity * pmsGetMass(secondComponent))),
            oneOverMassSum);

    pmsGetVelocity(secondComponent) =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(pmsGetVelocity(secondComponent),
                              pmsGetMass(secondComponent) - PHYSICS_MAIN_SCENE.elasticity * pmsGetMass(firstComponent)),
                Vector3_Scale(
                    tempVelocity1,
                    onePlusElasticity * pmsGetMass(firstComponent))),
            oneOverMassSum);
}

/// @brief
/// @param firstComponent
/// @param secondComponent
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

    PHYSICS_MAIN_SCENE.drag = drag;
    PHYSICS_MAIN_SCENE.gravity = gravity;
    PHYSICS_MAIN_SCENE.elasticity = elasticity;

    PHYSICS_MAIN_SCENE.capacity = componentCapacity;
    PHYSICS_MAIN_SCENE.count = 0;
    PHYSICS_MAIN_SCENE.firstFree = RJGLOBAL_INVALID_INDEX;

    PHYSICS_MAIN_SCENE.velocities = (Vector3 *)malloc(sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.colliderSizes = (Vector3 *)malloc(sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.masses = (float *)malloc(sizeof(float) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.positionIndices = (RJGlobal_Index *)malloc(sizeof(RJGlobal_Index) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.flags = (uint8_t *)malloc(sizeof(uint8_t) * PHYSICS_MAIN_SCENE.capacity / 8);
    PHYSICS_MAIN_SCENE.positionReferences = positionReferences;

    RJGlobal_MemorySet(PHYSICS_MAIN_SCENE.velocities, sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity, 0);
    RJGlobal_MemorySet(PHYSICS_MAIN_SCENE.colliderSizes, sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity, 0);
    RJGlobal_MemorySet(PHYSICS_MAIN_SCENE.masses, sizeof(float) * PHYSICS_MAIN_SCENE.capacity, 0);
    RJGlobal_MemorySet(PHYSICS_MAIN_SCENE.positionIndices, sizeof(RJGlobal_Index) * PHYSICS_MAIN_SCENE.capacity, 0);
    RJGlobal_MemorySet(PHYSICS_MAIN_SCENE.flags, sizeof(uint8_t) * PHYSICS_MAIN_SCENE.capacity / 8, 0);

    RJGlobal_DebugInfo("Physics initialized with capacity %u.", PHYSICS_MAIN_SCENE.capacity);
}

void Physics_Terminate()
{
    free(PHYSICS_MAIN_SCENE.velocities);
    free(PHYSICS_MAIN_SCENE.colliderSizes);
    free(PHYSICS_MAIN_SCENE.masses);
    free(PHYSICS_MAIN_SCENE.positionIndices);
    free(PHYSICS_MAIN_SCENE.flags);

    RJGlobal_DebugInfo("Physics terminated.");
}

void Physics_ConfigurePositionReferences(Vector3 *positionReferences, RJGlobal_Size limiting)
{
    RJGlobal_DebugAssertNullPointerCheck(positionReferences);
    RJGlobal_DebugAssert(limiting > PHYSICS_MAIN_SCENE.count, "Limiting must be greater than current physics component count.");

    PHYSICS_MAIN_SCENE.positionReferences = positionReferences;
    PHYSICS_MAIN_SCENE.capacity = limiting;

    PHYSICS_MAIN_SCENE.velocities = (Vector3 *)realloc(PHYSICS_MAIN_SCENE.velocities, sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.colliderSizes = (Vector3 *)realloc(PHYSICS_MAIN_SCENE.colliderSizes, sizeof(Vector3) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.masses = (float *)realloc(PHYSICS_MAIN_SCENE.masses, sizeof(float) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.positionIndices = (RJGlobal_Index *)realloc(PHYSICS_MAIN_SCENE.positionIndices, sizeof(RJGlobal_Index) * PHYSICS_MAIN_SCENE.capacity);
    PHYSICS_MAIN_SCENE.flags = (uint8_t *)realloc(PHYSICS_MAIN_SCENE.flags, sizeof(uint8_t) * PHYSICS_MAIN_SCENE.capacity / 8);

    RJGlobal_DebugInfo("Physics position references reconfigured with new limiting %u.", PHYSICS_MAIN_SCENE.capacity);
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
    for (RJGlobal_Size component = 0; component < PHYSICS_MAIN_SCENE.count; component++)
    {
        Physics_ComponentUpdate(component, deltaTime);
    }
}

void Physics_ResolveCollisions()
{
    for (RJGlobal_Size iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (RJGlobal_Size firstComponent = 0; firstComponent < PHYSICS_MAIN_SCENE.count; firstComponent++)
        {
            for (RJGlobal_Size secondComponent = firstComponent + 1; secondComponent < PHYSICS_MAIN_SCENE.count; secondComponent++)
            {
                PhysicsScene_ResolveCollision(firstComponent, secondComponent);
            }
        }
    }
}

PhysicsComponent Physics_ComponentCreate(RJGlobal_Size positionIndex, Vector3 colliderSize, float mass, bool isStatic)
{
    RJGlobal_DebugAssert(positionIndex < PHYSICS_MAIN_SCENE.capacity, "Position index %u exceeds physics capacity %u.", positionIndex, PHYSICS_MAIN_SCENE.capacity);

    PhysicsComponent newComponent = PHYSICS_MAIN_SCENE.count;

    pmsGetPositionIndex(newComponent) = positionIndex;
    pmsGetColliderSize(newComponent) = colliderSize;
    pmsGetMass(newComponent) = mass;
    pmsGetFlag(newComponent) = isStatic ? (pmsGetFlag(newComponent) | PHYSICS_FLAG_STATIC) : (pmsGetFlag(newComponent) & ~PHYSICS_FLAG_STATIC);

    PHYSICS_MAIN_SCENE.count++;

    return newComponent;
}

void Physics_ComponentDestroy(PhysicsComponent component)
{
    (void)component;
}

void Physics_ComponentUpdate(PhysicsComponent component, float deltaTime)
{
    if (pmsIsStatic(component))
    {
        return;
    }

    pmsGetVelocity(component) = Vector3_Add(pmsGetVelocity(component), Vector3_New(0.0f, PHYSICS_MAIN_SCENE.gravity * deltaTime, 0.0f));
    pmsGetVelocity(component) = Vector3_Scale(pmsGetVelocity(component), 1.0f - PHYSICS_MAIN_SCENE.drag);
    pmsGetPositionReference(component) = Vector3_Add(pmsGetPositionReference(component), Vector3_Scale(pmsGetVelocity(component), deltaTime));
}

// todo add border checks

Vector3 Physics_ComponentGetVelocity(PhysicsComponent component)
{
    return pmsGetVelocity(component);
}

void Physics_ComponentSetVelocity(PhysicsComponent component, Vector3 newVelocity)
{
    pmsGetVelocity(component) = newVelocity;
}

Vector3 Physics_ComponentGetColliderSize(PhysicsComponent component)
{
    return pmsGetColliderSize(component);
}

void Physics_ComponentSetColliderSize(PhysicsComponent component, Vector3 newColliderSize)
{
    pmsGetColliderSize(component) = newColliderSize;
}

float Physics_ComponentGetMass(PhysicsComponent component)
{
    return pmsGetMass(component);
}

void Physics_ComponentSetMass(PhysicsComponent component, float newMass)
{
    pmsGetMass(component) = newMass;
}

bool Physics_ComponentIsStatic(PhysicsComponent component)
{
    return pmsIsStatic(component);
}

void Physics_ComponentSetStatic(PhysicsComponent component, bool newIsStatic)
{
    pmsGetFlag(component) = newIsStatic ? (pmsGetFlag(component) | PHYSICS_FLAG_STATIC) : (pmsGetFlag(component) & ~PHYSICS_FLAG_STATIC);
}

#pragma endregion Physics Component
