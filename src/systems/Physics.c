#include "systems/Physics.h"

#include "utilities/Maths.h"

#pragma region Source Only

/// @brief
/// @param scene
/// @param staticComponent
/// @param dynamicComponent
/// @param overlap
void PhysicsScene_ResolveStaticVsDynamic(const PhysicsScene *scene, PhysicsComponent *staticComponent, PhysicsComponent *dynamicComponent, Vector3 overlap)
{
    float move = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move = overlap.x;

        if (dynamicComponent->positionReference->x < staticComponent->positionReference->x)
        {
            dynamicComponent->positionReference->x -= move;
        }
        else
        {
            dynamicComponent->positionReference->x += move;
        }

        dynamicComponent->velocity.x = -dynamicComponent->velocity.x * scene->elasticity;
    }
    else if (overlap.y < overlap.z)
    {
        move = overlap.y;

        if (dynamicComponent->positionReference->y < staticComponent->positionReference->y)
        {
            dynamicComponent->positionReference->y -= move;
        }
        else
        {
            dynamicComponent->positionReference->y += move;
        }

        dynamicComponent->velocity.y = -dynamicComponent->velocity.y * scene->elasticity;
    }
    else
    {
        move = overlap.z;

        if (dynamicComponent->positionReference->z < staticComponent->positionReference->z)
        {
            dynamicComponent->positionReference->z -= move;
        }
        else
        {
            dynamicComponent->positionReference->z += move;
        }

        dynamicComponent->velocity.z = -dynamicComponent->velocity.z * scene->elasticity;
    }
}

