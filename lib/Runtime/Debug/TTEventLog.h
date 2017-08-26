//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

#define TTD_EVENTLOG_LIST_BLOCK_SIZE 65536

namespace TTD
{
    //A class to ensure that even when exceptions are thrown we increment/decrement the root nesting depth
    class TTDNestingDepthAutoAdjuster
    {
    private:
        ThreadContext* m_threadContext;

    public:
        TTDNestingDepthAutoAdjuster(ThreadContext* threadContext)
            : m_threadContext(threadContext)
        {
            this->m_threadContext->TTDRootNestingCount++;
        }

        ~TTDNestingDepthAutoAdjuster() 
        {
            this->m_threadContext->TTDRootNestingCount--;
        }
    };

    //A class to manage the recording of JsRT action call state
    class TTDJsRTActionResultAutoRecorder
    {
    private:
        NSLogEvents::EventLogEntry* m_actionEvent;
        TTDVar* m_resultPtr;

    public:
        TTDJsRTActionResultAutoRecorder() 
            : m_actionEvent(nullptr), m_resultPtr(nullptr)
        {
            ;
        }

        void InitializeWithEventAndEnter(NSLogEvents::EventLogEntry* actionEvent)
        {
            TTDAssert(this->m_actionEvent == nullptr, "Don't double initialize");

            this->m_actionEvent = actionEvent;
        }

        void InitializeWithEventAndEnterWResult(NSLogEvents::EventLogEntry* actionEvent, TTDVar* resultPtr)
        {
            TTDAssert(this->m_actionEvent == nullptr, "Don't double initialize");

            this->m_actionEvent = actionEvent;

            this->m_resultPtr = resultPtr;
            *(this->m_resultPtr) = (TTDVar)nullptr; //set the result field to a default value in case we fail during execution
        }

        void SetResult(Js::Var* result)
        {
            TTDAssert(this->m_resultPtr != nullptr, "Why are we calling this then???");
            if(result != nullptr)
            {
                *(this->m_resultPtr) = TTD_CONVERT_JSVAR_TO_TTDVAR(*(result));
            }
        }

        void CompleteWithStatusCode(int32 exitStatus)
        {
            if(this->m_actionEvent != nullptr)
            {
                TTDAssert(this->m_actionEvent->ResultStatus == -1, "Hmm this got changed somewhere???");

                this->m_actionEvent->ResultStatus = exitStatus;
            }
        }
    };

    //A class to ensure that even when exceptions are thrown we record the time difference info
    class TTDJsRTFunctionCallActionPopperRecorder
    {
    private:
        Js::ScriptContext* m_ctx;
        double m_beginTime;
        NSLogEvents::EventLogEntry* m_callAction;

    public:
        TTDJsRTFunctionCallActionPopperRecorder();
        ~TTDJsRTFunctionCallActionPopperRecorder();

        void InitializeForRecording(Js::ScriptContext* ctx, double beginWallTime, NSLogEvents::EventLogEntry* callAction);
    };

    //A list class for the events that we accumulate in the event log
    class TTEventList
    {
    public:
        struct TTEventListLink
        {
            //The current end of the allocated data in the block
            size_t CurrPos;

            //The First index that holds data
            size_t StartPos;

            //The actual block for the data
            byte* BlockData;

            //The next block in the list
            TTEventListLink* Next;
            TTEventListLink* Previous;
        };

    private:
        //The the data in this
        TTEventListLink* m_headBlock;

        //the allocators we use for this work
        UnlinkableSlabAllocator* m_alloc;

        //The vtable with callbacks for the event log entries in the list
        const NSLogEvents::EventLogEntryVTableEntry* m_vtable;

        //Map from event entries to previous event entries -- only valid in replay mode otherwise empty
        JsUtil::BaseDictionary<const NSLogEvents::EventLogEntry*, size_t, HeapAllocator> m_previousEventMap;

        void AddArrayLink();
        void RemoveArrayLink(TTEventListLink* block);

    public:
        TTEventList(UnlinkableSlabAllocator* alloc);

        void SetVTable(const NSLogEvents::EventLogEntryVTableEntry* vtable);
        void InitializePreviousEventMap();

        void UnloadEventList();

        //Add the entry to the list
        template <typename T>
        NSLogEvents::EventLogEntry* GetNextAvailableEntry() 
        {
            const size_t esize = TTD_EVENT_PLUS_DATA_SIZE(T);
            if((this->m_headBlock == nullptr) || (this->m_headBlock->CurrPos + esize >= TTD_EVENTLOG_LIST_BLOCK_SIZE))
            {
                this->AddArrayLink();
            }

            NSLogEvents::EventLogEntry* entry = reinterpret_cast<NSLogEvents::EventLogEntry*>(this->m_headBlock->BlockData + this->m_headBlock->CurrPos);
            this->m_headBlock->CurrPos += esize;

            return entry;
        }

