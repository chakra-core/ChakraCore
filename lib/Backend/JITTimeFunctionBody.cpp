//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeFunctionBody::JITTimeFunctionBody(FunctionBodyDataIDL * bodyData) :
    m_bodyData(*bodyData)
{
    CompileAssert(sizeof(JITTimeFunctionBody) == sizeof(FunctionBodyDataIDL));
}

/* static */
void
JITTimeFunctionBody::InitializeJITFunctionData(
    __in Recycler * recycler,
    __in Js::FunctionBody *functionBody,
    __out FunctionBodyDataIDL * jitBody)
{
    Assert(functionBody != nullptr);

    // const table
    jitBody->constCount = functionBody->GetConstantCount();
    if (functionBody->GetConstantCount() > 0)
    {
        jitBody->constTable = (intptr_t *)functionBody->GetConstTable();
        if (!functionBody->GetIsAsmJsFunction())
        {
            jitBody->constTableContent = RecyclerNewStructZ(recycler, ConstTableContentIDL);
            jitBody->constTableContent->count = functionBody->GetConstantCount();
            jitBody->constTableContent->content = RecyclerNewArrayZ(recycler, RecyclableObjectIDL*, functionBody->GetConstantCount());

            for (Js::RegSlot reg = Js::FunctionBody::FirstRegSlot; reg < functionBody->GetConstantCount(); ++reg)
            {
                Js::Var varConst = functionBody->GetConstantVar(reg);
                Assert(varConst != nullptr);
                if (Js::TaggedInt::Is(varConst) ||
                    varConst == (Js::Var)&Js::NullFrameDisplay ||
                    varConst == (Js::Var)&Js::StrictNullFrameDisplay)
                {
                    // don't need TypeId for these
                }
                else
                {
                    if (Js::TaggedNumber::Is(varConst))
                    {
                        // the typeid should be TypeIds_Number, determine this directly from const table
                    }
                    else
                    {
                        // irbuilder relies on this assertion
                        Assert(!Js::JavascriptString::Is(varConst)
                            || VirtualTableInfo<Js::LiteralString>::HasVirtualTable(varConst)
                            || VirtualTableInfo<Js::PropertyString>::HasVirtualTable(varConst)
                            || VirtualTableInfo<Js::SingleCharString>::HasVirtualTable(varConst));

                        jitBody->constTableContent->content[reg - Js::FunctionBody::FirstRegSlot] = (RecyclableObjectIDL*)varConst;
                    }
                }
            }
        }
    }

    Js::SmallSpanSequence * statementMap = functionBody->GetStatementMapSpanSequence();

    // REVIEW: OOP JIT, is it possible for this to not match with isJitInDebugMode?
    if (functionBody->IsInDebugMode())
    {
        Assert(!statementMap);

        jitBody->byteCodeLength = functionBody->GetOriginalByteCode()->GetLength();
        jitBody->byteCodeBuffer = functionBody->GetOriginalByteCode()->GetBuffer();

        auto fullStatementMaps = functionBody->GetStatementMaps();
        jitBody->fullStatementMapCount = fullStatementMaps->Count();
        jitBody->fullStatementMaps = RecyclerNewArrayZ(recycler, StatementMapIDL, jitBody->fullStatementMapCount);
        fullStatementMaps->Map([jitBody](int index, Js::FunctionBody::StatementMap * map) {

            jitBody->fullStatementMaps[index] = *(StatementMapIDL*)map;

            Assert(jitBody->fullStatementMaps[index].byteCodeSpanBegin == map->byteCodeSpan.Begin());
            Assert(jitBody->fullStatementMaps[index].byteCodeSpanEnd == map->byteCodeSpan.End());
            Assert(jitBody->fullStatementMaps[index].sourceSpanBegin == map->sourceSpan.Begin());
            Assert(jitBody->fullStatementMaps[index].sourceSpanEnd == map->sourceSpan.End());
            Assert((jitBody->fullStatementMaps[index].isSubExpression != FALSE) == map->isSubexpression);
        });

        if (functionBody->GetPropertyIdOnRegSlotsContainer())
        {
            jitBody->propertyIdsForRegSlotsCount = functionBody->GetPropertyIdOnRegSlotsContainer()->length;
            jitBody->propertyIdsForRegSlots = functionBody->GetPropertyIdOnRegSlotsContainer()->propertyIdsForRegSlots;
        }
    }
    else
    {
        Assert(statementMap);

        jitBody->byteCodeLength = functionBody->GetByteCode()->GetLength();
        jitBody->byteCodeBuffer = functionBody->GetByteCode()->GetBuffer();

        jitBody->statementMap = RecyclerNewStructZ(recycler, SmallSpanSequenceIDL);
        jitBody->statementMap->baseValue = statementMap->baseValue;

        if (statementMap->pActualOffsetList)
        {
            jitBody->statementMap->actualOffsetLength = statementMap->pActualOffsetList->Count();
            jitBody->statementMap->actualOffsetList = statementMap->pActualOffsetList->GetBuffer();
        }

        if (statementMap->pStatementBuffer)
        {
            jitBody->statementMap->statementLength = statementMap->pStatementBuffer->Count();
            jitBody->statementMap->statementBuffer = statementMap->pStatementBuffer->GetBuffer();
        }

    }

    jitBody->inlineCacheCount = functionBody->GetInlineCacheCount();
    if (functionBody->GetInlineCacheCount() > 0)
    {
        jitBody->cacheIdToPropertyIdMap = functionBody->GetCacheIdToPropertyIdMap();
    }

    jitBody->inlineCaches = reinterpret_cast<intptr_t*>(functionBody->GetInlineCaches());

    // body data
    jitBody->functionBodyAddr = (intptr_t)functionBody;

    jitBody->funcNumber = functionBody->GetFunctionNumber();
    jitBody->sourceContextId = functionBody->GetSourceContextId();
    jitBody->nestedCount = functionBody->GetNestedCount();
    if (functionBody->GetNestedCount() > 0)
    {
        jitBody->nestedFuncArrayAddr = (intptr_t)functionBody->GetNestedFuncArray();
    }
    jitBody->scopeSlotArraySize = functionBody->scopeSlotArraySize;
    jitBody->paramScopeSlotArraySize = functionBody->paramScopeSlotArraySize;
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
        jitBody->loopHeaders = RecyclerNewArray(recycler, JITLoopHeaderIDL, functionBody->GetLoopCount());
        for (uint i = 0; i < functionBody->GetLoopCount(); ++i)
        {
            jitBody->loopHeaders[i].startOffset = functionBody->GetLoopHeader(i)->startOffset;
            jitBody->loopHeaders[i].endOffset = functionBody->GetLoopHeader(i)->endOffset;
            jitBody->loopHeaders[i].isNested = functionBody->GetLoopHeader(i)->isNested;
            jitBody->loopHeaders[i].isInTry = functionBody->GetLoopHeader(i)->isInTry;
            jitBody->loopHeaders[i].interpretCount = functionBody->GetLoopInterpretCount(functionBody->GetLoopHeader(i));
        }
    }

    jitBody->localFrameDisplayReg = functionBody->GetLocalFrameDisplayRegister();
    jitBody->localClosureReg = functionBody->GetLocalClosureRegister();
    jitBody->envReg = functionBody->GetEnvRegister();
    jitBody->firstTmpReg = functionBody->GetFirstTmpReg();
    jitBody->varCount = functionBody->GetVarCount();
    jitBody->innerScopeCount = functionBody->GetInnerScopeCount();
    if (functionBody->GetInnerScopeCount() > 0)
    {
        jitBody->firstInnerScopeReg = functionBody->GetFirstInnerScopeRegister();
    }
    jitBody->envDepth = functionBody->GetEnvDepth();
    jitBody->profiledIterations = functionBody->GetProfiledIterations();
    jitBody->profiledCallSiteCount = functionBody->GetProfiledCallSiteCount();
    jitBody->inParamCount = functionBody->GetInParamsCount();
    jitBody->thisRegisterForEventHandler = functionBody->GetThisRegisterForEventHandler();
    jitBody->funcExprScopeRegister = functionBody->GetFuncExprScopeRegister();
    jitBody->recursiveCallSiteCount = functionBody->GetNumberOfRecursiveCallSites();
    jitBody->argUsedForBranch = functionBody->m_argUsedForBranch;

    jitBody->flags = functionBody->GetFlags();

    jitBody->doBackendArgumentsOptimization = functionBody->GetDoBackendArgumentsOptimization();
    jitBody->isLibraryCode = functionBody->GetUtf8SourceInfo()->GetIsLibraryCode();
    jitBody->isAsmJsMode = functionBody->GetIsAsmjsMode();
    jitBody->isStrictMode = functionBody->GetIsStrictMode();
    jitBody->isEval = functionBody->IsEval();
    jitBody->isGlobalFunc = functionBody->GetIsGlobalFunc();
    jitBody->isInlineApplyDisabled = functionBody->IsInlineApplyDisabled();
    jitBody->doJITLoopBody = functionBody->DoJITLoopBody();
    jitBody->hasScopeObject = functionBody->HasScopeObject();
    jitBody->hasImplicitArgIns = functionBody->GetHasImplicitArgIns();
    jitBody->hasCachedScopePropIds = functionBody->HasCachedScopePropIds();
    jitBody->inlineCachesOnFunctionObject = functionBody->GetInlineCachesOnFunctionObject();
    jitBody->doInterruptProbe = functionBody->GetScriptContext()->GetThreadContext()->DoInterruptProbe(functionBody);
    jitBody->disableInlineSpread = functionBody->IsInlineSpreadDisabled();
    jitBody->hasNestedLoop = functionBody->GetHasNestedLoop();
    jitBody->isParamAndBodyScopeMerged = functionBody->IsParamAndBodyScopeMerged();
    jitBody->paramClosureReg = functionBody->GetParamClosureRegister();
    jitBody->usesArgumentsObject = functionBody->GetUsesArgumentsObject();
    
    //CompileAssert(sizeof(PropertyIdArrayIDL) == sizeof(Js::PropertyIdArray));
    jitBody->formalsPropIdArray = (PropertyIdArrayIDL*)functionBody->GetFormalsPropIdArray(false);
    jitBody->formalsPropIdArrayAddr = (intptr_t)functionBody->GetFormalsPropIdArray(false);

    if (functionBody->HasDynamicProfileInfo() && Js::DynamicProfileInfo::HasCallSiteInfo(functionBody))
    {
        jitBody->hasNonBuiltInCallee = functionBody->HasNonBuiltInCallee();
    }

    if (functionBody->GetAuxiliaryData() != nullptr)
    {
        jitBody->auxDataCount = functionBody->GetAuxiliaryData()->GetLength();
        jitBody->auxData = functionBody->GetAuxiliaryData()->GetBuffer();
        jitBody->auxDataBufferAddr = (intptr_t)functionBody->GetAuxiliaryData()->GetBuffer();
    }

    if (functionBody->GetAuxiliaryContextData() != nullptr)
    {
        jitBody->auxContextDataCount = functionBody->GetAuxiliaryContextData()->GetLength();
        jitBody->auxContextData = functionBody->GetAuxiliaryContextData()->GetBuffer();
    }

    jitBody->scriptIdAddr = (intptr_t)functionBody->GetAddressOfScriptId();
    jitBody->flagsAddr = (intptr_t)functionBody->GetAddressOfFlags();
    jitBody->probeCountAddr = (intptr_t)&functionBody->GetSourceInfo()->m_probeCount;
    jitBody->regAllocLoadCountAddr = (intptr_t)&functionBody->regAllocLoadCount;
    jitBody->regAllocStoreCountAddr = (intptr_t)&functionBody->regAllocStoreCount;
    jitBody->callCountStatsAddr = (intptr_t)&functionBody->callCountStats;

    jitBody->referencedPropertyIdCount = functionBody->GetReferencedPropertyIdCount();
    jitBody->referencedPropertyIdMap = functionBody->GetReferencedPropertyIdMap();
    jitBody->hasFinally = functionBody->GetHasFinally();

    jitBody->nameLength = functionBody->GetDisplayNameLength() + 1; // +1 for null terminator
    jitBody->displayName = (wchar_t *)functionBody->GetDisplayName();
    jitBody->objectLiteralTypesAddr = (intptr_t)functionBody->GetObjectLiteralTypes();
    jitBody->literalRegexCount = functionBody->GetLiteralRegexCount();
    jitBody->literalRegexes = (intptr_t*)functionBody->GetLiteralRegexes();

    if (functionBody->GetIsAsmJsFunction())
    {
        jitBody->asmJsData = RecyclerNew(recycler, AsmJsDataIDL);
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
}

