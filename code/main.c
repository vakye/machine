
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

        persist draw_rect Rects[32 * 32] = {0};

        for (usize Y = 0; Y < 32; Y++)
        {
            for (usize X = 0; X < 32; X++)
            {
                draw_rect* Rect = Rects + (Y * 32 + X);

                Rect->MinX = -0.5f + (X / 32.0f);
                Rect->MinY = -0.5f + (Y / 32.0f);
                Rect->MaxX = Rect->MinX + (1.0f / 32.0f);
                Rect->MaxY = Rect->MinY + (1.0f / 32.0f);

                Rect->R = X / 32.0f;
                Rect->G = Y / 32.0f;
                Rect->B = 0.0f;
                Rect->A = 1.0f;
            }
        }

        Render(Rects, ArrayCount(Rects));

        f32 SecondsElapsed = GetSecondsElapsed(FrameBegin, GetWallClock());
        Wait(TargetSeconds - SecondsElapsed);

        FrameBegin = GetWallClock();
    }

    DeleteRenderer();
    DeleteWindow();
}
