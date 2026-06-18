
#include "shared.c"
#include "print.c"
#include "platform.c"
#include "render.c"

void EntryPoint(void)
{
    MakeWindow(Str("Machine"), 1280, 720);
    MakeRenderer(RendererAPI_Vulkan);

    SetWindowVisible(true);

    u32 RefreshRate = GetRefreshRate();
    f32 TargetSeconds = 1.0f / RefreshRate;
    usize FrameBegin = GetWallClock();

    while (!IsWindowClosed())
    {
        PollEvents();

        BeginRendering();
        EndRendering();

        f32 SecondsElapsed = GetSecondsElapsed(FrameBegin, GetWallClock());
        Wait(TargetSeconds - SecondsElapsed);

        FrameBegin = GetWallClock();
    }

    DeleteRenderer();
    DeleteWindow();
}
