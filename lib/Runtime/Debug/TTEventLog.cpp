//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    TTDExceptionFramePopper::TTDExceptionFramePopper()
        : m_log(nullptr), m_function(nullptr)
    {
        ;
    }

    TTDExceptionFramePopper::~TTDExceptionFramePopper()
    {
#if ENABLE_TTD_DEBUGGING
        //we didn't clear this so an exception was thrown and we are propagating
        if(this->m_log != nullptr)
        {
            //if it doesn't have an exception frame then this is the frame where the exception was thrown so record our info
            this->m_log->PopCallEventException(this->m_function, !this->m_log->HasImmediateExceptionFrame());
        }
#endif
    }

    void TTDExceptionFramePopper::PushInfo(EventLog* log, Js::JavascriptFunction* function)
    {
        this->m_log = log; //set the log info so if the pop isn't called the destructor will record propagation
        this->m_function = function;
    }

    void TTDExceptionFramePopper::PopInfo()
    {
        this->m_log = nullptr; //normal pop (no exception) just clear so destructor nops
    }

    TTDRecordExternalFunctionCallActionPopper::TTDRecordExternalFunctionCallActionPopper(EventLog* log, Js::JavascriptFunction* function)
        : m_log(log), m_function(function), m_timer(), m_callAction(nullptr)
    {
        Js::ScriptContext* ctx = this->m_function->GetScriptContext();
        ctx->TTDRootNestingCount++;
    }

    TTDRecordExternalFunctionCallActionPopper::~TTDRecordExternalFunctionCallActionPopper()
    {
        if(this->m_callAction != nullptr)
        {
            double endTime = this->m_timer.Now();

            Js::ScriptContext* ctx = this->m_function->GetScriptContext();

            //
            //TODO: we will want to be a bit more detailed on this later
            //
            bool hasScriptException = ctx->HasRecordedException();
            bool hasTerminalException = false;

            this->m_log->RecordExternalCallEndEvent(this->m_function, this->m_callAction->GetEventTime(), this->m_callAction->GetRootNestingDepth(), hasScriptException, hasTerminalException, endTime, ctx->GetLibrary()->GetUndefined());

            ctx->TTDRootNestingCount--;
        }
    }

    void TTDRecordExternalFunctionCallActionPopper::NormalReturn(bool checkException, Js::Var returnValue)
    {
        AssertMsg(this->m_callAction != nullptr, "Should never be null on normal return!");

        double endTime = this->m_timer.Now();

        Js::ScriptContext* ctx = this->m_function->GetScriptContext();

        //
        //TODO: we will want to be a bit more detailed on this later
        //
        bool hasScriptException = checkException ? ctx->HasRecordedException() : false;
        bool hasTerminalException = false;

        this->m_log->RecordExternalCallEndEvent(this->m_function, this->m_callAction->GetEventTime(), this->m_callAction->GetRootNestingDepth(), hasScriptException, hasTerminalException, endTime, returnValue);

        this->m_callAction = nullptr;
        ctx->TTDRootNestingCount--;
    }

    void TTDRecordExternalFunctionCallActionPopper::SetCallAction(ExternalCallEventBeginLogEntry* action)
    {
        this->m_callAction = action;
    }

    double TTDRecordExternalFunctionCallActionPopper::GetStartTime()
    {
        this->m_startTime = this->m_timer.Now();

        return this->m_startTime;
    }

    TTDRecordJsRTFunctionCallActionPopper::TTDRecordJsRTFunctionCallActionPopper(EventLog* log, Js::ScriptContext* scriptContext)
        : m_log(log), m_scriptContext(scriptContext), m_timer(), m_callAction(nullptr)
    {
        ;
    }

    TTDRecordJsRTFunctionCallActionPopper::~TTDRecordJsRTFunctionCallActionPopper()
    {
        if(this->m_callAction != nullptr)
        {
            double endTime = this->m_timer.Now();

            //
            //TODO: we will want to be a bit more detailed on this later
            //
            bool hasScriptException = true; 
            bool hasTerminalException = false;

            this->m_log->RecordJsRTCallFunctionEnd(this->m_scriptContext, this->m_callAction->GetEventTime(), hasScriptException, hasTerminalException, this->m_callAction->GetCallDepth(), endTime);
            this->m_log->IncrementElapsedSnapshotTime(endTime - this->m_startTime);
        }
    }

    void TTDRecordJsRTFunctionCallActionPopper::NormalReturn()
    {
        AssertMsg(this->m_callAction != nullptr, "Should never be null on normal return!");

        double endTime = this->m_timer.Now();

        //
        //TODO: we will want to be a bit more detailed on this later
        //
        bool hasScriptException = false;
        bool hasTerminalException = false;

        this->m_log->RecordJsRTCallFunctionEnd(this->m_scriptContext, this->m_callAction->GetEventTime(), hasScriptException, hasTerminalException, this->m_callAction->GetCallDepth(), endTime);
        this->m_log->IncrementElapsedSnapshotTime(endTime - this->m_startTime);

        this->m_callAction = nullptr;
    }

    void TTDRecordJsRTFunctionCallActionPopper::SetCallAction(JsRTCallFunctionBeginAction* action)
    {
        this->m_callAction = action;
    }

    double TTDRecordJsRTFunctionCallActionPopper::GetStartTime()
    {
        this->m_startTime = this->m_timer.Now();

        return this->m_startTime;
    }

    /////////////

    void TTEventList::AddArrayLink()
    {
        TTEventListLink* newHeadBlock = this->m_alloc->SlabAllocateStruct<TTEventListLink>();
        newHeadBlock->BlockData = this->m_alloc->SlabAllocateFixedSizeArray<EventLogEntry*, TTD_EVENTLOG_LIST_BLOCK_SIZE>();

        newHeadBlock->CurrPos = 0;
        newHeadBlock->StartPos = 0;

        newHeadBlock->Next = nullptr;
        newHeadBlock->Previous = this->m_headBlock;

        if(this->m_headBlock != nullptr)
        {
            this->m_headBlock->Next = newHeadBlock;
        }

        this->m_headBlock = newHeadBlock;
    }

    void TTEventList::RemoveArrayLink(TTEventListLink* block)
    {
        AssertMsg(block->Previous == nullptr, "Not first event block in log!!!");
        AssertMsg(block->StartPos == block->CurrPos, "Haven't cleared all the events in this link");

        if(block->Next == nullptr)
        {
            this->m_headBlock = nullptr; //was only 1 block to we are now all null
        }
        else
        {
            block->Next->Previous = nullptr;
        }

        this->m_alloc->UnlinkAllocation(block->BlockData);
        this->m_alloc->UnlinkAllocation(block);
    }

    TTEventList::TTEventList(UnlinkableSlabAllocator* alloc)
        : m_alloc(alloc), m_headBlock(nullptr)
    {
        ;
    }

    void TTEventList::UnloadEventList()
    {
        if(this->m_headBlock == nullptr)
        {
            return;
        }

        TTEventListLink* firstBlock = this->m_headBlock;
        while(firstBlock->Previous != nullptr)
        {
            firstBlock = firstBlock->Previous;
        }

        TTEventListLink* curr = firstBlock;
        while(curr != nullptr)
        {
            for(uint32 i = curr->StartPos; i < curr->CurrPos; ++i)
            {
                curr->BlockData[i]->UnloadEventMemory(*(this->m_alloc));

                this->m_alloc->UnlinkAllocation(curr->BlockData[i]);
            }
            curr->StartPos = curr->CurrPos;

            TTEventListLink* next = curr->Next;
            this->RemoveArrayLink(curr);
            curr = next;
        }

        this->m_headBlock = nullptr;
    }

    void TTEventList::AddEntry(EventLogEntry* data)
    {
        if((this->m_headBlock == nullptr) || (this->m_headBlock->CurrPos == TTD_EVENTLOG_LIST_BLOCK_SIZE))
        {
            this->AddArrayLink();
        }

        this->m_headBlock->BlockData[this->m_headBlock->CurrPos] = data;
        this->m_headBlock->CurrPos++;
    }

    void TTEventList::DeleteFirstEntry(TTEventListLink* block, EventLogEntry* data)
    {
        AssertMsg(block->Previous == nullptr, "Not first event block in log!!!");
        AssertMsg(block->BlockData[block->StartPos] == data, "Not the data at the start of the list!!!");

        data->UnloadEventMemory(*(this->m_alloc));
        this->m_alloc->UnlinkAllocation(data);

        block->StartPos++;
        if(block->StartPos == block->CurrPos)
        {
            this->RemoveArrayLink(block);
        }
    }

    bool TTEventList::IsEmpty() const
    {
        return this->m_headBlock == nullptr;
    }

    uint32 TTEventList::Count() const
    {
        uint32 count = 0;

        for(TTEventListLink* curr = this->m_headBlock; curr != nullptr; curr = curr->Previous)
        {
            count += (curr->CurrPos - curr->StartPos);
        }

        return (uint32)count;
    }

    TTEventList::Iterator::Iterator()
        : m_currLink(nullptr), m_currIdx(0)
    {
        ;
    }

    TTEventList::Iterator::Iterator(TTEventListLink* head, uint32 pos)
        : m_currLink(head), m_currIdx(pos)
    {
        ;
    }

    const EventLogEntry* TTEventList::Iterator::Current() const
    {
        AssertMsg(this->IsValid(), "Iterator is invalid!!!");

        return this->m_currLink->BlockData[this->m_currIdx];
    }

    EventLogEntry* TTEventList::Iterator::Current()
    {
        AssertMsg(this->IsValid(), "Iterator is invalid!!!");

        return this->m_currLink->BlockData[this->m_currIdx];
    }

    bool TTEventList::Iterator::IsValid() const
    {
        return (this->m_currLink != nullptr && this->m_currLink->StartPos <= this->m_currIdx && this->m_currIdx < this->m_currLink->CurrPos);
    }

    void TTEventList::Iterator::MoveNext()
    {
        if(this->m_currIdx < (this->m_currLink->CurrPos - 1))
        {
            this->m_currIdx++;
        }
        else
        {
            this->m_currLink = this->m_currLink->Next;
            this->m_currIdx = (this->m_currLink != nullptr) ? this->m_currLink->StartPos : 0;
        }
    }

    void TTEventList::Iterator::MovePrevious()
    {
        if(this->m_currIdx > this->m_currLink->StartPos)
        {
            this->m_currIdx--;
        }
        else
        {
            this->m_currLink = this->m_currLink->Previous;
            this->m_currIdx = (this->m_currLink != nullptr) ? (this->m_currLink->CurrPos - 1) : 0;
        }
    }

    TTEventList::Iterator TTEventList::GetIteratorAtFirst() const
    {
        if(this->m_headBlock == nullptr)
        {
            return Iterator(nullptr, 0);
        }
        else
        {
            TTEventListLink* firstBlock = this->m_headBlock;
            while(firstBlock->Previous != nullptr)
            {
                firstBlock = firstBlock->Previous;
            }

            return Iterator(firstBlock, firstBlock->StartPos);
        }
    }

    TTEventList::Iterator TTEventList::GetIteratorAtLast() const
    {
        if(this->m_headBlock == nullptr)
        {
            return Iterator(nullptr, 0);
        }
        else
        {
            return Iterator(this->m_headBlock, this->m_headBlock->CurrPos - 1);
        }
    }

    //////

    const SingleCallCounter& EventLog::GetTopCallCounter() const
    {
        AssertMsg(this->m_callStack.Count() != 0, "Empty stack!");

        return this->m_callStack.Item(this->m_callStack.Count() - 1);
    }

    SingleCallCounter& EventLog::GetTopCallCounter()
    {
        AssertMsg(this->m_callStack.Count() != 0, "Empty stack!");

        return this->m_callStack.Item(this->m_callStack.Count() - 1);
    }

    const SingleCallCounter& EventLog::GetTopCallCallerCounter() const
    {
        AssertMsg(this->m_callStack.Count() >= 2, "Empty stack!");

        return this->m_callStack.Item(this->m_callStack.Count() - 2);
    }

    int64 EventLog::GetCurrentEventTimeAndAdvance()
    {
        return this->m_eventTimeCtr++;
    }

    void EventLog::AdvanceTimeAndPositionForReplay()
    {
        this->m_eventTimeCtr++;
        this->m_currentReplayEventIterator.MoveNext();

        AssertMsg(!this->m_currentReplayEventIterator.IsValid() || this->m_eventTimeCtr <= this->m_currentReplayEventIterator.Current()->GetEventTime(), "Something is out of sync.");
    }

    void EventLog::InsertEventAtHead(EventLogEntry* evnt)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        evnt->DiagnosticEventTagValue = this->m_threadContext->TTDInfo->GetLogTagValueForDiagnostics();
