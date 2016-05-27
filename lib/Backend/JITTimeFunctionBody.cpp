//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeFunctionBody::JITTimeFunctionBody(FunctionBodyJITData * bodyData) :
    m_bodyData(bodyData),
    m_asmJsInfo(bodyData->asmJsData),
    m_profileInfo(bodyData->profileData)
{
    InitializeStatementMap();
}

/* static */
void
JITTimeFunctionBody::InitializeJITFunctionData(
    __in Js::FunctionBody *functionBody,
    __out FunctionBodyJITData * jitBody)
{
    // bytecode
    jitBody->byteCodeLength = functionBody->GetByteCode()->GetLength();
    jitBody->byteCodeBuffer = functionBody->GetByteCode()->GetBuffer();

    // const table
    jitBody->constCount = functionBody->GetConstantCount();
    if (functionBody->GetConstantCount() > 0)
    {
        // TODO (michhol): OOP JIT, will be different for asm.js
        jitBody->constTable = (intptr_t *)functionBody->GetConstTable();

        if (functionBody->GetIsAsmJsFunction())
        {
            // asm.js has const table structured differently and doesn't need type info
            jitBody->constTypeCount = 0;
            jitBody->constTypeTable = nullptr;
        }
        else
        {
            jitBody->constTypeCount = functionBody->GetConstantCount();
            jitBody->constTypeTable = HeapNewArray(int32, functionBody->GetConstantCount());
            for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < functionBody->GetConstantCount(); ++reg)
            {
                Js::Var varConst = functionBody->GetConstantVar(reg);
                Assert(varConst != nullptr);
                if (Js::TaggedInt::Is(varConst) ||
                    varConst == (Js::Var)&Js::NullFrameDisplay ||
                    varConst == (Js::Var)&Js::StrictNullFrameDisplay)
                {
                    // don't need TypeId for these
                    jitBody->constTypeTable[reg - Js::FunctionBody::FirstRegSlot] = Js::TypeId::TypeIds_Limit;
                }
                else
                {
                    jitBody->constTypeTable[reg - Js::FunctionBody::FirstRegSlot] = Js::JavascriptOperators::GetTypeId(varConst);
                }
            }
        }
    }
    else
    {
        jitBody->constTypeCount = 0;
        jitBody->constTypeTable = nullptr;
        jitBody->constTable = nullptr;
    }

    // statement map
    Js::SmallSpanSequence * statementMap = functionBody->GetStatementMapSpanSequence();

    jitBody->statementMap.baseValue = statementMap->baseValue;

    if (statementMap->pActualOffsetList)
    {
        jitBody->statementMap.actualOffsetLength = statementMap->pActualOffsetList->GetLength();
        jitBody->statementMap.actualOffsetList = statementMap->pActualOffsetList->GetBuffer();
    }
    else
    {
        jitBody->statementMap.actualOffsetLength = 0;
        jitBody->statementMap.actualOffsetList = nullptr;
    }

    if (statementMap->pStatementBuffer)
    {
        jitBody->statementMap.statementLength = statementMap->pStatementBuffer->GetLength();
        jitBody->statementMap.statementBuffer = statementMap->pStatementBuffer->GetBuffer();
    }
    else
    {
        jitBody->statementMap.statementLength = 0;
        jitBody->statementMap.statementBuffer = nullptr;
    }

    jitBody->inlineCacheCount = functionBody->GetInlineCacheCount();
    if (functionBody->GetInlineCacheCount() > 0)
    {
        jitBody->cacheIdToPropertyIdMap = functionBody->GetCacheIdToPropertyIdMap();
        jitBody->inlineCaches = reinterpret_cast<intptr_t*>(functionBody->GetInlineCaches());
    }
    else
    {
        jitBody->inlineCaches = nullptr;
        jitBody->cacheIdToPropertyIdMap = nullptr;
    }

    // body data
    jitBody->functionBodyAddr = (intptr_t)functionBody;

    jitBody->funcNumber = functionBody->GetFunctionNumber();
    jitBody->localFuncId = functionBody->GetLocalFunctionId();
    jitBody->sourceContextId = functionBody->GetSourceContextId();
    jitBody->nestedCount = functionBody->GetNestedCount();
    if (functionBody->GetNestedCount() > 0)
    {
        jitBody->nestedFuncArrayAddr = (intptr_t)functionBody->GetNestedFuncArray();
    }
    else
    {
        jitBody->nestedFuncArrayAddr = 0;
    }
    jitBody->scopeSlotArraySize = functionBody->scopeSlotArraySize;
    jitBody->attributes = functionBody->GetAttributes();
    jitBody->isInstInlineCacheCount = functionBody->GetIsInstInlineCacheCount();

    if (functionBody->GetUtf8SourceInfo()->GetCbLength() > UINT_MAX)
    {
        Js::Throw::OutOfMemory();
    }

    jitBody->byteCodeCount = functionBody->GetByteCodeCount();
    jitBody->byteCodeInLoopCount = functionBody->GetByteCodeInLoopCount();
    jitBody->nonLoadByteCodeCount = functionBody->GetByteCodeWithoutLDACount();
    jitBody->loopCount = functionBody->GetLoopCount();

    if (functionBody->GetHasAllocatedLoopHeaders())
    {
        jitBody->loopHeaderArrayAddr = (intptr_t)functionBody->GetLoopHeaderArrayPtr();
        jitBody->loopHeaderArrayLength = functionBody->GetLoopCount();
        jitBody->loopHeaders = HeapNewArray(JITLoopHeader, functionBody->GetLoopCount());
        for (uint i = 0; i < functionBody->GetLoopCount(); ++i)
        {
            jitBody->loopHeaders[i].startOffset = functionBody->GetLoopHeader(i)->startOffset;
            jitBody->loopHeaders[i].endOffset = functionBody->GetLoopHeader(i)->endOffset;
            jitBody->loopHeaders[i].isNested = functionBody->GetLoopHeader(i)->isNested;
            jitBody->loopHeaders[i].isInTry = functionBody->GetLoopHeader(i)->isInTry;
            jitBody->loopHeaders[i].interpretCount = functionBody->GetLoopInterpretCount(functionBody->GetLoopHeader(i));
        }
    }
    else
    {
        jitBody->loopHeaderArrayAddr = 0;
        jitBody->loopHeaderArrayLength = 0;
        jitBody->loopHeaders = nullptr;
    }

    jitBody->localFrameDisplayReg = functionBody->GetLocalFrameDisplayReg();
    jitBody->localClosureReg = functionBody->GetLocalClosureReg();
    jitBody->envReg = functionBody->GetEnvReg();
    jitBody->firstTmpReg = functionBody->GetFirstTmpReg();
    jitBody->varCount = functionBody->GetVarCount();
    jitBody->innerScopeCount = functionBody->GetInnerScopeCount();
    if (functionBody->GetInnerScopeCount() > 0)
    {
        jitBody->firstInnerScopeReg = functionBody->FirstInnerScopeReg();
    }
    jitBody->envDepth = functionBody->GetEnvDepth();
    jitBody->profiledIterations = functionBody->GetProfiledIterations();
    jitBody->profiledCallSiteCount = functionBody->GetProfiledCallSiteCount();
    jitBody->inParamCount = functionBody->GetInParamsCount();
    jitBody->thisRegisterForEventHandler = functionBody->GetThisRegForEventHandler();
    jitBody->funcExprScopeRegister = functionBody->GetFuncExprScopeReg();
    jitBody->recursiveCallSiteCount = functionBody->GetNumberOfRecursiveCallSites();
    jitBody->argUsedForBranch = functionBody->m_argUsedForBranch;

    jitBody->flags = functionBody->GetFlags();

    jitBody->doBackendArgumentsOptimization = functionBody->GetDoBackendArgumentsOptimization();
    jitBody->isLibraryCode = functionBody->GetUtf8SourceInfo()->GetIsLibraryCode();
    jitBody->isAsmJsMode = functionBody->GetIsAsmjsMode();
    jitBody->isStrictMode = functionBody->GetIsStrictMode();
    jitBody->isEval = functionBody->IsEval();
    jitBody->isGlobalFunc = functionBody->GetIsGlobalFunc();
    jitBody->doJITLoopBody = functionBody->DoJITLoopBody();
    jitBody->hasScopeObject = functionBody->HasScopeObject();
    jitBody->hasImplicitArgIns = functionBody->GetHasImplicitArgIns();
    jitBody->hasCachedScopePropIds = functionBody->HasCachedScopePropIds();
    jitBody->inlineCachesOnFunctionObject = functionBody->GetInlineCachesOnFunctionObject();
    jitBody->doInterruptProbe = functionBody->GetScriptContext()->GetThreadContext()->DoInterruptProbe(functionBody);
    jitBody->disableInlineSpread = functionBody->IsInlineSpreadDisabled();
    jitBody->hasNestedLoop = functionBody->GetHasNestedLoop();
    jitBody->hasNonBuiltInCallee = functionBody->HasNonBuiltInCallee();

    jitBody->scriptIdAddr = (intptr_t)functionBody->GetAddressOfScriptId();
    jitBody->flagsAddr = (intptr_t)functionBody->GetAddressOfFlags();
    jitBody->probeCountAddr = (intptr_t)&functionBody->GetSourceInfo()->m_probeCount;
    jitBody->regAllocLoadCountAddr = (intptr_t)&functionBody->regAllocLoadCount;
    jitBody->regAllocStoreCountAddr = (intptr_t)&functionBody->regAllocStoreCount;
    jitBody->callCountStatsAddr = (intptr_t)&functionBody->callCountStats;

    jitBody->referencedPropertyIdCount = functionBody->GetReferencedPropertyIdCount();
    jitBody->referencedPropertyIdMap = functionBody->GetReferencedPropertyIdMap();

    jitBody->nameLength = functionBody->GetDisplayNameLength();
    jitBody->displayName = (wchar_t *)functionBody->GetDisplayName();

    if (functionBody->GetIsAsmJsFunction())
    {
        jitBody->asmJsData = HeapNew(AsmJsJITData);
        Js::AsmJsFunctionInfo * asmFuncInfo = functionBody->GetAsmJsFunctionInfo();
        jitBody->asmJsData->intConstCount = asmFuncInfo->GetIntConstCount();
        jitBody->asmJsData->doubleConstCount = asmFuncInfo->GetDoubleConstCount();
        jitBody->asmJsData->floatConstCount = asmFuncInfo->GetFloatConstCount();
        jitBody->asmJsData->simdConstCount = asmFuncInfo->GetSimdConstCount();
        jitBody->asmJsData->intTmpCount = asmFuncInfo->GetIntTmpCount();
        jitBody->asmJsData->doubleTmpCount = asmFuncInfo->GetDoubleTmpCount();
        jitBody->asmJsData->floatTmpCount = asmFuncInfo->GetFloatTmpCount();
        jitBody->asmJsData->simdTmpCount = asmFuncInfo->GetSimdTmpCount();
        jitBody->asmJsData->intVarCount = asmFuncInfo->GetIntVarCount();
        jitBody->asmJsData->doubleVarCount = asmFuncInfo->GetDoubleVarCount();
        jitBody->asmJsData->floatVarCount = asmFuncInfo->GetFloatVarCount();
        jitBody->asmJsData->simdVarCount = asmFuncInfo->GetSimdVarCount();
        jitBody->asmJsData->intByteOffset = asmFuncInfo->GetIntByteOffset();
        jitBody->asmJsData->doubleByteOffset = asmFuncInfo->GetDoubleByteOffset();
        jitBody->asmJsData->floatByteOffset = asmFuncInfo->GetFloatByteOffset();
        jitBody->asmJsData->simdByteOffset = asmFuncInfo->GetSimdByteOffset();
        jitBody->asmJsData->argCount = asmFuncInfo->GetArgCount();
        jitBody->asmJsData->argTypeArray = (byte*)asmFuncInfo->GetArgTypeArray();
        jitBody->asmJsData->argByteSize = asmFuncInfo->GetArgByteSize();
        jitBody->asmJsData->retType = asmFuncInfo->GetReturnType().which();
        jitBody->asmJsData->isHeapBufferConst = asmFuncInfo->IsHeapBufferConst();
        jitBody->asmJsData->usesHeapBuffer = asmFuncInfo->UsesHeapBuffer();
        jitBody->asmJsData->totalSizeInBytes = asmFuncInfo->GetTotalSizeinBytes();
    }
    else
    {
        jitBody->asmJsData = nullptr;
    }

    if (functionBody->GetCodeGenRuntimeData())
    {
        jitBody->runtimeDataCount = functionBody->GetProfiledCallSiteCount();
        Assert(functionBody->GetProfiledCallSiteCount() > 0);
        jitBody->profiledRuntimeData = HeapNewArrayZ(FunctionJITRuntimeData, jitBody->runtimeDataCount);
        for (int i = 0; i < jitBody->runtimeDataCount; ++i)
        {
            if (functionBody->GetCodeGenRuntimeData()[i]->ClonedInlineCaches())
            {
                jitBody->profiledRuntimeData[i]->clonedCacheCount = functionBody->GetInlineCacheCount();
                jitBody->profiledRuntimeData[i]->clonedInlineCaches = functionBody->GetCodeGenRuntimeData()[i]->ClonedInlineCaches()->inlineCaches;
            }
        }
    }
    else
    {
        jitBody->runtimeDataCount = 0;
        jitBody->profiledRuntimeData = nullptr;
    }
}

