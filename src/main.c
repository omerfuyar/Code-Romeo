#include "Global.h"
#include "tools/Renderer.h"
#include "tools/Resources.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main()
{
    Resource_Initialize();

    Resource vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders\\vertex.glsl"));
    Resource fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders\\fragment.glsl"));
    Resource maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models\\maxwell.obj"));

    RendererCamera mainCamera = RendererCamera_Create(scl("Main Camera"), 90.0f);

    Renderer_Initialize(scl("Juliette"),
                        NewVector2Int(720, 540),
                        vertexShaderResource.data,
                        fragmentShaderResource.data,
                        &mainCamera,
                        true);

    RendererMesh myMesh = RendererMesh_CreateOBJ(maxwellResource.data);

    // RendererBatch myBatch = RendererBatch_Create(scl("My Batch"), 1000, 1000);

    RendererDynamicObject myObj = RendererDynamicObject_Create(scl("myObj"), myMesh);
    // RendererStaticObject myObj = RendererStaticObject_Create(scl("myObj"), &myBatch, myMesh);
    DebugAssertNullPointerCheck(&myObj);

    while (true)
    {
        Renderer_StartRendering();

        Renderer_RenderDynamicObject(myObj);
        // Renderer_RenderBatch(myBatch);

        Renderer_FinishRendering();
    }

    return 0;
}