/// @brief
/// @param scene
/// @param firstComponent
/// @param secondComponent
/// @param overlap
void PhysicsScene_ResolveDynamicVsDynamic(const PhysicsScene *scene, PhysicsComponent *firstComponent, PhysicsComponent *secondComponent, Vector3 overlap)
{
    float totalInvMass = 1.0f / firstComponent->mass + 1.0f / secondComponent->mass;
    float move1 = 0.0f;
    float move2 = 0.0f;

    if (overlap.x < overlap.y && overlap.x < overlap.z)
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.x;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.x;

        if (firstComponent->positionReference->x < secondComponent->positionReference->x)
        {
            firstComponent->positionReference->x -= move1;
            secondComponent->positionReference->x += move2;
        }
        else
        {
            firstComponent->positionReference->x += move1;
            secondComponent->positionReference->x -= move2;
        }
    }
    else if (overlap.y < overlap.z)
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.y;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.y;

        if (firstComponent->positionReference->y < secondComponent->positionReference->y)
        {
            firstComponent->positionReference->y -= move1;
            secondComponent->positionReference->y += move2;
        }
        else
        {
            firstComponent->positionReference->y += move1;
            secondComponent->positionReference->y -= move2;
        }
    }
    else
    {
        move1 = (1.0f / firstComponent->mass) / totalInvMass * overlap.z;
        move2 = (1.0f / secondComponent->mass) / totalInvMass * overlap.z;

        if (firstComponent->positionReference->z < secondComponent->positionReference->z)
        {
            firstComponent->positionReference->z -= move1;
            secondComponent->positionReference->z += move2;
        }
        else
        {
            firstComponent->positionReference->z += move1;
            secondComponent->positionReference->z -= move2;
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

/// @brief
/// @param scene
/// @param firstComponent
/// @param secondComponent
void PhysicsScene_ResolveCollision(const PhysicsScene *scene, PhysicsComponent *firstComponent, PhysicsComponent *secondComponent)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    Vector3 overlap;
    if (!Physics_IsColliding(firstComponent, secondComponent, &overlap))
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

bool Physics_IsColliding(const PhysicsComponent *component1, const PhysicsComponent *component2, Vector3 *overlapRet)
{
    RJGlobal_DebugAssertNullPointerCheck(component1);
    RJGlobal_DebugAssertNullPointerCheck(component2);

    Vector3 position1 = *component1->positionReference;
    Vector3 position2 = *component2->positionReference;

    float overlapX = Maths_Min(position1.x + component1->colliderSize.x / 2.0f,
                               position2.x + component2->colliderSize.x / 2.0f) -
                     Maths_Max(position1.x - component1->colliderSize.x / 2.0f,
                               position2.x - component2->colliderSize.x / 2.0f);

    float overlapY = Maths_Min(position1.y + component1->colliderSize.y / 2.0f,
                               position2.y + component2->colliderSize.y / 2.0f) -
                     Maths_Max(position1.y - component1->colliderSize.y / 2.0f,
                               position2.y - component2->colliderSize.y / 2.0f);

    float overlapZ = Maths_Min(position1.z + component1->colliderSize.z / 2.0f,
                               position2.z + component2->colliderSize.z / 2.0f) -
                     Maths_Max(position1.z - component1->colliderSize.z / 2.0f,
                               position2.z - component2->colliderSize.z / 2.0f);

    if (overlapRet != NULL)
    {
        *overlapRet = Vector3_New(overlapX, overlapY, overlapZ);
    }

    return overlapX > 0.0f && overlapY > 0.0f && overlapZ > 0.0f;
}

#pragma endregion Physics

#pragma region Physics Scene

PhysicsScene *PhysicsScene_Create(StringView name, size_t initialColliderCapacity, float drag, float gravity, float elasticity)
{
    PhysicsScene *scene = (PhysicsScene *)malloc(sizeof(PhysicsScene));
    RJGlobal_DebugAssertNullPointerCheck(scene);

    scene->name = scc(name);
    scene->components = ListArray_Create("Physics Object", sizeof(PhysicsComponent), initialColliderCapacity);
    scene->drag = Maths_Clamp(drag, 0.0f, 1.0f);
    scene->elasticity = Maths_Clamp(elasticity, 0.0f, 1.0f);
    scene->gravity = gravity;

    RJGlobal_DebugInfo("Physics Scene '%s' created.", scene->name.characters);
    return scene;
}

void PhysicsScene_Destroy(PhysicsScene *scene)
{
    RJGlobal_DebugAssertNullPointerCheck(scene);

    char tempTitle[RJGLOBAL_TEMP_BUFFER_SIZE];
    RJGlobal_MemoryCopy(tempTitle, Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, scene->name.length), scene->name.characters);
    tempTitle[Maths_Min(RJGLOBAL_TEMP_BUFFER_SIZE - 1, scene->name.length)] = '\0';

    for (size_t i = scene->components.count - 1; i < scene->components.count; i--)
    {
        PhysicsComponent *component = (PhysicsComponent *)ListArray_Get(&scene->components, i);
        PhysicsScene_DestroyComponent(component);
    }

    ListArray_Destroy(&scene->components);
    String_Destroy(&scene->name);

    RJGlobal_DebugInfo("Physics Scene '%s' destroyed.", tempTitle);
}

void PhysicsScene_UpdateComponents(const PhysicsScene *scene, float deltaTime)
{
    for (size_t i = 0; i < scene->components.count; i++)
    {
        PhysicsComponent *component = (PhysicsComponent *)ListArray_Get(&scene->components, i);
        PhysicsComponent_Update(component, deltaTime);
    }
}

void PhysicsScene_ResolveCollisions(const PhysicsScene *scene)
{
    for (size_t iteration = 0; iteration < PHYSICS_COLLISION_RESOLVE_ITERATIONS; iteration++)
    {
        for (size_t i = 0; i < scene->components.count - 1; i++)
        {
            PhysicsComponent *firstComponent = (PhysicsComponent *)ListArray_Get(&scene->components, i);

            // if (firstComponent->isStatic)
            //{
            //     continue;
            // }

            for (size_t j = i + 1; j < scene->components.count; j++)
            {
                PhysicsComponent *secondComponent = (PhysicsComponent *)ListArray_Get(&scene->components, j);

                // if (secondComponent->isStatic)
                //{
                //     continue;
                // }

                PhysicsScene_ResolveCollision(scene, firstComponent, secondComponent);
            }
        }
    }
}

PhysicsComponent *PhysicsScene_CreateComponent(PhysicsScene *scene, Vector3 *positionReference, Vector3 colliderSize, float mass, bool isStatic)
{
    PhysicsComponent component = {0};

    component.scene = scene;
    component.colliderSize = colliderSize;
    component.isStatic = isStatic;
    component.mass = mass;
    component.positionReference = positionReference;
    component.componentOffsetInScene = scene->components.count;

    // RJGlobal_DebugInfo("Physics Component created in Scene '%s'.", scene->name.characters);

    return (PhysicsComponent *)ListArray_Add(&scene->components, &component);
}

void PhysicsScene_DestroyComponent(PhysicsComponent *component)
{
    RJGlobal_DebugAssertNullPointerCheck(component);

    // todo ! be may be wrong
    for (size_t i = component->componentOffsetInScene + 1; i < component->scene->components.count - component->componentOffsetInScene; i++)
    {
        PhysicsComponent *nextComponent = (PhysicsComponent *)ListArray_Get(&component->scene->components, i);
        nextComponent->componentOffsetInScene--;
    }

    ListArray_RemoveAtIndex(&component->scene->components, component->componentOffsetInScene);
}

#pragma endregion Physics Scene

#pragma region Physics Component

void PhysicsComponent_Update(PhysicsComponent *component, float deltaTime)
{
    if (component->isStatic)
    {
        return;
    }

    component->velocity = Vector3_Add(component->velocity, Vector3_New(0.0f, component->scene->gravity * deltaTime, 0.0f));
    component->velocity = Vector3_Scale(component->velocity, 1.0f - component->scene->drag);
    *component->positionReference = Vector3_Add(*component->positionReference, Vector3_Scale(component->velocity, deltaTime));
}

void PhysicsComponent_Configure(PhysicsComponent *component, Vector3 newColliderSize, float newMass, bool newIsStatic)
{
    RJGlobal_DebugAssertNullPointerCheck(component);

    component->colliderSize = newColliderSize;
    component->mass = newMass;
    component->isStatic = newIsStatic;
}

#pragma endregion Physics Component