#endif

        this->m_eventList.AddEntry(evnt);
    }

    void EventLog::UpdateComputedMode()
    {
        AssertMsg(this->m_modeStack.Count() >= 0, "Should never be empty!!!");

        TTDMode cm = TTDMode::Invalid;
        for(int32 i = 0; i < this->m_modeStack.Count(); ++i)
        {
            TTDMode m = this->m_modeStack.Item(i);
            switch(m)
            {
            case TTDMode::Pending:
            case TTDMode::Detached:
            case TTDMode::RecordEnabled:
            case TTDMode::DebuggingEnabled:
                AssertMsg(i == 0, "One of these should always be first on the stack.");
                cm = m;
                break;
            case TTDMode::ExcludedExecution:
                AssertMsg(i != 0, "A base mode should always be first on the stack.");
                cm |= m;
                break;
            default:
                AssertMsg(false, "This mode is unknown or should never appear.");
                break;
            }
        }

        this->m_currentMode = cm;

        if(this->m_ttdContext != nullptr)
        {
            this->m_ttdContext->SetMode_TTD(this->m_currentMode);
        }
    }

    void EventLog::UnloadRetainedData()
    {
        if(this->m_lastInflateMap != nullptr)
        {
            HeapDelete(this->m_lastInflateMap);
            this->m_lastInflateMap = nullptr;
        }

        if(this->m_propertyRecordPinSet != nullptr)
        {
            this->m_propertyRecordPinSet->GetAllocator()->RootRelease(this->m_propertyRecordPinSet);
            this->m_propertyRecordPinSet = nullptr;
        }
    }

    void EventLog::DoSnapshotExtract_Helper(SnapShot** snap, TTD_LOG_TAG* logTag)
    {
        AssertMsg(this->m_ttdContext != nullptr, "We aren't actually tracking anything!!!");

        JsUtil::List<Js::Var, HeapAllocator> roots(&HeapAllocator::Instance);
        JsUtil::List<Js::ScriptContext*, HeapAllocator> ctxs(&HeapAllocator::Instance);

        ctxs.Add(this->m_ttdContext);
        this->m_ttdContext->ExtractSnapshotRoots_TTD(roots);

        this->m_snapExtractor.BeginSnapshot(this->m_threadContext, roots, ctxs);
        this->m_snapExtractor.DoMarkWalk(roots, ctxs, this->m_threadContext);

        ///////////////////////////
        //Phase 2: Evacuate marked objects
        //Allows for parallel execute and evacuate (in conjunction with later refactoring)

        this->m_snapExtractor.EvacuateMarkedIntoSnapshot(this->m_threadContext, ctxs);

        ///////////////////////////
        //Phase 3: Complete and return snapshot

        *snap = this->m_snapExtractor.CompleteSnapshot();

        ///////////////////////////
        //Get the tag information

        this->m_threadContext->TTDInfo->GetTagsForSnapshot(logTag);
    }

    void EventLog::ReplaySnapshotEvent()
    {
#if ENABLE_SNAPSHOT_COMPARE
        SnapShot* snap = nullptr;
        TTD_LOG_TAG logTag = TTD_INVALID_LOG_TAG;

        BEGIN_ENTER_SCRIPT(this->m_ttdContext, true, true, true);
        {
            //this->m_threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);

            this->DoSnapshotExtract_Helper(&snap, &logTag);

            const SnapshotEventLogEntry* recordedSnapEntry = SnapshotEventLogEntry::As(this->m_currentReplayEventIterator.Current());
            recordedSnapEntry->EnsureSnapshotDeserialized(this->m_logInfoRootDir.Contents, this->m_threadContext);
            const SnapShot* recordedSnap = recordedSnapEntry->GetSnapshot();

            TTDCompareMap compareMap(this->m_threadContext);
            SnapShot::InitializeForSnapshotCompare(recordedSnap, snap, compareMap);
            SnapShot::DoSnapshotCompare(recordedSnap, snap, compareMap);

            HeapDelete(snap);

            //this->m_threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
        }
        END_ENTER_SCRIPT;
#endif


#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteLiteralMsg("---SNAPSHOT EVENT---\n");
#endif

        this->AdvanceTimeAndPositionForReplay(); //move along
    }

    EventLog::EventLog(ThreadContext* threadContext, LPCWSTR logDir, uint32 snapInterval, uint32 snapHistoryLength)
        : m_threadContext(threadContext), m_eventSlabAllocator(TTD_SLAB_BLOCK_ALLOCATION_SIZE_MID), m_miscSlabAllocator(TTD_SLAB_BLOCK_ALLOCATION_SIZE_SMALL), m_snapInterval(snapInterval), m_snapHistoryLength(snapHistoryLength),
        m_eventTimeCtr(0), m_runningFunctionTimeCtr(0), m_topLevelCallbackEventTime(-1), m_hostCallbackId(-1),
        m_eventList(&this->m_eventSlabAllocator), m_currentReplayEventIterator(),
        m_callStack(&HeapAllocator::Instance, 32), 
#if ENABLE_TTD_DEBUGGING
        m_isReturnFrame(false), m_isExceptionFrame(false), m_lastFrame(), m_pendingTTDBP(), m_activeBPId(-1), m_activeTTDBP(),
