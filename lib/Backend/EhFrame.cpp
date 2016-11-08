//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "EhFrame.h"

// AMD64 ABI -- DWARF register number mapping
static const ubyte DWARF_RegNum[] =
{
    // Exactly same order as RegList.h!
    -1, // NOREG,
    0,  // RAX,
    2,  // RCX,
    1,  // RDX,
    3,  // RBX,
    7,  // RSP,
    6,  // RBP,
    4,  // RSI,
    5,  // RDI,
    8,  // R8,
    9,  // R9,
    10, // R10,
    11, // R11,
    12, // R12,
    13, // R13,
    14, // R14,
    15, // R15,
    17,  // XMM0,
    18,  // XMM1,
    19,  // XMM2,
    20,  // XMM3,
    21,  // XMM4,
    22,  // XMM5,
    23,  // XMM6,
    24,  // XMM7,
    25,  // XMM8,
    26,  // XMM9,
    27,  // XMM10,
    28,  // XMM11,
    29,  // XMM12,
    30,  // XMM13,
    31,  // XMM14,
    32,  // XMM15,
};

static const ubyte DWARF_RegRA = 16;

ubyte GetDwarfRegNum(ubyte regNum)
{
    return DWARF_RegNum[regNum];
}

// Encode into ULEB128 (Unsigned Little Endian Base 128)
BYTE* EmitLEB128(BYTE* pc, unsigned value)
{
    do
    {
        BYTE b = value & 0x7F; // low order 7 bits
        value >>= 7;

        if (value)  // more bytes to come
        {
            b |= 0x80;
        }

        *pc++ = b;
    }
    while (value != 0);

    return pc;
}

// Encode into signed LEB128 (Signed Little Endian Base 128)
BYTE* EmitLEB128(BYTE* pc, int value)
{
    static const int size = sizeof(value) * 8;
    static const bool isLogicShift = (-1 >> 1) != -1;

    const bool signExtend = isLogicShift && value < 0;

    bool more = true;
    while (more)
    {
        BYTE b = value & 0x7F; // low order 7 bits
        value >>= 7;

        if (signExtend)
        {
            value |= - (1 << (size - 7)); // sign extend
        }

        const bool signBit = (b & 0x40) != 0;
        if ((value == 0 && !signBit) || (value == -1 && signBit))
        {
            more = false;
        }
        else
        {
            b |= 0x80;
        }

        *pc++ = b;
    }

    return pc;
}


void EhFrame::Entry::Begin()
{
    Assert(beginOffset == -1);
    beginOffset = writer->Count();

    // Write Length place holder
    const uword length = 0;
    writer->Write(length);
}

void EhFrame::Entry::End()
{
    Assert(beginOffset != -1); // Must have called Begin()

    // padding
    size_t padding = (MachPtr - writer->Count() % MachPtr) % MachPtr;
    for (size_t i = 0; i < padding; i++)
    {
        cfi_nop();
    }

    // update length record
    uword length = writer->Count() - beginOffset
                    - sizeof(length);  // exclude length itself
    writer->Write(beginOffset, length);
}

void EhFrame::Entry::cfi_advance(uword advance)
{
    if (advance <= 0x3F)        // 6-bits
    {
        cfi_advance_loc(static_cast<ubyte>(advance));
    }
    else if (advance <= 0xFF)   // 1-byte
    {
        cfi_advance_loc1(static_cast<ubyte>(advance));
    }
    else if (advance <= 0xFFFF) // 2-byte
    {
        cfi_advance_loc2(static_cast<uword>(advance));
    }
    else                        // 4-byte
    {
        cfi_advance_loc4(advance);
    }
}

void EhFrame::CIE::Begin()
{
    Assert(writer->Count() == 0);
    Entry::Begin();

    const uword cie_id = 0;
    Emit(cie_id);

    const ubyte version = 1;
    Emit(version);

    const ubyte augmentationString = 0; // none
    Emit(augmentationString);

    const ULEB128 codeAlignmentFactor = 1;
    Emit(codeAlignmentFactor);

    const LEB128 dataAlignmentFactor = - MachPtr;
    Emit(dataAlignmentFactor);

    const ubyte returnAddressRegister = DWARF_RegRA;
    Emit(returnAddressRegister);
}


void EhFrame::FDE::Begin()
{
    Entry::Begin();

    const uword cie_id = writer->Count();
    Emit(cie_id);

    // Write pc <begin, range> placeholder
    pcBeginOffset = writer->Count();
    const void* pc = nullptr;
    Emit(pc);
    Emit(pc);
}

void EhFrame::FDE::UpdateAddressRange(const void* pcBegin, size_t pcRange)
{
    writer->Write(pcBeginOffset, pcBegin);
    writer->Write(pcBeginOffset + sizeof(pcBegin),
        reinterpret_cast<const void*>(pcRange));
}


EhFrame::EhFrame(BYTE* buffer, size_t size)
        : writer(buffer, size), fde(&writer)
{
    CIE cie(&writer);
    cie.Begin();

    // CIE initial instructions
    // DW_CFA_def_cfa: r7 (rsp) ofs 8
    cie.cfi_def_cfa(DWARF_RegNum[LowererMDArch::GetRegStackPointer()], MachPtr);
    // DW_CFA_offset: r16 (rip) at cfa-8 (data alignment -8)
    cie.cfi_offset(DWARF_RegRA, 1);

    cie.End();

    fde.Begin();
}

void EhFrame::End()
{
    fde.End();

    // Write length 0 to mark terminate entry
    const uword terminate_entry_length = 0;
    writer.Write(terminate_entry_length);
}
