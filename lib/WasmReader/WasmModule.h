//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#include "WasmGlobal.h"

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
        FunctionIndexTypes::Type GetFunctionIndexType(uint32 funcIndex) const;
        // Returns index in the respective table's type
        uint32 NormalizeFunctionIndex(uint32 funcIndex) const;

        void InitializeMemory(uint32 minSize, uint32 maxSize);
        void SetMemoryIsExported() { m_memory.exported = true; }
        const Memory* GetMemory() const;

        void SetSignature(uint32 index, WasmSignature * signature);
        WasmSignature* GetSignature(uint32 index) const;
        void SetSignatureCount(uint32 count);
        uint32 GetSignatureCount() const;

        void CalculateEquivalentSignatures();
        uint32 GetEquivalentSignatureId(uint32 sigId) const;

        void SetTableSize(uint32 entries);
        void SetTableValues(WasmElementSegment* seg, uint32 index);
        uint32 GetTableValue(uint32 indirTableIndex) const;
        uint32 GetTableSize() const;

        uint GetWasmFunctionCount() const;
        void AllocateWasmFunctions(uint32 count);
        bool SetWasmFunctionInfo(WasmFunctionInfo* funsig, uint32 index);
        WasmFunctionInfo* GetWasmFunctionInfo(uint index) const;

        void AllocateFunctionExports(uint32 entries);
        uint GetExportCount() const { return m_exportCount; }
        void SetExport(uint32 iExport, uint32 funcIndex, char16* exportName, uint32 nameLength, ExternalKinds::ExternalKind kind);
        WasmExport* GetFunctionExport(uint32 iExport) const;

        void AllocateFunctionImports(uint32 entries);
        uint32 GetImportCount() const { return m_importCount; }
        void SetImportCount(uint count) { m_importCount = count;  }
        void SetFunctionImport(uint32 i, uint32 sigId, char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen, ExternalKinds::ExternalKind kind);
        WasmImport* GetFunctionImport(uint32 i) const;
        void AddGlobalImport(char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen, ExternalKinds::ExternalKind kind, WasmGlobal* importedGlobal);

        uint GetOffsetFromInit(const WasmNode& initexpr) const;

        void AllocateDataSegs(uint32 count);
        bool AddDataSeg(WasmDataSegment* seg, uint32 index);
        WasmDataSegment* GetDataSeg(uint32 index) const;
        uint32 GetDataSegCount() const { return m_datasegCount; }

        void AllocateElementSegs(uint32 count);
        void SetElementSeg(WasmElementSegment* seg, uint32 index);
        void ResolveTableElementOffsets();
        WasmElementSegment* GetElementSeg(uint32 index) const;
        uint32 GetElementSegCount() const { return m_elementsegCount; }

        void SetStartFunction(uint32 i);
        uint32 GetStartFunction() const;

        uint32 GetModuleEnvironmentSize() const;

        uint GetHeapOffset() const { return 0; }
        uint GetImportFuncOffset() const { return GetHeapOffset() + 1; }
        uint GetFuncOffset() const { return GetImportFuncOffset() + GetImportCount(); }
        uint GetTableEnvironmentOffset() const { return GetFuncOffset() + GetWasmFunctionCount(); }
        uint GetGlobalOffset() const { return GetTableEnvironmentOffset() + GetSignatureCount();  }

        uint GetOffsetForGlobal(WasmGlobal* global);
        uint AddGlobalByteSizeToOffset(WasmTypes::WasmType type, uint32 offset) const;
        uint GetGlobalsByteSize() const;

        WasmBinaryReader* GetReader() const { return m_reader; }

        virtual void Finalize(bool isShutdown) override;
        virtual void Dispose(bool isShutdown) override;
        virtual void Mark(Recycler * recycler) override;

        uint globalCounts[WasmTypes::Limit];
        JsUtil::List<WasmGlobal*, ArenaAllocator> globals;

    private:
        WasmSignature** m_signatures;
        uint32* m_indirectfuncs;
        WasmElementSegment** m_elementsegs;
        WasmFunctionInfo** m_functionsInfo;
        WasmExport* m_exports;
        WasmImport* m_imports;
        WasmDataSegment** m_datasegs;
        WasmBinaryReader* m_reader;
        uint32* m_equivalentSignatureMap;

        uint m_signaturesCount;
        uint m_indirectFuncCount;
        uint m_funcCount;
        uint m_exportCount;
        uint32 m_importCount;
        uint32 m_datasegCount;
        uint32 m_elementsegCount;

        uint32 m_startFuncIndex;

        ArenaAllocator m_alloc;
    };
} // namespace Wasm
