//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    class WasmBinaryReader;

    class WasmModule : public FinalizableObject
    {
    private:
        struct Memory
        {
            Memory() : minSize(0), maxSize(0), exported(false)
            {
            }
            uint64 minSize;
            uint64 maxSize;
            bool exported;
            static const uint64 PAGE_SIZE = 64 * 1024;
        } m_memory;
    public:
        WasmModule(Js::ScriptContext* scriptContext, byte* binaryBuffer, uint binaryBufferLength);

        // The index used by those methods is the function index as describe by the WebAssembly design, ie: imports first then wasm functions
        uint32 GetMaxFunctionIndex() const;
        WasmSignature* GetFunctionSignature(uint32 funcIndex) const;
        // normalizedIndex is the index in the respective table's type
        FunctionIndexTypes::Type GetFunctionIndexType(uint32 funcIndex, uint32* normalizedIndex = nullptr) const;

        void InitializeMemory(uint32 minSize, uint32 maxSize);
        void SetMemoryIsExported() { m_memory.exported = true; }
        const Memory* GetMemory() const;

        void SetSignature(uint32 sigId, WasmSignature * signature);
        WasmSignature* GetSignature(uint32 sigId) const;
        void SetSignatureCount(uint32 count);
        uint32 GetSignatureCount() const;

        void CalculateEquivalentSignatures();
        uint32 GetEquivalentSignatureId(uint32 sigId) const;

        void AllocateTable(uint32 entries);
        void SetTableValue(uint32 funcIndex, uint32 indirectIndex);
        uint32 GetTableValue(uint32 indirTableIndex) const;
        uint32 GetTableSize() const;

        uint GetWasmFunctionCount() const;
        void AllocateWasmFunctions(uint32 count);
        bool SetWasmFunctionInfo(WasmFunctionInfo* funcInfo, uint32 index);
        void AddWasmFunctionInfo(WasmFunctionInfo* funcInfo);
        WasmFunctionInfo* GetWasmFunctionInfo(uint index) const;

        void AllocateFunctionExports(uint32 entries);
        uint GetExportCount() const { return m_exportCount; }
        void SetFunctionExport(uint32 iExport, uint32 funcIndex, char16* exportName, uint32 nameLength);
        WasmExport* GetFunctionExport(uint32 iExport) const;

        void AllocateFunctionImports(uint32 entries);
        uint32 GetImportCount() const { return m_importCount; }
        void SetFunctionImport(uint32 i, uint32 sigId, char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen);
        WasmImport* GetFunctionImport(uint32 i) const;

        void AllocateDataSegs(uint32 count);
        bool AddDataSeg(WasmDataSegment* seg, uint32 index);
        WasmDataSegment* GetDataSeg(uint32 index) const;
        uint32 GetDataSegCount() const { return m_datasegCount; }

        void SetStartFunction(uint32 i);
        uint32 GetStartFunction() const;

        uint32 GetModuleEnvironmentSize() const;

        uint GetHeapOffset() const { return 0; }
        uint GetFuncOffset() const { return GetImportFuncOffset() + GetImportCount(); }
        uint GetImportFuncOffset() const { return GetHeapOffset() + 1; }
        uint GetTableEnvironmentOffset() const { return GetFuncOffset() + GetWasmFunctionCount(); }

        WasmBinaryReader* GetReader() const { return m_reader; }

        virtual void Finalize(bool isShutdown) override;
        virtual void Dispose(bool isShutdown) override;
        virtual void Mark(Recycler * recycler) override;

    private:
        WasmSignature** m_signatures;
        uint32* m_indirectfuncs;
        JsUtil::List<WasmFunctionInfo*, ArenaAllocator> m_functionsInfo;
        WasmExport* m_exports;
        WasmImport* m_imports;
        WasmDataSegment** m_datasegs;
        WasmBinaryReader* m_reader;
        uint32* m_equivalentSignatureMap;

        uint m_signaturesCount;
        uint m_indirectFuncCount;
        uint m_exportCount;
        uint32 m_importCount;
        uint32 m_datasegCount;

        uint32 m_startFuncIndex;

        ArenaAllocator m_alloc;

        // Describes the module's Environment
        uint wasmDefinedFuncCount;
    };
} // namespace Wasm
