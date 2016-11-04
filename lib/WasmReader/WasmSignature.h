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

    void AllocateParams(uint32 count, Recycler * recycler);
    void SetParam(WasmTypes::WasmType type, uint32 index);
    void SetResultType(WasmTypes::WasmType type);
    void SetSignatureId(uint32 id);

    Local GetParam(uint index) const;
    WasmTypes::WasmType GetResultType() const;
    uint32 GetParamCount() const;
    uint32 GetParamSize(uint index) const;
    uint32 GetParamsSize() const;
    uint32 GetSignatureId() const;

    bool IsEquivalent(const WasmSignature* sig) const;
private:
    WasmTypes::WasmType m_resultType;
    Local* m_params;
    uint32 m_paramsCount;
    uint32 m_id;
    uint32 m_paramSize;
};

} // namespace Wasm