intptr_t
JITTimeFunctionBody::GetAddr() const
{
    return m_bodyData->functionBodyAddr;
}

uint
JITTimeFunctionBody::GetFunctionNumber() const
{
    return m_bodyData->funcNumber;
}

uint
JITTimeFunctionBody::GetLocalFunctionId() const
{
    return m_bodyData->localFuncId;
}

uint
JITTimeFunctionBody::GetSourceContextId() const
{
    return m_bodyData->sourceContextId;
}

uint
JITTimeFunctionBody::GetNestedCount() const
{
    return m_bodyData->nestedCount;
}

uint
JITTimeFunctionBody::GetScopeSlotArraySize() const
{
    return m_bodyData->scopeSlotArraySize;
}

uint
JITTimeFunctionBody::GetByteCodeCount() const
{
    return m_bodyData->byteCodeCount;
}

uint
JITTimeFunctionBody::GetByteCodeInLoopCount() const
{
    return m_bodyData->byteCodeInLoopCount;
}

uint
JITTimeFunctionBody::GetNonLoadByteCodeCount() const
{
    return m_bodyData->nonLoadByteCodeCount;
}

uint
JITTimeFunctionBody::GetLoopCount() const
{
    return m_bodyData->loopCount;
}

bool
JITTimeFunctionBody::HasLoops() const
{
    return GetLoopCount() != 0;
}

