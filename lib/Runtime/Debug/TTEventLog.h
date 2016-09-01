//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

////
//Define compact macros for use in the JSRT API's
#if ENABLE_TTD
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) (CTX)->ShouldPerformRecordAction()

#define PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(CTX, ACTION_CODE) if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) { (ACTION_CODE); }
#define PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(CTX, ACTION_CODE) TTD::TTDVar* __ttd_resultPtr = nullptr; if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) { (ACTION_CODE); }
#define PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(RESULT) if((__ttd_resultPtr != nullptr) & ((RESULT) != nullptr)) { *__ttd_resultPtr = TTD_CONVERT_JSVAR_TO_TTDVAR(*(RESULT)); }

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX) if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX)) { AssertMsg(false, "Need to implement support here!!!"); }
#else
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) false

#define PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(CTX, ACTION_CODE) 
#define PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(CTX, ACTION_CODE) 
#define PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(RESULT) 

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX) 
#endif

////
//Begin the regular TTD code

#if ENABLE_TTD

#define TTD_EVENTLOG_LIST_BLOCK_SIZE 4096

namespace TTD
{
    //A class to ensure that even when exceptions are thrown the pop action for the TTD call stack is executed
    class TTDExceptionFramePopper
    {
    private:
        EventLog* m_log;
        Js::JavascriptFunction* m_function;

    public:
        TTDExceptionFramePopper();
        ~TTDExceptionFramePopper();

