//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeWorkItem::JITTimeWorkItem(CodeGenWorkItemIDL * workItemData) :
    m_workItemData(workItemData),
    m_jitBody(workItemData->jitData->bodyData),
    m_fullStatementList(nullptr)
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

    return m_workItemData->jitData->callsCountAddress;
}

intptr_t
JITTimeWorkItem::GetJittedLoopIterationsSinceLastBailoutAddr() const
{
    Assert(IsLoopBody());
    Assert(m_workItemData->jittedLoopIterationsSinceLastBailoutAddr != 0);

    return m_workItemData->jittedLoopIterationsSinceLastBailoutAddr;
}

const JITLoopHeaderIDL *
JITTimeWorkItem::GetLoopHeader() const
{
    return m_jitBody.GetLoopHeaderData(GetLoopNumber());
}

intptr_t
JITTimeWorkItem::GetLoopHeaderAddr() const
{
    return m_jitBody.GetLoopHeaderAddr(GetLoopNumber());
}

void
JITTimeWorkItem::InitializeReader(
    Js::ByteCodeReader * reader,
    Js::StatementReader<Js::FunctionBody::ArenaStatementMapList> * statementReader, ArenaAllocator* alloc)
{
    uint startOffset = IsLoopBody() ? GetLoopHeader()->startOffset : 0;

    if (IsJitInDebugMode())
    {
        // TODO: OOP JIT, directly use the array rather than making a list
        m_fullStatementList = Js::FunctionBody::ArenaStatementMapList::New(alloc);
        CompileAssert(sizeof(StatementMapIDL) == sizeof(Js::FunctionBody::StatementMap));

        StatementMapIDL * fullArr = m_jitBody.GetFullStatementMap();
        for (uint i = 0; i < m_jitBody.GetFullStatementMapCount(); ++i)
        {
            m_fullStatementList->Add((Js::FunctionBody::StatementMap*)&fullArr[i]);
        }
    }
#if DBG
    reader->Create(m_jitBody.GetByteCodeBuffer(), startOffset, m_jitBody.GetByteCodeLength());
    if (!JITManager::GetJITManager()->IsOOPJITEnabled())
    {
        Js::FunctionBody::StatementMapList * runtimeMap = ((Js::FunctionBody*)m_jitBody.GetAddr())->GetStatementMaps();
        Assert(!m_fullStatementList || ((int)m_jitBody.GetFullStatementMapCount() == runtimeMap->Count() && runtimeMap->Count() >= 0));
        for (uint i = 0; i < m_jitBody.GetFullStatementMapCount(); ++i)
        {
            Assert(runtimeMap->Item(i)->byteCodeSpan.begin == m_fullStatementList->Item(i)->byteCodeSpan.begin);
            Assert(runtimeMap->Item(i)->byteCodeSpan.end == m_fullStatementList->Item(i)->byteCodeSpan.end);
            Assert(runtimeMap->Item(i)->sourceSpan.begin == m_fullStatementList->Item(i)->sourceSpan.begin);
            Assert(runtimeMap->Item(i)->sourceSpan.end == m_fullStatementList->Item(i)->sourceSpan.end);
            Assert(runtimeMap->Item(i)->isSubexpression == m_fullStatementList->Item(i)->isSubexpression);
        }
    }
#else
    reader->Create(m_jitBody.GetByteCodeBuffer(), startOffset);
#endif
    bool hasSpanSequenceMap = m_jitBody.InitializeStatementMap(&m_statementMap, alloc);
    Js::SmallSpanSequence * spanSeq = hasSpanSequenceMap ? &m_statementMap : nullptr;
    statementReader->Create(m_jitBody.GetByteCodeBuffer(), startOffset, spanSeq, m_fullStatementList);
}

JITTimeFunctionBody *
JITTimeWorkItem::GetJITFunctionBody()
{
    return &m_jitBody;
}

uint16
JITTimeWorkItem::GetProfiledIterations() const
{
    return m_workItemData->profiledIterations;
}

CodeGenWorkItemIDL *
JITTimeWorkItem::GetWorkItemData()
{
    return m_workItemData;
}

JITTimePolymorphicInlineCacheInfo *
JITTimeWorkItem::GetPolymorphicInlineCacheInfo()
{
    return (JITTimePolymorphicInlineCacheInfo *)m_workItemData->selfInfo;
}

JITTimePolymorphicInlineCacheInfo *
JITTimeWorkItem::GetInlineePolymorphicInlineCacheInfo(intptr_t funcBodyAddr)
{
    for (uint i = 0; i < m_workItemData->inlineeInfoCount; ++i)
    {
        if (m_workItemData->inlineeInfo[i].functionBodyAddr == (void*)funcBodyAddr)
        {
            return (JITTimePolymorphicInlineCacheInfo *)&m_workItemData->inlineeInfo[i];
        }
    }
    return nullptr;
}

void
JITTimeWorkItem::SetJITTimeData(FunctionJITTimeDataIDL * jitData)
{
    m_workItemData->jitData = jitData;
}

FunctionJITTimeInfo *
JITTimeWorkItem::GetJITTimeInfo() const
{
    return reinterpret_cast<FunctionJITTimeInfo *>(m_workItemData->jitData);
}

bool
JITTimeWorkItem::HasSymIdToValueTypeMap() const
{
    return m_workItemData->symIdToValueTypeMap != nullptr;
}

bool
JITTimeWorkItem::TryGetValueType(uint symId, ValueType * valueType) const
{
    Assert(IsLoopBody());
    uint index = symId - m_jitBody.GetConstCount();
    if (symId >= m_jitBody.GetConstCount() && index < m_workItemData->symIdToValueTypeMapCount)
    {
        ValueType type = ((ValueType*)m_workItemData->symIdToValueTypeMap)[index];
        if (type.GetRawData() != 0)
        {
            *valueType = type;
            return true;
        }
    }
    return false;
}