uint
JITTimeFunctionBody::GetByteCodeLength() const
{
    return m_bodyData->byteCodeLength;
}

uint
JITTimeFunctionBody::GetInnerScopeCount() const
{
    return m_bodyData->innerScopeCount;
}

uint
JITTimeFunctionBody::GetInlineCacheCount() const
{
    return m_bodyData->inlineCacheCount;
}

uint
JITTimeFunctionBody::GetRecursiveCallSiteCount() const
{
    return m_bodyData->recursiveCallSiteCount;
}

Js::RegSlot
JITTimeFunctionBody::GetLocalFrameDisplayReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->localFrameDisplayReg);
}

Js::RegSlot
JITTimeFunctionBody::GetLocalClosureReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->localClosureReg);
}

Js::RegSlot
JITTimeFunctionBody::GetEnvReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->envReg);
}

Js::RegSlot
JITTimeFunctionBody::GetFirstTmpReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->firstTmpReg);
}

Js::RegSlot
JITTimeFunctionBody::GetFirstInnerScopeReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->firstInnerScopeReg);
}

Js::RegSlot
JITTimeFunctionBody::GetVarCount() const
{
    return static_cast<Js::RegSlot>(m_bodyData->varCount);
}

Js::RegSlot
JITTimeFunctionBody::GetConstCount() const
{
    return static_cast<Js::RegSlot>(m_bodyData->constCount);
}

