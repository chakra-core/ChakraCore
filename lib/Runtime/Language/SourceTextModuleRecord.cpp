//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

namespace Js
{
    SourceTextModuleRecord::SourceTextModuleRecord(ScriptContext* scriptContext) :
        ModuleRecordBase(scriptContext->GetLibrary()),
        scriptContext(scriptContext),
        parseTree(nullptr),
        parser(nullptr),
        pSourceInfo(nullptr),
        rootFunction(nullptr),
        requestedModuleList(nullptr),
        importRecordList(nullptr),
        localExportRecordList(nullptr),
        indirectExportRecordList(nullptr),
        starExportRecordList(nullptr),
        childrenModuleSet(nullptr),
        parentModuleList(nullptr),
        errorObject(nullptr),
        hostDefined(nullptr),
        tempAllocatorObject(nullptr),
        wasParsed(false),
        wasDeclarationInitialized(false),
        isRootModule(false),
        localExportSlots(nullptr),
        numUnParsedChildrenModule(0),
        moduleId(InvalidModuleIndex),
        localSlotCount(InvalidSlotCount)
    {
    }

    SourceTextModuleRecord* SourceTextModuleRecord::Create(ScriptContext* scriptContext)
    {
        Recycler* recycler = scriptContext->GetRecycler();
        SourceTextModuleRecord* childModuleRecord;
        childModuleRecord = RecyclerNew(recycler, Js::SourceTextModuleRecord, scriptContext);
        // There is no real reference to lifetime management in ecmascript
        // The life time of a module record should be controlled by the module registry as defined in WHATWG module loader spec
        // in practice the modulerecord lifetime should be the same as the scriptcontext so it could be retrieved for the same
        // site. DOM might hold a reference to the module as well after the initialize the module. 
        // In our implementation, we'll use the moduleId in bytecode to identify the module.
        childModuleRecord->moduleId = scriptContext->GetLibrary()->EnsureModuleRecordList()->Add(childModuleRecord);
        return childModuleRecord;
    }

