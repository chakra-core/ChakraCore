//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

ScriptContextInfo::ScriptContextInfo(ScriptContextData * contextData)
    : m_contextData(contextData)
{
}

intptr_t
ScriptContextInfo::GetNullAddr() const
{
    return m_contextData->nullAddr;
}

intptr_t
ScriptContextInfo::GetUndefinedAddr() const
{
    return m_contextData->undefinedAddr;
}

intptr_t
ScriptContextInfo::GetTrueAddr() const
{
    return m_contextData->trueAddr;
}

intptr_t
ScriptContextInfo::GetFalseAddr() const
{
    return m_contextData->falseAddr;
}

intptr_t
ScriptContextInfo::GetUndeclBlockVarAddr() const
{
    return m_contextData->undeclBlockVarAddr;
}

intptr_t
ScriptContextInfo::GetAddr() const
{
    return m_contextData->scriptContextAddr;
}
