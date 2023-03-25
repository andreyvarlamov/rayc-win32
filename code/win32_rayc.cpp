#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int32_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef float f64;

typedef i32 bool32;

#define internal static
#define local_persist static
#define global_variable static

#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}

struct game_offscreen_buffer
{
    void *Data;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_input
{
    bool32 Up;
    bool32 Down;
    bool32 Left;
    bool32 Right;
};

struct game_state
{
    int GreenOffset;
    int BlueOffset;
};

#include <windows.h>

global_variable bool32 GlobalRunning;

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

        HWND Window = CreateWindowExA(0,
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

        if (Window)
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
            
            game_input InputData = {};
            game_state GameState = {};
            
            GlobalRunning = true;
            while (GlobalRunning)
            {
                Win32ProcessPendingMessage(&InputData);

                u8 *Row = (u8 *)GameBuffer.Data;
                for (int Y = 0;
                     Y < GameBuffer.Height;
                     ++Y)
                {
                    u32 *Pixel = (u32 *)Row;
                    for (int X = 0;
                         X < GameBuffer.Width;
                         ++X)
                    {
                        u8 Blue = (u8)(X + GameState.BlueOffset);
                        u8 Green = (u8)(Y + GameState.GreenOffset);

                        *Pixel++ = ((Green << 8) | Blue);
                    }

                    Row += GameBuffer.Pitch;
                }

                ++GameState.BlueOffset;
                ++GameState.GreenOffset;
                
                HDC DeviceContext = GetDC(Window);
                StretchDIBits(DeviceContext,
                              0, 0, GameBuffer.Width, GameBuffer.Height,
                              0, 0, GameBuffer.Width, GameBuffer.Height,
                              GameBuffer.Data,
                              &BitmapInfo,
                              DIB_RGB_COLORS, SRCCOPY);
                ReleaseDC(Window, DeviceContext);
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
