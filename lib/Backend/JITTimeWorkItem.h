//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// class for the backend to use to access CodeGenWorkItem properties
class JITTimeWorkItem
{
public:
    JITTimeWorkItem(const CodeGenWorkItemJITData * const workItemData);

    CodeGenWorkItemType Type() const;
    ExecutionMode GetJitMode() const;
    WCHAR * GetDisplayName() const;
    size_t GetDisplayName(
        _Out_writes_opt_z_(sizeInChars) WCHAR* displayName,
        _In_ size_t sizeInChars);
    uint GetInterpretedCount() const;
    uint GetLoopNumber() const;
    bool IsLoopBody() const;
    bool IsJitInDebugMode() const;
    intptr_t GetCallsCountAddress() const;

    const JITTimeFunctionBody * const GetJITFunctionBody() const;

private:
    const CodeGenWorkItemJITData * const m_workItemData;
    const JITTimeFunctionBody m_jitBody;



public: // TODO: (michhol) remove these. currently needed to compile
    Js::EntryPointInfo * GetEntryPoint() { __debugbreak(); return nullptr; }
    void RecordNativeCodeSize(...) { __debugbreak(); }
    size_t GetCodeAddress() { __debugbreak(); return 0; }
    void RecordNativeCode(...) { __debugbreak(); }
    void RecordUnwindInfo(...) { __debugbreak(); }
    void FinalizeNativeCode(...) { __debugbreak(); }
    void DumpNativeOffsetMaps() { __debugbreak(); }
    void DumpNativeThrowSpanSequence() { __debugbreak(); }
    void RecordNativeMap(...) { __debugbreak(); }
    void RecordNativeThrowMap(...) { __debugbreak(); }
    void SetFrameHeight(...) { __debugbreak(); }
};
