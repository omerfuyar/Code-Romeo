#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

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
    Resource *gunSource = Resource_Create(scl("Gun"), scl("models" PATH_DELIMETER_STR "Pistol.obj"));

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(720, 540),
                           vertexShaderResource->data,
                           fragmentShaderResource->data,
                           false,
                           TEST_BENCH_FULL_SCREEN);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    RendererCamera *mainCamera = RendererCamera_Create(scl("Main Camera"), myScene);
    RendererObjectTransform_SetPosition(&mainCamera->transform, NewVector3(-1.0f, 0, 0));

    RendererModel *gunModel = RendererModel_CreateOBJ(scl("sig gun"), gunSource->data, scl("models" PATH_DELIMETER_STR), (RendererTransform){NewVector3(0.0f, 0.0f, 0.0f), NewVector3(0.0f, 0.0f, 0.0f), NewVector3(0.1f, 0.1f, 0.1f)});
    RendererBatch *gunBatch = RendererBatch_Create(scl("gun batch"), myScene, gunModel, 10);
    RendererObject *gun1 = RendererObject_Create(scl("myGun"), gunBatch);

    while (true)
    {
        Renderer_StartRendering();

        RendererObjectTransform_AddRotation(&gun1->transform, NewVector3(0.0f, RENDERER_DELTA_TIME * 1, 0.0f));
        RendererObject_Update(gun1);

        RendererCamera_Update(mainCamera);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering();
    }

    return 0;
}
