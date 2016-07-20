//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    struct FunctionBodyReaderInfo
    {
        uint32 index;
        uint32 size;
        intptr_t startOffset;
    };

    class WasmFunctionInfo
    {
    public:
        WasmFunctionInfo(ArenaAllocator * alloc);

        void AddLocal(WasmTypes::WasmType type, uint count = 1);

        WasmTypes::WasmType GetLocal(uint index) const;
        WasmTypes::WasmType GetParam(uint index) const;
        WasmTypes::WasmType GetResultType() const;

        uint32 GetLocalCount() const;
        uint32 GetParamCount() const;

        void SetName(char16* name);
        char16* GetName() const;

        void SetNumber(UINT32 number);
        UINT32 GetNumber() const;
        void SetSignature(WasmSignature * signature);
        WasmSignature * GetSignature() const;

        void SetExitLabel(Js::ByteCodeLabel label);
        Js::ByteCodeLabel GetExitLabel() const;

        void SetLocalName(uint i, char16* n);
        char16* GetLocalName(uint i);

        FunctionBodyReaderInfo m_readerInfo;
    private:
        WasmTypeArray * m_locals;

        ArenaAllocator * m_alloc;
        WasmSignature * m_signature;
        Js::ByteCodeLabel m_ExitLabel;
        char16* m_name;
        UINT32 m_number;
    };

    struct WasmFunction
    {
        WasmFunction() :
            body(nullptr)
        {
        }
        Js::FunctionBody * body;
    };
} // namespace Wasm
