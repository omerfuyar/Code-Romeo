#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main()
{
    Resource_Initialize();

    // todo make resources pointers/handles
    Resource vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders\\vertex.glsl"));
    Resource fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders\\fragment.glsl"));
    Resource maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models\\maxwell.obj"));

    Renderer_CreateContext(scl("Juliette"),
                           NewVector2Int(720, 540),
                           vertexShaderResource.data,
                           fragmentShaderResource.data,
                           false);

    RendererScene *myScene = RendererScene_Create(scl("My Scene"), 5);

    RendererCamera *mainCamera = RendererCamera_Create(scl("Main Camera"), myScene);

    RendererMesh *myMesh = RendererMesh_CreateOBJ(maxwellResource.data);

    RendererBatch *myBatch = RendererBatch_Create(scl("myBatch"), myScene, myMesh, 10);

    RendererObject *myObj = RendererObject_Create(scl("myObj"), myBatch);

    while (true)
    {
        Renderer_StartRendering();

        RendererObject_Update(myObj);
        RendererObjectTransform_SetRotation(&mainCamera->transform, Vector3_Add(mainCamera->transform.rotation, NewVector3(1, 1, 1)));
        RendererCamera_Update(mainCamera);
        Renderer_RenderScene(myScene);

        Renderer_FinishRendering();
    }

    return 0;
}
