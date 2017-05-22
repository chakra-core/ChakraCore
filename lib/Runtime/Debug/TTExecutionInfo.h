//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //An exception class for controlled aborts from the runtime to the toplevel TTD control loop
    class TTDebuggerAbortException
    {
    private:
        //An integer code to describe the reason for the abort -- 0 invalid, 1 end of log, 2 request etime move, 3 uncaught exception (propagate to top-level)
        const uint32 m_abortCode;

        //An optional target event time -- intent is interpreted based on the abort code
        const int64 m_optEventTime;

        //An optional move mode value -- should be built by host we just propagate it
        const int64 m_optMoveMode;

        //An optional -- and static string message to include
        const char16* m_staticAbortMessage;

        TTDebuggerAbortException(uint32 abortCode, int64 optEventTime, int64 optMoveMode, const char16* staticAbortMessage);

    public:
        ~TTDebuggerAbortException();

        static TTDebuggerAbortException CreateAbortEndOfLog(const char16* staticMessage);
        static TTDebuggerAbortException CreateTopLevelAbortRequest(int64 targetEventTime, int64 moveMode, const char16* staticMessage);
        static TTDebuggerAbortException CreateUncaughtExceptionAbortRequest(int64 targetEventTime, const char16* staticMessage);

        bool IsEndOfLog() const;
        bool IsEventTimeMove() const;
        bool IsTopLevelException() const;

        int64 GetTargetEventTime() const;
        int64 GetMoveMode() const;

        const char16* GetStaticAbortMessage() const;
    };

    //A struct for tracking time events in a single method
    struct SingleCallCounter
    {
        Js::FunctionBody* Function;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        const char16* Name; //only added for debugging can get rid of later.
#endif

        uint64 FunctionTime; //The function time when the function was called
        uint64 LoopTime; //The current loop taken time for the function

        int32 LastStatementIndex; //The previously executed statement
        uint64 LastStatementLoopTime; //The previously executed statement

        int32 CurrentStatementIndex; //The currently executing statement
        uint64 CurrentStatementLoopTime; //The currently executing statement

        //bytecode range of the current stmt
        uint32 CurrentStatementBytecodeMin;
        uint32 CurrentStatementBytecodeMax;
    };

    //A class to represent a source location
    class TTDebuggerSourceLocation
    {
    private:
        //The script context logid the breakpoint goes with
        TTD_LOG_PTR_ID m_sourceScriptLogId;

        //The id of the breakpoint (or -1 if no id is associated)
        int64 m_bpId;

        //The time aware parts of this location
        int64 m_etime;  //-1 indicates an INVALID location
        int64 m_ftime;  //-1 indicates any ftime is OK
        int64 m_ltime;  //-1 indicates any ltime is OK

        //The top-level body this source location is contained in
        uint32 m_topLevelBodyId;

        //The position of the function in the document
        uint32 m_functionLine;
        uint32 m_functionColumn;

        //The location in the fnuction
        uint32 m_line;
        uint32 m_column;

        //Update the specific body of this location from the root body and line number info
        Js::FunctionBody* UpdatePostInflateFunctionBody_Helper(Js::FunctionBody* rootBody) const;

    public:
        TTDebuggerSourceLocation();
        TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other);
        ~TTDebuggerSourceLocation();

        TTDebuggerSourceLocation& operator= (const TTDebuggerSourceLocation& other);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void PrintToConsole() const;
