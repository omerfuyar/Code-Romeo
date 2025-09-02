#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define TEST_OBJ_COUNT_X 16
#define TEST_OBJ_COUNT_Y 16
#define TEST_BENCH_TIME_SECONDS 10.0f
#define TEST_BENCH_FULL_SCREEN false

int main()
{
    // todo input system
    Resource *vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders" PATH_DELIMETER_STR "vertex.glsl"));
    Resource *fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders" PATH_DELIMETER_STR "fragment.glsl"));
    // Resource *maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models" PATH_DELIMETER_STR "maxwell.obj"));
    // Resource *pistolSource = Resource_Create(scl("Pistol"), scl("models" PATH_DELIMETER_STR "Pistol.obj"));
    Resource *tankSource = Resource_Create(scl("Tank"), scl("models" PATH_DELIMETER_STR "tank" PATH_DELIMETER_STR "Tiger_I.obj"));

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(720, 540),
                           vertexShaderResource->data,
                           fragmentShaderResource->data,
                           false,
                           TEST_BENCH_FULL_SCREEN);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    RendererCamera *mainCamera = RendererCamera_Create(scl("Main Camera"), myScene);
    RendererObjectTransform_SetPosition(&mainCamera->transform, NewVector3(-0.25f, 0, 0));

    RendererModel *tankModel = RendererModel_CreateOBJ(scl("tiger tank"), tankSource->data, scl("models" PATH_DELIMETER_STR "tank" PATH_DELIMETER_STR), RendererObjectTransformDefault);
    RendererBatch *tankBatch = RendererBatch_Create(scl("tank batch"), myScene, tankModel, 10);
    RendererObject *tank1 = RendererObject_Create(scl("myTank"), tankBatch);

    while (true)
    {
        Renderer_StartRendering();

        RendererObjectTransform_AddRotation(&tank1->transform, NewVector3(0.0f, RENDERER_DELTA_TIME * 10, 0.0f));
        RendererObject_Update(tank1);

        RendererCamera_Update(mainCamera);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering();
    }

    return 0;
}
