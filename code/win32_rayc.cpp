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

struct game_bitmap
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

struct win32_dib
{
    BITMAPINFO Info;
    game_bitmap Bitmap;
};

global_variable win32_dib GlobalWin32DIB;
global_variable bool32 GlobalRunning;

internal void
Win32ResizeDIBSection(win32_dib *Win32DIB, int Width, int Height)
{
    game_bitmap *Bitmap = &Win32DIB->Bitmap;
    BITMAPINFO *Info = &Win32DIB->Info;
    
    if (Bitmap->Data)
    {
        VirtualFree(Bitmap->Data, 0, MEM_RELEASE);
    }

    Bitmap->Width = Width;
    Bitmap->Height = Height;
    Bitmap->BytesPerPixel = 4;
    Bitmap->Pitch = Bitmap->Width * Bitmap->BytesPerPixel;

    Info->bmiHeader.biSize = sizeof(Info->bmiHeader);
    Info->bmiHeader.biWidth = Bitmap->Width;
    Info->bmiHeader.biHeight = - Bitmap->Height;
    Info->bmiHeader.biPlanes = 1;
    Info->bmiHeader.biBitCount = 32;
    Info->bmiHeader.biCompression = BI_RGB;

    int BitmapSize = (Bitmap->Width * Bitmap->Height) * Bitmap->BytesPerPixel;
    Bitmap->Data = VirtualAlloc(0, BitmapSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32RenderDIB(win32_dib *Win32DIB, HDC DeviceContext)
{
    StretchDIBits(DeviceContext,
                  0, 0, Win32DIB->Bitmap.Width, Win32DIB->Bitmap.Height,
                  0, 0, Win32DIB->Bitmap.Width, Win32DIB->Bitmap.Height,
                  Win32DIB->Bitmap.Data,
                  &Win32DIB->Info,
                  DIB_RGB_COLORS, SRCCOPY);
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);;
            int WindowWidth = ClientRect.right - ClientRect.left;
            int WindowHeight = ClientRect.bottom - ClientRect.top;

            Win32RenderDIB(&GlobalWin32DIB, DeviceContext);
           
            EndPaint(Window, &Paint);
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
    Win32ResizeDIBSection(&GlobalWin32DIB, 1600, 900);
    
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
        WindowRect.right = 1600;
        WindowRect.bottom = 900;
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
            game_input InputData = {};
            game_state GameState = {};
            
            GlobalRunning = true;
            while (GlobalRunning)
            {
                Win32ProcessPendingMessage(&InputData);

                //GameUpdateAndRender(&InputData, &GlobalWin32DIB.Bitmap);

                game_bitmap *Bitmap = &GlobalWin32DIB.Bitmap;

                u8 *Row = (u8 *)Bitmap->Data;
                for (int Y = 0;
                     Y < Bitmap->Height;
                     ++Y)
                {
                    u32 *Pixel = (u32 *)Row;
                    for (int X = 0;
                         X < Bitmap->Width;
                         ++X)
                    {
                        u8 Blue = (u8)(X + GameState.BlueOffset);
                        u8 Green = (u8)(Y + GameState.GreenOffset);

                        *Pixel++ = ((Green << 8) | Blue);
                    }

                    Row += Bitmap->Pitch;
                }

                ++GameState.BlueOffset;
                ++GameState.GreenOffset;
                
                HDC DeviceContext = GetDC(Window);
                Win32RenderDIB(&GlobalWin32DIB, DeviceContext);
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
