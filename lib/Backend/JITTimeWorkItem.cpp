//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"


JITTimeWorkItem::JITTimeWorkItem(const CodeGenWorkItemJITData * const workItemData) :
    m_workItemData(workItemData), m_jitBody(&workItemData->bodyData)
{
}

CodeGenWorkItemType
JITTimeWorkItem::Type() const
{
    return static_cast<CodeGenWorkItemType>(m_workItemData->type);
}

ExecutionMode
JITTimeWorkItem::GetJitMode() const
{
    return static_cast<ExecutionMode>(m_workItemData->jitMode);
}

WCHAR *
JITTimeWorkItem::GetDisplayName() const
{
    return m_workItemData->displayName;
}

size_t
JITTimeWorkItem::GetDisplayName(_Out_writes_opt_z_(sizeInChars) WCHAR* displayName, _In_ size_t sizeInChars)
{
    WCHAR * name = GetDisplayName();
    size_t nameSizeInChars = wcslen(name) + 1;
    size_t sizeInBytes = nameSizeInChars * sizeof(WCHAR);
    // only do copy if provided buffer is big enough
    if (displayName && sizeInChars >= nameSizeInChars)
    {
        js_memcpy_s(displayName, sizeInChars * sizeof(WCHAR), name, sizeInBytes);
    }
    return nameSizeInChars;
}

uint
JITTimeWorkItem::GetInterpretedCount() const
{
    return m_workItemData->interpretedCount;
}

// loop number if IsLoopBody, otherwise Js::LoopHeader::NoLoop
uint
JITTimeWorkItem::GetLoopNumber() const
{
    Assert(IsLoopBody() || m_workItemData->loopNumber == Js::LoopHeader::NoLoop);
    return m_workItemData->loopNumber;
}

bool
JITTimeWorkItem::IsLoopBody() const
{
    return Type() == JsLoopBodyWorkItemType;
}

bool
JITTimeWorkItem::IsJitInDebugMode() const
{
    // TODO (michhol): flags?
    return Js::Configuration::Global.EnableJitInDebugMode()
        && m_workItemData->isJitInDebugMode;
}

intptr_t
JITTimeWorkItem::GetCallsCountAddress() const
{
    Assert(Type() == JsFunctionType);

    return m_workItemData->readOnlyEPData.callsCountAddress;
}

const JITTimeFunctionBody * const
JITTimeWorkItem::GetJITFunctionBody() const
{
    return &m_jitBody;
}
