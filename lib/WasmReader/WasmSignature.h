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

    void SetSignatureId(uint32 id);

    void AllocateParams(Js::ArgSlot count, Recycler * recycler);
    void SetParam(WasmTypes::WasmType type, Js::ArgSlot index);
    Local GetParam(Js::ArgSlot index) const;
    Js::ArgSlot GetParamCount() const;
    Js::ArgSlot GetParamSize(Js::ArgSlot index) const;
    Js::ArgSlot GetParamsSize() const;

    void AllocateResults(uint32 count, Recycler * recycler);
    void SetResult(Local type, uint32 index);
    uint32 GetResultCount() const { return m_resultsCount; }
    Local GetResult(uint32 index) const;

    void FinalizeSignature();
    uint32 GetSignatureId() const;
    size_t GetShortSig() const;

    template<bool useShortSig = true>
    bool IsEquivalent(const WasmSignature* sig) const;
    static WasmSignature* FromIDL(WasmSignatureIDL* sig);

    static uint32 GetOffsetOfShortSig() { return offsetof(WasmSignature, m_shortSig); }

    uint32 WriteSignatureToString(_Out_writes_(maxlen) char16 *out, uint32 maxlen);
    void Dump(uint32 maxlen = 512);
private:
    Field(uint32) m_id;
    Field(uint32) m_resultsCount = 0;
    Field(Js::ArgSlot) m_paramSize;
    Field(Js::ArgSlot) m_paramsCount;
    Field(size_t) m_shortSig;
    Field(Local*) m_params;
    Field(Local*) m_results = nullptr;
};

} // namespace Wasm
