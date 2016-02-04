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
    // TODO: (michhol) put bodyData directly on function body rather than doing this copying
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

    // body data
    this->jitData.bodyData.funcNumber = functionBody->GetFunctionNumber();
    this->jitData.bodyData.localFuncId = functionBody->GetLocalFunctionId();
    this->jitData.bodyData.sourceContextId = functionBody->GetSourceContextId();
    this->jitData.bodyData.nestedCount = functionBody->GetNestedCount();
    this->jitData.bodyData.scopeSlotArraySize = functionBody->scopeSlotArraySize;
    this->jitData.bodyData.attributes = functionBody->GetAttributes();

    if (functionBody->GetUtf8SourceInfo()->GetCbLength() > UINT_MAX)
    {
        Js::Throw::OutOfMemory();
    }

    this->jitData.bodyData.byteCodeCount = functionBody->GetByteCodeCount();
    this->jitData.bodyData.byteCodeInLoopCount = functionBody->GetByteCodeInLoopCount();
    this->jitData.bodyData.loopCount = functionBody->GetLoopCount();
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
    this->jitData.bodyData.profiledCallSiteCount = functionBody->GetProfiledCallSiteCount();
    this->jitData.bodyData.inParamCount = functionBody->GetInParamsCount();
    this->jitData.bodyData.thisRegisterForEventHandler = functionBody->GetThisRegForEventHandler();
    this->jitData.bodyData.funcExprScopeRegister = functionBody->GetFuncExprScopeReg();

    this->jitData.bodyData.flags = functionBody->GetFlags();

    this->jitData.bodyData.doBackendArgumentsOptimization = functionBody->GetDoBackendArgumentsOptimization();
    this->jitData.bodyData.isLibraryCode = functionBody->GetUtf8SourceInfo()->GetIsLibraryCode();
    this->jitData.bodyData.isAsmJsMode = functionBody->GetIsAsmjsMode();
    this->jitData.bodyData.isStrictMode = functionBody->GetIsStrictMode();
    this->jitData.bodyData.hasScopeObject = functionBody->HasScopeObject();
    this->jitData.bodyData.hasImplicitArgIns = functionBody->GetHasImplicitArgIns();
    this->jitData.bodyData.hasCachedScopePropIds = functionBody->HasCachedScopePropIds();

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
    if(!functionBody->DoFullJit())
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
    uint straighLineSize = functionBody->GetByteCodeCount() - functionBody->GetByteCodeInLoopCount();

    // This ensures only small and loopy functions are prejitted.
    if(loopPercentage >= 50 || straighLineSize < 300)
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

void CodeGenWorkItem::RecordNativeCodeSize(Func *func, size_t bytes, ushort pdataCount, ushort xdataSize)
{
    BYTE *buffer;
#if defined(_M_ARM32_OR_ARM64)
    bool canAllocInPreReservedHeapPageSegment = false;
#else
    bool canAllocInPreReservedHeapPageSegment = func->CanAllocInPreReservedHeapPageSegment();
#endif
    EmitBufferAllocation *allocation = func->GetEmitBufferManager()->AllocateBuffer(bytes, &buffer, false, pdataCount, xdataSize, canAllocInPreReservedHeapPageSegment, true);

    Assert(allocation != nullptr);
    if (buffer == nullptr)
        Js::Throw::OutOfMemory();

    SetCodeAddress((size_t)buffer);
    SetCodeSize(bytes);
    SetPdataCount(pdataCount);
    SetXdataSize(xdataSize);
    SetAllocation(allocation);
}

void CodeGenWorkItem::RecordNativeCode(Func *func, const BYTE* sourceBuffer)
{
    if (!func->GetEmitBufferManager()->CommitBuffer(this->GetAllocation(), (BYTE *)GetCodeAddress(), GetCodeSize(), sourceBuffer))
    {
        Js::Throw::OutOfMemory();
    }

    this->isAllocationCommitted = true;

#if DBG_DUMP
    if (Type() == JsLoopBodyWorkItemType)
    {
        func->GetEmitBufferManager()->totalBytesLoopBody += GetCodeSize();
    }
#endif
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
