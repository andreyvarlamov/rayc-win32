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
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0])

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
    u8 RaycastHitMap[8][8];
    u32 MapColors[8][8];

    // TODO: Remove
    bool32 AlreadyPrinted;
};

internal void
DEBUGPrintString(const char *Format, ...);

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
              u32 Color, u32 BorderColor)
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
            u32 ColorToFill = Color;
            if (Y == MinY || Y == MaxY - 1 || X == MinX || X == MaxX - 1)
            {
                ColorToFill = BorderColor;
            }
            
            *Pixel++ = ColorToFill;
        }

        Row += Buffer->Pitch;
    }
}

internal void
DEBUGDrawMinimap(game_offscreen_buffer *Buffer,
                 game_state *State,
                 f32 RealMinX, f32 RealMinY,
                 f32 RealMaxX, f32 RealMaxY)
{
    DrawRectangle(Buffer, RealMinX, RealMinY, RealMaxX, RealMaxY, 0xFFAAAAAA, 0xFFAAAAAA);

    int MapWidth = 8;
    int MapHeight = 8;
    f32 Padding = 4.0f;
    f32 PaddedMinX = RealMinX + Padding;
    f32 PaddedMinY = RealMinY + Padding;
    f32 PaddedMaxX = RealMaxX - Padding;
    f32 PaddedMaxY = RealMaxY - Padding;
    f32 TileWidth = (PaddedMaxX - PaddedMinX) / (f32)MapWidth;
    f32 TileHeight = (PaddedMaxY - PaddedMinY) / (f32)MapHeight;
    u32 BorderColor = 0xFFAAAAAA;
    u32 EmptyTileColor = 0xFFFFFFFF;
    u32 SolidTileColor = 0xFF000000;
    u32 RaycastHitColor = 0xFFFF00FF;
    
    for (int MapY = 0;
         MapY < MapHeight;
         ++MapY)
    {
        f32 TileMinY = PaddedMinY + (f32)MapY*TileHeight;
        f32 TileMaxY = TileMinY + TileHeight;
        for (int MapX = 0;
             MapX < MapWidth;
             ++MapX)
        {
            f32 TileMinX = PaddedMinX + (f32)MapX*TileWidth;
            f32 TileMaxX = TileMinX + TileWidth;
            u32 FillColor = EmptyTileColor;
            if (State->Map[MapY][MapX])
            {
                FillColor = State->MapColors[MapY][MapX];
            }
        
            DrawRectangle(Buffer, TileMinX, TileMinY, TileMaxX, TileMaxY, FillColor, BorderColor);

            if (State->RaycastHitMap[MapY][MapX])
            {
                f32 RaycastHighlightTilePortion = 0.5f;
                f32 RaycastHighlightWidth = TileWidth*RaycastHighlightTilePortion;
                f32 RaycastHighlightHeight = TileHeight*RaycastHighlightTilePortion;
                f32 TileCenterX = TileMinX + TileWidth/2.0f;
                f32 TileCenterY = TileMinY + TileHeight/2.0f;
                f32 RaycastMinX = TileCenterX - RaycastHighlightWidth/2.0f;
                f32 RaycastMinY = TileCenterY - RaycastHighlightHeight/2.0f;
                f32 RaycastMaxX = RaycastMinX + RaycastHighlightWidth;
                f32 RaycastMaxY = RaycastMinY + RaycastHighlightHeight;
                DrawRectangle(Buffer, RaycastMinX, RaycastMinY, RaycastMaxX, RaycastMaxY, RaycastHitColor, BorderColor);
            }
        }
    }

    f32 PlayerDotHalfSize = 5.0f;
    f32 PlayerMinimapX = PaddedMinX + State->PlayerX*TileWidth;
    f32 PlayerMinimapY = PaddedMinY + State->PlayerY*TileHeight;
    f32 PlayerMinX = PlayerMinimapX - PlayerDotHalfSize;
    f32 PlayerMinY = PlayerMinimapY - PlayerDotHalfSize;
    f32 PlayerMaxX = PlayerMinimapX + PlayerDotHalfSize;
    f32 PlayerMaxY = PlayerMinimapY + PlayerDotHalfSize;
    u32 PlayerColor = 0xFFFF0000;
    DrawRectangle(Buffer, PlayerMinX, PlayerMinY, PlayerMaxX, PlayerMaxY, PlayerColor, PlayerColor);
}

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

    u8 *Source = (u8 *)Map;
    u8 *Dest = (u8 *)State->Map;

    u32 MapTileColor = 0xFF111111;
    u32 *MapColors = (u32 *)State->MapColors;
    
    for (int MapIndex = 0;
         MapIndex < 8*8;
         ++MapIndex)
    {
        *Dest++ = *Source++;

        *MapColors++ = MapTileColor;
        MapTileColor = (MapTileColor + 0xFFABCDEF) % 0xFFFFFFFF;
    }
}

