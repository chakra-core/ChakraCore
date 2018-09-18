//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

using namespace Js;

    ScriptFunctionBase::ScriptFunctionBase(DynamicType * type) :
        JavascriptFunction(type)
    {}

    ScriptFunctionBase::ScriptFunctionBase(DynamicType * type, FunctionInfo * functionInfo) :
        JavascriptFunction(type, functionInfo)
    {}

    template <> bool Js::VarIsImpl<ScriptFunctionBase>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            JavascriptFunction *function = UnsafeVarTo<JavascriptFunction>(obj);
            return ScriptFunction::Test(function) || JavascriptGeneratorFunction::Test(function)
                || JavascriptAsyncFunction::Test(function);
        }

        return false;
    }

    ScriptFunction::ScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType)
        : ScriptFunctionBase(deferredPrototypeType, proxy->GetFunctionInfo()),
        environment((FrameDisplay*)&NullFrameDisplay), cachedScopeObj(nullptr),
        hasInlineCaches(false)
    {
        Assert(proxy->GetFunctionInfo()->GetFunctionProxy() == proxy);
        Assert(proxy->EnsureDeferredPrototypeType() == deferredPrototypeType);
        DebugOnly(VerifyEntryPoint());

#if ENABLE_NATIVE_CODEGEN
        if (!proxy->IsDeferred())
        {
            FunctionBody* body = proxy->GetFunctionBody();
            if(!body->GetNativeEntryPointUsed() &&
                body->GetDefaultFunctionEntryPointInfo()->IsCodeGenDone())
            {
                MemoryBarrier();
#ifdef BGJIT_STATS
                type->GetScriptContext()->jitCodeUsed += body->GetByteCodeCount();
                type->GetScriptContext()->funcJitCodeUsed++;
#endif
                body->SetNativeEntryPointUsed(true);
            }
        }
#endif
    }

    ScriptFunction * ScriptFunction::OP_NewScFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef)
    {
        AssertMsg(infoRef!= nullptr, "BYTE-CODE VERIFY: Must specify a valid function to create");
        FunctionProxy* functionProxy = (*infoRef)->GetFunctionProxy();
        AssertMsg(functionProxy!= nullptr, "BYTE-CODE VERIFY: Must specify a valid function to create");
        ScriptContext* scriptContext = functionProxy->GetScriptContext();
        JIT_HELPER_NOT_REENTRANT_HEADER(ScrFunc_OP_NewScFunc, reentrancylock, scriptContext->GetThreadContext());

        ScriptFunction * pfuncScript = nullptr;
        if (functionProxy->IsFunctionBody() && functionProxy->GetFunctionBody()->GetInlineCachesOnFunctionObject())
        {
            FunctionBody * functionBody = functionProxy->GetFunctionBody();
            if (functionBody->GetIsFirstFunctionObject())
            {
                functionBody->SetIsNotFirstFunctionObject();
            }
            else
            {
                ScriptFunctionWithInlineCache* pfuncScriptWithInlineCache = scriptContext->GetLibrary()->CreateScriptFunctionWithInlineCache(functionProxy);
                // allocate inline cache for this function object
                pfuncScriptWithInlineCache->CreateInlineCache();

                Assert(functionBody->GetInlineCacheCount() + functionBody->GetIsInstInlineCacheCount());
                if (PHASE_TRACE1(Js::ScriptFunctionWithInlineCachePhase))
                {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                    Output::Print(_u("Function object with inline cache: function number: (%s)\tfunction name: %s\n"),
                        functionBody->GetDebugNumberSet(debugStringBuffer), functionBody->GetDisplayName());
                    Output::Flush();
                }

                pfuncScript = pfuncScriptWithInlineCache;
            }
        }

        if (pfuncScript == nullptr)
        {
            pfuncScript = scriptContext->GetLibrary()->CreateScriptFunction(functionProxy);
        }

        pfuncScript->SetEnvironment(environment);

        ScriptFunctionType *scFuncType = functionProxy->GetUndeferredFunctionType();
        if (scFuncType)
        {
            Assert(pfuncScript->GetType() == functionProxy->GetDeferredPrototypeType());
            pfuncScript->GetTypeHandler()->EnsureObjectReady(pfuncScript);
        }


        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_FUNCTION(pfuncScript, EtwTrace::GetFunctionId(functionProxy)));

        return pfuncScript;
        JIT_HELPER_END(ScrFunc_OP_NewScFunc);
    }

    ScriptFunction * ScriptFunction::OP_NewScFuncHomeObj(FrameDisplay *environment, FunctionInfoPtrPtr infoRef, Var homeObj)
    {
        Assert(homeObj != nullptr);
        Assert((*infoRef)->GetFunctionProxy()->GetFunctionInfo()->HasHomeObj());
        JIT_HELPER_NOT_REENTRANT_HEADER(ScrFunc_OP_NewScFuncHomeObj, reentrancylock, (*infoRef)->GetFunctionProxy()->GetScriptContext()->GetThreadContext());

        ScriptFunction* scriptFunc = ScriptFunction::OP_NewScFunc(environment, infoRef);
        scriptFunc->SetHomeObj(homeObj);

        return scriptFunc;
        JIT_HELPER_END(ScrFunc_OP_NewScFuncHomeObj);
    }

    void ScriptFunction::SetEnvironment(FrameDisplay * environment)
    {
        //Assert(ThreadContext::IsOnStack(this) || !ThreadContext::IsOnStack(environment));
        this->environment = environment;
    }

    void ScriptFunction::InvalidateCachedScopeChain()
    {
        // Note: Currently this helper assumes that we're in an eval-class case
        // where all the contents of the closure environment are dynamic objects.
        // Invalidating scopes that are raw slot arrays, etc., will have to be done
        // directly in byte code.

        // A function nested within this one has escaped.
        // Invalidate our own cached scope object, and walk the closure environment
        // doing this same.
        if (this->cachedScopeObj)
        {
            this->cachedScopeObj->InvalidateCachedScope();
        }
        FrameDisplay *pDisplay = this->environment;
        uint length = (uint)pDisplay->GetLength();
        for (uint i = 0; i < length; i++)
        {
            Var scope = pDisplay->GetItem(i);
            RecyclableObject *scopeObj = VarTo<RecyclableObject>(scope);
            scopeObj->InvalidateCachedScope();
        }
    }

    ProxyEntryPointInfo * ScriptFunction::GetEntryPointInfo() const
    {
        return this->GetScriptFunctionType()->GetEntryPointInfo();
    }

    ScriptFunctionType * ScriptFunction::GetScriptFunctionType() const
    {
        return (ScriptFunctionType *)GetDynamicType();
    }

    ScriptFunctionType * ScriptFunction::DuplicateType()
    {
        ScriptFunctionType* type = RecyclerNew(this->GetScriptContext()->GetRecycler(),
            ScriptFunctionType, this->GetScriptFunctionType());

        this->GetFunctionProxy()->RegisterFunctionObjectType(type);

        return type;
    }

    void ScriptFunction::PrepareForConversionToNonPathType()
    {
        // We have a path type handler that is currently responsible for holding some number of entry point infos alive.
        // The last one will be copied on to the new dictionary type handler, but if any previous instances in the path
        // are holding different entry point infos, those need to be copied to somewhere safe.
        // The number of entry points is likely low compared to length of path, so iterate those instead.

        ProxyEntryPointInfo* entryPointInfo = this->GetScriptFunctionType()->GetEntryPointInfo();

        this->GetFunctionProxy()->MapFunctionObjectTypes([&](ScriptFunctionType* functionType)
        {
            CopyEntryPointInfoToThreadContextIfNecessary(functionType->GetEntryPointInfo(), entryPointInfo);
        });
    }

    void ScriptFunction::ReplaceTypeWithPredecessorType(DynamicType * previousType)
    {
        ProxyEntryPointInfo* oldEntryPointInfo = this->GetScriptFunctionType()->GetEntryPointInfo();
        __super::ReplaceTypeWithPredecessorType(previousType);
        ProxyEntryPointInfo* newEntryPointInfo = this->GetScriptFunctionType()->GetEntryPointInfo();
        CopyEntryPointInfoToThreadContextIfNecessary(oldEntryPointInfo, newEntryPointInfo);
    }

    bool ScriptFunction::HasFunctionBody()
    {
        // for asmjs we want to first check if the FunctionObject has a function body. Check that the function is not deferred
        return  !this->GetFunctionInfo()->IsDeferredParseFunction() && !this->GetFunctionInfo()->IsDeferredDeserializeFunction() && GetParseableFunctionInfo()->IsFunctionParsed();
    }

    void ScriptFunction::ChangeEntryPoint(ProxyEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint)
    {
        Assert(this->GetTypeId() == TypeIds_Function);
#if ENABLE_NATIVE_CODEGEN
        Assert(!IsCrossSiteObject() || entryPoint != (Js::JavascriptMethod)checkCodeGenThunk);
#endif

        Assert((entryPointInfo != nullptr && this->GetFunctionProxy() != nullptr));

        bool isAsmJS = HasFunctionBody() && this->GetFunctionBody()->GetIsAsmjsMode();
        this->GetScriptFunctionType()->ChangeEntryPoint(entryPointInfo, entryPoint, isAsmJS);
    }

    void ScriptFunction::CopyEntryPointInfoToThreadContextIfNecessary(ProxyEntryPointInfo* oldEntryPointInfo, ProxyEntryPointInfo* newEntryPointInfo)
    {
        if (oldEntryPointInfo
            && oldEntryPointInfo != newEntryPointInfo
            && oldEntryPointInfo->SupportsExpiration())
        {
            // The old entry point could be executing so we need root it to make sure
            // it isn't prematurely collected. The rooting is done by queuing it up on the threadContext
            ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();

            threadContext->QueueFreeOldEntryPointInfoIfInScript((FunctionEntryPointInfo*)oldEntryPointInfo);
        }
    }

    FunctionProxy * ScriptFunction::GetFunctionProxy() const
    {
        Assert(this->functionInfo->HasBody());
        return this->functionInfo->GetFunctionProxy();
    }
    JavascriptMethod ScriptFunction::UpdateUndeferredBody(FunctionBody* newFunctionInfo)
    {
        // Update deferred parsed/serialized function to the real function body
        Assert(this->functionInfo->HasBody());
        Assert(this->functionInfo == newFunctionInfo->GetFunctionInfo());
        Assert(this->functionInfo->GetFunctionBody() == newFunctionInfo);
        Assert(!newFunctionInfo->IsDeferred());

        JavascriptMethod directEntryPoint = newFunctionInfo->GetDirectEntryPoint(newFunctionInfo->GetDefaultEntryPointInfo());
#if defined(ENABLE_SCRIPT_PROFILING) || defined(ENABLE_SCRIPT_DEBUGGING)
        Assert(directEntryPoint != DefaultDeferredParsingThunk
            && directEntryPoint != ProfileDeferredParsingThunk);
#else
        Assert(directEntryPoint != DefaultDeferredParsingThunk);
#endif

        Js::FunctionEntryPointInfo* defaultEntryPointInfo = newFunctionInfo->GetDefaultFunctionEntryPointInfo();
        JavascriptMethod thunkEntryPoint = this->UpdateThunkEntryPoint(defaultEntryPointInfo, directEntryPoint);

        this->GetScriptFunctionType()->SetEntryPointInfo(defaultEntryPointInfo);

        return thunkEntryPoint;

    }

    JavascriptMethod ScriptFunction::UpdateThunkEntryPoint(FunctionEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint)
    {
        this->ChangeEntryPoint(entryPointInfo, entryPoint);

        if (!CrossSite::IsThunk(this->GetEntryPoint()))
        {
            return entryPoint;
        }

        // We already pass through the cross site thunk, which would have called the profile thunk already if necessary
        // So just call the original entry point if our direct entry is the profile entry thunk
        // Otherwise, call the directEntryPoint which may have additional processing to do (e.g. ensure dynamic profile)
        Assert(this->IsCrossSiteObject());
        if (entryPoint != ProfileEntryThunk)
        {
            return entryPoint;
        }
        // Based on the comment below, this shouldn't be a defer deserialization function as it would have a deferred thunk
        FunctionBody * functionBody = this->GetFunctionBody();
        // The original entry point should be an interpreter thunk or the native entry point;
        Assert(functionBody->IsInterpreterThunk() || functionBody->IsNativeOriginalEntryPoint());
        return functionBody->GetOriginalEntryPoint();
    }

    bool ScriptFunction::IsNewEntryPointAvailable()
    {
        Js::FunctionEntryPointInfo *const defaultEntryPointInfo = this->GetFunctionBody()->GetDefaultFunctionEntryPointInfo();
        JavascriptMethod defaultEntryPoint = this->GetFunctionBody()->GetDirectEntryPoint(defaultEntryPointInfo);

        return this->GetEntryPoint() != defaultEntryPoint;
    }

    Var ScriptFunction::GetSourceString() const
    {
        return this->GetFunctionProxy()->EnsureDeserialized()->GetCachedSourceString();
    }

    JavascriptString * ScriptFunction::EnsureSourceString()
    {
        // The function may be defer serialize, need to be deserialized
        FunctionProxy* proxy = this->GetFunctionProxy();
        ParseableFunctionInfo * pFuncBody = proxy->EnsureDeserialized();
        JavascriptString * cachedSourceString = pFuncBody->GetCachedSourceString();
        if (cachedSourceString != nullptr)
        {
            return cachedSourceString;
        }

        ScriptContext * scriptContext = this->GetScriptContext();

        //Library code should behave the same way as RuntimeFunctions
        Utf8SourceInfo* source = pFuncBody->GetUtf8SourceInfo();
        if ((source != nullptr && source->GetIsLibraryCode())
#ifdef ENABLE_WASM
            || (pFuncBody->IsWasmFunction())
#endif
            )
        {
            //Don't display if it is anonymous function
            charcount_t displayNameLength = 0;
            PCWSTR displayName = pFuncBody->GetShortDisplayName(&displayNameLength);
            cachedSourceString = JavascriptFunction::GetLibraryCodeDisplayString(scriptContext, displayName);
        }
        else if (!pFuncBody->GetUtf8SourceInfo()->GetIsXDomain()
            // To avoid backward compat issue, we will not give out sourceString for function if it is called from
            // window.onerror trying to retrieve arguments.callee.caller.
            && !(pFuncBody->GetUtf8SourceInfo()->GetIsXDomainString() && scriptContext->GetThreadContext()->HasUnhandledException())
            )
        {
            // Decode UTF8 into Unicode
            // Consider: Should we have a JavascriptUtf8Substring class which defers decoding
            // until it's needed?

            charcount_t cch = pFuncBody->LengthInChars();
            size_t cbLength = pFuncBody->LengthInBytes();
            LPCUTF8 pbStart = pFuncBody->GetToStringSource(_u("ScriptFunction::EnsureSourceString"));
            // cch and cbLength refer to the length of the parse, which may be smaller than the length of the to-string function
            Assert(pFuncBody->StartOffset() >= pFuncBody->PrintableStartOffset());
            size_t cbPreludeLength = pFuncBody->StartOffset() - pFuncBody->PrintableStartOffset();
            Assert(cbPreludeLength < MaxCharCount);
            // the toString of a function may include some prelude, e.g. the computed name expression.
            // We do not store the char-index of the start, but if there are cbPreludeLength bytes difference,
            // then that is an upper bound on the number of characters difference.
            // We also assume that function.toString is relatively infrequent, and non-ascii characters in
            // a prelude are relatively infrequent, so the inaccuracy here should in general be insignificant

            BufferStringBuilder builder(cch + static_cast<charcount_t>(cbPreludeLength), scriptContext);
            utf8::DecodeOptions options = pFuncBody->GetUtf8SourceInfo()->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
            size_t decodedCount = utf8::DecodeUnitsInto(builder.DangerousGetWritableBuffer(), pbStart, pbStart + cbLength + cbPreludeLength, options);

            if (decodedCount < cch)
            {
                AssertMsg(false, "Decoded incorrect number of characters for function body");
                Js::Throw::FatalInternalError();
            }
            else if (decodedCount < cch + static_cast<charcount_t>(cbPreludeLength))
            {
                Recycler* recycler = scriptContext->GetRecycler();

                char16* buffer = RecyclerNewArrayLeaf(recycler, char16, decodedCount + 1);
                wmemcpy_s(buffer, decodedCount, builder.DangerousGetWritableBuffer(), decodedCount);
                buffer[decodedCount] = 0;

                cachedSourceString = JavascriptString::NewWithBuffer(buffer, static_cast<charcount_t>(decodedCount), scriptContext);
            }
            else
            {
                cachedSourceString = builder.ToString();
            }
        }
        else
        {
            cachedSourceString = scriptContext->GetLibrary()->GetXDomainFunctionDisplayString();
        }
        Assert(cachedSourceString != nullptr);
        pFuncBody->SetCachedSourceString(cachedSourceString);
        return cachedSourceString;
    }

