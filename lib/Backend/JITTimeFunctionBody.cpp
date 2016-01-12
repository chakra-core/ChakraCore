//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeFunctionBody::JITTimeFunctionBody(const FunctionBodyJITData * const bodyData) :
    m_bodyData(bodyData)
{
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
JITTimeFunctionBody::GetLengthInBytes() const
{
    return m_bodyData->cbLength;
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
JITTimeFunctionBody::IsGenerator() const
{
    return Js::FunctionInfo::IsGenerator(GetAttributes());
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
