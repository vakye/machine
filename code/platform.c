
// NOTE(vak): Platform-related definitions and implementation

#pragma once

// ----------------------------------------------------------
// NOTE(vak): Interface
// ----------------------------------------------------------

// NOTE(vak): Graphics

local void MakeWindow(string Title, u32 Width, u32 Height);
local void DeleteWindow(void);
local b32 IsWindowClosed(void);
local u32x2 GetWindowSize(void);
local void SetWindowVisible(b32 Visible);

local u32 GetRefreshRate(void);

local void* LoadVulkan(void); // NOTE(vak): Returns vkGetInstanceProcAddr()
local void UnloadVulkan(void);

// NOTE(vak): Input

local void PollEvents(void);

// NOTE(vak): Timing

local usize GetWallClock(void);
local f32 GetSecondsElapsed(usize From, usize To);
local void Wait(f32 Seconds);

// NOTE(vak): Console

local print_output GetStdOut(void);
local print_output GetStdErr(void);

// NOTE(vak): Process

local void Exit(u8 ExitCode);

// ----------------------------------------------------------
// NOTE(vak): Implementation
// ----------------------------------------------------------

#if PlatformWindows
#  include "win32.c"
#elif PlatformLinux
#  error No available platform implementation for Linux
#elif PlatformMacOS
#  error No available platform implementation for MacOS
#else
#  error Unknown Platform
#endif