#if ENABLE_TTD
    void ScriptFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        Js::FunctionBody* fb = TTD::JsSupport::ForceAndGetFunctionBody(this->GetParseableFunctionInfo());
        extractor->MarkFunctionBody(fb);

        Js::FrameDisplay* environment = this->GetEnvironment();
        if(environment->GetLength() != 0)
        {
            extractor->MarkScriptFunctionScopeInfo(environment);
        }

        if(this->cachedScopeObj != nullptr)
        {
            extractor->MarkVisitVar(this->cachedScopeObj);
        }

        if (this->GetComputedNameVar() != nullptr)
        {
            extractor->MarkVisitVar(this->GetComputedNameVar());
        }

        if (this->GetHomeObj() != nullptr)
        {
            extractor->MarkVisitVar(this->GetHomeObj());
        }
    }

    void ScriptFunction::ProcessCorePaths()
    {
        TTD::RuntimeContextInfo* rctxInfo = this->GetScriptContext()->TTDWellKnownInfo;

        //do the body path mark
        Js::FunctionBody* fb = TTD::JsSupport::ForceAndGetFunctionBody(this->GetParseableFunctionInfo());
        rctxInfo->EnqueueNewFunctionBodyObject(this, fb, _u("!fbody"));

        Js::FrameDisplay* environment = this->GetEnvironment();
        uint32 scopeCount = environment->GetLength();

        for(uint32 i = 0; i < scopeCount; ++i)
        {
            TTD::UtilSupport::TTAutoString scopePathString;
            rctxInfo->BuildEnvironmentIndexBuffer(i, scopePathString);

            void* scope = environment->GetItem(i);
            switch(environment->GetScopeType(scope))
            {
            case Js::ScopeType::ScopeType_ActivationObject:
            case Js::ScopeType::ScopeType_WithScope:
            {
                rctxInfo->EnqueueNewPathVarAsNeeded(this, (Js::Var)scope, scopePathString.GetStrValue());
                break;
            }
            case Js::ScopeType::ScopeType_SlotArray:
            {
                Js::ScopeSlots slotArray = (Field(Js::Var)*)scope;
                uint slotArrayCount = static_cast<uint>(slotArray.GetCount());

                //get the function body associated with the scope
                if (slotArray.IsDebuggerScopeSlotArray())
                {
                    rctxInfo->AddWellKnownDebuggerScopePath(this, slotArray.GetDebuggerScope(), i);
                }
                else
                {
                    rctxInfo->EnqueueNewFunctionBodyObject(this, slotArray.GetFunctionInfo()->GetFunctionBody(), scopePathString.GetStrValue());
                }

                for(uint j = 0; j < slotArrayCount; j++)
                {
                    Js::Var sval = slotArray.Get(j);

                    TTD::UtilSupport::TTAutoString slotPathString;
                    rctxInfo->BuildEnvironmentIndexAndSlotBuffer(i, j, slotPathString);

                    rctxInfo->EnqueueNewPathVarAsNeeded(this, sval, slotPathString.GetStrValue());
                }

                break;
            }
            default:
                TTDAssert(false, "Unknown scope kind");
            }
        }

        if(this->cachedScopeObj != nullptr)
        {
            this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->cachedScopeObj, _u("_cachedScopeObj"));
        }

        if (this->GetComputedNameVar() != nullptr)
        {
            this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->GetComputedNameVar(), _u("_computedName"));
        }

        if (this->GetHomeObj() != nullptr)
        {
            this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->GetHomeObj(), _u("_homeObj"));
        }
    }

    TTD::NSSnapObjects::SnapObjectType ScriptFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapScriptFunctionObject;
    }

    void ScriptFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(this->GetFunctionInfo() != nullptr, "We are only doing this for functions with ParseableFunctionInfo.");

        TTD::NSSnapObjects::SnapScriptFunctionInfo* ssfi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapScriptFunctionInfo>();

        this->ExtractSnapObjectDataIntoSnapScriptFunctionInfo(ssfi, alloc);

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapScriptFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapScriptFunctionObject>(objData, ssfi);
    }

    // TODO:  Fixup function definition - something funky w/ header file includes - cycles?
    void ScriptFunction::ExtractSnapObjectDataIntoSnapScriptFunctionInfo(/*TTD::NSSnapObjects::SnapScriptFunctionInfo* */ void* snapScriptFunctionInfo, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapScriptFunctionInfo* ssfi = reinterpret_cast<TTD::NSSnapObjects::SnapScriptFunctionInfo*>(snapScriptFunctionInfo);

        Js::FunctionBody* fb = TTD::JsSupport::ForceAndGetFunctionBody(this->GetParseableFunctionInfo());

        alloc.CopyNullTermStringInto(fb->GetDisplayName(), ssfi->DebugFunctionName);

        ssfi->BodyRefId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(fb);

        Js::FrameDisplay* environment = this->GetEnvironment();
        ssfi->ScopeId = TTD_INVALID_PTR_ID;
        if (environment->GetLength() != 0)
        {
            ssfi->ScopeId = TTD_CONVERT_SCOPE_TO_PTR_ID(environment);
        }

        ssfi->CachedScopeObjId = TTD_INVALID_PTR_ID;
        if (this->cachedScopeObj != nullptr)
        {
            ssfi->CachedScopeObjId = TTD_CONVERT_VAR_TO_PTR_ID(this->cachedScopeObj);
        }

        ssfi->HomeObjId = TTD_INVALID_PTR_ID;
        if (this->GetHomeObj() != nullptr)
        {
            ssfi->HomeObjId = TTD_CONVERT_VAR_TO_PTR_ID(this->GetHomeObj());
        }

        ssfi->ComputedNameInfo = TTD_CONVERT_JSVAR_TO_TTDVAR(this->GetComputedNameVar());
    }
