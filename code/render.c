
// NOTE(vak): Rendering-related definitions and implementation

#pragma once

// ----------------------------------------------------------
// NOTE(vak): Interface
// ----------------------------------------------------------

typedef enum
{
    RendererAPI_Unknown = 0,
    RendererAPI_Vulkan,
} renderer_api;

local void MakeRenderer(renderer_api API);

typedef struct
{
    f32 MinX, MinY;
    f32 MaxX, MaxY;
    f32 R, G, B, A;
} draw_rect;

typedef struct
{
    draw_rect* Rects;
    usize RectCount;
    m4x4 Projection;
} draw_command;

typedef void delete_renderer(void);
typedef void render(draw_command* Draw);

typedef struct
{
    renderer_api     CurrentAPI;

    delete_renderer* DeleteRenderer;
    render*          Render;
} renderer_functions;

local renderer_functions RendererFunctions = {0};

#define DeleteRenderer(...) RendererFunctions.DeleteRenderer(__VA_ARGS__)
#define Render(...)         RendererFunctions.Render(__VA_ARGS__)

// ----------------------------------------------------------
// NOTE(vak): Implementation
// ----------------------------------------------------------

#include "vulkan.c"

local void MakeRenderer(renderer_api API)
{
    AlwaysAssert(RendererFunctions.CurrentAPI == RendererAPI_Unknown);

    switch (API)
    {
        InvalidDefaultCase;

        case RendererAPI_Vulkan:
        {
            VulkanMakeRenderer();
        } break;
    }
}
