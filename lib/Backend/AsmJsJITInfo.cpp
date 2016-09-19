//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

AsmJsJITInfo::AsmJsJITInfo(AsmJsDataIDL * data) :
    m_data(*data)
{
    CompileAssert(sizeof(AsmJsJITInfo) == sizeof(AsmJsDataIDL));
}

int
AsmJsJITInfo::GetIntConstCount() const
{
    return m_data.intConstCount;
}

int
AsmJsJITInfo::GetDoubleConstCount() const
{
    return m_data.doubleConstCount;
}

int
AsmJsJITInfo::GetFloatConstCount() const
{
    return m_data.floatConstCount;
}

int
AsmJsJITInfo::GetSimdConstCount() const
{
    return m_data.simdConstCount;
}

int
AsmJsJITInfo::GetIntTmpCount() const
{
    return m_data.intTmpCount;
}

int
AsmJsJITInfo::GetDoubleTmpCount() const
{
    return m_data.doubleTmpCount;
}

int
AsmJsJITInfo::GetFloatTmpCount() const
{
    return m_data.floatTmpCount;
}

int
AsmJsJITInfo::GetSimdTmpCount() const
{
    return m_data.simdTmpCount;
}

int
AsmJsJITInfo::GetIntVarCount() const
{
    return m_data.intVarCount;
}

int
AsmJsJITInfo::GetDoubleVarCount() const
{
    return m_data.doubleVarCount;
}

int
AsmJsJITInfo::GetFloatVarCount() const
{
    return m_data.floatVarCount;
}

int
AsmJsJITInfo::GetSimdVarCount() const
{
    return m_data.simdVarCount;
}

int
AsmJsJITInfo::GetIntByteOffset() const
{
    return m_data.intByteOffset;
}

int
AsmJsJITInfo::GetDoubleByteOffset() const
{
    return m_data.doubleByteOffset;
}

int
AsmJsJITInfo::GetFloatByteOffset() const
{
    return m_data.floatByteOffset;
}

int
AsmJsJITInfo::GetSimdByteOffset() const
{
    return m_data.simdByteOffset;
}

int
AsmJsJITInfo::GetTotalSizeInBytes() const
{
    return m_data.totalSizeInBytes;
}

Js::ArgSlot
AsmJsJITInfo::GetArgCount() const
{
    return m_data.argCount;
}

Js::ArgSlot
AsmJsJITInfo::GetArgByteSize() const
{
    return m_data.argByteSize;
}

Js::AsmJsRetType::Which
AsmJsJITInfo::GetRetType() const
{
    return static_cast<Js::AsmJsRetType::Which>(m_data.retType);
}

Js::AsmJsVarType::Which *
AsmJsJITInfo::GetArgTypeArray() const
{
    return reinterpret_cast<Js::AsmJsVarType::Which *>(m_data.argTypeArray);
}

Js::AsmJsVarType::Which
AsmJsJITInfo::GetArgType(Js::ArgSlot argNum) const
{
    Assert(argNum < GetArgCount());
    return GetArgTypeArray()[argNum];
}

bool
AsmJsJITInfo::IsHeapBufferConst() const
{
    return m_data.isHeapBufferConst != FALSE;
}

bool
AsmJsJITInfo::UsesHeapBuffer() const
{
    return m_data.usesHeapBuffer != FALSE;
}

bool
AsmJsJITInfo::AccessNeedsBoundCheck(uint offset) const
{
    // Normally, heap has min size of 0x10000, but if you use ChangeHeap, min heap size is increased to 0x1000000
    return offset >= 0x1000000 || (IsHeapBufferConst() && offset >= 0x10000);
}
