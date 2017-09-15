//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef ENABLE_WASM
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
    struct CustomSection;
}

namespace Js
{
class WebAssemblyModule : public DynamicObject
{
protected:
    DEFINE_VTABLE_CTOR(WebAssemblyModule, DynamicObject);
    DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(WebAssemblyModule);

public:

    class EntryInfo
    {
    public:
        static FunctionInfo NewInstance;
        static FunctionInfo Exports;
        static FunctionInfo Imports;
        static FunctionInfo CustomSections;
    };

    static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryExports(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryImports(RecyclableObject* function, CallInfo callInfo, ...);
    static Var EntryCustomSections(RecyclableObject* function, CallInfo callInfo, ...);

    static bool Is(Var aValue);
    static WebAssemblyModule * FromVar(Var aValue);

    static WebAssemblyModule * CreateModule(
        ScriptContext* scriptContext,
        class WebAssemblySource* src);

    static bool ValidateModule(
        ScriptContext* scriptContext,
        class WebAssemblySource* src);

public:
    WebAssemblyModule(Js::ScriptContext* scriptContext, const byte* binaryBuffer, uint binaryBufferLength, DynamicType * type);

    const byte* GetBinaryBuffer() const { return m_binaryBuffer; }
    uint GetBinaryBufferLength() const { return m_binaryBufferLength; }

    // The index used by those methods is the function index as describe by the WebAssembly design, ie: imports first then wasm functions
    uint32 GetMaxFunctionIndex() const;
    Wasm::FunctionIndexTypes::Type GetFunctionIndexType(uint32 funcIndex) const;

    void InitializeMemory(uint32 minSize, uint32 maxSize);
    WebAssemblyMemory * CreateMemory() const;
    bool HasMemory() const { return m_hasMemory; }
    bool HasMemoryImport() const { return m_memImport != nullptr; }
    uint32 GetMemoryInitSize() const { return m_memoryInitSize; }
    uint32 GetMemoryMaxSize() const { return m_memoryMaxSize; }

    Wasm::WasmSignature * GetSignatures() const;
    Wasm::WasmSignature* GetSignature(uint32 index) const;
    void SetSignatureCount(uint32 count);
    uint32 GetSignatureCount() const;

    uint32 GetEquivalentSignatureId(uint32 sigId) const;

    void InitializeTable(uint32 minEntries, uint32 maxEntries);
    WebAssemblyTable * CreateTable() const;
    bool HasTable() const { return m_hasTable; }
    bool HasTableImport() const { return m_tableImport != nullptr; }
    uint32 GetTableInitSize() const { return m_tableInitSize; }
    uint32 GetTableMaxSize() const { return m_tableMaxSize; }

    uint GetWasmFunctionCount() const;
    Wasm::WasmFunctionInfo* AddWasmFunctionInfo(Wasm::WasmSignature* funsig);
    Wasm::WasmFunctionInfo* GetWasmFunctionInfo(uint index) const;
    void SwapWasmFunctionInfo(uint i1, uint i2);
#if ENABLE_DEBUG_CONFIG_OPTIONS
    void AttachCustomInOutTracingReader(Wasm::WasmFunctionInfo* func, uint callIndex);
#endif

    void AllocateFunctionExports(uint32 entries);
    uint GetExportCount() const { return m_exportCount; }
    void SetExport(uint32 iExport, uint32 funcIndex, const char16* exportName, uint32 nameLength, Wasm::ExternalKinds::ExternalKind kind);
    Wasm::WasmExport* GetExport(uint32 iExport) const;

    uint32 GetImportCount() const;
    Wasm::WasmImport* GetImport(uint32 i) const;
    void AddFunctionImport(uint32 sigId, const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen);
    void AddGlobalImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen);
    void AddMemoryImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen);
    void AddTableImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen);
    uint32 GetImportedFunctionCount() const { return m_importedFunctionCount; }

    uint GetOffsetFromInit(const Wasm::WasmNode& initExpr, const class WebAssemblyEnvironment* env) const;
    void ValidateInitExportForOffset(const Wasm::WasmNode& initExpr) const;

    void AddGlobal(Wasm::GlobalReferenceTypes::Type refType, Wasm::WasmTypes::WasmType type, bool isMutable, Wasm::WasmNode init);
    uint32 GetGlobalCount() const;
    Wasm::WasmGlobal* GetGlobal(uint32 index) const;

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
    uint GetFuncOffset() const { return GetImportFuncOffset() + GetImportedFunctionCount(); }
    uint GetTableEnvironmentOffset() const { return GetFuncOffset() + GetWasmFunctionCount(); }
    uint GetGlobalOffset() const { return GetTableEnvironmentOffset() + 1; }
    uint GetOffsetForGlobal(Wasm::WasmGlobal* global) const;
    uint AddGlobalByteSizeToOffset(Wasm::WasmTypes::WasmType type, uint32 offset) const;
    uint GetGlobalsByteSize() const;

    void AddCustomSection(Wasm::CustomSection customSection);
    uint32 GetCustomSectionCount() const;
    Wasm::CustomSection GetCustomSection(uint32 index) const;

    Wasm::WasmBinaryReader* GetReader() const { return m_reader; }

    virtual void Finalize(bool isShutdown) override;
    virtual void Dispose(bool isShutdown) override;
    virtual void Mark(Recycler * recycler) override;

private:
    static JavascriptString * GetExternalKindString(ScriptContext * scriptContext, Wasm::ExternalKinds::ExternalKind kind);

    Field(bool) m_hasTable;
    Field(bool) m_hasMemory;
    // The binary buffer is recycler allocated, tied the lifetime of the buffer to the module
    Field(const byte*) m_binaryBuffer;
    Field(uint) m_binaryBufferLength;
    Field(uint32) m_memoryInitSize;
    Field(uint32) m_memoryMaxSize;
    Field(uint32) m_tableInitSize;
    Field(uint32) m_tableMaxSize;
    Field(Wasm::WasmSignature*) m_signatures;
    Field(uint32*) m_indirectfuncs;
    Field(Wasm::WasmElementSegment**) m_elementsegs;
    typedef JsUtil::List<Wasm::WasmFunctionInfo*, Recycler> WasmFunctionInfosList;
    Field(WasmFunctionInfosList*) m_functionsInfo;
    Field(Wasm::WasmExport*) m_exports;
    typedef JsUtil::List<Wasm::WasmImport*, ArenaAllocator> WasmImportsList;
    Field(WasmImportsList*) m_imports;
    Field(Wasm::WasmImport*) m_memImport;
    Field(Wasm::WasmImport*) m_tableImport;
    Field(uint32) m_importedFunctionCount;
    Field(Wasm::WasmDataSegment**) m_datasegs;
    Field(Wasm::WasmBinaryReader*) m_reader;
    Field(uint32*) m_equivalentSignatureMap;
    typedef JsUtil::List<Wasm::CustomSection, ArenaAllocator> CustomSectionsList;
    Field(CustomSectionsList*) m_customSections;

    Field(uint) m_globalCounts[Wasm::WasmTypes::Limit];
    typedef JsUtil::List<Wasm::WasmGlobal*, ArenaAllocator> WasmGlobalsList;
    Field(WasmGlobalsList *) m_globals;

    Field(uint) m_signaturesCount;
    Field(uint) m_exportCount;
    Field(uint32) m_datasegCount;
    Field(uint32) m_elementsegCount;

    Field(uint32) m_startFuncIndex;

    FieldNoBarrier(ArenaAllocator*) m_alloc;
};

} // namespace Js
#endif