#endif
#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        m_diagnosticLogger(),
#endif
        m_modeStack(&HeapAllocator::Instance), m_currentMode(TTDMode::Pending),
        m_ttdContext(nullptr),
        m_snapExtractor(), m_elapsedExecutionTimeSinceSnapshot(0.0),
        m_lastInflateSnapshotTime(-1), m_lastInflateMap(nullptr), m_propertyRecordPinSet(nullptr), m_propertyRecordList(&this->m_miscSlabAllocator)
    {
        JsSupport::CopyStringToHeapAllocator(logDir, this->m_logInfoRootDir);

        if(this->m_snapHistoryLength < 2)
        {
            this->m_snapHistoryLength = 2;
        }

        this->m_modeStack.Add(TTDMode::Pending);

        this->m_propertyRecordPinSet = RecyclerNew(threadContext->GetRecycler(), PropertyRecordPinSet, threadContext->GetRecycler());
        this->m_threadContext->GetRecycler()->RootAddRef(this->m_propertyRecordPinSet);
    }

    EventLog::~EventLog()
    {
        this->m_eventList.UnloadEventList();

        this->UnloadRetainedData();

        JsSupport::DeleteStringFromHeapAllocator(this->m_logInfoRootDir);
    }

    void EventLog::InitForTTDRecord()
    {
        //Prepr the logging stream so it is ready for us to write into
        this->m_threadContext->TTDWriteInitializeFunction(this->m_logInfoRootDir.Contents);

        //pin all the current properties so they don't move/disappear on us
        for(Js::PropertyId pid = TotalNumberOfBuiltInProperties + 1; pid < this->m_threadContext->GetMaxPropertyId(); ++pid)
        {
            const Js::PropertyRecord* pRecord = this->m_threadContext->GetPropertyName(pid);
            this->AddPropertyRecord(pRecord);
        }
    }

    void EventLog::InitForTTDReplay()
    {
        this->ParseLogInto();

        Js::PropertyId maxPid = TotalNumberOfBuiltInProperties + 1;
        JsUtil::BaseDictionary<Js::PropertyId, NSSnapType::SnapPropertyRecord*, HeapAllocator> pidMap(&HeapAllocator::Instance);

        for(auto iter = this->m_propertyRecordList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            maxPid = max(maxPid, iter.Current()->PropertyId);
            pidMap.AddNew(iter.Current()->PropertyId, iter.Current());
        }

        for(Js::PropertyId cpid = TotalNumberOfBuiltInProperties + 1; cpid <= maxPid; ++cpid)
        {
            NSSnapType::SnapPropertyRecord* spRecord = pidMap.LookupWithKey(cpid, nullptr);
            AssertMsg(spRecord != nullptr, "We have a gap in the sequence of propertyIds. Not sure how that happens.");

            const Js::PropertyRecord* newPropertyRecord = NSSnapType::InflatePropertyRecord(spRecord, this->m_threadContext);

            if(!this->m_propertyRecordPinSet->ContainsKey(const_cast<Js::PropertyRecord*>(newPropertyRecord)))
            {
                this->m_propertyRecordPinSet->AddNew(const_cast<Js::PropertyRecord*>(newPropertyRecord));
            }
        }
    }

    void EventLog::StartTimeTravelOnScript(Js::ScriptContext* ctx, const HostScriptContextCallbackFunctor& callbackFunctor)
    {
        AssertMsg(this->m_ttdContext == nullptr, "Should only add 1 time!");

        ctx->SetMode_TTD(this->m_currentMode);
        ctx->SetCallbackFunctor_TTD(callbackFunctor);

        this->m_ttdContext = ctx;

        ctx->InitializeRecordingActionsAsNeeded_TTD();
    }

    void EventLog::StopTimeTravelOnScript(Js::ScriptContext* ctx)
    {
        AssertMsg(this->m_ttdContext == ctx, "Should be enabled before we disable!");

        ctx->SetMode_TTD(TTDMode::Detached);
        this->m_ttdContext = nullptr;
    }

    void EventLog::SetGlobalMode(TTDMode m)
    {
        AssertMsg(m == TTDMode::Pending || m == TTDMode::Detached || m == TTDMode::RecordEnabled || m == TTDMode::DebuggingEnabled, "These are the only valid global modes");

        this->m_modeStack.SetItem(0, m);
        this->UpdateComputedMode();
    }

    void EventLog::PushMode(TTDMode m)
    {
        AssertMsg(m == TTDMode::ExcludedExecution, "These are the only valid mode modifiers to push");

        this->m_modeStack.Add(m);
        this->UpdateComputedMode();
    }

    void EventLog::PopMode(TTDMode m)
    {
        AssertMsg(m == TTDMode::ExcludedExecution, "These are the only valid mode modifiers to push");
        AssertMsg(this->m_modeStack.Last() == m, "Push/Pop is not matched so something went wrong.");

        this->m_modeStack.RemoveAtEnd();
        this->UpdateComputedMode();
    }

    void EventLog::SetIntoDebuggingMode()
    {
        this->m_modeStack.SetItem(0, TTDMode::DebuggingEnabled);
        this->UpdateComputedMode();

        this->m_ttdContext->InitializeDebuggingActionsAsNeeded_TTD();
    }

    bool EventLog::ShouldPerformRecordAction() const
    {
        bool modeIsRecord = (this->m_currentMode & TTDMode::RecordEnabled) == TTDMode::RecordEnabled;
        bool inRecordableCode = (this->m_currentMode & TTDMode::ExcludedExecution) == TTDMode::Invalid;

        return modeIsRecord & inRecordableCode;
    }

    bool EventLog::ShouldPerformDebugAction() const
    {
        bool modeIsDebug = (this->m_currentMode & TTDMode::DebuggingEnabled) == TTDMode::DebuggingEnabled;
        bool inDebugableCode = (this->m_currentMode & TTDMode::ExcludedExecution) == TTDMode::Invalid;

        return modeIsDebug & inDebugableCode;
    }

    bool EventLog::ShouldTagForJsRT() const
    {
        bool modeIsPending = (this->m_currentMode & TTDMode::Pending) == TTDMode::Pending;
        bool modeIsRecord = (this->m_currentMode & TTDMode::RecordEnabled) == TTDMode::RecordEnabled;
        bool inDebugableCode = (this->m_currentMode & TTDMode::ExcludedExecution) == TTDMode::Invalid;

        return ((modeIsPending | modeIsRecord) & inDebugableCode);
    }

    bool EventLog::ShouldTagForExternalCall() const
    {
        bool modeIsPending = (this->m_currentMode & TTDMode::Pending) == TTDMode::Pending;
        bool modeIsRecord = (this->m_currentMode & TTDMode::RecordEnabled) == TTDMode::RecordEnabled;
        bool modeIsDebug = (this->m_currentMode & TTDMode::DebuggingEnabled) == TTDMode::DebuggingEnabled;
        bool inDebugableCode = (this->m_currentMode & TTDMode::ExcludedExecution) == TTDMode::Invalid;

        return ((modeIsPending | modeIsRecord | modeIsDebug) & inDebugableCode);
    }

    bool EventLog::IsTTDActive() const
    {
        return (this->m_currentMode & TTDMode::TTDActive) != TTDMode::Invalid;
    }

    bool EventLog::IsTTDDetached() const
    {
        return (this->m_currentMode & TTDMode::Detached) != TTDMode::Invalid;
    }

    void EventLog::AddPropertyRecord(const Js::PropertyRecord* record)
    {
        this->m_propertyRecordPinSet->AddNew(const_cast<Js::PropertyRecord*>(record));
    }

    void EventLog::RecordTelemetryLogEvent(Js::JavascriptString* infoStringJs, bool doPrint)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Mode is inconsistent!");

        TTString infoString;
        this->m_eventSlabAllocator.CopyStringIntoWLength(infoStringJs->GetSz(), infoStringJs->GetLength(), infoString);

        TelemetryEventLogEntry* tevent = this->m_eventSlabAllocator.SlabNew<TelemetryEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), infoString, doPrint);

        this->InsertEventAtHead(tevent);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.ForceFlush();
#endif
    }

    void EventLog::ReplayTelemetryLogEvent(Js::JavascriptString* infoStringJs)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TelemetryEventLogEntry* tevent = TelemetryEventLogEntry::As(this->m_currentReplayEventIterator.Current());

        const TTString& msg = tevent->GetInfoString();
        uint32 infoStrLength = (uint32)infoStringJs->GetLength();
        LPCWSTR infoStr = infoStringJs->GetSz();

        if(msg.Length != infoStrLength)
        {
            AssertMsg(false, "Telemetry messages differ??");
        }
        else
        {
            for(uint32 i = 0; i < infoStrLength; ++i)
            {
                AssertMsg(msg.Contents[i] == infoStr[i], "Telemetry messages differ??");
            }
        }
#endif

        this->AdvanceTimeAndPositionForReplay();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.ForceFlush();
#endif
    }

    void EventLog::RecordDateTimeEvent(double time)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Mode is inconsistent!");

        DoubleEventLogEntry* devent = this->m_eventSlabAllocator.SlabNew<DoubleEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), time);
        this->InsertEventAtHead(devent);
    }

    void EventLog::RecordDateStringEvent(Js::JavascriptString* stringValue)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Mode is inconsistent!");

        TTString copyStr;
        this->m_eventSlabAllocator.CopyStringIntoWLength(stringValue->GetSz(), stringValue->GetLength(), copyStr);

        StringValueEventLogEntry* sevent = this->m_eventSlabAllocator.SlabNew<StringValueEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), copyStr);
        this->InsertEventAtHead(sevent);
    }

    void EventLog::ReplayDateTimeEvent(double* result)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        DoubleEventLogEntry* devent = DoubleEventLogEntry::As(this->m_currentReplayEventIterator.Current());
        *result = devent->GetDoubleValue();

        this->AdvanceTimeAndPositionForReplay();
    }

    void EventLog::ReplayDateStringEvent(Js::ScriptContext* ctx, Js::JavascriptString** result)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        StringValueEventLogEntry* sevent = StringValueEventLogEntry::As(this->m_currentReplayEventIterator.Current());
        const TTString& str = sevent->GetStringValue();
        *result = Js::JavascriptString::NewCopyBuffer(str.Contents, str.Length, ctx);

        this->AdvanceTimeAndPositionForReplay();
    }

    void EventLog::RecordExternalEntropyRandomEvent(uint64 seed0, uint64 seed1)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        RandomSeedEventLogEntry* revent = this->m_eventSlabAllocator.SlabNew<RandomSeedEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), seed0, seed1);
        this->InsertEventAtHead(revent);
    }

    void EventLog::ReplayExternalEntropyRandomEvent(uint64* seed0, uint64* seed1)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        RandomSeedEventLogEntry* revent = RandomSeedEventLogEntry::As(this->m_currentReplayEventIterator.Current());
        *seed0 = revent->GetSeed0();
        *seed1 = revent->GetSeed1();

        this->AdvanceTimeAndPositionForReplay();
    }

    void EventLog::RecordPropertyEnumEvent(BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, Js::JavascriptString* propertyName)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        TTString optName;
        InitializeAsNullPtrTTString(optName);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        if(returnCode)
        {
            this->m_eventSlabAllocator.CopyStringIntoWLength(propertyName->GetSz(), propertyName->GetLength(), optName);
        }
#else
        if(returnCode && pid == Js::Constants::NoProperty)
        {
            this->m_eventSlabAllocator.CopyStringIntoWLength(propertyName->GetSz(), propertyName->GetLength(), optName);
        }