        //Add the entry to the list
        NSLogEvents::EventLogEntry* GetNextAvailableEntry(size_t requiredSize);

        //Delete the entry from the list (must always be the first link/entry in the list)
        //This also calls unload on the entry
        void DeleteFirstEntry(TTEventListLink* block, NSLogEvents::EventLogEntry* data);

        //Return true if this is empty
        bool IsEmpty() const;

        //NOT constant time
        uint32 Count() const;

        class Iterator
        {
        private:
            TTEventListLink* m_currLink;
            size_t m_currIdx;

            const NSLogEvents::EventLogEntryVTableEntry* m_vtable;
            const JsUtil::BaseDictionary<const NSLogEvents::EventLogEntry*, size_t, HeapAllocator>* m_previousEventMap;

        public:
            Iterator();
            Iterator(TTEventListLink* head, size_t pos, const NSLogEvents::EventLogEntryVTableEntry* vtable, const JsUtil::BaseDictionary<const NSLogEvents::EventLogEntry*, size_t, HeapAllocator>* previousEventMap);

            const NSLogEvents::EventLogEntry* Current() const;
            NSLogEvents::EventLogEntry* Current();

            //Get the underlying block for deletion support
            TTEventListLink* GetBlock();

            bool IsValid() const;

            void MoveNext();
            void MovePrevious_ReplayOnly();
        };

        Iterator GetIteratorAtFirst() const;
        Iterator GetIteratorAtLast_ReplayOnly() const;
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

        //Top-Level callback event time (or -1 if we are not in a callback)
        int64 m_topLevelCallbackEventTime;

        //The list of all the events and the iterator we use during replay
        NSLogEvents::EventLogEntryVTableEntry* m_eventListVTable;
        TTEventList m_eventList;
        TTEventList::Iterator m_currentReplayEventIterator;

        //The current mode the system is running in (and a stack of mode push/pops that we use to generate it)
        TTModeStack m_modeStack;
        TTDMode m_currentMode;

        //The snapshot extractor that this log uses
        SnapshotExtractor m_snapExtractor;

        //The execution time that has elapsed since the last snapshot
        double m_elapsedExecutionTimeSinceSnapshot;

        //If we are inflating a snapshot multiple times we want to re-use the inflated objects when possible so keep this recent info
        int64 m_lastInflateSnapshotTime;
        InflateMap* m_lastInflateMap;

        //Pin set of all property records created during this logging session
        RecyclerRootPtr<PropertyRecordPinSet> m_propertyRecordPinSet;
        UnorderedArrayList<NSSnapType::SnapPropertyRecord, TTD_ARRAY_LIST_SIZE_DEFAULT> m_propertyRecordList;

        //The value of the threadContext sourceInfoCounter in record -- in replay initialize to this value to avoid collisions
        uint32 m_sourceInfoCount;

        //A list of all *root* scripts that have been loaded during this session
        UnorderedArrayList<NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_MID> m_loadedTopLevelScripts;
        UnorderedArrayList<NSSnapValues::TopLevelNewFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_SMALL> m_newFunctionTopLevelScripts;
        UnorderedArrayList<NSSnapValues::TopLevelEvalFunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_SMALL> m_evalTopLevelScripts;

        ////
        //Helper methods

        //Get the current XTTDEventTime and advance the event time counter
        int64 GetCurrentEventTimeAndAdvance()
        {
            return this->m_eventTimeCtr++;
        }

        //Advance the time and event position for replay
        void AdvanceTimeAndPositionForReplay();

        //Look at the stack to get the new computed mode
        void UpdateComputedMode();

        //A helper for extracting snapshots
        SnapShot* DoSnapshotExtract_Helper(double gcTime, JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies);

        //Replay a snapshot event -- either just advance the event position or, if running diagnostics, take new snapshot and compare
        void ReplaySnapshotEvent();

        //Replay an event loop yield point event
        void ReplayEventLoopYieldPointEvent();

