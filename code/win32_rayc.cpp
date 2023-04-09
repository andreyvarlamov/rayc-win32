#include "rayc.cpp"

#include <windows.h>
#include <windowsx.h>

global_variable bool32 GlobalRunning;
global_variable bool32 GlobalSleepIsGranular;
global_variable bool32 GlobalShouldCaptureMouse;
global_variable i64 GlobalPerfCountFrequency;
global_variable HWND GlobalWindow;
global_variable game_input GlobalGameInput;

internal void
DEBUGPrintString(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    char CharBuffer[256];
    vsprintf_s(CharBuffer, 256, Format, Args);
    va_end(Args);

    OutputDebugStringA(CharBuffer);
}

internal void
PLATFORMFreeFileMemory(void *Memory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal platform_read_file_result
PLATFORMReadEntireFile(char *Filename)
{
    platform_read_file_result Result = {0};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = SafeTruncateU64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                    (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    PLATFORMFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // TODO: Logging
            }
        }
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

inline void
Win32SetMouseCursorVisibile(bool32 Enabled)
{
    if (Enabled)
    {
        while (ShowCursor(true) < 0) {}
    }
    else
    {
        while (ShowCursor(false) >= 0) {}
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        {
            if (GlobalShouldCaptureMouse)
            {
                RECT WindowRect;
                GetWindowRect(GlobalWindow, &WindowRect);
                i32 WindowWidth = WindowRect.right - WindowRect.left;
                i32 WindowHeight = WindowRect.bottom - WindowRect.top;
                POINT MouseDefaultPosition;
                MouseDefaultPosition.x = WindowWidth/2;
                MouseDefaultPosition.y = WindowHeight/2;

                i32 MouseCurrentX = GET_X_LPARAM(LParam);
                i32 MouseCurrentY = GET_Y_LPARAM(LParam);
                
                GlobalGameInput.MouseDX = MouseCurrentX - MouseDefaultPosition.x;
                GlobalGameInput.MouseDY = MouseCurrentY - MouseDefaultPosition.y;
                if (Message == WM_MOUSEWHEEL)
                {
                    GlobalGameInput.MouseDZ = GET_WHEEL_DELTA_WPARAM(WParam);
                }

                ClientToScreen(GlobalWindow, &MouseDefaultPosition);
                SetCursorPos(MouseDefaultPosition.x, MouseDefaultPosition.y);
            }
            else
            {
                GlobalGameInput.MouseDX = 0;
                GlobalGameInput.MouseDY = 0;
                GlobalGameInput.MouseDZ = 0;
            }
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal void
Win32ProcessPendingMessage(game_input *InputData)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

                if (WasDown != IsDown)
                {
                    switch (VKCode)
                    {
                        case VK_UP:
                        {
                            InputData->Up = IsDown;
                        } break;

                        case VK_DOWN:
                        {
                            InputData->Down = IsDown;
                        } break;

                        case VK_LEFT:
                        {
                            InputData->Left = IsDown;
                        } break;

                        case VK_RIGHT:
                        {
                            InputData->Right = IsDown;
                        } break;

                        case 'W':
                        {
                            InputData->Forward = IsDown;
                        } break;

                        case 'A':
                        {
                            InputData->StrafeLeft = IsDown;
                        } break;

                        case 'S':
                        {
                            InputData->Back = IsDown;
                        } break;

                        case 'D':
                        {
                            InputData->StrafeRight = IsDown;
                        } break;

                        case VK_F4:
                        {
                            bool32 AltKeyWasDown = Message.lParam & (1 << 29);
                            if (AltKeyWasDown)
                            {
                                GlobalRunning = false;
                            }
                        } break;

                        case 'M':
                        {
                            if (IsDown)
                            {
                                GlobalShouldCaptureMouse = !GlobalShouldCaptureMouse;
                                Win32SetMouseCursorVisibile(!GlobalShouldCaptureMouse);
                            }
                        } break;
                    }
                }
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    f32 Result = ((f32)(End.QuadPart - Start.QuadPart) /
                  (f32)GlobalPerfCountFrequency);
    return Result;
}

internal void
Win32PauseUntilFrameTime(LARGE_INTEGER LastCounter, f32 SecondsElapsed, f32 TargetSeconds)
{
    if (SecondsElapsed < TargetSeconds)
    {
        if (GlobalSleepIsGranular)
        {
            i32 SleepMS_Signed = (i32)(1000.0f * (TargetSeconds - SecondsElapsed)) - 1;
            if (SleepMS_Signed > 0)
            {
                Sleep((DWORD)SleepMS_Signed);
            }

            SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            if (SecondsElapsed > TargetSeconds)
            {
                DEBUGPrintString("Missed frame - sleep.\n");
            }
        }

        while (SecondsElapsed < TargetSeconds)
        {
            // NOTE: Spin
            SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
        }
    }
    else
    {
        DEBUGPrintString("Missed frame - work.\n");
    }
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    int ClientWidth = 1600;
    int ClientHeight = 900;
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "rayc";

    if (RegisterClassA(&WindowClass))
    {
        DWORD WindowStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE);
        
        RECT WindowRect;
        WindowRect.left = 0;
        WindowRect.top = 0;
        WindowRect.right = ClientWidth;
        WindowRect.bottom = ClientHeight;
        // TODO: Log failure instead of assert
        Assert(AdjustWindowRect(&WindowRect, WindowStyle, 0));
        int WindowWidth = WindowRect.right - WindowRect.left;
        int WindowHeight = WindowRect.bottom - WindowRect.top;

        GlobalWindow = CreateWindowExA(0,
                                      WindowClass.lpszClassName,
                                      "rayc",
                                      WindowStyle,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      WindowWidth,
                                      WindowHeight,
                                      0,
                                      0,
                                      Instance,
                                      0);

        if (GlobalWindow)
        {
            game_offscreen_buffer GameBuffer = {};
            GameBuffer.Width = ClientWidth;
            GameBuffer.Height = ClientHeight;
            GameBuffer.BytesPerPixel = 4;
            GameBuffer.Pitch = GameBuffer.Width * GameBuffer.BytesPerPixel;
            int GameBufferSize = (GameBuffer.Width * GameBuffer.Height) * GameBuffer.BytesPerPixel;
            GameBuffer.Data = VirtualAlloc(0, GameBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            BITMAPINFO BitmapInfo = {};
            BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
            BitmapInfo.bmiHeader.biWidth = GameBuffer.Width;
            BitmapInfo.bmiHeader.biHeight = - GameBuffer.Height;
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
            
            game_state GameState = {};
            GameStateInit(&GameState);

            LARGE_INTEGER LastCounter = Win32GetWallClock();
            f32 SecondsElapsedForFrame = 0.0f;

            LARGE_INTEGER PerfCountFrequencyResult;
            QueryPerformanceFrequency(&PerfCountFrequencyResult);
            GlobalPerfCountFrequency = (i64)PerfCountFrequencyResult.QuadPart;

            // NOTE: Set the Windows scheduler granularity to 1 ms for Sleep().
            UINT DesiredSchedulerMS = 1;
            GlobalSleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

            f32 TargetSecondsPerFrame = 1 / 60.0f; // 60FPS
            GlobalGameInput.SecondsPerFrame = TargetSecondsPerFrame;

            Win32SetMouseCursorVisibile(!GlobalShouldCaptureMouse);

            GlobalRunning = true;
            while (GlobalRunning)
            {
                // game_input ZeroGameInput = {0};
                // GlobalGameInput = ZeroGameInput;
                GlobalGameInput.MouseDZ = 0;
                GlobalGameInput.MouseLeft = GetKeyState(VK_LBUTTON) & (1 << 15);
                GlobalGameInput.MouseRight = GetKeyState(VK_RBUTTON) & (1 << 15);
                Win32ProcessPendingMessage(&GlobalGameInput);
                
                GameUpdateAndRender(&GameState, &GlobalGameInput, &GameBuffer);

                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                f32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                Win32PauseUntilFrameTime(LastCounter, WorkSecondsElapsed, TargetSecondsPerFrame);
                LARGE_INTEGER EndCounter = Win32GetWallClock();
                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, EndCounter);
                LastCounter = EndCounter;
                
                HDC DeviceContext = GetDC(GlobalWindow);
                StretchDIBits(DeviceContext,
                              0, 0, GameBuffer.Width, GameBuffer.Height,
                              0, 0, GameBuffer.Width, GameBuffer.Height,
                              GameBuffer.Data,
                              &BitmapInfo,
                              DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(GlobalWindow, DeviceContext);

                f32 WorkPercent  = WorkSecondsElapsed / SecondsElapsedForFrame * 100.0f;
                if (WorkPercent > 0.0f)
                {
                    
                    DEBUGPrintString("Frame=%.2fms; Work=%.2fms(%.0f%%)\n",
                                     SecondsElapsedForFrame * 1000.0f,
                                     WorkSecondsElapsed * 1000.0f,
                                     WorkPercent);
                }
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }

    return 0;
}
