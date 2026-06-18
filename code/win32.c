
// NOTE(vak): Windows implementation of platform-related
// functionality.

#pragma once

#include <windows.h>

// ----------------------------------------------------------
// NOTE(vak): State
// ----------------------------------------------------------

typedef struct
{
    b32 IsWindowOpen;
    HWND Window;

    HMODULE VulkanDLL;
} win32_state;

local win32_state Win32 = {0};

// ----------------------------------------------------------
// NOTE(vak): Entry Point
// ----------------------------------------------------------

local void EntryPoint(void);

void WinMainCRTStartup(void)
{
    HANDLE StdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if ((StdOut == 0) || (StdOut == INVALID_HANDLE_VALUE))
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
            AllocConsole();
    }

    EntryPoint();

    Exit(0);
}

// ----------------------------------------------------------
// NOTE(vak): Implementation
// ----------------------------------------------------------

// NOTE(vak): Helpers

local WCHAR* Win32TempWideString(string UTF8)
{
    persist WCHAR Buffer[16384] = {0};

    AlwaysAssert(UTF8.Size <= S32Max);

    usize Count = MultiByteToWideChar(
        CP_UTF8, MB_PRECOMPOSED,
        UTF8.Data, (s32)UTF8.Size,
        Buffer, ArrayCount(Buffer)
    );

    if (Count < ArrayCount(Buffer))
        Buffer[Count] = 0;
    else
        Buffer[Count - 1] = 0;

    return (Buffer);
}

// NOTE(vak): Graphics

local LRESULT CALLBACK Win32WindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_QUIT:
        case WM_CLOSE:
        {
            Win32.IsWindowOpen = false;
        } break;

        default:
        {
            Result = DefWindowProcW(Window, Message, WParam, LParam);
        } break;
    }

    return (Result);
}

local void MakeWindow(string Title, u32 Width, u32 Height)
{
    WNDCLASSEXW WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .hInstance = GetModuleHandleA(0),
        .hIcon = LoadIcon(0, IDI_APPLICATION),
        .hCursor = LoadCursor(0, IDC_ARROW),
        .lpfnWndProc = &Win32WindowCallback,
        .lpszClassName = L"MachineWindowClass",
    };

    AlwaysAssert(RegisterClassExW(&WindowClass));

    RECT WindowRect = {.right = Width, .bottom = Height};

    AdjustWindowRectEx(
        &WindowRect,
        WS_OVERLAPPEDWINDOW & ~WS_OVERLAPPED,
        FALSE,
        WS_EX_APPWINDOW
    );

    Win32.Window = CreateWindowExW(
        WS_EX_APPWINDOW,
        WindowClass.lpszClassName,
        Win32TempWideString(Title),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        0, 0, WindowClass.hInstance, 0
    );

    AlwaysAssert(Win32.Window);

    Win32.IsWindowOpen = true;
}

local void DeleteWindow(void)
{
    if (Win32.Window)
    {
        DestroyWindow(Win32.Window);
        UnregisterClassW(L"MachineWindowClass", GetModuleHandleA(0));

        Win32.IsWindowOpen = false;
    }
}

local b32 IsWindowClosed(void)
{
    b32 Result = !Win32.IsWindowOpen;
    return (Result);
}

local void SetWindowVisible(b32 Visible)
{
    if (Visible)
        ShowWindow(Win32.Window, SW_SHOWDEFAULT);
    else
        ShowWindow(Win32.Window, SW_HIDE);
}

local u32 GetRefreshRate(void)
{
    u32 Result = 0;

    DEVMODEA DisplayMode = {0};
    if (EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &DisplayMode))
    {
        Result = DisplayMode.dmDisplayFrequency;
    }

    if (!Result)
        Result = 60;

    return (Result);
}