        template <typename T, NSLogEvents::EventKind tag>
        NSLogEvents::EventLogEntry* RecordGetInitializedEvent(T** extraData)
        {
            AssertMsg(TTD_EVENT_PLUS_DATA_SIZE_DIRECT(sizeof(T)) == this->m_eventListVTable[(uint32)tag].DataSize, "Computed and extracted data sizes dont' match!!!");

            NSLogEvents::EventLogEntry* res = this->m_eventList.GetNextAvailableEntry<T>();
            NSLogEvents::EventLogEntry_Initialize<tag>(res, this->GetCurrentEventTimeAndAdvance());

            *extraData = NSLogEvents::GetInlineEventDataAs<T, tag>(res);
            return res;
        }

        template <typename T, NSLogEvents::EventKind tag>
        T* RecordGetInitializedEvent_DataOnly()
        {
            AssertMsg(TTD_EVENT_PLUS_DATA_SIZE_DIRECT(sizeof(T)) == this->m_eventListVTable[(uint32)tag].DataSize, "Computed and extracted data sizes dont' match!!!");

            NSLogEvents::EventLogEntry* res = this->m_eventList.GetNextAvailableEntry<T>();
            NSLogEvents::EventLogEntry_Initialize<tag>(res, this->GetCurrentEventTimeAndAdvance());

            //For these operations are not allowed to fail so success is always 0
            res->ResultStatus = 0;

            return NSLogEvents::GetInlineEventDataAs<T, tag>(res);
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
            TTDAssert(this->m_currentReplayEventIterator.Current()->EventTimeStamp == this->m_eventTimeCtr, "Out of Sync!!!");
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

        //When we stop recording we want to unload all of the data in the log (otherwise we get strange transitions if we start again later)
        void UnloadAllLogData();

        //Initialize the log so that it is ready to perform TTD (record or replay) and set into the correct global mode
        void InitForTTDRecord();
        void InitForTTDReplay(TTDataIOInfo& iofp, const char* parseUri, size_t parseUriLength, bool debug);

        //reset the bottom (global) mode with the specific value
        void SetGlobalMode(TTDMode m);

        //Mark that a snapshot is in (or or is now complete) 
        void SetSnapshotOrInflateInProgress(bool flag);

        //push a new debugger mode 
        void PushMode(TTDMode m);

        //pop the top debugger mode
        void PopMode(TTDMode m);

        //Get the current mode for TTD execution
        TTDMode GetCurrentTTDMode() const;

        //Set the mode flags on the script context based on the TTDMode in the Log
        void SetModeFlagsOnContext(Js::ScriptContext* ctx);

        //Get the global mode flags for creating a script context 
        void GetModesForExplicitContextCreate(bool& inRecord, bool& activelyRecording, bool& inReplay);

        //Just check if the debug mode flag has been set (don't check any active or suppressed properties)
        bool IsDebugModeFlagSet() const;

        //A special check for to see if we want to push the supression flag for getter exection
        bool ShouldDoGetterInvocationSupression() const;

        //Add a property record to our pin set
        void AddPropertyRecord(const Js::PropertyRecord* record);

        //Add top level function load info to our sets
        const NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* AddScriptLoad(Js::FunctionBody* fb, Js::ModuleID moduleId, uint64 sourceContextId, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag);
        const NSSnapValues::TopLevelNewFunctionBodyResolveInfo* AddNewFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen);
        const NSSnapValues::TopLevelEvalFunctionBodyResolveInfo* AddEvalFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode);

        uint32 GetSourceInfoCount() const;

        void RecordTopLevelCodeAction(uint32 bodyCtrId);
        uint32 ReplayTopLevelCodeAction();

        ////////////////////////////////
        //Logging support

        //Log an event generated by user telemetry
        void RecordTelemetryLogEvent(Js::JavascriptString* infoStringJs, bool doPrint);

        //Replay a user telemetry event
        void ReplayTelemetryLogEvent(Js::JavascriptString* infoStringJs);

        //Log an event generated to write the log to a given uri
        void RecordEmitLogEvent(Js::JavascriptString* uriString);

        //Replay a event that writes the log to a given uri
        void ReplayEmitLogEvent();

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
        void ReplayPropertyEnumEvent(Js::ScriptContext* requestContext, BOOL* returnCode, Js::BigPropertyIndex* newIndex, const Js::DynamicObject* obj, Js::PropertyId* pid, Js::PropertyAttributes* attributes, Js::JavascriptString** propertyName);

        //Log symbol creation
        void RecordSymbolCreationEvent(Js::PropertyId pid);

        //Replay symbol creation
        void ReplaySymbolCreationEvent(Js::PropertyId* pid);

        //Log if a weak collection contained a value when an operation occours
        void RecordWeakCollectionContainsEvent(bool contains);