        void PushInfo(EventLog* log, Js::JavascriptFunction* function);
        void PopInfo();
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDRecordExternalFunctionCallActionPopper
    {
    private:
        Js::JavascriptFunction* m_function;
        NSLogEvents::EventLogEntry* m_callAction;

    public:
        TTDRecordExternalFunctionCallActionPopper(Js::JavascriptFunction* function, NSLogEvents::EventLogEntry* callAction);
        ~TTDRecordExternalFunctionCallActionPopper();

        void NormalReturn(bool checkException, Js::Var returnValue);
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDReplayExternalFunctionCallActionPopper
    {
    private:
        Js::JavascriptFunction* m_function;

    public:
        TTDReplayExternalFunctionCallActionPopper(Js::JavascriptFunction* function);
        ~TTDReplayExternalFunctionCallActionPopper();
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDRecordJsRTFunctionCallActionPopper
    {
    private:
        Js::ScriptContext* m_ctx;
        NSLogEvents::EventLogEntry* m_callAction;

    public:
        TTDRecordJsRTFunctionCallActionPopper(Js::ScriptContext* ctx, NSLogEvents::EventLogEntry* callAction);
        ~TTDRecordJsRTFunctionCallActionPopper();

        void NormalReturn(Js::Var returnValue);
    };

#if ENABLE_TTD_DEBUGGING
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
#endif

    //A list class for the events that we accumulate in the event log
    class TTEventList
    {
    public:
        struct TTEventListLink
        {
            //The current end of the allocated data in the block
            uint32 CurrPos;

            //The First index that holds data
            uint32 StartPos;

            //The actual block for the data
            NSLogEvents::EventLogEntry* BlockData;

            //The next block in the list
            TTEventListLink* Next;
            TTEventListLink* Previous;
        };

    private:
        //The the data in this
        TTEventListLink* m_headBlock;

        //the allocators we use for this work
        UnlinkableSlabAllocator* m_alloc;

        void AddArrayLink();
        void RemoveArrayLink(TTEventListLink* block);

    public:
        TTEventList(UnlinkableSlabAllocator* alloc);
        void UnloadEventList(NSLogEvents::EventLogEntryVTableEntry* vtable);

        //Add the entry to the list
        NSLogEvents::EventLogEntry* GetNextAvailableEntry();

        //Delete the entry from the list (must always be the first link/entry in the list)
        //This also calls unload on the entry
        void DeleteFirstEntry(TTEventListLink* block, NSLogEvents::EventLogEntry* data, NSLogEvents::EventLogEntryVTableEntry* vtable);

        //Return true if this is empty
        bool IsEmpty() const;

        //NOT constant time
        uint32 Count() const;

        class Iterator
        {
        private:
            TTEventListLink* m_currLink;
            uint32 m_currIdx;

        public:
            Iterator();
            Iterator(TTEventListLink* head, uint32 pos);

            const NSLogEvents::EventLogEntry* Current() const;
            NSLogEvents::EventLogEntry* Current();

            //Get the underlying block for deletion support
            TTEventListLink* GetBlock();

            bool IsValid() const;

            void MoveNext();
            void MovePrevious();
        };

        Iterator GetIteratorAtFirst() const;
        Iterator GetIteratorAtLast() const;
    };

    //A class that represents the event log for the program execution
    class EventLog
    {
    private:
        ThreadContext* m_threadContext;

        //Allocator we use for all the events we see
        UnlinkableSlabAllocator m_eventSlabAllocator;

        //Allocator we use for all the property records
        SlabAllocator m_miscSlabAllocator;

        //The global event time variable and a high res timer we can use to extract some diagnostic timing info as we go
        int64 m_eventTimeCtr;

        //A high res timer we can use to extract some diagnostic timing info as we go
        TTDTimer m_timer;

        //A counter (per event dispatch) which holds the current value for the function counter
        uint64 m_runningFunctionTimeCtr;

        //Top-Level callback event time (or -1 if we are not in a callback)
        int64 m_topLevelCallbackEventTime;

        //The tag (from the host) that tells us which callback id this (toplevel) callback is associated with (-1 if not initiated by a callback)
        int64 m_hostCallbackId;

        //The list of all the events and the iterator we use during replay
        TTEventList m_eventList;
        NSLogEvents::EventLogEntryVTableEntry* m_eventListVTable;
        TTEventList::Iterator m_currentReplayEventIterator;

        //Array of call counters (used as stack)
        JsUtil::List<SingleCallCounter, HeapAllocator> m_callStack;

        //The current mode the system is running in (and a stack of mode push/pops that we use to generate it)
        JsUtil::List<TTDMode, HeapAllocator> m_modeStack;
        TTDMode m_currentMode;

        //A list of contexts that are being run in TTD mode (and the associated callback functors) -- We assume the host creates a single context for now 
        Js::ScriptContext* m_ttdContext;

        //The snapshot extractor that this log uses
        SnapshotExtractor m_snapExtractor;

        //The execution time that has elapsed since the last snapshot
        double m_elapsedExecutionTimeSinceSnapshot;

        //If we are inflating a snapshot multiple times we want to re-use the inflated objects when possible so keep this recent info
        int64 m_lastInflateSnapshotTime;
        InflateMap* m_lastInflateMap;

        //Pin set of all property records created during this logging session
        PropertyRecordPinSet* m_propertyRecordPinSet;
        UnorderedArrayList<NSSnapType::SnapPropertyRecord, TTD_ARRAY_LIST_SIZE_DEFAULT> m_propertyRecordList;

        //A list of all *root* scripts that have been loaded during this session
        UnorderedArrayList<NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_MID> m_loadedTopLevelScripts;
        UnorderedArrayList<NSSnapValues::TopLevelNewFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_SMALL> m_newFunctionTopLevelScripts;
        UnorderedArrayList<NSSnapValues::TopLevelEvalFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_SMALL> m_evalTopLevelScripts;

#if ENABLE_TTD_DEBUGGING
        //The most recently executed statement before return -- normal return or exception
        //We clear this after executing any following statements so this can be used for:
        // - Step back to uncaught exception
        // - Step to last statement in previous event
        // - Step back *into* possible if either case is true

        TTLastReturnLocationInfo m_lastReturnLocation;
        TTLastReturnLocationInfo m_lastReturnLocationJMC;

        //A flag indicating if we want to break on the entry to the user code 
        bool m_breakOnFirstUserCode;

        //A pending TTDBP we want to set and move to
        TTDebuggerSourceLocation m_pendingTTDBP;

        //The bp we are actively moving to in TT mode
        int64 m_activeBPId;
        bool m_shouldRemoveWhenDone;
        TTDebuggerSourceLocation m_activeTTDBP;

        //A list of breakpoints seen in the most recent scan
        JsUtil::List<TTDebuggerSourceLocation, HeapAllocator> m_breakpointInfoList;

        //A list of breakpoints we want to preserve when performing TTD moves (even if we create a new script context)
        JsUtil::List<TTDebuggerSourceLocation, HeapAllocator> m_bpPreserveList;
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        TraceLogger m_diagnosticLogger;
#endif

        ////
        //Helper methods

        //get the top call counter from the stack
        const SingleCallCounter& GetTopCallCounter() const;
        SingleCallCounter& GetTopCallCounter();

#if ENABLE_TTD_DEBUGGING
        //Check if a function is in the JustMyCode set as determined by the host debugger
        static bool IsFunctionJustMyCode(const Js::FunctionBody* fbody);
        static bool IsFunctionJustMyCode(const Js::JavascriptFunction* function);

        //Return true if the debugger is running in Just My Code Mode
        static bool IsDebuggerRunningJustMyCode(Js::ScriptContext* ctx);

        //get the caller for the top call counter that is user code from the stack (e.g. stack -2)
        const SingleCallCounter* GetTopCallCallerCounter(bool justMyCode) const;
#endif

        //Get the current XTTDEventTime and advance the event time counter
        int64 GetCurrentEventTimeAndAdvance();

        //Advance the time and event position for replay
        void AdvanceTimeAndPositionForReplay();

        //Look at the stack to get the new computed mode
        void UpdateComputedMode();

        //Unload any pinned or otherwise retained objects
        void UnloadRetainedData();

        //A helper for extracting snapshots
        SnapShot* DoSnapshotExtract_Helper();

        //Replay a snapshot event -- either just advance the event position or, if running diagnostics, take new snapshot and compare
        void ReplaySnapshotEvent();

        //Replay an event loop yield point event
        void ReplayEventLoopYieldPointEvent();

        //A helper for initializing and type casting EventLogEntry data for record
        template <typename T, NSLogEvents::EventKind tag>
        T* RecordGetInitializedEvent_Helper()
        {
            NSLogEvents::EventLogEntry* evt = this->m_eventList.GetNextAvailableEntry();
            NSLogEvents::EventLogEntry_Initialize(evt, tag, this->GetCurrentEventTimeAndAdvance());

            return NSLogEvents::GetInlineEventDataAs<T, tag>(evt);
        }

        template <typename T, NSLogEvents::EventKind tag>
        T* RecordGetInitializedEvent_HelperWithMainEvent(NSLogEvents::EventLogEntry** evt)
        {
            *evt = this->m_eventList.GetNextAvailableEntry();
            NSLogEvents::EventLogEntry_Initialize(*evt, tag, this->GetCurrentEventTimeAndAdvance());

            return NSLogEvents::GetInlineEventDataAs<T, tag>(*evt);
        }

        template <typename T, NSLogEvents::EventKind tag>
        T* RecordGetInitializedEvent_HelperWithResultPtr(TTDVar** resultPtr)
        {
            NSLogEvents::EventLogEntry* evt = this->m_eventList.GetNextAvailableEntry();
            NSLogEvents::EventLogEntry_Initialize(evt, tag, this->GetCurrentEventTimeAndAdvance());

            T* eventData = NSLogEvents::GetInlineEventDataAs<T, tag>(evt);
            *resultPtr = &(eventData->Result);

            return eventData;
        }

        //Sometimes we need to abort replay and immediately return to the top-level host (debugger) so it can decide what to do next
        //    (1) If we are trying to replay something and we are at the end of the log then we need to terminate
        //    (2) If we are at a breakpoint and we want to step back (in some form) then we need to terminate
        void AbortReplayReturnToHost();

        //A helper for getting and doing some iterator manipulation during replay
        template <typename T, NSLogEvents::EventKind tag>
        const T* ReplayGetReplayEvent_Helper()
        {
            if(!this->m_currentReplayEventIterator.IsValid())
            {
                this->AbortReplayReturnToHost();
            }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            AssertMsg(this->m_currentReplayEventIterator.Current()->EventTimeStamp == this->m_eventTimeCtr, "Out of Sync!!!");
#endif

            const NSLogEvents::EventLogEntry* evt = this->m_currentReplayEventIterator.Current();

            this->AdvanceTimeAndPositionForReplay();

            return NSLogEvents::GetInlineEventDataAs<T, tag>(evt);
        }

        //Initialize the vtable for the event list data
        void InitializeEventListVTable();

    public:
        EventLog(ThreadContext* threadContext);
        ~EventLog();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        //Get the trace logger for this 
        TraceLogger* GetTraceLogger();
#endif

        //Initialize the log so that it is ready to perform TTD (record or replay) and set into the correct global mode
        void InitForTTDRecord();
        void InitForTTDReplay();

        //Add/remove script contexts from time travel
        void StartTimeTravelOnScript(Js::ScriptContext* ctx, const HostScriptContextCallbackFunctor& callbackFunctor);
        void StopTimeTravelOnScript(Js::ScriptContext* ctx);

        //reset the bottom (global) mode with the specific value
        void SetGlobalMode(TTDMode m);

        //push a new debugger mode 
        void PushMode(TTDMode m);

        //pop the top debugger mode
        void PopMode(TTDMode m);

        //Set the log into debugging mode
        void SetIntoDebuggingMode();

        //Use this to check specifically if we are in record AND this code is being run on behalf of the user application when doing symbol creation
        bool ShouldPerformRecordAction_SymbolCreation() const
        {
            //return true if RecordEnabled and ~ExcludedExecution
            return (this->m_currentMode & TTD::TTDMode::TTDShouldRecordActionMask) == TTD::TTDMode::RecordEnabled;
        }

        //Use this to check specifically if we are in debugging mode AND this code is being run on behalf of the user application when doing symbol creation
        bool ShouldPerformDebugAction_SymbolCreation() const
        {
#if ENABLE_TTD_DEBUGGING
            //return true if DebuggingEnabled and ~ExcludedExecution
            return (this->m_currentMode & TTD::TTDMode::TTDShouldDebugActionMask) == TTD::TTDMode::DebuggingEnabled;

#else
            return false;
#endif
        }

        //Use this to check specifically if we are in debugging mode AND this code is being run on behalf of the user application when doing BP actions
        bool ShouldPerformDebugAction_BreakPointAction() const
        {
#if ENABLE_TTD_DEBUGGING
            //return true if DebuggingEnabled and ~ExcludedExecution
            return (this->m_currentMode & TTD::TTDMode::TTDShouldDebugActionMask) == TTD::TTDMode::TTDShouldDebugActionMask;

#else
            return false;
#endif
        }

        //Add a property record to our pin set
        void AddPropertyRecord(const Js::PropertyRecord* record);

        //Add top level function load info to our sets
        const NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* AddScriptLoad(Js::FunctionBody* fb, Js::ModuleID moduleId, DWORD_PTR documentID, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag);
        const NSSnapValues::TopLevelNewFunctionBodyResolveInfo* AddNewFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen);
        const NSSnapValues::TopLevelEvalFunctionBodyResolveInfo* AddEvalFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode);

        void RecordTopLevelCodeAction(uint64 bodyCtrId);
        uint64 ReplayTopLevelCodeAction();

        ////////////////////////////////
        //Logging support

        //Log an event generated by user telemetry
        void RecordTelemetryLogEvent(Js::JavascriptString* infoStringJs, bool doPrint);

        //Replay a user telemetry event
        void ReplayTelemetryLogEvent(Js::JavascriptString* infoStringJs);

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
        void ReplayPropertyEnumEvent(BOOL* returnCode, Js::BigPropertyIndex* newIndex, const Js::DynamicObject* obj, Js::PropertyId* pid, Js::PropertyAttributes* attributes, Js::JavascriptString** propertyName);

        //Log symbol creation
        void RecordSymbolCreationEvent(Js::PropertyId pid);

        //Replay symbol creation
        void ReplaySymbolCreationEvent(Js::PropertyId* pid);

        //Log a value event for return from an external call
        NSLogEvents::EventLogEntry* RecordExternalCallEvent(Js::JavascriptFunction* func, int32 rootDepth, uint32 argc, Js::Var* argv);
        void RecordExternalCallEvent_Complete(NSLogEvents::EventLogEntry* evt, Js::JavascriptFunction* func, bool normalReturn, bool checkException, Js::Var result);

        //replay an external return event (which should be the current event)
        void ReplayExternalCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, Js::Var* result);

