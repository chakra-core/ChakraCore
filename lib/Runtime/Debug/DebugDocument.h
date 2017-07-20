//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

struct StatementSpan
{
    int32 ich;
    int32 cch;
};

// A Document in Engine means a file, eval code or new function code. For each of these there is a Utf8SourceInfo.
// DebugDocument relates debug operations such as adding/remove breakpoints to a specific Utf8SourceInfo.

namespace Js
{
    class DebugDocument
    {
    public:
        DebugDocument(Utf8SourceInfo* utf8SourceInfo, Js::FunctionBody* functionBody);
        ~DebugDocument();
        virtual void CloseDocument();

        HRESULT SetBreakPoint(int32 ibos, BREAKPOINT_STATE bps);
        BreakpointProbe* SetBreakPoint(StatementLocation statement, BREAKPOINT_STATE bps);
        void RemoveBreakpointProbe(BreakpointProbe *probe);
        void ClearAllBreakPoints(void);

#if ENABLE_TTD
        BreakpointProbe* SetBreakPoint_TTDWbpId(int64 bpId, StatementLocation statement);
#endif

        BreakpointProbe* FindBreakpoint(StatementLocation statement);
        bool FindBPStatementLocation(UINT bpId, StatementLocation * statement);

        BOOL GetStatementSpan(int32 ibos, StatementSpan* pBos);
        BOOL GetStatementLocation(int32 ibos, StatementLocation* plocation);

        virtual bool HasDocumentText() const
        {
            Assert(false);
            return false;
        }
        virtual void* GetDocumentText() const
        {
            Assert(false);
            return nullptr;
        };

        Js::FunctionBody * GetFunctionBodyAt(int32 ibos);

        Utf8SourceInfo* GetUtf8SourceInfo() { return this->utf8SourceInfo; }

    private:
        Utf8SourceInfo* utf8SourceInfo;
        RecyclerRootPtr<Js::FunctionBody> functionBody;
        BreakpointProbeList* m_breakpointList;

        BreakpointProbeList* NewBreakpointList(ArenaAllocator* arena);
        BreakpointProbeList* GetBreakpointList();

        BOOL HasLineBreak(int32 _start, int32 _end);
    };
}
