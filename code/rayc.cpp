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
    f32 SecondsPerFrame;

    bool32 MouseLeft;
    bool32 MouseRight;
    i32 MouseDX;
    i32 MouseDY;
    i32 MouseDZ;

    bool32 Forward;
    bool32 StrafeLeft;
    bool32 StrafeRight;
    bool32 Back;
    
    bool32 Up;
    bool32 Down;
    bool32 Left;
    bool32 Right;
};

struct point_real
{
    f32 X;
    f32 Y;
};

struct ray_to_point
{
    f32 Distance;
    f32 Angle;
};

struct ray_data
{
    f32 RayAngle;

    f32 InterceptX;
    f32 InterceptY;

    i32 TileX;
    i32 TileY;

    f32 HitWallTexturePosition;
    f32 Distance;
};

struct texture
{
    void *Pixels;
    i32 Width;
    i32 Height;
    i32 BytesPerPixel;
    i32 Pitch;
};

#define TEXTURE_NUM 16
struct render_data
{
    texture Textures[TEXTURE_NUM];
};

#define RAYCAST_NUM 1600
struct game_state
{
    f32 PlayerX;
    f32 PlayerY;
    f32 PlayerAngle;

    f32 EnemyX;
    f32 EnemyY;

    u8 Map[8][8];
    u8 RaycastHitMap[8][8];
    u32 MapColors[8][8];
    ray_data RaycastData[RAYCAST_NUM];

    render_data RenderData;
};

struct platform_read_file_result
{
    u32 ContentsSize;
    void *Contents;
};

internal void
DEBUGPrintString(const char *Format, ...);

internal void
PLATFORMFreeFileMemory(void *Memory);

internal platform_read_file_result
PLATFORMReadEntireFile(char *Filename);

inline u32
SafeTruncateU64(u64 Value)
{
    // TODO: Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return Result;
}

inline i32
RoundF32ToI32(f32 Real)
{
    i32 Result = (i32)(Real + 0.5f);
    return Result;
}

inline i32
TruncateF32ToI32(f32 Real)
{
    i32 Result = (i32)Real;
    return Result;
}

inline f32
AbsoluteF32(f32 Value)
{
    f32 Result = ((Value > 0) ? Value : -Value);
    return Result;
}

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    i32 Width;
    i32 Height;
    u16 Planes;
    u16 BitsPerPixel;
};
#pragma pack(pop)

internal texture
LoadBMP(char *Filename)
{
    texture Result = {0};
    platform_read_file_result ReadResult = PLATFORMReadEntireFile(Filename);
    if (ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        Result.Pixels = (u8 *)ReadResult.Contents + Header->BitmapOffset;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.BytesPerPixel = 4;
        Result.Pitch = Result.Width * Result.BytesPerPixel;
    }

    return Result;
}