        NSLogEvents::EventLogEntry* RecordEnqueueTaskEvent(Js::Var taskVar);
        void RecordEnqueueTaskEvent_Complete(NSLogEvents::EventLogEntry* evt);

        void ReplayEnqueueTaskEvent(Js::ScriptContext* ctx, Js::Var taskVar);

        //Log a function call
        void PushCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool isInFinally);

        //Log a function return in normal case and exception
        void PopCallEvent(Js::JavascriptFunction* function, Js::Var result);
        void PopCallEventException(Js::JavascriptFunction* function);

#if ENABLE_TTD_DEBUGGING
        void ClearExceptionFrames();

        //Set that we want to break on the execution of the first user code
        void SetBreakOnFirstUserCode();

        bool HasPendingTTDBP() const;
        int64 GetPendingTTDBPTargetEventTime() const;
        void GetPendingTTDBPInfo(TTDebuggerSourceLocation& BPLocation) const;
        void ClearPendingTTDBPInfo();
        void SetPendingTTDBPInfo(const TTDebuggerSourceLocation& BPLocation);

        bool HasActiveBP() const;
        UINT GetActiveBPId() const;
        void ClearActiveBP();
        void SetActiveBP(UINT bpId, bool isNewBP, const TTDebuggerSourceLocation& bpLocation);

