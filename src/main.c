#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

#define TEST_BENCH_TIME_SECONDS 10.0f
#define TEST_OBJECT_ONE_SIDE 32
#define TEST_VSYNC false
#define TEST_FULL_SCREEN true
#define TEST_BENCHMARK true

typedef struct myObjectType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    RendererRenderable *renderable;
} myObjectType;

myObjectType objects[TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE];

typedef struct myCameraType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    RendererCamera *camera;
} myCameraType;

int main(int argc, char *argv[])
{
    // todo input system
    Resource *vertexShaderResource = Resource_Create(scl("vertex.glsl"), scl("shaders" PATH_DELIMETER_STR));
    Resource *fragmentShaderResource = Resource_Create(scl("fragment.glsl"), scl("shaders" PATH_DELIMETER_STR));
    Resource *objResource = argc == 1 ? Resource_Create(scl("Pistol.obj"), scl("models" PATH_DELIMETER_STR)) : Resource_Create(scl(argv[1]), scl("models" PATH_DELIMETER_STR));

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(1080, 720),
                           vertexShaderResource->data,
                           fragmentShaderResource->data,
                           TEST_VSYNC,
                           TEST_FULL_SCREEN);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    myCameraType mainCamera = {
        .name = scl("Main Camera"),
        .position = NewVector3(-4.0f * Max((float)log2((double)TEST_OBJECT_ONE_SIDE), 1.0f), 0.0f, 0.0f),
        .rotation = NewVector3(0.0f, 0.0f, 0.0f),
        .camera = RendererCamera_Create(myScene)};

    RendererModel *objModel = RendererModel_CreateOBJ(scl("Object Model"), objResource->data, objResource->lineCount, objResource->path, NewVector3N(0.0f), NewVector3N(0.0f), NewVector3N(1.0f));
    RendererBatch *objBatch = RendererScene_CreateBatch(myScene, scl("object Batch"), objModel, TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE);

    for (int x = 0; x < TEST_OBJECT_ONE_SIDE; x++)
    {
        for (int y = 0; y < TEST_OBJECT_ONE_SIDE; y++)
        {
            objects[x * TEST_OBJECT_ONE_SIDE + y] = (myObjectType){
                .name = scl("Gun"),
                .position = NewVector3(0.0f, (float)y - (float)(TEST_OBJECT_ONE_SIDE / 2), (float)x - (float)(TEST_OBJECT_ONE_SIDE / 2)),
                .rotation = NewVector3N(0.0f),
                .scale = NewVector3N(4.0f / Max((float)log2((double)TEST_OBJECT_ONE_SIDE), 1.0f)),
                .renderable = RendererBatch_CreateRenderable(objBatch)};
        }
    }

    Timer mainLoopTimer = Timer_Create("Main Loop Timer");
    float mainLoopDeltaTime = 0.0f;

    int benchMarkFrameCount = 0;
    float benchMarkTime = 0.0f;
    while (true)
    {
        Timer_Start(&mainLoopTimer);

        // rendering
        Renderer_StartRendering();

        for (size_t i = 0; i < TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE; i++)
        {
            objects[i].rotation.y += mainLoopDeltaTime;
            // objects[i].rotation.y += (float)(rand() % 100) / 1000;
            RendererRenderable_Update(objects[i].renderable, objects[i].position, objects[i].rotation, objects[i].scale);
        }

        // myObj.rotation.y += mainLoopDeltaTime;
        // RendererRenderable_Update(myObj.renderable, myObj.position, myObj.rotation, myObj.scale);

        RendererCamera_Update(mainCamera.camera, mainCamera.position, mainCamera.rotation);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering(mainLoopDeltaTime);

        Timer_Stop(&mainLoopTimer);
        mainLoopDeltaTime = (float)Timer_GetElapsedNanoseconds(mainLoopTimer) / 1000000000.0f;

        benchMarkFrameCount++;
        benchMarkTime += mainLoopDeltaTime;
        if (benchMarkTime > TEST_BENCH_TIME_SECONDS)
        {
            DebugError("Time : %f seconds | Objects : %d | Vertices : %d | Average FPS : %f | Full Screen : %s", benchMarkTime, objBatch->objectMatrices.count, objModel->vertices.count * objBatch->objectMatrices.count, (float)benchMarkFrameCount / TEST_BENCH_TIME_SECONDS, TEST_FULL_SCREEN ? "true" : "false");
        }
    }

    return EXIT_FAILURE;
}
