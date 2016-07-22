//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeBasePch.h"

ScriptContextInfo::ScriptContextInfo() :
    m_isPRNGSeeded(false),
    m_activeJITCount(0)
{
    m_domFastPathHelperMap = HeapNew(JITDOMFastPathHelperMap, &HeapAllocator::Instance, 17);
}

ScriptContextInfo::~ScriptContextInfo()
{
    HeapDelete(m_domFastPathHelperMap);
}

intptr_t
ScriptContextInfo::GetTrueOrFalseAddr(bool isTrue) const
{
    return isTrue ? GetTrueAddr() : GetFalseAddr();
}

bool
ScriptContextInfo::IsPRNGSeeded() const
{
    return m_isPRNGSeeded;
}

void
ScriptContextInfo::BeginJIT()
{
    InterlockedExchangeAdd(&m_activeJITCount, 1);
}

void
ScriptContextInfo::EndJIT()
{
    InterlockedExchangeSubtract(&m_activeJITCount, 1);
}

bool
ScriptContextInfo::IsJITActive()
{
    return m_activeJITCount != 0;
}

void
ScriptContextInfo::AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper)
{
    m_domFastPathHelperMap->Add(funcInfoAddr, helper);
}

IR::JnHelperMethod
ScriptContextInfo::GetDOMFastPathHelper(intptr_t funcInfoAddr)
{
    IR::JnHelperMethod helper;
    
    m_domFastPathHelperMap->LockResize();
    bool found = m_domFastPathHelperMap->TryGetValue(funcInfoAddr, &helper);
    m_domFastPathHelperMap->UnlockResize();

    Assert(found);
    return helper;
}