Js::RegSlot
JITTimeFunctionBody::GetLocalsCount() const
{
    return GetConstCount() + GetVarCount();
}

Js::RegSlot
JITTimeFunctionBody::GetTempCount() const
{
    return GetLocalsCount() - GetFirstTmpReg();
}

Js::RegSlot
JITTimeFunctionBody::GetFuncExprScopeReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData->funcExprScopeRegister);
}

Js::RegSlot
JITTimeFunctionBody::GetThisRegForEventHandler() const
{
    return static_cast<Js::RegSlot>(m_bodyData->thisRegisterForEventHandler);
}

Js::RegSlot
JITTimeFunctionBody::GetFirstNonTempLocalIndex() const
{
    // First local var starts when the const vars end.
    return GetConstCount();
}

Js::RegSlot
JITTimeFunctionBody::GetEndNonTempLocalIndex() const
{
    // It will give the index on which current non temp locals ends, which is a first temp reg.
    return GetFirstTmpReg() != Js::Constants::NoRegister ? GetFirstTmpReg() : GetLocalsCount();
}

Js::RegSlot
JITTimeFunctionBody::GetNonTempLocalVarCount() const
{
    Assert(GetEndNonTempLocalIndex() >= GetFirstNonTempLocalIndex());
    return GetEndNonTempLocalIndex() - GetFirstNonTempLocalIndex();
}

