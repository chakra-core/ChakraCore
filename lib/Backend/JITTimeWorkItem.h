//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// class for the backend to use to access CodeGenWorkItem properties
class JITTimeWorkItem
{
public:
    JITTimeWorkItem(CodeGenWorkItemJITData * workItemData);

    CodeGenWorkItemType Type() const;
    ExecutionMode GetJitMode() const;
    uint GetLoopNumber() const;

    const JITLoopHeader * GetLoopHeader() const;
    intptr_t GetLoopHeaderAddr() const;

    bool IsLoopBody() const;
    bool IsJitInDebugMode() const;
    
    intptr_t GetCallsCountAddress() const;

    void InitializeReader(
        Js::ByteCodeReader * reader,
        Js::StatementReader * statementReader);

    JITTimeFunctionBody * GetJITFunctionBody();

    CodeGenWorkItemJITData* GetWorkItemData();

    void SetJITTimeData(FunctionJITTimeData * jitData);
    const FunctionJITTimeInfo * GetJITTimeInfo() const;

private:
    CodeGenWorkItemJITData * m_workItemData;
    JITTimeFunctionBody m_jitBody;
    Js::SmallSpanSequence m_statementMap;


public: // TODO: (michhol) remove these. currently needed to compile
    Js::EntryPointInfo * GetEntryPoint() { __debugbreak(); return nullptr; }
    void DumpNativeOffsetMaps() { __debugbreak(); }
    void DumpNativeThrowSpanSequence() { __debugbreak(); }
    void RecordNativeMap(...) { __debugbreak(); }
    void RecordNativeThrowMap(...) { __debugbreak(); }
};