        //Process the breakpoint info as we enter a break statement and return true if we actually want to break
        bool ProcessBPInfoPreBreak(Js::FunctionBody* fb);

        //Process the breakpoint info as we resume from a break statement
        void ProcessBPInfoPostBreak(Js::FunctionBody* fb);

        //Clear the BP scan list
        void ClearBPScanList();

        //When scanning add the current location as a BP location
        void AddCurrentLocationDuringScan();

        //After a scan set the pending BP to the earliest breakpoint before the given current pending BP location and return true
        //If no such BP location then return false
        bool TryFindAndSetPreviousBP();

        //Load and restore all the breakpoints in the manager before and after we create new script contexts
        void LoadBPListForContextRecreate();
        void EventLog::UnLoadBPListAfterMoveForContextRecreate();
        const JsUtil::List<TTDebuggerSourceLocation, HeapAllocator>& EventLog::GetRestoreBPListAfterContextRecreate();
#endif

        //Update the loop count information
        void UpdateLoopCountInfo();

#if ENABLE_TTD_STACK_STMTS
        //
        //TODO: This is not great performance wise
        //
        //For debugging we currently brute force track the current/last source statements executed
        void UpdateCurrentStatementInfo(uint bytecodeOffset);

        //Get the current time/position info for the debugger -- all out arguments are optional (nullptr if you don't care)
        void GetTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;
#endif

#if ENABLE_OBJECT_SOURCE_TRACKING
        void GetTimeAndPositionForDiagnosticObjectTracking(DiagnosticOrigin& originInfo) const;
#endif 

#if ENABLE_TTD_DEBUGGING
        //Get the previous statement time/position for the debugger -- return false if this is the first statement of the event handler
        bool GetPreviousTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const;

