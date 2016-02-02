//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //A class to ensure that even when exceptions are thrown the pop action for the TTD call stack is executed
    class TTDExceptionFramePopper
    {
    private:
        EventLog* m_log;

    public:
        TTDExceptionFramePopper();
        ~TTDExceptionFramePopper();

        void PushInfo(EventLog* log);
        void PopInfo();
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDRecordExternalFunctionCallActionPopper
    {
    private:
        EventLog* m_log;
        Js::ScriptContext* m_scriptContext;
        Js::HiResTimer m_timer;
        double m_startTime;

        ExternalCallEventBeginLogEntry* m_callAction;

    public:
        TTDRecordExternalFunctionCallActionPopper(EventLog* log, Js::ScriptContext* scriptContext);
        ~TTDRecordExternalFunctionCallActionPopper();

        void NormalReturn(bool checkException, Js::Var returnValue);

        void SetCallAction(ExternalCallEventBeginLogEntry* action);
        double GetStartTime();
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDRecordJsRTFunctionCallActionPopper
    {
    private:
        EventLog* m_log;
        Js::ScriptContext* m_scriptContext;
        Js::HiResTimer m_timer;
        double m_startTime;

        JsRTCallFunctionBeginAction* m_callAction;

    public:
        TTDRecordJsRTFunctionCallActionPopper(EventLog* log, Js::ScriptContext* scriptContext);
        ~TTDRecordJsRTFunctionCallActionPopper();

        void NormalReturn();

        void SetCallAction(JsRTCallFunctionBeginAction* action);
        double GetStartTime();
    };

    //A struct for tracking time events in a single method
    struct SingleCallCounter
    {
        Js::FunctionBody* Function;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        LPCWSTR Name; //only added for debugging can get rid of later.
#endif

        uint64 EventTime; //The event time when the function was called
        uint64 FunctionTime; //The function time when the function was called
        uint64 LoopTime; //The current loop taken time for the function

#if ENABLE_TTD_DEBUGGING
        int32 LastStatementIndex; //The previously executed statement
        uint64 LastStatementLoopTime; //The previously executed statement

        int32 CurrentStatementIndex; //The currently executing statement
        uint64 CurrentStatementLoopTime; //The currently executing statement

        //bytecode range of the current stmt
        uint32 CurrentStatementBytecodeMin;
        uint32 CurrentStatementBytecodeMax;
#endif
    };

    //A class that represents the event log for the program execution
    class EventLog
    {
    private:
        ThreadContext* m_threadContext;
        SlabAllocator m_slabAllocator;

        //The root directory that the log info gets stored into
        LPCWSTR m_logInfoRootDir;

        //The global event time variable
        int64 m_eventTimeCtr;

        //A counter (per event dispatch) which holds the current value for the function counter
        uint64 m_runningFunctionTimeCtr;

        //Top-Level callback event time (or -1 if we are not in a callback)
        int64 m_topLevelCallbackEventTime;

        //The tag (from the host) that tells us which callback id this (toplevel) callback is associated with (-1 if not initiated by a callback)
        int64 m_hostCallbackId;

        //The list of all the events
        EventLogEntry* m_events;

        //The current position in the log (null if we are recording)
        EventLogEntry* m_currentEvent;

        //Array of call counters (used as stack)
        JsUtil::List<SingleCallCounter, HeapAllocator> m_callStack;

        //The current mode the system is running in (and a stack of mode push/pops that we use to generate it)
        JsUtil::List<TTDMode, HeapAllocator> m_modeStack;
        TTDMode m_currentMode;

        //A list of contexts that are being run in TTD mode -- We assume the host creates a single context for now 
        Js::ScriptContext* m_ttdContext;

        //The snapshot extractor that this log uses
        SnapshotExtractor m_snapExtractor;

        //The execution time that has elapsed since the last snapshot
        double m_elapsedExecutionTimeSinceSnapshot;

        //If we are inflating a snapshot multiple times we want to re-use the inflated objects when possible so keep this recent info
        int64 m_lastInflateSnapshotTime;
        InflateMap* m_lastInflateMap;

        //Pin set of all property records created during this logging session
        ReferencePinSet* m_propertyRecordPinSet;
        UnorderedArrayList<NSSnapType::SnapPropertyRecord, TTD_ARRAY_LIST_SIZE_DEFAULT> m_propertyRecordList;

#if ENABLE_TTD_DEBUGGING
        //The most recently executed statement before return -- normal return or exception
        //We clear this after executing any following statements so this can be used for:
        // - Step back to uncaught exception
        // - Step to last statement in previous event
        // - Step back *into* if either of the flags are true
        bool m_isReturnFrame;
        bool m_isExceptionFrame;
        SingleCallCounter m_lastFrame;
#endif

        ////
        //Helper methods

        //get the top call counter from the stack
        const SingleCallCounter& GetTopCallCounter() const;
        SingleCallCounter& GetTopCallCounter();

        //get the caller for the top call counter from the stack (e.g. stack -2)
        const SingleCallCounter& GetTopCallCallerCounter() const;

        //Get the current XTTDEventTime and advance the event time counter
        int64 GetCurrentEventTimeAndAdvance();

        //Advance the time and event position for replay
        void AdvanceTimeAndPositionForReplay();

        //insert the event at the head of the events list
        void InsertEventAtHead(EventLogEntry* evnt);

        //Look at the stack to get the new computed mode
        void UpdateComputedMode();

        //Unload any pinned or otherwise retained objects
        void UnloadRetainedData();

        //A helper for extracting snapshots
        void DoSnapshotExtract_Helper(bool firstSnap, SnapShot** snap, TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag);

    public:
        EventLog(ThreadContext* threadContext, LPCWSTR logDir);
        ~EventLog();

        //Initialize the log so that it is ready to perform TTD (record or replay) and set into the correct global mode
        void InitForTTDRecord();
        void InitForTTDReplay();

        //Add/remove script contexts from time travel
        void StartTimeTravelOnScript(Js::ScriptContext* ctx);
        void StopTimeTravelOnScript(Js::ScriptContext* ctx);

        //reset the bottom (global) mode with the specific value
        void SetGlobalMode(TTDMode m);

        //push a new debugger mode 
        void PushMode(TTDMode m);

        //pop the top debugger mode
        void PopMode(TTDMode m);

        //Set the log into debugging mode (it must already be in reaply mode
        void SetIntoDebuggingMode();

        //Use this to check specifically if we are in record AND this code is being run on behalf of the user application
        bool ShouldPerformRecordAction() const;

        //Use this to check specifically if we are in debugging mode (which is a superset of replay mode) AND this code is being run on behalf of the user application
        bool ShouldPerformDebugAction() const;

        //Use this to check if we should tag values that are passing to/from the JsRT host
        bool ShouldTagForJsRT() const;
        bool ShouldTagForExternalCall() const;

        //Use this to check if the TTD has been set into record/replay mode (although we still need to check if we should do any record ro replay)
        bool IsTTDActive() const;

        //Use this to check if the TTD has been detached (e.g., has traced a context execution and has now been detached)
        bool IsTTDDetached() const;

        //Add a property record to our pin set
        void AddPropertyRecord(const Js::PropertyRecord* record);

        ////////////////////////////////
        //Logging support

        //Log a time that is fetched during date operations
        void RecordDateTimeEvent(double time);

        //Log a time (as a string) that is fetched during date operations
        void RecordDateStringEvent(Js::JavascriptString* stringValue);

        //replay date event (which should be the current event)
        void ReplayDateTimeEvent(double* result);

        //replay date event with a string result (which should be the current event)
        void ReplayDateStringEvent(Js::ScriptContext* ctx, Js::JavascriptString** result);

        //Log a random seed value that is being generated using external entropy
        void RecordExternalEntropyRandomEvent(uint64 seed0, uint64 seed1);

        //Replay a random seed value that is being generated using external entropy
        void ReplayExternalEntropyRandomEvent(uint64* seed0, uint64* seed1);

        //Log property enumeration step
        void RecordPropertyEnumEvent(BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, Js::JavascriptString* propertyName);

        //Replay a property enumeration step
        void ReplayPropertyEnumEvent(BOOL* returnCode, int32* newIndex, const Js::DynamicObject* obj, Js::PropertyId* pid, Js::PropertyAttributes* attributes, Js::JavascriptString** propertyName);

        //Log symbol creation
        void RecordSymbolCreationEvent(Js::PropertyId pid);

        //Replay symbol creation
        void ReplaySymbolCreationEvent(Js::PropertyId* pid);

        //Log a value event for return from an external call
        ExternalCallEventBeginLogEntry* RecordExternalCallBeginEvent(Js::JavascriptFunction* func, int32 rootDepth, double beginTime);
        void RecordExternalCallEndEvent(int64 matchingBeginTime, int32 rootNestingDepth, bool hasScriptException, bool hasTerminatingException, double endTime, Js::Var value);

        ExternalCallEventBeginLogEntry* RecordPromiseRegisterBeginEvent(int32 rootDepth, double beginTime);
        void RecordPromiseRegisterEndEvent(int64 matchingBeginTime, int32 rootDepth, double endTime, Js::Var value);

        //replay an external return event (which should be the current event)
        void ReplayExternalCallEvent(Js::ScriptContext* ctx, Js::Var* result);

        //Log a function call
        void PushCallEvent(Js::FunctionBody* fbody);

        //Log a function return in normal case and exception
        void PopCallEvent(Js::FunctionBody* fbody, Js::Var result);
        void PopCallEventException(bool isFirstException);

#if ENABLE_TTD_DEBUGGING
        //To update the exception frame & last return frame info and access it in JSRT
        bool HasImmediateReturnFrame() const;
        bool HasImmediateExceptionFrame() const;
        const SingleCallCounter& GetImmediateReturnFrame() const;
        const SingleCallCounter& GetImmediateExceptionFrame() const;
        void ClearReturnAndExceptionFrames();
        void SetReturnAndExceptionFramesFromCurrent(bool setReturn, bool setException);
#endif

        //Update the loop count information
        void UpdateLoopCountInfo();

#if ENABLE_TTD_DEBUGGING
        //
        //TODO: This is not great performance wise
        //
        //For debugging we currently brute force track the current/last source statements executed
        bool UpdateCurrentStatementInfo(uint bytecodeOffset);

        //Get the current time/position info for the debugger -- all out arguments are optional (nullptr if you don't care)
        void GetTimeAndPositionForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the previous statement time/position for the debugger -- return false if this is the first statement of the event handler
        bool GetPreviousTimeAndPositionForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the last (uncaught or just caught) exception time/position for the debugger -- return true if the last return action was an exception and we have not made any additional calls
        bool GetExceptionTimeAndPositionForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the last statement in the just executed call time/position for the debugger -- return true if callerPreviousStmtIndex is the same as the stmt index for this (e.g. this is the immediately proceeding call)
        bool GetImmediateReturnTimeAndPositionForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const;

        //Get the current host callback id
        int64 GetCurrentHostCallbackId() const;

        //Get the current top-level event time 
        int64 GetCurrentTopLevelEventTime() const;

        //Get the time info around a host id creation/cancelation event -- return null if we can't find the event of interest (not in log or we were called directly by host -- host id == -1)
        JsRTCallbackAction* GetEventForHostCallbackId(bool wantRegisterOp, int64 hostIdOfInterest) const;

        //Get the event time corresponding to the k-th top-level event in the log
        int64 GetKthEventTime(uint32 k) const;

#if ENABLE_TTD_DEBUGGING_TEMP_WORKAROUND
        //The values that we have when we set breakpoints
        bool BPIsSet;
        int64 BPRootEventTime;
        uint64 BPFunctionTime;
        uint64 BPLoopTime;

        uint32 BPLine;
        uint32 BPColumn;
        uint32 BPSourceContextId;

        bool BPBreakAtNextStmtInto;
        int32 BPBreakAtNextStmtDepth;

        //set/clear a breakpoint for step to the next statement
        void ClearBreakpointOnNextStatement();
        void SetBreakpointOnNextStatement(bool into);

        //a callback function to invoke at breakpoints -- return true to continue normal execution false to abort toplevel with info
        TTDDbgCallback BPDbgCallback;

        //Print a variable on the global object
        void BPPrintBaseVariable(Js::ScriptContext* ctx, Js::Var var, bool expandObjects);
        void BPPrintVariable(Js::ScriptContext* ctx, LPCWSTR name);

        //Check if we hit a breakpoint and call the callback function
        void BPCheckAndAction(Js::ScriptContext* ctx);
#endif //end temp debugging workaround

#endif

        //Ensure the call stack is clear and counters are zeroed appropriately
        void ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime, int64 hostCallbackId);

        //Get/Increment the elapsed time since the last snapshot
        double GetElapsedSnapshotTime();
        void IncrementElapsedSnapshotTime(double addtlTime);

        ////////////////////////////////
        //Snapshot and replay support

        //Sometimes we need to abort replay and immediately return to the top-level host (debugger) so it can decide what to do next
        //    (1) If we are trying to replay something and we are at the end of the log then we need to terminate
        //    (2) If we are at a breakpoint and we want to step back (in some form) then we need to terminate
        void AbortReplayReturnToHost();

        //return true if we have taken the first snapshot and started recording
        bool HasDoneFirstSnapshot() const;

        //Do the snapshot extraction 
        void DoSnapshotExtract(bool firstSnap);

        //Take a ready-to-run snapshot for the event if needed 
        void DoRtrSnapIfNeeded();

        //Find the event time that has the snapshot we want to inflate from in order to replay to the requested target time
        //Return -1 if no such snapshot is available and set newCtxsNeed true if we want to inflate with "fresh" script contexts
        int64 FindSnapTimeForEventTime(int64 targetTime, bool* newCtxsNeeded);

        //If we decide to update with fresh contexts before the inflate then this will update the inflate map info in the log
        void UpdateInflateMapForFreshScriptContexts();

        //Do the inflation of the snapshot that is at the given event time
        void DoSnapshotInflate(int64 etime);

        //For replay the from the current event (should either be a top-level call/code-load action or a snapshot)
        void ReplaySingleEntry();

        //Run until the given top-level call event time
        void ReplayToTime(int64 eventTime);

        //For debugging replay the full trace from the current event
        void ReplayFullTrace();

        ////////////////////////////////
        //Host API record & replay support

        //Record allocate operations
        void RecordJsRTAllocateInt(Js::ScriptContext* ctx, uint32 ival);
        void RecordJsRTAllocateDouble(Js::ScriptContext* ctx, double dval);

        //Record conversions
        void RecordJsRTVarConversion(Js::ScriptContext* ctx, Js::Var var, bool toBool, bool toNumber, bool toString);

        //Record GetAndClearException
        void RecordGetAndClearException(Js::ScriptContext* ctx);

        //Record GetProperty
        void RecordGetProperty(Js::ScriptContext* ctx, Js::PropertyId pid, Js::Var var);

        //Record callback registration/cancelation
        void RecordJsRTCallbackOperation(Js::ScriptContext* ctx, bool isCancel, bool isRepeating, Js::JavascriptFunction* func, int64 createdCallbackId);

        //Record code parse
        void RecordCodeParse(Js::ScriptContext* ctx, bool isExpression, Js::JavascriptFunction* func, LPCWSTR srcCode, LPCWSTR sourceUri);

        //Record callback of an existing function
        JsRTCallFunctionBeginAction* RecordJsRTCallFunctionBegin(Js::ScriptContext* ctx, int32 rootDepth, int64 hostCallbackId, double beginTime, Js::JavascriptFunction* func, uint32 argCount, Js::Var* args);
        void RecordJsRTCallFunctionEnd(Js::ScriptContext* ctx, int64 matchingBeginTime, bool hasScriptException, bool hasTerminatingException, int32 callbackDepth, double endTime);

        //Replay a sequence of JsRT actions to get to the next event log item
        void ReplayActionLoopStep();

        ////////////////////////////////
        //Emit code and support

        void EmitLog();
        void ParseLogInto();
    };
}

#endif

