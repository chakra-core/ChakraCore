//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

ServerScriptContext::ServerScriptContext(ScriptContextDataIDL * contextData) :
    m_contextData(*contextData),
    m_isPRNGSeeded(false),
    m_activeJITCount(0)
{
    m_domFastPathHelperMap = HeapNew(JITDOMFastPathHelperMap, &HeapAllocator::Instance, 17);
}

ServerScriptContext::~ServerScriptContext()
{
    HeapDelete(m_domFastPathHelperMap);
}

intptr_t
ServerScriptContext::GetNullAddr() const
{
    return m_contextData.nullAddr;
}

intptr_t
ServerScriptContext::GetUndefinedAddr() const
{
    return m_contextData.undefinedAddr;
}

intptr_t
ServerScriptContext::GetTrueAddr() const
{
    return m_contextData.trueAddr;
}

intptr_t
ServerScriptContext::GetFalseAddr() const
{
    return m_contextData.falseAddr;
}

intptr_t
ServerScriptContext::GetUndeclBlockVarAddr() const
{
    return m_contextData.undeclBlockVarAddr;
}

intptr_t
ServerScriptContext::GetEmptyStringAddr() const
{
    return m_contextData.emptyStringAddr;
}

intptr_t
ServerScriptContext::GetNegativeZeroAddr() const
{
    return m_contextData.negativeZeroAddr;
}

intptr_t
ServerScriptContext::GetNumberTypeStaticAddr() const
{
    return m_contextData.numberTypeStaticAddr;
}

intptr_t
ServerScriptContext::GetStringTypeStaticAddr() const
{
    return m_contextData.stringTypeStaticAddr;
}

intptr_t
ServerScriptContext::GetObjectTypeAddr() const
{
    return m_contextData.objectTypeAddr;
}

intptr_t
ServerScriptContext::GetObjectHeaderInlinedTypeAddr() const
{
    return m_contextData.objectHeaderInlinedTypeAddr;
}

intptr_t
ServerScriptContext::GetRegexTypeAddr() const
{
    return m_contextData.regexTypeAddr;
}

intptr_t
ServerScriptContext::GetArrayTypeAddr() const
{
    return m_contextData.arrayTypeAddr;
}

intptr_t
ServerScriptContext::GetNativeIntArrayTypeAddr() const
{
    return m_contextData.nativeIntArrayTypeAddr;
}

intptr_t
ServerScriptContext::GetNativeFloatArrayTypeAddr() const
{
    return m_contextData.nativeFloatArrayTypeAddr;
}

intptr_t
ServerScriptContext::GetArrayConstructorAddr() const
{
    return m_contextData.arrayConstructorAddr;
}

intptr_t
ServerScriptContext::GetCharStringCacheAddr() const
{
    return m_contextData.charStringCacheAddr;
}

intptr_t
ServerScriptContext::GetSideEffectsAddr() const
{
    return m_contextData.sideEffectsAddr;
}

intptr_t
ServerScriptContext::GetArraySetElementFastPathVtableAddr() const
{
    return m_contextData.arraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetIntArraySetElementFastPathVtableAddr() const
{
    return m_contextData.intArraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetFloatArraySetElementFastPathVtableAddr() const
{
    return m_contextData.floatArraySetElementFastPathVtableAddr;
}

intptr_t
ServerScriptContext::GetLibraryAddr() const
{
    return m_contextData.libraryAddr;
}

intptr_t
ServerScriptContext::GetNumberAllocatorAddr() const
{
    return m_contextData.numberAllocatorAddr;
}

intptr_t
ServerScriptContext::GetRecyclerAddr() const
{
    return m_contextData.recyclerAddr;
}

bool
ServerScriptContext::GetRecyclerAllowNativeCodeBumpAllocation() const
{
    return m_contextData.recyclerAllowNativeCodeBumpAllocation != 0;
}

bool
ServerScriptContext::IsSIMDEnabled() const
{
    return m_contextData.isSIMDEnabled != 0;
}

intptr_t
ServerScriptContext::GetBuiltinFunctionsBaseAddr() const
{
    return m_contextData.builtinFunctionsBaseAddr;
}

intptr_t
ServerScriptContext::GetAddr() const
{
    return m_contextData.scriptContextAddr;
}

intptr_t
ServerScriptContext::GetVTableAddress(VTableValue vtableType) const
{
    Assert(vtableType < VTableValue::Count);
    return m_contextData.vtableAddresses[vtableType];
}

bool
ServerScriptContext::IsRecyclerVerifyEnabled() const
{
    return m_contextData.isRecyclerVerifyEnabled != FALSE;
}

uint
ServerScriptContext::GetRecyclerVerifyPad() const
{
    return m_contextData.recyclerVerifyPad;
}

bool
ServerScriptContext::IsPRNGSeeded() const
{
    return m_isPRNGSeeded;
}

void
ServerScriptContext::AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper)
{
    m_domFastPathHelperMap->Add(funcInfoAddr, helper);
}

IR::JnHelperMethod
ServerScriptContext::GetDOMFastPathHelper(intptr_t funcInfoAddr)
{
    IR::JnHelperMethod helper;

    m_domFastPathHelperMap->LockResize();
    bool found = m_domFastPathHelperMap->TryGetValue(funcInfoAddr, &helper);
    m_domFastPathHelperMap->UnlockResize();

    Assert(found);
    return helper;
}

void
ServerScriptContext::BeginJIT()
{
    InterlockedExchangeAdd(&m_activeJITCount, 1);
}

void
ServerScriptContext::EndJIT()
{
    InterlockedExchangeSubtract(&m_activeJITCount, 1);
}

bool
ServerScriptContext::IsJITActive()
{
    return m_activeJITCount != 0;
}
