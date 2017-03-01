//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    struct ByteCodeReader
    {
        static uint32 GetStartLocationOffset() { return offsetof(ByteCodeReader, m_startLocation); }
        static uint32 GetCurrentLocationOffset() { return offsetof(ByteCodeReader, m_currentLocation); }

    private:
        // TODO: (leish)(swb) this is not always stack allocated now
        // with ES6 Generator, this can be allocated with recycler
        // need to find a good way to set write barrier, or big refactor.
        const byte * m_startLocation;
        const byte * m_currentLocation;

#if DBG
        const byte * m_endLocation;
#endif

    public:
        void Create(FunctionBody* functionRead, uint startOffset = 0);
        void Create(FunctionBody* functionRead, uint startOffset, bool useOriginalByteCode);
#if DBG
        void Create(const byte * byteCodeStart, uint startOffset, uint byteCodeLength);
#else
        void Create(const byte * byteCodeStart, uint startOffset);
#endif
        uint GetCurrentOffset() const;
        const byte * SetCurrentOffset(int byteOffset);
        const byte * SetCurrentRelativeOffset(const byte * ip, int byteOffset);

        template<typename LayoutType> inline const unaligned LayoutType * GetLayout();
        template<typename LayoutType> inline const unaligned LayoutType * GetLayout(const byte*& ip);

        // Read*Op advance the IP,
        // Peek*Op doesn't move the IP
        // *ByteOp only read one byte of the opcode,
        // *Op interprets and remove the large layout prefix
    private:
        OpCode ReadOp(const byte *&ip, LayoutSize& layoutSize) const;
        OpCode ReadPrefixedOp(const byte *&ip, LayoutSize& layoutSize, OpCode prefix) const;
    public:
        OpCode ReadOp(LayoutSize& layoutSize);
        OpCodeAsmJs ReadAsmJsOp(LayoutSize& layoutSize);
        OpCode ReadPrefixedOp(LayoutSize& layoutSize, OpCode prefix);
        OpCode PeekOp(LayoutSize& layoutSize) const;
        OpCode PeekOp() const { LayoutSize layoutSize; return PeekOp(layoutSize); }
        OpCode PeekOp(const byte * ip, LayoutSize& layoutSize);

        static OpCode PeekByteOp(const byte*  ip);
        static OpCode ReadByteOp(const byte*& ip);
        static OpCode PeekExtOp(const byte*  ip);
        static OpCode ReadExtOp(const byte*& ip);

        // Declare reading functions
#define LAYOUT_TYPE(layout) \
        const unaligned OpLayout##layout* layout(); \
        const unaligned OpLayout##layout* layout(const byte*& ip);
#include "LayoutTypes.h"

#ifdef ASMJS_PLAT
#define LAYOUT_TYPE(layout) \
        const unaligned OpLayout##layout* layout(); \
        const unaligned OpLayout##layout* layout(const byte*& ip);
#define EXCLUDE_DUP_LAYOUT
#include "LayoutTypesAsmJs.h"
#endif

        template <typename T>
        static AuxArray<T> const * ReadAuxArray(uint offset, FunctionBody * functionBody);
        template <typename T>
        static AuxArray<T> const * ReadAuxArrayWithLock(uint offset, FunctionBody * functionBody);
        static PropertyIdArray const * ReadPropertyIdArray(uint offset, FunctionBody * functionBody);
        static PropertyIdArray const * ReadPropertyIdArrayWithLock(uint offset, FunctionBody * functionBody);
        static VarArrayVarCount const * ReadVarArrayVarCount(uint offset, FunctionBody * functionBody);
        static VarArrayVarCount const * ReadVarArrayVarCountWithLock(uint offset, FunctionBody * functionBody);

        const byte* GetIP();
        void SetIP(const byte *const ip);

        template<class T> const unaligned T* AuxiliaryContext(const byte*& ip, const byte ** content);

#if DBG_DUMP
        byte GetRawByte(int i);
#endif
    };

    template<typename LayoutType>
    inline const unaligned LayoutType * ByteCodeReader::GetLayout()
    {
        size_t layoutSize = sizeof(LayoutType);

        AssertMsg((layoutSize > 0) && (layoutSize < 100), "Ensure valid layout size");

        const byte * layoutData = m_currentLocation;
        m_currentLocation += layoutSize;

        Assert(m_currentLocation <= m_endLocation);

        return reinterpret_cast<const unaligned LayoutType *>(layoutData);
    }

    template<>
    inline const unaligned OpLayoutEmpty * ByteCodeReader::GetLayout<OpLayoutEmpty>()
    {
        return nullptr;
    }

} // namespace Js