        //Get the last (uncaught or just caught) exception time/position for the debugger -- if the last return action was an exception and we have not made any additional calls
        //Otherwise get the last statement executed call time/position for the debugger
        void GetLastExecutedTimeAndPositionForDebugger(bool* markedAsJustMyCode, TTDebuggerSourceLocation& sourceLocation) const;

        //Get the current host callback id
        int64 GetCurrentHostCallbackId() const;

        //Get the current top-level event time 
        int64 GetCurrentTopLevelEventTime() const;

        //Get the time info around a host id creation/cancelation event -- return null if we can't find the event of interest (not in log or we were called directly by host -- host id == -1)
        const NSLogEvents::JsRTCallbackAction* GetEventForHostCallbackId(bool wantRegisterOp, int64 hostIdOfInterest) const;

        //Get the event time corresponding to the first/last/k-th top-level event in the log
        int64 GetFirstEventTime(bool justMyCode) const;
        int64 GetLastEventTime(bool justMyCode) const;
        int64 GetKthEventTime(uint32 k) const;
#endif

        //Ensure the call stack is clear and counters are zeroed appropriately
        void ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime);

        //Check if we want to take a snapshot
        bool IsTimeForSnapshot() const;

        //After a snapshot we may want to discard old events so do that in here as needed
        void PruneLogLength();

        //Get/Increment the elapsed time since the last snapshot
        void IncrementElapsedSnapshotTime(double addtlTime);

        ////////////////////////////////
        //Snapshot and replay support

        //Do the snapshot extraction 
        void DoSnapshotExtract();

        //Take a ready-to-run snapshot for the event if needed 
        void DoRtrSnapIfNeeded();

        //Find the event time that has the snapshot we want to inflate from in order to replay to the requested target time
        //Return -1 if no such snapshot is available and set newCtxsNeed true if we want to inflate with "fresh" script contexts
        int64 FindSnapTimeForEventTime(int64 targetTime, bool allowRTR, bool* newCtxsNeeded, int64* optEndSnapTime);

        //If we decide to update with fresh contexts before the inflate then this will update the inflate map info in the log
        //Return true if we can delete the old script contexts as well
        bool UpdateInflateMapForFreshScriptContexts();

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

        //Return true if this is a propertyRecord reference
        bool IsPropertyRecordRef(void* ref) const;

        //Get the current time from the hi res timer
        double GetCurrentWallTime();

        //Get the most recently assigned event time value
        int64 GetLastEventTime() const;

