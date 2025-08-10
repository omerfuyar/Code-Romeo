#include "Global.h"
#include "app/Renderer.h"
#include "app/Resources.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main()
{
    Resource vertexShaderResource = Resource_Create(scl("Vertex Shader"), scl("shaders/vertex.glsl"));
    Resource fragmentShaderResource = Resource_Create(scl("Fragment Shader"), scl("shaders/fragment.glsl"));
    Resource maxwellResource = Resource_Create(scl("Maxwell the Cat"), scl("models/maxwell.obj"));

    Renderer_Initialize(scl("Juliette"), NewVector2Int(720, 540), vertexShaderResource.data, fragmentShaderResource.data);

    RendererMesh myMesh = RendererMesh_Create(maxwellResource.data);

    RendererBatch myBatch = RendererBatch_Create(scl("My Batch"), 1000, 1000);

    RendererStaticObject myObj = RendererStaticObject_Create(scl("myObj"), &myBatch, myMesh);
    DebugAssertNullPointerCheck(&myObj);

    while (true)
    {
        Renderer_StartRendering();

        Renderer_RenderBatch(myBatch);

        Renderer_FinishRendering();
    }

    return 0;
}