        //Replay a weak collection contained a value when an operation occours (return the truth value)
        bool ReplayWeakCollectionContainsEvent();

        //Log a value event for return from an external call
        NSLogEvents::EventLogEntry* RecordExternalCallEvent(Js::JavascriptFunction* func, int32 rootDepth, uint32 argc, Js::Var* argv, bool checkExceptions);
        void RecordExternalCallEvent_Complete(Js::JavascriptFunction* efunction, NSLogEvents::EventLogEntry* evt, Js::Var result);

        //replay an external return event (which should be the current event)
        void ReplayExternalCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, Js::Var* result);

        NSLogEvents::EventLogEntry* RecordEnqueueTaskEvent(Js::Var taskVar);
        void RecordEnqueueTaskEvent_Complete(NSLogEvents::EventLogEntry* evt);

        void ReplayEnqueueTaskEvent(Js::ScriptContext* ctx, Js::Var taskVar);

        //Get the current top-level event time 
        int64 GetCurrentTopLevelEventTime() const;

        //Get the event time corresponding to the first/last/k-th top-level event in the log
        int64 GetFirstEventTimeInLog() const;
        int64 GetLastEventTimeInLog() const;
        int64 GetKthEventTimeInLog(uint32 k) const;

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
        //Return -1 if no such snapshot is available
        int64 FindSnapTimeForEventTime(int64 targetTime, int64* optEndSnapTime);

        //Find the enclosing snapshot interval for the specified event time
        void GetSnapShotBoundInterval(int64 targetTime, int64* snapIntervalStart, int64* snapIntervalEnd) const;

        //Find the snapshot start time for the previous interval return -1 if no such time exists
        int64 GetPreviousSnapshotInterval(int64 currentSnapTime) const;

        //Do the inflation of the snapshot that is at the given event time
        void DoSnapshotInflate(int64 etime);

        //Run execute top level event calls until the given time is reached
        void ReplayRootEventsToTime(int64 eventTime);

        //For a single root level event -- snapshot, yield point, or ActionEvent
        void ReplaySingleRootEntry();

        //When we have an externalFunction (or promise register) we exit script context and need to play until the event time counts up to (and including) the given eventTime
        void ReplayActionEventSequenceThroughTime(int64 eventTime);

        //Replay the enter/exit and any iteration need to discharge all the effects of a single ActionEvent
        void ReplaySingleActionEventEntry();

        ////////////////////////////////
        //Host API record & replay support

        //Return true if this is a propertyRecord reference
        bool IsPropertyRecordRef(void* ref) const;

        //Get the current time from the hi res timer
        double GetCurrentWallTime();

        //Get the most recently assigned event time value
        int64 GetLastEventTime() const;

        NSLogEvents::EventLogEntry* RecordJsRTCreateScriptContext(TTDJsRTActionResultAutoRecorder& actionPopper);
        void RecordJsRTCreateScriptContextResult(NSLogEvents::EventLogEntry* evt, Js::ScriptContext* newCtx);

        void RecordJsRTSetCurrentContext(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var globalObject);

#if !INT32VAR
        void RecordJsRTCreateInteger(TTDJsRTActionResultAutoRecorder& actionPopper, int value);