#if !INT32VAR
        void RecordJsRTCreateInteger(Js::ScriptContext* ctx, int value, TTDVar** resultVarPtr);
#endif

        //Record creation operations
        void RecordJsRTCreateNumber(Js::ScriptContext* ctx, double value, TTDVar** resultVarPtr);
        void RecordJsRTCreateBoolean(Js::ScriptContext* ctx, bool value, TTDVar** resultVarPtr);
        void RecordJsRTCreateString(Js::ScriptContext* ctx, const char16* stringValue, size_t stringLength, TTDVar** resultVarPtr);
        void RecordJsRTCreateSymbol(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);

        //Record error creation
        void RecordJsRTCreateError(Js::ScriptContext* ctx, Js::Var msg, TTDVar** resultVarPtr);
        void RecordJsRTCreateRangeError(Js::ScriptContext* ctx, Js::Var vmsg, TTDVar** resultVarPtr);
        void RecordJsRTCreateReferenceError(Js::ScriptContext* ctx, Js::Var msg, TTDVar** resultVarPtr);
        void RecordJsRTCreateSyntaxError(Js::ScriptContext* ctx, Js::Var msg, TTDVar** resultVarPtr);
        void RecordJsRTCreateTypeError(Js::ScriptContext* ctx, Js::Var msg, TTDVar** resultVarPtr);
        void RecordJsRTCreateURIError(Js::ScriptContext* ctx, Js::Var msg, TTDVar** resultVarPtr);

        //Record conversions
        void RecordJsRTVarToNumberConversion(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTVarToBooleanConversion(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTVarToStringConversion(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTVarToObjectConversion(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);

        //Record lifetime management events
        void RecordJsRTAddRootRef(Js::ScriptContext* ctx, Js::Var var);
        void RecordJsRTRemoveRootRef(Js::ScriptContext* ctx, Js::Var var);
        void RecordJsRTEventLoopYieldPoint(Js::ScriptContext* ctx);

        //Record object allocate operations
        void RecordJsRTAllocateBasicObject(Js::ScriptContext* ctx, TTDVar** resultVarPtr);
        void RecordJsRTAllocateExternalObject(Js::ScriptContext* ctx, TTDVar** resultVarPtr);
        void RecordJsRTAllocateBasicArray(Js::ScriptContext* ctx, uint32 length, TTDVar** resultVarPtr);
        void RecordJsRTAllocateArrayBuffer(Js::ScriptContext* ctx, uint32 size, TTDVar** resultVarPtr);
        void RecordJsRTAllocateExternalArrayBuffer(Js::ScriptContext* ctx, byte* buff, uint32 size, TTDVar** resultVarPtr);
        void RecordJsRTAllocateFunction(Js::ScriptContext* ctx, bool isNamed, Js::Var optName, TTDVar** resultVarPtr);

        //Record GetAndClearException
        void RecordJsRTHostExitProcess(Js::ScriptContext* ctx, int32 exitCode);
        void RecordJsRTGetAndClearException(Js::ScriptContext* ctx, TTDVar** resultVarPtr);

        //Record Object Getters
        void RecordJsRTGetProperty(Js::ScriptContext* ctx, Js::PropertyId pid, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTGetIndex(Js::ScriptContext* ctx, Js::Var index, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTGetOwnPropertyInfo(Js::ScriptContext* ctx, Js::PropertyId pid, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTGetOwnPropertyNamesInfo(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);
        void RecordJsRTGetOwnPropertySymbolsInfo(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);

        //Record Object Setters
        void RecordJsRTDefineProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, Js::Var propertyDescriptor);
        void RecordJsRTDeleteProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, bool useStrictRules, TTDVar** resultVarPtr);
        void RecordJsRTSetPrototype(Js::ScriptContext* ctx, Js::Var var, Js::Var proto);
        void RecordJsRTSetProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, Js::Var val, bool useStrictRules);
        void RecordJsRTSetIndex(Js::ScriptContext* ctx, Js::Var var, Js::Var index, Js::Var val);

        //Record a get info from a typed array
        void RecordJsRTGetTypedArrayInfo(Js::ScriptContext* ctx, Js::Var var, TTDVar** resultVarPtr);

        //Record various raw byte* from ArrayBuffer manipulations
        void RecordJsRTRawBufferCopySync(Js::ScriptContext* ctx, Js::Var dst, uint32 dstIndex, Js::Var src, uint32 srcIndex, uint32 length);
        void RecordJsRTRawBufferModifySync(Js::ScriptContext* ctx, Js::Var dst, uint32 index, uint32 count);
        Js::Var RecordJsRTRawBufferAsyncModificationRegister(Js::ScriptContext* ctx, Js::Var dst, byte* initialModPos);
        Js::Var RecordJsRTRawBufferAsyncModifyComplete(Js::ScriptContext* ctx, byte* finalModPos);

        //Record a constructor call from JsRT
        void RecordJsRTConstructCall(Js::ScriptContext* ctx, Js::JavascriptFunction* func, uint32 argCount, Js::Var* args, TTDVar** resultVarPtr);

        //Record callback registration/cancelation
        void RecordJsRTCallbackOperation(Js::ScriptContext* ctx, bool isCreate, bool isCancel, bool isRepeating, Js::JavascriptFunction* func, int64 callbackId);

        //Record code parse
        void RecordJsRTCodeParse(Js::ScriptContext* ctx, uint64 bodyCtrId, LoadScriptFlag loadFlag, Js::JavascriptFunction* func, bool isUft8, const byte* script, uint32 scriptByteLength, const char16* sourceUri, Js::JavascriptFunction* resultFunction);

        //Record callback of an existing function
        NSLogEvents::EventLogEntry* RecordJsRTCallFunction(Js::ScriptContext* ctx, int32 rootDepth, Js::JavascriptFunction* func, uint32 argCount, Js::Var* args);

        //Replay a sequence of JsRT actions until (and including) the one at eventTimeLimit
        void ReplayActionLoopRange(int64 eventTimeLimit);

        ////////////////////////////////
        //Emit code and support

        void EmitLogIfNeeded();
        void ParseLogInto();
    };
}

#endif