Js::RegSlot
JITTimeFunctionBody::GetRestParamRegSlot() const
{
    Js::RegSlot dstRegSlot = GetConstCount();
    if (HasImplicitArgIns())
    {
        dstRegSlot += GetInParamsCount() - 1;
    }
    return dstRegSlot;
}

Js::PropertyId
JITTimeFunctionBody::GetPropertyIdFromCacheId(uint cacheId) const
{
    Assert(m_bodyData->cacheIdToPropertyIdMap);
    Assert(cacheId < GetInlineCacheCount());
    return static_cast<Js::PropertyId>(m_bodyData->cacheIdToPropertyIdMap[cacheId]);
}

Js::PropertyId
JITTimeFunctionBody::GetReferencedPropertyId(uint index) const
{
    if (index < (uint)TotalNumberOfBuiltInProperties)
    {
        return index;
    }
    uint mapIndex = index - TotalNumberOfBuiltInProperties;

    Assert(m_bodyData->referencedPropertyIdMap != nullptr);
    Assert(mapIndex < m_bodyData->referencedPropertyIdCount);

    return m_bodyData->referencedPropertyIdMap[mapIndex];
}

uint16
JITTimeFunctionBody::GetArgUsedForBranch() const
{
    return m_bodyData->argUsedForBranch;
}

uint16
JITTimeFunctionBody::GetEnvDepth() const
{
    return m_bodyData->envDepth;
}

uint16
JITTimeFunctionBody::GetProfiledIterations() const
{
    return m_bodyData->profiledIterations;
}

Js::ProfileId
JITTimeFunctionBody::GetProfiledCallSiteCount() const
{
    return static_cast<Js::ProfileId>(m_bodyData->profiledCallSiteCount);
}

Js::ArgSlot
JITTimeFunctionBody::GetInParamsCount() const
{
    return static_cast<Js::ArgSlot>(m_bodyData->inParamCount);
}

bool
JITTimeFunctionBody::DoStackNestedFunc() const
{
    return Js::FunctionBody::DoStackNestedFunc(GetFlags());
}

bool
JITTimeFunctionBody::DoStackClosure() const
{
    return DoStackNestedFunc()
        && GetNestedCount() != 0
        && GetScopeSlotArraySize() != 0
        && GetEnvDepth() != (uint16)-1;
}