intptr_t
JITTimeFunctionBody::GetAddr() const
{
    return m_bodyData.functionBodyAddr;
}

uint
JITTimeFunctionBody::GetFunctionNumber() const
{
    return m_bodyData.funcNumber;
}

uint
JITTimeFunctionBody::GetSourceContextId() const
{
    return m_bodyData.sourceContextId;
}

uint
JITTimeFunctionBody::GetNestedCount() const
{
    return m_bodyData.nestedCount;
}

uint
JITTimeFunctionBody::GetScopeSlotArraySize() const
{
    return m_bodyData.scopeSlotArraySize;
}

uint
JITTimeFunctionBody::GetParamScopeSlotArraySize() const
{
    return m_bodyData.paramScopeSlotArraySize;
}

uint
JITTimeFunctionBody::GetByteCodeCount() const
{
    return m_bodyData.byteCodeCount;
}

uint
JITTimeFunctionBody::GetByteCodeInLoopCount() const
{
    return m_bodyData.byteCodeInLoopCount;
}

uint
JITTimeFunctionBody::GetNonLoadByteCodeCount() const
{
    return m_bodyData.nonLoadByteCodeCount;
}

uint
JITTimeFunctionBody::GetLoopCount() const
{
    return m_bodyData.loopCount;
}

bool
JITTimeFunctionBody::HasLoops() const
{
    return GetLoopCount() != 0;
}

