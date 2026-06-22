
#include "shared.c"
#include "math.c"
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

        u32x2 WindowSize = GetWindowSize();

        draw_rect_2d Rects[] =
        {
            {
                100.0f, 100.0f,
                200.0f, 200.0f,
                1.0f,  0.8f, 0.5f, 1.0f,
            },
        };

        draw_command_2d Command =
        {
            .Rects = Rects,
            .RectCount = ArrayCount(Rects),
            .Projection = OrthographicProjection(
                F32x2(0.0f, (f32)WindowSize.Y),
                F32x2((f32)WindowSize.X, 0.0f)
            ),
        };

        Render2D(&Command);

        f32 SecondsElapsed = GetSecondsElapsed(FrameBegin, GetWallClock());
        Wait(TargetSeconds - SecondsElapsed);

        FrameBegin = GetWallClock();
    }

    DeleteRenderer();
    DeleteWindow();
}