#endif

        void Initialize();

        bool HasValue() const;
        bool HasTTDTimeValue() const;
        void Clear();

        void SetLocationCopy(const TTDebuggerSourceLocation& other);
        void SetLocationFromFrame(int64 topLevelETime, const SingleCallCounter& callFrame);
        void SetLocationFromFunctionEntryAnyTime(int64 topLevelETime, Js::FunctionBody* body);
        void SetLocationFull(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column);
        void SetLocationWithBP(int64 bpId, Js::FunctionBody* body, ULONG line, LONG column);

        int64 GetRootEventTime() const;
        int64 GetFunctionTime() const;
        int64 GetLoopTime() const;

        TTD_LOG_PTR_ID GetScriptLogTagId() const;
        Js::FunctionBody* LoadFunctionBodyIfPossible(Js::ScriptContext* ctx) const;

        int64 GetBPId() const;
        uint32 GetTopLevelBodyId() const;

        uint32 GetSourceLine() const;
        uint32 GetSourceColumn() const;

        //return true if the two locations refer to the same time/place
        static bool AreSameLocations(const TTDebuggerSourceLocation& sl1, const TTDebuggerSourceLocation& sl2);
        static bool AreSameLocations_PlaceOnly(const TTDebuggerSourceLocation& sl1, const TTDebuggerSourceLocation& sl2);
    };

    //A by value class representing the state of the last returned from location in execution (return x or exception)
    class TTLastReturnLocationInfo
    {
    private:
        bool m_isExceptionFrame;
        SingleCallCounter m_lastFrame;

    public:
        TTLastReturnLocationInfo();

        void SetReturnLocation(const SingleCallCounter& cframe);
        void SetExceptionLocation(const SingleCallCounter& cframe);

        bool IsDefined() const;
        bool IsReturnLocation() const;
        bool IsExceptionLocation() const;
        const SingleCallCounter& GetLocation() const;

        void Clear();
        void ClearReturnOnly();
        void ClearExceptionOnly();
    };

    //////////////////

    //This class manages exeuction info for our replay execution including
    //call-stack, current statements, breakpoints, loaded code, etc.
    class ExecutionInfoManager
    {
    private:
        //Stash the current top-level event time here
        int64 m_topLevelCallbackEventTime;

        //A counter (per event dispatch) which holds the current value for the function counter
        uint64 m_runningFunctionTimeCtr;

        //Array of call counters (used as stack)
        JsUtil::List<SingleCallCounter, HeapAllocator> m_callStack;

        //A set containing every top level body that we have notified the debugger of (so we don't tell them multiple times)
        JsUtil::BaseHashSet<uint32, HeapAllocator> m_debuggerNotifiedTopLevelBodies;

        //The most recently executed statement before return -- normal return or exception
        //We clear this after executing any following statements so this can be used for:
        // - Step back to uncaught exception
        // - Step to last statement in previous event
        // - Step back *into* possible if either case is true
        TTLastReturnLocationInfo m_lastReturnLocation;

        //Always keep the last exception location as well -- even if it is caught
        bool m_lastExceptionPropagating;
        TTDebuggerSourceLocation m_lastExceptionLocation; 

        //A flag indicating if we want to break on the entry to the user code 
        bool m_breakOnFirstUserCode;

        //A pending TTDBP we want to set and move to
        TTDebuggerSourceLocation m_pendingTTDBP;
        int64 m_pendingTTDMoveMode;

        //The bp we are actively moving to in TT mode
        int64 m_activeBPId;
        bool m_shouldRemoveWhenDone;
        TTDebuggerSourceLocation m_activeTTDBP;

        //The last breakpoint seen in the most recent scan
        bool m_hitContinueSearchBP;
        TTDebuggerSourceLocation m_continueBreakPoint;

        //Used to preserve breakpoints accross inflate operations
        JsUtil::List<TTDebuggerSourceLocation*, HeapAllocator> m_unRestoredBreakpoints;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        TraceLogger m_diagnosticLogger;
#endif

        //A special check to see if we are in the process of a time-travel move and do not want to stop at any breakpoints
        static bool ShouldSuppressBreakpointsForTimeTravelMove(TTDMode mode);

        //A special check to see if we are in the process of a time-travel move and do not want to stop at any breakpoints
        static bool ShouldRecordBreakpointsDuringTimeTravelScan(TTDMode mode);

    public:
        ExecutionInfoManager();
        ~ExecutionInfoManager();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        //Get the trace logger for this 
        TraceLogger* GetTraceLogger();
#endif

        //get the top call counter from the stack
        const SingleCallCounter& GetTopCallCounter() const;
        SingleCallCounter& GetTopCallCounter();

        //get the caller for the top call counter that is user code from the stack (e.g. stack -2)
        bool TryGetTopCallCallerCounter(SingleCallCounter& caller) const;

        //Log a function call
        void PushCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool isInFinally);

        //Log a function return in normal case and exception
        void PopCallEvent(Js::JavascriptFunction* function, Js::Var result);
        void PopCallEventException(Js::JavascriptFunction* function);

        void ProcessCatchInfoForLastExecutedStatements();

        //Set that we want to break on the execution of the first user code
        void SetBreakOnFirstUserCode();

        //Set the requested breakpoint and move mode in debugger stepping
        void SetPendingTTDStepBackMove();
        void SetPendingTTDStepBackIntoMove();
        void SetPendingTTDReverseContinueMove(uint64 moveflag);
        void SetPendingTTDUnhandledException();

        //Process the breakpoint info as we enter a break statement and return true if we actually want to break
        bool ProcessBPInfoPreBreak(Js::FunctionBody* fb, const EventLog* elog);

        //Process the breakpoint info as we resume from a break statement
        void ProcessBPInfoPostBreak(Js::FunctionBody* fb);

        //When scanning we don't want to add the active BP as a seen breakpoint *unless* is it is also explicitly added as a breakpoint by the user
        bool IsLocationActiveBPAndNotExplicitBP(const TTDebuggerSourceLocation& current) const;

        //When scanning add the current location as a BP location
        void AddCurrentLocationDuringScan(int64 topLevelEventTime);

        //After a scan set the pending BP to the earliest breakpoint before the given current pending BP location and return true
        //If no such BP location then return false
        bool TryFindAndSetPreviousBP();

        //Get the target event time for the pending TTD breakpoint
        int64 GetPendingTTDBPTargetEventTime() const;

        //Load and restore all the breakpoints in the manager before and after we create new script contexts  
        void LoadPreservedBPInfo(ThreadContext* threadContext);

        //When we load in a top level function we need to add any breakpoints associated with it
        void AddPreservedBPsForTopLevelLoad(uint32 topLevelBodyId, Js::FunctionBody* body);

        //Update the loop count information
        void UpdateLoopCountInfo();

        //
        //TODO: This is not great performance wise
        //
        //For debugging we currently brute force track the current/last source statements executed
        void UpdateCurrentStatementInfo(uint bytecodeOffset);

        //Get the current time/position info for the debugger -- all out arguments are optional (nullptr if you don't care)
        void GetTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;

