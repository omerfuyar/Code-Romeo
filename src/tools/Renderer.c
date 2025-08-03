#include "tools/Renderer.h"

typedef struct Object
{
    int foo;
} Object;

void Renderer_RenderObjects(Object *object)
{
    DebugAssertPointerNullCheck(object);
}
