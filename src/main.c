#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define TEST_OBJ_COUNT_X 16
#define TEST_OBJ_COUNT_Y 16
#define TEST_BENCH_TIME_SECONDS 10.0f
#define TEST_BENCH_FULL_SCREEN true

int main()
{
    // todo input system
    Resource *vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders" PATH_DELIMETER_STR "vertex.glsl"));
    Resource *fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders" PATH_DELIMETER_STR "fragment.glsl"));
    Resource *maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models" PATH_DELIMETER_STR "maxwell.obj"));
    Resource *pistolSource = Resource_Create(scl("Pistol"), scl("models" PATH_DELIMETER_STR "Pistol.obj"));

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(720, 540),
                           vertexShaderResource->data,
                           fragmentShaderResource->data,
                           false,
                           TEST_BENCH_FULL_SCREEN);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    RendererCamera *mainCamera = RendererCamera_Create(scl("Main Camera"), myScene);
    RendererObjectTransform_SetPosition(&mainCamera->transform, NewVector3(-0.25f, 0, 0));

    RendererMesh *gunMesh = RendererMesh_CreateOBJ(pistolSource->data);
    RendererMesh *catMesh = RendererMesh_CreateOBJ(maxwellResource->data);

    RendererBatch *gunBatch = RendererBatch_Create(scl("gunBatch"), myScene, gunMesh, TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y);
    RendererBatch *catBatch = RendererBatch_Create(scl("catBatch"), myScene, catMesh, TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y);

    RendererObject *gunObjs[TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y] = {0};
    RendererObject *catObjs[TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y] = {0};

    for (size_t row = 0; row < TEST_OBJ_COUNT_Y; row++)
    {
        for (size_t column = 0; column < TEST_OBJ_COUNT_X; column++)
        {
            size_t index = row * TEST_OBJ_COUNT_X + column;
            gunObjs[index] = RendererObject_Create(scl("gunObj"), gunBatch);
            RendererObjectTransform_MultiplyScale(&gunObjs[index]->transform, NewVector3(1.0f / TEST_OBJ_COUNT_X, 1.0f / TEST_OBJ_COUNT_Y, 1.0f / TEST_OBJ_COUNT_X));
            RendererObjectTransform_SetPosition(&gunObjs[index]->transform, NewVector3(0.25f, (TEST_OBJ_COUNT_Y / 2.0f - (float)row) / TEST_OBJ_COUNT_Y - 0.1f, (TEST_OBJ_COUNT_X / 2.0f - (float)column) / TEST_OBJ_COUNT_X - 0.1f));
        }
    }

    for (size_t row = 0; row < TEST_OBJ_COUNT_Y; row++)
    {
        for (size_t column = 0; column < TEST_OBJ_COUNT_X; column++)
        {
            size_t index = row * TEST_OBJ_COUNT_X + column;
            catObjs[index] = RendererObject_Create(scl("gunObj"), catBatch);
            RendererObjectTransform_MultiplyScale(&catObjs[index]->transform, NewVector3(0.5f / TEST_OBJ_COUNT_X, 0.5f / TEST_OBJ_COUNT_Y, 0.5f / TEST_OBJ_COUNT_X));
            RendererObjectTransform_SetPosition(&catObjs[index]->transform, NewVector3(1.0f, (TEST_OBJ_COUNT_Y / 2.0f - (float)row) / TEST_OBJ_COUNT_Y - 0.1f, (TEST_OBJ_COUNT_X / 2.0f - (float)column) / TEST_OBJ_COUNT_X - 0.1f));
        }
    }

    // RendererObject *catObj = RendererObject_Create(scl("catObj"), catBatch);
    // RendererObjectTransform_MultiplyScale(&catObj->transform, NewVector3(0.01f, 0.01f, 0.01f));
    // RendererObjectTransform_SetPosition(&catObj->transform, NewVector3(0, 0, 0.25));

    float totalTime = 0.0f;
    size_t frameCount = 0;
    size_t totalFaces = 0;
    size_t totalObjects = 0;

    totalObjects += gunBatch->objectMatrices.count;
    totalFaces += gunBatch->objectMatrices.count * gunBatch->mesh->indices.count / 3;

    totalObjects += catBatch->objectMatrices.count;
    totalFaces += catBatch->objectMatrices.count * catBatch->mesh->indices.count / 3;

    while (true)
    {
        Renderer_StartRendering();

        // RendererObjectTransform_MultiplyScale(&gunObj->transform, NewVector3(0, 1 + RENDERER_DELTA_TIME, 1 + RENDERER_DELTA_TIME));

        for (size_t i = 0; i < TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y; i++)
        {
            RendererObjectTransform_AddRotation(&gunObjs[i]->transform, NewVector3(0, RENDERER_DELTA_TIME, 0));
            RendererObject_Update(gunObjs[i]);
        }

        for (size_t i = 0; i < TEST_OBJ_COUNT_X * TEST_OBJ_COUNT_Y; i++)
        {
            RendererObjectTransform_AddRotation(&catObjs[i]->transform, NewVector3(0, RENDERER_DELTA_TIME, 0));
            RendererObject_Update(catObjs[i]);
        }

        // RendererObjectTransform_AddRotation(&catObj->transform, NewVector3(0, RENDERER_DELTA_TIME, 0));
        // RendererObject_Update(catObj);
        //  RendererObjectTransform_SetRotation(&mainCamera->transform, Vector3_Add(mainCamera->transform.rotation, NewVector3(0, RENDERER_DELTA_TIME, 0)));
        RendererCamera_Update(mainCamera);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering();

        frameCount++;
        totalTime += RENDERER_DELTA_TIME;
        if (totalTime > TEST_BENCH_TIME_SECONDS)
        {
            DebugError("Time : %f seconds | Batches : 2 | Objects : %d | Faces : %d | Average FPS : %f | Full Screen : %s", totalTime, totalObjects, totalFaces, (float)frameCount / TEST_BENCH_TIME_SECONDS, TEST_BENCH_FULL_SCREEN ? "true" : "false");
        }
    }

    return 0;
}
