//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

ThreadContextInfo::ThreadContextInfo(ThreadContextData * data) :
    m_threadContextData(data)
{
}

intptr_t
ThreadContextInfo::GetNullFrameDisplayAddr() const
{
    return static_cast<intptr_t>(m_threadContextData->nullFrameDisplayAddr);
}

intptr_t
ThreadContextInfo::GetStrictNullFrameDisplayAddr() const
{
    return static_cast<intptr_t>(m_threadContextData->strictNullFrameDisplayAddr);
}

intptr_t
ThreadContextInfo::GetThreadStackLimitAddr() const
{
    return static_cast<intptr_t>(m_threadContextData->threadStackLimitAddr);
}

size_t
ThreadContextInfo::GetScriptStackLimit() const
{
    return static_cast<size_t>(m_threadContextData->scriptStackLimit);
}

bool
ThreadContextInfo::IsThreadBound() const
{
    return m_threadContextData->isThreadBound != FALSE;
}
