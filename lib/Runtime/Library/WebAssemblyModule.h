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
    class WasmElementSegment;
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
        const uint lengthBytes);

    static bool ValidateModule(
        ScriptContext* scriptContext,
        const byte* buffer,
        const uint lengthBytes);

public:
    WebAssemblyModule(Js::ScriptContext* scriptContext, const byte* binaryBuffer, uint binaryBufferLength, DynamicType * type);

    // The index used by those methods is the function index as describe by the WebAssembly design, ie: imports first then wasm functions
    uint32 GetMaxFunctionIndex() const;
    Wasm::WasmSignature* GetFunctionSignature(uint32 funcIndex) const;
    Wasm::FunctionIndexTypes::Type GetFunctionIndexType(uint32 funcIndex) const;

    void InitializeMemory(uint32 minSize, uint32 maxSize);
    WebAssemblyMemory * CreateMemory() const;
    bool HasMemory() const { return m_hasMemory; }
    bool HasMemoryImport() const { return m_memImport != nullptr; }
    bool IsValidMemoryImport(const WebAssemblyMemory * memory) const;

    Wasm::WasmSignature * GetSignatures() const;
    Wasm::WasmSignature* GetSignature(uint32 index) const;
    void SetSignatureCount(uint32 count);
    uint32 GetSignatureCount() const;

    uint32 GetEquivalentSignatureId(uint32 sigId) const;

    void InitializeTable(uint32 minEntries, uint32 maxEntries);
    WebAssemblyTable * CreateTable() const;
    bool HasTable() const { return m_hasTable; }
    bool HasTableImport() const { return m_tableImport != nullptr; }
    bool IsValidTableImport(const WebAssemblyTable * table) const;

    uint GetWasmFunctionCount() const;
    Wasm::WasmFunctionInfo* AddWasmFunctionInfo(Wasm::WasmSignature* funsig);
    Wasm::WasmFunctionInfo* GetWasmFunctionInfo(uint index) const;

    void AllocateFunctionExports(uint32 entries);
    uint GetExportCount() const { return m_exportCount; }
    void SetExport(uint32 iExport, uint32 funcIndex, const char16* exportName, uint32 nameLength, Wasm::ExternalKinds::ExternalKind kind);
    Wasm::WasmExport* GetFunctionExport(uint32 iExport) const;

    uint32 GetImportCount() const;
    void AddFunctionImport(uint32 sigId, const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen);
    Wasm::WasmImport* GetFunctionImport(uint32 i) const;
    void AddGlobalImport(const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen, Wasm::WasmGlobal* importedGlobal);
    void AddMemoryImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen);
    void AddTableImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen);
    Wasm::WasmImport * GetMemoryImport() const { return m_memImport; }
    Wasm::WasmImport * GetTableImport() const { return m_tableImport; }

    uint GetOffsetFromInit(const Wasm::WasmNode& initexpr) const;

    void AllocateDataSegs(uint32 count);
    void SetDataSeg(Wasm::WasmDataSegment* seg, uint32 index);
    Wasm::WasmDataSegment* GetDataSeg(uint32 index) const;
    uint32 GetDataSegCount() const { return m_datasegCount; }

    void AllocateElementSegs(uint32 count);
    void SetElementSeg(Wasm::WasmElementSegment* seg, uint32 index);
    Wasm::WasmElementSegment* GetElementSeg(uint32 index) const;
    uint32 GetElementSegCount() const { return m_elementsegCount; }

    void SetStartFunction(uint32 i);
    uint32 GetStartFunction() const;

    uint32 GetModuleEnvironmentSize() const;

    // elements at known offsets
    static uint GetMemoryOffset() { return 0; }
    static uint GetImportFuncOffset() { return GetMemoryOffset() + 1; }

    // elements at instance dependent offsets
    uint GetFuncOffset() const { return GetImportFuncOffset() + GetImportCount(); }
    uint GetTableEnvironmentOffset() const { return GetFuncOffset() + GetWasmFunctionCount(); }
    uint GetGlobalOffset() const { return GetTableEnvironmentOffset() + 1; }
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
    bool m_hasTable;
    bool m_hasMemory;
    // The binary buffer is recycler allocated, tied the lifetime of the buffer to the module
    const byte* m_binaryBuffer;
    uint32 m_memoryInitSize;
    uint32 m_memoryMaxSize;
    uint32 m_tableInitSize;
    uint32 m_tableMaxSize;
    Wasm::WasmSignature* m_signatures;
    uint32* m_indirectfuncs;
    Wasm::WasmElementSegment** m_elementsegs;
    typedef JsUtil::List<Wasm::WasmFunctionInfo*, Recycler> WasmFunctionInfosList;
    WasmFunctionInfosList* m_functionsInfo;
    Wasm::WasmExport* m_exports;
    typedef JsUtil::List<Wasm::WasmImport*, ArenaAllocator> WasmImportsList;
    WasmImportsList* m_imports;
    Wasm::WasmImport* m_memImport;
    Wasm::WasmImport* m_tableImport;
    Wasm::WasmDataSegment** m_datasegs;
    Wasm::WasmBinaryReader* m_reader;
    uint32* m_equivalentSignatureMap;

    uint m_signaturesCount;
    uint m_exportCount;
    uint32 m_datasegCount;
    uint32 m_elementsegCount;

    uint32 m_startFuncIndex;

    ArenaAllocator m_alloc;
};

} // namespace Js
