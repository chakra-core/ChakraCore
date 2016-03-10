//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    class WasmFunctionInfo
    {
    public:
        WasmFunctionInfo(ArenaAllocator * alloc);

        void AddLocal(WasmTypes::WasmType type, uint count = 1);
        template <typename T> void AddConst(T constVal, Js::RegSlot reg);

        WasmTypes::WasmType GetLocal(uint index) const;
        WasmTypes::WasmType GetParam(uint index) const;
        template <typename T> Js::RegSlot GetConst(T constVal) const;
        WasmTypes::WasmType GetResultType() const;

        uint32 GetLocalCount() const;
        uint32 GetParamCount() const;

        void SetName(LPCUTF8 name);
        LPCUTF8 GetName() const;
        void SetModuleName(LPCUTF8 name);
        LPCUTF8 GetModuleName() const;

        void SetNumber(UINT32 number);
        UINT32 GetNumber() const;
        void SetSignature(WasmSignature * signature);
        WasmSignature * GetSignature() const;

        void SetExitLabel(Js::ByteCodeLabel label);
        Js::ByteCodeLabel GetExitLabel() const;
    private:

        // TODO: need custom comparator so -0 != 0
        template <typename T>
        using ConstMap = JsUtil::BaseDictionary<T, Js::RegSlot, ArenaAllocator>;
        ConstMap<int32> * m_i32Consts;
        ConstMap<int64> * m_i64Consts;
        ConstMap<float> * m_f32Consts;
        ConstMap<double> * m_f64Consts;

        WasmTypeArray * m_locals;

        ArenaAllocator * m_alloc;
        WasmSignature * m_signature;
        Js::ByteCodeLabel m_ExitLabel;
        LPCUTF8 m_name;
        LPCUTF8 m_mod; // imported module
        UINT32 m_number;
    };
} // namespace Wasm
