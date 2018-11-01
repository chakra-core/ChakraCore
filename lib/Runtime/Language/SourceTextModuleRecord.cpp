//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/SimpleDictionaryPropertyDescriptor.h"
#include "Types/SimpleDictionaryTypeHandler.h"
#include "ModuleNamespace.h"
#include "Library/JavascriptPromise.h"

namespace Js
{
    const uint32 ModuleRecordBase::ModuleMagicNumber = *(const uint32*)"Mode";

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
        localExportMapByExportName(nullptr),
        localExportMapByLocalName(nullptr),
        localExportIndexList(nullptr),
        normalizedSpecifier(nullptr),
        moduleUrl(nullptr),
        errorObject(nullptr),
        hostDefined(nullptr),
        exportedNames(nullptr),
        resolvedExportMap(nullptr),
        wasParsed(false),
        wasDeclarationInitialized(false),
        parentsNotified(false),
        isRootModule(false),
        hadNotifyHostReady(false),
        localExportSlots(nullptr),
        moduleId(InvalidModuleIndex),
        localSlotCount(InvalidSlotCount),
        promise(nullptr),
        localExportCount(0)
    {
        namespaceRecord.module = this;
        namespaceRecord.bindingName = PropertyIds::star_;
    }

    SourceTextModuleRecord* SourceTextModuleRecord::Create(ScriptContext* scriptContext)
    {
        Recycler* recycler = scriptContext->GetRecycler();
        SourceTextModuleRecord* childModuleRecord;
        childModuleRecord = RecyclerNewFinalized(recycler, Js::SourceTextModuleRecord, scriptContext);
        // There is no real reference to lifetime management in ecmascript
        // The life time of a module record should be controlled by the module registry as defined in WHATWG module loader spec
        // in practice the modulerecord lifetime should be the same as the scriptcontext so it could be retrieved for the same
        // site. Host might hold a reference to the module as well after initializing the module.
        // In our implementation, we'll use the moduleId in bytecode to identify the module.
        childModuleRecord->moduleId = scriptContext->GetLibrary()->EnsureModuleRecordList()->Add(childModuleRecord);

        return childModuleRecord;
    }

    void SourceTextModuleRecord::Finalize(bool isShutdown)
    {
        parseTree = nullptr;
        requestedModuleList = nullptr;
        importRecordList = nullptr;
        localExportRecordList = nullptr;
        indirectExportRecordList = nullptr;
        starExportRecordList = nullptr;
        parentModuleList = nullptr;
        if (!isShutdown)
        {
            AdeleteUnlessNull(scriptContext->GeneralAllocator(), parser);
            AdeleteUnlessNull(scriptContext->GeneralAllocator(), childrenModuleSet);
        }
    }

    HRESULT SourceTextModuleRecord::ParseSource(__in_bcount(sourceLength) byte* sourceText, uint32 sourceLength, SRCINFO * srcInfo, Var* exceptionVar, bool isUtf8)
    {
        Assert(!wasParsed || sourceText == nullptr);
        Assert(parser == nullptr);
        Assert(exceptionVar != nullptr);
        HRESULT hr = NOERROR;

        // Return if loading failure has been reported
        if (sourceText == nullptr && wasParsed)
        {
            return hr;
        }

        ScriptContext* scriptContext = GetScriptContext();
        CompileScriptException se;
        ArenaAllocator* allocator = scriptContext->GeneralAllocator();
        *exceptionVar = nullptr;
        if (!scriptContext->GetConfig()->IsES6ModuleEnabled())
        {
            JavascriptError *pError = scriptContext->GetLibrary()->CreateError();
            JavascriptError::SetErrorMessageProperties(pError, hr, _u("ES6Module not supported"), scriptContext);
            *exceptionVar = pError;
            return E_NOTIMPL;
        }

        // Host indicates that the current module failed to load.
        if (sourceText == nullptr)
        {
            Assert(sourceLength == 0);
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("Failed to load: %s\n"), this->GetSpecifierSz());
            hr = E_FAIL;

            // We cannot just use the buffer in the specifier string - need to make a copy here.
            const char16* moduleName = this->GetSpecifierSz();
            size_t length = wcslen(moduleName);
            char16* allocatedString = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, length + 1);
            wmemcpy_s(allocatedString, length + 1, moduleName, length);
            allocatedString[length] = _u('\0');