#endif

    AsmJsScriptFunction::AsmJsScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType) :
        ScriptFunction(proxy, deferredPrototypeType), m_moduleEnvironment(nullptr)
    {}

    AsmJsScriptFunction * AsmJsScriptFunction::OP_NewAsmJsFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef)
    {
        AssertMsg(infoRef != nullptr, "BYTE-CODE VERIFY: Must specify a valid function to create");
        FunctionProxy* functionProxy = (*infoRef)->GetFunctionProxy();
        AssertMsg(functionProxy != nullptr, "BYTE-CODE VERIFY: Must specify a valid function to create");

        ScriptContext* scriptContext = functionProxy->GetScriptContext();

        Assert(!functionProxy->HasSuperReference());
        AsmJsScriptFunction* asmJsFunc = scriptContext->GetLibrary()->CreateAsmJsScriptFunction(functionProxy);
        asmJsFunc->SetEnvironment(environment);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_FUNCTION(asmJsFunc, EtwTrace::GetFunctionId(functionProxy)));

        return asmJsFunc;
    }

    JavascriptArrayBuffer* AsmJsScriptFunction::GetAsmJsArrayBuffer() const
    {
#ifdef ASMJS_PLAT
        return (JavascriptArrayBuffer*)PointerValue(
            *(this->GetModuleEnvironment() + AsmJsModuleMemory::MemoryTableBeginOffset));
#else
        Assert(UNREACHED);
        return nullptr;
#endif
    }

