#include "tools/Physics.h"
#include "utilities/Maths.h"

#pragma region Source Only

void PhysicsScene_ResolveStaticVsDynamic(PhysicsScene *scene, PhysicsComponent *staticComponent, PhysicsComponent *dynamicComponent, Vector3 overlap)
{
    float move = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move = overlap.x;

        if (dynamicComponent->position->x < staticComponent->position->x)
        {
            dynamicComponent->position->x -= move;
        }
        else
        {
            dynamicComponent->position->x += move;
        }

        dynamicComponent->velocity.x = -dynamicComponent->velocity.x * scene->elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        move = overlap.y;

        if (dynamicComponent->position->y < staticComponent->position->y)
        {
            dynamicComponent->position->y -= move;
        }
        else
        {
            dynamicComponent->position->y += move;
        }

        dynamicComponent->velocity.y = -dynamicComponent->velocity.y * scene->elasticity;
    }
    else
    {
        move = overlap.z;

        if (dynamicComponent->position->z < staticComponent->position->z)
        {
            dynamicComponent->position->z -= move;
        }
        else
        {
            dynamicComponent->position->z += move;
        }

        dynamicComponent->velocity.z = -dynamicComponent->velocity.z * scene->elasticity;
    }
}

void PhysicsScene_ResolveDynamicVsDynamic(PhysicsScene *scene, PhysicsComponent *firstComponent, PhysicsComponent *secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / firstComponent->mass + 1.0f / secondComponent->mass;
    float move1 = 0.0f;
    float move2 = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.x;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.x;

        if (firstComponent->position->x < secondComponent->position->x)
        {
            firstComponent->position->x -= move1;
            secondComponent->position->x += move2;
        }
        else
        {
            firstComponent->position->x += move1;
            secondComponent->position->x -= move2;
        }
    }
    else if (overlap.y < overlap.z)
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.y;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.y;

        if (firstComponent->position->y < secondComponent->position->y)
        {
            firstComponent->position->y -= move1;
            secondComponent->position->y += move2;
        }
        else
        {
            firstComponent->position->y += move1;
            secondComponent->position->y -= move2;
        }
    }
    else
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.z;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.z;

        if (firstComponent->position->z < secondComponent->position->z)
        {
            firstComponent->position->z -= move1;
            secondComponent->position->z += move2;
        }
        else
        {
            firstComponent->position->z += move1;
            secondComponent->position->z -= move2;
        }
    }

    // v1' = ( (m1 - e*m2)*v1 + (1+e)*m2*v2 ) / (m1+m2)
    // v2' = ( (m2 - e*m1)*v2 + (1+e)*m1*v1 ) / (m1+m2)

    Vector3 tempVelocity1 = firstComponent->velocity;
    float oneOverMassSum = 1.0f / (firstComponent->mass + secondComponent->mass);
    float onePlusElasticity = 1.0f + scene->elasticity;

    firstComponent->velocity =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(firstComponent->velocity,
                              firstComponent->mass - scene->elasticity * secondComponent->mass),
                Vector3_Scale(
                    secondComponent->velocity,
                    onePlusElasticity * secondComponent->mass)),
            oneOverMassSum);

    secondComponent->velocity =
        Vector3_Scale(
            Vector3_Add(
                Vector3_Scale(secondComponent->velocity,
                              secondComponent->mass - scene->elasticity * firstComponent->mass),
                Vector3_Scale(
                    tempVelocity1,
                    onePlusElasticity * firstComponent->mass)),
            oneOverMassSum);
}

void PhysicsScene_ResolveCollision(PhysicsScene *scene, PhysicsComponent *firstComponent, PhysicsComponent *secondComponent)
{
    DebugAssertNullPointerCheck(scene);

    Vector3 overlap;
    if (!Physics_IsColliding(firstComponent, secondComponent, &overlap) ||
        (firstComponent->isStatic && secondComponent->isStatic))
    {
        return;
    }

    if (firstComponent->isStatic)
    {
        PhysicsScene_ResolveStaticVsDynamic(scene, firstComponent, secondComponent, overlap);
    }
    else if (secondComponent->isStatic)
    {
        PhysicsScene_ResolveStaticVsDynamic(scene, secondComponent, firstComponent, overlap);
    }
    else
    {
        PhysicsScene_ResolveDynamicVsDynamic(scene, firstComponent, secondComponent, overlap);
    }
}

#pragma endregion Source Only

#pragma region Physics

