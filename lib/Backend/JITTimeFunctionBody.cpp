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

uint16
JITTimeFunctionBody::GetEnvDepth() const
{
    return m_bodyData->envDepth;
}

bool
JITTimeFunctionBody::DoStackNestedFunc() const
{
    return Js::FunctionBody::DoStackNestedFunc(GetFlags());
}

bool
JITTimeFunctionBody::DoStackClosure() const
{
    return DoStackNestedFunc() && GetNestedCount() != 0 && GetScopeSlotArraySize() != 0 && GetEnvDepth() != (uint16)-1;
}

Js::FunctionBody::FunctionBodyFlags
JITTimeFunctionBody::GetFlags() const
{
    return static_cast<Js::FunctionBody::FunctionBodyFlags>(m_bodyData->flags);
}