            JavascriptError *pError = scriptContext->GetLibrary()->CreateURIError();
            JavascriptError::SetErrorMessageProperties(pError, hr, allocatedString, scriptContext);
            *exceptionVar = pError;
        }
        else
        {
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("ParseSource(%s)\n"), this->GetSpecifierSz());
            try
            {
                AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
                this->parser = Anew(allocator, Parser, scriptContext);
                srcInfo->moduleID = moduleId;

                LoadScriptFlag loadScriptFlag = (LoadScriptFlag)(LoadScriptFlag_Expression | LoadScriptFlag_Module |
                    LoadScriptFlag_disableAsmJs | (isUtf8 ? LoadScriptFlag_Utf8Source : LoadScriptFlag_None));

                Utf8SourceInfo* pResultSourceInfo = nullptr;
                this->parseTree = scriptContext->ParseScript(parser, sourceText,
                    sourceLength, srcInfo, &se, &pResultSourceInfo, _u("module"),
                    loadScriptFlag, &sourceIndex, nullptr);
                this->pSourceInfo = pResultSourceInfo;
            }
            catch (Js::OutOfMemoryException)
            {
                hr = E_OUTOFMEMORY;
            }
            catch (Js::StackOverflowException)
            {
                hr = VBSERR_OutOfStack;
            }

            if (FAILED(hr))
            {
                se.ProcessError(nullptr, hr, nullptr);
            }
            else if (parseTree == nullptr)
            {
                hr = E_FAIL;
            }
            else
            {
                hr = PostParseProcess();
                if (hr == S_OK && this->errorObject != nullptr && this->hadNotifyHostReady)
                {
                    // This would be the case where the child module got error and current module has notified error already.
                    if (*exceptionVar == nullptr)
                    {
                        *exceptionVar = this->errorObject;
                    }

                    // Cleanup in case of error.
                    this->ReleaseParserResources();
                    return E_FAIL;
                }
            }
        }

        if (FAILED(hr))
        {
            if (*exceptionVar == nullptr)
            {
                const WCHAR * sourceUrl = nullptr;
                if (this->GetModuleUrl())
                {
                  sourceUrl = this->GetModuleUrlSz();
                }
                *exceptionVar = JavascriptError::CreateFromCompileScriptException(scriptContext, &se, sourceUrl);
            }
            if (this->parser)
            {
                this->parseTree = nullptr;
                Adelete(allocator, this->parser);
                this->parser = nullptr;
            }
            if (this->errorObject == nullptr)
            {
                this->errorObject = *exceptionVar;
            }

            if (this->promise != nullptr)
            {
                SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, this->errorObject, this->scriptContext, this, false);
            }

            // Notify host if current module is dynamically-loaded module, or is root module and the host hasn't been notified
            if (this->promise != nullptr || (isRootModule && !hadNotifyHostReady))
            {
                OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyHostAboutModuleReady %s (ParseSource error)\n"), this->GetSpecifierSz());
                LEAVE_SCRIPT_IF_ACTIVE(scriptContext,
                {
                    scriptContext->GetHostScriptContext()->NotifyHostAboutModuleReady(this, this->errorObject);
                });

                hadNotifyHostReady = true;
            }

            // Notify parent if applicable
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyParentAsNeeded\n"));
            NotifyParentsAsNeeded();
        }
        return hr;
    }

    void SourceTextModuleRecord::NotifyParentsAsNeeded()
    {
        if (notifying)
        {
            return;
        }
        notifying = true;
        // Notify the parent modules that this child module is either in fault state or finished.
        if (this->parentModuleList != nullptr)
        {
            parentModuleList->Map([=](uint i, SourceTextModuleRecord* parentModule)
            {
                parentModule->OnChildModuleReady(this, this->errorObject);
            });
        }
        notifying = false;
        SetParentsNotified();
    }

    void SourceTextModuleRecord::ImportModuleListsFromParser()
    {
        Assert(scriptContext->GetConfig()->IsES6ModuleEnabled());
        ParseNodeModule* moduleParseNode = this->parseTree->AsParseNodeModule();
        SetrequestedModuleList(moduleParseNode->requestedModules);
        SetImportRecordList(moduleParseNode->importEntries);
        SetStarExportRecordList(moduleParseNode->starExportEntries);
        SetIndirectExportRecordList(moduleParseNode->indirectExportEntries);
        SetLocalExportRecordList(moduleParseNode->localExportEntries);
    }

    void SourceTextModuleRecord::ReleaseParserResources()
    {
        if (this->parser != nullptr)
        {
            this->parser->ReleaseTemporaryGuestArena();
        }
    }

    HRESULT SourceTextModuleRecord::PostParseProcess()
    {
        HRESULT hr = NOERROR;
        SetWasParsed();
        ImportModuleListsFromParser();
        hr = ResolveExternalModuleDependencies();

        if (SUCCEEDED(hr))
        {
            hr = PrepareForModuleDeclarationInitialization();
        }
        else
        {
            // Cleanup in case of error.
            this->ReleaseParserResources();
        }

        return hr;
    }

    Var SourceTextModuleRecord::PostProcessDynamicModuleImport()
    {
        JavascriptPromise *promise = this->GetPromise();
        ScriptContext* scriptContext = GetScriptContext();
        AnalysisAssert(scriptContext != nullptr);
        if (promise == nullptr)
        {
            promise = JavascriptPromise::CreateEnginePromise(scriptContext);
            this->SetPromise(promise);
        }

        if (this->ParentsNotified())
        {
            HRESULT hr = NOERROR;
            if (!WasDeclarationInitialized())
            {
                if (ModuleDeclarationInstantiation())
                {
                    GenerateRootFunction();
                }

                if (this->errorObject != nullptr)
                {
                    // Cleanup in case of error.
                    this->ReleaseParserResources();
                    SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, this->errorObject, scriptContext, this, false);
                }
                else
                {
                    if (!hadNotifyHostReady && !WasEvaluated())
                    {
                        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyHostAboutModuleReady %s (PostProcessDynamicModuleImport)\n"), this->GetSpecifierSz());
                        LEAVE_SCRIPT_IF_ACTIVE(scriptContext,
                        {
                            hr = scriptContext->GetHostScriptContext()->NotifyHostAboutModuleReady(this, this->errorObject);
                        });

                        hadNotifyHostReady = true;
                    }
                }
            }

            if (FAILED(hr))
            {
                // Cleanup in case of error.
                this->ReleaseParserResources();

                // We cannot just use the buffer in the specifier string - need to make a copy here.
                const char16* moduleName = this->GetSpecifierSz();
                size_t length = wcslen(moduleName);
                char16* allocatedString = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, length + 1);
                wmemcpy_s(allocatedString, length + 1, moduleName, length);
                allocatedString[length] = _u('\0');

                Js::JavascriptError * error = scriptContext->GetLibrary()->CreateURIError();
                JavascriptError::SetErrorMessageProperties(error, hr, allocatedString, scriptContext);
                return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, error, scriptContext, this);
            }
        }

        return JavascriptPromise::CreatePassThroughPromise(this->promise, scriptContext);
    }

    HRESULT SourceTextModuleRecord::PrepareForModuleDeclarationInitialization()
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("PrepareForModuleDeclarationInitialization(%s)\n"), this->GetSpecifierSz());
        HRESULT hr = NO_ERROR;

        if (ConfirmChildrenParsed())
        {
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyParentsAsNeeded\n"));
            NotifyParentsAsNeeded();

            if (!WasDeclarationInitialized() && (isRootModule || this->promise != nullptr))
            {
                // TODO: move this as a promise call? if parser is called from a different thread
                // We'll need to call the bytecode gen in the main thread as we are accessing GC.
                ScriptContext* scriptContext = GetScriptContext();
                Assert(!scriptContext->GetThreadContext()->IsScriptActive() || this->promise != nullptr);

                if (ModuleDeclarationInstantiation())
                {
                    GenerateRootFunction();
                }
                if (!hadNotifyHostReady && !WasEvaluated())
                {
                    OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyHostAboutModuleReady %s (PrepareForModuleDeclarationInitialization)\n"), this->GetSpecifierSz());
                    LEAVE_SCRIPT_IF_ACTIVE(scriptContext,
                    {
                        hr = scriptContext->GetHostScriptContext()->NotifyHostAboutModuleReady(this, this->errorObject);
                    });

                    hadNotifyHostReady = true;
                }
            }
        }
        return hr;
    }

    HRESULT SourceTextModuleRecord::OnChildModuleReady(SourceTextModuleRecord* childModule, Var childException)
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("OnChildModuleReady(%s)\n"), this->GetSpecifierSz(), childModule->GetSpecifierSz());
        HRESULT hr = NOERROR;
        if (childException != nullptr)
        {
            // propagate the error up as needed.
            if (this->errorObject == nullptr)
            {
                this->errorObject = childException;
            }

            // Cleanup in case of error.
            this->ReleaseParserResources();

            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyParentAsNeeded (childException)\n"), this->GetSpecifierSz());
            NotifyParentsAsNeeded();

            if (this->promise != nullptr)
            {
                SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, this->errorObject, this->scriptContext, this, false);
            }

            if (this->promise != nullptr || (isRootModule && !hadNotifyHostReady))
            {
                OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyHostAboutModuleReady %s (OnChildModuleReady)\n"), this->GetSpecifierSz());
                LEAVE_SCRIPT_IF_ACTIVE(scriptContext,
                {
                    hr = scriptContext->GetHostScriptContext()->NotifyHostAboutModuleReady(this, this->errorObject);
                });

                hadNotifyHostReady = true;
            }
        }
        else
        {
            hr = PrepareForModuleDeclarationInitialization();
        }
        return hr;
    }

    ModuleNamespace* SourceTextModuleRecord::GetNamespace()
    {
        Assert(localExportSlots != nullptr);
        Assert(PointerValue(localExportSlots[GetLocalExportSlotCount()]) == __super::GetNamespace());
        return (ModuleNamespace*)(void*)(localExportSlots[GetLocalExportSlotCount()]);
    }

    void SourceTextModuleRecord::SetNamespace(ModuleNamespace* moduleNamespace)
    {
        Assert(localExportSlots != nullptr);
        __super::SetNamespace(moduleNamespace);
        localExportSlots[GetLocalExportSlotCount()] = moduleNamespace;
    }


    ExportedNames* SourceTextModuleRecord::GetExportedNames(ExportModuleRecordList* exportStarSet)
    {
        const bool isRootGetExportedNames = (exportStarSet == nullptr);

        // this->exportedNames only caches root "GetExportedNames(nullptr)"
        if (isRootGetExportedNames && exportedNames != nullptr)
        {
            return exportedNames;
        }

        ArenaAllocator* allocator = scriptContext->GeneralAllocator();

        if (exportStarSet == nullptr)
        {
            exportStarSet = Anew(allocator, ExportModuleRecordList, allocator);
        }
        if (exportStarSet->Has(this))
        {
            return nullptr;
        }
        exportStarSet->Prepend(this);

        ExportedNames* tempExportedNames = nullptr;
        if (this->localExportRecordList != nullptr)
        {
            tempExportedNames = Anew(allocator, ExportedNames, allocator);
            this->localExportRecordList->Map([=](ModuleImportOrExportEntry exportEntry) {
                PropertyId exportNameId = EnsurePropertyIdForIdentifier(exportEntry.exportName);
                tempExportedNames->Prepend(exportNameId);
            });
        }
        if (this->indirectExportRecordList != nullptr)
        {
            if (tempExportedNames == nullptr)
            {
                tempExportedNames = Anew(allocator, ExportedNames, allocator);
            }
            this->indirectExportRecordList->Map([=](ModuleImportOrExportEntry exportEntry) {
                PropertyId exportedNameId = EnsurePropertyIdForIdentifier(exportEntry.exportName);
                tempExportedNames->Prepend(exportedNameId);
            });
        }
        if (this->starExportRecordList != nullptr)
        {
            if (tempExportedNames == nullptr)
            {
                tempExportedNames = Anew(allocator, ExportedNames, allocator);
            }
            this->starExportRecordList->Map([=](ModuleImportOrExportEntry exportEntry) {
                Assert(exportEntry.moduleRequest != nullptr);
                SourceTextModuleRecord* moduleRecord = nullptr;
                if (this->childrenModuleSet->TryGetValue(exportEntry.moduleRequest->Psz(), &moduleRecord))
                {
                    Assert(moduleRecord->WasParsed());
                    Assert(moduleRecord->WasDeclarationInitialized()); // we should be half way during initialization
                    Assert(!moduleRecord->WasEvaluated());
                    ExportedNames* starExportedNames = moduleRecord->GetExportedNames(exportStarSet);
                    // We are not rejecting ambiguous resolution at this time.
                    if (starExportedNames != nullptr)
                    {
                        starExportedNames->Map([&](PropertyId propertyId) {
                            if (propertyId != PropertyIds::default_ && !tempExportedNames->Has(propertyId))
                            {
                                tempExportedNames->Prepend(propertyId);
                            }
                        });
                    }
                }
#if DBG
                else
                {
                    AssertMsg(false, "dependent modules should have been initialized");
                }
#endif
            });
        }

        // this->exportedNames only caches root "GetExportedNames(nullptr)"
        if (isRootGetExportedNames)
        {
            exportedNames = tempExportedNames;
        }

        return tempExportedNames;
    }

    bool SourceTextModuleRecord::ResolveImport(PropertyId localName, ModuleNameRecord** importRecord)
    {
        *importRecord = nullptr;

        importRecordList->MapUntil([&](ModuleImportOrExportEntry& importEntry) {
            Js::PropertyId localNamePid = EnsurePropertyIdForIdentifier(importEntry.localName);
            if (localNamePid == localName)
            {
                SourceTextModuleRecord* childModule = this->GetChildModuleRecord(importEntry.moduleRequest->Psz());
                Js::PropertyId importName = EnsurePropertyIdForIdentifier(importEntry.importName);
                if (importName == Js::PropertyIds::star_)
                {
                    *importRecord = childModule->GetNamespaceNameRecord();
                }
                else
                {
                    childModule->ResolveExport(importName, nullptr, importRecord);
                }
                return true;
            }
            return false;
        });

        return *importRecord != nullptr;
    }

    // return false when "ambiguous".
    // otherwise nullptr means "null" where we have circular reference/cannot resolve.
    bool SourceTextModuleRecord::ResolveExport(PropertyId exportName, ResolveSet* resolveSet, ModuleNameRecord** exportRecord)
    {
        ArenaAllocator* allocator = scriptContext->GeneralAllocator();
        if (resolvedExportMap == nullptr)
        {
            resolvedExportMap = Anew(allocator, ResolvedExportMap, allocator);
        }
        if (resolvedExportMap->TryGetReference(exportName, exportRecord))
        {
            return true;
        }
        // TODO: use per-call/loop allocator?
        if (resolveSet == nullptr)
        {
            resolveSet = Anew(allocator, ResolveSet, allocator);
        }

        *exportRecord = nullptr;
        bool hasCircularRef = false;
        resolveSet->MapUntil([&](ModuleNameRecord moduleNameRecord) {
            if (moduleNameRecord.module == this && moduleNameRecord.bindingName == exportName)
            {
                *exportRecord = nullptr;
                hasCircularRef = true;
                return true;
            }
            return false;
        });
        if (hasCircularRef)
        {
            Assert(*exportRecord == nullptr);
            return true;
        }
        resolveSet->Prepend(ModuleNameRecord(this, exportName));

        if (localExportRecordList != nullptr)
        {
            PropertyId localNameId = Js::Constants::NoProperty;
            localExportRecordList->MapUntil([&](ModuleImportOrExportEntry exportEntry) {
                PropertyId exportNameId = EnsurePropertyIdForIdentifier(exportEntry.exportName);
                if (exportNameId == exportName)
                {
                    localNameId = EnsurePropertyIdForIdentifier(exportEntry.localName);
                    return true;
                }
                return false;
            });
            if (localNameId != Js::Constants::NoProperty)
            {
                // Check to see if we are exporting something we imported from another module without using a re-export.
                // ex: import { foo } from 'module'; export { foo };
                ModuleRecordBase* sourceModule = this;
                ModuleNameRecord* importRecord = nullptr;
                if (this->importRecordList != nullptr
                    && this->ResolveImport(localNameId, &importRecord)
                    && importRecord != nullptr)
                {
                    sourceModule = importRecord->module;
                    localNameId = importRecord->bindingName;
                }
                resolvedExportMap->AddNew(exportName, { sourceModule, localNameId });
                // return the address from Map buffer.
                resolvedExportMap->TryGetReference(exportName, exportRecord);
                return true;
            }
        }

        if (indirectExportRecordList != nullptr)
        {
            bool isAmbiguous = false;
            indirectExportRecordList->MapUntil([&](ModuleImportOrExportEntry exportEntry) {
                PropertyId reexportNameId = EnsurePropertyIdForIdentifier(exportEntry.exportName);
                if (exportName != reexportNameId)
                {
                    return false;
                }

                PropertyId importNameId = EnsurePropertyIdForIdentifier(exportEntry.importName);
                SourceTextModuleRecord* childModuleRecord = GetChildModuleRecord(exportEntry.moduleRequest->Psz());
                if (childModuleRecord == nullptr)
                {
                    JavascriptError::ThrowReferenceError(scriptContext, JSERR_CannotResolveModule, exportEntry.moduleRequest->Psz());
                }

                if (exportEntry.importName->GetPropertyId() == PropertyIds::star_)
                {
                    // export * as someName from "foo"
                    *exportRecord = childModuleRecord->GetNamespaceNameRecord();
                    return true;
                }

                isAmbiguous = !childModuleRecord->ResolveExport(importNameId, resolveSet, exportRecord);
                if (isAmbiguous)
                {
                    // ambiguous; don't need to search further
                    return true;
                }

                // found a resolution. done;
                if (*exportRecord != nullptr)
                {
                    return true;
                }
                return false;
            });
            if (isAmbiguous)
            {
                return false;
            }
            if (*exportRecord != nullptr)
            {
                return true;
            }
        }

        if (exportName == PropertyIds::default_)
        {
            JavascriptError* errorObj = scriptContext->GetLibrary()->CreateSyntaxError();
            JavascriptError::SetErrorMessage(errorObj, JSERR_ModuleResolveExport, scriptContext->GetPropertyName(exportName)->GetBuffer(), scriptContext);
            this->errorObject = errorObj;
            return false;
        }

        bool ambiguousResolution = false;
        if (this->starExportRecordList != nullptr)
        {
            ModuleNameRecord* starResolution = nullptr;
            starExportRecordList->MapUntil([&](ModuleImportOrExportEntry starExportEntry) {
                ModuleNameRecord* currentResolution = nullptr;
                SourceTextModuleRecord* childModule = GetChildModuleRecord(starExportEntry.moduleRequest->Psz());
                if (childModule == nullptr)
                {
                    JavascriptError::ThrowReferenceError(GetScriptContext(), JSERR_CannotResolveModule, starExportEntry.moduleRequest->Psz());
                }
                if (childModule->errorObject != nullptr)
                {
                    JavascriptExceptionOperators::Throw(childModule->errorObject, GetScriptContext());
                }

                // if ambiguous, return "ambiguous"
                if (!childModule->ResolveExport(exportName, resolveSet, &currentResolution))
                {
                    ambiguousResolution = true;
                    return true;
                }
                if (currentResolution != nullptr)
                {
                    if (starResolution == nullptr)
                    {
                        starResolution = currentResolution;
                    }
                    else
                    {
                        if (currentResolution->bindingName != starResolution->bindingName ||
                            currentResolution->module != starResolution->module)
                        {
                            ambiguousResolution = true;
                        }
                        return true;
                    }
                }
                return false;
            });
            if (!ambiguousResolution)
            {
                *exportRecord = starResolution;
            }
        }
        return !ambiguousResolution;
    }

    void SourceTextModuleRecord::SetParent(SourceTextModuleRecord* parentRecord, LPCOLESTR moduleName)
    {
        Assert(parentRecord != nullptr);
        parentRecord->EnsureChildModuleSet(GetScriptContext());
        if (!parentRecord->childrenModuleSet->ContainsKey(moduleName))
        {
            parentRecord->childrenModuleSet->AddNew(moduleName, this);

            if (this->parentModuleList == nullptr)
            {
                Recycler* recycler = GetScriptContext()->GetRecycler();
                this->parentModuleList = RecyclerNew(recycler, ModuleRecordList, recycler);
            }

            if (!this->parentModuleList->Contains(parentRecord))
            {
                this->parentModuleList->Add(parentRecord);
            }
        }
    }

    bool SourceTextModuleRecord::ConfirmChildrenParsed()
    {
        if (!this->WasParsed())
        {
            return false;
        }
        if (confirmedReady || this->ParentsNotified())
        {
            return true;
        }
        bool result = true;
        confirmedReady = true;
        EnsureChildModuleSet(GetScriptContext());
        childrenModuleSet->EachValue([&](SourceTextModuleRecord* childModuleRecord) {
            if (childModuleRecord->ParentsNotified())
            {
                return false;
            }
            else
            {
                if (childModuleRecord->ConfirmChildrenParsed())
                {
                    return false;
                }
                else
                {
                    result = false;
                    return true;
                }
            }
        });
        confirmedReady = false;
        return result;
    }

    void SourceTextModuleRecord::EnsureChildModuleSet(ScriptContext *scriptContext)
    {
        if (nullptr == this->childrenModuleSet)
        {
            ArenaAllocator* allocator = scriptContext->GeneralAllocator();
            this->childrenModuleSet = Anew(allocator, ChildModuleRecordSet, allocator);
        }
    }

    HRESULT SourceTextModuleRecord::ResolveExternalModuleDependencies()
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("ResolveExternalModuleDependencies(%s)\n"), this->GetSpecifierSz());

        ScriptContext* scriptContext = GetScriptContext();

        HRESULT hr = NOERROR;
        if (requestedModuleList != nullptr)
        {
            EnsureChildModuleSet(scriptContext);

            requestedModuleList->MapUntil([&](IdentPtr specifier) {
                LPCOLESTR moduleName = specifier->Psz();
                ModuleRecordBase* moduleRecordBase = nullptr;
                SourceTextModuleRecord* moduleRecord = nullptr;
                bool itemFound = childrenModuleSet->TryGetValue(moduleName, &moduleRecord);
                if (!itemFound)
                {
                    hr = scriptContext->GetHostScriptContext()->FetchImportedModule(this, moduleName, &moduleRecordBase);
                    if (FAILED(hr))
                    {
                        return true;
                    }
                    moduleRecord = SourceTextModuleRecord::FromHost(moduleRecordBase);
                    Var errorObject = moduleRecord->GetErrorObject();
                    if (errorObject == nullptr)
                    {
                        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>SetParent in (%s)\n"), moduleRecord->GetSpecifierSz());
                        moduleRecord->SetParent(this, moduleName);
                    }
                    else
                    {
                        this->errorObject = errorObject;
                        hr = E_FAIL;
                        return true;
                    }
                }
                return false;
            });

            if (FAILED(hr))
            {
                if (this->errorObject == nullptr)
                {
                    JavascriptError *error = scriptContext->GetLibrary()->CreateError();
                    JavascriptError::SetErrorMessageProperties(error, hr, _u("fetch import module failed"), scriptContext);
                    this->errorObject = error;
                }

                OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\tfetch import module failed\n"));
                NotifyParentsAsNeeded();
            }
        }
        return hr;
    }

    void SourceTextModuleRecord::CleanupBeforeExecution()
    {
        // zero out fields is more a defense in depth as those fields are not needed anymore
        Assert(wasParsed);
        Assert(wasEvaluated);
        Assert(wasDeclarationInitialized);
        // Debugger can reparse the source and generate the byte code again. Don't cleanup the
        // helper information for now.

        // Parser uses a temporary guest arena to keep regex patterns alive. We need to release this arena only after we have no further use
        // for the regex pattern objects.
        this->ReleaseParserResources();
    }

    bool SourceTextModuleRecord::ModuleDeclarationInstantiation()
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("ModuleDeclarationInstantiation(%s)\n"), this->GetSpecifierSz());
        ScriptContext* scriptContext = GetScriptContext();

        if (this->WasDeclarationInitialized() || this->errorObject != nullptr)
        {
            return false;
        }

        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory|ExceptionType_JavascriptException));
            InitializeLocalExports();

            InitializeLocalImports();

            InitializeIndirectExports();

            SetWasDeclarationInitialized();
            if (childrenModuleSet != nullptr)
            {
                childrenModuleSet->EachValue([=](SourceTextModuleRecord* childModuleRecord)
                {
                    Assert(childModuleRecord->WasParsed());
                    childModuleRecord->ModuleDeclarationInstantiation();
                });
            }

            ENTER_SCRIPT_IF(scriptContext, true, false, false, !scriptContext->GetThreadContext()->IsScriptActive(),
            {
                ModuleNamespace::GetModuleNamespace(this);
            });
        }
        catch (const JavascriptException& err)
        {
            this->errorObject = err.GetAndClear()->GetThrownObject(scriptContext);
        }

        if (this->errorObject != nullptr)
        {
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyParentsAsNeeded (errorObject)\n"));
            // Cleanup in case of error.
            this->ReleaseParserResources();
            NotifyParentsAsNeeded();
            return false;
        }

        return true;
    }

    void SourceTextModuleRecord::GenerateRootFunction()
    {
        // On cyclic dependency, we may endup generating the root function twice
        // so make sure we don't
        if (this->rootFunction != nullptr)
        {
            return;
        }

        ScriptContext* scriptContext = GetScriptContext();
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        CompileScriptException se;

        Assert(this->WasDeclarationInitialized());
        Assert(this == scriptContext->GetLibrary()->GetModuleRecord(this->pSourceInfo->GetSrcInfo()->moduleID));

        this->rootFunction = scriptContext->GenerateRootFunction(parseTree, sourceIndex, this->parser, this->pSourceInfo->GetParseFlags(), &se, _u("module"));
        if (rootFunction == nullptr)
        {
            const WCHAR * sourceUrl = nullptr;
            if (this->GetModuleUrl())
            {
                sourceUrl = this->GetModuleUrlSz();
            }
            this->errorObject = JavascriptError::CreateFromCompileScriptException(scriptContext, &se, sourceUrl);
            OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("\t>NotifyParentAsNeeded rootFunction == nullptr\n"));
            NotifyParentsAsNeeded();
        }