bool
JITTimeFunctionBody::HasTry() const
{
    return Js::FunctionBody::GetHasTry(GetFlags());
}

bool
JITTimeFunctionBody::HasOrParentHasArguments() const
{
    return Js::FunctionBody::GetHasOrParentHasArguments(GetFlags());
}

bool
JITTimeFunctionBody::DoBackendArgumentsOptimization() const
{
    return m_bodyData->doBackendArgumentsOptimization != FALSE;
}

bool
JITTimeFunctionBody::IsLibraryCode() const
{
    return m_bodyData->isLibraryCode != FALSE;
}

bool
JITTimeFunctionBody::IsAsmJsMode() const
{
    return m_bodyData->isAsmJsMode != FALSE;
}

bool
JITTimeFunctionBody::IsStrictMode() const
{
    return m_bodyData->isStrictMode != FALSE;
}

bool
JITTimeFunctionBody::IsEval() const
{
    return m_bodyData->isEval != FALSE;
}

bool
JITTimeFunctionBody::HasScopeObject() const
{
    return m_bodyData->hasScopeObject != FALSE;
}

bool
JITTimeFunctionBody::HasNestedLoop() const
{
    return m_bodyData->hasNestedLoop != FALSE;
}

bool
JITTimeFunctionBody::IsGenerator() const
{
    return Js::FunctionInfo::IsGenerator(GetAttributes());
}

bool
JITTimeFunctionBody::HasImplicitArgIns() const
{
    return m_bodyData->hasImplicitArgIns != FALSE;
}

bool
JITTimeFunctionBody::HasCachedScopePropIds() const
{
    return m_bodyData->hasCachedScopePropIds != FALSE;
}

bool
JITTimeFunctionBody::HasInlineCachesOnFunctionObject() const
{
    return m_bodyData->inlineCachesOnFunctionObject != FALSE;
}

bool
JITTimeFunctionBody::DoInterruptProbe() const
{
    // TODO michhol: this is technically a threadcontext flag,
    // may want to pass all these when initializing thread context
    return m_bodyData->doInterruptProbe != FALSE;
}

bool
JITTimeFunctionBody::HasRestParameter() const
{
    return Js::FunctionBody::GetHasRestParameter(GetFlags());
}

bool
JITTimeFunctionBody::IsGlobalFunc() const
{
    return m_bodyData->isGlobalFunc != FALSE;
}

bool
JITTimeFunctionBody::IsNonTempLocalVar(uint32 varIndex) const
{
    return GetFirstNonTempLocalIndex() <= varIndex && varIndex < GetEndNonTempLocalIndex();
}

bool
JITTimeFunctionBody::DoJITLoopBody() const
{
    return m_bodyData->doJITLoopBody != FALSE;
}

bool
JITTimeFunctionBody::IsInlineSpreadDisabled() const
{
    return m_bodyData->disableInlineSpread != FALSE;
}

bool
JITTimeFunctionBody::HasNonBuiltInCallee() const
{
    return m_bodyData->hasNonBuiltInCallee != FALSE;
}

bool
JITTimeFunctionBody::ForceJITLoopBody() const
{
    return
        !PHASE_OFF(Js::JITLoopBodyPhase, this) &&
        !PHASE_OFF(Js::FullJitPhase, this) &&
        !this->IsGenerator() &&
        !this->HasTry() &&
        (
            PHASE_FORCE(Js::JITLoopBodyPhase, this)
#ifdef ENABLE_PREJIT
            || Js::Configuration::Global.flags.Prejit
#endif
        );
}

