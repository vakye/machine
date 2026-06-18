
// NOTE(vak): Printing functions for characters, strings,
// integers, ...

#pragma once

// ----------------------------------------------------------
// NOTE(vak): Interface
// ----------------------------------------------------------

typedef usize print_write_bytes(
    void* UserData,
    void* Bytes,
    usize Size
);

typedef struct
{
    print_write_bytes* WriteBytes;
    void*              UserData;
} print_output;

// NOTE(vak): Character

local usize PrintCharacter(print_output Out, char Character);
local usize PrintNewLine(print_output Out);

// NOTE(vak): String

local usize Print(print_output Out, string Message);
local usize Println(print_output Out, string Message);

// NOTE(vak): Boolean

#define PrintBool(Out, Value) Print(Out, (Value) ? Str("true") : Str("false"))

// NOTE(vak): Integer
//
// Use integer printing functions like this:
//     PrintUInt(Out, 23871);
//
// Additional settings can be supplied by setting print_int_settings members
// like this:
//     PrintUInt(Out, 23871, .Padding = 10, .LeftPad = true);
//
// For more settings, see print_int_settings structure below

#define PrintSInt(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Base = 10, .Signed = true })
#define PrintUInt(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Base = 10})

#define PrintBin(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Prefix = true, .Base = 2 })
#define PrintOct(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Prefix = true, .Base = 8 })
#define PrintHex(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Prefix = true, .Base = 16})

#define PrintRawBin(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Base = 2 })
#define PrintRawOct(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Base = 8 })
#define PrintRawHex(Out, ...) PrintInt(Out, (print_int_settings){(ssize)__VA_ARGS__, .Base = 16})

typedef struct
{
    // NOTE(vak): Value should always be first
    // as the first argument of any integer
    // printing function is a value.

    ssize Value;

    // NOTE(vak): Options

    u64   Padding   : 32;
    u64   Base      : 16;

    b64   LeftPad   : 1; // NOTE(vak): Prints padding before integer
    b64   ZeroPad   : 1; // NOTE(vak): Pads using '0' instead of ' '
    b64   ForceSign : 1; // NOTE(vak): If positive, prints '+'
    b64   BlankSign : 1; // NOTE(vak): If positive, prints ' '
    b64   Prefix    : 1; // NOTE(vak): Print prefixes: '0b' (bin), '0o' (oct), '0x' (hex)
    b64   Upper     : 1; // NOTE(vak): Enable uppercase letters for hexadecimal

    // NOTE(vak): Internal

    b64   Signed    : 1; // NOTE(vak): Intepret 'Value' as a signed integer
} print_int_settings;

local usize PrintInt(print_output Out, print_int_settings Settings);

// ----------------------------------------------------------
// NOTE(vak): Implementation
// ----------------------------------------------------------

local usize PrintCharacter(print_output Out, char Character)
{
    usize Result = Out.WriteBytes(Out.UserData, &Character, 1);
    return (Result);
}

local usize PrintNewLine(print_output Out)
{
    usize Result = PrintCharacter(Out, '\n');
    return (Result);
}

local usize Print(print_output Out, string Message)
{
    usize Result = Out.WriteBytes(Out.UserData, Message.Data, Message.Size);
    return (Result);
}

local usize Println(print_output Out, string Message)
{
    usize Result = 0;

    Result += Print(Out, Message);
    Result += PrintNewLine(Out);

    return (Result);
}

local usize PrintInt(print_output Out, print_int_settings Settings)
{
    // NOTE(vak): Setup

    usize Value   = Settings.Value;
    usize Base    = Settings.Base;
    usize Padding = Settings.Padding;

    b32 LeftPad   = (b32) Settings.LeftPad;
    b32 ZeroPad   = (b32) Settings.ZeroPad;
    b32 ForceSign = (b32) Settings.ForceSign;
    b32 BlankSign = (b32) Settings.BlankSign;
    b32 Prefix    = (b32) Settings.Prefix;
    b32 Upper     = (b32) Settings.Upper;
    b32 Signed    = (b32) Settings.Signed;

    b32 IsNegative = (Signed) && (Value >> 63);

    if (IsNegative)
        Value = ~Value + 1; // NOTE(vak): Twos complement

    // NOTE(vak): Safety
    {
        if ((Base < 2) || (Base > 16))
            Base = 10;
    }

    // NOTE(vak): Prefix

    string PrefixString = Str("");

    if (Prefix)
    {
        switch (Base)
        {
            case 2:  { PrefixString = Str("0b"); } break;
            case 8:  { PrefixString = Str("0o"); } break;
            case 16: { PrefixString = Str("0x"); } break;
        }
    }

    // NOTE(vak): Conversion

    char Buffer[64] = {0};
    usize DigitCount = 0;
    usize DigitIndex = ArrayCount(Buffer);

    {
        char LowerDigits[] = "0123456789abcdef";
        char UpperDigits[] = "0123456789ABCDEF";

        char* DigitMap = (Upper) ? UpperDigits : LowerDigits;

        do
        {
            char Digit = DigitMap[Value % Base];

            Value /= Base;

            DigitCount++;
            DigitIndex--;

            Buffer[DigitIndex] = Digit;
        } while (Value);
    }

    string Digits = StrData(Buffer + DigitIndex, DigitCount);

    // NOTE(vak): Output

    usize BytesWritten  = 0;
    {
        usize BytesToWrite = Digits.Size + PrefixString.Size;

        if (IsNegative)
            BytesToWrite++;
        else if (ForceSign || BlankSign)
            BytesToWrite++;

        if (LeftPad)
        {
            for (usize Index = BytesToWrite; Index < Padding; Index++)
                BytesWritten += PrintCharacter(Out, ZeroPad ? '0' : ' ');
        }

        if (IsNegative)
            BytesWritten += PrintCharacter(Out, '-');
        else if (ForceSign)
            BytesWritten += PrintCharacter(Out, '+');
        else if (BlankSign)
            BytesWritten += PrintCharacter(Out, ' ');

        BytesWritten += Print(Out, PrefixString);
        BytesWritten += Print(Out, Digits);

        if (!LeftPad)
        {
            for (usize Index = BytesToWrite; Index < Padding; Index++)
                BytesWritten += PrintCharacter(Out, ZeroPad ? '0' : ' ');
        }
    }

    return (BytesWritten);
}