internal void
DrawBitmap(game_offscreen_buffer *DestBuffer, texture *SourceBitmap,
           f32 RealDestMinX, f32 RealDestMinY,
           f32 RealDestMaxX, f32 RealDestMaxY,
           f32 RealScaleX, f32 RealScaleY,
           f32 RealScaledOffsetX, f32 RealScaledOffsetY)
{
    i32 DestMinX = RoundF32ToI32(RealDestMinX);
    i32 DestMinY = RoundF32ToI32(RealDestMinY);
    i32 DestMaxX = RoundF32ToI32(RealDestMaxX);
    i32 DestMaxY = RoundF32ToI32(RealDestMaxY);
    if (DestMinX < 0) DestMinX = 0;
    if (DestMinY < 0) DestMinY = 0;
    if (DestMaxX > DestBuffer->Width) DestMaxX = DestBuffer->Width;
    if (DestMaxY > DestBuffer->Height) DestMaxY = DestBuffer->Height;

    u8 *Destination = ((u8 *)DestBuffer->Data +
                   DestMinX*DestBuffer->BytesPerPixel +
                   DestMinY*DestBuffer->Pitch);
    u8 *Source = ((u8 *)SourceBitmap->Pixels +
                  (SourceBitmap->Height-1)*SourceBitmap->Pitch);

    i32 RoundedScaleX = RoundF32ToI32(RealScaleX);
    i32 RoundedScaleY = RoundF32ToI32(RealScaleY);

    f32 RealSourceDX, RealSourceDY;
    if (RoundedScaleX > 0)
    {
        RealSourceDX = (f32)SourceBitmap->Width / (f32)RoundedScaleX;
    }
    else
    {
        RealSourceDX = 1.0f;
        RoundedScaleX = SourceBitmap->Width;
    }
    if (RoundedScaleY > 0)
    {
        RealSourceDY = (f32)SourceBitmap->Height/ (f32)RoundedScaleY;
    }
    else
    {
        RealSourceDY = 1.0f;
        RoundedScaleY = SourceBitmap->Height;
    }

    if (RealScaledOffsetX < 0.0f) RealScaledOffsetX = 0.0f;
    if (RealScaledOffsetY < 0.0f) RealScaledOffsetY = 0.0f;
    f32 RealSourceOffsetX = RealScaledOffsetX*RealSourceDX;
    f32 RealSourceOffsetY = RealScaledOffsetY*RealSourceDY;
    
    i32 ScaledColumnsToDraw = RoundedScaleX-RoundF32ToI32(RealScaledOffsetX);
    i32 ScaledRowsToDraw = RoundedScaleY-RoundF32ToI32(RealScaledOffsetY);

    f32 RealSourceCursorY = RealSourceOffsetY;
    for (int DestY = DestMinY;
         DestY < DestMaxY;
         ++DestY)
    {
        i32 RowsDrawn = DestY-DestMinY;
        if (RowsDrawn >= ScaledRowsToDraw)
        {
            break;
        }
        
        i32 TruncatedSourceCursorY = TruncateF32ToI32(RealSourceCursorY);
        if (TruncatedSourceCursorY >= SourceBitmap->Height)
        {
            TruncatedSourceCursorY = SourceBitmap->Height - 1;
        }
        
        u32 *DestPixel = (u32 *)(Destination + RowsDrawn*DestBuffer->Pitch);
        u32 *SourcePixel = (u32 *)(Source - TruncatedSourceCursorY*SourceBitmap->Pitch);
        
        f32 RealSourceCursorX = RealSourceOffsetX;
        for (int DestX = DestMinX;
             DestX < DestMaxX;
             ++DestX)
        {
            i32 ColumnsDrawn = DestX-DestMinX;
            if (ColumnsDrawn >= ScaledColumnsToDraw)
            {
                break;
            }
            
            i32 TruncatedSourceCursorX = TruncateF32ToI32(RealSourceCursorX);
            if (TruncatedSourceCursorX >= SourceBitmap->Width)
            {
                TruncatedSourceCursorX = SourceBitmap->Width - 1;
            }
            
            *(DestPixel + (DestX-DestMinX)) = *(SourcePixel + TruncatedSourceCursorX);
            
            RealSourceCursorX += RealSourceDX;
        }

        RealSourceCursorY += RealSourceDY;
    }
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
    if (MinX < 0) MinX = 0;
    if (MinY < 0) MinY = 0;
    if (MaxX > Buffer->Width) MaxX = Buffer->Width;
    if (MaxY > Buffer->Height) MaxY = Buffer->Height;

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
DrawWallVerticalSection(game_offscreen_buffer *Buffer,
                        f32 RealMinX, f32 RealMinY,
                        f32 RealMaxX, f32 RealMaxY,
                        texture *Texture,
                        f32 TextureHorizontalPosition)
{
    f32 TextureOffsetY = 0.0f;
    if (RealMinY < 0.0f)
    {
        TextureOffsetY = (-RealMinY);
    }
    DrawBitmap(Buffer, Texture,
               RealMinX, RealMinY,
               RealMaxX, RealMaxY,
               900.0f, RealMaxY-RealMinY,
               900.0f*TextureHorizontalPosition, TextureOffsetY);
    // DrawRectangle(Buffer, RealMinX, RealMinY, RealMaxX, RealMaxY, 0xFF0000FF, 0xFF0000FF);
}

internal void
DrawLine(game_offscreen_buffer *Buffer,
         f32 RealStartX, f32 RealStartY,
         f32 RealEndX, f32 RealEndY,
         u32 Color)
{
    f32 DistanceX = RealEndX - RealStartX;
    f32 DistanceY = RealEndY - RealStartY;
    // TODO: Use absolute function
    f32 DistanceX_Abs = ((DistanceX > 0) ? DistanceX : -DistanceX);
    f32 DistanceY_Abs = ((DistanceY > 0) ? DistanceY : -DistanceY);
    f32 SteppingDistance = ((DistanceX_Abs >= DistanceY_Abs) ? DistanceX_Abs : DistanceY_Abs);
    f32 dX = DistanceX / SteppingDistance;
    f32 dY = DistanceY / SteppingDistance;

    f32 X = RealStartX;
    f32 Y = RealStartY;
    u32 *Pixels = (u32 *)Buffer->Data;
    for (i32 SteppingIndex = 0;
         SteppingIndex < (i32)SteppingDistance;
         ++SteppingIndex)
    {
        i32 RoundedX = RoundF32ToI32(X);
        i32 RoundedY = RoundF32ToI32(Y);
        if (RoundedX >= 0 && RoundedX < Buffer->Width &&
            RoundedY >= 0 && RoundedY < Buffer->Height)
        {
            Pixels[RoundedY * Buffer->Width + RoundedX] = Color;
        }

        X += dX;
        Y += dY;
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

    f32 EntityDotHalfSize = 5.0f;
    
    f32 PlayerMinimapX = PaddedMinX + State->PlayerX*TileWidth;
    f32 PlayerMinimapY = PaddedMinY + State->PlayerY*TileHeight;
    f32 PlayerMinX = PlayerMinimapX - EntityDotHalfSize;
    f32 PlayerMinY = PlayerMinimapY - EntityDotHalfSize;
    f32 PlayerMaxX = PlayerMinimapX + EntityDotHalfSize;
    f32 PlayerMaxY = PlayerMinimapY + EntityDotHalfSize;
    u32 PlayerColor = 0xFFFF00FF;
    DrawRectangle(Buffer, PlayerMinX, PlayerMinY, PlayerMaxX, PlayerMaxY, PlayerColor, PlayerColor);

    for (int RayIndex = 0;
         RayIndex < RAYCAST_NUM;
         ++RayIndex)
    {
        f32 LineStartX = PaddedMinX + State->PlayerX * TileWidth;
        f32 LineStartY = PaddedMinY + State->PlayerY * TileHeight;
        f32 LineEndX = PaddedMinX + State->RaycastData[RayIndex].InterceptX * TileWidth;
        f32 LineEndY = PaddedMinY + State->RaycastData[RayIndex].InterceptY * TileHeight;
        DrawLine(Buffer, LineStartX, LineStartY, LineEndX, LineEndY, RaycastHitColor);
    }

    f32 EnemyMinimapX = PaddedMinX + State->EnemyX*TileWidth;
    f32 EnemyMinimapY = PaddedMinY + State->EnemyY*TileHeight;
    f32 EnemyMinX = EnemyMinimapX - EntityDotHalfSize;
    f32 EnemyMinY = EnemyMinimapY - EntityDotHalfSize;
    f32 EnemyMaxX = EnemyMinimapX + EntityDotHalfSize;
    f32 EnemyMaxY = EnemyMinimapY + EntityDotHalfSize;
    u32 EnemyColor = 0xFFFF0000;
    DrawRectangle(Buffer, EnemyMinX, EnemyMinY, EnemyMaxX, EnemyMaxY, EnemyColor, EnemyColor);
}

internal ray_data
CastARay(game_state *State, f32 PlayerAngle, f32 RayAngle)
{
    Assert(RayAngle > -2*Pi32 && RayAngle < 4*Pi32);

    ray_data Result = {0};
    
    if (RayAngle >= 2*Pi32)
    {
        RayAngle -= 2*Pi32;
    }
    if (RayAngle < 0.0f)
    {
        RayAngle += 2*Pi32;
    }

    Result.RayAngle = RayAngle;

    f32 X_DecimalPart = State->PlayerX - (f32)TruncateF32ToI32(State->PlayerX);
    f32 Y_DecimalPart = State->PlayerY - (f32)TruncateF32ToI32(State->PlayerY);
    f32 OffsetX, OffsetY;
    f32 X_StepDirection, Y_StepDirection;
    if (RayAngle > 0.0f && RayAngle <= Pi32/2.0f)
    {
        // NOTE: NE Quadrant
        OffsetX = 1.0f-X_DecimalPart;
        OffsetY = Y_DecimalPart;
        X_StepDirection = 1.0f;
        Y_StepDirection = -1.0f;
    }
    else if (RayAngle > Pi32/2.0f && RayAngle <= Pi32)
    {
        // NOTE: NW Quadrant
        OffsetX = X_DecimalPart;
        OffsetY = Y_DecimalPart;
        X_StepDirection = -1.0f;
        Y_StepDirection = -1.0f;
    }
    else if (RayAngle > Pi32 && RayAngle <= 3.0f*Pi32/2.0f)
    {
        // NOTE: SW Quadrant
        OffsetX = X_DecimalPart;
        OffsetY = 1.0f-Y_DecimalPart;
        X_StepDirection = -1.0f;
        Y_StepDirection = 1.0f;
    }
    else
    {
        // NOTE: SE Quadrant
        OffsetX = 1.0f-X_DecimalPart;
        OffsetY = 1.0f-Y_DecimalPart;
        X_StepDirection = 1.0f;
        Y_StepDirection = 1.0f;
    }

    f32 RayAngleTan = AbsoluteF32(tanf(RayAngle));

    bool32 IsInterceptHorizontal = false;

    // Vertical Intercepts (traversing horizontally)
    f32 VerticalInterceptX = State->PlayerX + X_StepDirection*OffsetX;
    f32 VerticalInterceptY = State->PlayerY + Y_StepDirection*OffsetX*RayAngleTan;
    for (;;)
    {
        i32 HitTileX = RoundF32ToI32(VerticalInterceptX);
        if (X_StepDirection < 0)
        {
            --HitTileX;
        }
        i32 HitTileY = TruncateF32ToI32(VerticalInterceptY);
        
        if (HitTileX < 0 || HitTileX >= 8 ||
            HitTileY < 0 || HitTileY >= 8 ||
            State->Map[HitTileY][HitTileX])
        {
            // NOTE: Hit a wall or end of map
            Result.InterceptX = VerticalInterceptX;
            Result.InterceptY = VerticalInterceptY;
            Result.TileX = HitTileX;
            Result.TileY = HitTileY;
            break;
        }
        
        VerticalInterceptX += X_StepDirection;
        VerticalInterceptY += Y_StepDirection*RayAngleTan;
    }

    // Horizontal Intercepts (traversing vertically)
    f32 HorizontalInterceptX = State->PlayerX + X_StepDirection*OffsetY/RayAngleTan;
    f32 HorizontalInterceptY = State->PlayerY + Y_StepDirection*OffsetY;
    for (;;)
    {
        i32 HitTileX = TruncateF32ToI32(HorizontalInterceptX);
        i32 HitTileY = RoundF32ToI32(HorizontalInterceptY);
        if (Y_StepDirection < 0)
        {
            --HitTileY;
        }
        
        if (HitTileX < 0 || HitTileX >= 8 ||
            HitTileY < 0 || HitTileY >= 8 ||
            State->Map[HitTileY][HitTileX])
        {
            // NOTE: Hit a wall or end of map

            bool32 ShouldReplaceVerticalIntercept = false;
            if (RayAngleTan >= 1)
            {
                // NOTE:
                // Absolute value of tangent of theta is greater than 1 => Slope (y1-y0)/(x1-x0) is greater than 1
                // Determine the closer to player point using Y axis
                f32 HorizontalInterceptDistanceToPlayerY = AbsoluteF32(State->PlayerY - HorizontalInterceptY);
                f32 VerticalInterceptDistanceToPlayerY = AbsoluteF32(State->PlayerY - VerticalInterceptY);
                ShouldReplaceVerticalIntercept = HorizontalInterceptDistanceToPlayerY < VerticalInterceptDistanceToPlayerY;
            }
            else
            {
                // NOTE:
                // Absolute value of tangent of theta is less than 1 => Slope (y1-y0)/(x1-x0) is less than 1
                // Determine the closer to player point using X axis
                f32 HorizontalInterceptDistanceToPlayerX = AbsoluteF32(State->PlayerX - HorizontalInterceptX);
                f32 VerticalInterceptDistanceToPlayerX = AbsoluteF32(State->PlayerX - VerticalInterceptX);
                ShouldReplaceVerticalIntercept = HorizontalInterceptDistanceToPlayerX < VerticalInterceptDistanceToPlayerX;
            }

            if (ShouldReplaceVerticalIntercept)
            {
                Result.InterceptX = HorizontalInterceptX;
                Result.InterceptY = HorizontalInterceptY;
                Result.TileX = HitTileX;
                Result.TileY = HitTileY;

                IsInterceptHorizontal = true;
            }

            break;
        }

        HorizontalInterceptX += X_StepDirection/RayAngleTan;
        HorizontalInterceptY += Y_StepDirection;
    }

    f32 PlayerInterceptDistanceX = Result.InterceptX - State->PlayerX;
    f32 PlayerInterceptDistanceY = State->PlayerY - Result.InterceptY; // ???????????????????????????????????????????????????
                                                                 // IMPORTANT: Review this. Only 70% understand why Y should be inverted
    // f32 OldDistance = sqrtf(PlayerInterceptDistanceX*PlayerInterceptDistanceX +
    //                      PlayerInterceptDistanceY*PlayerInterceptDistanceY);

    // // f32 DiffAngle = PlayerAngle - RayAngle;
    // f32 DiffAngle = RayAngle - PlayerAngle;
    // if (DiffAngle < 0.0f)
    // {
    //     DiffAngle += 2*Pi32;
    // }
    // if (DiffAngle > 2*Pi32)
    // {
    //     DiffAngle -= 2*Pi32;
    // }
    // f32 HahaDistance = OldDistance * cosf(DiffAngle);

    // f32 LolDistance = OldDistance * cosf(RayAngle) * cosf (PlayerAngle) + OldDistance * sinf(RayAngle) * sinf(PlayerAngle);

    // f32 OldDistCosPlayer = OldDistance * cosf(RayAngle);
    // f32 OldDistSinPlayer = OldDistance * sinf(RayAngle);
    // f32 SuperLolDistance = OldDistCosPlayer*cosf(PlayerAngle) + OldDistSinPlayer*sinf(PlayerAngle);

    Result.Distance = (PlayerInterceptDistanceX*cosf(PlayerAngle) +
                    PlayerInterceptDistanceY*sinf(PlayerAngle));

    if (IsInterceptHorizontal)
    {
        Result.HitWallTexturePosition = Result.InterceptX - TruncateF32ToI32(Result.InterceptX);
    }
    else
    {
        Result.HitWallTexturePosition = Result.InterceptY - TruncateF32ToI32(Result.InterceptY);
    }

    // bool32 ShouldBreak = AbsoluteF32(Distance - SuperLolDistance) > 0.1f;
    return Result;
}

internal ray_to_point
CastARayToPoint(game_state *State, f32 X1, f32 Y1, f32 X2, f32 Y2)
{
    ray_to_point Result = {0};
    
    f32 dX = X2 - X1;
    f32 dY = Y1 - Y2;

    Result.Distance = sqrtf(dX*dX + dY*dY);
    Result.Angle = atan2f(dY, dX);

    return Result;
}

internal void
GameStateInit(game_state *State)
{
    State->PlayerX = 1.5f;
    State->PlayerY = 2.5f;
    // State->PlayerAngle = -Pi32/12.0f;
    State->PlayerAngle = 2*Pi32-Pi32/8;

    State->EnemyX = 6.5f;
    State->EnemyY = 3.5f;

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

    render_data RenderData = {0};

    RenderData.Textures[0] = LoadBMP("textures/brick.bmp");
    RenderData.Textures[1] = LoadBMP("textures/pumpkin.bmp");
    RenderData.Textures[2] = LoadBMP("textures/enemy.bmp");
    
    State->RenderData = RenderData;
}

internal void
ProcessInput(game_state *State, game_input *Input)
{
    i32 MouseXDeltaRange = 700;
    State->PlayerAngle += -((f32)Input->MouseDX/(f32)MouseXDeltaRange) * Input->SecondsPerFrame * 20.0f;
    if (State->PlayerAngle >= 2*Pi32)
    {
        State->PlayerAngle -= 2*Pi32;
    }
    else if (State->PlayerAngle < 0.0f)
    {
        State->PlayerAngle += 2*Pi32;
    }

    f32 PlayerVelocity = 2.0f; // tiles/sec

    f32 DiagonalMovementCoefficient = 0.707107f; // 1/sqrt(2)

    f32 PlayerDForward = 0.0f;
    f32 PlayerDStrafe = 0.0f;
    if (Input->Forward)
    {
        PlayerDForward = PlayerVelocity * Input->SecondsPerFrame;
    }
    else if (Input->Back)
    {
        PlayerDForward = -PlayerVelocity * Input->SecondsPerFrame;
    }
    
    if (Input->StrafeLeft)
    {
        PlayerDStrafe = -PlayerVelocity * Input->SecondsPerFrame;
    }
    else if (Input->StrafeRight)
    {
        PlayerDStrafe = PlayerVelocity * Input->SecondsPerFrame;
    }

    if (AbsoluteF32(PlayerDForward) > 0.0f && AbsoluteF32(PlayerDStrafe) > 0.0f)
    {
        PlayerDForward *= DiagonalMovementCoefficient;
        PlayerDStrafe *= DiagonalMovementCoefficient;
    }

    f32 PlayerAngleCos = cosf(State->PlayerAngle);
    f32 PlayerAngleSin = sinf(State->PlayerAngle);

    f32 PlayerDX = PlayerDForward*PlayerAngleCos + PlayerDStrafe*PlayerAngleSin;
    f32 PlayerDY = -PlayerDForward*PlayerAngleSin + PlayerDStrafe*PlayerAngleCos;
    f32 NewPlayerX = State->PlayerX + PlayerDX;
    f32 NewPlayerY = State->PlayerY + PlayerDY;

    f32 CollisionTestDirectionX = ((PlayerDX > 0.0f) ? 1.0f : -1.0f);
    f32 CollisionTestDirectionY = ((PlayerDY > 0.0f) ? 1.0f : -1.0f);
    f32 CollisionTestOffset = 0.2f;
    f32 PlayerCollisionTestPositionX = NewPlayerX + CollisionTestDirectionX*CollisionTestOffset;
    f32 PlayerCollisionTestPositionY = NewPlayerY + CollisionTestDirectionY*CollisionTestOffset;

    f32 WallSlideDeadzone = 0.015f;

    if (!State->Map[TruncateF32ToI32(PlayerCollisionTestPositionY)][TruncateF32ToI32(PlayerCollisionTestPositionX)])
    {
        State->PlayerX = NewPlayerX;
        State->PlayerY = NewPlayerY;
    }
    else if (!State->Map[TruncateF32ToI32(PlayerCollisionTestPositionY)][TruncateF32ToI32(State->PlayerX)])
    {
        if (AbsoluteF32(PlayerDY) > WallSlideDeadzone)
        {
            State->PlayerY = NewPlayerY;
        }
    }
    else if (!State->Map[TruncateF32ToI32(State->PlayerY)][TruncateF32ToI32(PlayerCollisionTestPositionX)])
    {
        if (AbsoluteF32(PlayerDX) > WallSlideDeadzone)
        {
            State->PlayerX = NewPlayerX;
        }
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

    ProcessInput(State, Input);

    i32 RayNumber = RAYCAST_NUM;
    f32 ColumnWidth = (f32)Buffer->Width / (f32)RayNumber;
    f32 CurrentColumn = 0.0f;
    f32 ScreenCenter = (f32)Buffer->Height / 2.0f;

    // NOTE: Start is the smaller angle. Going counterclockwise to the end - the greater angle.
    // But drawing from left to right, so going clockwise.
    f32 PlayerFovStart = State->PlayerAngle - Pi32 / 6.0f;
    f32 PlayerFovEnd = State->PlayerAngle + Pi32 / 6.0f;
    
    f32 dAngle = (PlayerFovStart - PlayerFovEnd) / (f32)RayNumber;
    
    // TODO: Is this right? What's the reasonable max distance?
    f32 ColumnHeightConstant = 900.0f;

    f32 RayAngle = PlayerFovEnd;
    for (int RayIndex = 0;
         RayIndex < RayNumber;
         ++RayIndex)
    {
        ray_data RayData = CastARay(State, State->PlayerAngle, RayAngle);

        f32 ColumnHeight = ColumnHeightConstant / RayData.Distance;
        f32 ColumnMinY = ScreenCenter - ColumnHeight / 2.0f;
        f32 ColumnMaxY = ScreenCenter + ColumnHeight / 2.0f;
        f32 ColumnMinX = CurrentColumn;
        f32 ColumnMaxX = CurrentColumn + ColumnWidth;
        u32 ColumnColor = 0;
        if (RayData.TileX >= 0 && RayData.TileX < 8 &&
            RayData.TileY >= 0 && RayData.TileY < 8)
        {
            ColumnColor = State->MapColors[RayData.TileY][RayData.TileX];
            texture *Texture = (ColumnColor % 2 == 0) ? &State->RenderData.Textures[1] : &State->RenderData.Textures[0]; 
            DrawWallVerticalSection(Buffer, ColumnMinX, ColumnMinY, ColumnMaxX, ColumnMaxY,
                                    Texture, RayData.HitWallTexturePosition);
        }

        // if (State->Map[RayData.TileY][RayData.TileX] == 2)
        // {
        //     DrawWallVerticalSection(Buffer, ColumnMinX, ColumnMinY, ColumnMaxX, ColumnMaxY,
        //                             Texture, RayData.HitWallTexturePosition);
        // }
        // else
        // {
        //     DrawRectangle(Buffer, ColumnMinX, ColumnMinY, ColumnMaxX, ColumnMaxY, ColumnColor, ColumnColor);
        // }

        State->RaycastData[RayIndex] = RayData;

        RayAngle += dAngle;
        CurrentColumn += ColumnWidth;
    }

    ray_to_point RayToEnemy = CastARayToPoint(State, State->PlayerX, State->PlayerY, State->EnemyX, State->EnemyY);
    f32 AngleToEnemy = RayToEnemy.Angle;

    f32 NormalizedPlayerFovStart = PlayerFovStart;
    if (NormalizedPlayerFovStart < -Pi32)
    {
        NormalizedPlayerFovStart += 2.0f*Pi32;
    }
    if (NormalizedPlayerFovStart > Pi32)
    {
        NormalizedPlayerFovStart -= 2.0f*Pi32;
    }
    f32 NormalizedPlayerFovEnd = PlayerFovEnd;
    if (NormalizedPlayerFovEnd < -Pi32)
    {
        NormalizedPlayerFovEnd += 2.0f*Pi32;
    }
    if (NormalizedPlayerFovEnd > Pi32)
    {
        NormalizedPlayerFovEnd -= 2.0f*Pi32;
    }
    DEBUGPrintString("PlayerFovStart: %.02f; NormalizedPlayerFovStart: %.02f; PlayerFovEnd: %.02f; NormalizedPlayerFovEnd: %.02f; AngleToEnemy: %.02f\n",
                     PlayerFovStart, NormalizedPlayerFovStart, PlayerFovEnd, NormalizedPlayerFovEnd, AngleToEnemy);

sddd    if ((AngleToEnemy >= NormalizedPlayerFovStart) && (AngleToEnemy <= NormalizedPlayerFovEnd))
    {
        f32 DistanceToEnemy = RayToEnemy.Distance;
        f32 SpriteHeight = ColumnHeightConstant / DistanceToEnemy;
        f32 SpriteMinY = ScreenCenter - SpriteHeight / 2.0f;
        f32 SpriteMaxY = ScreenCenter + SpriteHeight / 2.0f;
        f32 SpriteMinX = (f32)Buffer->Width/2.0f - (f32)State->RenderData.Textures[2].Width/2.0f;
        f32 SpriteMaxX = (f32)Buffer->Width/2.0f + (f32)State->RenderData.Textures[2].Width/2.0f;
        DrawBitmap(Buffer, &State->RenderData.Textures[2],
                   SpriteMinX, SpriteMinY,
                   SpriteMaxX, SpriteMaxY,
                   SpriteMaxX-SpriteMinX, SpriteMaxY-SpriteMinY,
                   -1.0f, -1.0f);
    }

    f32 MinimapWidth = 450;
    f32 MinimapHeight = 450;
    f32 MinimapMinX = (f32)Buffer->Width - MinimapWidth;
    f32 MinimapMaxX = (f32)Buffer->Width;
    f32 MinimapMinY = (f32)Buffer->Height - MinimapHeight;
    f32 MinimapMaxY = (f32)Buffer->Height;
    DEBUGDrawMinimap(Buffer, State, MinimapMinX, MinimapMinY, MinimapMaxX, MinimapMaxY);

    // for (int TextureXOffset = 0;
    //      TextureXOffset < 1600;
    //      ++TextureXOffset)
    // {
    //     f32 ColumnHeight = 450.0f + (f32)TextureXOffset/1600.0f * 450.0f;
    //     f32 ColumnMinY = ScreenCenter - ColumnHeight/2.0f;
    //     f32 ColumnMaxY = ScreenCenter + ColumnHeight/2.0f;
    //     DrawBitmap(Buffer, State->RenderData.Textures[1],
    //                0.0f+(f32)TextureXOffset, 0.0f,
    //                1.0f+(f32)TextureXOffset, ColumnHeight,
    //                1600.0f, ColumnHeight,
    //                0.0f+(f32)TextureXOffset, 0.0f);
                   
    // }
    // DrawBitmap(Buffer, State->RenderData.Textures[1],
    //            0.0f, 0.0f,
    //            1000.0f, 1000.0f,
    //            800.0f, 800.0f,
    //            0.0f, 0.0f);
               
    
    // for (int TextureXOffset = 0;
    //      TextureXOffset < 400;
    //      ++TextureXOffset)
    // {
    //     f32 ColumnHeight = 200.0f + ((f32)TextureXOffset/400.0f)*200.0f;
    //     f32 ColumnMinY = ScreenCenter - ColumnHeight / 2.0f;
    //     f32 ColumnMaxY = ScreenCenter + ColumnHeight / 2.0f;
    //     DrawBitmap(Buffer, State->RenderData.Textures[0],
    //                0.0f+(f32)TextureXOffset, ColumnMinY,
    //                1.0f+(f32)TextureXOffset, ColumnMaxY,
    //                TextureXOffset, 0,
    //                false, true);
    // }
}