#ifdef ENABLE_WASM
    WasmScriptFunction::WasmScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType) :
        AsmJsScriptFunction(proxy, deferredPrototypeType), m_signature(nullptr)
    {
        Assert(!proxy->GetFunctionInfo()->HasComputedName());
    }

    WebAssemblyMemory* WasmScriptFunction::GetWebAssemblyMemory() const
    {
        return (WebAssemblyMemory*)PointerValue(
            *(this->GetModuleEnvironment() + AsmJsModuleMemory::MemoryTableBeginOffset));
    }
#endif

    ScriptFunctionWithInlineCache::ScriptFunctionWithInlineCache(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType) :
        ScriptFunction(proxy, deferredPrototypeType)
    {}

    InlineCache * ScriptFunctionWithInlineCache::GetInlineCache(uint index)
    {
        void** inlineCaches = this->GetInlineCaches();
        AssertOrFailFast(inlineCaches != nullptr);
        AssertOrFailFast(index < this->GetInlineCacheCount());
#if DBG
        Assert(this->m_inlineCacheTypes[index] == InlineCacheTypeNone ||
            this->m_inlineCacheTypes[index] == InlineCacheTypeInlineCache);
        this->m_inlineCacheTypes[index] = InlineCacheTypeInlineCache;
#endif
        return reinterpret_cast<InlineCache *>(PointerValue(inlineCaches[index]));
    }

    void ScriptFunctionWithInlineCache::CreateInlineCache()
    {
        Js::FunctionBody *functionBody = this->GetFunctionBody();
        this->rootObjectLoadInlineCacheStart = functionBody->GetRootObjectLoadInlineCacheStart();
        this->rootObjectStoreInlineCacheStart = functionBody->GetRootObjectStoreInlineCacheStart();
        this->inlineCacheCount = functionBody->GetInlineCacheCount();
        this->isInstInlineCacheCount = functionBody->GetIsInstInlineCacheCount();

        SetHasInlineCaches(true);
        AllocateInlineCache();
    }

    void ScriptFunctionWithInlineCache::Finalize(bool isShutdown)
    {
        if (isShutdown)
        {
            FreeOwnInlineCaches<true>();
        }
        else
        {
            FreeOwnInlineCaches<false>();
        }
    }
    template<bool isShutdown>
    void ScriptFunctionWithInlineCache::FreeOwnInlineCaches()
    {
        uint isInstInlineCacheStart = this->GetInlineCacheCount();
        uint totalCacheCount = isInstInlineCacheStart + isInstInlineCacheCount;
        if (this->GetHasInlineCaches() && this->m_inlineCaches)
        {
            Js::ScriptContext* scriptContext = this->GetParseableFunctionInfo()->GetScriptContext();
            uint i = 0;
            uint unregisteredInlineCacheCount = 0;
            uint plainInlineCacheEnd = rootObjectLoadInlineCacheStart;
            __analysis_assume(plainInlineCacheEnd < totalCacheCount);
            for (; i < plainInlineCacheEnd; i++)
            {
                if (this->m_inlineCaches[i])
                {
                    InlineCache* inlineCache = (InlineCache*)(void*)this->m_inlineCaches[i];
                    if (isShutdown)
                    {
                        inlineCache->Clear();
                    }
                    else if(!scriptContext->IsClosed())
                    {
                        if (inlineCache->RemoveFromInvalidationList())
                        {
                            unregisteredInlineCacheCount++;
                        }
                        AllocatorDelete(InlineCacheAllocator, scriptContext->GetInlineCacheAllocator(), inlineCache);
                    }
                    this->m_inlineCaches[i] = nullptr;
                }
            }

            i = isInstInlineCacheStart;
            for (; i < totalCacheCount; i++)
            {
                if (this->m_inlineCaches[i])
                {
                    if (isShutdown)
                    {
                        ((IsInstInlineCache*)this->m_inlineCaches[i])->Clear();
                    }
                    else if (!scriptContext->IsClosed())
                    {
                        AllocatorDelete(CacheAllocator, scriptContext->GetIsInstInlineCacheAllocator(), (IsInstInlineCache*)(void*)this->m_inlineCaches[i]);
                    }
                    this->m_inlineCaches[i] = nullptr;
                }
            }

            if (unregisteredInlineCacheCount > 0)
            {
                AssertMsg(!isShutdown && !scriptContext->IsClosed(), "Unregistration of inlineCache should only be done if this is not shutdown or scriptContext closing.");
                scriptContext->GetThreadContext()->NotifyInlineCacheBatchUnregistered(unregisteredInlineCacheCount);
            }
        }
    }

    void ScriptFunctionWithInlineCache::AllocateInlineCache()
    {
        Assert(this->m_inlineCaches == nullptr);
        uint isInstInlineCacheStart = this->GetInlineCacheCount();
        uint totalCacheCount = isInstInlineCacheStart + isInstInlineCacheCount;
        Js::FunctionBody* functionBody = this->GetFunctionBody();

        if (totalCacheCount != 0)
        {
            // Root object inline cache are not leaf
            Js::ScriptContext* scriptContext = this->GetFunctionBody()->GetScriptContext();
            void ** inlineCaches = RecyclerNewArrayZ(scriptContext->GetRecycler() ,
                void*, totalCacheCount);
#if DBG
            this->m_inlineCacheTypes = RecyclerNewArrayLeafZ(scriptContext->GetRecycler(),
                byte, totalCacheCount);
#endif
            uint i = 0;
            uint plainInlineCacheEnd = rootObjectLoadInlineCacheStart;
            __analysis_assume(plainInlineCacheEnd <= totalCacheCount);
            for (; i < plainInlineCacheEnd; i++)
            {
                inlineCaches[i] = AllocatorNewZ(InlineCacheAllocator,
                    scriptContext->GetInlineCacheAllocator(), InlineCache);
            }
            Js::RootObjectBase * rootObject = functionBody->GetRootObject();
            ThreadContext * threadContext = scriptContext->GetThreadContext();
            uint rootObjectLoadInlineCacheEnd = rootObjectLoadMethodInlineCacheStart;
            __analysis_assume(rootObjectLoadInlineCacheEnd <= totalCacheCount);
            for (; i < rootObjectLoadInlineCacheEnd; i++)
            {
                inlineCaches[i] = rootObject->GetInlineCache(
                    threadContext->GetPropertyName(functionBody->GetPropertyIdFromCacheId(i)), false, false);
            }
            uint rootObjectLoadMethodInlineCacheEnd = rootObjectStoreInlineCacheStart;
            __analysis_assume(rootObjectLoadMethodInlineCacheEnd <= totalCacheCount);
            for (; i < rootObjectLoadMethodInlineCacheEnd; i++)
            {
                inlineCaches[i] = rootObject->GetInlineCache(
                    threadContext->GetPropertyName(functionBody->GetPropertyIdFromCacheId(i)), true, false);
            }
            uint rootObjectStoreInlineCacheEnd = isInstInlineCacheStart;
            __analysis_assume(rootObjectStoreInlineCacheEnd <= totalCacheCount);
            for (; i < rootObjectStoreInlineCacheEnd; i++)
            {
#pragma prefast(suppress:6386, "The analysis assume didn't help prefast figure out this is in range")
                inlineCaches[i] = rootObject->GetInlineCache(
                    threadContext->GetPropertyName(functionBody->GetPropertyIdFromCacheId(i)), false, true);
            }
            for (; i < totalCacheCount; i++)
            {
                inlineCaches[i] = AllocatorNewStructZ(CacheAllocator,
                    functionBody->GetScriptContext()->GetIsInstInlineCacheAllocator(), IsInstInlineCache);
            }
#if DBG
            this->m_inlineCacheTypes = RecyclerNewArrayLeafZ(functionBody->GetScriptContext()->GetRecycler(),
                byte, totalCacheCount);
#endif
            this->m_inlineCaches = inlineCaches;
        }
    }

    bool ScriptFunction::GetSymbolName(Var computedNameVar, const char16** symbolName, charcount_t* length)
    {
        if (nullptr != computedNameVar && VarIs<JavascriptSymbol>(computedNameVar))
        {
            const PropertyRecord* symbolRecord = VarTo<JavascriptSymbol>(computedNameVar)->GetValue();
            *symbolName = symbolRecord->GetBuffer();
            *length = symbolRecord->GetLength();
            return true;
        }
        *symbolName = nullptr;
        *length = 0;
        return false;
    }

    JavascriptString* ScriptFunction::GetDisplayNameImpl() const
    {
        Assert(this->GetFunctionProxy() != nullptr); // The caller should guarantee a proxy exists
        ParseableFunctionInfo * func = this->GetFunctionProxy()->EnsureDeserialized();
        const char16* name = nullptr;
        charcount_t length = 0;
        JavascriptString* returnStr = nullptr;
        ENTER_PINNED_SCOPE(JavascriptString, computedName);
        Var computedNameVar = this->GetComputedNameVar();
        if (computedNameVar != nullptr)
        {
            const char16* symbolName = nullptr;
            charcount_t symbolNameLength = 0;
            if (ScriptFunction::GetSymbolName(computedNameVar, &symbolName, &symbolNameLength))
            {
                if (symbolNameLength == 0)
                {
                    name = symbolName;
                }
                else
                {
                    name = FunctionProxy::WrapWithBrackets(symbolName, symbolNameLength, this->GetScriptContext());
                    length = symbolNameLength + 2; //adding 2 to length for  brackets
                }
            }
            else
            {
                computedName = ScriptFunction::GetComputedName(computedNameVar, this->GetScriptContext());
                if (!func->GetIsAccessor())
                {
                    return computedName;
                }
                name = computedName->GetString();
                length = computedName->GetLength();
            }
        }
        else
        {
            name = Constants::Empty;
            if (func->GetIsNamedFunctionExpression()) // GetIsNamedFunctionExpression -> ex. var a = function foo() {} where name is foo
            {
                name = func->GetShortDisplayName(&length);
            }
            else if (func->GetIsNameIdentifierRef()) // GetIsNameIdentifierRef        -> confirms a name is not attached like o.x = function() {}
            {
                if (this->GetScriptContext()->GetConfig()->IsES6FunctionNameFullEnabled())
                {
                    name = func->GetShortDisplayName(&length);
                }
                else if (func->GetIsDeclaration() || // GetIsDeclaration -> ex. function foo () {}
                         func->GetIsAccessor()    || // GetIsAccessor    -> ex. var a = { get f() {}} new enough syntax that we do not have to disable by default
                         func->IsLambda()         || // IsLambda         -> ex. var y = { o : () => {}}
                         GetHomeObj())               // GetHomeObj       -> ex. var o = class {}, confirms this is a constructor or method on a class
                {
                    name = func->GetShortDisplayName(&length);
                }
            }
        }
        AssertMsg(IsValidCharCount(length), "JavascriptString can't be larger than charcount_t");
        returnStr = DisplayNameHelper(name, static_cast<charcount_t>(length));

        LEAVE_PINNED_SCOPE();

        return returnStr;
    }

    bool ScriptFunction::IsAnonymousFunction() const
    {
        return this->GetFunctionProxy()->GetIsAnonymousFunction();
    }

    JavascriptString* ScriptFunction::GetComputedName(Var computedNameVar, ScriptContext * scriptContext)
    {
        JavascriptString* computedName = nullptr;
        if (computedNameVar != nullptr)
        {
            if (TaggedInt::Is(computedNameVar))
            {
                computedName = TaggedInt::ToString(computedNameVar, scriptContext);
            }
            else
            {
                computedName = JavascriptConversion::ToString(computedNameVar, scriptContext);
            }
            return computedName;
        }
        return nullptr;
    }

    bool ScriptFunction::HasSuperReference()
    {
        return this->GetFunctionProxy()->HasSuperReference();
    }

    void ScriptFunctionWithInlineCache::ClearInlineCacheOnFunctionObject()
    {
        if (NULL != this->m_inlineCaches)
        {
            FreeOwnInlineCaches<false>();
            this->m_inlineCaches = nullptr;
            this->inlineCacheCount = 0;
            this->rootObjectLoadInlineCacheStart = 0;
            this->rootObjectLoadMethodInlineCacheStart = 0;
            this->rootObjectStoreInlineCacheStart = 0;
            this->isInstInlineCacheCount = 0;
        }
        SetHasInlineCaches(false);
    }
