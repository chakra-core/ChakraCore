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
        requestModuleList(nullptr),
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
        isRootModule(false)
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
        scriptContext->BindReference(childModuleRecord);
        return childModuleRecord;
    }

    HRESULT SourceTextModuleRecord::ParseSource(__in_bcount(sourceLength) byte* sourceText, unsigned long sourceLength, Var* exceptionVar, bool isUtf8)
    {
        Assert(!wasParsed);
        Assert(parser == nullptr);
        HRESULT hr = NOERROR;
        ScriptContext* scriptContext = GetScriptContext();
        CompileScriptException se;
        TempArenaAllocatorObject* allocatorObject = EnsureTempAllocator();
        ArenaAllocator* allocator = allocatorObject->GetAllocator();
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            this->parser = (Parser*)AllocatorNew(ArenaAllocator, allocator, Parser, scriptContext);
            LoadScriptFlag loadScriptFlag = (LoadScriptFlag)(LoadScriptFlag_Expression | LoadScriptFlag_Module |
                (isUtf8 ? LoadScriptFlag_Utf8Source : LoadScriptFlag_None));
            this->parseTree = scriptContext->ParseScript(parser, sourceText, sourceLength, nullptr, &se, &pSourceInfo, L"module", loadScriptFlag, &sourceIndex);
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
    }

    HRESULT SourceTextModuleRecord::PostParseProcess()
    {
        HRESULT hr = NOERROR;
        SetWasParsed();
        ImportModuleListsFromParser();
        ResolveExternalModuleDependencies();

        if (parentModuleList != nullptr)
        {
            parentModuleList->Map([=](SourceTextModuleRecord* parentModule)
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

    TempArenaAllocatorObject* SourceTextModuleRecord::EnsureTempAllocator()
    {
        if (tempAllocatorObject == nullptr)
        {
            tempAllocatorObject = scriptContext->GetThreadContext()->GetTemporaryAllocator(L"Module");
        }
        return tempAllocatorObject;
    }

    bool SourceTextModuleRecord::ResolveExport(PropertyId exportName, ResolutionDictionary* resolveSet, ResolveSet* exportStarSet, ModuleNameRecord* exportRecord)
    {
        return false;
    }

    void SourceTextModuleRecord::ResolveExternalModuleDependencies()
    {
        ScriptContext* scriptContext = GetScriptContext();
        if (requestModuleList != nullptr)
        {
            if (nullptr == childrenModuleSet)
            {
                TempArenaAllocatorObject* allocatorObject = EnsureTempAllocator();
                ArenaAllocator* allocator = allocatorObject->GetAllocator();
                childrenModuleSet = (ChildModuleRecordSet*)AllocatorNew(ArenaAllocator, allocator, ChildModuleRecordSet, allocator);
            }
            requestModuleList->Map([=](LiteralString* specifier) {
                ModuleRecordBase* moduleRecordBase = nullptr;
                SourceTextModuleRecord* moduleRecord = nullptr;
                bool itemFound = childrenModuleSet->TryGetValue(specifier, &moduleRecord);
                if (!itemFound)
                {
                    HRESULT result = scriptContext->GetHostScriptContext()->FetchImportedModule(this, specifier, &moduleRecordBase);
                    if (result == NOERROR)
                    {
                        moduleRecord = SourceTextModuleRecord::FromHost(moduleRecordBase);
                        childrenModuleSet->AddNew(specifier, moduleRecord);
                        if (moduleRecord->parentModuleList == nullptr)
                        {
                            TempArenaAllocatorObject* allocatorObject = EnsureTempAllocator();
                            ArenaAllocator* allocator = allocatorObject->GetAllocator();
                            moduleRecord->parentModuleList = AllocatorNew(ArenaAllocator, allocator, ModuleRecordList, allocator);
                        }
                        moduleRecord->parentModuleList->Prepend(this);
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
        Assert(wasDeclarationInitialized);
        Assert(!wasEvaluated);
        parseTree = nullptr;
        requestModuleList = nullptr;
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
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        uint sourceIndex = scriptContext->SaveSourceNoCopy(this->pSourceInfo, static_cast<charcount_t>(this->pSourceInfo->GetCchLength()), /*isCesu8*/ true);
        // BUGBUG: pse
        this->rootFunction = scriptContext->GenerateRootFunction(parseTree, sourceIndex, this->parser, this->pSourceInfo->GetParseFlags(), nullptr, L"module");
        SetWasDeclarationInitialized();
        CleanupBeforeExecution();
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
#if DBG
    void SourceTextModuleRecord::AddParent(SourceTextModuleRecord* parentRecord, LPCWSTR specifier, unsigned long specifierLength)
    {

    }
#endif
}