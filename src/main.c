#include "Global.h"

#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "tools/Input.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"

#define TEST_BENCH_TIME_SECONDS 10.0f
#define TEST_OBJECT_ONE_SIDE 1
#define TEST_VSYNC false
#define TEST_FULL_SCREEN false
#define TEST_BENCHMARK false

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
    float rotationSpeed;
    float speed;
} myCameraType;

int main(int argc, char **argv)
{
    Resource *vertexShaderResource = Resource_Create(scl("vertex.glsl"), scl("shaders" PATH_DELIMETER_STR));
    Resource *fragmentShaderResource = Resource_Create(scl("fragment.glsl"), scl("shaders" PATH_DELIMETER_STR));
    Resource *objResource = argc == 1 ? Resource_Create(scl("Pistol.obj"), scl("models" PATH_DELIMETER_STR)) : Resource_Create(scl(argv[1]), scl("models" PATH_DELIMETER_STR));

    void *window = Renderer_CreateContext(scl("Juliette"),
                                          NewVector2Int(1080, 720),
                                          vertexShaderResource->data,
                                          fragmentShaderResource->data,
                                          TEST_VSYNC,
                                          TEST_FULL_SCREEN);
    Input_Initialize(window);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    myCameraType mainCamera = {
        .name = scl("Main Camera"),
        .position = NewVector3(-5.0f * Max((float)log2((double)TEST_OBJECT_ONE_SIDE), 1.0f), 0.0f, 0.0f),
        .rotation = NewVector3(0.0f, 0.0f, 0.0f),
        .camera = RendererCamera_Create(myScene),
        .speed = 10.0f,
        .rotationSpeed = 75.0f};

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
                .scale = NewVector3N(4.0f / Max((float)log2((double)TEST_OBJECT_ONE_SIDE), 4.0f)),
                .renderable = RendererBatch_CreateRenderable(objBatch)};
        }
    }

    Timer DTimer = Timer_Create("Main Loop Timer");
    float DT = 0.0f;

    int benchMarkFrameCount = 0;
    float benchMarkTime = 0.0f;
    while (true)
    {
        Timer_Start(&DTimer);

        Input_Update();

        if (Input_GetKey(InputKeyCode_Escape, InputState_Down))
        {
            DebugInfo("Escape key pressed, exiting...");
            break;
        }

        // rendering
        Renderer_StartRendering();

        for (size_t i = 0; i < TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE; i++)
        {
            objects[i].rotation.y += DT;
            // objects[i].rotation.y += (float)(rand() % 100) / 1000;
            RendererRenderable_Update(objects[i].renderable, objects[i].position, objects[i].rotation, objects[i].scale);
        }

        Vector2Int mousePositionDelta = Input_GetMousePositionDelta();
        Vector3 movementVector = Input_GetMovementVector();

        mainCamera.rotation.y += (float)mousePositionDelta.x * mainCamera.rotationSpeed * DT;
        mainCamera.rotation.x -= (float)mousePositionDelta.y * mainCamera.rotationSpeed * DT;
        mainCamera.rotation.x = Clamp(mainCamera.rotation.x, -89.0f, 89.0f);

        Vector3 direction = Vector3_Normalized(NewVector3(
            Cos(mainCamera.rotation.x) * Cos(mainCamera.rotation.y),
            Sin(mainCamera.rotation.x),
            Cos(mainCamera.rotation.x) * Sin(mainCamera.rotation.y)));

        Vector3 right = Vector3_Normalized(Vector3_Cross(direction, Vector3_Up));

        Vector3 move = Vector3_Scale(direction, movementVector.y);
        move = Vector3_Add(move, Vector3_Scale(right, movementVector.x));
        move.y += movementVector.z;

        if (Vector3_Magnitude(move) > 0.0f)
        {
            move = Vector3_Normalized(move);
            mainCamera.position = Vector3_Add(mainCamera.position, Vector3_Scale(move, mainCamera.speed * DT));
        }

        RendererCamera_Update(mainCamera.camera, mainCamera.position, mainCamera.rotation);

        Renderer_RenderScene(myScene);

        Renderer_FinishRendering(DT);

        Timer_Stop(&DTimer);
        DT = (float)Timer_GetElapsedNanoseconds(DTimer) / 1000000000.0f;

        if (TEST_BENCHMARK)
        {
            benchMarkFrameCount++;
            benchMarkTime += DT;
            if (benchMarkTime > TEST_BENCH_TIME_SECONDS)
            {
                DebugError("Time : %f seconds | Objects : %d | Vertices : %d | Average FPS : %f | Full Screen : %s", benchMarkTime, objBatch->objectMatrices.count, objModel->vertices.count * objBatch->objectMatrices.count, (float)benchMarkFrameCount / TEST_BENCH_TIME_SECONDS, TEST_FULL_SCREEN ? "true" : "false");
            }
        }
    }

    return EXIT_FAILURE;
}