internal void
GameUpdateAndRender(game_state *State, game_input *Input, game_offscreen_buffer *Buffer)
{
    DrawRectangle(Buffer, 0.0f, 0.0f, (f32)Buffer->Width, (f32)Buffer->Height, 0xFF000000, 0xFF000000);
    {
        u8 *RaycastHitMap = (u8 *)State->RaycastHitMap;
        for (int RaycastHitMapIndex = 0;
             RaycastHitMapIndex < 8*8;
             ++RaycastHitMapIndex)
        {
            *RaycastHitMap++ = 0;
        }
    }

    // TODO: Start measuring performance
    f32 RayNumber = 6000.0f;
    f32 ColumnWidth = (f32)Buffer->Width / RayNumber;
    f32 CurrentColumn = 0.0f;
    f32 ScreenCenter = (f32)Buffer->Height / 2.0f;
    
    f32 RayAngle = State->PlayerAngle + Pi32 / 6.0f;
    f32 FovEnd = State->PlayerAngle - Pi32 / 6.0f;
    f32 dAngle = (FovEnd - RayAngle) / RayNumber;

    int Count = 0;


    f32 StepLength = 0.01f;

    while (RayAngle > FovEnd)
    {
        f32 AngleCosine = cosf(RayAngle);
        
        // sin theta = opp / hyp ; cos theta = adj / hyp
        // TODO: DDA
        f32 dX = AngleCosine*StepLength;
        f32 dY = -sinf(RayAngle) * StepLength;

        f32 CurrentX = State->PlayerX;
        f32 CurrentY = State->PlayerY;

        while (CurrentX > 0.0f && CurrentX < 8.0f &&
               CurrentY > 0.0f && CurrentY < 8.0f)
        {
            if (State->Map[(i32)CurrentY][(i32)CurrentX])
            {
                // NOTE: Ray hit a wall
                State->RaycastHitMap[(i32)CurrentY][(i32)CurrentX] = 1;
                break;
            }
        
            CurrentX += dX;
            CurrentY += dY;
        }

        f32 DistanceX = CurrentX - State->PlayerX;
        f32 DistanceY = CurrentY - State->PlayerY;

        // TODO: Calculate dist perpendicular to player fov plane
        // hyp = adj / cos theta
        f32 Distance = DistanceX / AngleCosine;

#if 1
        if (!State->AlreadyPrinted)
        {
            DEBUGPrintString("%02d: Distance = %2.2f. Found tile at %02d, %02d\n", Count, Distance, (i32)CurrentX, (i32)CurrentY);
        }
#endif
        ++Count;

        // Distance: 0.0f >>> MaxDistance
        // ColumnHeight: Buffer->Height >>> 0.0f;
        // TODO: Is this right? What's the reasonable max distance?
        f32 MaxDistance = 8.0f;
        f32 ColumnHeightConstant = (MaxDistance - Distance)/MaxDistance;
        f32 ColumnHeight = ColumnHeightConstant * (f32)Buffer->Height;
        f32 ColumnMinY = ScreenCenter - ColumnHeight / 2.0f;
        f32 ColumnMaxY = ScreenCenter + ColumnHeight / 2.0f;
        f32 ColumnMinX = CurrentColumn;
        f32 ColumnMaxX = CurrentColumn + ColumnWidth;
        u32 ColumnColor = State->MapColors[(i32)CurrentY][(i32)CurrentX];
        DrawRectangle(Buffer, ColumnMinX, ColumnMinY, ColumnMaxX, ColumnMaxY, ColumnColor, ColumnColor);

        RayAngle += dAngle;
        CurrentColumn += ColumnWidth;
    }

    State->AlreadyPrinted = true;

    State->PlayerAngle += 0.01f;

    f32 MinimapWidth = 450;
    f32 MinimapHeight = 450;
    f32 MinimapMinX = (f32)Buffer->Width - MinimapWidth;
    f32 MinimapMaxX = (f32)Buffer->Width;
    f32 MinimapMinY = (f32)Buffer->Height - MinimapHeight;
    f32 MinimapMaxY = (f32)Buffer->Height;
    DEBUGDrawMinimap(Buffer, State, MinimapMinX, MinimapMinY, MinimapMaxX, MinimapMaxY);
    
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
