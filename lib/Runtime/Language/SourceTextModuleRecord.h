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
        bool ResolveExport(PropertyId exportName, ResolveSet* resolveSet, ModuleNameRecord** exportRecord) override;
        bool ResolveImport(PropertyId localName, ModuleNameRecord** importRecord);
        void ModuleDeclarationInstantiation() override;
        Var ModuleEvaluation() override;
        virtual ModuleNamespace* GetNamespace();
        virtual void SetNamespace(ModuleNamespace* moduleNamespace);

        void Finalize(bool isShutdown) override;
        void Dispose(bool isShutdown) override { return; }
        void Mark(Recycler * recycler) override { return; }

        HRESULT ResolveExternalModuleDependencies();
        void EnsureChildModuleSet(ScriptContext *scriptContext);

        void* GetHostDefined() const { return hostDefined; }
        void SetHostDefined(void* hostObj) { hostDefined = hostObj; }

        void SetSpecifier(Var specifier) { this->normalizedSpecifier = specifier; }
        Var GetSpecifier() const { return normalizedSpecifier; }
        const char16 *GetSpecifierSz() const { return JavascriptString::FromVar(this->normalizedSpecifier)->GetSz(); }

        Var GetErrorObject() const { return errorObject; }

        bool WasParsed() const { return wasParsed; }
        void SetWasParsed() { wasParsed = true; }
        bool WasDeclarationInitialized() const { return wasDeclarationInitialized; }
        void SetWasDeclarationInitialized() { wasDeclarationInitialized = true; }
        void SetIsRootModule() { isRootModule = true; }
        JavascriptPromise *GetPromise() { return this->promise; }
        void SetPromise(JavascriptPromise *value) { this->promise = value; }

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
        Field(Var)* GetLocalExportSlots() const { return localExportSlots; }
        uint GetLocalExportSlotCount() const { return localSlotCount; }
        uint GetModuleId() const { return moduleId; }
        uint GetLocalExportCount() const { return localExportCount; }

        ModuleNameRecord* GetNamespaceNameRecord() { return &namespaceRecord; }

        SourceTextModuleRecord* GetChildModuleRecord(LPCOLESTR specifier) const;

        void SetParent(SourceTextModuleRecord* parentRecord, LPCOLESTR moduleName);
        Utf8SourceInfo* GetSourceInfo() { return this->pSourceInfo; }
        static Var ResolveOrRejectDynamicImportPromise(bool isResolve, Var value, ScriptContext *scriptContext, SourceTextModuleRecord *mr = nullptr);
        Var PostProcessDynamicModuleImport();

    private:
        const static uint InvalidModuleIndex = 0xffffffff;
        const static uint InvalidSlotCount = 0xffffffff;
        const static uint InvalidSlotIndex = 0xffffffff;
        // TODO: move non-GC fields out to avoid false reference?
        // This is the parsed tree resulted from compilation. 
        Field(bool) wasParsed;
        Field(bool) wasDeclarationInitialized;
        Field(bool) parentsNotified;
        Field(bool) isRootModule;
        Field(bool) hadNotifyHostReady;
        Field(ParseNodePtr) parseTree;
        Field(Utf8SourceInfo*) pSourceInfo;
        Field(uint) sourceIndex;
        FieldNoBarrier(Parser*) parser;  // we'll need to keep the parser around till we are done with bytecode gen.
        Field(ScriptContext*) scriptContext;
        Field(IdentPtrList*) requestedModuleList;
        Field(ModuleImportOrExportEntryList*) importRecordList;
        Field(ModuleImportOrExportEntryList*) localExportRecordList;
        Field(ModuleImportOrExportEntryList*) indirectExportRecordList;
        Field(ModuleImportOrExportEntryList*) starExportRecordList;
        Field(ChildModuleRecordSet*) childrenModuleSet;
        Field(ModuleRecordList*) parentModuleList;
        Field(LocalExportMap*) localExportMapByExportName;  // from propertyId to index map: for bytecode gen.
        Field(LocalExportMap*) localExportMapByLocalName;  // from propertyId to index map: for bytecode gen.
        Field(LocalExportIndexList*) localExportIndexList; // from index to propertyId: for typehandler.
        Field(uint) numPendingChildrenModule;
        Field(ExportedNames*) exportedNames;
        Field(ResolvedExportMap*) resolvedExportMap;

        Field(Js::JavascriptFunction*) rootFunction;
        Field(void*) hostDefined;
        Field(Var) normalizedSpecifier;
        Field(Var) errorObject;
        Field(Field(Var)*) localExportSlots;
        Field(uint) localSlotCount;

        // module export allows aliasing, like export {foo as foo1, foo2, foo3}.
        Field(uint) localExportCount;
        Field(uint) moduleId;

        Field(ModuleNameRecord) namespaceRecord;
        Field(JavascriptPromise*) promise;

        HRESULT PostParseProcess();
        HRESULT PrepareForModuleDeclarationInitialization();
        void ImportModuleListsFromParser();
        HRESULT OnChildModuleReady(SourceTextModuleRecord* childModule, Var errorObj);
        void NotifyParentsAsNeeded();
        void CleanupBeforeExecution();
        void InitializeLocalImports();
        void InitializeLocalExports();
        void InitializeIndirectExports();
        bool ParentsNotified() const { return parentsNotified; }
        void SetParentsNotified() { parentsNotified = true; }
        PropertyId EnsurePropertyIdForIdentifier(IdentPtr pid);
        LocalExportMap* GetLocalExportMap() const { return localExportMapByExportName; }
        LocalExportIndexList* GetLocalExportIndexList() const { return localExportIndexList; }
        ResolvedExportMap* GetExportedNamesMap() const { return resolvedExportMap; }
    };

    struct ServerSourceTextModuleRecord
    {
        uint moduleId;
        Field(Var)* localExportSlotsAddr;
    };
}