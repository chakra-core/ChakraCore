//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    class WasmGlobal;
    struct WasmConstLitNode;
}
namespace Js
{
    class WebAssemblyModule;
    class WebAssemblyTable;
    class WebAssemblyMemory;
    class WebAssemblyEnvironment
    {
    public:
        WebAssemblyEnvironment(WebAssemblyModule* module);

        Var* GetStartPtr() const { return (Var*)PointerValue(start); }

        AsmJsScriptFunction* GetWasmFunction(uint32 index) const;
        void SetWasmFunction(uint32 index, AsmJsScriptFunction* func);
        void SetImportedFunction(uint32 index, Var importedFunc);
        WebAssemblyTable* GetTable(uint32 index) const;
        void SetTable(uint32 index, class WebAssemblyTable* table);
        WebAssemblyMemory* GetMemory(uint32 index) const;
        void SetMemory(uint32 index, WebAssemblyMemory* mem);
        Wasm::WasmConstLitNode GetGlobalValue(Wasm::WasmGlobal* global) const;
        void SetGlobalValue(class Wasm::WasmGlobal* global, Wasm::WasmConstLitNode cnst);

        void CalculateOffsets(WebAssemblyTable* table, WebAssemblyMemory* memory);
        uint32 GetElementSegmentOffset(uint32 index) const;
        uint32 GetDataSegmentOffset(uint32 index) const;

    private:
        Field(WebAssemblyModule*) module;
        Field(Field(Var)*) start;
        Field(Field(Var)*) end;
        // Precalculated pointer from ptr using the offsets
        Field(Field(Var)*) memory;
        Field(Field(Var)*) imports;
        Field(Field(Var)*) functions;
        Field(Field(Var)*) table;
        Field(Field(Var)*) globals;

        uint32* elementSegmentOffsets;
        uint32* dataSegmentOffsets;
        DebugOnly(bool offsetInitialized = false);

        ArenaAllocator m_alloc;
    private:
        template<typename T> void CheckPtrIsValid(intptr_t ptr) const;
        template<typename T> T* GetVarElement(Field(Var)* ptr, uint32 index, uint32 maxCount) const;
        template<typename T> void SetVarElement(Field(Var)* ptr, T* val, uint32 index, uint32 maxCount);
        template<typename T> T GetGlobalInternal(uint32 offset) const;
        template<typename T> void SetGlobalInternal(uint32 offset, T val);
    };

} // namespace Js
