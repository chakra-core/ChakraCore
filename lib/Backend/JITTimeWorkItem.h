//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// class for the backend to use to access CodeGenWorkItem properties
class JITTimeWorkItem
{
public:

    JITTimeWorkItem(CodeGenWorkItemIDL * workItemData);

    CodeGenWorkItemType Type() const;
    ExecutionMode GetJitMode() const;
    uint GetLoopNumber() const;

    const JITLoopHeaderIDL * GetLoopHeader() const;
    intptr_t GetLoopHeaderAddr() const;

    bool IsLoopBody() const;
    bool IsJitInDebugMode() const;
    
    intptr_t GetCallsCountAddress() const;
    intptr_t GetJittedLoopIterationsSinceLastBailoutAddr() const;

    void InitializeReader(
        Js::ByteCodeReader * reader,
        Js::StatementReader<Js::FunctionBody::ArenaStatementMapList> * statementReader, ArenaAllocator* alloc);

    JITTimeFunctionBody * GetJITFunctionBody();
    uint16 GetProfiledIterations() const;

    CodeGenWorkItemIDL* GetWorkItemData();
    JITTimePolymorphicInlineCacheInfo * GetPolymorphicInlineCacheInfo();
    JITTimePolymorphicInlineCacheInfo * GetInlineePolymorphicInlineCacheInfo(intptr_t funcBodyAddr);

    void SetJITTimeData(FunctionJITTimeDataIDL * jitData);
    FunctionJITTimeInfo * GetJITTimeInfo() const;

    bool TryGetValueType(uint symId, ValueType * valueType) const;
    bool HasSymIdToValueTypeMap() const;

private:
    Js::FunctionBody::ArenaStatementMapList * m_fullStatementList;

    CodeGenWorkItemIDL * m_workItemData;
    JITTimeFunctionBody m_jitBody;
    Js::SmallSpanSequence m_statementMap;
};
