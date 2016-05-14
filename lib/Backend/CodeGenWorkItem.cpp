//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "BackEnd.h"
#include "Language\SourceDynamicProfileManager.h"

CodeGenWorkItem::CodeGenWorkItem(
    JsUtil::JobManager *const manager,
    Js::FunctionBody *const functionBody,
    Js::EntryPointInfo* entryPointInfo,
    bool isJitInDebugMode,
    CodeGenWorkItemType type)
    : JsUtil::Job(manager)
    , codeAddress(NULL)
    , functionBody(functionBody)
    , entryPointInfo(entryPointInfo)
    , recyclableData(nullptr)
    , isInJitQueue(false)
    , isAllocationCommitted(false)
    , queuedFullJitWorkItem(nullptr)
    , allocation(nullptr)
#ifdef IR_VIEWER
    , isRejitIRViewerFunction(false)
    , irViewerOutput(nullptr)
    , irViewerRequestContext(nullptr)
#endif
{
    // TODO: (michhol) OOP JIT put bodyData directly on function body rather than doing this copying
    // bytecode
    this->jitData.bodyData.byteCodeLength = functionBody->GetByteCode()->GetLength();
    this->jitData.bodyData.byteCodeBuffer = functionBody->GetByteCode()->GetBuffer();

    // const table
    this->jitData.bodyData.constCount = functionBody->GetConstantCount();
    if (functionBody->GetConstantCount() > 0)
    {
        // TODO (michhol): OOP JIT, will be different for asm.js
        this->jitData.bodyData.constTable = (intptr_t *)functionBody->GetConstTable();

        this->jitData.bodyData.constTypeTable = HeapNewArray(int32, functionBody->GetConstantCount());
        for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < functionBody->GetConstantCount(); ++reg)
        {
            Js::Var varConst = functionBody->GetConstantVar(reg);
            Assert(varConst != nullptr);
            if (Js::TaggedInt::Is(varConst) ||
                varConst == (Js::Var)&Js::NullFrameDisplay ||
                varConst == (Js::Var)&Js::StrictNullFrameDisplay)
            {
                // don't need TypeId for these
                jitData.bodyData.constTypeTable[reg - Js::FunctionBody::FirstRegSlot] = Js::TypeId::TypeIds_Limit;
            }
            else
            {
                jitData.bodyData.constTypeTable[reg - Js::FunctionBody::FirstRegSlot] = Js::JavascriptOperators::GetTypeId(varConst);
            }
        }
    }

    // statement map
    Js::SmallSpanSequence * statementMap = functionBody->GetStatementMapSpanSequence();

    this->jitData.bodyData.statementMap.baseValue = statementMap->baseValue;

    if (statementMap->pActualOffsetList)
    {
        this->jitData.bodyData.statementMap.actualOffsetLength = statementMap->pActualOffsetList->GetLength();
        this->jitData.bodyData.statementMap.actualOffsetList = statementMap->pActualOffsetList->GetBuffer();
    }
    else
    {
        this->jitData.bodyData.statementMap.actualOffsetLength = 0;
        this->jitData.bodyData.statementMap.actualOffsetList = nullptr;
    }

    if (statementMap->pStatementBuffer)
    {
        this->jitData.bodyData.statementMap.statementLength = statementMap->pStatementBuffer->GetLength();
        this->jitData.bodyData.statementMap.statementBuffer = statementMap->pStatementBuffer->GetBuffer();
    }
    else
    {
        this->jitData.bodyData.statementMap.statementLength = 0;
        this->jitData.bodyData.statementMap.statementBuffer = nullptr;
    }

    this->jitData.bodyData.inlineCacheCount = functionBody->GetInlineCacheCount();
    if (functionBody->GetInlineCacheCount() > 0)
    {
        this->jitData.bodyData.cacheIdToPropertyIdMap = functionBody->GetCacheIdToPropertyIdMap();
        this->jitData.bodyData.inlineCaches = reinterpret_cast<intptr_t*>(functionBody->GetInlineCaches());
    }

    // body data
    this->jitData.bodyData.functionBodyAddr = (intptr_t)functionBody;

    this->jitData.bodyData.funcNumber = functionBody->GetFunctionNumber();
    this->jitData.bodyData.localFuncId = functionBody->GetLocalFunctionId();
    this->jitData.bodyData.sourceContextId = functionBody->GetSourceContextId();
    this->jitData.bodyData.nestedCount = functionBody->GetNestedCount();
    this->jitData.bodyData.scopeSlotArraySize = functionBody->scopeSlotArraySize;
    this->jitData.bodyData.attributes = functionBody->GetAttributes();
    this->jitData.bodyData.isInstInlineCacheCount = functionBody->GetIsInstInlineCacheCount();

    if (functionBody->GetUtf8SourceInfo()->GetCbLength() > UINT_MAX)
    {
        Js::Throw::OutOfMemory();
    }

    this->jitData.bodyData.byteCodeCount = functionBody->GetByteCodeCount();
    this->jitData.bodyData.byteCodeInLoopCount = functionBody->GetByteCodeInLoopCount();
    this->jitData.bodyData.nonLoadByteCodeCount = functionBody->GetByteCodeWithoutLDACount();
    this->jitData.bodyData.loopCount = functionBody->GetLoopCount();

    if (functionBody->GetLoopCount() > 0)
    {
        this->jitData.bodyData.loopHeaderArrayAddr = (intptr_t)functionBody->GetLoopHeaderArrayPtr();
        this->jitData.bodyData.loopHeaders = HeapNewArray(JITLoopHeader, functionBody->GetLoopCount());
        for (uint i = 0; i < functionBody->GetLoopCount(); ++i)
        {
            this->jitData.bodyData.loopHeaders->startOffset = functionBody->GetLoopHeader(i)->startOffset;
            this->jitData.bodyData.loopHeaders->endOffset = functionBody->GetLoopHeader(i)->endOffset;
            this->jitData.bodyData.loopHeaders->isNested = functionBody->GetLoopHeader(i)->isNested;
        }
    } // TODO: OOP JIT, in else case, maybe we should pass specific bad data or nullptr to be defensive/help assert we don't use this

    this->jitData.bodyData.localFrameDisplayReg = functionBody->GetLocalFrameDisplayReg();
    this->jitData.bodyData.localClosureReg = functionBody->GetLocalClosureReg();
    this->jitData.bodyData.envReg = functionBody->GetEnvReg();
    this->jitData.bodyData.firstTmpReg = functionBody->GetFirstTmpReg();
    this->jitData.bodyData.varCount = functionBody->GetVarCount();
    this->jitData.bodyData.innerScopeCount = functionBody->GetInnerScopeCount();
    if (functionBody->GetInnerScopeCount() > 0)
    {
        this->jitData.bodyData.firstInnerScopeReg = functionBody->FirstInnerScopeReg();
    }
    this->jitData.bodyData.envDepth = functionBody->GetEnvDepth();
    this->jitData.bodyData.profiledIterations = functionBody->GetProfiledIterations();
    this->jitData.bodyData.profiledCallSiteCount = functionBody->GetProfiledCallSiteCount();
    this->jitData.bodyData.inParamCount = functionBody->GetInParamsCount();
    this->jitData.bodyData.thisRegisterForEventHandler = functionBody->GetThisRegForEventHandler();
    this->jitData.bodyData.funcExprScopeRegister = functionBody->GetFuncExprScopeReg();

    this->jitData.bodyData.flags = functionBody->GetFlags();

    this->jitData.bodyData.doBackendArgumentsOptimization = functionBody->GetDoBackendArgumentsOptimization();
    this->jitData.bodyData.isLibraryCode = functionBody->GetUtf8SourceInfo()->GetIsLibraryCode();
    this->jitData.bodyData.isAsmJsMode = functionBody->GetIsAsmjsMode();
    this->jitData.bodyData.isStrictMode = functionBody->GetIsStrictMode();
    this->jitData.bodyData.isEval = functionBody->IsEval();
    this->jitData.bodyData.isGlobalFunc = functionBody->GetIsGlobalFunc();
    this->jitData.bodyData.doJITLoopBody = functionBody->DoJITLoopBody();
    this->jitData.bodyData.hasScopeObject = functionBody->HasScopeObject();
    this->jitData.bodyData.hasImplicitArgIns = functionBody->GetHasImplicitArgIns();
    this->jitData.bodyData.hasCachedScopePropIds = functionBody->HasCachedScopePropIds();
    this->jitData.bodyData.inlineCachesOnFunctionObject = functionBody->GetInlineCachesOnFunctionObject();
    this->jitData.bodyData.doInterruptProbe = functionBody->GetScriptContext()->GetThreadContext()->DoInterruptProbe(functionBody);
    this->jitData.bodyData.disableInlineSpread = functionBody->IsInlineSpreadDisabled();

    this->jitData.bodyData.scriptIdAddr = (intptr_t)functionBody->GetAddressOfScriptId();
    this->jitData.bodyData.flagsAddr = (intptr_t)functionBody->GetAddressOfFlags();
    this->jitData.bodyData.probeCountAddr = (intptr_t)&functionBody->GetSourceInfo()->m_probeCount;

    this->jitData.bodyData.referencedPropertyIdCount = functionBody->GetReferencedPropertyIdCount();
    this->jitData.bodyData.referencedPropertyIdMap = functionBody->GetReferencedPropertyIdMap();

    // work item data
    this->jitData.type = type;
    this->jitData.isJitInDebugMode = isJitInDebugMode;
    ResetJitMode();

    this->jitData.loopNumber = GetLoopNumber();
}

