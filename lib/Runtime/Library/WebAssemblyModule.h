//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "../WasmReader/WasmParseTree.h"

namespace Wasm
{
    class WasmSignature;
    class WasmFunctionInfo;
    class WasmBinaryReader;
    class WasmDataSegment;
    class WasmGlobal;
    struct WasmImport;
    struct WasmExport;

    namespace WasmTypes
    {
        enum WasmType;
    }
    namespace FunctionIndexTypes
    {
        enum Type;
    }
    namespace ExternalKinds
    {
        enum ExternalKind;
    }
}

// TODO: merge with WasmModule
namespace Js
{
class WebAssemblyModule : public DynamicObject
{
public:

    class EntryInfo
    {
    public:
        static FunctionInfo NewInstance;
    };

    static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

    static bool Is(Var aValue);
    static WebAssemblyModule * FromVar(Var aValue);

    static WebAssemblyModule * CreateModule(
        ScriptContext* scriptContext,
        const byte* buffer,
        const uint lengthBytes,
        Var bufferSrc = nullptr);

    static bool ValidateModule(
        ScriptContext* scriptContext,
        const byte* buffer,
        const uint lengthBytes,
        Var bufferSrc);

public:
    WebAssemblyModule(Js::ScriptContext* scriptContext, const byte* binaryBuffer, uint binaryBufferLength, DynamicType * type);

    // The index used by those methods is the function index as describe by the WebAssembly design, ie: imports first then wasm functions
    uint32 GetMaxFunctionIndex() const;
    Wasm::WasmSignature* GetFunctionSignature(uint32 funcIndex) const;
    Wasm::FunctionIndexTypes::Type GetFunctionIndexType(uint32 funcIndex) const;
    // Returns index in the respective table's type
    uint32 NormalizeFunctionIndex(uint32 funcIndex) const;

    void InitializeMemory(uint32 minSize, uint32 maxSize);
    void SetMemoryExported() { isMemExported = true; }
    bool IsMemoryExported() const { return isMemExported; }
    WebAssemblyMemory * GetMemory() const;

    void SetSignature(uint32 index, Wasm::WasmSignature * signature);
    Wasm::WasmSignature* GetSignature(uint32 index) const;
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
    bool SetWasmFunctionInfo(Wasm::WasmFunctionInfo* funsig, uint32 index);
    Wasm::WasmFunctionInfo* GetWasmFunctionInfo(uint index) const;

    void AllocateFunctionExports(uint32 entries);
    uint GetExportCount() const { return m_exportCount; }
    void SetExport(uint32 iExport, uint32 funcIndex, const char16* exportName, uint32 nameLength, Wasm::ExternalKinds::ExternalKind kind);
    Wasm::WasmExport* GetFunctionExport(uint32 iExport) const;

    void AllocateFunctionImports(uint32 entries);
    uint32 GetImportCount() const { return m_importCount; }
    void SetImportCount(uint count) { m_importCount = count; }
    void SetFunctionImport(uint32 i, uint32 sigId, const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen, Wasm::ExternalKinds::ExternalKind kind);
    Wasm::WasmImport* GetFunctionImport(uint32 i) const;
    void AddGlobalImport(const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen, Wasm::ExternalKinds::ExternalKind kind, Wasm::WasmGlobal* importedGlobal);

    void AllocateDataSegs(uint32 count);
    bool AddDataSeg(Wasm::WasmDataSegment* seg, uint32 index);
    Wasm::WasmDataSegment* GetDataSeg(uint32 index) const;
    uint32 GetDataSegCount() const { return m_datasegCount; }

    void SetStartFunction(uint32 i);
    uint32 GetStartFunction() const;

    uint32 GetModuleEnvironmentSize() const;

    uint GetHeapOffset() const { return 0; }
    uint GetImportFuncOffset() const { return GetHeapOffset() + 1; }
    uint GetFuncOffset() const { return GetImportFuncOffset() + GetImportCount(); }
    uint GetTableEnvironmentOffset() const { return GetFuncOffset() + GetWasmFunctionCount(); }
    uint GetGlobalOffset() const { return GetTableEnvironmentOffset() + GetSignatureCount(); }
    uint GetOffsetForGlobal(Wasm::WasmGlobal* global);
    uint AddGlobalByteSizeToOffset(Wasm::WasmTypes::WasmType type, uint32 offset) const;
    uint GetGlobalsByteSize() const;

    Wasm::WasmBinaryReader* GetReader() const { return m_reader; }

    virtual void Finalize(bool isShutdown) override;
    virtual void Dispose(bool isShutdown) override;
    virtual void Mark(Recycler * recycler) override;

    uint globalCounts[Wasm::WasmTypes::Limit];
    typedef JsUtil::List<Wasm::WasmGlobal*, ArenaAllocator> WasmGlobalsList;
    WasmGlobalsList * globals;

private:
    WebAssemblyMemory * m_memory;
    Wasm::WasmSignature** m_signatures;
    uint32* m_indirectfuncs;
    Wasm::WasmFunctionInfo** m_functionsInfo;
    Wasm::WasmExport* m_exports;
    Wasm::WasmImport* m_imports;
    Wasm::WasmDataSegment** m_datasegs;
    Wasm::WasmBinaryReader* m_reader;
    uint32* m_equivalentSignatureMap;

    uint m_signaturesCount;
    uint m_indirectFuncCount;
    uint m_funcCount;
    uint m_exportCount;
    uint32 m_importCount;
    uint32 m_datasegCount;

    uint32 m_startFuncIndex;

    ArenaAllocator m_alloc;

    bool isMemExported;
};

} // namespace Js
