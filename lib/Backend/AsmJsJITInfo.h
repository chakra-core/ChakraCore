//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class AsmJsJITInfo
{
#ifdef ASMJS_PLAT
public:
    AsmJsJITInfo(AsmJsDataIDL * data);

    WAsmJs::TypedSlotInfo GetTypedSlotInfo(WAsmJs::Types type) const;
#define TYPED_SLOT_INFO_GETTER(name, type) \
    int Get##name##ByteOffset() const   { return m_data.typedSlotInfos[WAsmJs::##type].byteOffset; }\
    int Get##name##ConstCount() const   { return m_data.typedSlotInfos[WAsmJs::##type].constCount; }\
    int Get##name##TmpCount() const     { return m_data.typedSlotInfos[WAsmJs::##type].tmpCount; }\
    int Get##name##VarCount() const     { return m_data.typedSlotInfos[WAsmJs::##type].varCount; }

    TYPED_SLOT_INFO_GETTER(Double, FLOAT64);
    TYPED_SLOT_INFO_GETTER(Float, FLOAT32);
    TYPED_SLOT_INFO_GETTER(Int, INT32);
    TYPED_SLOT_INFO_GETTER(Int64, INT64);
    TYPED_SLOT_INFO_GETTER(Simd, SIMD);

    int GetTotalSizeInBytes() const;
    Js::ArgSlot GetArgCount() const;
    Js::ArgSlot GetArgByteSize() const;
    Js::AsmJsRetType::Which GetRetType() const;
    Js::AsmJsVarType::Which GetArgType(Js::ArgSlot argNum) const;

#ifdef ENABLE_WASM
    Wasm::WasmSignature * GetWasmSignature(uint index) const;
    intptr_t GetWasmSignatureAddr(uint index) const;
#endif

    bool UsesHeapBuffer() const;
    bool AccessNeedsBoundCheck(uint offset) const;

private:
    Js::AsmJsVarType::Which * GetArgTypeArray() const;
    AsmJsDataIDL m_data;
#endif
};