CodeGenWorkItem::~CodeGenWorkItem()
{
    if(queuedFullJitWorkItem)
    {
        HeapDelete(queuedFullJitWorkItem);
    }
}

//
// Helps determine whether a function should be speculatively jitted.
// This function is only used once and is used in a time-critical area, so
// be careful with it (moving it around actually caused around a 5% perf
// regression on a test).
//
bool CodeGenWorkItem::ShouldSpeculativelyJit(uint byteCodeSizeGenerated) const
{
    if(PHASE_OFF(Js::FullJitPhase, this->functionBody))
    {
        return false;
    }

    byteCodeSizeGenerated += this->GetByteCodeCount();
    if(CONFIG_FLAG(ProfileBasedSpeculativeJit))
    {
        Assert(!CONFIG_ISENABLED(Js::NoDynamicProfileInMemoryCacheFlag));

        // JIT this now if we are under the speculation cap.
        return
            byteCodeSizeGenerated < (uint)CONFIG_FLAG(SpeculationCap) ||
            (
                byteCodeSizeGenerated < (uint)CONFIG_FLAG(ProfileBasedSpeculationCap) &&
                this->ShouldSpeculativelyJitBasedOnProfile()
            );
    }
    else
    {
        return byteCodeSizeGenerated < (uint)CONFIG_FLAG(SpeculationCap);
    }
}

