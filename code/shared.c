
// NOTE(vak): Commonly used types, defines, macros, ...
// throughout the codebase.

#pragma once

// ----------------------------------------------------------
// NOTE(vak): Build Configurations
// ----------------------------------------------------------

#if !defined(DebugBuild)
#  define DebugBuild (0)
#endif

// ----------------------------------------------------------
// NOTE(vak): Compiler Detection
// ----------------------------------------------------------

#if defined(__clang__)
#  define CompilerClang (1)
#endif

#if defined(_MSC_VER)
#  define CompilerMSVC (1)
#elif defined(__GNUC__)
#  define CompilerGCC (1)
#else
#  error Unknown Compiler
#endif

#if !defined(CompilerClang)
#  define CompilerClang (0)
#endif

#if !defined(CompilerMSVC)
#  define CompilerMSVC (0)
#endif

#if !defined(CompilerGCC)
#  define CompilerGCC (0)
#endif

// ----------------------------------------------------------
// NOTE(vak): Platform Detection
// ----------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
#  define PlatformWindows (1)
#elif defined(__linux__)
#  define PlatformLinux (1)
#elif (defined(__APPLE__) && defined(__MACH__)) || defined(Macintosh) || defined(macintosh)
#  define PlatformMacOS (1)
#else
#  error Unknown Platform
#endif

// NOTE(vak): Android is based on Linux, so "PlatformLinux"
// is also toggled on Android platforms.
#if defined(__ANDROID__)
#  define PlatformAndroid (1)
#else
#  define PlatformAndroid (0)
#endif

#if !defined(PlatformWindows)
#  define PlatformWindows (0)
#endif

#if !defined(PlatformLinux)
#  define PlatformLinux (0)
#endif

#if !defined(PlatformMacOS)
#  define PlatformMacOS (0)
#endif

// ----------------------------------------------------------
// NOTE(vak): CPU Architecture Detection
// ----------------------------------------------------------

#if defined(_M_AMD64) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#  define ArchitectureX64 (1)
#elif defined (__aarch64__)
#  define ArchitectureARM64 (1)
#else
#  error Unknown CPU Architecture
#endif

#if !defined(ArchitectureX64)
#  define ArchitectureX64 (0)
#endif

#if !defined(ArchitectureARM64)
#  define ArchitectureARM64 (0)
#endif

// ----------------------------------------------------------
// NOTE(vak): Keywords
// ----------------------------------------------------------

#define local static
#define persist static

#if CompilerGCC
#  define InvalidCodePath __builtin_trap()
#else
#  define InvalidCodePath *(int*)0 = 0
#endif

#define InvalidDefaultCase default: { InvalidCodePath; } break

// ----------------------------------------------------------
// NOTE(vak): Assertion
// ----------------------------------------------------------

#define CTAssert(Expression) \
    _Static_assert(Expression, "Compile-time assertion failed")

#define AlwaysAssert(Expression) \
    if (!(Expression)) \
        InvalidCodePath

#if defined(DebugBuild)
#  define DebugAssert(Expression) AlwaysAssert(Expression)
#else
#  define DebugAssert(...) (void)0
#endif

// ----------------------------------------------------------
// NOTE(vak): Types
// ----------------------------------------------------------

typedef signed char      s8;
typedef signed short     s16;
typedef signed int       s32;
typedef signed long long s64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef s64 ssize;
typedef u64 usize;

typedef float f32;
typedef double f64;

typedef union { f32 F32; u32 U32; } ieee754_single;
typedef union { f64 F64; u64 U64; } ieee754_double;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

// ----------------------------------------------------------
// NOTE(vak): Type size checks
// ----------------------------------------------------------

CTAssert(sizeof(s8)  == 1);
CTAssert(sizeof(s16) == 2);
CTAssert(sizeof(s32) == 4);
CTAssert(sizeof(s64) == 8);

CTAssert(sizeof(u8)  == 1);
CTAssert(sizeof(u16) == 2);
CTAssert(sizeof(u32) == 4);
CTAssert(sizeof(u64) == 8);

CTAssert(sizeof(ssize) == sizeof(void*));
CTAssert(sizeof(usize) == sizeof(void*));

CTAssert(sizeof(f32) == 4);
CTAssert(sizeof(f64) == 8);

// ----------------------------------------------------------
// NOTE(vak): Constants
// ----------------------------------------------------------

#define true  (1)
#define false (0)

#define S8Min  ((s8 )(-128))
#define S16Min ((s16)(-32768))
#define S32Min ((s32)(-2147483648))
#define S64Min ((s64)(-9223372036854775808ll))

#define S8Max  ((s8 )(+127))
#define S16Max ((s16)(+32767))
#define S32Max ((s32)(+2147483647))
#define S64Max ((s64)(+9223372036854775807ll))

#define U8Max  ((u8 )(+255))
#define U16Max ((u16)(+65535))
#define U32Max ((u32)(+4294967295))
#define U64Max ((u64)(+18446744073709551615ull))

#define F32Eps (((ieee754_single){.U32 = 0x34000000}).F32)
#define F32Min (((ieee754_single){.U32 = 0xFF7FFFFF}).F32)
#define F32Max (((ieee754_single){.U32 = 0x7F7FFFFF}).F32)

#define F64Eps (((ieee754_double){.U64 = 0x3CA0000000000000}).F64)
#define F64Min (((ieee754_double){.U64 = 0xFFEFFFFFFFFFFFFF}).F64)
#define F64Max (((ieee754_double){.U64 = 0x7FEFFFFFFFFFFFFF}).F64)

// ----------------------------------------------------------
// NOTE(vak): Macros
// ----------------------------------------------------------

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define OffsetOf(Structure, Member) \
    ((usize)(&(((Structure*)0)->Member)))

#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Clamp(Min, Value, Max) \
    Maximum(Min, Minimum(Max, Value))

#define Clamp01(Value) Clamp(0, Value, 1)

#define KB(Amount) ((ssize)(Amount) << 10)
#define MB(Amount) ((ssize)(Amount) << 20)
#define GB(Amount) ((ssize)(Amount) << 30)
#define TB(Amount) ((ssize)(Amount) << 40)

#define Align(Value, PowerOf2) \
    (((Value) + (PowerOf2) - 1) & ~((PowerOf2) - 1))

#define IsAligned(Value, PowerOf2) \
    (((Value) & ((PowerOf2) - 1)) == Value)

#define IsPowerOf2(Value) IsAligned(Value, Value)

// ----------------------------------------------------------
// NOTE(vak): Memory
// ----------------------------------------------------------

#define ZeroType(Pointer)         ZeroMemory(Pointer, sizeof(*(Pointer)))
#define ZeroArray(Pointer, Count) ZeroMemory(Pointer, sizeof(*(Pointer)) * (Count))

local void ZeroMemory(void* DestInit, usize Size)
{
    u8* Dest = (u8*)DestInit;

    while (Size--)
        *Dest++ = 0;
}

// ----------------------------------------------------------
// NOTE(vak): Strings
// ----------------------------------------------------------

typedef struct
{
    char* Data;
    usize Size;
} string;

#define Str(Literal) (string){Literal, sizeof(Literal) - 1}

#define StrData(Data, Size) (string){Data, Size}
