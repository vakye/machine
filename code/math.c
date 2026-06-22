
#pragma once

typedef union
{
    struct { f32 X, Y; };
    struct { f32 U, V; };
    struct { f32 R, G; };
    struct { f32 E[2]; };
} f32x2;

#define F32x2(X, Y)    (f32x2){(f32)(X), (f32)(Y)}
#define ScalarF32x2(S) F32x2(S, S)

local f32x2 AddF32x2(f32x2 A, f32x2 B) { return F32x2(A.X+B.X, A.Y+B.Y); }
local f32x2 SubF32x2(f32x2 A, f32x2 B) { return F32x2(A.X-B.X, A.Y-B.Y); }
local f32x2 MulF32x2(f32x2 A, f32x2 B) { return F32x2(A.X*B.X, A.Y*B.Y); }
local f32x2 DivF32x2(f32x2 A, f32x2 B) { return F32x2(A.X/B.X, A.Y/B.Y); }

#define ScalarAddF32x2(A, B) AddF32x2(ScalarF32x2(A), B)
#define ScalarSubF32x2(A, B) SubF32x2(ScalarF32x2(A), B)
#define ScalarMulF32x2(A, B) MulF32x2(ScalarF32x2(A), B)
#define ScalarDivF32x2(A, B) DivF32x2(ScalarF32x2(A), B)

#define AddScalarF32x2(A, B) AddF32x2(A, ScalarF32x2(B))
#define SubScalarF32x2(A, B) SubF32x2(A, ScalarF32x2(B))
#define MulScalarF32x2(A, B) MulF32x2(A, ScalarF32x2(B))
#define DivScalarF32x2(A, B) DivF32x2(A, ScalarF32x2(B))

typedef union
{
    struct { u32 X, Y; };
    struct { u32 U, V; };
    struct { u32 R, G; };
    struct { u32 E[2]; };
} u32x2;

#define U32x2(X, Y) (u32x2){(u32)(X), (u32)(Y)}

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

    m4x4 Result = IdentityMatrix();

    Result.E[0]  =  Scale.X;
    Result.E[5]  =  Scale.Y;
    Result.E[12] = -Center.X * Scale.X;
    Result.E[13] = -Center.Y * Scale.Y;

    return (Result);
}
