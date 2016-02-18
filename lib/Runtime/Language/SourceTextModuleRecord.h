//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ModuleImportRecord
    {
    public:
        ModuleImportRecord();
        LiteralString* GetImportName() const { return importName; }
        LiteralString* GetLocalName() const { return localName; }
        LiteralString* GetModuleRequest() const { return moduleRequest; }

    private:
        LiteralString* moduleRequest;
        LiteralString* importName;
        LiteralString* localName;
    };

    class ModuleExportRecord
    {
    public:
        ModuleExportRecord() :
            moduleRequest(nullptr), importName(nullptr),
            exportName(nullptr), localName(nullptr) {};
        ModuleExportRecord(LiteralString* moduleRequest, LiteralString* importName, LiteralString* exportName, LiteralString* localName) :
            moduleRequest(moduleRequest), importName(importName), exportName(exportName), localName(localName) {}
        LiteralString* GetModuleRequest() const { return moduleRequest; }
        LiteralString* GetImportName() const { return importName; }
        LiteralString* GetExportName() const { return exportName; }
        LiteralString* GetLocalname() const { return localName; }
    private:
        LiteralString* localName, *exportName, *importName, *moduleRequest;
    };

    class SourceTextModuleRecord;
    typedef SList<LiteralString*> ModuleNameList;
    typedef SList<ModuleImportRecord*> ModuleImportRecordList;
    typedef SList<ModuleExportRecord*> ModuleExportRecordList;
    typedef SList<SourceTextModuleRecord*> ModuleRecordList;
    typedef  JsUtil::BaseDictionary<LiteralString*, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ChildModuleRecordSet;
    typedef  JsUtil::BaseDictionary<SourceTextModuleRecord*, SourceTextModuleRecord*, ArenaAllocator, PowerOf2SizePolicy> ParentModuleRecordSet;

    class SourceTextModuleRecord : public ModuleRecordBase
    {
    public:
        SourceTextModuleRecord(ScriptContext* scriptContext);
        ModuleNameList* GetRequestModuleList() const { return requestModuleList; }
        ModuleImportRecordList* GetImportRecordList() const { return importRecordList; }
        ModuleExportRecordList* GetLocalExportRecordList() const { return localExportRecordList; }
        ModuleExportRecordList* GetIndirectExportRecordList() const { return indirectExportRecordList; }
        ModuleExportRecordList* GetStarExportRecordList() const { return starExportRecordList; }
        ExportedNames* GetExportedNames(ResolveSet* exportStarSet) override { Assert(false); return nullptr; }
        // return false when "ambiguous". otherwise exportRecord.
        bool ResolveExport(PropertyId exportName, ResolutionDictionary* resolveSet, ResolveSet* exportStarSet, ModuleNameRecord* exportRecord) override;
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

        void SetImportRecordList(ModuleImportRecordList* importList) { importRecordList = importList; }
        void SetLocalExportRecordList(ModuleExportRecordList* localExports) { localExportRecordList = localExports; }
        void SetIndirectExportRecordList(ModuleExportRecordList* indirectExports) { indirectExportRecordList = indirectExports; }
        void SetStarExportRecordList(ModuleExportRecordList* starExports) { starExportRecordList = starExports; }
        void SetRequestModuleList(ModuleNameList* requestModules) { requestModuleList = requestModules; }

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
#if DBG
        void AddParent(SourceTextModuleRecord* parentRecord, LPCWSTR specifier, unsigned long specifierLength);
#endif

    private:
        // This is the parsed tree resulted from compilation. 
        bool wasParsed;
        bool wasDeclarationInitialized;
        bool isRootModule;
        ParseNodePtr parseTree;
//         SRCINFO scrInfo; // debugger support.
        Utf8SourceInfo* pSourceInfo;
        uint sourceIndex;
        Parser* parser;  // we'll need to keep the parser around till we are done with bytecode gen.
        Js::JavascriptFunction* rootFunction;
        ScriptContext* scriptContext;
        ModuleNameList* requestModuleList;
        ModuleImportRecordList* importRecordList;
        ModuleExportRecordList* localExportRecordList;
        ModuleExportRecordList* indirectExportRecordList;
        ModuleExportRecordList* starExportRecordList;
        void* hostDefined;
        ChildModuleRecordSet* childrenModuleSet;
        ModuleRecordList* parentModuleList;
        Var errorObject;

        uint numUnParsedChildrenModule;
        TempArenaAllocatorObject* tempAllocatorObject;

        TempArenaAllocatorObject* EnsureTempAllocator();
        HRESULT PostParseProcess();
        void ImportModuleListsFromParser();
        HRESULT OnChildModuleReady(SourceTextModuleRecord* childModule, Var errorObj);
        void CleanupBeforeExecution();
    };
}