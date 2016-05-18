//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

ScriptContextInfo::ScriptContextInfo(ScriptContextData * contextData) :
    m_contextData(*contextData),
    m_isPRNGSeeded(false),
    m_activeJITCount(0)
{
}

intptr_t
ScriptContextInfo::GetNullAddr() const
{
    return m_contextData.nullAddr;
}

intptr_t
ScriptContextInfo::GetUndefinedAddr() const
{
    return m_contextData.undefinedAddr;
}

intptr_t
ScriptContextInfo::GetTrueAddr() const
{
    return m_contextData.trueAddr;
}

intptr_t
ScriptContextInfo::GetFalseAddr() const
{
    return m_contextData.falseAddr;
}

intptr_t
ScriptContextInfo::GetTrueOrFalseAddr(bool isTrue) const
{
    return isTrue ? GetTrueAddr() : GetFalseAddr();
}

intptr_t
ScriptContextInfo::GetUndeclBlockVarAddr() const
{
    return m_contextData.undeclBlockVarAddr;
}

intptr_t
ScriptContextInfo::GetEmptyStringAddr() const
{
    return m_contextData.emptyStringAddr;
}

intptr_t
ScriptContextInfo::GetNegativeZeroAddr() const
{
    return m_contextData.negativeZeroAddr;
}

intptr_t
ScriptContextInfo::GetNumberTypeStaticAddr() const
{
    return m_contextData.numberTypeStaticAddr;
}

intptr_t
ScriptContextInfo::GetStringTypeStaticAddr() const
{
    return m_contextData.stringTypeStaticAddr;
}

intptr_t
ScriptContextInfo::GetObjectTypeAddr() const
{
    return m_contextData.objectTypeAddr;
}

intptr_t
ScriptContextInfo::GetObjectHeaderInlinedTypeAddr() const
{
    return m_contextData.objectHeaderInlinedTypeAddr;
}

intptr_t
ScriptContextInfo::GetRegexTypeAddr() const
{
    return m_contextData.regexTypeAddr;
}

intptr_t
ScriptContextInfo::GetArrayTypeAddr() const
{
    return m_contextData.arrayTypeAddr;
}

intptr_t
ScriptContextInfo::GetNativeIntArrayTypeAddr() const
{
    return m_contextData.nativeIntArrayTypeAddr;

}

intptr_t
ScriptContextInfo::GetNativeFloatArrayTypeAddr() const
{
    return m_contextData.nativeFloatArrayTypeAddr;

}

intptr_t
ScriptContextInfo::GetArrayConstructorAddr() const
{
    return m_contextData.arrayConstructorAddr;
}

intptr_t
ScriptContextInfo::GetCharStringCacheAddr() const
{
    return m_contextData.charStringCacheAddr;
}

intptr_t
ScriptContextInfo::GetSideEffectsAddr() const
{
    return m_contextData.sideEffectsAddr;
}

intptr_t
ScriptContextInfo::GetArraySetElementFastPathVtableAddr() const
{
    return m_contextData.arraySetElementFastPathVtableAddr;
}

intptr_t
ScriptContextInfo::GetIntArraySetElementFastPathVtableAddr() const
{
    return m_contextData.intArraySetElementFastPathVtableAddr;
}

intptr_t
ScriptContextInfo::GetFloatArraySetElementFastPathVtableAddr() const
{
    return m_contextData.floatArraySetElementFastPathVtableAddr;
}

intptr_t
ScriptContextInfo::GetLibraryAddr() const
{
    return m_contextData.libraryAddr;
}

intptr_t
ScriptContextInfo::GetNumberAllocatorAddr() const
{
    return m_contextData.numberAllocatorAddr;
}

intptr_t
ScriptContextInfo::GetRecyclerAddr() const
{
    return m_contextData.recyclerAddr;
}

intptr_t
ScriptContextInfo::GetAddr() const
{
    return m_contextData.scriptContextAddr;
}

intptr_t
ScriptContextInfo::GetVTableAddress(VTableValue vtableType) const
{
    Assert(vtableType < VTableValue::Count);
    return m_contextData.vtableAddresses[vtableType];
}

bool
ScriptContextInfo::IsRecyclerVerifyEnabled() const
{
    return m_contextData.isRecyclerVerifyEnabled != FALSE;
}

uint
ScriptContextInfo::GetRecyclerVerifyPad() const
{
    return m_contextData.recyclerVerifyPad;
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