local void* LoadVulkan(void)
{
    void* Result = {0};

    Win32.VulkanDLL = LoadLibraryA("vulkan-1.dll");

    if (Win32.VulkanDLL)
    {
        Result = (void*)GetProcAddress(Win32.VulkanDLL, "vkGetInstanceProcAddr");
    }

    return (Result);
}

local void UnloadVulkan(void)
{
    if (Win32.VulkanDLL)
        FreeLibrary(Win32.VulkanDLL);
}

// NOTE(vak): Input

local void PollEvents(void)
{
    MSG Message;
    while (PeekMessageW(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        DispatchMessageW(&Message);
    }
}

// NOTE(vak): Timing

local usize GetWallClock(void)
{
    LARGE_INTEGER Counter = {0};
    QueryPerformanceCounter(&Counter);

    usize Result = Counter.QuadPart;
    return (Result);
}

local f32 GetSecondsElapsed(usize From, usize To)
{
    LARGE_INTEGER Frequency = {0};
    QueryPerformanceFrequency(&Frequency);

    usize CounterElapsed = To - From;
    f32 SecondsElapsed = (f32)CounterElapsed / (f32)Frequency.QuadPart;

    return (SecondsElapsed);
}

local void Wait(f32 Seconds)
{
    if (Seconds <= 0.0)
        return;

    LARGE_INTEGER Frequency = {0};
    QueryPerformanceFrequency(&Frequency);

    usize CountsToWait = (ssize)(Frequency.QuadPart * Seconds);

    timeBeginPeriod(1);
    {
        usize WaitBegin = GetWallClock();

        f32 MillisecondBias = 0.5f;
        DWORD Milliseconds = (DWORD)Maximum(0, Seconds * 1000.0f - MillisecondBias);

        if (Milliseconds)
            Sleep(Milliseconds);

        usize Elapsed = GetWallClock() - WaitBegin;

        while (Elapsed < CountsToWait)
            Elapsed = GetWallClock() - WaitBegin;
    }
    timeEndPeriod(1);
}

// NOTE(vak): Console

local usize Win32WriteStdOut(void* UserData, void* Bytes, usize Size)
{
    usize TotalBytes = 0;

    HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if ((Handle != 0) && (Handle != INVALID_HANDLE_VALUE))
    {
        u8* Source = Bytes;

        while (TotalBytes < Size)
        {
            usize Remaining = Size - TotalBytes;

            DWORD BytesToWrite = (DWORD) Minimum(Remaining, U32Max);
            DWORD BytesWritten = 0;

            WriteFile(Handle, Source, BytesToWrite, &BytesWritten, 0);

            if (BytesWritten == 0)
                break;

            TotalBytes += BytesWritten;
            Source += BytesWritten;
        }
    }

    return (TotalBytes);
}

local usize Win32WriteStdErr(void* UserData, void* Bytes, usize Size)
{
    usize TotalBytes = 0;

    HANDLE Handle = GetStdHandle(STD_ERROR_HANDLE);

    if ((Handle != 0) && (Handle != INVALID_HANDLE_VALUE))
    {
        u8* Source = Bytes;

        while (TotalBytes < Size)
        {
            usize Remaining = Size - TotalBytes;

            DWORD BytesToWrite = (DWORD) Minimum(Remaining, U32Max);
            DWORD BytesWritten = 0;

            WriteFile(Handle, Source, BytesToWrite, &BytesWritten, 0);

            if (BytesWritten == 0)
                break;

            TotalBytes += BytesWritten;
            Source += BytesWritten;
        }
    }

    return (TotalBytes);
}

local print_output GetStdOut(void)
{
    print_output Result =
    {
        .WriteBytes = &Win32WriteStdOut,
    };

    return (Result);
}

local print_output GetStdErr(void)
{
    print_output Result =
    {
        .WriteBytes = &Win32WriteStdErr,
    };

    return (Result);
}

// NOTE(vak): Process

local void Exit(u8 ExitCode)
{
    ExitProcess(ExitCode);
}

// NOTE(vak): Required by linker

const int _fltused = 0x9875;