bool
JITTimeFunctionBody::CanInlineRecursively(uint depth, bool tryAggressive) const
{
    uint recursiveInlineSpan = GetRecursiveCallSiteCount();

    uint minRecursiveInlineDepth = (uint)CONFIG_FLAG(RecursiveInlineDepthMin);

    if (recursiveInlineSpan != GetProfiledCallSiteCount() || tryAggressive == false)
    {
        return depth < minRecursiveInlineDepth;
    }

    uint maxRecursiveInlineDepth = (uint)CONFIG_FLAG(RecursiveInlineDepthMax);
    uint maxRecursiveBytecodeBudget = (uint)CONFIG_FLAG(RecursiveInlineThreshold);
    uint numberOfAllowedFuncs = maxRecursiveBytecodeBudget / GetNonLoadByteCodeCount();
    uint maxDepth;

    if (recursiveInlineSpan == 1)
    {
        maxDepth = numberOfAllowedFuncs;
    }
    else
    {
        maxDepth = (uint)ceil(log((double)((double)numberOfAllowedFuncs) / log((double)recursiveInlineSpan)));
    }
    maxDepth = maxDepth < minRecursiveInlineDepth ? minRecursiveInlineDepth : maxDepth;
    maxDepth = maxDepth < maxRecursiveInlineDepth ? maxDepth : maxRecursiveInlineDepth;
    return depth < maxDepth;
}

const byte *
JITTimeFunctionBody::GetByteCodeBuffer() const
{
    return m_bodyData->byteCodeBuffer;
}

Js::SmallSpanSequence *
JITTimeFunctionBody::GetStatementMapSpanSequence()
{
    return &m_statementMap;
}

intptr_t
JITTimeFunctionBody::GetScriptIdAddr() const
{
    return m_bodyData->scriptIdAddr;
}

intptr_t
JITTimeFunctionBody::GetProbeCountAddr() const
{
    return m_bodyData->probeCountAddr;
}

intptr_t
JITTimeFunctionBody::GetFlagsAddr() const
{
    return m_bodyData->flagsAddr;
}

intptr_t
JITTimeFunctionBody::GetRegAllocLoadCountAddr() const
{
    return m_bodyData->regAllocLoadCountAddr;
}

intptr_t
JITTimeFunctionBody::GetRegAllocStoreCountAddr() const
{
    return m_bodyData->regAllocStoreCountAddr;
}

intptr_t
JITTimeFunctionBody::GetCallCountStatsAddr() const
{
    return m_bodyData->callCountStatsAddr;
}

intptr_t
JITTimeFunctionBody::GetConstantVar(Js::RegSlot location) const
{
    Assert(m_bodyData->constTable != nullptr);
    Assert(location < GetConstCount());
    Assert(location != 0);

    return static_cast<intptr_t>(m_bodyData->constTable[location - Js::FunctionBody::FirstRegSlot]);
}

intptr_t
JITTimeFunctionBody::GetInlineCache(uint index) const
{
    Assert(m_bodyData->inlineCaches != nullptr);
    Assert(index < GetInlineCacheCount());
#if 0 // TODO: michhol OOP JIT, add these asserts
    Assert(this->m_inlineCacheTypes[index] == InlineCacheTypeNone ||
        this->m_inlineCacheTypes[index] == InlineCacheTypeInlineCache);
    this->m_inlineCacheTypes[index] = InlineCacheTypeInlineCache;
#endif
    return static_cast<intptr_t>(m_bodyData->inlineCaches[index]);
}

intptr_t
JITTimeFunctionBody::GetIsInstInlineCache(uint index) const
{
    Assert(m_bodyData->inlineCaches != nullptr);
    Assert(index < m_bodyData->isInstInlineCacheCount);
    index += GetInlineCacheCount();
#if 0 // TODO: michhol OOP JIT, add these asserts
    Assert(this->m_inlineCacheTypes[index] == InlineCacheTypeNone ||
        this->m_inlineCacheTypes[index] == InlineCacheTypeIsInst);
    this->m_inlineCacheTypes[index] = InlineCacheTypeIsInst;
#endif
    return static_cast<intptr_t>(m_bodyData->inlineCaches[index]);
}

Js::TypeId
JITTimeFunctionBody::GetConstantType(Js::RegSlot location) const
{
    Assert(m_bodyData->constTable != nullptr);
    Assert(location < GetConstCount());
    Assert(location != 0);

    return static_cast<Js::TypeId>(m_bodyData->constTypeTable[location - Js::FunctionBody::FirstRegSlot]);
}

void *
JITTimeFunctionBody::GetConstTable() const
{
    return m_bodyData->constTable;
}

