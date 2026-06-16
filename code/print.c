
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

local usize PrintCharacter(print_output Out, char Character);
local usize PrintNewLine(print_output Out);

local usize Print(print_output Out, string Message);
local usize Println(print_output Out, string Message);

#define PrintBool(Out, Value) Print(Out, (Value) ? Str("true") : Str("false"))

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
    ssize Value;
    u16   Base;
    u16   Padding;

    // NOTE(vak): Options

    b32   LeftPad   : 1;
    b32   ZeroPad   : 1;
    b32   ForceSign : 1;
    b32   BlankSign : 1;
    b32   Prefix    : 1;
    b32   Upper     : 1;

    // NOTE(vak): Internal

    b32   Signed    : 1;
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

    b32 LeftPad   = Settings.LeftPad;
    b32 ZeroPad   = Settings.ZeroPad;
    b32 ForceSign = Settings.ForceSign;
    b32 BlankSign = Settings.BlankSign;
    b32 Prefix    = Settings.Prefix;
    b32 Upper     = Settings.Upper;
    b32 Signed    = Settings.Signed;

    b32 IsNegative = (Signed) && (Value >> 63);

    if (IsNegative)
        Value = ~Value + 1; // NOTE(vak): Twos complement

    // NOTE(vak): Checks
    {
        AlwaysAssert((Base >= 2) && (Base <= 16));
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

    // NOTE(vak): Translation to digits

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

    // NOTE(vak): Output

    usize BytesWritten  = 0;
    {
        usize BytesToWrite = DigitCount + PrefixString.Size;

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
        BytesWritten += Print(Out, StrData(Buffer + DigitIndex, DigitCount));

        if (!LeftPad)
        {
            for (usize Index = BytesToWrite; Index < Padding; Index++)
                BytesWritten += PrintCharacter(Out, ZeroPad ? '0' : ' ');
        }
    }

    return (BytesWritten);
}