#ifdef ENABLE_SCRIPT_DEBUGGING
        else
        {
            scriptContext->GetDebugContext()->RegisterFunction(this->rootFunction->GetFunctionBody(), nullptr);
        }
#endif
        if (childrenModuleSet != nullptr)
        {
            childrenModuleSet->EachValue([=](SourceTextModuleRecord* childModuleRecord)
            {
                childModuleRecord->GenerateRootFunction();
            });
        }
    }

    Var SourceTextModuleRecord::ModuleEvaluation()
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ModulePhase, _u("ModuleEvaluation(%s)\n"), this->GetSpecifierSz());

        if (!scriptContext->GetConfig()->IsES6ModuleEnabled() || WasEvaluated())
        {
            return nullptr;
        }

        if (this->errorObject != nullptr)
        {
            // Cleanup in case of error.
            this->ReleaseParserResources();

            if (this->promise != nullptr)
            {
                SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, this->errorObject, this->scriptContext, this, false);
                return scriptContext->GetLibrary()->GetUndefined();
            }
            else
            {
                JavascriptExceptionOperators::Throw(errorObject, this->scriptContext);
            }
        }

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

        Assert(this->errorObject == nullptr);
        SetWasEvaluated();

        JavascriptExceptionObject *exception = nullptr;
        Var ret = nullptr;

        try
        {
            if (childrenModuleSet != nullptr)
            {
                childrenModuleSet->EachValue([=](SourceTextModuleRecord* childModuleRecord)
                {
                    if (!childModuleRecord->WasEvaluated())
                    {
                        childModuleRecord->ModuleEvaluation();
                    }
                    // if child module was evaluated before and threw need to re-throw now
                    // if child module has been dynamically imported and has exception need to throw
                    if (childModuleRecord->GetErrorObject() != nullptr)
                    {
                        JavascriptExceptionOperators::Throw(childModuleRecord->GetErrorObject(), this->scriptContext);
                    }
                });
            }
            CleanupBeforeExecution();

            Arguments outArgs(CallInfo(CallFlags_Value, 0), nullptr);

            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_JavascriptException));
            ENTER_SCRIPT_IF(scriptContext, true, false, false, !scriptContext->GetThreadContext()->IsScriptActive(),
            {
                ret = rootFunction->CallRootFunction(outArgs, scriptContext, true);
            });
        }
        catch (const Js::JavascriptException &err)
        {
            exception = err.GetAndClear();
            Var errorObject = exception->GetThrownObject(scriptContext);
            AssertOrFailFastMsg(errorObject != nullptr, "ModuleEvaluation: null error object thrown from root function");
            this->errorObject = errorObject;
            if (this->promise != nullptr)
            {
                ResolveOrRejectDynamicImportPromise(false, errorObject, scriptContext, this, false);
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        if (exception != nullptr)
        {
            JavascriptExceptionOperators::DoThrowCheckClone(exception, scriptContext);
        }

        if (this->promise != nullptr)
        {
            SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(true, this->GetNamespace(), this->GetScriptContext(), this, false);
        }

        return ret;
    }

    HRESULT SourceTextModuleRecord::OnHostException(void* errorVar)
    {
        if (!VarIs<RecyclableObject>(errorVar))
        {
            return E_INVALIDARG;
        }
        if (WasParsed())
        {
            return E_INVALIDARG;
        }
        this->errorObject = errorVar;

        // a sub module failed to download. we need to notify that the parent that sub module failed and we need to report back.
        return NOERROR;
    }

    SourceTextModuleRecord* SourceTextModuleRecord::GetChildModuleRecord(LPCOLESTR specifier) const
    {
        SourceTextModuleRecord* childModuleRecord = nullptr;
        if (childrenModuleSet == nullptr)
        {
            AssertMsg(false, "We should have some child modulerecords first before trying to get child modulerecord.");
            return nullptr;
        }
        if (!childrenModuleSet->TryGetValue(specifier, &childModuleRecord))
        {
            return nullptr;
        }
        return childModuleRecord;
    }

    void SourceTextModuleRecord::InitializeLocalImports()
    {
        if (importRecordList != nullptr)
        {
            importRecordList->Map([&](ModuleImportOrExportEntry& importEntry) {
                Js::PropertyId importName = EnsurePropertyIdForIdentifier(importEntry.importName);

                SourceTextModuleRecord* childModule = this->GetChildModuleRecord(importEntry.moduleRequest->Psz());
                ModuleNameRecord* importRecord = nullptr;
                // We don't need to initialize anything for * import.
                if (importName != Js::PropertyIds::star_)
                {
                    if (!childModule->ResolveExport(importName, nullptr, &importRecord)
                        || importRecord == nullptr)
                    {
                        JavascriptError* errorObj = scriptContext->GetLibrary()->CreateSyntaxError();
                        JavascriptError::SetErrorMessage(errorObj, JSERR_ModuleResolveImport, importEntry.importName->Psz(), scriptContext);
                        this->errorObject = errorObj;
                        return;
                    }
                }
            });
        }
    }

    // Local exports are stored in the slotarray in the SourceTextModuleRecord.
    void SourceTextModuleRecord::InitializeLocalExports()
    {
        Recycler* recycler = scriptContext->GetRecycler();
        Var undefineValue = scriptContext->GetLibrary()->GetUndefined();
        if (localSlotCount == InvalidSlotCount)
        {
            uint currentSlotCount = 0;
            if (localExportRecordList != nullptr)
            {
                ArenaAllocator* allocator = scriptContext->GeneralAllocator();
                localExportMapByExportName = Anew(allocator, LocalExportMap, allocator);
                localExportMapByLocalName = Anew(allocator, LocalExportMap, allocator);
                localExportIndexList = Anew(allocator, LocalExportIndexList, allocator);
                localExportRecordList->Map([&](ModuleImportOrExportEntry exportEntry)
                {
                    Assert(exportEntry.moduleRequest == nullptr);
                    Assert(exportEntry.importName == nullptr);
                    PropertyId exportNameId = EnsurePropertyIdForIdentifier(exportEntry.exportName);
                    PropertyId localNameId = EnsurePropertyIdForIdentifier(exportEntry.localName);

                    // We could have exports that look local but actually exported from other module
                    // import {foo} from "module1.js"; export {foo};
                    ModuleNameRecord* importRecord = nullptr;
                    if (this->GetImportEntryList() != nullptr
                        && this->ResolveImport(localNameId, &importRecord)
                        && importRecord != nullptr)
                    {
                        return;
                    }

                    // 2G is too big already.
                    if (localExportCount >= INT_MAX)
                    {
                        JavascriptError::ThrowRangeError(scriptContext, JSERR_TooManyImportExports);
                    }
                    localExportCount++;
                    uint exportSlot = UINT_MAX;

                    for (uint i = 0; i < (uint)localExportIndexList->Count(); i++)
                    {
                        if (localExportIndexList->Item(i) == localNameId)
                        {
                            exportSlot = i;
                            break;
                        }
                    }

                    if (exportSlot == UINT_MAX)
                    {
                        exportSlot = currentSlotCount;
                        localExportMapByLocalName->Add(localNameId, exportSlot);
                        localExportIndexList->Add(localNameId);
                        Assert(localExportIndexList->Item(currentSlotCount) == localNameId);
                        currentSlotCount++;
                        if (currentSlotCount >= INT_MAX)
                        {
                            JavascriptError::ThrowRangeError(scriptContext, JSERR_TooManyImportExports);
                        }
                    }

                    localExportMapByExportName->Add(exportNameId, exportSlot);

                });
            }
            // Namespace object will be added to the end of the array though invisible through namespace object itself.
            localExportSlots = RecyclerNewArray(recycler, Field(Var), currentSlotCount + 1);
            for (uint i = 0; i < currentSlotCount; i++)
            {
                localExportSlots[i] = undefineValue;
            }
            localExportSlots[currentSlotCount] = nullptr;

            localSlotCount = currentSlotCount;

#if ENABLE_NATIVE_CODEGEN
            if (JITManager::GetJITManager()->IsOOPJITEnabled() && JITManager::GetJITManager()->IsConnected())
            {
                PSCRIPTCONTEXT_HANDLE remoteScriptContext = this->scriptContext->GetRemoteScriptAddr();
                if (remoteScriptContext)
                {
                    HRESULT hr = JITManager::GetJITManager()->AddModuleRecordInfo(
                        remoteScriptContext,
                        this->GetModuleId(),
                        (intptr_t)this->GetLocalExportSlots());
                    JITManager::HandleServerCallResult(hr, RemoteCallType::StateUpdate);
                }
            }
#endif
        }
    }

    PropertyId SourceTextModuleRecord::EnsurePropertyIdForIdentifier(IdentPtr pid)
    {
        PropertyId propertyId = pid->GetPropertyId();
        if (propertyId == Js::Constants::NoProperty)
        {
            propertyId = scriptContext->GetOrAddPropertyIdTracked(pid->Psz(), pid->Cch());
            pid->SetPropertyId(propertyId);
        }
        return propertyId;
    }

    void SourceTextModuleRecord::InitializeIndirectExports()
    {
        ModuleNameRecord* exportRecord = nullptr;
        if (indirectExportRecordList != nullptr)
        {
            indirectExportRecordList->Map([&](ModuleImportOrExportEntry exportEntry)
            {
                PropertyId propertyId = EnsurePropertyIdForIdentifier(exportEntry.importName);
                SourceTextModuleRecord* childModuleRecord = GetChildModuleRecord(exportEntry.moduleRequest->Psz());
                if (childModuleRecord == nullptr)
                {
                    JavascriptError* errorObj = scriptContext->GetLibrary()->CreateReferenceError();
                    JavascriptError::SetErrorMessage(errorObj, JSERR_CannotResolveModule, exportEntry.moduleRequest->Psz(), scriptContext);
                    this->errorObject = errorObj;
                    return;
                }
                if (propertyId != PropertyIds::star_ &&
                    (!childModuleRecord->ResolveExport(propertyId, nullptr, &exportRecord) ||
                    (exportRecord == nullptr)))
                {
                    JavascriptError* errorObj = scriptContext->GetLibrary()->CreateSyntaxError();
                    JavascriptError::SetErrorMessage(errorObj, JSERR_ModuleResolveExport, exportEntry.exportName->Psz(), scriptContext);
                    this->errorObject = errorObj;
                    return;
                }
            });
        }
    }

    uint SourceTextModuleRecord::GetLocalExportSlotIndexByExportName(PropertyId exportNameId)
    {
        Assert(localSlotCount != 0);
        Assert(localExportSlots != nullptr);
        uint slotIndex = InvalidSlotIndex;
        if (!localExportMapByExportName->TryGetValue(exportNameId, &slotIndex))
        {
            AssertMsg(false, "exportNameId is not in local export list");
            return InvalidSlotIndex;
        }
        else
        {
            return slotIndex;
        }
    }

    uint SourceTextModuleRecord::GetLocalExportSlotIndexByLocalName(PropertyId localNameId)
    {
        Assert(localSlotCount != 0 || localNameId == PropertyIds::star_);
        Assert(localExportSlots != nullptr);
        uint slotIndex = InvalidSlotIndex;
        if (localNameId == PropertyIds::star_)
        {
            return localSlotCount;  // namespace is put on the last slot.
        } else if (!localExportMapByLocalName->TryGetValue(localNameId, &slotIndex))
        {
            AssertMsg(false, "exportNameId is not in local export list");
            return InvalidSlotIndex;
        }
        return slotIndex;
    }

    // static
    Var SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(bool isResolve, Var value, ScriptContext *scriptContext, SourceTextModuleRecord *moduleRecord, bool useReturn)
    {
        bool isScriptActive = scriptContext->GetThreadContext()->IsScriptActive();
        JavascriptPromise *promise = nullptr;
        if (moduleRecord != nullptr)
        {
            promise = moduleRecord->GetPromise();
        }

        if (promise == nullptr)
        {
            promise = JavascriptPromise::CreateEnginePromise(scriptContext);
        }

        ENTER_SCRIPT_IF(scriptContext, true, false, false, !isScriptActive,
        {
            if (isResolve)
            {
                promise->Resolve(value, scriptContext);
            }
            else
            {
                promise->Reject(value, scriptContext);
            }
        });

        if (moduleRecord != nullptr)
        {
            moduleRecord->SetPromise(nullptr);
            if (useReturn)
            {
                return JavascriptPromise::CreatePassThroughPromise(promise, scriptContext);
            }
        }
        return promise;
    }
}
