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
ThreadContextInfo::GetNullFrameDisplayAddress() const
{
    return m_threadContextData->nullFrameDisplayAddress;
}

intptr_t
ThreadContextInfo::GetStrictNullFrameDisplayAddress() const
{
    return m_threadContextData->strictNullFrameDisplayAddress;
}
