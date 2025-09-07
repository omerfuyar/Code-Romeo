#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"
#include "utilities/Timer.h"

#define TEST_OBJECT_ONE_SIDE 4
#define TEST_VSYNC false
#define TEST_FULL_SCREEN false

typedef struct myObjectType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    RendererRenderable *renderable;
} myObjectType;

typedef struct myCameraType
{
    String name;
    Vector3 position;
    Vector3 rotation;
    RendererCamera *camera;
} myCameraType;

int main(int argc, char **argv)
{
    // todo input system
    Resource *vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders" PATH_DELIMETER_STR "vertex.glsl"));
    Resource *fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders" PATH_DELIMETER_STR "fragment.glsl"));

    Resource *objResource = NULL;
    if (argc == 1)
    {
        objResource = Resource_Create(scl("Object"), scl("models" PATH_DELIMETER_STR "Pistol.obj"));
    }
    else
    {
        String modelPath = String_CreateCopy("models" PATH_DELIMETER_STR);
        String_ConcatEnd(&modelPath, scl(argv[1]));
        objResource = Resource_Create(scl("Object"), modelPath);
        String_Destroy(&modelPath);
    }

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(720, 540),
                           vertexShaderResource->data,
                           fragmentShaderResource->data,
                           TEST_VSYNC,
                           TEST_FULL_SCREEN);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    myCameraType mainCamera = {
        .name = scl("Main Camera"),
        .position = NewVector3(-3.0f, 0.0f, 0.0f),
        .rotation = NewVector3(0.0f, 0.0f, 0.0f),
        .camera = RendererCamera_Create(myScene)};

    RendererModel *objModel = RendererModel_CreateOBJ(scl("Object Model"), objResource->data, objResource->lineCount, scl("models" PATH_DELIMETER_STR), NewVector3(0.0f, 0.0f, 0.0f), NewVector3(0.0f, 0.0f, 0.0f), NewVector3(1.0f, 1.0f, 1.0f));
    RendererBatch *objBatch = RendererScene_CreateBatch(myScene, scl("object Batch"), objModel, TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE);

    // ListArray objectList = ListArray_Create("My Object Type", sizeof(myObjectType), TEST_OBJECT_ONE_SIDE * TEST_OBJECT_ONE_SIDE);

    // for (int x = 0; x < TEST_OBJECT_ONE_SIDE; x++)
    //{
    //     for (int y = 0; y < TEST_OBJECT_ONE_SIDE; y++)
    //     {
    //         myObjectType newObject;
    //         newObject.name = scl("Gun");
    //         newObject.position = NewVector3(0.0f, (float)y - (float)(TEST_OBJECT_ONE_SIDE / 2), (float)x - (float)(TEST_OBJECT_ONE_SIDE / 2));
    //         newObject.rotation = NewVector3(0.0f, 0.0f, 0.0f);
    //         newObject.scale = NewVector3(0.1f, 0.1f, 0.1f);
    //         newObject.renderable = RendererBatch_CreateRenderable(gunBatch);
    //         RendererRenderable_Update(newObject.renderable, newObject.position, newObject.rotation, newObject.scale);
    //
    //        ListArray_Add(&objectList, &newObject);
    //    }
    //}

    myObjectType myObj = {
        .name = scl("Gun"),
        .position = NewVector3(0.0f, 0.0f, 0.0f),
        .rotation = NewVector3(0.0f, 0.0f, 0.0f),
        .scale = NewVector3(1.0f, 1.0f, 1.0f),
        .renderable = RendererBatch_CreateRenderable(objBatch)};

    Timer mainLoopTimer = Timer_Create("Main Loop Timer");
    float mainLoopDeltaTime = 0.0f;
    while (true)
    {
        Timer_Start(&mainLoopTimer);

        // rendering
        Renderer_StartRendering();

        // for (size_t i = 0; i < objectList.count; i++)
        //{
        //     myObjectType myGun = *(myObjectType *)ListArray_Get(objectList, i);
        //     myGun.rotation.y += mainLoopDeltaTime * 1000;
        //     RendererRenderable_Update(myGun.renderable, myGun.position, myGun.rotation, myGun.scale);
        // }

        myObj.rotation.y += mainLoopDeltaTime;
        RendererRenderable_Update(myObj.renderable, myObj.position, myObj.rotation, myObj.scale);

        RendererCamera_Update(mainCamera.camera, mainCamera.position, mainCamera.rotation);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering(mainLoopDeltaTime);

        Timer_Stop(&mainLoopTimer);
        mainLoopDeltaTime = (float)Timer_GetElapsedNanoseconds(mainLoopTimer) / 1000000000.0f;
    }

    return 1;
}