#endif

        PropertyEnumStepEventLogEntry* eevent = this->m_eventSlabAllocator.SlabNew<PropertyEnumStepEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), returnCode, pid, attributes, optName);
        this->InsertEventAtHead(eevent);
    }

    void EventLog::ReplayPropertyEnumEvent(BOOL* returnCode, int32* newIndex, const Js::DynamicObject* obj, Js::PropertyId* pid, Js::PropertyAttributes* attributes, Js::JavascriptString** propertyName)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        PropertyEnumStepEventLogEntry* eevent = PropertyEnumStepEventLogEntry::As(this->m_currentReplayEventIterator.Current());
        *returnCode = eevent->GetReturnCode();
        *pid = eevent->GetPropertyId();
        *attributes = eevent->GetAttributes();

        if(*returnCode)
        {
            AssertMsg(*pid != Js::Constants::NoProperty, "This is so weird we need to figure out what this means.");
            Js::PropertyString* propertyString = obj->GetScriptContext()->GetPropertyString(*pid);
            *propertyName = propertyString;

            const Js::PropertyRecord* pRecord = obj->GetScriptContext()->GetPropertyName(*pid);
            *newIndex = obj->GetDynamicType()->GetTypeHandler()->GetPropertyIndex(pRecord);
        }
        else
        {
            *propertyName = nullptr;

            *newIndex = obj->GetDynamicType()->GetTypeHandler()->GetPropertyCount();
        }

        this->AdvanceTimeAndPositionForReplay();
    }

    void EventLog::RecordSymbolCreationEvent(Js::PropertyId pid)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        SymbolCreationEventLogEntry* sevent = this->m_eventSlabAllocator.SlabNew<SymbolCreationEventLogEntry>(this->GetCurrentEventTimeAndAdvance(), pid);
        this->InsertEventAtHead(sevent);
    }

    void EventLog::ReplaySymbolCreationEvent(Js::PropertyId* pid)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        SymbolCreationEventLogEntry* sevent = SymbolCreationEventLogEntry::As(this->m_currentReplayEventIterator.Current());
        *pid = sevent->GetPropertyId();

        this->AdvanceTimeAndPositionForReplay();
    }

    ExternalCallEventBeginLogEntry* EventLog::RecordExternalCallBeginEvent(Js::JavascriptFunction* func, int32 rootDepth, uint32 argc, Js::Var* argv, double beginTime)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        ExternalCallEventBeginLogEntry* eevent = this->m_eventSlabAllocator.SlabNew<ExternalCallEventBeginLogEntry>(this->GetCurrentEventTimeAndAdvance(), rootDepth, beginTime);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString fname;
        Js::JavascriptString* displayName = func->GetDisplayName();
        this->m_eventSlabAllocator.CopyStringIntoWLength(displayName->GetSz(), displayName->GetLength(), fname);

        eevent->SetFunctionName(fname);
#endif

        this->InsertEventAtHead(eevent);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteCall(func, true, argc, argv);
#endif

        return eevent;
    }

    void EventLog::RecordExternalCallEndEvent(Js::JavascriptFunction* func, int64 matchingBeginTime, int32 rootNestingDepth, bool hasScriptException, bool hasTerminatingException, double endTime, Js::Var value)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        NSLogValue::ArgRetValue retVal;
        NSLogValue::ExtractArgRetValueFromVar(value, retVal, this->m_eventSlabAllocator);

        ExternalCallEventEndLogEntry* eevent = this->m_eventSlabAllocator.SlabNew<ExternalCallEventEndLogEntry>(this->GetCurrentEventTimeAndAdvance(), matchingBeginTime, rootNestingDepth, hasScriptException, hasTerminatingException, endTime, retVal);

        this->InsertEventAtHead(eevent);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturn(func, value);
#endif
    }

    ExternalCallEventBeginLogEntry* EventLog::RecordEnqueueTaskBeginEvent(int32 rootDepth, double beginTime)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        ExternalCallEventBeginLogEntry* eevent = this->m_eventSlabAllocator.SlabNew<ExternalCallEventBeginLogEntry>(this->GetCurrentEventTimeAndAdvance(), rootDepth, beginTime);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString pname;
        this->m_eventSlabAllocator.CopyNullTermStringInto(_u("Register Promise Function"), pname);

        eevent->SetFunctionName(pname);
#endif

        this->InsertEventAtHead(eevent);

        return eevent;
    }

    void EventLog::RecordEnqueueTaskEndEvent(int64 matchingBeginTime, int32 rootDepth, double endTime, Js::Var value)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        NSLogValue::ArgRetValue retVal;
        NSLogValue::ExtractArgRetValueFromVar(value, retVal, this->m_eventSlabAllocator);

        ExternalCallEventEndLogEntry* eevent = this->m_eventSlabAllocator.SlabNew<ExternalCallEventEndLogEntry>(this->GetCurrentEventTimeAndAdvance(), matchingBeginTime, rootDepth, false, false, endTime, retVal);

        this->InsertEventAtHead(eevent);
    }

    void EventLog::ReplayExternalCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, Js::Var* result)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        //advance the begin event item off the event list
        ExternalCallEventBeginLogEntry* eeventBegin = ExternalCallEventBeginLogEntry::As(this->m_currentReplayEventIterator.Current());
        this->AdvanceTimeAndPositionForReplay();

        Js::ScriptContext* ctx = function->GetScriptContext();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteCall(function, true, argc, argv);
#endif

        //replay anything that happens when we are out of the call
        if(this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag)
        {
            if(!ctx->GetThreadContext()->IsScriptActive())
            {
                this->ReplayActionLoopStep();
            }
            else
            {
                BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(ctx)
                {
                    this->ReplayActionLoopStep();
                }
                END_LEAVE_SCRIPT_WITH_EXCEPTION(ctx);
            }
        }

        //May have exited inside the external call without anything else
        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        //advance the end event item off the event list and get the return value
        ExternalCallEventEndLogEntry* eeventEnd = ExternalCallEventEndLogEntry::As(this->m_currentReplayEventIterator.Current());
        this->AdvanceTimeAndPositionForReplay();

        AssertMsg(eeventBegin->GetRootNestingDepth() == eeventEnd->GetRootNestingDepth(), "These should always match!!!");

        *result = NSLogValue::InflateArgRetValueIntoVar(eeventEnd->GetReturnValue(), ctx);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturn(function, *result);
#endif
    }

    void EventLog::ReplayEnqueueTaskEvent(Js::ScriptContext* ctx, Js::Var* result)
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        //advance the begin event item off the event list
        ExternalCallEventBeginLogEntry* eeventBegin = ExternalCallEventBeginLogEntry::As(this->m_currentReplayEventIterator.Current());
        this->AdvanceTimeAndPositionForReplay();

        //replay anything that happens when we are out of the call
        if(this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag)
        {
            if(!ctx->GetThreadContext()->IsScriptActive())
            {
                this->ReplayActionLoopStep();
            }
            else
            {
                BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(ctx)
                {
                    this->ReplayActionLoopStep();
                }
                END_LEAVE_SCRIPT_WITH_EXCEPTION(ctx);
            }
        }

        //May have exited inside the external call without anything else
        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        AssertMsg(this->m_currentReplayEventIterator.Current()->GetEventTime() == this->m_eventTimeCtr, "Out of Sync!!!");

        //advance the end event item off the event list and get the return value
        ExternalCallEventEndLogEntry* eeventEnd = ExternalCallEventEndLogEntry::As(this->m_currentReplayEventIterator.Current());
        this->AdvanceTimeAndPositionForReplay();

        AssertMsg(eeventBegin->GetRootNestingDepth() == eeventEnd->GetRootNestingDepth(), "These should always match!!!");

        *result = NSLogValue::InflateArgRetValueIntoVar(eeventEnd->GetReturnValue(), ctx);
    }

    void EventLog::PushCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool isInFinally)
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");

#if ENABLE_TTD_DEBUGGING
        //Clear any previous last return frame info
        this->ClearReturnFrame();
#endif

        this->m_runningFunctionTimeCtr++;

        SingleCallCounter cfinfo;
        cfinfo.Function = function->GetFunctionBody();

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        cfinfo.Name = cfinfo.Function->GetExternalDisplayName();
#endif

        cfinfo.EventTime = this->m_eventTimeCtr; //don't need to advance just note what the event time was when this is called
        cfinfo.FunctionTime = this->m_runningFunctionTimeCtr;
        cfinfo.LoopTime = 0;

#if ENABLE_TTD_STACK_STMTS
        cfinfo.CurrentStatementIndex = -1;
        cfinfo.CurrentStatementLoopTime = 0;

        cfinfo.LastStatementIndex = -1;
        cfinfo.LastStatementLoopTime = 0;

        cfinfo.CurrentStatementBytecodeMin = UINT32_MAX;
        cfinfo.CurrentStatementBytecodeMax = UINT32_MAX;
#endif

        this->m_callStack.Add(cfinfo);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteCall(function, false, argc, argv);
#endif
    }

    void EventLog::PopCallEvent(Js::JavascriptFunction* function, Js::Var result)
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");

#if ENABLE_TTD_DEBUGGING
        if(!this->HasImmediateExceptionFrame())
        {
            this->SetReturnAndExceptionFramesFromCurrent(true, false);
        }
#endif

        this->m_runningFunctionTimeCtr++;
        this->m_callStack.RemoveAtEnd();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturn(function, result);
#endif
    }

    void EventLog::PopCallEventException(Js::JavascriptFunction* function, bool isFirstException)
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");

#if ENABLE_TTD_DEBUGGING
        if(isFirstException)
        {
            this->SetReturnAndExceptionFramesFromCurrent(false, true);
        }
#endif

        this->m_runningFunctionTimeCtr++;
        this->m_callStack.RemoveAtEnd();

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteReturnException(function);
#endif
    }