#endif

        //Record creation operations
        void RecordJsRTCreateNumber(TTDJsRTActionResultAutoRecorder& actionPopper, double value);
        void RecordJsRTCreateBoolean(TTDJsRTActionResultAutoRecorder& actionPopper, bool value);
        void RecordJsRTCreateString(TTDJsRTActionResultAutoRecorder& actionPopper, const char16* stringValue, size_t stringLength);
        void RecordJsRTCreateSymbol(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);

        //Record error creation
        void RecordJsRTCreateError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg);
        void RecordJsRTCreateRangeError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var vmsg);
        void RecordJsRTCreateReferenceError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg);
        void RecordJsRTCreateSyntaxError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg);
        void RecordJsRTCreateTypeError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg);
        void RecordJsRTCreateURIError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg);

        //Record conversions
        void RecordJsRTVarToNumberConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTVarToBooleanConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTVarToStringConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTVarToObjectConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);

        //Record lifetime management events
        void RecordJsRTAddRootRef(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTAddWeakRootRef(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTEventLoopYieldPoint();

        //Record object allocate operations
        void RecordJsRTAllocateBasicObject(TTDJsRTActionResultAutoRecorder& actionPopper);
        void RecordJsRTAllocateExternalObject(TTDJsRTActionResultAutoRecorder& actionPopper);
        void RecordJsRTAllocateBasicArray(TTDJsRTActionResultAutoRecorder& actionPopper, uint32 length);
        void RecordJsRTAllocateArrayBuffer(TTDJsRTActionResultAutoRecorder& actionPopper, uint32 size);
        void RecordJsRTAllocateExternalArrayBuffer(TTDJsRTActionResultAutoRecorder& actionPopper, byte* buff, uint32 size);
        void RecordJsRTAllocateFunction(TTDJsRTActionResultAutoRecorder& actionPopper, bool isNamed, Js::Var optName);

        //Record GetAndClearException
        void RecordJsRTHostExitProcess(TTDJsRTActionResultAutoRecorder& actionPopper, int32 exitCode);
        void RecordJsRTGetAndClearExceptionWithMetadata(TTDJsRTActionResultAutoRecorder& actionPopper);
        void RecordJsRTGetAndClearException(TTDJsRTActionResultAutoRecorder& actionPopper);
        void RecordJsRTSetException(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, bool propagateToDebugger);

        //Record query operations
        void RecordJsRTHasProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var);
        void RecordJsRTHasOwnProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var);
        void RecordJsRTInstanceOf(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var object, Js::Var constructor);
        void RecordJsRTEquals(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var1, Js::Var var2, bool doStrict);

        //Record getters with native results
        void RecordJsRTGetPropertyIdFromSymbol(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var sym);

        //Record Object Getters
        void RecordJsRTGetPrototype(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTGetProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var);
        void RecordJsRTGetIndex(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var index, Js::Var var);
        void RecordJsRTGetOwnPropertyInfo(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var);
        void RecordJsRTGetOwnPropertyNamesInfo(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);
        void RecordJsRTGetOwnPropertySymbolsInfo(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var);

        //Record Object Setters
        void RecordJsRTDefineProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, Js::Var propertyDescriptor);
        void RecordJsRTDeleteProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, bool useStrictRules);
        void RecordJsRTSetPrototype(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, Js::Var proto);
        void RecordJsRTSetProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, Js::Var val, bool useStrictRules);
        void RecordJsRTSetIndex(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, Js::Var index, Js::Var val);

        //Record a get info from a typed array
        void RecordJsRTGetTypedArrayInfo(Js::Var var, Js::Var result);
        void RecordJsRTGetDataViewInfo(Js::Var var, Js::Var result);

        //Record various raw byte* from ArrayBuffer manipulations
        void RecordJsRTRawBufferCopySync(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 dstIndex, Js::Var src, uint32 srcIndex, uint32 length);
        void RecordJsRTRawBufferModifySync(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 index, uint32 count);
        void RecordJsRTRawBufferAsyncModificationRegister(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 index);
        void RecordJsRTRawBufferAsyncModifyComplete(TTDJsRTActionResultAutoRecorder& actionPopper, TTDPendingAsyncBufferModification& pendingAsyncInfo, byte* finalModPos);

        //Record a constructor call from JsRT
        void RecordJsRTConstructCall(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var funcVar, uint32 argCount, Js::Var* args);

        //Record code parse
        NSLogEvents::EventLogEntry* RecordJsRTCodeParse(TTDJsRTActionResultAutoRecorder& actionPopper, LoadScriptFlag loadFlag, bool isUft8, const byte* script, uint32 scriptByteLength, uint64 sourceContextId, const char16* sourceUri);

        //Record callback of an existing function
        NSLogEvents::EventLogEntry* RecordJsRTCallFunction(TTDJsRTActionResultAutoRecorder& actionPopper, int32 rootDepth, Js::Var funcVar, uint32 argCount, Js::Var* args);

        ////////////////////////////////
        //Emit code and support

        void EmitLog(const char* emitUri, size_t emitUriLength);
        void ParseLogInto(TTDataIOInfo& iofp, const char* parseUri, size_t parseUriLength);
    };

    //In cases where we may have many exits where we need to pop something we pushed earlier (i.e. exceptions)
    class TTModeStackAutoPopper
    {
    private:
        EventLog* m_log;
        TTDMode m_popMode; //the mode to pop or invalid if we don't need to pop anything

    public:
        TTModeStackAutoPopper(EventLog* log)
            : m_log(log), m_popMode(TTDMode::Invalid)
        {
            ;
        }

        void PushModeAndSetToAutoPop(TTDMode mode)
        {
            this->m_log->PushMode(mode);
            this->m_popMode = mode;
        }

        ~TTModeStackAutoPopper()
        {
            if(this->m_popMode != TTDMode::Invalid)
            {
                this->m_log->PopMode(this->m_popMode);
            }
        }
    };
}

#endif