    HRESULT SourceTextModuleRecord::ParseSource(__in_bcount(sourceLength) byte* sourceText, unsigned long sourceLength, Var* exceptionVar, bool isUtf8)
    {
        Assert(!wasParsed);
        Assert(parser == nullptr);
        HRESULT hr = NOERROR;
        ScriptContext* scriptContext = GetScriptContext();
        CompileScriptException se;
        ArenaAllocator* allocator = EnsureTempAllocator();
        *exceptionVar = nullptr;
        if (!scriptContext->GetConfig()->IsES6ModuleEnabled())
        {
            return E_NOTIMPL;
        }
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            this->parser = (Parser*)AllocatorNew(ArenaAllocator, allocator, Parser, scriptContext);
            this->srcInfo = {};
            this->srcInfo.sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceLength, Js::Constants::NoHostSourceContext);
            // TODO: enable this for byte code generation.
            // this->srcInfo.moduleID = moduleId;
            LoadScriptFlag loadScriptFlag = (LoadScriptFlag)(LoadScriptFlag_Expression | LoadScriptFlag_Module |
                (isUtf8 ? LoadScriptFlag_Utf8Source : LoadScriptFlag_None));
            this->parseTree = scriptContext->ParseScript(parser, sourceText, sourceLength, &srcInfo, &se, &pSourceInfo, L"module", loadScriptFlag, &sourceIndex);
            if (parseTree == nullptr)
            {
                // Assert have error.
                hr = E_FAIL;
            }
        }
        catch (Js::OutOfMemoryException)
        {
            hr = E_OUTOFMEMORY;
            se.ProcessError(nullptr, E_OUTOFMEMORY, nullptr);
        }
        catch (Js::StackOverflowException)
        {
            hr = VBSERR_OutOfStack;
            se.ProcessError(nullptr, VBSERR_OutOfStack, nullptr);
        }
        if (SUCCEEDED(hr))
        {
            hr = PostParseProcess();
        }
        if (FAILED(hr))
        {
            *exceptionVar = JavascriptError::CreateFromCompileScriptException(scriptContext, &se);
            if (this->parser)
            {
                this->parseTree = nullptr;
                AllocatorDelete(ArenaAllocator, allocator, this->parser);
                this->parser = nullptr;
            }
        }
        return hr;
    }

    // TODO: read the importentries/exportentries etc.
    void SourceTextModuleRecord::ImportModuleListsFromParser()
    {
        Assert(scriptContext->GetConfig()->IsES6ModuleEnabled());
        PnModule* moduleParseNode = static_cast<PnModule*>(&this->parseTree->sxModule);
        SetrequestedModuleList(moduleParseNode->requestedModules);
        SetImportRecordList(moduleParseNode->importEntries);
        SetStarExportRecordList(moduleParseNode->starExportEntries);
        SetIndirectExportRecordList(moduleParseNode->indirectExportEntries);
        SetLocalExportRecordList(moduleParseNode->localExportEntries);
    }

    HRESULT SourceTextModuleRecord::PostParseProcess()
    {
        HRESULT hr = NOERROR;
        SetWasParsed();
        ImportModuleListsFromParser();
        ResolveExternalModuleDependencies();

        if (parentModuleList != nullptr)
        {
            parentModuleList->Map([=](uint i, SourceTextModuleRecord* parentModule)
            {
                parentModule->OnChildModuleReady(this, this->errorObject);
            });
        }
        if (numUnParsedChildrenModule == 0 && !WasDeclarationInitialized() && isRootModule)
        {
            // TODO: move this as a promise call? if parser is called from a different thread
            // We'll need to call the bytecode gen in the main thread as we are accessing GC.
            ModuleDeclarationInstantiation();
            hr = GetScriptContext()->GetHostScriptContext()->NotifyHostAboutModuleReady(this, errorObject);
        }
        return hr;
    }

    HRESULT SourceTextModuleRecord::OnChildModuleReady(SourceTextModuleRecord* childModule, Var childException)
    {
        HRESULT hr = NOERROR;
        if (numUnParsedChildrenModule == 0)
        {
            return NOERROR; // this is only in case of recursive module reference. Let the higher stack frame handle this module.
        }
        numUnParsedChildrenModule--;
        if (numUnParsedChildrenModule == 0 && !WasDeclarationInitialized() && isRootModule)
        {
            ModuleDeclarationInstantiation();
            hr = GetScriptContext()->GetHostScriptContext()->NotifyHostAboutModuleReady(this, errorObject);
        }
        return hr;
    }

    ArenaAllocator* SourceTextModuleRecord::EnsureTempAllocator()
    {
        if (tempAllocatorObject == nullptr)
        {
            tempAllocatorObject = scriptContext->GetThreadContext()->GetTemporaryAllocator(L"Module");
        }
        return tempAllocatorObject->GetAllocator();
    }

    bool SourceTextModuleRecord::ResolveExport(PropertyId exportName, ResolutionDictionary* resolveSet, ResolveSet* exportStarSet, ModuleNameRecord** exportRecord)
    {
        ArenaAllocator* allocator = EnsureTempAllocator();
        if (resolveSet == nullptr)
        {
            resolveSet = (ResolutionDictionary*)AllocatorNew(ArenaAllocator, allocator, ResolutionDictionary, allocator);
        }
        if (exportStarSet == nullptr)
        {
            exportStarSet = (ResolveSet*)AllocatorNew(ArenaAllocator, allocator, ResolveSet, allocator);
        }
        Assert(false);
        return false;
    }

    void SourceTextModuleRecord::ResolveExternalModuleDependencies()
    {
        ScriptContext* scriptContext = GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();
        if (requestedModuleList != nullptr)
        {
            if (nullptr == childrenModuleSet)
            {
                ArenaAllocator* allocator = EnsureTempAllocator();
                childrenModuleSet = (ChildModuleRecordSet*)AllocatorNew(ArenaAllocator, allocator, ChildModuleRecordSet, allocator);
            }
            requestedModuleList->Map([=](IdentPtr specifier) {
                ModuleRecordBase* moduleRecordBase = nullptr;
                SourceTextModuleRecord* moduleRecord = nullptr;
                LPCOLESTR moduleName = specifier->Psz();
                bool itemFound = childrenModuleSet->TryGetValue(moduleName, &moduleRecord);
                if (!itemFound)
                {
                    HRESULT result = scriptContext->GetHostScriptContext()->FetchImportedModule(this, moduleName, &moduleRecordBase);
                    if (result == NOERROR)
                    {
                        moduleRecord = SourceTextModuleRecord::FromHost(moduleRecordBase);
                        childrenModuleSet->AddNew(moduleName, moduleRecord);
                        if (moduleRecord->parentModuleList == nullptr)
                        {
                            moduleRecord->parentModuleList = RecyclerNew(recycler, ModuleRecordList, recycler);
                        }
                        moduleRecord->parentModuleList->Add(this);
                        if (!moduleRecord->WasParsed())
                        {
                            numUnParsedChildrenModule++;
                        }
                    }
                }
            });
        }
    }

    void SourceTextModuleRecord::CleanupBeforeExecution()
    {
        // zero out fields is more a defense in depth as those fields are not needed anymore
        Assert(wasParsed);
        Assert(!wasEvaluated);
        parseTree = nullptr;
        requestedModuleList = nullptr;
        importRecordList = nullptr;
        localExportRecordList = nullptr;
        indirectExportRecordList = nullptr;
        starExportRecordList = nullptr;
        childrenModuleSet = nullptr;
        parentModuleList = nullptr;
        AllocatorDelete(ArenaAllocator, tempAllocatorObject->GetAllocator(), parser);
        parser = nullptr;
        GetScriptContext()->GetThreadContext()->ReleaseTemporaryAllocator(tempAllocatorObject);
        tempAllocatorObject = nullptr;
    }

    void SourceTextModuleRecord::ModuleDeclarationInstantiation()
    {
        ScriptContext* scriptContext = GetScriptContext();
        SetWasDeclarationInitialized();
        if (childrenModuleSet != nullptr)
        {
            childrenModuleSet->Map([](LPCOLESTR specifier, SourceTextModuleRecord* moduleRecord)
            {
                Assert(moduleRecord->WasParsed());
                if (!moduleRecord->WasDeclarationInitialized())
                {
                    moduleRecord->ModuleDeclarationInstantiation();
                }
            });
        }
        if (this->localExportRecordList != nullptr)
        {
            InitializeLocalExports();
        }
        //if (this->indirectExportRecordList != nullptr)
        //{
        //    ModuleNameRecord* exportRecord = nullptr;
        //    indirectExportRecordList->Map([&](ModuleExportEntry exportEntry)
        //    {
        //        const PropertyRecord* propertyRecord;
        //        scriptContext->GetOrAddPropertyRecord(exportEntry.exportName->Psz(), exportEntry.exportName->Cch(), &propertyRecord);
        //        if (!this->ResolveExport(propertyRecord->GetPropertyId(), nullptr, nullptr, &exportRecord) ||
        //            (exportRecord == nullptr))
        //        {
        //            JavascriptError::ThrowSyntaxError(GetScriptContext(), JSERR_ModuleResolveExport, exportEntry.exportName->Psz());
        //        }
        //    });
        //}
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        // TODO: enable this after fixing bytecode gen.
        //Assert(this == scriptContext->GetLibrary()->GetModuleRecord(srcInfo.moduleID));
        uint sourceIndex = scriptContext->SaveSourceNoCopy(this->pSourceInfo, static_cast<charcount_t>(this->pSourceInfo->GetCchLength()), /*isCesu8*/ true);
        CompileScriptException se;
         //TODO: fix up byte code generation. moduleID in the SRCINFO is the moduleId of current module.
        this->rootFunction = scriptContext->GenerateRootFunction(parseTree, sourceIndex, this->parser, this->pSourceInfo->GetParseFlags(), &se, L"module");
        CleanupBeforeExecution();
        if (rootFunction == nullptr)
        {
            this->errorObject = JavascriptError::CreateFromCompileScriptException(scriptContext, &se);
            JavascriptExceptionOperators::Throw(this->errorObject, scriptContext);
        }
    }

    Var SourceTextModuleRecord::ModuleEvaluation()
    {
#if DBG
        if (childrenModuleSet != nullptr)
        {
            childrenModuleSet->EachValue([=](SourceTextModuleRecord* childModuleRecord)
            {
                AssertMsg(childModuleRecord->WasParsed(), "child module needs to have been parsed");
                AssertMsg(childModuleRecord->WasDeclarationInitialized(), "child module needs to have been initialized.");
            });
        }
#endif
        if (!scriptContext->GetConfig()->IsES6ModuleEnabled())
        {
            return nullptr;
        }
        SetWasEvaluated();
        if (childrenModuleSet != nullptr)
        {
            childrenModuleSet->EachValue([=](SourceTextModuleRecord* childModuleRecord)
            {
                if (!childModuleRecord->WasEvaluated())
                {
                    childModuleRecord->ModuleEvaluation();
                }
            });
        }
        Arguments outArgs(CallInfo(CallFlags_Value, 0), nullptr);
        return rootFunction->CallRootFunction(outArgs, scriptContext, true);
    }

    HRESULT SourceTextModuleRecord::OnHostException(void* errorVar)
    {
        if (!RecyclableObject::Is(errorVar))
        {
            return E_INVALIDARG;
        }
        AssertMsg(!WasParsed(), "shouldn't be called after a module is parsed");
        if (WasParsed())
        {
            return E_INVALIDARG;
        }
        this->errorObject = errorVar;

        // a sub module failed to download. we need to notify that the parent that sub module failed and we need to report back.
        return NOERROR;
    }

    // Local exports are stored in the slotarray in the SourceTextModuleRecord.
    void SourceTextModuleRecord::InitializeLocalExports()
    {
        Recycler* recycler = scriptContext->GetRecycler();
        Var undefineValue = scriptContext->GetLibrary()->GetUndefined();
        if (localSlotCount == InvalidSlotCount)
        {
            if (localExportRecordList != nullptr)
            {
                uint currentSlotCount = 0;
                ArenaAllocator* allocator = EnsureTempAllocator();
                localExportMap = AllocatorNew(ArenaAllocator, allocator, LocalExportMap, allocator);
                localExportRecordList->Map([&](ModuleExportEntry exportEntry)
                {
                    Assert(exportEntry.moduleRequest == nullptr);
                    Assert(exportEntry.importName == nullptr);
                    const PropertyRecord* propertyRecord;
                    scriptContext->GetOrAddPropertyRecord(exportEntry.exportName->Psz(), exportEntry.exportName->Cch(), &propertyRecord);
                    localExportMap->Add(propertyRecord->GetPropertyId(), currentSlotCount);
                    currentSlotCount++;
                    if (currentSlotCount >= UINT_MAX)
                    {
                        JavascriptError::ThrowRangeError(scriptContext, JSERR_TooManyImportExprots);
                    }
                });
                localExportSlots = RecyclerNewArray(recycler, Var, currentSlotCount);
                for (uint i = 0; i < currentSlotCount; i++)
                {
                    localExportSlots[i] = undefineValue;
                }
                localSlotCount = currentSlotCount;
            }
            else
            {
                localSlotCount = 0;
            }
        }
    }

    uint SourceTextModuleRecord::GetLocalExportSlotIndex(PropertyId exportNameId)
    {
        Assert(localSlotCount != 0);
        Assert(localExportSlots != nullptr);
        uint slotIndex = InvalidSlotIndex;
        if (!localExportMap->TryGetValue(exportNameId, &slotIndex))
        {
            AssertMsg(false, "exportNameId is not in local export list");
            return InvalidSlotIndex;
        }
        else
        {
            return slotIndex;
        }
    }

#if DBG
    void SourceTextModuleRecord::AddParent(SourceTextModuleRecord* parentRecord, LPCWSTR specifier, unsigned long specifierLength)
    {

    }
#endif
}