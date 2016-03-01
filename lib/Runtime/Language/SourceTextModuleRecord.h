//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class SourceTextModuleRecord;
    typedef JsUtil::List<SourceTextModuleRecord*> ModuleRecordList;
    typedef  JsUtil::BaseDictionary<LPCOLESTR, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ChildModuleRecordSet;
    typedef  JsUtil::BaseDictionary<SourceTextModuleRecord*, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ParentModuleRecordSet;
    typedef  JsUtil::BaseDictionary<PropertyId, uint, ArenaAllocator, PowerOf2SizePolicy> LocalExportMap;

    class SourceTextModuleRecord : public ModuleRecordBase
    {
    public:
        SourceTextModuleRecord(ScriptContext* scriptContext);
        IdentPtrList* GetRequestedModuleList() const { return requestedModuleList; }
        ModuleImportEntryList* GetImportEntryList() const { return importRecordList; }
        ModuleExportEntryList* GetLocalExportEntryList() const { return localExportRecordList; }
        ModuleExportEntryList* GetIndirectExportEntryList() const { return indirectExportRecordList; }
        ModuleExportEntryList* GetStarExportRecordList() const { return starExportRecordList; }
        ExportedNames* GetExportedNames(ResolveSet* exportStarSet) override { Assert(false); return nullptr; }
        
        // return false when "ambiguous". otherwise exportRecord.
        bool ResolveExport(PropertyId exportName, ResolutionDictionary* resolveSet, ResolveSet* exportStarSet, ModuleNameRecord** exportRecord) override;
        void ModuleDeclarationInstantiation() override;
        Var ModuleEvaluation() override;

        void Finalize(bool isShutdown) override { return; }
        void Dispose(bool isShutdown) override { return; }
        void Mark(Recycler * recycler) override { return; }

        void ResolveExternalModuleDependencies();

        void* GetHostDefined() const { return hostDefined; }
        void SetHostDefined(void* hostObj) { hostDefined = hostObj; }

        bool WasParsed() const { return wasParsed; }
        void SetWasParsed() { wasParsed = true; }
        bool WasDeclarationInitialized() const { return wasDeclarationInitialized; }
        void SetWasDeclarationInitialized() { wasDeclarationInitialized = true; }
        void SetIsRootModule() { isRootModule = true; }

        void SetImportRecordList(ModuleImportEntryList* importList) { importRecordList = importList; }
        void SetLocalExportRecordList(ModuleExportEntryList* localExports) { localExportRecordList = localExports; }
        void SetIndirectExportRecordList(ModuleExportEntryList* indirectExports) { indirectExportRecordList = indirectExports; }
        void SetStarExportRecordList(ModuleExportEntryList* starExports) { starExportRecordList = starExports; }
        void SetrequestedModuleList(IdentPtrList* requestModules) { requestedModuleList = requestModules; }

        ScriptContext* GetScriptContext() const { return scriptContext; }
        HRESULT ParseSource(__in_bcount(sourceLength) byte* sourceText, unsigned long sourceLength, Var* exceptionVar, bool isUtf8);
        HRESULT OnHostException(void* errorVar);

        static SourceTextModuleRecord* FromHost(void* hostModuleRecord)
        {
            SourceTextModuleRecord* moduleRecord = static_cast<SourceTextModuleRecord*>(hostModuleRecord);
            Assert((moduleRecord == nullptr) || (moduleRecord->magicNumber == moduleRecord->ModuleMagicNumber));
            return moduleRecord;
        }
        static SourceTextModuleRecord* Create(ScriptContext* scriptContext);

        uint GetLocalExportSlotIndex(PropertyId exportNameId);
        Var* GetLocalExportSlots() const { return localExportSlots; }
        uint GetModuleId() const { return moduleId; }

#if DBG
        void AddParent(SourceTextModuleRecord* parentRecord, LPCWSTR specifier, unsigned long specifierLength);
#endif

    private:
        const static uint InvalidModuleIndex = 0xffffffff;
        const static uint InvalidSlotCount = 0xffffffff;
        const static uint InvalidSlotIndex = 0xffffffff;
        // TODO: move non-GC fields out to avoid false reference?
        // This is the parsed tree resulted from compilation. 
        bool wasParsed;
        bool wasDeclarationInitialized;
        bool isRootModule;
        ParseNodePtr parseTree;
        SRCINFO srcInfo; 
        Utf8SourceInfo* pSourceInfo;
        uint sourceIndex;
        Parser* parser;  // we'll need to keep the parser around till we are done with bytecode gen.
        ScriptContext* scriptContext;
        IdentPtrList* requestedModuleList;
        ModuleImportEntryList* importRecordList;
        ModuleExportEntryList* localExportRecordList;
        ModuleExportEntryList* indirectExportRecordList;
        ModuleExportEntryList* starExportRecordList;
        ChildModuleRecordSet* childrenModuleSet;
        ModuleRecordList* parentModuleList;
        LocalExportMap* localExportMap;
        uint numUnParsedChildrenModule;
        TempArenaAllocatorObject* tempAllocatorObject;

        uint moduleIndex;

        Js::JavascriptFunction* rootFunction;
        void* hostDefined;
        Var errorObject;
        Var* localExportSlots;

        uint localSlotCount;
        uint moduleId;

        ArenaAllocator* EnsureTempAllocator();
        HRESULT PostParseProcess();
        void ImportModuleListsFromParser();
        HRESULT OnChildModuleReady(SourceTextModuleRecord* childModule, Var errorObj);
        void CleanupBeforeExecution();
        void InitializeLocalExports();
    };
}