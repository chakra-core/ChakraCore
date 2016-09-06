//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class SourceTextModuleRecord;
    typedef JsUtil::BaseDictionary<LPCOLESTR, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ChildModuleRecordSet;
    typedef JsUtil::BaseDictionary<SourceTextModuleRecord*, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ParentModuleRecordSet;
    typedef JsUtil::BaseDictionary<PropertyId, uint, ArenaAllocator, PowerOf2SizePolicy> LocalExportMap;
    typedef JsUtil::BaseDictionary<PropertyId, ModuleNameRecord, ArenaAllocator, PowerOf2SizePolicy> ResolvedExportMap;
    typedef JsUtil::List<PropertyId, ArenaAllocator> LocalExportIndexList;

    class SourceTextModuleRecord : public ModuleRecordBase
    {
    public:
        friend class ModuleNamespace;

        SourceTextModuleRecord(ScriptContext* scriptContext);
        IdentPtrList* GetRequestedModuleList() const { return requestedModuleList; }
        ModuleImportOrExportEntryList* GetImportEntryList() const { return importRecordList; }
        ModuleImportOrExportEntryList* GetLocalExportEntryList() const { return localExportRecordList; }
        ModuleImportOrExportEntryList* GetIndirectExportEntryList() const { return indirectExportRecordList; }
        ModuleImportOrExportEntryList* GetStarExportRecordList() const { return starExportRecordList; }
        virtual ExportedNames* GetExportedNames(ExportModuleRecordList* exportStarSet) override;
        virtual bool IsSourceTextModuleRecord() override { return true; } // we don't really have other kind of modulerecord at this time.

        // return false when "ambiguous". 
        // otherwise nullptr means "null" where we have circular reference/cannot resolve.
        bool ResolveExport(PropertyId exportName, ResolveSet* resolveSet, ExportModuleRecordList* exportStarSet, ModuleNameRecord** exportRecord) override;
        bool ResolveImport(PropertyId localName, ModuleNameRecord** importRecord);
        void ModuleDeclarationInstantiation() override;
        Var ModuleEvaluation() override;
        virtual ModuleNamespace* GetNamespace();
        virtual void SetNamespace(ModuleNamespace* moduleNamespace);

        void Finalize(bool isShutdown) override;
        void Dispose(bool isShutdown) override { return; }
        void Mark(Recycler * recycler) override { return; }

        HRESULT ResolveExternalModuleDependencies();

        void* GetHostDefined() const { return hostDefined; }
        void SetHostDefined(void* hostObj) { hostDefined = hostObj; }

        void SetSpecifier(Var specifier) { this->normalizedSpecifier = specifier; }
        Var GetSpecifier() const { return normalizedSpecifier; }

        Var GetErrorObject() const { return errorObject; }

        bool WasParsed() const { return wasParsed; }
        void SetWasParsed() { wasParsed = true; }
        bool WasDeclarationInitialized() const { return wasDeclarationInitialized; }
        void SetWasDeclarationInitialized() { wasDeclarationInitialized = true; }
        void SetIsRootModule() { isRootModule = true; }

        void SetImportRecordList(ModuleImportOrExportEntryList* importList) { importRecordList = importList; }
        void SetLocalExportRecordList(ModuleImportOrExportEntryList* localExports) { localExportRecordList = localExports; }
        void SetIndirectExportRecordList(ModuleImportOrExportEntryList* indirectExports) { indirectExportRecordList = indirectExports; }
        void SetStarExportRecordList(ModuleImportOrExportEntryList* starExports) { starExportRecordList = starExports; }
        void SetrequestedModuleList(IdentPtrList* requestModules) { requestedModuleList = requestModules; }

        ScriptContext* GetScriptContext() const { return scriptContext; }
        HRESULT ParseSource(__in_bcount(sourceLength) byte* sourceText, uint32 sourceLength, SRCINFO * srcInfo, Var* exceptionVar, bool isUtf8);
        HRESULT OnHostException(void* errorVar);

        static SourceTextModuleRecord* FromHost(void* hostModuleRecord)
        {
            SourceTextModuleRecord* moduleRecord = static_cast<SourceTextModuleRecord*>(hostModuleRecord);
            Assert((moduleRecord == nullptr) || (moduleRecord->magicNumber == moduleRecord->ModuleMagicNumber));
            return moduleRecord;
        }

        static bool Is(void* hostModuleRecord)
        {
            SourceTextModuleRecord* moduleRecord = static_cast<SourceTextModuleRecord*>(hostModuleRecord);
            if (moduleRecord != nullptr && (moduleRecord->magicNumber == moduleRecord->ModuleMagicNumber))
            {
                return true;
            }
            return false;
        }

        static SourceTextModuleRecord* Create(ScriptContext* scriptContext);

        uint GetLocalExportSlotIndexByExportName(PropertyId exportNameId);
        uint GetLocalExportSlotIndexByLocalName(PropertyId localNameId);
        Var* GetLocalExportSlots() const { return localExportSlots; }
        uint GetLocalExportSlotCount() const { return localSlotCount; }
        uint GetModuleId() const { return moduleId; }
        uint GetLocalExportCount() const { return localExportCount; }

        ModuleNameRecord* GetNamespaceNameRecord() { return &namespaceRecord; }

        SourceTextModuleRecord* GetChildModuleRecord(LPCOLESTR specifier) const;
#if DBG
        void AddParent(SourceTextModuleRecord* parentRecord, LPCWSTR specifier, uint32 specifierLength);
#endif

        Utf8SourceInfo* GetSourceInfo() { return this->pSourceInfo; }

    private:
        const static uint InvalidModuleIndex = 0xffffffff;
        const static uint InvalidSlotCount = 0xffffffff;
        const static uint InvalidSlotIndex = 0xffffffff;
        // TODO: move non-GC fields out to avoid false reference?
        // This is the parsed tree resulted from compilation. 
        bool wasParsed;
        bool wasDeclarationInitialized;
        bool isRootModule;
        bool hadNotifyHostReady;
        ParseNodePtr parseTree;
        Utf8SourceInfo* pSourceInfo;
        uint sourceIndex;
        Parser* parser;  // we'll need to keep the parser around till we are done with bytecode gen.
        ScriptContext* scriptContext;
        IdentPtrList* requestedModuleList;
        ModuleImportOrExportEntryList* importRecordList;
        ModuleImportOrExportEntryList* localExportRecordList;
        ModuleImportOrExportEntryList* indirectExportRecordList;
        ModuleImportOrExportEntryList* starExportRecordList;
        ChildModuleRecordSet* childrenModuleSet;
        ModuleRecordList* parentModuleList;
        LocalExportMap* localExportMapByExportName;  // from propertyId to index map: for bytecode gen.
        LocalExportMap* localExportMapByLocalName;  // from propertyId to index map: for bytecode gen.
        LocalExportIndexList* localExportIndexList; // from index to propertyId: for typehandler.
        uint numUnInitializedChildrenModule;
        ExportedNames* exportedNames;
        ResolvedExportMap* resolvedExportMap;

        Js::JavascriptFunction* rootFunction;
        void* hostDefined;
        Var normalizedSpecifier;
        Var errorObject;
        Var* localExportSlots;
        uint localSlotCount;

        // module export allows aliasing, like export {foo as foo1, foo2, foo3}.
        uint localExportCount;
        uint moduleId;

        ModuleNameRecord namespaceRecord;

        HRESULT PostParseProcess();
        HRESULT PrepareForModuleDeclarationInitialization();
        void ImportModuleListsFromParser();
        HRESULT OnChildModuleReady(SourceTextModuleRecord* childModule, Var errorObj);
        void NotifyParentsAsNeeded();
        void CleanupBeforeExecution();
        void InitializeLocalImports();
        void InitializeLocalExports();
        void InitializeIndirectExports();
        PropertyId EnsurePropertyIdForIdentifier(IdentPtr pid);
        LocalExportMap* GetLocalExportMap() const { return localExportMapByExportName; }
        LocalExportIndexList* GetLocalExportIndexList() const { return localExportIndexList; }
        ResolvedExportMap* GetExportedNamesMap() const { return resolvedExportMap; }
    };
}