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
        WasmFunctionInfo(ArenaAllocator* alloc, WasmSignature* signature, uint32 number);

        void AddLocal(WasmTypes::WasmType type, uint32 count = 1);
        Local GetLocal(uint32 index) const;
        WasmTypes::WasmType GetResultType() const;

        uint32 GetLocalCount() const;
        Js::ArgSlot GetParamCount() const;

        void SetName(const char16* name, uint32 nameLength) { m_name = name; m_nameLength = nameLength; }
        const char16* GetName() const { return m_name; }
        uint32 GetNameLength() const { return m_nameLength; }

        uint32 GetNumber() const { return m_number; }
        WasmSignature* GetSignature() const { return m_signature; }

        void SetExitLabel(Js::ByteCodeLabel label) { m_ExitLabel = label; }
        Js::ByteCodeLabel GetExitLabel() const { return m_ExitLabel; }
        Js::FunctionBody* GetBody() const { return m_body; }
        void SetBody(Js::FunctionBody* val) { m_body = val; }

        WasmReaderBase* GetCustomReader() const { return m_customReader; }
        void SetCustomReader(WasmReaderBase* customReader) { m_customReader = customReader; }
#if DBG_DUMP
        FieldNoBarrier(WasmImport*) importedFunctionReference;
#endif

        Field(FunctionBodyReaderInfo) m_readerInfo;
    private:

        FieldNoBarrier(ArenaAllocator*) m_alloc;
        typedef JsUtil::GrowingArray<Local, ArenaAllocator> WasmTypeArray;
        Field(WasmTypeArray) m_locals;
        Field(Js::FunctionBody*) m_body;
        Field(WasmSignature*) m_signature;
        Field(Js::ByteCodeLabel) m_ExitLabel;
        Field(WasmReaderBase*) m_customReader;
        Field(const char16*) m_name;
        Field(uint32) m_nameLength;
        Field(uint32) m_number;
    };
} // namespace Wasm
