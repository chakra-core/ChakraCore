//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Security
{
private:
    Func *func;
    IR::RegOpnd * basePlusCookieOpnd;
    IR::IntConstOpnd * cookieOpnd;
    IR::RegOpnd * baseOpnd;

public:
    Security(Func * func) : func(func), basePlusCookieOpnd(nullptr), cookieOpnd(nullptr), baseOpnd(nullptr) { }

    void            EncodeLargeConstants();
    void            InsertNOPs();
    static bool     DontEncode(IR::Opnd *opnd);
    static void     InsertRandomFunctionPad(IR::Instr * instrBeforeInstr);

private:
    bool            EncodeOpnd(IR::Instr *instr, IR::Opnd *opnd);
    uint            CalculateConstSize(IR::Opnd *opnd);
    IntConstType    EncodeValue(IR::Instr *instr, IR::Opnd *opnd, IntConstType constValue, _Out_ IR::RegOpnd ** pNewOpnd);
#if TARGET_64
    size_t          EncodeAddress(IR::Instr *instr, IR::Opnd *opnd, size_t value, _Out_ IR::RegOpnd **pNewOpnd);
#endif
    static IR::IntConstOpnd * BuildCookieOpnd(IRType type, Func * func);

    template<typename T> static uint GetByteCount(T value)
    {
        uint byteCount = 0;
        for (uint i = 0; i < sizeof(T); ++i)
        {
            if (IsByteSet(value, i))
            {
                ++byteCount;
            }
        }
        return byteCount;
    }

    template<typename T> static bool IsByteSet(T value, uint32 index)
    {
        const byte byteValue = (byte)(value >> (index * MachBits));
        return byteValue != 0 && byteValue != 0xFF;
    }

    void            InsertNOPBefore(IR::Instr *instr);
    int             GetNextNOPInsertPoint();

    // Insert 1-4 bytes of NOPs
    static void     InsertSmallNOP(IR::Instr * instrBeforeInstr, DWORD nopSize);
};