bool CodeGenWorkItem::ShouldSpeculativelyJitBasedOnProfile() const
{
    Js::FunctionBody* functionBody = this->GetFunctionBody();

    uint loopPercentage = (functionBody->GetByteCodeInLoopCount()*100) / (functionBody->GetByteCodeCount() + 1);
    uint straightLineSize = functionBody->GetByteCodeCount() - functionBody->GetByteCodeInLoopCount();

    // This ensures only small and loopy functions are prejitted.
    if(loopPercentage >= 50 || straightLineSize < 300)
    {
        Js::SourceDynamicProfileManager* profileManager = functionBody->GetSourceContextInfo()->sourceDynamicProfileManager;
        if(profileManager != nullptr)
        {
            functionBody->SetIsSpeculativeJitCandidate();

            if(!functionBody->HasDynamicProfileInfo())
            {
                return false;
            }

            Js::ExecutionFlags executionFlags = profileManager->IsFunctionExecuted(functionBody->GetLocalFunctionId());
            if(executionFlags == Js::ExecutionFlags_Executed)
            {
                return true;
            }
        }
    }
    return false;
}

/*
    A comment about how to cause certain phases to only be on:

    INT = Interpreted, SJ = SimpleJit, FJ = FullJit

    To get only the following levels on, use the flags:

    INT:         -noNative
    SJ :         -forceNative -off:fullJit
    FJ :         -forceNative -off:simpleJit
    INT, SJ:     -off:fullJit
    INT, FJ:     -off:simpleJit
    SJ, FG:      -forceNative
    INT, SJ, FG: (default)
*/