intptr_t
JITTimeFunctionBody::GetRootObject() const
{
    Assert(m_bodyData->constTable != nullptr);
    return m_bodyData->constTable[Js::FunctionBody::RootObjectRegSlot - Js::FunctionBody::FirstRegSlot];
}

intptr_t
JITTimeFunctionBody::GetNestedFuncRef(uint index) const
{
    Assert(index < GetNestedCount());
    intptr_t baseAddr = m_bodyData->nestedFuncArrayAddr;
    return baseAddr + (index * sizeof(void*));
}

intptr_t
JITTimeFunctionBody::GetLoopHeaderAddr(uint loopNum) const
{
    Assert(loopNum < GetLoopCount());
    intptr_t baseAddr = m_bodyData->loopHeaderArrayAddr;
    return baseAddr + (loopNum * sizeof(Js::LoopHeader));
}

const JITLoopHeader *
JITTimeFunctionBody::GetLoopHeaderData(uint loopNum) const
{
    Assert(loopNum < GetLoopCount());
    return &m_bodyData->loopHeaders[loopNum];
}

const AsmJsJITInfo *
JITTimeFunctionBody::GetAsmJsInfo() const
{
    return &m_asmJsInfo;
}


const JITTimeProfileInfo *
JITTimeFunctionBody::GetProfileInfo() const
{
    return &m_profileInfo;
}

/* static */
bool
JITTimeFunctionBody::LoopContains(const JITLoopHeader * loop1, const JITLoopHeader * loop2)
{
    return (loop1->startOffset <= loop2->startOffset && loop2->endOffset <= loop1->endOffset);
}

Js::FunctionBody::FunctionBodyFlags
JITTimeFunctionBody::GetFlags() const
{
    return static_cast<Js::FunctionBody::FunctionBodyFlags>(m_bodyData->flags);
}

Js::FunctionInfo::Attributes
JITTimeFunctionBody::GetAttributes() const
{
    return static_cast<Js::FunctionInfo::Attributes>(m_bodyData->attributes);
}

void
JITTimeFunctionBody::InitializeStatementMap()
{
    const uint statementsLength = m_bodyData->statementMap.statementLength;
    const uint offsetsLength = m_bodyData->statementMap.actualOffsetLength;

    m_statementMap.baseValue = m_bodyData->statementMap.baseValue;

    if (statementsLength > 0)
    {
        // TODO: (michhol OOP JIT) should be able to directly use statementMap.statementBuffer
        m_statementMap.pStatementBuffer = JsUtil::GrowingUint32HeapArray::Create(statementsLength);
        m_statementMap.pStatementBuffer->SetCount(statementsLength);
        js_memcpy_s(
            m_statementMap.pStatementBuffer->GetBuffer(),
            m_statementMap.pStatementBuffer->Count() * sizeof(uint32),
            m_bodyData->statementMap.statementBuffer,
            statementsLength * sizeof(uint32));
    }

    if (offsetsLength > 0)
    {
        m_statementMap.pActualOffsetList = JsUtil::GrowingUint32HeapArray::Create(offsetsLength);
        m_statementMap.pActualOffsetList->SetCount(offsetsLength);
        js_memcpy_s(
            m_statementMap.pActualOffsetList->GetBuffer(),
            m_statementMap.pActualOffsetList->Count() * sizeof(uint32),
            m_bodyData->statementMap.actualOffsetList,
            offsetsLength * sizeof(uint32));
    }
}

wchar_t*
JITTimeFunctionBody::GetDisplayName() const
{
    return m_bodyData->displayName;
}

wchar_t*
JITTimeFunctionBody::GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const
{
    // (#%u.%u), #%u --> (source file Id . function Id) , function Number
    int len = swprintf_s(bufferToWriteTo, MAX_FUNCTION_BODY_DEBUG_STRING_SIZE, L" (#%d.%u), #%u",
        (int)GetSourceContextId(), GetLocalFunctionId(), GetFunctionNumber());
    Assert(len > 8);
    return bufferToWriteTo;
}