uint
JITTimeFunctionBody::GetByteCodeLength() const
{
    return m_bodyData.byteCodeLength;
}

uint
JITTimeFunctionBody::GetInnerScopeCount() const
{
    return m_bodyData.innerScopeCount;
}

uint
JITTimeFunctionBody::GetInlineCacheCount() const
{
    return m_bodyData.inlineCacheCount;
}

uint
JITTimeFunctionBody::GetRecursiveCallSiteCount() const
{
    return m_bodyData.recursiveCallSiteCount;
}

Js::RegSlot
JITTimeFunctionBody::GetLocalFrameDisplayReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.localFrameDisplayReg);
}

Js::RegSlot
JITTimeFunctionBody::GetLocalClosureReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.localClosureReg);
}

Js::RegSlot
JITTimeFunctionBody::GetEnvReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.envReg);
}

Js::RegSlot
JITTimeFunctionBody::GetFirstTmpReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.firstTmpReg);
}

Js::RegSlot
JITTimeFunctionBody::GetFirstInnerScopeReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.firstInnerScopeReg);
}

Js::RegSlot
JITTimeFunctionBody::GetVarCount() const
{
    return static_cast<Js::RegSlot>(m_bodyData.varCount);
}

Js::RegSlot
JITTimeFunctionBody::GetConstCount() const
{
    return static_cast<Js::RegSlot>(m_bodyData.constCount);
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
    return static_cast<Js::RegSlot>(m_bodyData.funcExprScopeRegister);
}

