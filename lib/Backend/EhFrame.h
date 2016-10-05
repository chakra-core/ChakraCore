//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

typedef BYTE ubyte;
typedef uint16 uhalf;
typedef uint32 uword;
CompileAssert(sizeof(ubyte) == 1);
CompileAssert(sizeof(uhalf) == 2);
CompileAssert(sizeof(uword) == 4);

BYTE* EmitLEB128(BYTE* pc, unsigned value);
BYTE* EmitLEB128(BYTE* pc, int value);
ubyte GetDwarfRegNum(ubyte regNum);

template <class T>
class LEB128Wrapper
{
private:
    T value;

public:
    LEB128Wrapper(T value): value(value)
    {}

    BYTE* Write(BYTE* pc) const
    {
        return EmitLEB128(pc, value);
    }
};

typedef LEB128Wrapper<unsigned> ULEB128;
typedef LEB128Wrapper<int> LEB128;

//
// EhFrame emits .eh_frame unwind data for our JIT code. We emit only one CIE
// followed by one FDE for each JIT function.
//
class EhFrame
{
    // Simple buffer writer. Must operate on a buffer of sufficient size.
    class Writer
    {
    private:
        BYTE* buffer;   // original buffer head
        BYTE* cur;      // current output position
        const size_t size;  // original size of buffer, for debug only

    public:
        Writer(BYTE* buffer, size_t size) : buffer(buffer), cur(buffer), size(size)
        {}

        // Write a value, and advance cur position
        template <class T>
        void Write(T value)
        {
            *reinterpret_cast<T*>(cur) = value;
            cur += sizeof(value);
            Assert(Count() <= size);
        }

        // Write a ULEB128 or LEB128 value, and advance cur position
        template <class T>
        void Write(const LEB128Wrapper<T>& leb128)
        {
            cur = leb128.Write(cur);
            Assert(Count() <= size);
        }

        // Write a value at an absolute position
        template <class T>
        void Write(size_t offset, T value)
        {
            Assert(offset + sizeof(value) <= size);
            *reinterpret_cast<T*>(buffer + offset) = value;
        }

        // Get original buffer head
        BYTE* Buffer() const
        {
            return buffer;
        }

        // Get count of written bytes (== offset of cur position)
        size_t Count() const
        {
            return cur - buffer;
        }
    };

    // Base class for CIE and FDE
    class Entry
    {
    protected:
        Writer* writer;
        size_t  beginOffset;    // where we'll update "length" record

        // To limit supported value types
        void Emit(ubyte value) { writer->Write(value); }
        void Emit(uhalf value) { writer->Write(value); }
        void Emit(uword value) { writer->Write(value); }
        void Emit(const void* absptr) { writer->Write(absptr); }
        void Emit(LEB128 value) { writer->Write(value); }
        void Emit(ULEB128 value) { writer->Write(value); }

        template <class T1>
        void Emit(ubyte op, T1 arg1)
        {
            Emit(op);
            Emit(arg1);
        }

        template <class T1, class T2>
        void Emit(ubyte op, T1 arg1, T2 arg2)
        {
            Emit(op, arg1);
            Emit(arg2);
        }

    public:
        Entry(Writer* writer) : writer(writer), beginOffset(-1)
        {}

        void Begin();
        void End();

#define ENTRY(name, op) \
    void cfi_##name() \
    { Emit(static_cast<ubyte>(op)); }

#define ENTRY1(name, op, arg1_type) \
    void cfi_##name(arg1_type arg1) \
    { Emit(op, arg1); }

#define ENTRY2(name, op, arg1_type, arg2_type) \
    void cfi_##name(arg1_type arg1, arg2_type arg2) \
    { Emit(op, arg1, arg2); }

#define ENTRY_SM1(name, op, arg1_type) \
    void cfi_##name(arg1_type arg1) \
    { Assert((arg1) <= 0x3F); Emit(static_cast<ubyte>((op) | arg1)); }

#define ENTRY_SM2(name, op, arg1_type, arg2_type) \
    void cfi_##name(arg1_type arg1, arg2_type arg2) \
    { Assert((arg1) <= 0x3F); Emit((op) | arg1, arg2); }

#include "EhFrameCFI.inc"

        void cfi_advance(uword advance);
    };

    // Common Information Entry
    class CIE : public Entry
    {
    public:
        CIE(Writer* writer) : Entry(writer)
        {}

        void Begin();
    };

    // Frame Description Entry
    class FDE: public Entry
    {
    private:
        size_t pcBeginOffset;

    public:
        FDE(Writer* writer) : Entry(writer)
        {}

        void Begin();
        void UpdateAddressRange(const void* pcBegin, size_t pcRange);
    };

private:
    Writer writer;
    FDE fde;

public:
    EhFrame(BYTE* buffer, size_t size);

    Writer* GetWriter()
    {
        return &writer;
    }

    FDE* GetFDE()
    {
        return &fde;
    }

    void End();

    BYTE* Buffer() const
    {
        return writer.Buffer();
    }

    size_t Count() const
    {
        return writer.Count();
    }
};
