#include <stdio.h>
#include <stdint.h>
#include <math.h>

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

#define Pi32 3.14159265359f

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

    f32 PlayerX;
    f32 PlayerY;
    f32 PlayerAngle;

    u8 Map[8][8];

    // TODO: Remove
    bool32 AlreadyPrinted;
};

internal void
DEBUGPrintString(const char *Format, ...);

internal void
GameStateInit(game_state *State)
{
    State->PlayerX = 1.5f;
    State->PlayerY = 3.5f;

    u8 Map[8][8] = {
        { 1, 1, 1, 1,  1, 1, 1, 1 },
        { 1, 0, 0, 0,  0, 0, 0, 1 },
        { 1, 0, 0, 0,  0, 0, 0, 1 },
        { 1, 0, 0, 1,  1, 0, 0, 1 },

        { 1, 0, 0, 1,  1, 0, 0, 1 },
        { 1, 0, 0, 0,  0, 0, 0, 1 },
        { 1, 0, 0, 0,  0, 0, 0, 1 },
        { 1, 1, 1, 1,  1, 1, 1, 1 },
    };
}

internal i32
RoundF32ToI32(f32 Real)
{
    i32 Result = (i32)(Real + 0.5f);
    return Result;
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              f32 RealMinX, f32 RealMinY,
              f32 RealMaxX, f32 RealMaxY,
              u32 Color)
{
    i32 MinX = RoundF32ToI32(RealMinX);
    i32 MinY = RoundF32ToI32(RealMinY);
    i32 MaxX = RoundF32ToI32(RealMaxX);
    i32 MaxY = RoundF32ToI32(RealMaxY);

    if (MinX < 0)
    {
        MinX = 0;
    }
    
    if (MinY < 0)
    {
        MinY = 0;
    }
    
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    u8 *Row = ((u8 *)Buffer->Data +
               MinX * Buffer->BytesPerPixel +
               MinY * Buffer->Pitch);

    for (int Y = MinY;
         Y < MaxY;
         ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        
        for (int X = MinX;
             X < MaxX;
             ++X)
        {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

internal void
GameUpdateAndRender(game_state *State, game_input *Input, game_offscreen_buffer *Buffer)
{
    DrawRectangle(Buffer, 0.0f, 0.0f, (f32)Buffer->Width, (f32)Buffer->Height, 0xFF000000);
    
    f32 RayNumber = 60.0f;
    f32 ColumnWidth = (f32)Buffer->Width / RayNumber;
    f32 CurrentColumn = 0.0f;
    f32 ScreenCenter = (f32)Buffer->Height / 2.0f;
    
    f32 RayAngle = State->PlayerAngle + Pi32 / 6.0f;
    f32 FovEnd = State->PlayerAngle - Pi32 / 6.0f;
    f32 dAngle = (FovEnd - RayAngle) / RayNumber;

    int Count = 0;

    u32 ColumnColor = 0xFF111111;

    while (RayAngle > FovEnd)
    {
        // sin theta = opp / hyp ; cos theta = adj / hyp
        f32 dX = cosf(RayAngle);
        f32 dY = -sinf(RayAngle);

        f32 CurrentX = State->PlayerX;
        f32 CurrentY = State->PlayerY;

        while (CurrentX > 0.0f && CurrentX < 8.0f &&
               CurrentY > 0.0f && CurrentY < 8.0f)
        {
            if (State->Map[(i32)CurrentY][(i32)CurrentX])
            {
                // NOTE: Hit a wall, return
                break;
            }
        
            CurrentX += dX;
            CurrentY += dY;
        }

        f32 DistanceX = CurrentX - State->PlayerX;
        f32 DistanceY = CurrentY - State->PlayerY;

        // hyp = adj / cos theta
        f32 Distance = DistanceX / dX;

#if 1
        if (!State->AlreadyPrinted)
        {
            DEBUGPrintString("%02d: Distance = %2.2f. Found tile at %02d, %02d\n", Count, Distance, (i32)CurrentX, (i32)CurrentY);
        }
#endif
        ++Count;

        // Distance: 0.0f >>> MaxDistance
        // ColumnHeight: Buffer->Height >>> 0.0f;
        f32 MaxDistance = 10.0f;
        f32 ColumnHeightConstant = (MaxDistance - Distance)/MaxDistance;
        f32 ColumnHeight = ColumnHeightConstant * (f32)Buffer->Height;
        f32 ColumnMinY = ScreenCenter - ColumnHeight / 2.0f;
        f32 ColumnMaxY = ScreenCenter + ColumnHeight / 2.0f;
        f32 ColumnMinX = CurrentColumn;
        f32 ColumnMaxX = CurrentColumn + ColumnWidth;
        DrawRectangle(Buffer, ColumnMinX, ColumnMinY, ColumnMaxX, ColumnMaxY, ColumnColor);
        ColumnColor = (ColumnColor + 0xFFABCDEF) % 0xFFFFFFFF;

        RayAngle += dAngle;
        CurrentColumn += ColumnWidth;
    }

    State->AlreadyPrinted = true;

    // State->PlayerAngle += 0.0001f;
    
    // u8 *Row = (u8 *)Buffer->Data;
    // for (int Y = 0;
    //      Y < Buffer->Height;
    //      ++Y)
    // {
    //     u32 *Pixel = (u32 *)Row;
    //     for (int X = 0;
    //          X < Buffer->Width;
    //          ++X)
    //     {
    //         u8 Blue = (u8)(X + State->BlueOffset);
    //         u8 Green = (u8)(Y + State->GreenOffset);

    //         *Pixel++ = ((Green << 8) | Blue);
    //     }

    //     Row += Buffer->Pitch;
    // }

    // ++State->BlueOffset;
    // ++State->GreenOffset;
}