Js::RegSlot
JITTimeFunctionBody::GetThisRegForEventHandler() const
{
    return static_cast<Js::RegSlot>(m_bodyData.thisRegisterForEventHandler);
}

Js::RegSlot
JITTimeFunctionBody::GetParamClosureReg() const
{
    return static_cast<Js::RegSlot>(m_bodyData.paramClosureReg);
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
    Assert(m_bodyData.cacheIdToPropertyIdMap);
    Assert(cacheId < GetInlineCacheCount());
    return static_cast<Js::PropertyId>(m_bodyData.cacheIdToPropertyIdMap[cacheId]);
}

Js::PropertyId
JITTimeFunctionBody::GetReferencedPropertyId(uint index) const
{
    if (index < (uint)TotalNumberOfBuiltInProperties)
    {
        return index;
    }
    uint mapIndex = index - TotalNumberOfBuiltInProperties;

    Assert(m_bodyData.referencedPropertyIdMap != nullptr);
    Assert(mapIndex < m_bodyData.referencedPropertyIdCount);

    return m_bodyData.referencedPropertyIdMap[mapIndex];
}

uint16
JITTimeFunctionBody::GetArgUsedForBranch() const
{
    return m_bodyData.argUsedForBranch;
}

uint16
JITTimeFunctionBody::GetEnvDepth() const
{
    return m_bodyData.envDepth;
}