#if ENABLE_TTD_DEBUGGING
    bool EventLog::HasImmediateReturnFrame() const
    {
        return this->m_isReturnFrame;
    }

    bool EventLog::HasImmediateExceptionFrame() const
    {
        return this->m_isExceptionFrame;
    }

    const SingleCallCounter& EventLog::GetImmediateReturnFrame() const
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");
        AssertMsg(this->m_isReturnFrame, "This data is invalid if we haven't recorded a return!!!");

        return this->m_lastFrame;
    }

    const SingleCallCounter& EventLog::GetImmediateExceptionFrame() const
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");
        AssertMsg(this->m_isExceptionFrame, "This data is invalid if we haven't recorded an exception!!!");

        return this->m_lastFrame;
    }

    void EventLog::ClearReturnFrame()
    {
        this->m_isReturnFrame = false;
    }

    void EventLog::ClearExceptionFrame()
    {
        this->m_isExceptionFrame = false;
    }

    void EventLog::SetReturnAndExceptionFramesFromCurrent(bool setReturn, bool setException)
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");
        AssertMsg(this->m_callStack.Count() != 0, "We must have pushed something in order to have an exception or return!!!");
        AssertMsg((setReturn | setException) & (!setReturn | !setException), "We can only have a return or exception -- exactly one not both!!!");

        this->m_isReturnFrame = setReturn;
        this->m_isExceptionFrame = setException;

        this->m_lastFrame = this->m_callStack.Last();
    }

    bool EventLog::HasPendingTTDBP() const
    {
        return this->m_pendingTTDBP.HasValue();
    }

    int64 EventLog::GetPendingTTDBPTargetEventTime() const
    {
        return this->m_pendingTTDBP.GetRootEventTime();
    }

    void EventLog::GetPendingTTDBPInfo(TTDebuggerSourceLocation& BPLocation) const
    {
        BPLocation.SetLocation(this->m_pendingTTDBP);
    }

    void EventLog::ClearPendingTTDBPInfo()
    {
        this->m_pendingTTDBP.Clear();
    }

    void EventLog::SetPendingTTDBPInfo(const TTDebuggerSourceLocation& BPLocation)
    {
        this->m_pendingTTDBP.SetLocation(BPLocation);
    }

    bool EventLog::HasActiveBP() const
    {
        return this->m_activeBPId != -1;
    }

    UINT EventLog::GetActiveBPId() const
    {
        AssertMsg(this->HasActiveBP(), "Should check this first!!!");

        return (UINT)this->m_activeBPId;
    }

    void EventLog::ClearActiveBP()
    {
        this->m_activeBPId = -1;
        this->m_activeTTDBP.Clear();
    }

    void EventLog::SetActiveBP(UINT bpId, const TTDebuggerSourceLocation& bpLocation)
    {
        this->m_activeBPId = bpId;
        this->m_activeTTDBP.SetLocation(bpLocation);
    }

    bool EventLog::ProcessBPInfoPreBreak()
    {
        if(!this->ShouldPerformDebugAction())
        {
            return true;
        }

        if(!this->HasActiveBP())
        {
            return true;
        }

        const SingleCallCounter& cfinfo = this->GetTopCallCounter();
        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        bool locationOk = ((uint32)srcLine == this->m_activeTTDBP.GetLine()) & ((uint32)srcColumn == this->m_activeTTDBP.GetColumn());
        bool ftimeOk = (this->m_activeTTDBP.GetFunctionTime() == -1) | ((uint64)this->m_activeTTDBP.GetFunctionTime() == cfinfo.FunctionTime);
        bool ltimeOk = (this->m_activeTTDBP.GetLoopTime() == -1) | ((uint64)this->m_activeTTDBP.GetLoopTime() == cfinfo.CurrentStatementLoopTime);

        return locationOk & ftimeOk & ltimeOk;
    }

    void EventLog::ProcessBPInfoPostBreak(Js::FunctionBody* fb)
    {
        if(!this->ShouldPerformDebugAction())
        {
            return;
        }

        if(this->HasActiveBP())
        {
            Js::DebugDocument* debugDocument = fb->GetUtf8SourceInfo()->GetDebugDocument();
            Js::StatementLocation statement;
            if(debugDocument->FindBPStatementLocation(this->GetActiveBPId(), &statement))
            {
                debugDocument->SetBreakPoint(statement, BREAKPOINT_DELETED);
            }

            this->ClearActiveBP();
        }

        if(this->HasPendingTTDBP())
        {
            throw TTD::TTDebuggerAbortException::CreateTopLevelAbortRequest(this->GetPendingTTDBPTargetEventTime(), _u("Reverse operation requested."));
        }
    }
#endif

    void EventLog::UpdateLoopCountInfo()
    {
        AssertMsg(this->IsTTDActive(), "Should check this first.");

        SingleCallCounter& cfinfo = this->m_callStack.Last();
        cfinfo.LoopTime++;
    }

#if ENABLE_TTD_STACK_STMTS
    void EventLog::UpdateCurrentStatementInfo(uint bytecodeOffset)
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
            AssertMsg(cIndex != -1, "Should always have a mapping.");

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

    void EventLog::GetTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        sourceLocation.SetLocation(this->m_topLevelCallbackEventTime, cfinfo.FunctionTime, cfinfo.LoopTime, cfinfo.Function, srcLine, srcColumn);
    }
#endif

#if ENABLE_OBJECT_SOURCE_TRACKING
    void EventLog::GetTimeAndPositionForDiagnosticObjectTracking(DiagnosticOrigin& originInfo) const
    {
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = cfinfo.Function->GetStatementStartOffset(cfinfo.CurrentStatementIndex);
        cfinfo.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        SetDiagnosticOriginInformation(originInfo, srcLine, cfinfo.EventTime, cfinfo.FunctionTime, cfinfo.LoopTime);
    }
#endif 

#if ENABLE_TTD_DEBUGGING
    bool EventLog::GetPreviousTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        const SingleCallCounter& cfinfo = this->GetTopCallCounter();

        //check if we are at the first statement in the callback event
        if(this->m_callStack.Count() == 1 && cfinfo.LastStatementIndex == -1)
        {
            //Set the position info to the current statement and return true
            this->GetTimeAndPositionForDebugger(sourceLocation);

            return true;
        }

        //if we are at the first statement in the function then we want the parents current
        Js::FunctionBody* fbody = nullptr;
        int32 statementIndex = -1;
        uint64 ftime = 0;
        uint64 ltime = 0;
        if(cfinfo.LastStatementIndex == -1)
        {
            const SingleCallCounter& cfinfoCaller = this->GetTopCallCallerCounter();
            ftime = cfinfoCaller.FunctionTime;
            ltime = cfinfoCaller.CurrentStatementLoopTime;

            fbody = cfinfoCaller.Function;
            statementIndex = cfinfoCaller.CurrentStatementIndex;
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

        sourceLocation.SetLocation(this->m_topLevelCallbackEventTime, ftime, ltime, fbody, srcLine, srcColumn);

        return false;
    }

    bool EventLog::GetExceptionTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        if(!this->m_isExceptionFrame)
        {
            sourceLocation.Clear();
            return false;
        }
        else
        {
            ULONG srcLine = 0;
            LONG srcColumn = -1;
            uint32 startOffset = this->m_lastFrame.Function->GetStatementStartOffset(this->m_lastFrame.CurrentStatementIndex);
            this->m_lastFrame.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

            sourceLocation.SetLocation(this->m_topLevelCallbackEventTime, this->m_lastFrame.FunctionTime, this->m_lastFrame.CurrentStatementLoopTime, this->m_lastFrame.Function, srcLine, srcColumn);
            return true;
        }
    }

    bool EventLog::GetImmediateReturnTimeAndPositionForDebugger(TTDebuggerSourceLocation& sourceLocation) const
    {
        if(!this->m_isReturnFrame)
        {
            sourceLocation.Clear();
            return false;
        }
        else
        {
            ULONG srcLine = 0;
            LONG srcColumn = -1;
            uint32 startOffset = this->m_lastFrame.Function->GetStatementStartOffset(this->m_lastFrame.CurrentStatementIndex);
            this->m_lastFrame.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

            sourceLocation.SetLocation(this->m_topLevelCallbackEventTime, this->m_lastFrame.FunctionTime, this->m_lastFrame.CurrentStatementLoopTime, this->m_lastFrame.Function, srcLine, srcColumn);
            return true;
        }
    }

    int64 EventLog::GetCurrentHostCallbackId() const
    {
        return this->m_hostCallbackId;
    }

    int64 EventLog::GetCurrentTopLevelEventTime() const
    {
        return this->m_topLevelCallbackEventTime;
    }

    JsRTCallbackAction* EventLog::GetEventForHostCallbackId(bool wantRegisterOp, int64 hostIdOfInterest) const
    {
        if(hostIdOfInterest == -1)
        {
            return nullptr;
        }

        for(auto iter = this->m_currentReplayEventIterator; iter.IsValid(); iter.MovePrevious())
        {
            if(iter.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag && JsRTActionLogEntry::As(iter.Current())->GetActionTypeTag() == JsRTActionType::CallbackOp)
            {
                JsRTCallbackAction* callbackAction = JsRTCallbackAction::As(JsRTActionLogEntry::As(iter.Current()));
                if(callbackAction->GetAssociatedHostCallbackId() == hostIdOfInterest && callbackAction->IsCreateOp() == wantRegisterOp)
                {
                    return callbackAction;
                }
            }
        }

        return nullptr;
    }

    int64 EventLog::GetKthEventTime(uint32 k) const
    {
        uint32 topLevelCount = 0;
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag && JsRTActionLogEntry::As(iter.Current())->IsRootCallBegin())
            {
                JsRTCallFunctionBeginAction* rootCallAction = JsRTCallFunctionBeginAction::As(JsRTActionLogEntry::As(iter.Current()));
                topLevelCount++;

                if(topLevelCount == k)
                {
                    return rootCallAction->GetEventTime();
                }
            }
        }

        AssertMsg(false, "Bad event index!!!");
        return -1;
    }
