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
    // Resource maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models\\maxwell.obj"));

    Renderer_Initialize(scl("Juliette"),
                        NewVector2Int(720, 540),
                        vertexShaderResource.data,
                        fragmentShaderResource.data,
                        true);

    RendererCamera mainCamera = RendererCamera_Create(scl("Main Camera"));
    RendererCamera_Configure(&mainCamera, true, 90);

    // RendererMesh myMesh = RendererMesh_CreateOBJ(maxwellResource.data);

    // RendererScene myScene = RendererScene_Create(scl("My Scene"), &mainCamera, 10, 1000, 1000);

    // RendererObject myObj = RendererObject_Create(scl("myObj"), &myScene, myMesh);

    while (true)
    {
        Renderer_StartRendering();

        // RendererObject_Update(&myObj);
        // RendererCamera_Update(&mainCamera);
        // RendererScene_Update(&myScene);
        // Renderer_RenderScene(&myScene);

        Renderer_FinishRendering();
    }

    return 0;
}
