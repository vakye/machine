
#include "shared.c"
#include "print.c"
#include "platform.c"
#include "render.c"

void EntryPoint(void)
{
    MakeWindow(Str("Machine"), 1280, 720);
    MakeRenderer(RendererAPI_Vulkan);

    SetWindowVisible(true);

    while (!IsWindowClosed())
    {
        PollEvents();

        BeginRendering();
        EndRendering();
    }

    DeleteRenderer();
    DeleteWindow();
}
