#include "Global.h"

#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "tools/Input.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"

#define TEST_BENCH_TIME_SECONDS 10.0f
#define TEST_OBJECT_ONE_SIDE 1
#define TEST_WINDOW_SIZE NewVector2Int(1080, 720)
#define TEST_VSYNC false
#define TEST_FULL_SCREEN false
#define TEST_BENCHMARK false

typedef struct myObjectType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    RendererComponent *renderable;
} myObjectType;

myObjectType objects[TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE];

typedef struct myCameraType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    RendererCameraComponent *camera;
    float rotationSpeed;
    float speed;
} myCameraType;

void mainTerminate(int exitCode, char *message)
{
    (void)exitCode;
    (void)message;

    Renderer_Terminate();
    Context_Terminate();
}

int main(int argc, char **argv)
{
    Global_SetTerminateCallback(mainTerminate);
    srand((unsigned int)time(NULL));

    Resource *vertexShaderResource = Resource_Create(scl("vertex.glsl"), scl("shaders" PATH_DELIMETER_STR));
    Resource *fragmentShaderResource = Resource_Create(scl("fragment.glsl"), scl("shaders" PATH_DELIMETER_STR));

    Resource *objResource = argc == 1 ? Resource_Create(scl("Pistol.obj"), scl("models" PATH_DELIMETER_STR)) : Resource_Create(scl(argv[1]), scl("models" PATH_DELIMETER_STR));

    ContextWindow *mainWindow = Context_Initialize();
    Context_Configure(scl("Juliette"), TEST_WINDOW_SIZE, TEST_VSYNC, TEST_FULL_SCREEN, NULL);

    Input_Initialize(mainWindow);
    Renderer_Initialize(mainWindow);
    Renderer_ConfigureShaders(vertexShaderResource->data, fragmentShaderResource->data);

    RendererScene *myRendererScene = RendererScene_Create(scl("My Scene"), TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE);
    RendererModel *objModel = RendererModel_CreateOBJ(scl("Object Model"), objResource->data, objResource->lineCount, objResource->path, NewVector3N(0.0f), NewVector3N(0.0f), NewVector3N(1.0f));
    RendererBatch *objBatch = RendererScene_CreateBatch(myRendererScene, scl("Object Batch"), objModel, TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE);

    myCameraType mainCamera = {
        .name = scl("Main Camera"),
        .position = Vector3_Zero,
        .rotation = Vector3_Zero,
        .camera = RendererCameraComponent_Create(&mainCamera.position, &mainCamera.rotation),
        .speed = 10.0f,
        .rotationSpeed = 75.0f};
    RendererCameraComponent_Configure(mainCamera.camera, true, 90.0f, 0.1f, 1000.0f);
    RendererScene_SetMainCamera(myRendererScene, mainCamera.camera);

    for (int x = 0; x < TEST_OBJECT_ONE_SIDE; x++)
    {
        for (int y = 0; y < TEST_OBJECT_ONE_SIDE; y++)
        {
            myObjectType *obj = &objects[x * TEST_OBJECT_ONE_SIDE + y];
            obj->name = scl("Gun");
            obj->position = NewVector3(0.0f, (float)y - (float)(TEST_OBJECT_ONE_SIDE / 2), (float)x - (float)(TEST_OBJECT_ONE_SIDE / 2));
            obj->rotation = NewVector3N(0.0f);
            obj->scale = NewVector3N(6.0f / Max((float)log2((double)TEST_OBJECT_ONE_SIDE), 4.0f));
            obj->renderable = RendererBatch_CreateComponent(objBatch, &obj->position, &obj->rotation, &obj->scale);
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

        for (size_t i = 0; i < TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE; i++)
        {
            objects[i].rotation.y += DT / 2.0f;
        }

        if (Input_GetKey(InputKeyCode_F, InputState_Down))
        {
            Context_ConfigureFullScreen(!mainWindow->fullScreen);
        }

        mainCamera.camera->size -= Input_GetMouseButtonScroll();

        if (Input_GetMouseButton(InputMouseButtonCode_Left, InputState_Pressed))
        {
            Input_ConfigureMouseMode(InputMouseMode_Captured);

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
        }
        else
        {
            Input_ConfigureMouseMode(InputMouseMode_Normal);

            Vector3 movementVector = Input_GetMovementVector();

            objects[0].position.x += movementVector.x * DT * mainCamera.speed;
            objects[0].position.z -= movementVector.y * DT * mainCamera.speed;
        }

        // user check collisions
        RendererScene_Update(myRendererScene);

        // rendering
        Renderer_StartRendering();
        Renderer_RenderScene(myRendererScene);

        Renderer_FinishRendering();

        Timer_Stop(&DTimer);
        DT = (float)Timer_GetElapsedNanoseconds(DTimer) / 1000000000.0f;

        char titleBuffer[TEMP_BUFFER_SIZE];
        snprintf(titleBuffer, sizeof(titleBuffer), "%s | FPS: %f | Frame Time: %f ms", "Juliette", 1.0f / DT, DT * 1000);
        Context_ConfigureTitle(scl(titleBuffer));

        if (TEST_BENCHMARK)
        {
            benchMarkFrameCount++;
            benchMarkTime += DT;

            if (benchMarkTime > TEST_BENCH_TIME_SECONDS)
            {
                DebugError(
                    "Time : %f seconds | Objects : %d | Vertices : %d | Average FPS : %f | Full Screen : %s",
                    benchMarkTime,
                    objBatch->objectMatrices.count,
                    objModel->vertices.count * objBatch->objectMatrices.count,
                    (float)benchMarkFrameCount / TEST_BENCH_TIME_SECONDS,
                    TEST_FULL_SCREEN ? "true" : "false");
            }
        }
    }

    return EXIT_FAILURE;
}