uint16
JITTimeFunctionBody::GetProfiledIterations() const
{
    return m_bodyData.profiledIterations;
}

Js::ProfileId
JITTimeFunctionBody::GetProfiledCallSiteCount() const
{
    return static_cast<Js::ProfileId>(m_bodyData.profiledCallSiteCount);
}

Js::ArgSlot
JITTimeFunctionBody::GetInParamsCount() const
{
    return static_cast<Js::ArgSlot>(m_bodyData.inParamCount);
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
JITTimeFunctionBody::HasThis() const
{
    return Js::FunctionBody::GetHasThis(GetFlags());
}

bool
JITTimeFunctionBody::HasFinally() const
{
    return m_bodyData.hasFinally != FALSE;
}

bool
JITTimeFunctionBody::HasOrParentHasArguments() const
{
    return Js::FunctionBody::GetHasOrParentHasArguments(GetFlags());
}

bool
JITTimeFunctionBody::DoBackendArgumentsOptimization() const
{
    return m_bodyData.doBackendArgumentsOptimization != FALSE;
}

bool
JITTimeFunctionBody::IsLibraryCode() const
{
    return m_bodyData.isLibraryCode != FALSE;
}

bool
JITTimeFunctionBody::IsAsmJsMode() const
{
    return m_bodyData.isAsmJsMode != FALSE;
}

bool
JITTimeFunctionBody::IsStrictMode() const
{
    return m_bodyData.isStrictMode != FALSE;
}

bool
JITTimeFunctionBody::IsEval() const
{
    return m_bodyData.isEval != FALSE;
}

bool
JITTimeFunctionBody::HasScopeObject() const
{
    return m_bodyData.hasScopeObject != FALSE;
}

bool
JITTimeFunctionBody::HasNestedLoop() const
{
    return m_bodyData.hasNestedLoop != FALSE;
}

bool
JITTimeFunctionBody::UsesArgumentsObject() const
{
    return m_bodyData.usesArgumentsObject != FALSE;
}

bool
JITTimeFunctionBody::IsParamAndBodyScopeMerged() const
{
    return m_bodyData.isParamAndBodyScopeMerged != FALSE;
}

bool
JITTimeFunctionBody::IsCoroutine() const
{
    return Js::FunctionInfo::IsCoroutine(GetAttributes());
}

bool
JITTimeFunctionBody::IsGenerator() const
{
    return Js::FunctionInfo::IsGenerator(GetAttributes());
}

bool
JITTimeFunctionBody::IsLambda() const
{
    return Js::FunctionInfo::IsLambda(GetAttributes());
}

bool
JITTimeFunctionBody::HasImplicitArgIns() const
{
    return m_bodyData.hasImplicitArgIns != FALSE;
}

bool
JITTimeFunctionBody::HasCachedScopePropIds() const
{
    return m_bodyData.hasCachedScopePropIds != FALSE;
}

bool
JITTimeFunctionBody::HasInlineCachesOnFunctionObject() const
{
    return m_bodyData.inlineCachesOnFunctionObject != FALSE;
}

bool
JITTimeFunctionBody::DoInterruptProbe() const
{
    // TODO michhol: this is technically a threadcontext flag,
    // may want to pass all these when initializing thread context
    return m_bodyData.doInterruptProbe != FALSE;
}

bool
JITTimeFunctionBody::HasRestParameter() const
{
    return Js::FunctionBody::GetHasRestParameter(GetFlags());
}

bool
JITTimeFunctionBody::IsGlobalFunc() const
{
    return m_bodyData.isGlobalFunc != FALSE;
}

void
JITTimeFunctionBody::DisableInlineApply() 
{
    m_bodyData.isInlineApplyDisabled = TRUE;
}

bool
JITTimeFunctionBody::IsInlineApplyDisabled() const
{
    return m_bodyData.isInlineApplyDisabled != FALSE;
}

bool
JITTimeFunctionBody::IsNonTempLocalVar(uint32 varIndex) const
{
    return GetFirstNonTempLocalIndex() <= varIndex && varIndex < GetEndNonTempLocalIndex();
}

bool
JITTimeFunctionBody::DoJITLoopBody() const
{
    return m_bodyData.doJITLoopBody != FALSE;
}

void
JITTimeFunctionBody::DisableInlineSpread()
{
    m_bodyData.disableInlineSpread = TRUE;
}

bool
JITTimeFunctionBody::IsInlineSpreadDisabled() const
{
    return m_bodyData.disableInlineSpread != FALSE;
}

bool
JITTimeFunctionBody::HasNonBuiltInCallee() const
{
    return m_bodyData.hasNonBuiltInCallee != FALSE;
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

bool
JITTimeFunctionBody::NeedScopeObjectForArguments(bool hasNonSimpleParams) const
{
    // TODO: OOP JIT, enable assert
    //Assert(HasReferenceableBuiltInArguments());
    // We can avoid creating a scope object with arguments present if:
    bool dontNeedScopeObject =
        // Either we are in strict mode, or have strict mode formal semantics from a non-simple parameter list, and
        (IsStrictMode() || hasNonSimpleParams)
        // Neither of the scopes are objects
        && !HasScopeObject();

    return
        // Regardless of the conditions above, we won't need a scope object if there aren't any formals.
        (GetInParamsCount() > 1 || HasRestParameter())
        && !dontNeedScopeObject;
}

const byte *
JITTimeFunctionBody::GetByteCodeBuffer() const
{
    return m_bodyData.byteCodeBuffer;
}

StatementMapIDL *
JITTimeFunctionBody::GetFullStatementMap() const
{
    return m_bodyData.fullStatementMaps;
}

uint
JITTimeFunctionBody::GetFullStatementMapCount() const
{
    return m_bodyData.fullStatementMapCount;
}

intptr_t
JITTimeFunctionBody::GetScriptIdAddr() const
{
    return m_bodyData.scriptIdAddr;
}

intptr_t
JITTimeFunctionBody::GetProbeCountAddr() const
{
    return m_bodyData.probeCountAddr;
}

intptr_t
JITTimeFunctionBody::GetFlagsAddr() const
{
    return m_bodyData.flagsAddr;
}

intptr_t
JITTimeFunctionBody::GetRegAllocLoadCountAddr() const
{
    return m_bodyData.regAllocLoadCountAddr;
}

intptr_t
JITTimeFunctionBody::GetFormalsPropIdArrayAddr() const
{
    return m_bodyData.formalsPropIdArrayAddr;
}

intptr_t
JITTimeFunctionBody::GetRegAllocStoreCountAddr() const
{
    return m_bodyData.regAllocStoreCountAddr;
}

intptr_t
JITTimeFunctionBody::GetCallCountStatsAddr() const
{
    return m_bodyData.callCountStatsAddr;
}

intptr_t
JITTimeFunctionBody::GetObjectLiteralTypeRef(uint index) const
{
    Assert(m_bodyData.objectLiteralTypesAddr != 0);
    return m_bodyData.objectLiteralTypesAddr + index * MachPtr;
}

intptr_t
JITTimeFunctionBody::GetConstantVar(Js::RegSlot location) const
{
    Assert(m_bodyData.constTable != nullptr);
    Assert(location < GetConstCount());
    Assert(location != 0);

    return static_cast<intptr_t>(m_bodyData.constTable[location - Js::FunctionBody::FirstRegSlot]);
}

intptr_t
JITTimeFunctionBody::GetInlineCache(uint index) const
{
    Assert(m_bodyData.inlineCaches != nullptr);
    Assert(index < GetInlineCacheCount());
#if 0 // TODO: michhol OOP JIT, add these asserts
    Assert(this->m_inlineCacheTypes[index] == InlineCacheTypeNone ||
        this->m_inlineCacheTypes[index] == InlineCacheTypeInlineCache);
    this->m_inlineCacheTypes[index] = InlineCacheTypeInlineCache;
#endif
    return static_cast<intptr_t>(m_bodyData.inlineCaches[index]);
}

intptr_t
JITTimeFunctionBody::GetIsInstInlineCache(uint index) const
{
    Assert(m_bodyData.inlineCaches != nullptr);
    Assert(index < m_bodyData.isInstInlineCacheCount);
    index += GetInlineCacheCount();
#if 0 // TODO: michhol OOP JIT, add these asserts
    Assert(this->m_inlineCacheTypes[index] == InlineCacheTypeNone ||
        this->m_inlineCacheTypes[index] == InlineCacheTypeIsInst);
    this->m_inlineCacheTypes[index] = InlineCacheTypeIsInst;
#endif
    return static_cast<intptr_t>(m_bodyData.inlineCaches[index]);
}

Js::TypeId
JITTimeFunctionBody::GetConstantType(Js::RegSlot location) const
{
    Assert(m_bodyData.constTable != nullptr);
    Assert(m_bodyData.constTableContent != nullptr);
    Assert(location < GetConstCount());
    Assert(location != 0);
    auto obj = m_bodyData.constTableContent->content[location - Js::FunctionBody::FirstRegSlot];

    if (obj == nullptr)
    {
        if (Js::TaggedNumber::Is((Js::Var)GetConstantVar(location)))
        {
            // tagged float
            return Js::TypeId::TypeIds_Number;
        }
        else
        {
            return Js::TypeId::TypeIds_Limit;
        }
    }

    return static_cast<Js::TypeId>(*(obj->typeId));
}

intptr_t
JITTimeFunctionBody::GetLiteralRegexAddr(uint index) const
{
    Assert(index < m_bodyData.literalRegexCount);

    return m_bodyData.literalRegexes[index];
}

void *
JITTimeFunctionBody::GetConstTable() const
{
    return m_bodyData.constTable;
}

bool
JITTimeFunctionBody::IsConstRegPropertyString(Js::RegSlot reg, ScriptContextInfo * context) const
{
    RecyclableObjectIDL * content = m_bodyData.constTableContent->content[reg - Js::FunctionBody::FirstRegSlot];
    if (content != nullptr && content->vtbl == context->GetVTableAddress(VtablePropertyString))
    {
        return true;
    }
    return false;
}

intptr_t
JITTimeFunctionBody::GetRootObject() const
{
    Assert(m_bodyData.constTable != nullptr);
    return m_bodyData.constTable[Js::FunctionBody::RootObjectRegSlot - Js::FunctionBody::FirstRegSlot];
}

intptr_t
JITTimeFunctionBody::GetNestedFuncRef(uint index) const
{
    Assert(index < GetNestedCount());
    intptr_t baseAddr = m_bodyData.nestedFuncArrayAddr;
    return baseAddr + (index * sizeof(void*));
}

intptr_t
JITTimeFunctionBody::GetLoopHeaderAddr(uint loopNum) const
{
    Assert(loopNum < GetLoopCount());
    intptr_t baseAddr = m_bodyData.loopHeaderArrayAddr;
    return baseAddr + (loopNum * sizeof(Js::LoopHeader));
}

const JITLoopHeaderIDL *
JITTimeFunctionBody::GetLoopHeaderData(uint loopNum) const
{
    Assert(loopNum < GetLoopCount());
    return &m_bodyData.loopHeaders[loopNum];
}

const AsmJsJITInfo *
JITTimeFunctionBody::GetAsmJsInfo() const
{
    return reinterpret_cast<const AsmJsJITInfo *>(m_bodyData.asmJsData);
}

JITTimeProfileInfo *
JITTimeFunctionBody::GetProfileInfo() const
{
    return reinterpret_cast<JITTimeProfileInfo *>(m_bodyData.profileData);
}

const JITTimeProfileInfo *
JITTimeFunctionBody::GetReadOnlyProfileInfo() const
{
    return reinterpret_cast<const JITTimeProfileInfo *>(m_bodyData.profileData);
}

bool
JITTimeFunctionBody::HasProfileInfo() const
{
    return m_bodyData.profileData != nullptr;
}

bool
JITTimeFunctionBody::HasPropIdToFormalsMap() const
{
    return m_bodyData.propertyIdsForRegSlotsCount > 0 && GetFormalsPropIdArray() != nullptr;
}

bool
JITTimeFunctionBody::IsRegSlotFormal(Js::RegSlot reg) const
{
    Assert(reg < m_bodyData.propertyIdsForRegSlotsCount);
    Js::PropertyId propId = (Js::PropertyId)m_bodyData.propertyIdsForRegSlots[reg];
    Js::PropertyIdArray * formalProps = GetFormalsPropIdArray();
    for (uint32 i = 0; i < formalProps->count; i++)
    {
        if (formalProps->elements[i] == propId)
        {
            return true;
        }
    }
    return false;
}

/* static */
bool
JITTimeFunctionBody::LoopContains(const JITLoopHeaderIDL * loop1, const JITLoopHeaderIDL * loop2)
{
    return (loop1->startOffset <= loop2->startOffset && loop2->endOffset <= loop1->endOffset);
}

Js::FunctionBody::FunctionBodyFlags
JITTimeFunctionBody::GetFlags() const
{
    return static_cast<Js::FunctionBody::FunctionBodyFlags>(m_bodyData.flags);
}

Js::FunctionInfo::Attributes
JITTimeFunctionBody::GetAttributes() const
{
    return static_cast<Js::FunctionInfo::Attributes>(m_bodyData.attributes);
}

intptr_t
JITTimeFunctionBody::GetAuxDataAddr(uint offset) const
{
    return m_bodyData.auxDataBufferAddr + offset;
}

void *
JITTimeFunctionBody::ReadFromAuxData(uint offset) const
{
    return (void *)(m_bodyData.auxData + offset);
}

void *
JITTimeFunctionBody::ReadFromAuxContextData(uint offset) const
{
    return (void *)(m_bodyData.auxContextData + offset);
}

const Js::PropertyIdArray *
JITTimeFunctionBody::ReadPropertyIdArrayFromAuxData(uint offset) const
{
    Js::PropertyIdArray * auxArray = (Js::PropertyIdArray *)(m_bodyData.auxData + offset);
    Assert(offset + auxArray->GetDataSize() <= m_bodyData.auxDataCount);
    return auxArray;
}

Js::PropertyIdArray *
JITTimeFunctionBody::GetFormalsPropIdArray() const
{
    return  (Js::PropertyIdArray *)m_bodyData.formalsPropIdArray;
}

bool
JITTimeFunctionBody::InitializeStatementMap(Js::SmallSpanSequence * statementMap, ArenaAllocator* alloc) const
{
    if (!m_bodyData.statementMap)
    {
        return false;
    }
    const uint statementsLength = m_bodyData.statementMap->statementLength;
    const uint offsetsLength = m_bodyData.statementMap->actualOffsetLength;

    statementMap->baseValue = m_bodyData.statementMap->baseValue;

    // TODO: (leish OOP JIT) using arena to prevent memory leak, fix to really implement GrowingUint32ArenaArray::Create()
    // or find other way to reuse like michhol's comments
    typedef JsUtil::GrowingArray<uint32, ArenaAllocator> GrowingUint32ArenaArray;

    if (statementsLength > 0)
    {
        // TODO: (michhol OOP JIT) should be able to directly use statementMap.statementBuffer        
        statementMap->pStatementBuffer = (JsUtil::GrowingUint32HeapArray*)Anew(alloc, GrowingUint32ArenaArray, alloc, statementsLength);
        statementMap->pStatementBuffer->SetCount(statementsLength);
        js_memcpy_s(
            statementMap->pStatementBuffer->GetBuffer(),
            statementMap->pStatementBuffer->Count() * sizeof(uint32),
            m_bodyData.statementMap->statementBuffer,
            statementsLength * sizeof(uint32));
    }

    if (offsetsLength > 0)
    {
        statementMap->pActualOffsetList = (JsUtil::GrowingUint32HeapArray*)Anew(alloc, GrowingUint32ArenaArray, alloc, offsetsLength);
        statementMap->pActualOffsetList->SetCount(offsetsLength);
        js_memcpy_s(
            statementMap->pActualOffsetList->GetBuffer(),
            statementMap->pActualOffsetList->Count() * sizeof(uint32),
            m_bodyData.statementMap->actualOffsetList,
            offsetsLength * sizeof(uint32));
    }
    return true;
}

wchar_t*
JITTimeFunctionBody::GetDisplayName() const
{
    return m_bodyData.displayName;
}