#if ENABLE_OBJECT_SOURCE_TRACKING
        void GetTimeAndPositionForDiagnosticObjectTracking(DiagnosticOrigin& originInfo) const;
#endif 

        //Get the previous statement time/position for the debugger -- return false if this is the first statement of the event handler
        bool GetPreviousTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;

        //Get the last (uncaught or just caught) exception time/position for the debugger -- if the last return action was an exception and we have not made any additional calls
        //Otherwise get the last statement executed call time/position for the debugger
        void GetLastExecutedTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;
        void GetLastExceptionTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;

        //Ensure the call stack is clear and counters are zeroed appropriately
        void ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime);

        //Setup and tear down info on the bp location in the active interval the reverse continue was invoked from
        //We don't want to find a BP after the current statement so we need to track this
        void SetBPInfoForActiveSegmentContinueScan(ThreadContextTTD* ttdThreadContext);
        void ClearBPInfoForActiveSegmentContinueScan(ThreadContextTTD* ttdThreadContext);

        //Transfer the pending bp info to the active BP info (and set the BP) as needed to make sure BP is hit in reverse step operations
        void SetActiveBP_Helper(ThreadContextTTD* ttdThreadContext);
        void SetActiveBPInfoAsNeeded(ThreadContextTTD* ttdThreadContext);
        void ClearActiveBPInfo(ThreadContextTTD* ttdThreadContext);

        //After loading script process it to (1) notify the debugger if needed and (2) set any breakpoints we need
        void ProcessScriptLoad(Js::ScriptContext* ctx, uint32 topLevelBodyId, Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* se);
        void ProcessScriptLoad_InflateReuseBody(uint32 topLevelBodyId, Js::FunctionBody* body);
    };

    //A class to ensure that even when exceptions are thrown the pop action for the TTD call stack is executed -- defined after EventLog so we can refer to it in the .h file
    class TTDExceptionFramePopper
    {
    private:
        ExecutionInfoManager* m_eimanager;
        Js::JavascriptFunction* m_function;

    public:
        TTDExceptionFramePopper()
            : m_eimanager(nullptr), m_function(nullptr)
        {
            ;
        }

        ~TTDExceptionFramePopper()
        {
            //we didn't clear this so an exception was thrown and we are propagating
            if(this->m_eimanager != nullptr)
            {
                //if it doesn't have an exception frame then this is the frame where the exception was thrown so record our info
                this->m_eimanager->PopCallEventException(this->m_function);
            }
        }

        void PushInfo(ExecutionInfoManager* eimanager, Js::JavascriptFunction* function)
        {
            this->m_eimanager = eimanager; //set the log info so if the pop isn't called the destructor will record propagation
            this->m_function = function;
        }

        void PopInfo()
        {
            this->m_eimanager = nullptr; //normal pop (no exception) just clear so destructor nops
        }
    };
}

#endif