bool Physics_IsColliding(PhysicsComponent *component1, PhysicsComponent *component2, Vector3 *overlapRet)
{
    DebugAssertNullPointerCheck(component1);
    DebugAssertNullPointerCheck(component2);

    float overlapX = Min(component1->position->x + component1->colliderSize.x / 2.0f,
                         component2->position->x + component2->colliderSize.x / 2.0f) -
                     Max(component1->position->x - component1->colliderSize.x / 2.0f,
                         component2->position->x - component2->colliderSize.x / 2.0f);

    float overlapY = Min(component1->position->y + component1->colliderSize.y / 2.0f,
                         component2->position->y + component2->colliderSize.y / 2.0f) -
                     Max(component1->position->y - component1->colliderSize.y / 2.0f,
                         component2->position->y - component2->colliderSize.y / 2.0f);

    float overlapZ = Min(component1->position->z + component1->colliderSize.z / 2.0f,
                         component2->position->z + component2->colliderSize.z / 2.0f) -
                     Max(component1->position->z - component1->colliderSize.z / 2.0f,
                         component2->position->z - component2->colliderSize.z / 2.0f);

    if (overlapRet != NULL)
    {
        *overlapRet = NewVector3(overlapX, overlapY, overlapZ);
    }

    return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
}

#pragma endregion Physics

#pragma region Physics Scene

PhysicsScene *PhysicsScene_Create(String name, size_t initialColliderCapacity, float drag, float gravity, float elasticity)
{
    PhysicsScene *scene = (PhysicsScene *)malloc(sizeof(PhysicsScene));
    DebugAssertNullPointerCheck(scene);

    scene->name = scc(name);
    scene->components = ListArray_Create("Physics Object", sizeof(PhysicsComponent), initialColliderCapacity);
    scene->drag = Clamp(drag, 0.0f, 1.0f);
    scene->elasticity = Clamp(elasticity, 0.0f, 1.0f);
    scene->gravity = gravity;

    DebugInfo("Physics Scene '%s' created.", scene->name.characters);
    return scene;
}

void PhysicsScene_Destroy(PhysicsScene *scene)
{
    DebugAssertNullPointerCheck(scene);

    char tempTitle[TEMP_BUFFER_SIZE];
    MemoryCopy(tempTitle, Min(TEMP_BUFFER_SIZE, scene->name.length + 1), scene->name.characters);

    String_Destroy(&scene->name);
    for (size_t i = scene->components.count - 1; i >= 0; i--)
    {
        PhysicsComponent *component = (PhysicsComponent *)ListArray_Get(scene->components, i);
        PhysicsScene_DestroyComponent(component);
    }

    ListArray_Destroy(&scene->components);

    DebugInfo("Physics Scene '%s' destroyed.", tempTitle);
}

void PhysicsScene_UpdateComponentPositions(PhysicsScene *scene, float deltaTime)
{
    for (size_t i = 0; i < scene->components.count; i++)
    {
        PhysicsComponent *component = (PhysicsComponent *)ListArray_Get(scene->components, i);

        if (component->isStatic)
        {
            continue;
        }

        component->velocity = Vector3_Add(component->velocity, NewVector3(0.0f, scene->gravity * deltaTime, 0.0f));
        component->velocity = Vector3_Scale(component->velocity, 1.0f - scene->drag);
        *component->position = Vector3_Add(*component->position, Vector3_Scale(component->velocity, deltaTime));
    }
}

void PhysicsScene_DetectAndResolveCollisions(PhysicsScene *scene)
{
    // for (size_t iter = 0; iter < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iter++)
    //{
    for (size_t i = 0; i < scene->components.count - 1; i++)
    {
        PhysicsComponent *firstComponent = (PhysicsComponent *)ListArray_Get(scene->components, i);

        for (size_t j = i + 1; j < scene->components.count; j++)
        {
            PhysicsComponent *secondComponent = (PhysicsComponent *)ListArray_Get(scene->components, j);
            PhysicsScene_ResolveCollision(scene, firstComponent, secondComponent);
        }
    }
    //}
}

PhysicsComponent *PhysicsScene_CreateComponent(PhysicsScene *scene, Vector3 *positionReference, Vector3 colliderSize, float mass, bool isStatic)
{
    PhysicsComponent component = {0};

    component.scene = scene;
    component.colliderSize = colliderSize;
    component.isStatic = isStatic;
    component.mass = mass;
    component.position = positionReference;
    component.componentOffsetInScene = scene->components.count;

    DebugInfo("Physics Component created in Scene '%s'.", scene->name.characters);

    return (PhysicsComponent *)ListArray_Add(&scene->components, &component);
}

void PhysicsScene_DestroyComponent(PhysicsComponent *component)
{
    DebugAssertNullPointerCheck(component);

    for (size_t i = component->componentOffsetInScene; i < component->scene->components.count - component->componentOffsetInScene; i++)
    {
        PhysicsComponent *nextComponent = (PhysicsComponent *)ListArray_Get(component->scene->components, i);
        nextComponent->componentOffsetInScene--;
    }

    ListArray_RemoveAtIndex(&component->scene->components, component->componentOffsetInScene);

    DebugInfo("Physics Component destroyed in Scene '%s'.", component->scene->name.characters);
}

#pragma endregion Physics Scene

#pragma region Physics Component

void PhysicsComponent_Configure(PhysicsComponent *firstComponent, Vector3 newColliderSize, float newMass, bool newIsStatic)
{
    DebugAssertNullPointerCheck(firstComponent);

    firstComponent->colliderSize = newColliderSize;
    firstComponent->mass = newMass;
    firstComponent->isStatic = newIsStatic;
}

#pragma endregion Physics Component
