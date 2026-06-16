
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

typedef void delete_renderer(void);
typedef void begin_rendering(void);
typedef void end_rendering(void);

typedef struct
{
    renderer_api     CurrentAPI;

    delete_renderer* DeleteRenderer;
    begin_rendering* BeginRendering;
    end_rendering*   EndRendering;
} renderer_functions;

local renderer_functions RendererFunctions = {0};

#define DeleteRenderer(...) RendererFunctions.DeleteRenderer(__VA_ARGS__)
#define BeginRendering(...) RendererFunctions.BeginRendering(__VA_ARGS__)
#define EndRendering(...)   RendererFunctions.EndRendering(__VA_ARGS__)

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
