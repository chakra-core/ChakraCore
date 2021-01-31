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
        friend class JavascriptLibrary;

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
        bool ModuleDeclarationInstantiation() override;
        void GenerateRootFunction();
        Var ModuleEvaluation() override;
        void FinishModuleEvaluation(bool shouldIncrementAwait);
        bool ModuleEvaluationPrepass();
        void DecrementAndEvaluateIfNothingAwaited();
        void PropogateRejection(Var reason);
        virtual ModuleNamespace* GetNamespace();
        virtual void SetNamespace(ModuleNamespace* moduleNamespace);

        void Finalize(bool isShutdown) override;
        void Dispose(bool isShutdown) override { return; }
        void Mark(Recycler * recycler) override { return; }

        HRESULT ResolveExternalModuleDependencies();
        void EnsureChildModuleSet(ScriptContext *scriptContext);
        bool ConfirmChildrenParsed();

        void* GetHostDefined() const { return hostDefined; }
        void SetHostDefined(void* hostObj) { hostDefined = hostObj; }

        void SetSpecifier(Var specifier) { this->normalizedSpecifier = specifier; }
        Var GetSpecifier() const { return normalizedSpecifier; }
        const char16 *GetSpecifierSz() const
        {
            return this->normalizedSpecifier != nullptr ? 
                VarTo<JavascriptString>(this->normalizedSpecifier)->GetSz() : _u("module"); 
        }

        Var GetErrorObject() const { return errorObject; }

        bool WasParsed() const { return wasParsed; }
        void SetWasParsed() { wasParsed = true; }
        bool WasEvaluationPrepassed() const { return wasPrepassed; }
        void SetEvaluationPrepassed() { wasPrepassed = true; }
        bool IsEvaluating() const { return evaluating; }
        void SetEvaluating(bool status) { evaluating = status; }
        void IncrementAwaited() { ++awaitedModules; }
        bool DecrementAwaited() { return (--awaitedModules) == 0; }
        bool WasDeclarationInitialized() const { return wasDeclarationInitialized; }
        void SetWasDeclarationInitialized() { wasDeclarationInitialized = true; }
        void SetIsRootModule() { isRootModule = true; }
        bool GetIsRootModule() { return isRootModule; }
        JavascriptPromise *GetPromise() { return this->promise; }
        void SetPromise(JavascriptPromise *value) { this->promise = value; }

        Var GetImportMetaObject();

        void SetImportRecordList(ModuleImportOrExportEntryList* importList) { importRecordList = importList; }
        void SetLocalExportRecordList(ModuleImportOrExportEntryList* localExports) { localExportRecordList = localExports; }
        void SetIndirectExportRecordList(ModuleImportOrExportEntryList* indirectExports) { indirectExportRecordList = indirectExports; }
        void SetStarExportRecordList(ModuleImportOrExportEntryList* starExports) { starExportRecordList = starExports; }
        void SetRequestedModuleList(IdentPtrList* requestModules) { requestedModuleList = requestModules; }

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

        static Var EntryAsyncModuleFulfilled(
            RecyclableObject* function,
            CallInfo callInfo, ...);

        static Var EntryAsyncModuleRejected(
            RecyclableObject* function,
            CallInfo callInfo, ...);

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
        static Var ResolveOrRejectDynamicImportPromise(bool isResolve, Var value, ScriptContext *scriptContext, SourceTextModuleRecord *mr = nullptr, bool useReturn = true);
        Var PostProcessDynamicModuleImport();

    private:
        const static uint InvalidModuleIndex = 0xffffffff;
        const static uint InvalidSlotCount = 0xffffffff;
        const static uint InvalidSlotIndex = 0xffffffff;
        // TODO: move non-GC fields out to avoid false reference?
        Field(bool) confirmedReady = false;
        Field(bool) notifying = false;
        Field(bool) wasPrepassed = false;
        Field(bool) wasParsed = false;
        Field(bool) wasDeclarationInitialized = false;
        Field(bool) parentsNotified = false;
        Field(bool) isRootModule = false;
        Field(bool) hadNotifyHostReady = false;
        Field(bool) evaluating = false;
        Field(JavascriptGenerator*) generator;
        Field(ParseNodeProg *) parseTree; // This is the parsed tree resulted from compilation.
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

        // for Top level Await
        Field(uint) awaitedModules;

        Field(ModuleNameRecord) namespaceRecord;
        Field(JavascriptPromise*) promise;

        Field(Var) importMetaObject;

        HRESULT PostParseProcess();
        HRESULT PrepareForModuleDeclarationInitialization();
        void ReleaseParserResources();
        void ReleaseParserResourcesForHierarchy();
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

    class AsyncModuleCallbackFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(AsyncModuleCallbackFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(AsyncModuleCallbackFunction);

    public:
        AsyncModuleCallbackFunction(
            DynamicType* type,
            FunctionInfo* functionInfo,
            SourceTextModuleRecord* module) :
                RuntimeFunction(type, functionInfo),
                module(module) {}

        Field(SourceTextModuleRecord*) module;
    };

    template<>
    bool VarIsImpl<AsyncModuleCallbackFunction>(RecyclableObject* obj);

}
