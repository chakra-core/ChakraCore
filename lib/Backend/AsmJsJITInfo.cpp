//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#ifdef ASMJS_PLAT
AsmJsJITInfo::AsmJsJITInfo(AsmJsDataIDL * data) :
    m_data(*data)
{
    CompileAssert(sizeof(AsmJsJITInfo) == sizeof(AsmJsDataIDL));
}

WAsmJs::TypedSlotInfo
AsmJsJITInfo::GetTypedSlotInfo(WAsmJs::Types type) const
{
    WAsmJs::TypedSlotInfo info;
    if (type >= 0 && type < WAsmJs::LIMIT)
    {
        info.byteOffset = m_data.typedSlotInfos[type].byteOffset;
        info.constCount = m_data.typedSlotInfos[type].constCount;
        info.constSrcByteOffset = m_data.typedSlotInfos[type].constSrcByteOffset;
        info.tmpCount = m_data.typedSlotInfos[type].tmpCount;
        info.varCount = m_data.typedSlotInfos[type].varCount;
    }
    return info;
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
    AssertOrFailFast(argNum < GetArgCount());
    return GetArgTypeArray()[argNum];
}

#ifdef ENABLE_WASM
Wasm::WasmSignature *
AsmJsJITInfo::GetWasmSignature(uint index) const
{
    Assert(index < m_data.wasmSignatureCount);
    return Wasm::WasmSignature::FromIDL(&m_data.wasmSignatures[index]);
}

intptr_t
AsmJsJITInfo::GetWasmSignatureAddr(uint index) const
{
    Assert(index < m_data.wasmSignatureCount);
    return m_data.wasmSignaturesBaseAddr + index * sizeof(Wasm::WasmSignature);
}
#endif

bool
AsmJsJITInfo::UsesHeapBuffer() const
{
    return m_data.usesHeapBuffer != FALSE;
}

bool
AsmJsJITInfo::AccessNeedsBoundCheck(uint offset) const
{
    return offset >= 0x10000;
}
#endif