#endif

    void EventLog::ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime, int64 hostCallbackId)
    {
        AssertMsg(this->m_callStack.Count() == 0, "We should be at the top-level entry!!!");

        this->m_runningFunctionTimeCtr = 0;
        this->m_topLevelCallbackEventTime = topLevelCallbackEventTime;
        this->m_hostCallbackId = hostCallbackId;

#if ENABLE_TTD_DEBUGGING
        this->ClearReturnFrame();
        this->ClearExceptionFrame();
#endif
    }

    bool EventLog::IsTimeForSnapshot() const
    {
        return (this->m_elapsedExecutionTimeSinceSnapshot > this->m_snapInterval);
    }

    void EventLog::PruneLogLength()
    {
        //
        //TODO: add code to see if we have more snapshots than the specified limit and if so unload them
        //
    }

    void EventLog::IncrementElapsedSnapshotTime(double addtlTime)
    {
        this->m_elapsedExecutionTimeSinceSnapshot += addtlTime;
    }

    void EventLog::AbortReplayReturnToHost()
    {
        throw TTDebuggerAbortException::CreateAbortEndOfLog(_u("End of log reached -- returning to top-level."));
    }

    void EventLog::DoSnapshotExtract()
    {
        AssertMsg(this->m_ttdContext != nullptr, "We aren't actually tracking anything!!!");

        SnapShot* snap = nullptr;
        TTD_LOG_TAG logTag = TTD_INVALID_LOG_TAG;

        this->DoSnapshotExtract_Helper(&snap, &logTag);

        ///////////////////////////
        //Create the event object and addi it to the log

        uint64 etime = this->GetCurrentEventTimeAndAdvance();

        SnapshotEventLogEntry* sevent = this->m_eventSlabAllocator.SlabNew<SnapshotEventLogEntry>(etime, snap, etime, logTag);
        this->InsertEventAtHead(sevent);

        this->m_elapsedExecutionTimeSinceSnapshot = 0.0;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteLiteralMsg("---SNAPSHOT EVENT---\n");
#endif
    }

    void EventLog::DoRtrSnapIfNeeded()
    {
        AssertMsg(this->m_ttdContext != nullptr, "We aren't actually tracking anything!!!");
        AssertMsg(this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag, "Something in wrong with the event position.");
        AssertMsg(JsRTActionLogEntry::As(this->m_currentReplayEventIterator.Current())->IsRootCallBegin(), "Something in wrong with the event position.");

        JsRTCallFunctionBeginAction* rootCall = JsRTCallFunctionBeginAction::As(JsRTActionLogEntry::As(this->m_currentReplayEventIterator.Current()));

        if(!rootCall->HasReadyToRunSnapshotInfo())
        {
            SnapShot* snap = nullptr;
            TTD_LOG_TAG logTag = TTD_INVALID_LOG_TAG;
            this->DoSnapshotExtract_Helper(&snap, &logTag);

            rootCall->SetReadyToRunSnapshotInfo(snap, logTag);
        }
    }

    int64 EventLog::FindSnapTimeForEventTime(int64 targetTime, bool* newCtxsNeeded)
    {
        *newCtxsNeeded = false;
        int64 snapTime = -1;

        for(auto iter = this->m_eventList.GetIteratorAtLast(); iter.IsValid(); iter.MovePrevious())
        {
            if(iter.Current()->GetEventTime() <= targetTime)
            {
                if(iter.Current()->GetEventKind() == EventLogEntry::EventKind::SnapshotTag)
                {
                    snapTime = iter.Current()->GetEventTime();
                    break;
                }

                if(iter.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag && JsRTActionLogEntry::As(iter.Current())->IsRootCallBegin())
                {
                    JsRTCallFunctionBeginAction* rootEntry = JsRTCallFunctionBeginAction::As(JsRTActionLogEntry::As(iter.Current()));

                    if(rootEntry->HasReadyToRunSnapshotInfo())
                    {
                        snapTime = iter.Current()->GetEventTime();
                        break;
                    }
                }
            }
        }

        //if this->m_lastInflateMap then this is the first time we have inflated (otherwise we always nullify and recreate as a pair)
        if(this->m_lastInflateMap != nullptr)
        {
            *newCtxsNeeded = (snapTime != this->m_lastInflateSnapshotTime);
        }

        return snapTime;
    }

    void EventLog::UpdateInflateMapForFreshScriptContexts()
    {
        this->m_ttdContext = nullptr;

        if(this->m_lastInflateMap != nullptr)
        {
            HeapDelete(this->m_lastInflateMap);
            this->m_lastInflateMap = nullptr;
        }
    }

    void EventLog::DoSnapshotInflate(int64 etime)
    {
        //collect anything that is dead
        this->m_threadContext->GetRecycler()->CollectNow<CollectNowForceInThread>();

        const SnapShot* snap = nullptr;
        int64 restoreEventTime = -1;
        TTD_LOG_TAG restoreLogTagCtr = TTD_INVALID_LOG_TAG;

        for(auto iter = this->m_eventList.GetIteratorAtLast(); iter.IsValid(); iter.MovePrevious())
        {
            if(iter.Current()->GetEventTime() == etime)
            {
                if(iter.Current()->GetEventKind() == EventLogEntry::EventKind::SnapshotTag)
                {
                    SnapshotEventLogEntry* snpEntry = SnapshotEventLogEntry::As(iter.Current());
                    snpEntry->EnsureSnapshotDeserialized(this->m_logInfoRootDir.Contents, this->m_threadContext);

                    restoreEventTime = snpEntry->GetRestoreEventTime();
                    restoreLogTagCtr = snpEntry->GetRestoreLogTag();

                    snap = snpEntry->GetSnapshot();
                }
                else
                {
                    JsRTCallFunctionBeginAction* rootEntry = JsRTCallFunctionBeginAction::As(JsRTActionLogEntry::As(iter.Current()));

                    SnapShot* ncSnap = nullptr;
                    rootEntry->GetReadyToRunSnapshotInfo(&ncSnap, &restoreLogTagCtr);
                    snap = ncSnap;

                    restoreEventTime = rootEntry->GetEventTime();
                }

                break;
            }
        }
        AssertMsg(snap != nullptr, "Log should start with a snapshot!!!");

        //
        //TODO: we currently assume a single context here which we load into the existing ctx
        //
        const UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_SMALL>& snpCtxs = snap->GetContextList();
        AssertMsg(this->m_ttdContext != nullptr, "We are assuming a single context");
        const NSSnapValues::SnapContext* sCtx = snpCtxs.GetIterator().Current();

        if(this->m_lastInflateMap != nullptr)
        {
            this->m_lastInflateMap->PrepForReInflate(snap->ContextCount(), snap->HandlerCount(), snap->TypeCount(), snap->PrimitiveCount() + snap->ObjectCount(), snap->BodyCount(), snap->EnvCount(), snap->SlotArrayCount());

            NSSnapValues::InflateScriptContext(sCtx, this->m_ttdContext, this->m_lastInflateMap);
        }
        else
        {
            this->m_lastInflateMap = HeapNew(InflateMap);
            this->m_lastInflateMap->PrepForInitialInflate(this->m_threadContext, snap->ContextCount(), snap->HandlerCount(), snap->TypeCount(), snap->PrimitiveCount() + snap->ObjectCount(), snap->BodyCount(), snap->EnvCount(), snap->SlotArrayCount());
            this->m_lastInflateSnapshotTime = etime;

            NSSnapValues::InflateScriptContext(sCtx, this->m_ttdContext, this->m_lastInflateMap);

            //We don't want to have a bunch of snapshots in memory (that will get big fast) so unload all but the current one
            for(auto iter = this->m_eventList.GetIteratorAtLast(); iter.IsValid(); iter.MovePrevious())
            {
                if(iter.Current()->GetEventTime() != etime)
                {
                    iter.Current()->UnloadSnapshot();
                }
            }
        }

        //reset the tagged object maps before we do the inflate
        this->m_threadContext->TTDInfo->ResetTagsForRestore_TTD(restoreLogTagCtr);
        this->m_eventTimeCtr = restoreEventTime;

        snap->Inflate(this->m_lastInflateMap, sCtx);
        this->m_lastInflateMap->CleanupAfterInflate();

        if(!this->m_eventList.IsEmpty())
        {
            this->m_currentReplayEventIterator = this->m_eventList.GetIteratorAtLast();
            while(this->m_currentReplayEventIterator.Current()->GetEventTime() != this->m_eventTimeCtr)
            {
                this->m_currentReplayEventIterator.MovePrevious();
            }

            //we want to advance to the event immediately after the snapshot as well so do that
            if(this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::SnapshotTag)
            {
                this->m_eventTimeCtr++;
                this->m_currentReplayEventIterator.MoveNext();
            }

            //clear this out -- it shouldn't matter for most JsRT actions (alloc etc.) and should be reset by any call actions
            this->ResetCallStackForTopLevelCall(-1, -1);
        }

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.WriteLiteralMsg("---INFLATED SNAPSHOT---\n");
#endif
    }

    void EventLog::ReplaySingleEntry()
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");

        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTD_LOG_TAG diagnosticsCurrentTag = this->m_threadContext->TTDInfo->GetLogTagValueForDiagnostics();
        const EventLogEntry* diagnosticsEntry = this->m_currentReplayEventIterator.Current();
        AssertMsg(diagnosticsCurrentTag == diagnosticsEntry->DiagnosticEventTagValue, "Log Tag is out of sync!!!");
