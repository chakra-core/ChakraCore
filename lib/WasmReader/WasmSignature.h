//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class WasmSignature
{
public:
    WasmSignature();

    void AllocateParams(Js::ArgSlot count, Recycler * recycler);
    void SetParam(WasmTypes::WasmType type, Js::ArgSlot index);
    void SetResultType(WasmTypes::WasmType type);
    void SetSignatureId(uint32 id);

    Local GetParam(Js::ArgSlot index) const;
    WasmTypes::WasmType GetResultType() const;
    Js::ArgSlot GetParamCount() const;
    Js::ArgSlot GetParamSize(Js::ArgSlot index) const;
    Js::ArgSlot GetParamsSize() const;
    void FinalizeSignature();
    uint32 GetSignatureId() const;
    size_t GetShortSig() const;

    bool IsEquivalent(const WasmSignature* sig) const;
    static WasmSignature* FromIDL(WasmSignatureIDL* sig);

    static uint32 GetOffsetOfShortSig() { return offsetof(WasmSignature, m_shortSig); }

    uint32 WriteSignatureToString(_Out_writes_(maxlen) char16 *out, uint32 maxlen);
    void Dump();
private:
    Field(WasmTypes::WasmType) m_resultType;
    Field(uint32) m_id;
    Field(Js::ArgSlot) m_paramSize;
    Field(Js::ArgSlot) m_paramsCount;
    Field(size_t) m_shortSig;
    Field(Local*) m_params;
};

} // namespace Wasm
