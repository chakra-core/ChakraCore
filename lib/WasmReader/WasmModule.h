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

        void InitializeMemory(uint32 minSize, uint32 maxSize);
        void SetMemoryIsExported() { m_memory.exported = true; }

        const Memory* GetMemory() const;

        void SetSignature(uint32 index, WasmSignature * signature);
        WasmSignature* GetSignature(uint32 index) const;
        void SetSignatureCount(uint32 count);
        uint32 GetSignatureCount() const;

        void AllocateIndirectFunctions(uint32 entries);
        void SetIndirectFunction(uint32 funcIndex, uint32 indirectIndex);
        uint32 GetIndirectFunctionIndex(uint32 indirTableIndex) const;
        uint32 GetIndirectFunctionCount() const;

        uint GetFunctionCount() const;
        void AllocateFunctions(uint32 count);
        bool SetFunctionInfo(WasmFunctionInfo* funsig, uint32 index);
        WasmFunctionInfo* GetFunctionInfo(uint index) const;

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

        uint GetHeapOffset() const { return heapOffset; }
        void SetHeapOffset(uint val) { heapOffset = val; }
        uint GetFuncOffset() const { return funcOffset; }
        void SetFuncOffset(uint val) { funcOffset = val; }
        uint GetImportFuncOffset() const { return importFuncOffset; }
        void SetImportFuncOffset(uint val) { importFuncOffset = val; }
        uint GetIndirFuncTableOffset() const { return indirFuncTableOffset; }
        void SetIndirFuncTableOffset(uint val) { indirFuncTableOffset = val; }

        WasmBinaryReader* GetReader() const { return m_reader; }

        virtual void Finalize(bool isShutdown) override;
        virtual void Dispose(bool isShutdown) override;
        virtual void Mark(Recycler * recycler) override;

    private:
        WasmSignature** m_signatures;
        uint32* m_indirectfuncs;
        WasmFunctionInfo** m_functionsInfo;
        WasmExport* m_exports;
        WasmImport* m_imports;
        WasmDataSegment** m_datasegs;
        WasmBinaryReader* m_reader;

        uint m_signaturesCount;
        uint m_indirectFuncCount;
        uint m_funcCount;
        uint m_exportCount;
        uint32 m_importCount;
        uint32 m_datasegCount;

        uint32 m_startFuncIndex;

        ArenaAllocator m_alloc;

        // Describes the module's Environment
        uint heapOffset;
        uint funcOffset;
        uint importFuncOffset;
        uint indirFuncTableOffset;
    };
} // namespace Wasm
