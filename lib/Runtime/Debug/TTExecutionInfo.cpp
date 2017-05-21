//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    TTDebuggerAbortException::TTDebuggerAbortException(uint32 abortCode, int64 optEventTime, int64 optMoveMode, const char16* staticAbortMessage)
        : m_abortCode(abortCode), m_optEventTime(optEventTime), m_optMoveMode(optMoveMode), m_staticAbortMessage(staticAbortMessage)
    {
        ;
    }

    TTDebuggerAbortException::~TTDebuggerAbortException()
    {
        ;
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateAbortEndOfLog(const char16* staticMessage)
    {
        return TTDebuggerAbortException(1, -1, 0, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateTopLevelAbortRequest(int64 targetEventTime, int64 moveMode, const char16* staticMessage)
    {
        return TTDebuggerAbortException(2, targetEventTime, moveMode, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateUncaughtExceptionAbortRequest(int64 targetEventTime, const char16* staticMessage)
    {
        return TTDebuggerAbortException(3, targetEventTime, 0, staticMessage);
    }

    bool TTDebuggerAbortException::IsEndOfLog() const
    {
        return this->m_abortCode == 1;
    }

    bool TTDebuggerAbortException::IsEventTimeMove() const
    {
        return this->m_abortCode == 2;
    }

    bool TTDebuggerAbortException::IsTopLevelException() const
    {
        return this->m_abortCode == 3;
    }

    int64 TTDebuggerAbortException::GetTargetEventTime() const
    {
        return this->m_optEventTime;
    }

    int64 TTDebuggerAbortException::GetMoveMode() const
    {
        return this->m_optMoveMode;
    }

    const char16* TTDebuggerAbortException::GetStaticAbortMessage() const
    {
        return this->m_staticAbortMessage;
    }

    Js::FunctionBody* TTDebuggerSourceLocation::UpdatePostInflateFunctionBody_Helper(Js::FunctionBody* rootBody) const
    {
        for(uint32 i = 0; i < rootBody->GetNestedCount(); ++i)
        {
            Js::ParseableFunctionInfo* ipfi = rootBody->GetNestedFunctionForExecution(i);
            Js::FunctionBody* ifb = JsSupport::ForceAndGetFunctionBody(ipfi);

            if(this->m_functionLine == ifb->GetLineNumber() && this->m_functionColumn == ifb->GetColumnNumber())
            {
                return ifb;
            }
            else
            {
                Js::FunctionBody* found = this->UpdatePostInflateFunctionBody_Helper(ifb);
                if(found != nullptr)
                {
                    return found;
                }
            }
        }

        return nullptr;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation()
        : m_sourceScriptLogId(TTD_INVALID_LOG_PTR_ID), m_bpId(-1),
        m_etime(-1), m_ftime(0), m_ltime(0),
        m_topLevelBodyId(0), m_functionLine(0), m_functionColumn(0), m_line(0), m_column(0)
    {
        ;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other)
        : m_sourceScriptLogId(other.m_sourceScriptLogId), m_bpId(other.m_bpId),
        m_etime(other.m_etime), m_ftime(other.m_ftime), m_ltime(other.m_ltime),
        m_topLevelBodyId(other.m_topLevelBodyId), m_functionLine(other.m_functionLine), m_functionColumn(other.m_functionColumn), m_line(other.m_line), m_column(other.m_column)
    {
        ;
    }

    TTDebuggerSourceLocation::~TTDebuggerSourceLocation()
    {
        this->Clear();
    }

    TTDebuggerSourceLocation& TTDebuggerSourceLocation::operator= (const TTDebuggerSourceLocation& other)
    {
        if(this != &other)
        {
            this->SetLocationCopy(other);
        }

        return *this;
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void TTDebuggerSourceLocation::PrintToConsole() const
    {
        if(!this->HasValue())
        {
            wprintf(_u("undef"));
        }
        else
        {
            printf("BP: { bpId: %I64i, ctx: %I64u, topId: %I32u, fline: %I32u, fcolumn: %I32u, line: %I32u, column: %I32u }", this->m_bpId, this->m_sourceScriptLogId, this->m_topLevelBodyId, this->m_functionLine, this->m_functionColumn, this->m_line, this->m_column);
            if(this->m_etime != -1)
            {
                printf(" TTDTime: { etime: %I64i, ftime: %I64i, ltime: %I64i }", this->m_etime, this->m_ftime, this->m_ltime);
            }
        }
    }
#endif

    void TTDebuggerSourceLocation::Initialize()
    {
        this->m_sourceScriptLogId = TTD_INVALID_LOG_PTR_ID;
        this->m_bpId = -1;

        this->m_etime = -1;
        this->m_ftime = 0;
        this->m_ltime = 0;

        this->m_topLevelBodyId = 0;

        this->m_functionLine = 0;
        this->m_functionColumn = 0;
        this->m_line = 0;
        this->m_column = 0;
    }

    bool TTDebuggerSourceLocation::HasValue() const
    {
        return this->m_sourceScriptLogId != TTD_INVALID_LOG_PTR_ID;
    }

    bool TTDebuggerSourceLocation::HasTTDTimeValue() const
    {
        return this->m_etime != -1;
    }

    void TTDebuggerSourceLocation::Clear()
    {
        this->m_sourceScriptLogId = TTD_INVALID_LOG_PTR_ID;
        this->m_bpId = -1;

        this->m_etime = -1;
        this->m_ftime = 0;
        this->m_ltime = 0;

        this->m_topLevelBodyId = 0;

        this->m_functionLine = 0;
        this->m_functionColumn = 0;

        this->m_line = 0;
        this->m_column = 0;
    }

    void TTDebuggerSourceLocation::SetLocationCopy(const TTDebuggerSourceLocation& other)
    {
        this->m_sourceScriptLogId = other.m_sourceScriptLogId;
        this->m_bpId = other.m_bpId;

        this->m_etime = other.m_etime;
        this->m_ftime = other.m_ftime;
        this->m_ltime = other.m_ltime;

        this->m_topLevelBodyId = other.m_topLevelBodyId;

        this->m_functionLine = other.m_functionLine;
        this->m_functionColumn = other.m_functionColumn;

        this->m_line = other.m_line;
        this->m_column = other.m_column;
    }

    void TTDebuggerSourceLocation::SetLocationFromFrame(int64 topLevelETime, const SingleCallCounter& callFrame)
    {
        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = callFrame.Function->GetStatementStartOffset(callFrame.CurrentStatementIndex);
        callFrame.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        this->SetLocationFull(topLevelETime, callFrame.FunctionTime, callFrame.LoopTime, callFrame.Function, (uint32)srcLine, (uint32)srcColumn);
    }

    void TTDebuggerSourceLocation::SetLocationFromFunctionEntryAnyTime(int64 topLevelETime, Js::FunctionBody* body)
    {
        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = body->GetStatementStartOffset(0);
        body->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        this->SetLocationFull(topLevelETime, -1, -1, body, (uint32)srcLine, (uint32)srcColumn);
    }

    void TTDebuggerSourceLocation::SetLocationFull(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column)
    {
        this->m_sourceScriptLogId = body->GetScriptContext()->ScriptContextLogTag;
        this->m_bpId = -1;

        this->m_etime = etime;
        this->m_ftime = ftime;
        this->m_ltime = ltime;

        this->m_topLevelBodyId = body->GetScriptContext()->TTDContextInfo->FindTopLevelCtrForBody(body);

        this->m_functionLine = body->GetLineNumber();
        this->m_functionColumn = body->GetColumnNumber();

        this->m_line = (uint32)line;
        this->m_column = (uint32)column;
    }

    void TTDebuggerSourceLocation::SetLocationWithBP(int64 bpId, Js::FunctionBody* body, ULONG line, LONG column)
    {
        this->m_sourceScriptLogId = body->GetScriptContext()->ScriptContextLogTag;
        this->m_bpId = bpId;

        this->m_etime = -1;
        this->m_ftime = -1;
        this->m_ltime = -1;

        this->m_topLevelBodyId = body->GetScriptContext()->TTDContextInfo->FindTopLevelCtrForBody(body);

        this->m_functionLine = body->GetLineNumber();
        this->m_functionColumn = body->GetColumnNumber();

        this->m_line = (uint32)line;
        this->m_column = (uint32)column;
    }

    int64 TTDebuggerSourceLocation::GetRootEventTime() const
    {
        return this->m_etime;
    }

    int64 TTDebuggerSourceLocation::GetFunctionTime() const
    {
        return this->m_ftime;
    }

    int64 TTDebuggerSourceLocation::GetLoopTime() const
    {
        return this->m_ltime;
    }

    TTD_LOG_PTR_ID TTDebuggerSourceLocation::GetScriptLogTagId() const
    {
        return this->m_sourceScriptLogId;
    }

    Js::FunctionBody* TTDebuggerSourceLocation::LoadFunctionBodyIfPossible(Js::ScriptContext* inCtx) const
    {
        Js::FunctionBody* rootBody = inCtx->TTDContextInfo->FindRootBodyByTopLevelCtr(this->m_topLevelBodyId);
        if(rootBody == nullptr)
        {
            return nullptr;
        }

        if(this->m_functionLine == rootBody->GetLineNumber() && this->m_functionColumn == rootBody->GetColumnNumber())
        {
            return rootBody;
        }
        else
        {
            return this->UpdatePostInflateFunctionBody_Helper(rootBody);
        }
    }

    int64 TTDebuggerSourceLocation::GetBPId() const
    {
        return this->m_bpId;
    }

    uint32 TTDebuggerSourceLocation::GetTopLevelBodyId() const
    {
        return this->m_topLevelBodyId;
    }

    uint32 TTDebuggerSourceLocation::GetSourceLine() const
    {
        return this->m_line;
    }

    uint32 TTDebuggerSourceLocation::GetSourceColumn() const
    {
        return this->m_column;
    }

    bool TTDebuggerSourceLocation::AreSameLocations(const TTDebuggerSourceLocation& sl1, const TTDebuggerSourceLocation& sl2)
    {
        //first check the code locations
        if(sl1.m_topLevelBodyId != sl2.m_topLevelBodyId || sl1.m_line != sl2.m_line || sl1.m_column != sl2.m_column)
        {
            return false;
        }

        //next check the time values
        if(sl1.m_etime != sl2.m_etime || sl1.m_ftime != sl2.m_ftime || sl1.m_ltime != sl2.m_ltime)
        {
            return false;
        }

        return true;
    }

    bool TTDebuggerSourceLocation::AreSameLocations_PlaceOnly(const TTDebuggerSourceLocation& sl1, const TTDebuggerSourceLocation& sl2)
    {
        return (sl1.m_topLevelBodyId == sl2.m_topLevelBodyId && sl1.m_line == sl2.m_line && sl1.m_column == sl2.m_column);
    }

    TTLastReturnLocationInfo::TTLastReturnLocationInfo()
        : m_isExceptionFrame(false)
    {
        this->m_lastFrame = { 0 };
    }

    void TTLastReturnLocationInfo::SetReturnLocation(const SingleCallCounter& cframe)
    {
        this->m_isExceptionFrame = false;
        this->m_lastFrame = cframe;
    }

    void TTLastReturnLocationInfo::SetExceptionLocation(const SingleCallCounter& cframe)
    {
        this->m_isExceptionFrame = true;
        this->m_lastFrame = cframe;
    }

    bool TTLastReturnLocationInfo::IsDefined() const
    {
        return this->m_lastFrame.Function != nullptr;
    }

    bool TTLastReturnLocationInfo::IsReturnLocation() const
    {
        return this->IsDefined() && !this->m_isExceptionFrame;
    }

    bool TTLastReturnLocationInfo::IsExceptionLocation() const
    {
        return this->IsDefined() && this->m_isExceptionFrame;
    }

    const SingleCallCounter& TTLastReturnLocationInfo::GetLocation() const
    {
        TTDAssert(this->IsDefined(), "Should check this!");

        return this->m_lastFrame;
    }

    void TTLastReturnLocationInfo::Clear()
    {
        if(this->IsDefined())
        {
            this->m_isExceptionFrame = false;
            this->m_lastFrame = { 0 };
        }
    }

    void TTLastReturnLocationInfo::ClearReturnOnly()
    {
        if(this->IsDefined() && !this->m_isExceptionFrame)
        {
            this->Clear();
        }
    }

    void TTLastReturnLocationInfo::ClearExceptionOnly()
    {
        if(this->IsDefined() && this->m_isExceptionFrame)
        {
            this->Clear();
        }
    }

    //////////////////

    bool ExecutionInfoManager::ShouldSuppressBreakpointsForTimeTravelMove(TTDMode mode)
    {
        return (mode & TTD::TTDMode::DebuggerSuppressBreakpoints) == TTD::TTDMode::DebuggerSuppressBreakpoints;
    }

    bool ExecutionInfoManager::ShouldRecordBreakpointsDuringTimeTravelScan(TTDMode mode)
    {
        return (mode & TTD::TTDMode::DebuggerLogBreakpoints) == TTD::TTDMode::DebuggerLogBreakpoints;
    }


    ExecutionInfoManager::ExecutionInfoManager()
        : m_topLevelCallbackEventTime(-1), m_runningFunctionTimeCtr(0), m_callStack(&HeapAllocator::Instance),
        m_debuggerNotifiedTopLevelBodies(&HeapAllocator::Instance),
        m_lastReturnLocation(), m_lastExceptionPropagating(false), m_lastExceptionLocation(),
        m_breakOnFirstUserCode(false),
        m_pendingTTDBP(), m_pendingTTDMoveMode(-1), m_activeBPId(-1), m_shouldRemoveWhenDone(false), m_activeTTDBP(),
        m_hitContinueSearchBP(false), m_continueBreakPoint(),
        m_unRestoredBreakpoints(&HeapAllocator::Instance)
    {
        ;
    }

    ExecutionInfoManager::~ExecutionInfoManager()
    {
        if(this->m_unRestoredBreakpoints.Count() != 0)
        {
            for(int32 i = 0; i < this->m_unRestoredBreakpoints.Count(); ++i)
            {
                HeapDelete(this->m_unRestoredBreakpoints.Item(i));
            }
        }
    }

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
    TraceLogger* ExecutionInfoManager::GetTraceLogger()
    {
        return &(this->m_diagnosticLogger);
    }
#endif

    const SingleCallCounter& ExecutionInfoManager::GetTopCallCounter() const
    {
        TTDAssert(this->m_callStack.Count() != 0, "Empty stack!");

        return this->m_callStack.Item(this->m_callStack.Count() - 1);
    }

    SingleCallCounter& ExecutionInfoManager::GetTopCallCounter()
    {
        TTDAssert(this->m_callStack.Count() != 0, "Empty stack!");

        return this->m_callStack.Item(this->m_callStack.Count() - 1);
    }

    bool ExecutionInfoManager::TryGetTopCallCallerCounter(SingleCallCounter& caller) const
    {
        if(this->m_callStack.Count() < 2)
        {
            return false;
        }
        else
        {
            caller = this->m_callStack.Item(this->m_callStack.Count() - 2);
            return true;
        }
    }

    void ExecutionInfoManager::PushCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool isInFinally)
    {
        //Clear any previous last return frame info
        this->m_lastReturnLocation.ClearReturnOnly();

        this->m_runningFunctionTimeCtr++;

        SingleCallCounter cfinfo;
        cfinfo.Function = function->GetFunctionBody();

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        cfinfo.Name = cfinfo.Function->GetExternalDisplayName();
#endif

        cfinfo.FunctionTime = this->m_runningFunctionTimeCtr;
        cfinfo.LoopTime = 0;

        cfinfo.CurrentStatementIndex = -1;
        cfinfo.CurrentStatementLoopTime = 0;

        cfinfo.LastStatementIndex = -1;
        cfinfo.LastStatementLoopTime = 0;

        cfinfo.CurrentStatementBytecodeMin = UINT32_MAX;
        cfinfo.CurrentStatementBytecodeMax = UINT32_MAX;

        this->m_callStack.Add(cfinfo);

        ////
        //If we are running for debugger then check if we need to set a breakpoint at entry to the first function we execute
        if(this->m_breakOnFirstUserCode)
        {
            this->m_breakOnFirstUserCode = false;

            this->m_activeTTDBP.SetLocationFromFunctionEntryAnyTime(this->m_topLevelCallbackEventTime, cfinfo.Function);
            this->SetActiveBP_Helper(function->GetFunctionBody()->GetScriptContext()->GetThreadContext()->TTDContext);
        }
        ////

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteCall(function, false, argc, argv, function->GetFunctionBody()->GetScriptContext()->GetThreadContext()->TTDLog->GetLastEventTime());
#endif
    }

    void ExecutionInfoManager::PopCallEvent(Js::JavascriptFunction* function, Js::Var result)
    {
        this->m_lastReturnLocation.SetReturnLocation(this->m_callStack.Last());

        this->m_runningFunctionTimeCtr++;
        this->m_callStack.RemoveAtEnd();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturn(function, result, function->GetFunctionBody()->GetScriptContext()->GetThreadContext()->TTDLog->GetLastEventTime());
#endif
    }

    void ExecutionInfoManager::PopCallEventException(Js::JavascriptFunction* function)
    {
        //If we already have the last return as an exception then just leave it.
        //That is where the exception was first rasied, this return is just propagating it in this return.

        if(!this->m_lastReturnLocation.IsExceptionLocation())
        {
            this->m_lastReturnLocation.SetExceptionLocation(this->m_callStack.Last());
        }

        if(!m_lastExceptionPropagating)
        {
            this->m_lastExceptionLocation.SetLocationFromFrame(this->m_topLevelCallbackEventTime, this->m_callStack.Last());
            this->m_lastExceptionPropagating = true;
        }

        this->m_runningFunctionTimeCtr++;
        this->m_callStack.RemoveAtEnd();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturnException(function, function->GetFunctionBody()->GetScriptContext()->GetThreadContext()->TTDLog->GetLastEventTime());
#endif
    }

    void ExecutionInfoManager::ProcessCatchInfoForLastExecutedStatements()
    {
        this->m_lastReturnLocation.Clear();

        this->m_lastExceptionPropagating = false;
    }

    void ExecutionInfoManager::SetBreakOnFirstUserCode()
    {
        this->m_breakOnFirstUserCode = true;
    }

    void ExecutionInfoManager::SetPendingTTDStepBackMove()
    {
        this->GetPreviousTimeAndPositionForDebugger(this->m_pendingTTDBP);

        this->m_pendingTTDMoveMode = 0;
    }

    void ExecutionInfoManager::SetPendingTTDStepBackIntoMove()
    {
        this->GetLastExecutedTimeAndPositionForDebugger(this->m_pendingTTDBP);

        this->m_pendingTTDMoveMode = 0;
    }

    void ExecutionInfoManager::SetPendingTTDReverseContinueMove(uint64 moveflag)
    {
        this->m_continueBreakPoint.Clear();
        this->GetTimeAndPositionForDebugger(this->m_pendingTTDBP);

        this->m_pendingTTDMoveMode = moveflag;
    }

    void ExecutionInfoManager::SetPendingTTDUnhandledException()
    {
        this->GetLastExceptionTimeAndPositionForDebugger(this->m_pendingTTDBP);
        this->m_lastExceptionPropagating = false;

        this->m_pendingTTDMoveMode = 0;
    }

    bool ExecutionInfoManager::ProcessBPInfoPreBreak(Js::FunctionBody* fb, const EventLog* elog)
    {
        //if we aren't in debug mode then we always trigger BP's
        if(!fb->GetScriptContext()->ShouldPerformDebuggerAction())
        {
            return true;
        }

        //If we are in debugger mode but are suppressing BP's for movement then suppress them
        if(ExecutionInfoManager::ShouldSuppressBreakpointsForTimeTravelMove(elog->GetCurrentTTDMode()))
        {
            //Check if we need to record the visit to this bp
            if(ExecutionInfoManager::ShouldRecordBreakpointsDuringTimeTravelScan(elog->GetCurrentTTDMode()))
            {
                this->AddCurrentLocationDuringScan(elog->GetCurrentTopLevelEventTime());
            }

            return false;
        }

        //If we are in debug mode and don't have an active BP target then we treat BP's as usual
        if(!this->m_activeTTDBP.HasValue())
        {
            return true;
        }

        //Finally we are in debug mode and we have an active BP target so only break if the BP is satisfied
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        //Make sure we are in the right source code (via top level body ids)
        if(this->m_activeTTDBP.GetTopLevelBodyId() != cfinfo.Function->GetScriptContext()->TTDContextInfo->FindTopLevelCtrForBody(cfinfo.Function))
        {
            return false;
        }

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        bool locationOk = ((uint32)srcLine == this->m_activeTTDBP.GetSourceLine()) & ((uint32)srcColumn == this->m_activeTTDBP.GetSourceColumn());
        bool ftimeOk = (this->m_activeTTDBP.GetFunctionTime() == -1) | ((uint64)this->m_activeTTDBP.GetFunctionTime() == cfinfo.FunctionTime);
        bool ltimeOk = (this->m_activeTTDBP.GetLoopTime() == -1) | ((uint64)this->m_activeTTDBP.GetLoopTime() == cfinfo.CurrentStatementLoopTime);

        return locationOk & ftimeOk & ltimeOk;
    }

    void ExecutionInfoManager::ProcessBPInfoPostBreak(Js::FunctionBody* fb)
    {
        if(!fb->GetScriptContext()->ShouldPerformDebuggerAction())
        {
            return;
        }

        if(this->m_activeTTDBP.HasValue())
        {
            this->ClearActiveBPInfo(fb->GetScriptContext()->GetThreadContext()->TTDContext);
        }

        if(this->m_pendingTTDBP.HasValue())
        {
            //Reset any step controller logic
            fb->GetScriptContext()->GetThreadContext()->GetDebugManager()->stepController.Deactivate();

            throw TTD::TTDebuggerAbortException::CreateTopLevelAbortRequest(this->m_pendingTTDBP.GetRootEventTime(), this->m_pendingTTDMoveMode, _u("Reverse operation requested."));
        }
    }

    bool ExecutionInfoManager::IsLocationActiveBPAndNotExplicitBP(const TTDebuggerSourceLocation& current) const
    {
        //If we shound't remove the active BP when done then it is a user visible one so we just return false
        if(!this->m_shouldRemoveWhenDone)
        {
            return false;
        }

        //Check to see if the locations are the same
        return TTDebuggerSourceLocation::AreSameLocations_PlaceOnly(this->m_activeTTDBP, current);
    }

    void ExecutionInfoManager::AddCurrentLocationDuringScan(int64 topLevelEventTime)
    {
        TTDebuggerSourceLocation current;
        current.SetLocationFromFrame(topLevelEventTime, this->m_callStack.Last());

        if(this->m_activeTTDBP.HasValue() && TTDebuggerSourceLocation::AreSameLocations(this->m_activeTTDBP, current))
        {
            this->m_hitContinueSearchBP = true;
        }

        if(!this->m_hitContinueSearchBP && !this->IsLocationActiveBPAndNotExplicitBP(current))
        {
            this->m_continueBreakPoint.SetLocationCopy(current);
        }
    }

    bool ExecutionInfoManager::TryFindAndSetPreviousBP()
    {
        if(!this->m_continueBreakPoint.HasValue())
        {
            return false;
        }
        else
        {
            this->m_pendingTTDBP.SetLocationCopy(this->m_continueBreakPoint);
            this->m_continueBreakPoint.Clear();

            return true;
        }
    }

    //Get the target event time for the pending TTD breakpoint
    int64 ExecutionInfoManager::GetPendingTTDBPTargetEventTime() const
    {
        TTDAssert(this->m_pendingTTDBP.HasValue(), "We did not set the pending TTD breakpoint value?");

        return this->m_pendingTTDBP.GetRootEventTime();
    }

    void ExecutionInfoManager::LoadPreservedBPInfo(ThreadContext* threadContext)
    {
        while(this->m_unRestoredBreakpoints.Contains(nullptr))
        {
            this->m_unRestoredBreakpoints.Remove(nullptr);
        }

        const JsUtil::List<Js::ScriptContext*, HeapAllocator>& ctxs = threadContext->TTDContext->GetTTDContexts();
        for(int32 i = 0; i < ctxs.Count(); ++i)
        {
            Js::ScriptContext* ctx = ctxs.Item(i);
            JsUtil::List<uint32, HeapAllocator> bpIdsToDelete(&HeapAllocator::Instance);

            Js::ProbeContainer* probeContainer = ctx->GetDebugContext()->GetProbeContainer();
            probeContainer->MapProbes([&](int j, Js::Probe* pProbe)
            {
                Js::BreakpointProbe* bp = (Js::BreakpointProbe*)pProbe;
                if((int64)bp->GetId() != this->m_activeBPId)
                {
                    Js::FunctionBody* body = bp->GetFunctionBody();
                    int32 bpIndex = body->GetEnclosingStatementIndexFromByteCode(bp->GetBytecodeOffset());

                    ULONG srcLine = 0;
                    LONG srcColumn = -1;
                    uint32 startOffset = body->GetStatementStartOffset(bpIndex);
                    body->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

                    TTDebuggerSourceLocation* bpLocation = TT_HEAP_NEW(TTDebuggerSourceLocation);
                    bpLocation->SetLocationWithBP(bp->GetId(), body, srcLine, srcColumn);
                    this->m_unRestoredBreakpoints.Add(bpLocation);

                    bpIdsToDelete.Add(bp->GetId());
                }
            });

            for(int32 j = 0; j < bpIdsToDelete.Count(); ++j)
            {
                ctx->TTDHostCallbackFunctor.pfOnBPDeleteCallback(ctx->TTDHostCallbackFunctor.HostRuntime, bpIdsToDelete.Item(j));
            }

            //done with last context so release the debug document dictionary info
            if(i == ctxs.Count() - 1)
            {
                ctx->TTDHostCallbackFunctor.pfOnBPClearDocumentCallback(ctx->TTDHostCallbackFunctor.HostRuntime);
            }
        }
    }

    void ExecutionInfoManager::AddPreservedBPsForTopLevelLoad(uint32 topLevelBodyId, Js::FunctionBody* body)
    {
        Js::ScriptContext* ctx = body->GetScriptContext();
        Js::Utf8SourceInfo* utf8SourceInfo = body->GetUtf8SourceInfo();

        for(int32 i = 0; i < this->m_unRestoredBreakpoints.Count(); ++i)
        {
            if(this->m_unRestoredBreakpoints.Item(i) == nullptr)
            {
                continue;
            }

            const TTDebuggerSourceLocation* bpLocation = this->m_unRestoredBreakpoints.Item(i);
            if(bpLocation->GetTopLevelBodyId() == topLevelBodyId)
            {
                BOOL isNewBP = FALSE;
                ctx->TTDHostCallbackFunctor.pfOnBPRegisterCallback(ctx->TTDHostCallbackFunctor.HostRuntime, bpLocation->GetBPId(), ctx, utf8SourceInfo, bpLocation->GetSourceLine(), bpLocation->GetSourceColumn(), &isNewBP);
                TTDAssert(isNewBP, "We should be restoring a breapoint we removed previously!");

                //Now that it is set we can clear it from our list
                this->m_unRestoredBreakpoints.SetItem(i, nullptr);
            }
        }

        if(this->m_activeTTDBP.HasValue() && (this->m_activeBPId == -1) && (this->m_activeTTDBP.GetTopLevelBodyId() == topLevelBodyId))
        {
            this->SetActiveBP_Helper(body->GetScriptContext()->GetThreadContext()->TTDContext);
        }
    }

    void ExecutionInfoManager::UpdateLoopCountInfo()
    {
        SingleCallCounter& cfinfo = this->m_callStack.Last();
        cfinfo.LoopTime++;
    }

    void ExecutionInfoManager::UpdateCurrentStatementInfo(uint bytecodeOffset)
    {
        SingleCallCounter& cfinfo = this->GetTopCallCounter();

        if((cfinfo.CurrentStatementBytecodeMin <= bytecodeOffset) & (bytecodeOffset <= cfinfo.CurrentStatementBytecodeMax))
        {
            return;
        }
        else
        {
            Js::FunctionBody* fb = cfinfo.Function;

            int32 cIndex = fb->GetEnclosingStatementIndexFromByteCode(bytecodeOffset, true);
            TTDAssert(cIndex != -1, "Should always have a mapping.");

            //we moved to a new statement
            Js::FunctionBody::StatementMap* pstmt = fb->GetStatementMaps()->Item(cIndex);
            bool newstmt = (cIndex != cfinfo.CurrentStatementIndex && pstmt->byteCodeSpan.begin <= (int)bytecodeOffset && (int)bytecodeOffset <= pstmt->byteCodeSpan.end);
            if(newstmt)
            {
                cfinfo.LastStatementIndex = cfinfo.CurrentStatementIndex;
                cfinfo.LastStatementLoopTime = cfinfo.CurrentStatementLoopTime;

                cfinfo.CurrentStatementIndex = cIndex;
                cfinfo.CurrentStatementLoopTime = cfinfo.LoopTime;

                cfinfo.CurrentStatementBytecodeMin = (uint32)pstmt->byteCodeSpan.begin;
                cfinfo.CurrentStatementBytecodeMax = (uint32)pstmt->byteCodeSpan.end;

#if ENABLE_FULL_BC_TRACE
                ULONG srcLine = 0;
                LONG srcColumn = -1;
                uint32 startOffset = cfinfo.Function->GetFunctionBody()->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
                cfinfo.Function->GetFunctionBody()->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

                this->m_diagnosticLogger.WriteStmtIndex((uint32)srcLine, (uint32)srcColumn);
#endif
            }
        }
    }

    void ExecutionInfoManager::GetTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        sourceLocation.SetLocationFull(this->m_topLevelCallbackEventTime, cfinfo.FunctionTime, cfinfo.LoopTime, cfinfo.Function, srcLine, srcColumn);
    }

#if ENABLE_OBJECT_SOURCE_TRACKING
    void ExecutionInfoManager::GetTimeAndPositionForDiagnosticObjectTracking(DiagnosticOrigin& originInfo) const
    {
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        SetDiagnosticOriginInformation(originInfo, srcLine, cfinfo.Function->GetFunctionBody()->GetScriptContext()->GetThreadContext()->TTDLog->GetLastEventTime(), cfinfo.FunctionTime, cfinfo.LoopTime);
    }
#endif

    bool ExecutionInfoManager::GetPreviousTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        bool noPrevious = false;
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        //if we are at the first statement in the function then we want the parents current
        Js::FunctionBody* fbody = nullptr;
        int32 statementIndex = -1;
        uint64 ftime = 0;
        uint64 ltime = 0;
        if(cfinfo.LastStatementIndex == -1)
        {
            SingleCallCounter cfinfoCaller = { 0 };
            bool hasCaller = this->TryGetTopCallCallerCounter(cfinfoCaller);

            //check if we are at the first statement in the callback event
            if(!hasCaller)
            {
                //Set the position info to the current statement and return true
                noPrevious = true;

                ftime = cfinfo.FunctionTime;
                ltime = cfinfo.CurrentStatementLoopTime;

                fbody = cfinfo.Function;
                statementIndex = cfinfo.CurrentStatementIndex;
            }
            else
            {
                ftime = cfinfoCaller.FunctionTime;
                ltime = cfinfoCaller.CurrentStatementLoopTime;

                fbody = cfinfoCaller.Function;
                statementIndex = cfinfoCaller.CurrentStatementIndex;
            }
        }
        else
        {
            ftime = cfinfo.FunctionTime;
            ltime = cfinfo.LastStatementLoopTime;

            fbody = cfinfo.Function;
            statementIndex = cfinfo.LastStatementIndex;
        }

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = fbody->GetStatementStartOffset(statementIndex);
        fbody->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        sourceLocation.SetLocationFull(this->m_topLevelCallbackEventTime, ftime, ltime, fbody, srcLine, srcColumn);

        return noPrevious;
    }

    void ExecutionInfoManager::GetLastExecutedTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        const TTLastReturnLocationInfo& cframe = this->m_lastReturnLocation;
        if(!cframe.IsDefined())
        {
            sourceLocation.Clear();
            return;
        }
        else
        {
            ULONG srcLine = 0;
            LONG srcColumn = -1;
            uint32 startOffset = cframe.GetLocation().Function->GetStatementStartOffset(cframe.GetLocation().CurrentStatementIndex);
            cframe.GetLocation().Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

            sourceLocation.SetLocationFull(this->m_topLevelCallbackEventTime, cframe.GetLocation().FunctionTime, cframe.GetLocation().CurrentStatementLoopTime, cframe.GetLocation().Function, srcLine, srcColumn);
        }
    }

    void ExecutionInfoManager::GetLastExceptionTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        //If no exception then this will also clear sourceLocation
        sourceLocation.SetLocationCopy(this->m_lastExceptionLocation);
    }

    void ExecutionInfoManager::ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime)
    {
        TTDAssert(this->m_callStack.Count() == 0, "We should be at the top-level entry!!!");

        this->m_topLevelCallbackEventTime = topLevelCallbackEventTime;

        this->m_runningFunctionTimeCtr = 0;
        this->m_lastReturnLocation.Clear();
        this->m_lastExceptionLocation.Clear();
    }

    void ExecutionInfoManager::SetBPInfoForActiveSegmentContinueScan(ThreadContextTTD* ttdThreadContext)
    {
        TTDAssert(this->m_pendingTTDBP.HasValue(), "We should always set this when launching a reverse continue!");

        this->m_activeTTDBP.SetLocationCopy(this->m_pendingTTDBP);
        this->SetActiveBPInfoAsNeeded(ttdThreadContext);
    }

    void ExecutionInfoManager::ClearBPInfoForActiveSegmentContinueScan(ThreadContextTTD* ttdThreadContext)
    {
        TTDAssert(this->m_activeTTDBP.HasValue(), "We should always have this set when we complete the active segment scan.");

        this->m_hitContinueSearchBP = false;
        this->ClearActiveBPInfo(ttdThreadContext);
    }

    void ExecutionInfoManager::SetActiveBP_Helper(ThreadContextTTD* ttdThreadContext)
    {
        Js::ScriptContext* ctx = ttdThreadContext->LookupContextForScriptId(this->m_activeTTDBP.GetScriptLogTagId());
        Js::FunctionBody* body = this->m_activeTTDBP.LoadFunctionBodyIfPossible(ctx);
        if(body == nullptr)
        {
            return;
        }

        Js::Utf8SourceInfo* utf8SourceInfo = body->GetUtf8SourceInfo();
        Js::DebugDocument* debugDocument = utf8SourceInfo->GetDebugDocument();
        TTDAssert(debugDocument != nullptr, "Something went wrong with our TTD debug breakpoints.");

        utf8SourceInfo->EnsureLineOffsetCacheNoThrow();

        charcount_t charPosition = 0;
        charcount_t byteOffset = 0;
        utf8SourceInfo->GetCharPositionForLineInfo(this->m_activeTTDBP.GetSourceLine(), &charPosition, &byteOffset);
        long ibos = charPosition + this->m_activeTTDBP.GetSourceColumn() + 1;

        Js::StatementLocation statement;
        debugDocument->GetStatementLocation(ibos, &statement);

        // Don't see a use case for supporting multiple breakpoints at same location.
        // If a breakpoint already exists, just return that
        Js::BreakpointProbe* probe = debugDocument->FindBreakpoint(statement);

        if(probe == nullptr)
        {
            this->m_shouldRemoveWhenDone = true;
            probe = debugDocument->SetBreakPoint(statement, BREAKPOINT_ENABLED);
        }

        this->m_activeBPId = probe->GetId();
    }

    void ExecutionInfoManager::SetActiveBPInfoAsNeeded(ThreadContextTTD* ttdThreadContext)
    {
        if(this->m_pendingTTDBP.HasValue())
        {
            this->m_activeTTDBP.SetLocationCopy(this->m_pendingTTDBP);
            this->SetActiveBP_Helper(ttdThreadContext);

            //Finally clear the pending BP info so we don't get confused later
            this->m_pendingTTDMoveMode = 0;
            this->m_pendingTTDBP.Clear();
        }
    }

    void ExecutionInfoManager::ClearActiveBPInfo(ThreadContextTTD* ttdThreadContext)
    {
        TTDAssert(this->m_activeTTDBP.HasValue(), "Need to check that we actually have info to clear first.");

        Js::ScriptContext* ctx = ttdThreadContext->LookupContextForScriptId(this->m_activeTTDBP.GetScriptLogTagId());
        Js::FunctionBody* body = this->m_activeTTDBP.LoadFunctionBodyIfPossible(ctx);

        Js::DebugDocument* debugDocument = body->GetUtf8SourceInfo()->GetDebugDocument();
        Js::StatementLocation statement;
        if(this->m_shouldRemoveWhenDone && debugDocument->FindBPStatementLocation((UINT)this->m_activeBPId, &statement))
        {
            debugDocument->SetBreakPoint(statement, BREAKPOINT_DELETED);
        }

        this->m_activeBPId = -1;
        this->m_shouldRemoveWhenDone = false;
        this->m_activeTTDBP.Clear();
    }

    void ExecutionInfoManager::ProcessScriptLoad(Js::ScriptContext* ctx, uint32 topLevelBodyId, Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* se)
    {
        bool newScript = !this->m_debuggerNotifiedTopLevelBodies.Contains(topLevelBodyId);

        //Only notify of loaded script -- eval and new script is only notified later if we need to hit a bp in it?
        if(se != nullptr)
        {
            ctx->TTDHostCallbackFunctor.pfOnScriptLoadCallback(ctx->TTDHostCallbackFunctor.HostContext, body, utf8SourceInfo, se, newScript);
        }

        if(newScript)
        {
            this->m_debuggerNotifiedTopLevelBodies.AddNew(topLevelBodyId);
        }

        this->AddPreservedBPsForTopLevelLoad(topLevelBodyId, body);
    }

    void ExecutionInfoManager::ProcessScriptLoad_InflateReuseBody(uint32 topLevelBodyId, Js::FunctionBody* body)
    {
        this->AddPreservedBPsForTopLevelLoad(topLevelBodyId, body);
    }
}

#endif
