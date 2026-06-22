
#pragma once

typedef union
{
    struct { f32 X, Y; };
    struct { f32 U, V; };
    struct { f32 R, G; };
    struct { f32 E[2]; };
} f32x2;

local f32x2 F32x2       (f32 X, f32 Y) { return (f32x2){X, Y}; }
local f32x2 ScalarF32x2 (f32 S)        { return (f32x2){S, S}; }

local f32x2 AddF32x2(f32x2 A, f32x2 B) { return F32x2(A.X+B.X, A.Y+B.Y); }
local f32x2 SubF32x2(f32x2 A, f32x2 B) { return F32x2(A.X-B.X, A.Y-B.Y); }
local f32x2 MulF32x2(f32x2 A, f32x2 B) { return F32x2(A.X*B.X, A.Y*B.Y); }
local f32x2 DivF32x2(f32x2 A, f32x2 B) { return F32x2(A.X/B.X, A.Y/B.Y); }

local f32x2 ScalarAddF32x2(f32 A, f32x2 B) { return F32x2(A+B.X, A+B.Y); }
local f32x2 ScalarSubF32x2(f32 A, f32x2 B) { return F32x2(A-B.X, A-B.Y); }
local f32x2 ScalarMulF32x2(f32 A, f32x2 B) { return F32x2(A*B.X, A*B.Y); }
local f32x2 ScalarDivF32x2(f32 A, f32x2 B) { return F32x2(A/B.X, A/B.Y); }

local f32x2 AddScalarF32x2(f32x2 A, f32 B) { return F32x2(A.X+B, A.Y+B); }
local f32x2 SubScalarF32x2(f32x2 A, f32 B) { return F32x2(A.X-B, A.Y-B); }
local f32x2 MulScalarF32x2(f32x2 A, f32 B) { return F32x2(A.X*B, A.Y*B); }
local f32x2 DivScalarF32x2(f32x2 A, f32 B) { return F32x2(A.X/B, A.Y/B); }

typedef union
{
    struct { u32 X, Y; };
    struct { u32 U, V; };
    struct { u32 R, G; };
    struct { u32 E[2]; };
} u32x2;

local u32x2 U32x2       (u32 X, u32 Y) { return (u32x2){X, Y}; }
local u32x2 ScalarU32x2 (u32 S)        { return (u32x2){S, S}; }

typedef struct
{
    f32 E[16];
} m4x4;

local m4x4 IdentityMatrix(void)
{
    m4x4 Result =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };

    return (Result);
}

local m4x4 OrthographicProjection(f32x2 Min, f32x2 Max)
{
    f32x2 Scale  = ScalarDivF32x2(2.0f, SubF32x2(Max, Min));
    f32x2 Center = ScalarMulF32x2(0.5f, AddF32x2(Min, Max));

    m4x4 Result  = {0};

    Result.E[0]  =  Scale.X;
    Result.E[5]  =  Scale.Y;
    Result.E[10] =  0.0f;
    Result.E[15] =  1.0f;

    Result.E[12] = -Center.X * Scale.X;
    Result.E[13] = -Center.Y * Scale.Y;

    return (Result);
}
