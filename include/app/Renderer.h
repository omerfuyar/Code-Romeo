#pragma once

#include "Global.h"
#include "utilities/String.h"
#include "utilities/ListArray.h"

typedef struct Object
{
    String name;
    ListArray foo;
} Object;

void Renderer_RenderObjects(Object *object);