#endif

        switch(this->m_currentReplayEventIterator.Current()->GetEventKind())
        {
            case EventLogEntry::EventKind::SnapshotTag:
                this->ReplaySnapshotEvent();
                break;
            case EventLogEntry::EventKind::JsRTActionTag:
                this->ReplayActionLoopStep(); 
                break;
            default:
                AssertMsg(false, "Either this is an invalid tag to replay directly (should be driven internally) or it is not known!!!");
        }
    }

    void EventLog::ReplayToTime(int64 eventTime)
    {
        AssertMsg(this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventTime() <= eventTime, "This isn't going to work.");

        //Note use of == in test as we want a specific root event not just sometime later
        while(this->m_currentReplayEventIterator.Current()->GetEventTime() != eventTime)
        {
            this->ReplaySingleEntry();

            AssertMsg(this->m_currentReplayEventIterator.IsValid() && m_currentReplayEventIterator.Current()->GetEventTime() <= eventTime, "Something is not lined up correctly.");
        }
    }

    void EventLog::ReplayFullTrace()
    {
        while(this->m_currentReplayEventIterator.IsValid())
        {
            this->ReplaySingleEntry();
        }

        //we are at end of trace so abort to top level
        this->AbortReplayReturnToHost();
    }

    void EventLog::RecordJsRTVarToObjectConversion(Js::ScriptContext* ctx, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue vval;
        NSLogValue::ExtractArgRetValueFromVar(var, vval, this->m_eventSlabAllocator);

        JsRTVarConvertToObjectAction* convertEvent = this->m_eventSlabAllocator.SlabNew<JsRTVarConvertToObjectAction>(etime, ctxTag, vval);

        this->InsertEventAtHead(convertEvent);
    }

    void EventLog::RecordJsRTCreateSymbol(Js::ScriptContext* ctx, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue vval;
        NSLogValue::ExtractArgRetValueFromVar(var, vval, this->m_eventSlabAllocator);

        JsRTCreateSymbol* createEvent = this->m_eventSlabAllocator.SlabNew<JsRTCreateSymbol>(etime, ctxTag, vval);

        this->InsertEventAtHead(createEvent);
    }

    void EventLog::RecordJsRTAllocateBasicObject(Js::ScriptContext* ctx, bool isRegularObject)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        JsRTObjectAllocateAction* allocEvent = this->m_eventSlabAllocator.SlabNew<JsRTObjectAllocateAction>(etime, ctxTag, isRegularObject);

        this->InsertEventAtHead(allocEvent);
    }

    void EventLog::RecordJsRTAllocateBasicClearArray(Js::ScriptContext* ctx, Js::TypeId arrayType, uint32 length)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        JsRTArrayAllocateAction* allocEvent = this->m_eventSlabAllocator.SlabNew<JsRTArrayAllocateAction>(etime, ctxTag, arrayType, length);

        this->InsertEventAtHead(allocEvent);
    }

    void EventLog::RecordJsRTAllocateArrayBuffer(Js::ScriptContext* ctx, byte* buff, uint32 size)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        byte* abuff = this->m_eventSlabAllocator.SlabAllocateArray<byte>(size);
        js_memcpy_s(abuff, size, buff, size);

        JsRTArrayBufferAllocateAction* allocEvent = this->m_eventSlabAllocator.SlabNew<JsRTArrayBufferAllocateAction>(etime, ctxTag, size, buff);

        this->InsertEventAtHead(allocEvent);
    }

    void EventLog::RecordJsRTAllocateFunction(Js::ScriptContext* ctx, bool isNamed, Js::Var optName)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue name;
        NSLogValue::InitializeArgRetValueAsInvalid(name);

        if(isNamed)
        {
            NSLogValue::ExtractArgRetValueFromVar(optName, name, this->m_eventSlabAllocator);
        }

        JsRTFunctionAllocateAction* allocEvent = this->m_eventSlabAllocator.SlabNew<JsRTFunctionAllocateAction>(etime, ctxTag, isNamed, name);

        this->InsertEventAtHead(allocEvent);
    }

    void EventLog::RecordJsRTGetAndClearException(Js::ScriptContext* ctx)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        JsRTGetAndClearExceptionAction* exceptionEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetAndClearExceptionAction>(etime, ctxTag);

        this->InsertEventAtHead(exceptionEvent);
    }

    void EventLog::RecordJsRTGetProperty(Js::ScriptContext* ctx, Js::PropertyId pid, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue val;
        NSLogValue::ExtractArgRetValueFromVar(var, val, this->m_eventSlabAllocator);

        JsRTGetPropertyAction* getEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetPropertyAction>(etime, ctxTag, pid, val);

        this->InsertEventAtHead(getEvent);
    }

    void EventLog::RecordJsRTGetIndex(Js::ScriptContext* ctx, Js::Var index, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue aindex;
        NSLogValue::ExtractArgRetValueFromVar(index, aindex, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue aval;
        NSLogValue::ExtractArgRetValueFromVar(var, aval, this->m_eventSlabAllocator);

        JsRTGetIndexAction* getEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetIndexAction>(etime, ctxTag, aindex, aval);

        this->InsertEventAtHead(getEvent);
    }

    void EventLog::RecordJsRTGetOwnPropertyInfo(Js::ScriptContext* ctx, Js::PropertyId pid, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue val;
        NSLogValue::ExtractArgRetValueFromVar(var, val, this->m_eventSlabAllocator);

        JsRTGetOwnPropertyInfoAction* getInfoEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetOwnPropertyInfoAction>(etime, ctxTag, pid, val);

        this->InsertEventAtHead(getInfoEvent);
    }

    void EventLog::RecordJsRTGetOwnPropertiesInfo(Js::ScriptContext* ctx, bool isGetNames, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue val;
        NSLogValue::ExtractArgRetValueFromVar(var, val, this->m_eventSlabAllocator);

        JsRTGetOwnPropertiesInfoAction* getInfoEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetOwnPropertiesInfoAction>(etime, ctxTag, isGetNames, val);

        this->InsertEventAtHead(getInfoEvent);
    }

    void EventLog::RecordJsRTDefineProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, Js::Var propertyDescriptor)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue pdval;
        NSLogValue::ExtractArgRetValueFromVar(propertyDescriptor, pdval, this->m_eventSlabAllocator);

        JsRTDefinePropertyAction* defineEvent = this->m_eventSlabAllocator.SlabNew<JsRTDefinePropertyAction>(etime, ctxTag, avar, pid, pdval);

        this->InsertEventAtHead(defineEvent);
    }

    void EventLog::RecordJsRTDeleteProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, bool useStrictRules)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        JsRTDeletePropertyAction* deleteEvent = this->m_eventSlabAllocator.SlabNew<JsRTDeletePropertyAction>(etime, ctxTag, avar, pid, useStrictRules);

        this->InsertEventAtHead(deleteEvent);
    }

    void EventLog::RecordJsRTSetPrototype(Js::ScriptContext* ctx, Js::Var var, Js::Var proto)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue aproto;
        NSLogValue::ExtractArgRetValueFromVar(proto, aproto, this->m_eventSlabAllocator);

        JsRTSetPrototypeAction* setEvent = this->m_eventSlabAllocator.SlabNew<JsRTSetPrototypeAction>(etime, ctxTag, avar, aproto);

        this->InsertEventAtHead(setEvent);
    }

    void EventLog::RecordJsRTSetProperty(Js::ScriptContext* ctx, Js::Var var, Js::PropertyId pid, Js::Var val, bool useStrictRules)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue aval;
        NSLogValue::ExtractArgRetValueFromVar(val, aval, this->m_eventSlabAllocator);

        JsRTSetPropertyAction* setEvent = this->m_eventSlabAllocator.SlabNew<JsRTSetPropertyAction>(etime, ctxTag, avar, pid, aval, useStrictRules);

        this->InsertEventAtHead(setEvent);
    }

    void EventLog::RecordJsRTSetIndex(Js::ScriptContext* ctx, Js::Var var, Js::Var index, Js::Var val)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue aindex;
        NSLogValue::ExtractArgRetValueFromVar(index, aindex, this->m_eventSlabAllocator);

        NSLogValue::ArgRetValue aval;
        NSLogValue::ExtractArgRetValueFromVar(val, aval, this->m_eventSlabAllocator);

        JsRTSetIndexAction* setEvent = this->m_eventSlabAllocator.SlabNew<JsRTSetIndexAction>(etime, ctxTag, avar, aindex, aval);

        this->InsertEventAtHead(setEvent);
    }

    void EventLog::RecordJsRTGetTypedArrayInfo(Js::ScriptContext* ctx, bool returnsArrayBuff, Js::Var var)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        NSLogValue::ArgRetValue avar;
        NSLogValue::ExtractArgRetValueFromVar(var, avar, this->m_eventSlabAllocator);

        JsRTGetTypedArrayInfoAction* infoEvent = this->m_eventSlabAllocator.SlabNew<JsRTGetTypedArrayInfoAction>(etime, ctxTag, returnsArrayBuff, avar);

        this->InsertEventAtHead(infoEvent);
    }

    void EventLog::RecordJsRTConstructCall(Js::ScriptContext* ctx, Js::JavascriptFunction* func, uint32 argCount, Js::Var* args)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);
        TTD_LOG_TAG fTag = ctx->GetThreadContext()->TTDInfo->LookupTagForObject(func);

        NSLogValue::ArgRetValue* argArray = (argCount != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<NSLogValue::ArgRetValue>(argCount) : 0;
        for(uint32 i = 0; i < argCount; ++i)
        {
            Js::Var arg = args[i];
            NSLogValue::ExtractArgRetValueFromVar(arg, argArray[i], this->m_eventSlabAllocator);
        }
        Js::Var* execArgs = (argCount != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<Js::Var>(argCount) : nullptr;

        JsRTConstructCallAction* constructEvent = this->m_eventSlabAllocator.SlabNew<JsRTConstructCallAction>(etime, ctxTag, fTag, argCount, argArray, execArgs);

        this->InsertEventAtHead(constructEvent);
    }

    void EventLog::RecordJsRTCallbackOperation(Js::ScriptContext* ctx, bool isCancel, bool isRepeating, Js::JavascriptFunction* func, int64 callbackId)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);
        TTD_LOG_TAG fTag = (func != nullptr) ? ctx->GetThreadContext()->TTDInfo->LookupTagForObject(func) : TTD_INVALID_LOG_TAG;

        JsRTCallbackAction* createAction = this->m_eventSlabAllocator.SlabNew<JsRTCallbackAction>(etime, ctxTag, isCancel, isRepeating, this->m_hostCallbackId, fTag, callbackId);

        this->InsertEventAtHead(createAction);
    }

    void EventLog::RecordJsRTCodeParse(Js::ScriptContext* ctx, LoadScriptFlag loadFlag, Js::JavascriptFunction* func, LPCWSTR srcCode, LPCWSTR sourceUri)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        Js::FunctionBody* fb = JsSupport::ForceAndGetFunctionBody(func->GetFunctionBody());

        TTString optSrcUri;
        this->m_eventSlabAllocator.CopyNullTermStringInto(fb->GetSourceContextInfo()->url, optSrcUri);

        DWORD_PTR optDocumentID = fb->GetUtf8SourceInfo()->GetSourceInfoId();

        TTString sourceCode;
        this->m_eventSlabAllocator.CopyNullTermStringInto(srcCode, sourceCode);

        TTString dir;
        this->m_eventSlabAllocator.CopyStringIntoWLength(this->m_logInfoRootDir.Contents, this->m_logInfoRootDir.Length, dir);

        TTString ssUri;
        this->m_eventSlabAllocator.CopyNullTermStringInto(sourceUri, ssUri);

        JsRTCodeParseAction* parseEvent = this->m_eventSlabAllocator.SlabNew<JsRTCodeParseAction>(etime, ctxTag, sourceCode, loadFlag, optDocumentID, optSrcUri, dir, ssUri);

        this->InsertEventAtHead(parseEvent);
    }

    JsRTCallFunctionBeginAction* EventLog::RecordJsRTCallFunctionBegin(Js::ScriptContext* ctx, int32 rootDepth, int64 hostCallbackId, double beginTime, Js::JavascriptFunction* func, uint32 argCount, Js::Var* args)
    {
        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);
        TTD_LOG_TAG fTag = ctx->GetThreadContext()->TTDInfo->LookupTagForObject(func);

        NSLogValue::ArgRetValue* argArray = (argCount != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<NSLogValue::ArgRetValue>(argCount) : 0;
        for(uint32 i = 0; i < argCount; ++i)
        {
            Js::Var arg = args[i];
            NSLogValue::ExtractArgRetValueFromVar(arg, argArray[i], this->m_eventSlabAllocator);
        }
        Js::Var* execArgs = (argCount != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<Js::Var>(argCount) : nullptr;

        JsRTCallFunctionBeginAction* callEvent = this->m_eventSlabAllocator.SlabNew<JsRTCallFunctionBeginAction>(etime, ctxTag, rootDepth, hostCallbackId, beginTime, fTag, argCount, argArray, execArgs);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString fname;
        Js::JavascriptString* dname = func->GetDisplayName();
        this->m_eventSlabAllocator.CopyStringIntoWLength(dname->GetSz(), dname->GetLength(), fname);
        callEvent->SetFunctionName(fname);
#endif

        this->InsertEventAtHead(callEvent);

        return callEvent;
    }

    void EventLog::RecordJsRTCallFunctionEnd(Js::ScriptContext* ctx, int64 matchingBeginTime, bool hasScriptException, bool hasTerminatingException, int32 callbackDepth, double endTime)
    {
        AssertMsg(this->ShouldPerformRecordAction(), "Shouldn't be logging during replay!");

        uint64 etime = this->GetCurrentEventTimeAndAdvance();
        TTD_LOG_TAG ctxTag = TTD_EXTRACT_CTX_LOG_TAG(ctx);

        JsRTCallFunctionEndAction* callEvent = this->m_eventSlabAllocator.SlabNew<JsRTCallFunctionEndAction>(etime, ctxTag, matchingBeginTime, hasScriptException, hasTerminatingException, callbackDepth, endTime);

        this->InsertEventAtHead(callEvent);
    }

    void EventLog::ReplayActionLoopStep()
    {
        AssertMsg(this->ShouldPerformDebugAction(), "Mode is inconsistent!");
        AssertMsg(this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag, "Should check this first!");

        bool nextActionValid = false;
        bool nextActionRootCall = false;
        do
        {
            JsRTActionLogEntry* action = JsRTActionLogEntry::As(this->m_currentReplayEventIterator.Current());
            this->AdvanceTimeAndPositionForReplay();

            Js::ScriptContext* ctx = action->GetScriptContextForAction(this->m_threadContext);
            if(action->IsExecutedInScriptWrapper())
            {
                BEGIN_ENTER_SCRIPT(ctx, true, true, true);
                {
                    action->ExecuteAction(this->m_threadContext);
                }
                END_ENTER_SCRIPT;
            }
            else
            {
                action->ExecuteAction(this->m_threadContext);
            }

            nextActionValid = (this->m_currentReplayEventIterator.IsValid() && this->m_currentReplayEventIterator.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag);
            nextActionRootCall = (nextActionValid && JsRTActionLogEntry::As(this->m_currentReplayEventIterator.Current())->IsRootCallBegin());

        } while(nextActionValid & !nextActionRootCall);
    }

    LPCWSTR EventLog::EmitLogIfNeeded()
    {
        //See if we have been running record mode (even if we are suspended for runtime execution) -- if we aren't then we don't want to emit anything
        if((this->m_currentMode & TTDMode::RecordEnabled) != TTDMode::RecordEnabled)
        {
            return _u("Record Disabled -- No Log Written!");
        }

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_diagnosticLogger.ForceFlush();
#endif

        HANDLE logHandle = this->m_threadContext->TTDStreamFunctions.pfGetLogStream(this->m_logInfoRootDir.Contents, false, true);
        TTD_LOG_WRITER writer(logHandle, TTD_COMPRESSED_OUTPUT, this->m_threadContext->TTDStreamFunctions.pfWriteBytesToStream, this->m_threadContext->TTDStreamFunctions.pfFlushAndCloseStream);

        writer.WriteRecordStart();
        writer.AdjustIndent(1);

        TTString archString;
#if defined(_M_IX86)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("x86"), archString);
#elif defined(_M_X64)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("x64"), archString);
#elif defined(_M_ARM)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("arm64"), archString);
#else
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u(L"unknown"), archString);
#endif

        writer.WriteString(NSTokens::Key::arch, archString);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        bool diagEnabled = true;
#else
        bool diagEnabled = false;
#endif

        writer.WriteBool(NSTokens::Key::diagEnabled, diagEnabled, NSTokens::Separator::CommaSeparator);

        uint64 usedSpace = 0;
        uint64 reservedSpace = 0;
        this->m_eventSlabAllocator.ComputeMemoryUsed(&usedSpace, &reservedSpace);

        writer.WriteUInt64(NSTokens::Key::usedMemory, usedSpace, NSTokens::Separator::CommaSeparator);
        writer.WriteUInt64(NSTokens::Key::reservedMemory, reservedSpace, NSTokens::Separator::CommaSeparator);

        uint32 ecount = this->m_eventList.Count();
        writer.WriteLengthValue(ecount, NSTokens::Separator::CommaAndBigSpaceSeparator);

        bool firstEvent = true;
        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            bool isJsRTEndCall = (iter.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag && JsRTActionLogEntry::As(iter.Current())->GetActionTypeTag() == JsRTActionType::CallExistingFunctionEnd);
            bool isExternalEndCall = (iter.Current()->GetEventKind() == EventLogEntry::EventKind::ExternalCallEndTag);
            if(isJsRTEndCall | isExternalEndCall)
            {
                writer.AdjustIndent(-1);
            }

            iter.Current()->EmitEvent(this->m_logInfoRootDir.Contents, &writer, this->m_threadContext, !firstEvent ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);
            firstEvent = false;

            bool isJsRTBeginCall = (iter.Current()->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag && JsRTActionLogEntry::As(iter.Current())->GetActionTypeTag() == JsRTActionType::CallExistingFunctionBegin);
            bool isExternalBeginCall = (iter.Current()->GetEventKind() == EventLogEntry::EventKind::ExternalCallBeginTag);
            if(isJsRTBeginCall | isExternalBeginCall)
            {
                writer.AdjustIndent(1);
            }
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        //we haven't moved the properties to their serialized form them take care of it 
        AssertMsg(this->m_propertyRecordList.Count() == 0, "We only compute this when we are ready to emit.");

        for(auto iter = this->m_propertyRecordPinSet->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::PropertyRecord* pRecord = static_cast<Js::PropertyRecord*>(iter.CurrentValue());
            NSSnapType::SnapPropertyRecord* sRecord = this->m_propertyRecordList.NextOpenEntry();

            sRecord->PropertyId = pRecord->GetPropertyId();
            sRecord->IsNumeric = pRecord->IsNumeric();
            sRecord->IsBound = pRecord->IsBound();
            sRecord->IsSymbol = pRecord->IsSymbol();

            this->m_miscSlabAllocator.CopyStringIntoWLength(pRecord->GetBuffer(), pRecord->GetLength(), sRecord->PropertyName);
        }

        //emit the properties
        writer.WriteLengthValue(this->m_propertyRecordList.Count(), NSTokens::Separator::CommaSeparator);

        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        bool firstProperty = true;
        for(auto iter = this->m_propertyRecordList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSTokens::Separator sep = (!firstProperty) ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSSnapType::EmitSnapPropertyRecord(iter.Current(), &writer, sep);

            firstProperty = false;
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        writer.AdjustIndent(-1);
        writer.WriteRecordEnd(NSTokens::Separator::BigSpaceSeparator);

        writer.FlushAndClose();

        return this->m_logInfoRootDir.Contents;
    }

    void EventLog::ParseLogInto()
    {
        HANDLE logHandle = this->m_threadContext->TTDStreamFunctions.pfGetLogStream(this->m_logInfoRootDir.Contents, true, false);
        TTD_LOG_READER reader(logHandle, TTD_COMPRESSED_OUTPUT, this->m_threadContext->TTDStreamFunctions.pfReadBytesFromStream, this->m_threadContext->TTDStreamFunctions.pfFlushAndCloseStream);

        reader.ReadRecordStart();

        TTString archString;
        reader.ReadString(NSTokens::Key::arch, this->m_miscSlabAllocator, archString);

#if defined(_M_IX86)
        AssertMsg(wcscmp(_u("x86"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#elif defined(_M_X64)
        AssertMsg(wcscmp(_u("x64"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#elif defined(_M_ARM)
        AssertMsg(wcscmp(_u("arm64"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#else
        AssertMsg(false, "Unknown arch!!!");
#endif

        bool diagEnabled = reader.ReadBool(NSTokens::Key::diagEnabled, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        AssertMsg(diagEnabled, "Diag was enabled in record so it shoud be in replay as well!!!");
#else
        AssertMsg(!diagEnabled, "Diag was *not* enabled in record so it shoud *not* be in replay either!!!");
#endif

        reader.ReadUInt64(NSTokens::Key::usedMemory, true);
        reader.ReadUInt64(NSTokens::Key::reservedMemory, true);

        uint32 ecount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < ecount; ++i)
        {
            EventLogEntry* curr = EventLogEntry::Parse(i != 0, this->m_threadContext, &reader, this->m_eventSlabAllocator);

            this->m_eventList.AddEntry(curr);
        }
        reader.ReadSequenceEnd();

        //parse the properties
        uint32 propertyCount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < propertyCount; ++i)
        {
            NSSnapType::SnapPropertyRecord* sRecord = this->m_propertyRecordList.NextOpenEntry();
            NSSnapType::ParseSnapPropertyRecord(sRecord, i != 0, &reader, this->m_miscSlabAllocator);
        }
        reader.ReadSequenceEnd();

        reader.ReadRecordEnd();
    }
}

#endif
