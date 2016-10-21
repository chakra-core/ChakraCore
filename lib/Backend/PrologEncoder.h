//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define IS_UNIBBLE(x) (!(((size_t)x) & ~((size_t)0xF)))
#define TO_UNIBBLE(x) (x & 0xF)
#define IS_UINT16(x)  (!(((size_t)x) & ~((size_t)0xFFFF)))
#define TO_UINT16(x)  (x & 0xFFFF)
#define IS_UINT32(x)  (!(((size_t)x) & ~((size_t)0xFFFFFFFF)))
#define TO_UINT32(x)  (x & 0xFFFFFFFF)

enum UnwindOp : unsigned __int8 {
    UWOP_IGNORE      =  (unsigned __int8)-1,
    UWOP_PUSH_NONVOL =  0,
    UWOP_ALLOC_LARGE =  1,
    UWOP_ALLOC_SMALL =  2,
    UWOP_SAVE_XMM128 =  8
};

#ifdef _WIN32
// ----------------------------------------------------------------------------
//  _WIN32 x64 unwind uses PDATA
// ----------------------------------------------------------------------------

class PrologEncoder
{
private:

#pragma pack(push, 1)
    struct UNWIND_CODE
    {
        /*
         * ntdll!UNWIND_CODE
         *    +0x000 CodeOffset       : UChar
         *    +0x001 UnwindOp         : Pos 0, 4 Bits
         *    +0x001 OpInfo           : Pos 4, 4 Bits
         *    +0x000 FrameOffset      : Uint2B
         */
        union {
            struct {
                unsigned __int8 CodeOffset;
                unsigned __int8 UnwindOp : 4;
                unsigned __int8 OpInfo   : 4;
            };
            unsigned __int16    FrameOffset;
        };

        void SetOffset(unsigned __int8 offset)
        {
            CodeOffset = offset;
        }

        void SetOp(unsigned __int8 op)
        {
            Assert(IS_UNIBBLE(op));
            UnwindOp = TO_UNIBBLE(op);
        }

        void SetOpInfo(unsigned __int8 info)
        {
            Assert(IS_UNIBBLE(info));
            OpInfo = TO_UNIBBLE(info);
        }
    };


    struct UNWIND_INFO
    {
        /*
         * ntdll!UNWIND_INFO
         *    +0x000 Version          : Pos 0, 3 Bits
         *    +0x000 Flags            : Pos 3, 5 Bits
         *    +0x001 SizeOfProlog     : UChar
         *    +0x002 CountOfCodes     : UChar
         *    +0x003 FrameRegister    : Pos 0, 4 Bits
         *    +0x003 FrameOffset      : Pos 4, 4 Bits
         *    +0x004 UnwindCode       : [1] _UNWIND_CODE
         */
        unsigned __int8 Version : 3;
        unsigned __int8 Flags   : 5;
        unsigned __int8 SizeOfProlog;
        unsigned __int8 CountOfCodes;
        unsigned __int8 FrameRegister : 4;
        unsigned __int8 FrameOffset : 4;
        UNWIND_CODE     unwindCodes[0];
    };

    struct UnwindCode : public UNWIND_CODE
    {
    private:
        union {
            unsigned __int16 uint16Val;
            unsigned __int32 uint32Val;
        } u;

    public:
        void SetValue(unsigned __int16 value)
        {
            Assert(UnwindOp && (UnwindOp == UWOP_ALLOC_LARGE));
            u.uint16Val = value;
        }

        void SetValue(unsigned __int32 value)
        {
            Assert(UnwindOp && (UnwindOp == UWOP_ALLOC_LARGE));

            // insert assert to check that value actually warrants the use of
            // 32 bits to encoder.
            u.uint32Val = value;
        }
    };

    struct PData
    {
        RUNTIME_FUNCTION runtimeFunction;
        UNWIND_INFO      unwindInfo;
    };
#pragma pack(pop)

    static const unsigned __int8   MaxRequiredUnwindCodeNodeCount = 34;
    static const size_t MaxPDataSize = sizeof(PData) + (sizeof(UNWIND_CODE) * MaxRequiredUnwindCodeNodeCount);

    union
    {
        PData         pdata;
        BYTE          pdataBuffer[MaxPDataSize];
    };
    unsigned __int8   currentUnwindCodeNodeIndex;
    unsigned __int8   requiredUnwindCodeNodeCount;
    unsigned __int8   currentInstrOffset;



public:
    PrologEncoder()
        : requiredUnwindCodeNodeCount(0),
          currentUnwindCodeNodeIndex(0),
          currentInstrOffset(0)
    {
    }

    void RecordNonVolRegSave();
    void RecordXmmRegSave();
    void RecordAlloca(size_t size);
    void EncodeInstr(IR::Instr *instr, unsigned __int8 size);
    void EncodeSmallProlog(uint8 prologSize, size_t size);

    //
    // Pre-Win8 PDATA registration.
    //
    DWORD SizeOfPData();
    BYTE *Finalize(BYTE *functionStart,
                        DWORD codeSize,
                        BYTE *pdataBuffer);
    //
    // Win8 PDATA registration.
    //
    void Begin(size_t prologStartOffset) {}  // No op on _WIN32
    void End() {}  // No op on _WIN32
    DWORD SizeOfUnwindInfo();
    BYTE *GetUnwindInfo();
    void FinalizeUnwindInfo(BYTE *functionStart, DWORD codeSize);
private:
    UnwindCode *GetUnwindCode(unsigned __int8 nodeCount);

};

#else  // !_WIN32
// ----------------------------------------------------------------------------
//  !_WIN32 x64 unwind uses .eh_frame
// ----------------------------------------------------------------------------
#include "EhFrame.h"

class PrologEncoder
{
public:
    static const int SMALL_EHFRAME_SIZE = 0x40;
    static const int JIT_EHFRAME_SIZE = 0x80;

private:
    EhFrame ehFrame;
    BYTE buffer[JIT_EHFRAME_SIZE];

    size_t cfiInstrOffset;      // last cfi emit instr offset
    size_t currentInstrOffset;  // current instr offset
                                // currentInstrOffset - cfiInstrOffset == advance
    unsigned cfaWordOffset;

public:
    PrologEncoder()
        :ehFrame(buffer, JIT_EHFRAME_SIZE),
          cfiInstrOffset(0), currentInstrOffset(0), cfaWordOffset(1)
    {}

    void RecordNonVolRegSave() {}
    void RecordXmmRegSave() {}
    void RecordAlloca(size_t size) {}
    void EncodeInstr(IR::Instr *instr, uint8 size);

    void EncodeSmallProlog(uint8 prologSize, size_t size);
    DWORD SizeOfPData();
    BYTE *Finalize(BYTE *functionStart, DWORD codeSize, BYTE *pdataBuffer);

    void Begin(size_t prologStartOffset);
    void End();
    DWORD SizeOfUnwindInfo() { return SizeOfPData(); }
    BYTE *GetUnwindInfo() { return ehFrame.Buffer(); }
    void FinalizeUnwindInfo(BYTE *functionStart, DWORD codeSize);
};

#endif  // !_WIN32
