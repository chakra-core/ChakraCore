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
    WasmSignature(ArenaAllocator * alloc);

    void AddParam(WasmTypes::WasmType type);
    void SetResultType(WasmTypes::WasmType type);
    void SetSignatureId(uint32 id);

    WasmTypes::WasmType GetParam(uint index) const;
    WasmTypes::WasmType GetResultType() const;
    uint32 GetParamCount() const;
    uint32 GetParamSize() const;
    uint32 GetSignatureId() const;

private:
    WasmTypes::WasmType m_resultType;
    WasmTypeArray * m_params;
    ArenaAllocator * m_alloc;
    uint32 m_id;
    uint32 m_paramSize;
};

} // namespace Wasm