void CodeGenWorkItem::OnAddToJitQueue()
{
    Assert(!this->isInJitQueue);
    this->isInJitQueue = true;
    VerifyJitMode();

    this->entryPointInfo->SetCodeGenQueued();
    if(IS_JS_ETW(EventEnabledJSCRIPT_FUNCTION_JIT_QUEUED()))
    {
        WCHAR displayNameBuffer[256];
        WCHAR* displayName = displayNameBuffer;
        size_t sizeInChars = this->GetDisplayName(displayName, 256);
        if(sizeInChars > 256)
        {
            displayName = HeapNewArray(WCHAR, sizeInChars);
            this->GetDisplayName(displayName, 256);
        }
        JS_ETW(EventWriteJSCRIPT_FUNCTION_JIT_QUEUED(
            this->GetFunctionNumber(),
            displayName,
            this->GetScriptContext(),
            this->GetInterpretedCount()));

        if(displayName != displayNameBuffer)
        {
            HeapDeleteArray(sizeInChars, displayName);
        }
    }
}

void CodeGenWorkItem::OnRemoveFromJitQueue(NativeCodeGenerator* generator)
{
    // This is called from within the lock

    this->isInJitQueue = false;
    this->entryPointInfo->SetCodeGenPending();
    functionBody->GetScriptContext()->GetThreadContext()->UnregisterCodeGenRecyclableData(this->recyclableData);
    this->recyclableData = nullptr;

    if(IS_JS_ETW(EventEnabledJSCRIPT_FUNCTION_JIT_DEQUEUED()))
    {
        WCHAR displayNameBuffer[256];
        WCHAR* displayName = displayNameBuffer;
        size_t sizeInChars = this->GetDisplayName(displayName, 256);
        if(sizeInChars > 256)
        {
            displayName = HeapNewArray(WCHAR, sizeInChars);
            this->GetDisplayName(displayName, 256);
        }
        JS_ETW(EventWriteJSCRIPT_FUNCTION_JIT_DEQUEUED(
            this->GetFunctionNumber(),
            displayName,
            this->GetScriptContext(),
            this->GetInterpretedCount()));

        if(displayName != displayNameBuffer)
        {
            HeapDeleteArray(sizeInChars, displayName);
        }
    }

    if(this->Type() == JsLoopBodyWorkItemType)
    {
        // Go ahead and delete it and let it re-queue if more interpreting of the loop happens
        auto loopBodyWorkItem = static_cast<JsLoopBodyCodeGen*>(this);
        loopBodyWorkItem->loopHeader->ResetInterpreterCount();
        loopBodyWorkItem->GetEntryPoint()->Reset();
        HeapDelete(loopBodyWorkItem);
    }
    else
    {
        Assert(GetJitMode() == ExecutionMode::FullJit); // simple JIT work items are not removed from the queue

        GetFunctionBody()->OnFullJitDequeued(static_cast<Js::FunctionEntryPointInfo *>(GetEntryPoint()));

        // Add it back to the list of available functions to be jitted
        generator->AddWorkItem(this);
    }
}

void CodeGenWorkItem::OnWorkItemProcessFail(NativeCodeGenerator* codeGen)
{
    if (!isAllocationCommitted && this->allocation != nullptr && this->allocation->allocation != nullptr)
    {
#if DBG
        this->allocation->allocation->isNotExecutableBecauseOOM = true;
#endif
        codeGen->FreeNativeCodeGenAllocation(this->allocation->allocation->address);
    }
}

void CodeGenWorkItem::FinalizeNativeCode(Func *func)
{
    NativeCodeData * data = func->GetNativeCodeDataAllocator()->Finalize();
    NativeCodeData * transferData = func->GetTransferDataAllocator()->Finalize();
    CodeGenNumberChunk * numberChunks = func->GetNumberAllocator()->Finalize();
    this->functionBody->RecordNativeBaseAddress((BYTE *)GetCodeAddress(), GetCodeSize(), data, transferData, numberChunks, GetEntryPoint(), GetLoopNumber());
    func->GetEmitBufferManager()->CompletePreviousAllocation(this->GetAllocation());
}

QueuedFullJitWorkItem *CodeGenWorkItem::GetQueuedFullJitWorkItem() const
{
    return queuedFullJitWorkItem;
}

QueuedFullJitWorkItem *CodeGenWorkItem::EnsureQueuedFullJitWorkItem()
{
    if(queuedFullJitWorkItem)
    {
        return queuedFullJitWorkItem;
    }

    queuedFullJitWorkItem = HeapNewNoThrow(QueuedFullJitWorkItem, this);
    return queuedFullJitWorkItem;
}
