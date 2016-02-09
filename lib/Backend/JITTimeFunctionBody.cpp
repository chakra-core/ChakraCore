//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeFunctionBody::JITTimeFunctionBody(FunctionBodyJITData * bodyData) :
    m_bodyData(bodyData)
{
    InitializeStatementMap();
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

Js::PropertyId
JITTimeFunctionBody::GetPropertyIdFromCacheId(uint cacheId) const
{
    Assert(m_bodyData->cacheIdToPropertyIdMap);
    Assert(cacheId < GetInlineCacheCount());
    return static_cast<Js::PropertyId>(m_bodyData->cacheIdToPropertyIdMap[cacheId]);
}

uint16
JITTimeFunctionBody::GetEnvDepth() const
{
    return m_bodyData->envDepth;
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
JITTimeFunctionBody::HasScopeObject() const
{
    return m_bodyData->hasScopeObject != FALSE;
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
JITTimeFunctionBody::HasRestParameter() const
{
    return Js::FunctionBody::GetHasRestParameter(GetFlags());
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

Js::TypeId
JITTimeFunctionBody::GetConstantType(Js::RegSlot location) const
{
    Assert(m_bodyData->constTable != nullptr);
    Assert(location < GetConstCount());
    Assert(location != 0);

    return static_cast<Js::TypeId>(m_bodyData->constTypeTable[location - Js::FunctionBody::FirstRegSlot]);
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
