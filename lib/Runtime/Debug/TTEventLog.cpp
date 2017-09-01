//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

#define TTD_CREATE_EVENTLIST_VTABLE_ENTRY(TAG, WRAPPER, TYPE, EXEC_FP, UNLOAD_FP, EMIT_FP, PARSE_FP) this->m_eventListVTable[(uint32)NSLogEvents::EventKind:: ## TAG] = { NSLogEvents::ContextExecuteKind:: ## WRAPPER, EXEC_FP, UNLOAD_FP, EMIT_FP, PARSE_FP, TTD_EVENT_PLUS_DATA_SIZE_DIRECT(sizeof(NSLogEvents:: TYPE)) }
#define TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(TAG, WRAPPER, TYPE, EXEC_FP) this->m_eventListVTable[(uint32)NSLogEvents::EventKind:: ## TAG] = { NSLogEvents::ContextExecuteKind:: ## WRAPPER, NSLogEvents:: ## EXEC_FP, nullptr, NSLogEvents:: ## TYPE ## _Emit ## <NSLogEvents::EventKind:: ## TAG ## >, NSLogEvents:: ## TYPE ## _Parse ## <NSLogEvents::EventKind:: ## TAG ## >, TTD_EVENT_PLUS_DATA_SIZE_DIRECT(sizeof(NSLogEvents:: ## TYPE)) }

namespace TTD
{
    TTDJsRTFunctionCallActionPopperRecorder::TTDJsRTFunctionCallActionPopperRecorder()
        : m_ctx(nullptr), m_beginTime(0.0), m_callAction(nullptr)
    {
        ;
    }

    TTDJsRTFunctionCallActionPopperRecorder::~TTDJsRTFunctionCallActionPopperRecorder()
    {
        if(this->m_ctx != nullptr)
        {
            TTDAssert(this->m_callAction != nullptr, "Should be set in sync with ctx!!!");

            TTD::EventLog* elog = this->m_ctx->GetThreadContext()->TTDLog;
            NSLogEvents::JsRTCallFunctionAction* cfAction = NSLogEvents::GetInlineEventDataAs<NSLogEvents::JsRTCallFunctionAction, NSLogEvents::EventKind::CallExistingFunctionActionTag>(this->m_callAction);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            NSLogEvents::JsRTCallFunctionAction_ProcessDiagInfoPost(this->m_callAction, this->m_ctx->GetThreadContext()->TTDLog->GetLastEventTime());
#endif

            //Update the time elapsed since a snapshot if needed
            if(cfAction->CallbackDepth == 0)
            {
                double elapsedTime = (elog->GetCurrentWallTime() - this->m_beginTime);
                elog->IncrementElapsedSnapshotTime(elapsedTime);
            }
        }
    }

    void TTDJsRTFunctionCallActionPopperRecorder::InitializeForRecording(Js::ScriptContext* ctx, double beginWallTime, NSLogEvents::EventLogEntry* callAction)
    {
        TTDAssert(this->m_ctx == nullptr && this->m_callAction == nullptr, "Don't double initialize!!!");

        this->m_ctx = ctx;
        this->m_beginTime = beginWallTime;
        this->m_callAction = callAction;
    }

    /////////////

    void TTEventList::AddArrayLink()
    {
        TTEventListLink* newHeadBlock = this->m_alloc->SlabAllocateStruct<TTEventListLink>();
        newHeadBlock->BlockData = this->m_alloc->SlabAllocateFixedSizeArray<byte, TTD_EVENTLOG_LIST_BLOCK_SIZE>();
        memset(newHeadBlock->BlockData, 0, TTD_EVENTLOG_LIST_BLOCK_SIZE);

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
        TTDAssert(block->Previous == nullptr, "Not first event block in log!!!");
        TTDAssert(block->StartPos == block->CurrPos, "Haven't cleared all the events in this link");

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
        : m_alloc(alloc), m_headBlock(nullptr), m_vtable(nullptr), m_previousEventMap(&HeapAllocator::Instance)
    {
        ;
    }

    void TTEventList::SetVTable(const NSLogEvents::EventLogEntryVTableEntry* vtable)
    {
        this->m_vtable = vtable;
    }

    void TTEventList::InitializePreviousEventMap()
    {
        for(TTEventListLink* curr = this->m_headBlock; curr != nullptr; curr = curr->Previous)
        {
            size_t cpos = curr->StartPos;
            size_t ppos = TTD_EVENTLOG_LIST_BLOCK_SIZE; //an invalid sentinal value

            while(cpos != curr->CurrPos)
            {
                NSLogEvents::EventLogEntry* data = reinterpret_cast<NSLogEvents::EventLogEntry*>(curr->BlockData + cpos);
                if(cpos != curr->StartPos)
                {
                    this->m_previousEventMap.AddNew(data, ppos);
                }

                ppos = cpos;
                cpos += this->m_vtable[(uint32)data->EventKind].DataSize;
            }
        }
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
            size_t cpos = curr->StartPos;
            while(cpos < curr->CurrPos)
            {
                NSLogEvents::EventLogEntry* entry = reinterpret_cast<NSLogEvents::EventLogEntry*>(curr->BlockData + cpos);
                auto unloadFP = this->m_vtable[(uint32)entry->EventKind].UnloadFP; //use vtable magic here

                if(unloadFP != nullptr)
                {
                    unloadFP(entry, *(this->m_alloc));
                }

                cpos += this->m_vtable[(uint32)entry->EventKind].DataSize;
            }
            curr->StartPos = curr->CurrPos;

            TTEventListLink* next = curr->Next;
            this->RemoveArrayLink(curr);
            curr = next;
        }

        this->m_headBlock = nullptr;
    }

    NSLogEvents::EventLogEntry* TTEventList::GetNextAvailableEntry(size_t requiredSize)
    {
        if((this->m_headBlock == nullptr) || (this->m_headBlock->CurrPos + requiredSize >= TTD_EVENTLOG_LIST_BLOCK_SIZE))
        {
            this->AddArrayLink();
        }

        NSLogEvents::EventLogEntry* entry = reinterpret_cast<NSLogEvents::EventLogEntry*>(this->m_headBlock->BlockData + this->m_headBlock->CurrPos);
        this->m_headBlock->CurrPos += requiredSize;

        return entry;
    }

    void TTEventList::DeleteFirstEntry(TTEventListLink* block, NSLogEvents::EventLogEntry* data)
    {
        TTDAssert(reinterpret_cast<NSLogEvents::EventLogEntry*>(block->BlockData + block->StartPos) == data, "Not the data at the start of the list!!!");

        auto unloadFP = this->m_vtable[(uint32)data->EventKind].UnloadFP; //use vtable magic here

        if(unloadFP != nullptr)
        {
            unloadFP(data, *(this->m_alloc));
        }

        block->StartPos += this->m_vtable[(uint32)data->EventKind].DataSize;
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
            size_t cpos = curr->StartPos; 
            while(cpos != curr->CurrPos)
            {
                count++;

                NSLogEvents::EventLogEntry* data = reinterpret_cast<NSLogEvents::EventLogEntry*>(curr->BlockData + cpos);
                cpos += this->m_vtable[(uint32)data->EventKind].DataSize;
            }
        }

        return count;
    }

    TTEventList::Iterator::Iterator()
        : m_currLink(nullptr), m_currIdx(0), m_previousEventMap(nullptr)
    {
        ;
    }

    TTEventList::Iterator::Iterator(TTEventListLink* head, size_t pos, const NSLogEvents::EventLogEntryVTableEntry* vtable, const JsUtil::BaseDictionary<const NSLogEvents::EventLogEntry*, size_t, HeapAllocator>* previousEventMap)
        : m_currLink(head), m_currIdx(pos), m_vtable(vtable), m_previousEventMap(previousEventMap)
    {
        ;
    }

    const NSLogEvents::EventLogEntry* TTEventList::Iterator::Current() const
    {
        TTDAssert(this->IsValid(), "Iterator is invalid!!!");

        return reinterpret_cast<const NSLogEvents::EventLogEntry*>(this->m_currLink->BlockData + this->m_currIdx);
    }

    NSLogEvents::EventLogEntry* TTEventList::Iterator::Current()
    {
        TTDAssert(this->IsValid(), "Iterator is invalid!!!");

        return reinterpret_cast<NSLogEvents::EventLogEntry*>(this->m_currLink->BlockData + this->m_currIdx);
    }

    TTEventList::TTEventListLink* TTEventList::Iterator::GetBlock()
    {
        return this->m_currLink;
    }

    bool TTEventList::Iterator::IsValid() const
    {
        return (this->m_currLink != nullptr && this->m_currLink->StartPos <= this->m_currIdx && this->m_currIdx < this->m_currLink->CurrPos);
    }

    void TTEventList::Iterator::MoveNext()
    {
        NSLogEvents::EventLogEntry* data = this->Current();
        size_t dataSize = this->m_vtable[(uint32)data->EventKind].DataSize;

        if(this->m_currIdx + dataSize < this->m_currLink->CurrPos)
        {
            this->m_currIdx += dataSize;
        }
        else
        {
            this->m_currLink = this->m_currLink->Next;
            this->m_currIdx = (this->m_currLink != nullptr) ? this->m_currLink->StartPos : 0;
        }
    }

    void TTEventList::Iterator::MovePrevious_ReplayOnly()
    {
        if(this->m_currIdx > this->m_currLink->StartPos)
        {
            this->m_currIdx = this->m_previousEventMap->Item(this->Current());
        }
        else
        {
            this->m_currLink = this->m_currLink->Previous;
            this->m_currIdx = 0;

            //move index to the last element
            if(this->m_currLink != nullptr && this->m_currIdx < this->m_currLink->CurrPos)
            {
                NSLogEvents::EventLogEntry* data = this->Current();
                size_t npos = this->m_vtable[(uint32)data->EventKind].DataSize;
                while(npos < this->m_currLink->CurrPos)
                {
                    this->m_currIdx = npos;
                    data = this->Current();
                    npos += this->m_vtable[(uint32)data->EventKind].DataSize;
                }
            }
        }
    }

    TTEventList::Iterator TTEventList::GetIteratorAtFirst() const
    {
        if(this->m_headBlock == nullptr)
        {
            return Iterator(nullptr, 0, this->m_vtable, &this->m_previousEventMap);
        }
        else
        {
            TTEventListLink* firstBlock = this->m_headBlock;
            while(firstBlock->Previous != nullptr)
            {
                firstBlock = firstBlock->Previous;
            }

            return Iterator(firstBlock, firstBlock->StartPos, this->m_vtable, &this->m_previousEventMap);
        }
    }

    TTEventList::Iterator TTEventList::GetIteratorAtLast_ReplayOnly() const
    {
        if(this->m_headBlock == nullptr)
        {
            return Iterator(nullptr, 0, this->m_vtable, &this->m_previousEventMap);
        }
        else
        {
            size_t cpos = this->m_headBlock->StartPos;
            size_t ipos = 0;
            do
            {
                ipos = cpos;

                NSLogEvents::EventLogEntry* data = reinterpret_cast<NSLogEvents::EventLogEntry*>(this->m_headBlock->BlockData + cpos);
                cpos += this->m_vtable[(uint32)data->EventKind].DataSize;
            } while(cpos != this->m_headBlock->CurrPos);

            return Iterator(this->m_headBlock, ipos, this->m_vtable, &this->m_previousEventMap);
        }
    }

    //////

    void EventLog::AdvanceTimeAndPositionForReplay()
    {
        this->m_eventTimeCtr++;
        this->m_currentReplayEventIterator.MoveNext();

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTDAssert(!this->m_currentReplayEventIterator.IsValid() || this->m_eventTimeCtr == this->m_currentReplayEventIterator.Current()->EventTimeStamp, "Something is out of sync.");
#endif
    }

    void EventLog::UpdateComputedMode()
    {
        TTDAssert(this->m_modeStack.Count() > 0, "Should never be empty!!!");

        TTDMode cm = TTDMode::Invalid;
        for(uint32 i = 0; i < this->m_modeStack.Count(); ++i)
        {
            TTDMode m = this->m_modeStack.GetAt(i);
            switch(m)
            {
            case TTDMode::RecordMode:
            case TTDMode::ReplayMode:
            case TTDMode::DebuggerMode:
                TTDAssert(i == 0, "One of these should always be first on the stack.");
                cm = m;
                break;
            case TTDMode::CurrentlyEnabled:
            case TTDMode::ExcludedExecutionTTAction:
            case TTDMode::ExcludedExecutionDebuggerAction:
            case TTDMode::DebuggerSuppressGetter:
            case TTDMode::DebuggerSuppressBreakpoints:
            case TTDMode::DebuggerLogBreakpoints:
                TTDAssert(i != 0, "A base mode should always be first on the stack.");
                cm |= m;
                break;
            default:
                TTDAssert(false, "This mode is unknown or should never appear.");
                break;
            }
        }

        this->m_currentMode = cm;

        //Set fast path values on ThreadContext
        const JsUtil::List<Js::ScriptContext*, HeapAllocator>& contexts = this->m_threadContext->TTDContext->GetTTDContexts();
        for(int32 i = 0; i < contexts.Count(); ++i)
        {
            this->SetModeFlagsOnContext(contexts.Item(i));
        }
    }

    SnapShot* EventLog::DoSnapshotExtract_Helper(double gcTime, JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies)
    {
        SnapShot* snap = nullptr;

        //Begin the actual snapshot operation
        this->m_snapExtractor.BeginSnapshot(this->m_threadContext, gcTime);
        this->m_snapExtractor.DoMarkWalk(this->m_threadContext);

        ///////////////////////////
        //Phase 2: Evacuate marked objects
        //Allows for parallel execute and evacuate (in conjunction with later refactoring)

        this->m_snapExtractor.EvacuateMarkedIntoSnapshot(this->m_threadContext, liveTopLevelBodies);

        ///////////////////////////
        //Phase 3: Complete and return snapshot

        snap = this->m_snapExtractor.CompleteSnapshot();

        return snap;
    }

    void EventLog::ReplaySnapshotEvent()
    {
        SnapShot* snap = nullptr;
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

            //clear the weak collection pin set and force a GC (to get weak containers in a consistent state)
            NSLogEvents::EventLogEntry* revt = this->m_currentReplayEventIterator.Current();
            NSLogEvents::SnapshotEventLogEntry* snapUpdateEvt = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(revt);

            this->m_threadContext->TTDContext->SyncCtxtsAndRootsWithSnapshot_Replay(snapUpdateEvt->LiveContextCount, snapUpdateEvt->LiveContextIdArray, snapUpdateEvt->LongLivedRefRootsCount, snapUpdateEvt->LongLivedRefRootsIdArray);
            this->m_threadContext->GetRecycler()->CollectNow<CollectNowForceInThread>();

            //need to do a visit of some sort to reset the weak collection pin set
            this->m_snapExtractor.DoResetWeakCollectionPinSet(this->m_threadContext);

            //We always need to cleanup references (above but only do compare in extra diagnostics mode)
#if ENABLE_SNAPSHOT_COMPARE
            this->SetSnapshotOrInflateInProgress(true);
            this->PushMode(TTDMode::ExcludedExecutionTTAction);

            JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator> liveTopLevelBodies(&HeapAllocator::Instance);
            snap = this->DoSnapshotExtract_Helper(0.0, liveTopLevelBodies);

            for(int32 i = 0; i < this->m_threadContext->TTDContext->GetTTDContexts().Count(); ++i)
            {
                this->m_threadContext->TTDContext->GetTTDContexts().Item(i)->TTDContextInfo->CleanUnreachableTopLevelBodies(liveTopLevelBodies);
            }

            NSLogEvents::EventLogEntry* evt = this->m_currentReplayEventIterator.Current();
            NSLogEvents::SnapshotEventLogEntry_EnsureSnapshotDeserialized(evt, this->m_threadContext);

            const NSLogEvents::SnapshotEventLogEntry* recordedSnapEntry = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(evt);
            const SnapShot* recordedSnap = recordedSnapEntry->Snap;

            TTDCompareMap compareMap(this->m_threadContext);
            SnapShot::InitializeForSnapshotCompare(recordedSnap, snap, compareMap);
            SnapShot::DoSnapshotCompare(recordedSnap, snap, compareMap);

            TT_HEAP_DELETE(SnapShot, snap);

            this->PopMode(TTDMode::ExcludedExecutionTTAction);
            this->SetSnapshotOrInflateInProgress(false);
#endif
        }
        catch(...)
        {
            if(snap != nullptr)
            {
                TT_HEAP_DELETE(SnapShot, snap);
            }
            TTDAssert(false, "OOM in snapshot replay...");
        }

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteLiteralMsg("---SNAPSHOT EVENT---\n");
#endif

        this->AdvanceTimeAndPositionForReplay(); //move along
    }

    void EventLog::ReplayEventLoopYieldPointEvent()
    {
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

            this->m_threadContext->TTDContext->ClearLocalRootsAndRefreshMap_Replay();
        }
        catch(...)
        {
            TTDAssert(false, "OOM in yield point replay...");
        }

        this->AdvanceTimeAndPositionForReplay(); //move along
    }

    void EventLog::AbortReplayReturnToHost()
    {
        throw TTDebuggerAbortException::CreateAbortEndOfLog(_u("End of log reached -- returning to top-level."));
    }

    void EventLog::InitializeEventListVTable()
    {
        this->m_eventListVTable = this->m_miscSlabAllocator.SlabAllocateArray<NSLogEvents::EventLogEntryVTableEntry>((uint32)NSLogEvents::EventKind::Count);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(SnapshotTag, GlobalAPIWrapper, SnapshotEventLogEntry, nullptr, NSLogEvents::SnapshotEventLogEntry_UnloadEventMemory, NSLogEvents::SnapshotEventLogEntry_Emit, NSLogEvents::SnapshotEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(EventLoopYieldPointTag, GlobalAPIWrapper, EventLoopYieldPointEntry, nullptr, nullptr, NSLogEvents::EventLoopYieldPointEntry_Emit, NSLogEvents::EventLoopYieldPointEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(TopLevelCodeTag, None, CodeLoadEventLogEntry, nullptr, nullptr, NSLogEvents::CodeLoadEventLogEntry_Emit, NSLogEvents::CodeLoadEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(TelemetryLogTag, None, TelemetryEventLogEntry, nullptr, NSLogEvents::TelemetryEventLogEntry_UnloadEventMemory, NSLogEvents::TelemetryEventLogEntry_Emit, NSLogEvents::TelemetryEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(DoubleTag, None, DoubleEventLogEntry, nullptr, nullptr, NSLogEvents::DoubleEventLogEntry_Emit, NSLogEvents::DoubleEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(StringTag, None, StringValueEventLogEntry, nullptr, NSLogEvents::StringValueEventLogEntry_UnloadEventMemory, NSLogEvents::StringValueEventLogEntry_Emit, NSLogEvents::StringValueEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(RandomSeedTag, None, RandomSeedEventLogEntry, nullptr, nullptr, NSLogEvents::RandomSeedEventLogEntry_Emit, NSLogEvents::RandomSeedEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(PropertyEnumTag, None, PropertyEnumStepEventLogEntry, nullptr, NSLogEvents::PropertyEnumStepEventLogEntry_UnloadEventMemory, NSLogEvents::PropertyEnumStepEventLogEntry_Emit, NSLogEvents::PropertyEnumStepEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(SymbolCreationTag, None, SymbolCreationEventLogEntry, nullptr, nullptr, NSLogEvents::SymbolCreationEventLogEntry_Emit, NSLogEvents::SymbolCreationEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(WeakCollectionContainsTag, None, WeakCollectionContainsEventLogEntry, nullptr, nullptr, NSLogEvents::WeakCollectionContainsEventLogEntry_Emit, NSLogEvents::WeakCollectionContainsEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(ExternalCbRegisterCall, None, ExternalCbRegisterCallEventLogEntry, nullptr, nullptr, NSLogEvents::ExternalCbRegisterCallEventLogEntry_Emit, NSLogEvents::ExternalCbRegisterCallEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(ExternalCallTag, None, ExternalCallEventLogEntry, nullptr, NSLogEvents::ExternalCallEventLogEntry_UnloadEventMemory, NSLogEvents::ExternalCallEventLogEntry_Emit, NSLogEvents::ExternalCallEventLogEntry_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(ExplicitLogWriteTag, None, ExplicitLogWriteEventLogEntry, nullptr, nullptr, NSLogEvents::ExplicitLogWriteEntry_Emit, NSLogEvents::ExplicitLogWriteEntry_Parse);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(CreateScriptContextActionTag, GlobalAPIWrapper, JsRTCreateScriptContextAction, NSLogEvents::CreateScriptContext_Execute, NSLogEvents::CreateScriptContext_UnloadEventMemory, NSLogEvents::CreateScriptContext_Emit, NSLogEvents::CreateScriptContext_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(SetActiveScriptContextActionTag, GlobalAPIWrapper, JsRTSingleVarArgumentAction, SetActiveScriptContext_Execute);

#if !INT32VAR
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateIntegerActionTag, ContextAPINoScriptWrapper, JsRTIntegralArgumentAction, CreateInt_Execute);
#endif

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(CreateNumberActionTag, ContextAPINoScriptWrapper, JsRTDoubleArgumentAction, NSLogEvents::CreateNumber_Execute, nullptr, NSLogEvents::JsRTDoubleArgumentAction_Emit<NSLogEvents::EventKind::CreateNumberActionTag>, NSLogEvents::JsRTDoubleArgumentAction_Parse<NSLogEvents::EventKind::CreateNumberActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateBooleanActionTag, ContextAPINoScriptWrapper, JsRTIntegralArgumentAction, CreateBoolean_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(CreateStringActionTag, ContextAPINoScriptWrapper, JsRTStringArgumentAction, NSLogEvents::CreateString_Execute, NSLogEvents::JsRTStringArgumentAction_UnloadEventMemory<NSLogEvents::EventKind::CreateStringActionTag>, NSLogEvents::JsRTStringArgumentAction_Emit<NSLogEvents::EventKind::CreateStringActionTag>, NSLogEvents::JsRTStringArgumentAction_Parse<NSLogEvents::EventKind::CreateStringActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateSymbolActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateSymbol_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateErrorActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateRangeErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateRangeErrorActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateReferenceErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateReferenceErrorActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateSyntaxErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateSyntaxErrorActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateTypeErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateTypeErrorActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(CreateURIErrorActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, CreateError_Execute<NSLogEvents::EventKind::CreateURIErrorActionTag>);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(VarConvertToNumberActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, VarConvertToNumber_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(VarConvertToBooleanActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, VarConvertToBoolean_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(VarConvertToStringActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, VarConvertToString_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(VarConvertToObjectActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, VarConvertToObject_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AddRootRefActionTag, GlobalAPIWrapper, JsRTSingleVarArgumentAction, AddRootRef_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AddWeakRootRefActionTag, GlobalAPIWrapper, JsRTSingleVarArgumentAction, AddWeakRootRef_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AllocateObjectActionTag, ContextAPINoScriptWrapper, JsRTResultOnlyAction, AllocateObject_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AllocateExternalObjectActionTag, ContextAPINoScriptWrapper, JsRTResultOnlyAction, AllocateExternalObject_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AllocateArrayActionTag, ContextAPINoScriptWrapper, JsRTIntegralArgumentAction, AllocateArrayAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AllocateArrayBufferActionTag, ContextAPIWrapper, JsRTIntegralArgumentAction, AllocateArrayBufferAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(AllocateExternalArrayBufferActionTag, ContextAPINoScriptWrapper, JsRTByteBufferAction, NSLogEvents::AllocateExternalArrayBufferAction_Execute, NSLogEvents::JsRTByteBufferAction_UnloadEventMemory<NSLogEvents::EventKind::AllocateExternalArrayBufferActionTag>, NSLogEvents::JsRTByteBufferAction_Emit<NSLogEvents::EventKind::AllocateExternalArrayBufferActionTag>, NSLogEvents::JsRTByteBufferAction_Parse<NSLogEvents::EventKind::AllocateExternalArrayBufferActionTag>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(AllocateFunctionActionTag, ContextAPIWrapper, JsRTSingleVarScalarArgumentAction, AllocateFunctionAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(HostExitProcessTag, ContextAPIWrapper, JsRTIntegralArgumentAction, HostProcessExitAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetAndClearExceptionWithMetadataActionTag, None, JsRTResultOnlyAction, GetAndClearExceptionWithMetadataAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetAndClearExceptionActionTag, None, JsRTResultOnlyAction, GetAndClearExceptionAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(SetExceptionActionTag, ContextAPINoScriptWrapper, JsRTSingleVarScalarArgumentAction, SetExceptionAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(HasPropertyActionTag, ContextAPIWrapper, JsRTSingleVarScalarArgumentAction, HasPropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(HasOwnPropertyActionTag, ContextAPIWrapper, JsRTSingleVarScalarArgumentAction, HasOwnPropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(InstanceOfActionTag, ContextAPIWrapper, JsRTDoubleVarArgumentAction, InstanceOfAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(EqualsActionTag, ContextAPIWrapper, JsRTDoubleVarSingleScalarArgumentAction, EqualsAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetPropertyIdFromSymbolTag, ContextAPINoScriptWrapper, JsRTSingleVarArgumentAction, GetPropertyIdFromSymbolAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetPrototypeActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, GetPrototypeAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetPropertyActionTag, ContextAPIWrapper, JsRTSingleVarScalarArgumentAction, GetPropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetIndexActionTag, ContextAPIWrapper, JsRTDoubleVarArgumentAction, GetIndexAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetOwnPropertyInfoActionTag, ContextAPIWrapper, JsRTSingleVarScalarArgumentAction, GetOwnPropertyInfoAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetOwnPropertyNamesInfoActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, GetOwnPropertyNamesInfoAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetOwnPropertySymbolsInfoActionTag, ContextAPIWrapper, JsRTSingleVarArgumentAction, GetOwnPropertySymbolsInfoAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(DefinePropertyActionTag, ContextAPIWrapper, JsRTDoubleVarSingleScalarArgumentAction, DefinePropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(DeletePropertyActionTag, ContextAPIWrapper, JsRTSingleVarDoubleScalarArgumentAction, DeletePropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(SetPrototypeActionTag, ContextAPIWrapper, JsRTDoubleVarArgumentAction, SetPrototypeAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(SetPropertyActionTag, ContextAPIWrapper, JsRTDoubleVarDoubleScalarArgumentAction, SetPropertyAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(SetIndexActionTag, ContextAPIWrapper, JsRTTrippleVarArgumentAction, SetIndexAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetTypedArrayInfoActionTag, None, JsRTSingleVarArgumentAction, GetTypedArrayInfoAction_Execute);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY_COMMON(GetDataViewInfoActionTag, None, JsRTSingleVarArgumentAction, GetDataViewInfoAction_Execute);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(RawBufferCopySync, ContextAPIWrapper, JsRTRawBufferCopyAction, NSLogEvents::RawBufferCopySync_Execute, nullptr, NSLogEvents::JsRTRawBufferCopyAction_Emit, NSLogEvents::JsRTRawBufferCopyAction_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(RawBufferModifySync, ContextAPIWrapper, JsRTRawBufferModifyAction, NSLogEvents::RawBufferModifySync_Execute, NSLogEvents::JsRTRawBufferModifyAction_UnloadEventMemory<NSLogEvents::EventKind::RawBufferModifySync>, NSLogEvents::JsRTRawBufferModifyAction_Emit<NSLogEvents::EventKind::RawBufferModifySync>, NSLogEvents::JsRTRawBufferModifyAction_Parse<NSLogEvents::EventKind::RawBufferModifySync>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(RawBufferAsyncModificationRegister, ContextAPIWrapper, JsRTRawBufferModifyAction, NSLogEvents::RawBufferAsyncModificationRegister_Execute, NSLogEvents::JsRTRawBufferModifyAction_UnloadEventMemory<NSLogEvents::EventKind::RawBufferAsyncModificationRegister>, NSLogEvents::JsRTRawBufferModifyAction_Emit<NSLogEvents::EventKind::RawBufferAsyncModificationRegister>, NSLogEvents::JsRTRawBufferModifyAction_Parse<NSLogEvents::EventKind::RawBufferAsyncModificationRegister>);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(RawBufferAsyncModifyComplete, ContextAPIWrapper, JsRTRawBufferModifyAction, NSLogEvents::RawBufferAsyncModifyComplete_Execute, NSLogEvents::JsRTRawBufferModifyAction_UnloadEventMemory<NSLogEvents::EventKind::RawBufferAsyncModifyComplete>, NSLogEvents::JsRTRawBufferModifyAction_Emit<NSLogEvents::EventKind::RawBufferAsyncModifyComplete>, NSLogEvents::JsRTRawBufferModifyAction_Parse<NSLogEvents::EventKind::RawBufferAsyncModifyComplete>);

        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(ConstructCallActionTag, ContextAPIWrapper, JsRTConstructCallAction, NSLogEvents::JsRTConstructCallAction_Execute, NSLogEvents::JsRTConstructCallAction_UnloadEventMemory, NSLogEvents::JsRTConstructCallAction_Emit, NSLogEvents::JsRTConstructCallAction_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(CodeParseActionTag, ContextAPINoScriptWrapper, JsRTCodeParseAction, NSLogEvents::JsRTCodeParseAction_Execute, NSLogEvents::JsRTCodeParseAction_UnloadEventMemory, NSLogEvents::JsRTCodeParseAction_Emit, NSLogEvents::JsRTCodeParseAction_Parse);
        TTD_CREATE_EVENTLIST_VTABLE_ENTRY(CallExistingFunctionActionTag, ContextAPIWrapper, JsRTCallFunctionAction, NSLogEvents::JsRTCallFunctionAction_Execute, NSLogEvents::JsRTCallFunctionAction_UnloadEventMemory, NSLogEvents::JsRTCallFunctionAction_Emit, NSLogEvents::JsRTCallFunctionAction_Parse);
    }

    EventLog::EventLog(ThreadContext* threadContext)
        : m_threadContext(threadContext), m_eventSlabAllocator(TTD_SLAB_BLOCK_ALLOCATION_SIZE_MID), m_miscSlabAllocator(TTD_SLAB_BLOCK_ALLOCATION_SIZE_SMALL),
        m_eventTimeCtr(0), m_timer(), m_topLevelCallbackEventTime(-1),
        m_eventListVTable(nullptr), m_eventList(&this->m_eventSlabAllocator), m_currentReplayEventIterator(),
        m_modeStack(), m_currentMode(TTDMode::Invalid),
        m_snapExtractor(), m_elapsedExecutionTimeSinceSnapshot(0.0),
        m_lastInflateSnapshotTime(-1), m_lastInflateMap(nullptr), m_propertyRecordList(&this->m_miscSlabAllocator),
        m_sourceInfoCount(0), m_loadedTopLevelScripts(&this->m_miscSlabAllocator), m_newFunctionTopLevelScripts(&this->m_miscSlabAllocator), m_evalTopLevelScripts(&this->m_miscSlabAllocator)
    {
        this->InitializeEventListVTable();
        this->m_eventList.SetVTable(this->m_eventListVTable);

        this->m_modeStack.Push(TTDMode::Invalid);

        Recycler * recycler = threadContext->GetRecycler();
        this->m_propertyRecordPinSet.Root(RecyclerNew(recycler, PropertyRecordPinSet, recycler), recycler);
    }

    EventLog::~EventLog()
    {
        this->m_eventList.UnloadEventList();

        if(this->m_lastInflateMap != nullptr)
        {
            TT_HEAP_DELETE(InflateMap, this->m_lastInflateMap);
            this->m_lastInflateMap = nullptr;
        }

        if(this->m_propertyRecordPinSet != nullptr)
        {
            this->m_propertyRecordPinSet.Unroot(this->m_propertyRecordPinSet->GetAllocator());
        }
    }

    void EventLog::UnloadAllLogData()
    {
        this->m_eventList.UnloadEventList();
    }

    void EventLog::InitForTTDRecord()
    {
        //pin all the current properties so they don't move/disappear on us
        for(Js::PropertyId pid = TotalNumberOfBuiltInProperties; pid < this->m_threadContext->GetMaxPropertyId(); ++pid)
        {
            const Js::PropertyRecord* pRecord = this->m_threadContext->GetPropertyName(pid);
            this->AddPropertyRecord(pRecord);
        }

        this->SetGlobalMode(TTDMode::RecordMode);
    }

    void EventLog::InitForTTDReplay(TTDataIOInfo& iofp, const char* parseUri, size_t parseUriLength, bool debug)
    {
        if (debug)
        {
            this->SetGlobalMode(TTDMode::DebuggerMode);
        }
        else
        {
            this->SetGlobalMode(TTDMode::ReplayMode);
        }

        this->ParseLogInto(iofp, parseUri, parseUriLength);

        Js::PropertyId maxPid = TotalNumberOfBuiltInProperties + 1;
        JsUtil::BaseDictionary<Js::PropertyId, NSSnapType::SnapPropertyRecord*, HeapAllocator> pidMap(&HeapAllocator::Instance);

        for(auto iter = this->m_propertyRecordList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            maxPid = max(maxPid, iter.Current()->PropertyId);
            pidMap.AddNew(iter.Current()->PropertyId, iter.Current());
        }

        for(Js::PropertyId cpid = TotalNumberOfBuiltInProperties; cpid <= maxPid; ++cpid)
        {
            NSSnapType::SnapPropertyRecord* spRecord = pidMap.Item(cpid);
            const Js::PropertyRecord* newPropertyRecord = NSSnapType::InflatePropertyRecord(spRecord, this->m_threadContext);

            if(!this->m_propertyRecordPinSet->ContainsKey(const_cast<Js::PropertyRecord*>(newPropertyRecord)))
            {
                this->m_propertyRecordPinSet->AddNew(const_cast<Js::PropertyRecord*>(newPropertyRecord));
            }
        }
    }

    void EventLog::SetGlobalMode(TTDMode m)
    {
        TTDAssert(m == TTDMode::RecordMode || m == TTDMode::ReplayMode || m == TTDMode::DebuggerMode, "These are the only valid global modes");

        this->m_modeStack.SetAt(0, m);
        this->UpdateComputedMode();
    }

    void EventLog::SetSnapshotOrInflateInProgress(bool flag)
    {
        const JsUtil::List<Js::ScriptContext*, HeapAllocator>& contexts = this->m_threadContext->TTDContext->GetTTDContexts();
        for(int32 i = 0; i < contexts.Count(); ++i)
        {
            TTDAssert(contexts.Item(i)->TTDSnapshotOrInflateInProgress != flag, "This is not re-entrant!!!");

            contexts.Item(i)->TTDSnapshotOrInflateInProgress = flag;
        }
    }

    void EventLog::PushMode(TTDMode m)
    {
        TTDAssert(m == TTDMode::CurrentlyEnabled || m == TTDMode::ExcludedExecutionTTAction || m == TTDMode::ExcludedExecutionDebuggerAction ||
            m == TTDMode::DebuggerSuppressGetter || m == TTDMode::DebuggerSuppressBreakpoints || m == TTDMode::DebuggerLogBreakpoints, "These are the only valid mode modifiers to push");

        this->m_modeStack.Push(m);
        this->UpdateComputedMode();
    }

    void EventLog::PopMode(TTDMode m)
    {
        TTDAssert(m == TTDMode::CurrentlyEnabled || m == TTDMode::ExcludedExecutionTTAction || m == TTDMode::ExcludedExecutionDebuggerAction ||
            m == TTDMode::DebuggerSuppressGetter || m == TTDMode::DebuggerSuppressBreakpoints || m == TTDMode::DebuggerLogBreakpoints, "These are the only valid mode modifiers to pop");
        TTDAssert(this->m_modeStack.Peek() == m, "Push/Pop is not matched so something went wrong.");

        this->m_modeStack.Pop();
        this->UpdateComputedMode();
    }

    TTDMode EventLog::GetCurrentTTDMode() const
    {
        return this->m_currentMode;
    }

    void EventLog::SetModeFlagsOnContext(Js::ScriptContext* ctx)
    {
        TTDMode cm = this->m_currentMode;

        ctx->TTDRecordModeEnabled = (cm & (TTDMode::RecordMode | TTDMode::AnyExcludedMode)) == TTDMode::RecordMode;
        ctx->TTDReplayModeEnabled = (cm & (TTDMode::ReplayMode | TTDMode::AnyExcludedMode)) == TTDMode::ReplayMode;
        ctx->TTDRecordOrReplayModeEnabled = (ctx->TTDRecordModeEnabled | ctx->TTDReplayModeEnabled);

        ctx->TTDShouldPerformRecordAction = (cm & (TTDMode::RecordMode | TTDMode::CurrentlyEnabled | TTDMode::AnyExcludedMode)) == (TTDMode::RecordMode | TTDMode::CurrentlyEnabled);
        ctx->TTDShouldPerformReplayAction = (cm & (TTDMode::ReplayMode | TTDMode::CurrentlyEnabled | TTDMode::AnyExcludedMode)) == (TTDMode::ReplayMode | TTDMode::CurrentlyEnabled);
        ctx->TTDShouldPerformRecordOrReplayAction = (ctx->TTDShouldPerformRecordAction | ctx->TTDShouldPerformReplayAction);

        ctx->TTDShouldPerformDebuggerAction = (cm & (TTDMode::DebuggerMode | TTDMode::CurrentlyEnabled | TTDMode::AnyExcludedMode)) == (TTDMode::DebuggerMode | TTDMode::CurrentlyEnabled);
        ctx->TTDShouldSuppressGetterInvocationForDebuggerEvaluation = (cm & TTDMode::DebuggerSuppressGetter) == TTDMode::DebuggerSuppressGetter;
    }

    void EventLog::GetModesForExplicitContextCreate(bool& inRecord, bool& activelyRecording, bool& inReplay)
    {
        inRecord = (this->m_currentMode & (TTDMode::RecordMode | TTDMode::AnyExcludedMode)) == TTDMode::RecordMode;
        activelyRecording = (this->m_currentMode & (TTDMode::RecordMode | TTDMode::CurrentlyEnabled | TTDMode::AnyExcludedMode)) == (TTDMode::RecordMode | TTDMode::CurrentlyEnabled);
        inReplay = (this->m_currentMode & (TTDMode::ReplayMode | TTDMode::AnyExcludedMode)) == TTDMode::ReplayMode;
    }

    bool EventLog::IsDebugModeFlagSet() const
    {
        return (this->m_currentMode & TTDMode::DebuggerMode) == TTDMode::DebuggerMode;
    }

    bool EventLog::ShouldDoGetterInvocationSupression() const
    {
        return (this->m_currentMode & TTD::TTDMode::DebuggerMode) == TTD::TTDMode::DebuggerMode;
    }

    void EventLog::AddPropertyRecord(const Js::PropertyRecord* record)
    {
        this->m_propertyRecordPinSet->AddNew(const_cast<Js::PropertyRecord*>(record));
    }

    const NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* EventLog::AddScriptLoad(Js::FunctionBody* fb, Js::ModuleID moduleId, uint64 sourceContextId, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag)
    {
        NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo = this->m_loadedTopLevelScripts.NextOpenEntry();
        uint32 fCount = (this->m_loadedTopLevelScripts.Count() + this->m_newFunctionTopLevelScripts.Count() + this->m_evalTopLevelScripts.Count());
        bool isUtf8 = ((loadFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source);

        NSSnapValues::ExtractTopLevelLoadedFunctionBodyInfo(fbInfo, fb, fCount, moduleId, sourceContextId, isUtf8, source, sourceLen, loadFlag, this->m_miscSlabAllocator);
        this->m_sourceInfoCount = max(this->m_sourceInfoCount, fb->GetUtf8SourceInfo()->GetSourceInfoId() + 1);

        return fbInfo;
    }

    const NSSnapValues::TopLevelNewFunctionBodyResolveInfo* EventLog::AddNewFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen)
    {
        NSSnapValues::TopLevelNewFunctionBodyResolveInfo* fbInfo = this->m_newFunctionTopLevelScripts.NextOpenEntry();
        uint32 fCount = (this->m_loadedTopLevelScripts.Count() + this->m_newFunctionTopLevelScripts.Count() + this->m_evalTopLevelScripts.Count());

        NSSnapValues::ExtractTopLevelNewFunctionBodyInfo(fbInfo, fb, fCount, moduleId, source, sourceLen, this->m_miscSlabAllocator);
        this->m_sourceInfoCount = max(this->m_sourceInfoCount, fb->GetUtf8SourceInfo()->GetSourceInfoId() + 1);

        return fbInfo;
    }

    const NSSnapValues::TopLevelEvalFunctionBodyResolveInfo* EventLog::AddEvalFunction(Js::FunctionBody* fb, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode)
    {
        NSSnapValues::TopLevelEvalFunctionBodyResolveInfo* fbInfo = this->m_evalTopLevelScripts.NextOpenEntry();
        uint32 fCount = (this->m_loadedTopLevelScripts.Count() + this->m_newFunctionTopLevelScripts.Count() + this->m_evalTopLevelScripts.Count());

        NSSnapValues::ExtractTopLevelEvalFunctionBodyInfo(fbInfo, fb, fCount, moduleId, source, sourceLen, grfscr, registerDocument, isIndirect, strictMode, this->m_miscSlabAllocator);
        this->m_sourceInfoCount = max(this->m_sourceInfoCount, fb->GetUtf8SourceInfo()->GetSourceInfoId() + 1);

        return fbInfo;
    }

    uint32 EventLog::GetSourceInfoCount() const
    {
        return this->m_sourceInfoCount;
    }

    void EventLog::RecordTopLevelCodeAction(uint32 bodyCtrId)
    {
        NSLogEvents::CodeLoadEventLogEntry* clEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::CodeLoadEventLogEntry, NSLogEvents::EventKind::TopLevelCodeTag>();
        clEvent->BodyCounterId = bodyCtrId;
    }

    uint32 EventLog::ReplayTopLevelCodeAction()
    {
        const NSLogEvents::CodeLoadEventLogEntry* clEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::CodeLoadEventLogEntry, NSLogEvents::EventKind::TopLevelCodeTag>();

        return clEvent->BodyCounterId;
    }

    void EventLog::RecordTelemetryLogEvent(Js::JavascriptString* infoStringJs, bool doPrint)
    {
        NSLogEvents::TelemetryEventLogEntry* tEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::TelemetryEventLogEntry, NSLogEvents::EventKind::TelemetryLogTag>();
        this->m_eventSlabAllocator.CopyStringIntoWLength(infoStringJs->GetSz(), infoStringJs->GetLength(), tEvent->InfoString);
        tEvent->DoPrint = doPrint;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->ForceFlush();
#endif
    }

    void EventLog::ReplayTelemetryLogEvent(Js::JavascriptString* infoStringJs)
    {
#if !ENABLE_TTD_INTERNAL_DIAGNOSTICS
        this->AdvanceTimeAndPositionForReplay(); //just eat the telemetry event
#else
        const NSLogEvents::TelemetryEventLogEntry* tEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::TelemetryEventLogEntry, NSLogEvents::EventKind::TelemetryLogTag>();

        uint32 infoStrLength = (uint32)infoStringJs->GetLength();
        const char16* infoStr = infoStringJs->GetSz();

        if(tEvent->InfoString.Length != infoStrLength)
        {
            wprintf(_u("New Telemetry Msg: %ls\n"), infoStr);
            wprintf(_u("Original Telemetry Msg: %ls\n"), tEvent->InfoString.Contents);
            TTDAssert(false, "Telemetry messages differ??");
        }
        else
        {
            for(uint32 i = 0; i < infoStrLength; ++i)
            {
                if(tEvent->InfoString.Contents[i] != infoStr[i])
                {
                    wprintf(_u("New Telemetry Msg: %ls\n"), infoStr);
                    wprintf(_u("Original Telemetry Msg: %ls\n"), tEvent->InfoString.Contents);
                    TTDAssert(false, "Telemetry messages differ??");

                    break;
                }
            }
        }
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->ForceFlush();
#endif
    }

    void EventLog::RecordEmitLogEvent(Js::JavascriptString* uriString)
    {
        this->RecordGetInitializedEvent_DataOnly<NSLogEvents::ExplicitLogWriteEventLogEntry, NSLogEvents::EventKind::ExplicitLogWriteTag>();

        AutoArrayPtr<char> uri(HeapNewArrayZ(char, uriString->GetLength() * 3), uriString->GetLength() * 3);
        size_t uriLength = utf8::EncodeInto((LPUTF8)((char*)uri), uriString->GetSz(), uriString->GetLength());

        this->EmitLog(uri, uriLength);
    }

    void EventLog::ReplayEmitLogEvent()
    {
        this->ReplayGetReplayEvent_Helper<NSLogEvents::ExplicitLogWriteEventLogEntry, NSLogEvents::EventKind::ExplicitLogWriteTag>();

        //check if at end of log -- if so we are done and don't want to execute any more
        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }
    }

    void EventLog::RecordDateTimeEvent(double time)
    {
        NSLogEvents::DoubleEventLogEntry* dEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::DoubleEventLogEntry, NSLogEvents::EventKind::DoubleTag>();
        dEvent->DoubleValue = time;
    }

    void EventLog::RecordDateStringEvent(Js::JavascriptString* stringValue)
    {
        NSLogEvents::StringValueEventLogEntry* sEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::StringValueEventLogEntry, NSLogEvents::EventKind::StringTag>();
        this->m_eventSlabAllocator.CopyStringIntoWLength(stringValue->GetSz(), stringValue->GetLength(), sEvent->StringValue);
    }

    void EventLog::ReplayDateTimeEvent(double* result)
    {
        const NSLogEvents::DoubleEventLogEntry* dEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::DoubleEventLogEntry, NSLogEvents::EventKind::DoubleTag>();
        *result = dEvent->DoubleValue;
    }

    void EventLog::ReplayDateStringEvent(Js::ScriptContext* ctx, Js::JavascriptString** result)
    {
        const NSLogEvents::StringValueEventLogEntry* sEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::StringValueEventLogEntry, NSLogEvents::EventKind::StringTag>();

        const TTString& str = sEvent->StringValue;
        *result = Js::JavascriptString::NewCopyBuffer(str.Contents, str.Length, ctx);
    }

    void EventLog::RecordExternalEntropyRandomEvent(uint64 seed0, uint64 seed1)
    {
        NSLogEvents::RandomSeedEventLogEntry* rsEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::RandomSeedEventLogEntry, NSLogEvents::EventKind::RandomSeedTag>();
        rsEvent->Seed0 = seed0;
        rsEvent->Seed1 = seed1;
    }

    void EventLog::ReplayExternalEntropyRandomEvent(uint64* seed0, uint64* seed1)
    {
        const NSLogEvents::RandomSeedEventLogEntry* rsEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::RandomSeedEventLogEntry, NSLogEvents::EventKind::RandomSeedTag>();
        *seed0 = rsEvent->Seed0;
        *seed1 = rsEvent->Seed1;
    }

    void EventLog::RecordPropertyEnumEvent(BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, Js::JavascriptString* propertyName)
    {
        //When we replay we can just skip this pid cause it should never matter -- but if return code is false then we need to record the "at end" info
        if(returnCode && Js::IsInternalPropertyId(pid))
        {
            return;
        }

        NSLogEvents::PropertyEnumStepEventLogEntry* peEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::PropertyEnumStepEventLogEntry, NSLogEvents::EventKind::PropertyEnumTag>();
        peEvent->ReturnCode = returnCode;
        peEvent->Pid = pid;
        peEvent->Attributes = attributes;

        InitializeAsNullPtrTTString(peEvent->PropertyString);
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        if(returnCode)
        {
            this->m_eventSlabAllocator.CopyStringIntoWLength(propertyName->GetSz(), propertyName->GetLength(), peEvent->PropertyString);
        }
#else
        if(returnCode && pid == Js::Constants::NoProperty)
        {
            this->m_eventSlabAllocator.CopyStringIntoWLength(propertyName->GetSz(), propertyName->GetLength(), peEvent->PropertyString);
        }
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteEnumAction(this->m_eventTimeCtr - 1, returnCode, pid, attributes, propertyName);
#endif
    }

    void EventLog::ReplayPropertyEnumEvent(Js::ScriptContext* requestContext, BOOL* returnCode, Js::BigPropertyIndex* newIndex, const Js::DynamicObject* obj, Js::PropertyId* pid, Js::PropertyAttributes* attributes, Js::JavascriptString** propertyName)
    {
        const NSLogEvents::PropertyEnumStepEventLogEntry* peEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::PropertyEnumStepEventLogEntry, NSLogEvents::EventKind::PropertyEnumTag>();

        *returnCode = peEvent->ReturnCode;
        *pid = peEvent->Pid;
        *attributes = peEvent->Attributes;

        if(*returnCode)
        {
            TTDAssert(*pid != Js::Constants::NoProperty, "This is so weird we need to figure out what this means.");
            TTDAssert(!Js::IsInternalPropertyId(*pid), "We should skip recording this.");

            Js::PropertyString* propertyString = requestContext->GetPropertyString(*pid);
            *propertyName = propertyString;

            const Js::PropertyRecord* pRecord = requestContext->GetPropertyName(*pid);
            *newIndex = obj->GetDynamicType()->GetTypeHandler()->GetPropertyIndex_EnumerateTTD(pRecord);

            TTDAssert(*newIndex != Js::Constants::NoBigSlot, "If *returnCode is true then we found it during record -- but missing in replay.");
        }
        else
        {
            *propertyName = nullptr;

            *newIndex = obj->GetDynamicType()->GetTypeHandler()->GetPropertyCount();
        }

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteEnumAction(this->m_eventTimeCtr - 1, *returnCode, *pid, *attributes, *propertyName);
#endif
    }

    void EventLog::RecordSymbolCreationEvent(Js::PropertyId pid)
    {
        NSLogEvents::SymbolCreationEventLogEntry* scEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::SymbolCreationEventLogEntry, NSLogEvents::EventKind::SymbolCreationTag>();
        scEvent->Pid = pid;
    }

    void EventLog::ReplaySymbolCreationEvent(Js::PropertyId* pid)
    {
        const NSLogEvents::SymbolCreationEventLogEntry* scEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::SymbolCreationEventLogEntry, NSLogEvents::EventKind::SymbolCreationTag>();
        *pid = scEvent->Pid;
    }

    void EventLog::RecordWeakCollectionContainsEvent(bool contains)
    {
        NSLogEvents::WeakCollectionContainsEventLogEntry* wcEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::WeakCollectionContainsEventLogEntry, NSLogEvents::EventKind::WeakCollectionContainsTag>();
        wcEvent->ContainsValue = contains;
    }

    bool EventLog::ReplayWeakCollectionContainsEvent()
    {
        const NSLogEvents::WeakCollectionContainsEventLogEntry* wcEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::WeakCollectionContainsEventLogEntry, NSLogEvents::EventKind::WeakCollectionContainsTag>();
        return wcEvent->ContainsValue;
    }

    NSLogEvents::EventLogEntry* EventLog::RecordExternalCallEvent(Js::JavascriptFunction* func, int32 rootDepth, uint32 argc, Js::Var* argv, bool checkExceptions)
    {
        NSLogEvents::ExternalCallEventLogEntry* ecEvent = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::ExternalCallEventLogEntry, NSLogEvents::EventKind::ExternalCallTag>(&ecEvent);

        //We never fail with an exception (instead we set the HasRecordedException in script context)
        evt->ResultStatus = 0;

        NSLogEvents::ExternalCallEventLogEntry_ProcessArgs(evt, rootDepth, func, argc, argv, checkExceptions, this->m_eventSlabAllocator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        NSLogEvents::ExternalCallEventLogEntry_ProcessDiagInfoPre(evt, func, this->m_eventSlabAllocator);
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteCall(func, true, argc, argv, this->GetLastEventTime());
#endif

        return evt;
    }

    void EventLog::RecordExternalCallEvent_Complete(Js::JavascriptFunction* efunction, NSLogEvents::EventLogEntry* evt, Js::Var result)
    {
        NSLogEvents::ExternalCallEventLogEntry_ProcessReturn(evt, result, this->GetLastEventTime());

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteReturn(efunction, result, this->GetLastEventTime());
#endif
    }

    void EventLog::ReplayExternalCallEvent(Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, Js::Var* result)
    {
        TTDAssert(result != nullptr, "Must be non-null!!!");
        TTDAssert(*result == nullptr, "And initialized to a default value.");

        const NSLogEvents::ExternalCallEventLogEntry* ecEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::ExternalCallEventLogEntry, NSLogEvents::EventKind::ExternalCallTag>();

        Js::ScriptContext* ctx = function->GetScriptContext();
        TTDAssert(ctx != nullptr, "Not sure how this would be possible but check just in case.");

        ThreadContextTTD* executeContext = ctx->GetThreadContext()->TTDContext;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteCall(function, true, argc, argv, this->GetLastEventTime());
#endif

        //make sure we log all of the passed arguments in the replay host
        TTDAssert(argc + 1 == ecEvent->ArgCount, "Mismatch in args!!!");

        TTDVar recordedFunction = ecEvent->ArgArray[0];
        NSLogEvents::PassVarToHostInReplay(executeContext, recordedFunction, function);

        for(uint32 i = 0; i < argc; ++i)
        {
            Js::Var replayVar = argv[i];
            TTDVar recordedVar = ecEvent->ArgArray[i + 1];
            NSLogEvents::PassVarToHostInReplay(executeContext, recordedVar, replayVar);
        }

        //replay anything that happens in the external call
        BEGIN_LEAVE_SCRIPT(ctx)
        {
            this->ReplayActionEventSequenceThroughTime(ecEvent->LastNestedEventTime);
        }
        END_LEAVE_SCRIPT(ctx);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTDAssert(!this->m_currentReplayEventIterator.IsValid() || this->m_currentReplayEventIterator.Current()->EventTimeStamp == this->m_eventTimeCtr, "Out of Sync!!!");
#endif

        *result = NSLogEvents::InflateVarInReplay(executeContext, ecEvent->ReturnValue);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteReturn(function, *result, this->GetLastEventTime());
#endif

        //if we had exception info then we need to patch it up and do what the external call did
        if(ecEvent->CheckExceptionStatus)
        {
            if(ctx->HasRecordedException())
            {
                bool considerPassingToDebugger = false;
                Js::JavascriptExceptionObject* recordedException = ctx->GetAndClearRecordedException(&considerPassingToDebugger);
                if(recordedException != nullptr)
                {
                    // If this is script termination, then throw ScriptAbortExceptio, else throw normal Exception object.
                    if(recordedException == ctx->GetThreadContext()->GetPendingTerminatedErrorObject())
                    {
                        throw Js::ScriptAbortException();
                    }
                    else
                    {
                        Js::JavascriptExceptionOperators::RethrowExceptionObject(recordedException, ctx, considerPassingToDebugger);
                    }
                }
            }
        }

        if(*result == nullptr)
        {
            *result = ctx->GetLibrary()->GetUndefined();
        }
        else
        {
            *result = Js::CrossSite::MarshalVar(ctx, *result);
        }
    }

    NSLogEvents::EventLogEntry* EventLog::RecordEnqueueTaskEvent(Js::Var taskVar)
    {
        NSLogEvents::ExternalCbRegisterCallEventLogEntry* ecEvent = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::ExternalCbRegisterCallEventLogEntry, NSLogEvents::EventKind::ExternalCbRegisterCall>(&ecEvent);

        ecEvent->CallbackFunction = static_cast<TTDVar>(taskVar);
        ecEvent->LastNestedEventTime = TTD_EVENT_MAXTIME;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteLiteralMsg("Enqueue Task: ");
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteVar(taskVar);
#endif

        return evt;
    }

    void EventLog::RecordEnqueueTaskEvent_Complete(NSLogEvents::EventLogEntry* evt)
    {
        NSLogEvents::ExternalCbRegisterCallEventLogEntry* ecEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::ExternalCbRegisterCallEventLogEntry, NSLogEvents::EventKind::ExternalCbRegisterCall>(evt);

        ecEvent->LastNestedEventTime = this->GetLastEventTime();
    }

    void EventLog::ReplayEnqueueTaskEvent(Js::ScriptContext* ctx, Js::Var taskVar)
    {
        const NSLogEvents::ExternalCbRegisterCallEventLogEntry* ecEvent = this->ReplayGetReplayEvent_Helper<NSLogEvents::ExternalCbRegisterCallEventLogEntry, NSLogEvents::EventKind::ExternalCbRegisterCall>();
        ThreadContextTTD* executeContext = ctx->GetThreadContext()->TTDContext;

        NSLogEvents::PassVarToHostInReplay(executeContext, ecEvent->CallbackFunction, taskVar);

        //replay anything that happens when we are out of the call
        BEGIN_LEAVE_SCRIPT(ctx)
        {
            this->ReplayActionEventSequenceThroughTime(ecEvent->LastNestedEventTime);
        }
        END_LEAVE_SCRIPT(ctx);
    }

    int64 EventLog::GetCurrentTopLevelEventTime() const
    {
        return this->m_topLevelCallbackEventTime;
    }

    int64 EventLog::GetFirstEventTimeInLog() const
    {
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            if(NSLogEvents::IsJsRTActionRootCall(iter.Current()))
            {
                return NSLogEvents::GetTimeFromRootCallOrSnapshot(iter.Current());
            }
        }

        return -1;
    }

    int64 EventLog::GetLastEventTimeInLog() const
    {
        for(auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly(); iter.IsValid(); iter.MovePrevious_ReplayOnly())
        {
            if(NSLogEvents::IsJsRTActionRootCall(iter.Current()))
            {
                return NSLogEvents::GetTimeFromRootCallOrSnapshot(iter.Current());
            }
        }

        return -1;
    }

    int64 EventLog::GetKthEventTimeInLog(uint32 k) const
    {
        uint32 topLevelCount = 0;
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            if(NSLogEvents::IsJsRTActionRootCall(iter.Current()))
            {
                topLevelCount++;

                if(topLevelCount == k)
                {
                    return NSLogEvents::GetTimeFromRootCallOrSnapshot(iter.Current());
                }
            }
        }

        return -1;
    }

    void EventLog::ResetCallStackForTopLevelCall(int64 topLevelCallbackEventTime)
    {
        this->m_topLevelCallbackEventTime = topLevelCallbackEventTime;
    }

    bool EventLog::IsTimeForSnapshot() const
    {
        return (this->m_elapsedExecutionTimeSinceSnapshot > this->m_threadContext->TTDContext->SnapInterval);
    }

    void EventLog::PruneLogLength()
    {
        uint32 maxSnaps = this->m_threadContext->TTDContext->SnapHistoryLength;
        uint32 snapCount = 0;
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.Current()->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                snapCount++;
            }
        }

        //If we have more than the desired number of snaps we will trim them off
        if(snapCount > maxSnaps)
        {
            uint32 snapDelCount = snapCount - maxSnaps;
            auto delIter = this->m_eventList.GetIteratorAtFirst();

            while(true)
            {
                NSLogEvents::EventLogEntry* evt = delIter.Current();
                if(delIter.Current()->EventKind == NSLogEvents::EventKind::SnapshotTag)
                {
                    if(snapDelCount == 0)
                    {
                        break;
                    }

                    snapDelCount--;
                }

                TTEventList::TTEventListLink* block = delIter.GetBlock();
                delIter.MoveNext();

                this->m_eventList.DeleteFirstEntry(block, evt);
            }
        }
    }

    void EventLog::IncrementElapsedSnapshotTime(double addtlTime)
    {
        this->m_elapsedExecutionTimeSinceSnapshot += addtlTime;
    }

    void EventLog::DoSnapshotExtract()
    {
        //force a GC to get weak containers in a consistent state
        TTDTimer timer;
        double startTime = timer.Now();
        this->m_threadContext->GetRecycler()->CollectNow<CollectNowForceInThread>();
        this->m_threadContext->TTDContext->SyncRootsBeforeSnapshot_Record();
        double endTime = timer.Now();

        //do the rest of the snapshot
        this->SetSnapshotOrInflateInProgress(true);
        this->PushMode(TTDMode::ExcludedExecutionTTAction);

        ///////////////////////////
        //Create the event object and add it to the log
        NSLogEvents::SnapshotEventLogEntry* snapEvent = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>();
        snapEvent->RestoreTimestamp = this->GetLastEventTime();

        JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator> liveTopLevelBodies(&HeapAllocator::Instance);
        snapEvent->Snap = this->DoSnapshotExtract_Helper((endTime - startTime) / 1000.0, liveTopLevelBodies);

        for(int32 i = 0; i < this->m_threadContext->TTDContext->GetTTDContexts().Count(); ++i)
        {
            this->m_threadContext->TTDContext->GetTTDContexts().Item(i)->TTDContextInfo->CleanUnreachableTopLevelBodies(liveTopLevelBodies);
        }

        //get info about live weak roots etc. we want to use in the replay from the snapshot into the event as well
        snapEvent->LiveContextCount = snapEvent->Snap->GetContextList().Count();
        snapEvent->LiveContextIdArray = nullptr;
        if(snapEvent->LiveContextCount != 0)
        {
            snapEvent->LiveContextIdArray = this->m_eventSlabAllocator.SlabAllocateArray<TTD_LOG_PTR_ID>(snapEvent->LiveContextCount);
            uint32 clpos = 0;
            for(auto iter = snapEvent->Snap->GetContextList().GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                snapEvent->LiveContextIdArray[clpos] = iter.Current()->ScriptContextLogId;
                clpos++;
            }
        }

        //walk the roots and count all of the "interesting weak ref roots"
        snapEvent->LongLivedRefRootsCount = 0;
        for(auto iter = snapEvent->Snap->GetRootList().GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapRootInfoEntry* spe = iter.Current();
            if(spe->MaybeLongLivedRoot)
            {
                snapEvent->LongLivedRefRootsCount++;
            }
        }

        //Now allocate the arrays for them and do the processing
        snapEvent->LongLivedRefRootsIdArray = nullptr; 
        if(snapEvent->LongLivedRefRootsCount != 0)
        {
            snapEvent->LongLivedRefRootsIdArray = this->m_eventSlabAllocator.SlabAllocateArray<TTD_LOG_PTR_ID>(snapEvent->LongLivedRefRootsCount);
            uint32 rpos = 0;
            for(auto iter = snapEvent->Snap->GetRootList().GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                const NSSnapValues::SnapRootInfoEntry* spe = iter.Current();
                if(spe->MaybeLongLivedRoot)
                {
                    snapEvent->LongLivedRefRootsIdArray[rpos] = spe->LogId;
                    rpos++;
                }
            }
        }

        this->m_elapsedExecutionTimeSinceSnapshot = 0.0;

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteLiteralMsg("---SNAPSHOT EVENT---\n");
#endif

        this->PopMode(TTDMode::ExcludedExecutionTTAction);
        this->SetSnapshotOrInflateInProgress(false);
    }

    void EventLog::DoRtrSnapIfNeeded()
    {
        TTDAssert(this->m_currentReplayEventIterator.IsValid() && NSLogEvents::IsJsRTActionRootCall(this->m_currentReplayEventIterator.Current()), "Something in wrong with the event position.");

        this->SetSnapshotOrInflateInProgress(true);
        this->PushMode(TTDMode::ExcludedExecutionTTAction);

        NSLogEvents::JsRTCallFunctionAction* rootCall = NSLogEvents::GetInlineEventDataAs<NSLogEvents::JsRTCallFunctionAction, NSLogEvents::EventKind::CallExistingFunctionActionTag>(this->m_currentReplayEventIterator.Current());
        if(rootCall->AdditionalReplayInfo->RtRSnap == nullptr)
        {
            //Be careful to ensure that caller is actually doing this
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_JavascriptException));

            JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator> liveTopLevelBodies(&HeapAllocator::Instance); //don't actually care about the result here
            rootCall->AdditionalReplayInfo->RtRSnap = this->DoSnapshotExtract_Helper(0.0, liveTopLevelBodies);
        }

        this->PopMode(TTDMode::ExcludedExecutionTTAction);
        this->SetSnapshotOrInflateInProgress(false);
    }

    int64 EventLog::FindSnapTimeForEventTime(int64 targetTime, int64* optEndSnapTime)
    {
        int64 snapTime = -1;
        if(optEndSnapTime != nullptr)
        {
            *optEndSnapTime = -1;
        }

        for(auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly(); iter.IsValid(); iter.MovePrevious_ReplayOnly())
        {
            bool isSnap = false;
            bool isRoot = false;
            bool hasRtrSnap = false;
            int64 time = NSLogEvents::AccessTimeInRootCallOrSnapshot(iter.Current(), isSnap, isRoot, hasRtrSnap);

            bool validSnap =  isSnap | (isRoot & hasRtrSnap);
            if(validSnap && time <= targetTime)
            {
                snapTime = time;
                break;
            }
        }

        if(optEndSnapTime != nullptr)
        {
            for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
            {
                if(iter.Current()->EventKind == NSLogEvents::EventKind::SnapshotTag)
                {
                    NSLogEvents::SnapshotEventLogEntry* snapEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(iter.Current());
                    if(snapEvent->RestoreTimestamp > snapTime)
                    {
                        *optEndSnapTime = snapEvent->RestoreTimestamp;
                        break;
                    }
                }
            }
        }

        return snapTime;
    }

    void EventLog::GetSnapShotBoundInterval(int64 targetTime, int64* snapIntervalStart, int64* snapIntervalEnd) const
    {
        *snapIntervalStart = -1;
        *snapIntervalEnd = -1;

        //move the iterator to the current snapshot just before the event
        auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly();
        while(iter.IsValid())
        {
            NSLogEvents::EventLogEntry* evt = iter.Current();
            if(evt->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                NSLogEvents::SnapshotEventLogEntry* snapEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(iter.Current());
                if(snapEvent->RestoreTimestamp <= targetTime)
                {
                    *snapIntervalStart = snapEvent->RestoreTimestamp;
                    break;
                }
            }

            iter.MovePrevious_ReplayOnly();
        }

        //now move the iter to the next snapshot
        while(iter.IsValid())
        {
            NSLogEvents::EventLogEntry* evt = iter.Current();
            if(evt->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                NSLogEvents::SnapshotEventLogEntry* snapEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(iter.Current());
                if(*snapIntervalStart < snapEvent->RestoreTimestamp)
                {
                    *snapIntervalEnd = snapEvent->RestoreTimestamp;
                    break;
                }
            }

            iter.MoveNext();
        }
    }

    int64 EventLog::GetPreviousSnapshotInterval(int64 currentSnapTime) const
    {
        //move the iterator to the current snapshot just before the event
        for(auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly(); iter.IsValid(); iter.MovePrevious_ReplayOnly())
        {
            NSLogEvents::EventLogEntry* evt = iter.Current();
            if(evt->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                NSLogEvents::SnapshotEventLogEntry* snapEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(iter.Current());
                if(snapEvent->RestoreTimestamp < currentSnapTime)
                {
                    return snapEvent->RestoreTimestamp;
                }
            }
        }

        return -1;
    }

    void EventLog::DoSnapshotInflate(int64 etime)
    {
        this->PushMode(TTDMode::ExcludedExecutionTTAction);

        const SnapShot* snap = nullptr;
        int64 restoreEventTime = -1;

        for(auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly(); iter.IsValid(); iter.MovePrevious_ReplayOnly())
        {
            NSLogEvents::EventLogEntry* evt = iter.Current();
            if(evt->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                NSLogEvents::SnapshotEventLogEntry* snapEvent = NSLogEvents::GetInlineEventDataAs<NSLogEvents::SnapshotEventLogEntry, NSLogEvents::EventKind::SnapshotTag>(evt);
                if(snapEvent->RestoreTimestamp == etime)
                {
                    NSLogEvents::SnapshotEventLogEntry_EnsureSnapshotDeserialized(evt, this->m_threadContext);

                    restoreEventTime = snapEvent->RestoreTimestamp;
                    snap = snapEvent->Snap;
                    break;
                }
            }

            if(NSLogEvents::IsJsRTActionRootCall(evt))
            {
                const NSLogEvents::JsRTCallFunctionAction* rootEntry = NSLogEvents::GetInlineEventDataAs<NSLogEvents::JsRTCallFunctionAction, NSLogEvents::EventKind::CallExistingFunctionActionTag>(evt);

                if(rootEntry->CallEventTime == etime)
                {
                    restoreEventTime = rootEntry->CallEventTime;
                    snap = rootEntry->AdditionalReplayInfo->RtRSnap;
                    break;
                }
            }
        }
        TTDAssert(snap != nullptr, "Log should start with a snapshot!!!");

        uint32 dbgScopeCount = snap->GetDbgScopeCountNonTopLevel();

        TTDIdentifierDictionary<uint64, NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo*> topLevelLoadScriptMap;
        topLevelLoadScriptMap.Initialize(this->m_loadedTopLevelScripts.Count());
        for(auto iter = this->m_loadedTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            topLevelLoadScriptMap.AddItem(iter.Current()->TopLevelBase.TopLevelBodyCtr, iter.Current());
            dbgScopeCount += iter.Current()->TopLevelBase.ScopeChainInfo.ScopeCount;
        }

        TTDIdentifierDictionary<uint64, NSSnapValues::TopLevelNewFunctionBodyResolveInfo*> topLevelNewScriptMap;
        topLevelNewScriptMap.Initialize(this->m_newFunctionTopLevelScripts.Count());
        for(auto iter = this->m_newFunctionTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            topLevelNewScriptMap.AddItem(iter.Current()->TopLevelBase.TopLevelBodyCtr, iter.Current());
            dbgScopeCount += iter.Current()->TopLevelBase.ScopeChainInfo.ScopeCount;
        }

        TTDIdentifierDictionary<uint64, NSSnapValues::TopLevelEvalFunctionBodyResolveInfo*> topLevelEvalScriptMap;
        topLevelEvalScriptMap.Initialize(this->m_evalTopLevelScripts.Count());
        for(auto iter = this->m_evalTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            topLevelEvalScriptMap.AddItem(iter.Current()->TopLevelBase.TopLevelBodyCtr, iter.Current());
            dbgScopeCount += iter.Current()->TopLevelBase.ScopeChainInfo.ScopeCount;
        }

        uint32 topFunctionCount = topLevelLoadScriptMap.Count() + topLevelNewScriptMap.Count() + topLevelEvalScriptMap.Count();

        ThreadContextTTD* threadCtx = this->m_threadContext->TTDContext;
        const UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& snpCtxs = snap->GetContextList();

        //check if we can reuse script contexts or we need to create new ones
        bool reuseInflateMap = (this->m_lastInflateMap != nullptr && this->m_lastInflateSnapshotTime == etime && !threadCtx->ContextCreatedOrDestoyedInReplay());

        //Fast checks ok but make sure we aren't blocked by a non-restorable well known object
        if(reuseInflateMap)
        {
            reuseInflateMap = snap->AllWellKnownObjectsReusable(this->m_lastInflateMap);
        }

        if(reuseInflateMap)
        {
            this->m_lastInflateMap->PrepForReInflate(snap->ContextCount(), snap->HandlerCount(), snap->TypeCount(), snap->PrimitiveCount() + snap->ObjectCount(), snap->BodyCount() + topFunctionCount, dbgScopeCount, snap->EnvCount(), snap->SlotArrayCount());

            //collect anything that is dead
            threadCtx->ClearRootsForSnapRestore();
            this->m_threadContext->GetRecycler()->CollectNow<CollectNowForceInThread>();

            //inflate into existing contexts
            const JsUtil::List<Js::ScriptContext*, HeapAllocator>& oldCtxts = threadCtx->GetTTDContexts();
            for(auto iter = snpCtxs.GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                const NSSnapValues::SnapContext* sCtx = iter.Current();
                Js::ScriptContext* vCtx = nullptr;
                for(int32 i = 0; i < oldCtxts.Count(); ++i)
                {
                    if(oldCtxts.Item(i)->ScriptContextLogTag == sCtx->ScriptContextLogId)
                    {
                        vCtx = oldCtxts.Item(i);
                        break;
                    }
                }
                TTDAssert(vCtx != nullptr, "We lost a context somehow!!!");

                NSSnapValues::InflateScriptContext(sCtx, vCtx, this->m_lastInflateMap, topLevelLoadScriptMap, topLevelNewScriptMap, topLevelEvalScriptMap);
            }
        }
        else
        {
            bool shouldReleaseCtxs = false;
            if(this->m_lastInflateMap != nullptr)
            {
                shouldReleaseCtxs = true;

                TT_HEAP_DELETE(InflateMap, this->m_lastInflateMap);
                this->m_lastInflateMap = nullptr;
            }

            this->m_lastInflateMap = TT_HEAP_NEW(InflateMap);
            this->m_lastInflateMap->PrepForInitialInflate(this->m_threadContext, snap->ContextCount(), snap->HandlerCount(), snap->TypeCount(), snap->PrimitiveCount() + snap->ObjectCount(), snap->BodyCount() + topFunctionCount, dbgScopeCount, snap->EnvCount(), snap->SlotArrayCount());
            this->m_lastInflateSnapshotTime = etime;

            //collect anything that is dead
            JsUtil::List<FinalizableObject*, HeapAllocator> deadCtxs(&HeapAllocator::Instance);
            threadCtx->ClearContextsForSnapRestore(deadCtxs);
            threadCtx->ClearRootsForSnapRestore();

            //allocate and inflate into new contexts
            for(auto iter = snpCtxs.GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                const NSSnapValues::SnapContext* sCtx = iter.Current();

                Js::ScriptContext* vCtx = nullptr;
                threadCtx->TTDExternalObjectFunctions.pfCreateJsRTContextCallback(threadCtx->GetRuntimeHandle(), &vCtx);

                NSSnapValues::InflateScriptContext(sCtx, vCtx, this->m_lastInflateMap, topLevelLoadScriptMap, topLevelNewScriptMap, topLevelEvalScriptMap);
            }
            threadCtx->ResetContextCreatedOrDestoyedInReplay();

            if(shouldReleaseCtxs)
            {
                for(int32 i = 0; i < deadCtxs.Count(); ++i)
                {
                    threadCtx->TTDExternalObjectFunctions.pfReleaseJsRTContextCallback(deadCtxs.Item(i));
                }
                this->m_threadContext->GetRecycler()->CollectNow<CollectNowForceInThread>();
            }

            //We don't want to have a bunch of snapshots in memory (that will get big fast) so unload all but the current one
            for(auto iter = this->m_eventList.GetIteratorAtLast_ReplayOnly(); iter.IsValid(); iter.MovePrevious_ReplayOnly())
            {
                bool isSnap = false;
                bool isRoot = false;
                bool hasRtrSnap = false;
                int64 time = NSLogEvents::AccessTimeInRootCallOrSnapshot(iter.Current(), isSnap, isRoot, hasRtrSnap);

                bool hasSnap = isSnap | (isRoot & hasRtrSnap);
                if(hasSnap && time != etime)
                {
                    if(isSnap)
                    {
                        NSLogEvents::SnapshotEventLogEntry_UnloadSnapshot(iter.Current());
                    }
                    else
                    {
                        NSLogEvents::JsRTCallFunctionAction_UnloadSnapshot(iter.Current());
                    }
                }
            }
        }

        this->SetSnapshotOrInflateInProgress(true); //make sure we don't do any un-intended CrossSite conversions

        snap->Inflate(this->m_lastInflateMap, this->m_threadContext->TTDContext);
        this->m_lastInflateMap->CleanupAfterInflate();

        this->SetSnapshotOrInflateInProgress(false); //re-enable CrossSite conversions

        this->m_eventTimeCtr = restoreEventTime;
        if(!this->m_eventList.IsEmpty())
        {
            this->m_currentReplayEventIterator = this->m_eventList.GetIteratorAtLast_ReplayOnly();

            while(true)
            {
                bool isSnap = false;
                bool isRoot = false;
                bool hasRtrSnap = false;
                int64 time = NSLogEvents::AccessTimeInRootCallOrSnapshot(this->m_currentReplayEventIterator.Current(), isSnap, isRoot, hasRtrSnap);

                if((isSnap | isRoot) && time == this->m_eventTimeCtr)
                {
                    break;
                }

                this->m_currentReplayEventIterator.MovePrevious_ReplayOnly();
            }

            //we want to advance to the event immediately after the snapshot as well so do that
            if(this->m_currentReplayEventIterator.Current()->EventKind == NSLogEvents::EventKind::SnapshotTag)
            {
                this->m_eventTimeCtr++;
                this->m_currentReplayEventIterator.MoveNext();
            }
        }

        this->PopMode(TTDMode::ExcludedExecutionTTAction);

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->WriteLiteralMsg("---INFLATED SNAPSHOT---\n");
#endif
    }

    void EventLog::ReplayRootEventsToTime(int64 eventTime)
    {
        while(this->m_eventTimeCtr < eventTime)
        {
            this->ReplaySingleRootEntry();
        }
    }

    void EventLog::ReplaySingleRootEntry()
    {
        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        NSLogEvents::EventKind eKind = this->m_currentReplayEventIterator.Current()->EventKind;
        if(eKind == NSLogEvents::EventKind::SnapshotTag)
        {
            this->ReplaySnapshotEvent();
        }
        else if(eKind == NSLogEvents::EventKind::EventLoopYieldPointTag)
        {
            this->ReplayEventLoopYieldPointEvent();
        }
        else
        {
            TTDAssert(eKind > NSLogEvents::EventKind::JsRTActionTag, "Either this is an invalid tag to replay directly (should be driven internally) or it is not known!!!");

            this->ReplaySingleActionEventEntry();
        }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTDAssert(!this->m_currentReplayEventIterator.IsValid() || this->m_currentReplayEventIterator.Current()->EventTimeStamp == this->m_eventTimeCtr, "We are out of sync here");
#endif
    }

    void EventLog::ReplayActionEventSequenceThroughTime(int64 eventTime)
    {
        while(this->m_eventTimeCtr <= eventTime)
        {
            this->ReplaySingleActionEventEntry();
        }
    }

    void EventLog::ReplaySingleActionEventEntry()
    {
        if(!this->m_currentReplayEventIterator.IsValid())
        {
            this->AbortReplayReturnToHost();
        }

        NSLogEvents::EventLogEntry* evt = this->m_currentReplayEventIterator.Current();
        this->AdvanceTimeAndPositionForReplay();

        NSLogEvents::ContextExecuteKind execKind = this->m_eventListVTable[(uint32)evt->EventKind].ContextKind;
        auto executeFP = this->m_eventListVTable[(uint32)evt->EventKind].ExecuteFP;

        TTDAssert(!NSLogEvents::EventFailsWithRuntimeError(evt), "We have a failing Event in the Log -- we assume host is correct!");

        ThreadContextTTD* executeContext = this->m_threadContext->TTDContext;
        if(execKind == NSLogEvents::ContextExecuteKind::GlobalAPIWrapper)
        {
            //enter/exit global wrapper -- see JsrtInternal.h
            try
            {
                AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

                executeFP(evt, executeContext);

                TTDAssert(NSLogEvents::EventCompletesNormally(evt), "All my action events should exit or terminate before return so no need to loop yet but may want to later");
            }
            catch(TTD::TTDebuggerAbortException)
            {
                throw;
            }
            catch(...)
            {
                TTDAssert(false, "Encountered other kind of exception in replay??");
            }
        }
        else if(execKind == NSLogEvents::ContextExecuteKind::ContextAPIWrapper)
        {
            //enter/exit context wrapper -- see JsrtInternal.h
            Js::ScriptContext* ctx = executeContext->GetActiveScriptContext();

            TTDAssert(ctx != nullptr, "This should be set!!!");
            TTDAssert(ctx->GetThreadContext()->GetRecordedException() == nullptr, "Shouldn't have outstanding exceptions (assume always CheckContext when recording).");
            TTDAssert(this->m_threadContext->TTDContext->GetActiveScriptContext() == ctx, "Make sure the replay host didn't change contexts on us unexpectedly without resetting back to the correct one.");

            try
            {
                AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_JavascriptException));

                // Enter script
                BEGIN_ENTER_SCRIPT(ctx, true, true, true)
                {
                    executeFP(evt, executeContext);
                }
                END_ENTER_SCRIPT

                TTDAssert(NSLogEvents::EventCompletesNormally(evt), "All my action events should exit / terminate before return so no need to loop yet but may want to later");
            }
            catch(const Js::JavascriptException& err)
            {
                TTDAssert(NSLogEvents::EventCompletesWithException(evt), "Should see same execption here");

                ctx->GetThreadContext()->SetRecordedException(err.GetAndClear());
            }
            catch(Js::ScriptAbortException)
            {
                TTDAssert(NSLogEvents::EventCompletesWithException(evt), "Should see same execption here");

                Assert(ctx->GetThreadContext()->GetRecordedException() == nullptr);
                ctx->GetThreadContext()->SetRecordedException(ctx->GetThreadContext()->GetPendingTerminatedErrorObject());
            }
            catch(TTD::TTDebuggerAbortException)
            {
                throw;
            }
            catch(...)
            {
                TTDAssert(false, "Encountered other kind of exception in replay??");
            }
        }
        else if(execKind == NSLogEvents::ContextExecuteKind::ContextAPINoScriptWrapper)
        {
            //enter/exit context no script wrapper -- see JsrtInternal.h
            Js::ScriptContext* ctx = executeContext->GetActiveScriptContext();

            TTDAssert(ctx != nullptr, "This should be set!!!");
            TTDAssert(ctx->GetThreadContext()->GetRecordedException() == nullptr, "Shouldn't have outstanding exceptions (assume always CheckContext when recording).");
            TTDAssert(this->m_threadContext->TTDContext->GetActiveScriptContext() == ctx, "Make sure the replay host didn't change contexts on us unexpectedly without resetting back to the correct one.");

            try
            {
                AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

                executeFP(evt, executeContext);

                TTDAssert(NSLogEvents::EventCompletesNormally(evt), "All my action events should both exit / terminate before return so no need to loop yet but may want to later");
            }
            catch(const Js::JavascriptException& err)
            {
                TTDAssert(NSLogEvents::EventCompletesWithException(evt), "Should see same execption here");

                TTDAssert(false, "Should never get JavascriptExceptionObject for ContextAPINoScriptWrapper.");
                ctx->GetThreadContext()->SetRecordedException(err.GetAndClear());
            }
            catch(Js::ScriptAbortException)
            {
                TTDAssert(NSLogEvents::EventCompletesWithException(evt), "Should see same execption here");

                Assert(ctx->GetThreadContext()->GetRecordedException() == nullptr);
                ctx->GetThreadContext()->SetRecordedException(ctx->GetThreadContext()->GetPendingTerminatedErrorObject());
            }
            catch(TTD::TTDebuggerAbortException)
            {
                throw;
            }
            catch(...)
            {
                TTDAssert(false, "Encountered other kind of exception in replay??");
            }
        }
        else
        {
            TTDAssert(executeContext->GetActiveScriptContext() == nullptr || !executeContext->GetActiveScriptContext()->GetThreadContext()->IsScriptActive(), "These should all be outside of script context!!!");

            //No need to move into script context just execute the action
            executeFP(evt, executeContext);
        }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTDAssert(!this->m_currentReplayEventIterator.IsValid() || this->m_currentReplayEventIterator.Current()->EventTimeStamp == this->m_eventTimeCtr, "We are out of sync here");
#endif
    }

    bool EventLog::IsPropertyRecordRef(void* ref) const
    {
        //This is an ugly cast but we just want to know if the pointer is in the set so it is ok here
        return this->m_propertyRecordPinSet->ContainsKey((Js::PropertyRecord*)ref);
    }

    double EventLog::GetCurrentWallTime()
    {
        return this->m_timer.Now();
    }

    int64 EventLog::GetLastEventTime() const
    {
        return this->m_eventTimeCtr - 1;
    }

    NSLogEvents::EventLogEntry* EventLog::RecordJsRTCreateScriptContext(TTDJsRTActionResultAutoRecorder& actionPopper)
    {
        NSLogEvents::JsRTCreateScriptContextAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTCreateScriptContextAction, NSLogEvents::EventKind::CreateScriptContextActionTag>(&cAction);

        cAction->KnownObjects = this->m_eventSlabAllocator.SlabAllocateStruct<NSLogEvents::JsRTCreateScriptContextAction_KnownObjects>();
        cAction->KnownObjects = { 0 };

        actionPopper.InitializeWithEventAndEnter(evt);

        return evt;
    }

    void EventLog::RecordJsRTCreateScriptContextResult(NSLogEvents::EventLogEntry* evt, Js::ScriptContext* newCtx)
    {
        NSLogEvents::JsRTCreateScriptContextAction* cAction = NSLogEvents::GetInlineEventDataAs<NSLogEvents::JsRTCreateScriptContextAction, NSLogEvents::EventKind::CreateScriptContextActionTag>(evt);
        cAction->KnownObjects = this->m_eventSlabAllocator.SlabAllocateStruct<NSLogEvents::JsRTCreateScriptContextAction_KnownObjects>();

        cAction->GlobalObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(newCtx->GetGlobalObject());
        cAction->KnownObjects->UndefinedObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(newCtx->GetLibrary()->GetUndefined());
        cAction->KnownObjects->NullObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(newCtx->GetLibrary()->GetNull());
        cAction->KnownObjects->TrueObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(newCtx->GetLibrary()->GetTrue());
        cAction->KnownObjects->FalseObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(newCtx->GetLibrary()->GetFalse());
    }

    void EventLog::RecordJsRTSetCurrentContext(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var globalObject)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::SetActiveScriptContextActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(globalObject));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

#if !INT32VAR
    void EventLog::RecordJsRTCreateInteger(TTDJsRTActionResultAutoRecorder& actionPopper, int value)
    {
        NSLogEvents::JsRTIntegralArgumentAction* iAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTIntegralArgumentAction, NSLogEvents::EventKind::CreateIntegerActionTag>(&iAction);
        iAction->Scalar = value;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(iAction->Result));
    }
#endif

    void EventLog::RecordJsRTCreateNumber(TTDJsRTActionResultAutoRecorder& actionPopper, double value)
    {
        NSLogEvents::JsRTDoubleArgumentAction* dAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleArgumentAction, NSLogEvents::EventKind::CreateNumberActionTag>(&dAction);
        dAction->DoubleValue = value;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(dAction->Result));
    }

    void EventLog::RecordJsRTCreateBoolean(TTDJsRTActionResultAutoRecorder& actionPopper, bool value)
    {
        NSLogEvents::JsRTIntegralArgumentAction* bAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTIntegralArgumentAction, NSLogEvents::EventKind::CreateBooleanActionTag>(&bAction);
        bAction->Scalar = value;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(bAction->Result));
    }

    void EventLog::RecordJsRTCreateString(TTDJsRTActionResultAutoRecorder& actionPopper, const char16* stringValue, size_t stringLength)
    {
        NSLogEvents::JsRTStringArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTStringArgumentAction, NSLogEvents::EventKind::CreateStringActionTag>(&sAction);
        this->m_eventSlabAllocator.CopyStringIntoWLength(stringValue, (uint32)stringLength, sAction->StringValue);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateSymbol(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateSymbolActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateRangeError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateRangeErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateReferenceError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateReferenceErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateSyntaxError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateSyntaxErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateTypeError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateTypeErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTCreateURIError(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var msg)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* sAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::CreateURIErrorActionTag>(&sAction);
        NSLogEvents::SetVarItem_0(sAction, TTD_CONVERT_JSVAR_TO_TTDVAR(msg));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(sAction->Result));
    }

    void EventLog::RecordJsRTVarToNumberConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::VarConvertToNumberActionTag>(&cAction);
        NSLogEvents::SetVarItem_0(cAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTVarToBooleanConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::VarConvertToBooleanActionTag>(&cAction);
        NSLogEvents::SetVarItem_0(cAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTVarToStringConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::VarConvertToStringActionTag>(&cAction);
        NSLogEvents::SetVarItem_0(cAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTVarToObjectConversion(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::VarConvertToObjectActionTag>(&cAction);
        NSLogEvents::SetVarItem_0(cAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAddRootRef(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* addAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::AddRootRefActionTag>(&addAction);
        NSLogEvents::SetVarItem_0(addAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTAddWeakRootRef(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* addAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::AddWeakRootRefActionTag>(&addAction);
        NSLogEvents::SetVarItem_0(addAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTEventLoopYieldPoint()
    {
        NSLogEvents::EventLoopYieldPointEntry* ypEvt = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::EventLoopYieldPointEntry, NSLogEvents::EventKind::EventLoopYieldPointTag >();
        ypEvt->EventTimeStamp = this->GetLastEventTime();
        ypEvt->EventWallTime = this->GetCurrentWallTime();

        //Put this here in the hope that after handling an event there is an idle period where we can work without blocking user work
        if(this->IsTimeForSnapshot())
        {
            this->DoSnapshotExtract();
            this->PruneLogLength();
        }
    }

    void EventLog::RecordJsRTAllocateBasicObject(TTDJsRTActionResultAutoRecorder& actionPopper)
    {
        NSLogEvents::JsRTResultOnlyAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTResultOnlyAction, NSLogEvents::EventKind::AllocateObjectActionTag>(&cAction);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAllocateExternalObject(TTDJsRTActionResultAutoRecorder& actionPopper)
    {
        NSLogEvents::JsRTResultOnlyAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTResultOnlyAction, NSLogEvents::EventKind::AllocateExternalObjectActionTag>(&cAction);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAllocateBasicArray(TTDJsRTActionResultAutoRecorder& actionPopper, uint32 length)
    {
        NSLogEvents::JsRTIntegralArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTIntegralArgumentAction, NSLogEvents::EventKind::AllocateArrayActionTag>(&cAction);
        cAction->Scalar = length;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAllocateArrayBuffer(TTDJsRTActionResultAutoRecorder& actionPopper, uint32 size)
    {
        NSLogEvents::JsRTIntegralArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTIntegralArgumentAction, NSLogEvents::EventKind::AllocateArrayBufferActionTag>(&cAction);
        cAction->Scalar = size;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAllocateExternalArrayBuffer(TTDJsRTActionResultAutoRecorder& actionPopper, byte* buff, uint32 size)
    {
        NSLogEvents::JsRTByteBufferAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTByteBufferAction, NSLogEvents::EventKind::AllocateExternalArrayBufferActionTag>(&cAction);
        cAction->Length = size;

        cAction->Buffer = nullptr; 
        if(cAction->Length != 0)
        {
            cAction->Buffer = this->m_eventSlabAllocator.SlabAllocateArray<byte>(cAction->Length);
            js_memcpy_s(cAction->Buffer, cAction->Length, buff, size);
        }

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTAllocateFunction(TTDJsRTActionResultAutoRecorder& actionPopper, bool isNamed, Js::Var optName)
    {
        NSLogEvents::JsRTSingleVarScalarArgumentAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::AllocateFunctionActionTag>(&cAction);
        NSLogEvents::SetVarItem_0(cAction, TTD_CONVERT_JSVAR_TO_TTDVAR(optName));
        NSLogEvents::SetScalarItem_0(cAction, isNamed);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));
    }

    void EventLog::RecordJsRTHostExitProcess(TTDJsRTActionResultAutoRecorder& actionPopper, int32 exitCode)
    {
        NSLogEvents::JsRTIntegralArgumentAction* eAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTIntegralArgumentAction, NSLogEvents::EventKind::HostExitProcessTag>(&eAction);
        eAction->Scalar = exitCode;

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTGetAndClearExceptionWithMetadata(TTDJsRTActionResultAutoRecorder& actionPopper)
    {
        NSLogEvents::JsRTResultOnlyAction* gcAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTResultOnlyAction, NSLogEvents::EventKind::GetAndClearExceptionWithMetadataActionTag>(&gcAction);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gcAction->Result));
    }

    void EventLog::RecordJsRTGetAndClearException(TTDJsRTActionResultAutoRecorder& actionPopper)
    {
        NSLogEvents::JsRTResultOnlyAction* gcAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTResultOnlyAction, NSLogEvents::EventKind::GetAndClearExceptionActionTag>(&gcAction);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gcAction->Result));
    }

    void EventLog::RecordJsRTSetException(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, bool propagateToDebugger)
    {
        NSLogEvents::JsRTSingleVarScalarArgumentAction* spAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::SetExceptionActionTag>(&spAction);
        NSLogEvents::SetVarItem_0(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetScalarItem_0(spAction, propagateToDebugger);

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTHasProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTSingleVarScalarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::HasPropertyActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetPropertyIdItem(gpAction, pRecord->GetPropertyId());

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTHasOwnProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTSingleVarScalarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::HasOwnPropertyActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetPropertyIdItem(gpAction, pRecord->GetPropertyId());

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTInstanceOf(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var object, Js::Var constructor)
    {
        NSLogEvents::JsRTDoubleVarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarArgumentAction, NSLogEvents::EventKind::InstanceOfActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(object));
        NSLogEvents::SetVarItem_1(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(constructor));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTEquals(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var1, Js::Var var2, bool doStrict)
    {
        NSLogEvents::JsRTDoubleVarSingleScalarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarSingleScalarArgumentAction, NSLogEvents::EventKind::EqualsActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var1));
        NSLogEvents::SetVarItem_1(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var2));
        NSLogEvents::SetScalarItem_0(gpAction, doStrict);

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTGetPropertyIdFromSymbol(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var sym)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetPropertyIdFromSymbolTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(sym));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTGetPrototype(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetPrototypeActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gpAction->Result));
    }

    void EventLog::RecordJsRTGetProperty(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTSingleVarScalarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::GetPropertyActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetPropertyIdItem(gpAction, pRecord->GetPropertyId());

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gpAction->Result));
    }

    void EventLog::RecordJsRTGetIndex(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var index, Js::Var var)
    {
        NSLogEvents::JsRTDoubleVarArgumentAction* giAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarArgumentAction, NSLogEvents::EventKind::GetIndexActionTag>(&giAction);
        NSLogEvents::SetVarItem_0(giAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetVarItem_1(giAction, TTD_CONVERT_JSVAR_TO_TTDVAR(index));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(giAction->Result));
    }

    void EventLog::RecordJsRTGetOwnPropertyInfo(TTDJsRTActionResultAutoRecorder& actionPopper, const Js::PropertyRecord* pRecord, Js::Var var)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTSingleVarScalarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarScalarArgumentAction, NSLogEvents::EventKind::GetOwnPropertyInfoActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetPropertyIdItem(gpAction, pRecord->GetPropertyId());

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gpAction->Result));
    }

    void EventLog::RecordJsRTGetOwnPropertyNamesInfo(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetOwnPropertyNamesInfoActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gpAction->Result));
    }

    void EventLog::RecordJsRTGetOwnPropertySymbolsInfo(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* gpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetOwnPropertySymbolsInfoActionTag>(&gpAction);
        NSLogEvents::SetVarItem_0(gpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(gpAction->Result));
    }

    void EventLog::RecordJsRTDefineProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, Js::Var propertyDescriptor)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTDoubleVarSingleScalarArgumentAction* dpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarSingleScalarArgumentAction, NSLogEvents::EventKind::DefinePropertyActionTag>(&dpAction);
        NSLogEvents::SetVarItem_0(dpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetVarItem_1(dpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(propertyDescriptor));
        NSLogEvents::SetPropertyIdItem(dpAction, pRecord->GetPropertyId());

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTDeleteProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, bool useStrictRules)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTSingleVarDoubleScalarArgumentAction* dpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTSingleVarDoubleScalarArgumentAction, NSLogEvents::EventKind::DeletePropertyActionTag>(&dpAction);
        NSLogEvents::SetVarItem_0(dpAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetPropertyIdItem(dpAction, pRecord->GetPropertyId());
        NSLogEvents::SetScalarItem_1(dpAction, useStrictRules);

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(dpAction->Result));
    }

    void EventLog::RecordJsRTSetPrototype(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, Js::Var proto)
    {
        NSLogEvents::JsRTDoubleVarArgumentAction* spAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarArgumentAction, NSLogEvents::EventKind::SetPrototypeActionTag>(&spAction);
        NSLogEvents::SetVarItem_0(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetVarItem_1(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(proto));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTSetProperty(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, const Js::PropertyRecord* pRecord, Js::Var val, bool useStrictRules)
    {
        //The host may not have validated this yet (and will exit early if the check fails) so we check it here as well before getting the property id below
        if(pRecord == nullptr || Js::IsInternalPropertyId(pRecord->GetPropertyId()))
        {
            return;
        }

        NSLogEvents::JsRTDoubleVarDoubleScalarArgumentAction* spAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTDoubleVarDoubleScalarArgumentAction, NSLogEvents::EventKind::SetPropertyActionTag>(&spAction);
        NSLogEvents::SetVarItem_0(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetVarItem_1(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(val));
        NSLogEvents::SetPropertyIdItem(spAction, pRecord->GetPropertyId());
        NSLogEvents::SetScalarItem_1(spAction, useStrictRules);

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTSetIndex(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var var, Js::Var index, Js::Var val)
    {
        NSLogEvents::JsRTTrippleVarArgumentAction* spAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTTrippleVarArgumentAction, NSLogEvents::EventKind::SetIndexActionTag>(&spAction);
        NSLogEvents::SetVarItem_0(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));
        NSLogEvents::SetVarItem_1(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(index));
        NSLogEvents::SetVarItem_2(spAction, TTD_CONVERT_JSVAR_TO_TTDVAR(val));

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTGetTypedArrayInfo(Js::Var var, Js::Var result)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* giAction = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetTypedArrayInfoActionTag>();
        NSLogEvents::SetVarItem_0(giAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        // entry/exit status should be set to clear by initialization so don't need to do anything
        giAction->Result = TTD_CONVERT_JSVAR_TO_TTDVAR(result);
    }

    void EventLog::RecordJsRTGetDataViewInfo(Js::Var var, Js::Var result)
    {
        NSLogEvents::JsRTSingleVarArgumentAction* giAction = this->RecordGetInitializedEvent_DataOnly<NSLogEvents::JsRTSingleVarArgumentAction, NSLogEvents::EventKind::GetDataViewInfoActionTag>();
        NSLogEvents::SetVarItem_0(giAction, TTD_CONVERT_JSVAR_TO_TTDVAR(var));

        // entry/exit status should be set to clear by initialization so don't need to do anything
        giAction->Result = TTD_CONVERT_JSVAR_TO_TTDVAR(result);
    }

    void EventLog::RecordJsRTRawBufferCopySync(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 dstIndex, Js::Var src, uint32 srcIndex, uint32 length)
    {
        TTDAssert(Js::ArrayBuffer::Is(dst) && Js::ArrayBuffer::Is(src), "Not array buffer objects!!!");
        TTDAssert(dstIndex + length <= Js::ArrayBuffer::FromVar(dst)->GetByteLength(), "Copy off end of buffer!!!");
        TTDAssert(srcIndex + length <= Js::ArrayBuffer::FromVar(src)->GetByteLength(), "Copy off end of buffer!!!");

        NSLogEvents::JsRTRawBufferCopyAction* rbcAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTRawBufferCopyAction, NSLogEvents::EventKind::RawBufferCopySync>(&rbcAction);
        rbcAction->Dst = TTD_CONVERT_JSVAR_TO_TTDVAR(dst);
        rbcAction->Src = TTD_CONVERT_JSVAR_TO_TTDVAR(src);
        rbcAction->DstIndx = dstIndex;
        rbcAction->SrcIndx = srcIndex;
        rbcAction->Count = length;

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTRawBufferModifySync(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 index, uint32 count)
    {
        TTDAssert(Js::ArrayBuffer::Is(dst), "Not array buffer object!!!");
        TTDAssert(index + count <= Js::ArrayBuffer::FromVar(dst)->GetByteLength(), "Copy off end of buffer!!!");

        NSLogEvents::JsRTRawBufferModifyAction* rbmAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTRawBufferModifyAction, NSLogEvents::EventKind::RawBufferModifySync>(&rbmAction);
        rbmAction->Trgt = TTD_CONVERT_JSVAR_TO_TTDVAR(dst);
        rbmAction->Index = index;
        rbmAction->Length = count;

        rbmAction->Data = (rbmAction->Length != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<byte>(rbmAction->Length) : nullptr;
        byte* copyBuff = Js::ArrayBuffer::FromVar(dst)->GetBuffer() + index;
        js_memcpy_s(rbmAction->Data, rbmAction->Length, copyBuff, count);

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTRawBufferAsyncModificationRegister(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var dst, uint32 index)
    {
            NSLogEvents::JsRTRawBufferModifyAction* rbrAction = nullptr;
            NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTRawBufferModifyAction, NSLogEvents::EventKind::RawBufferAsyncModificationRegister>(&rbrAction);
            rbrAction->Trgt = TTD_CONVERT_JSVAR_TO_TTDVAR(dst);
            rbrAction->Index = index;

            actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTRawBufferAsyncModifyComplete(TTDJsRTActionResultAutoRecorder& actionPopper, TTDPendingAsyncBufferModification& pendingAsyncInfo, byte* finalModPos)
    {
        Js::ArrayBuffer* dstBuff = Js::ArrayBuffer::FromVar(pendingAsyncInfo.ArrayBufferVar);
        byte* copyBuff = dstBuff->GetBuffer() + pendingAsyncInfo.Index;

        NSLogEvents::JsRTRawBufferModifyAction* rbrAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTRawBufferModifyAction, NSLogEvents::EventKind::RawBufferAsyncModifyComplete>(&rbrAction);
        rbrAction->Trgt = TTD_CONVERT_JSVAR_TO_TTDVAR(dstBuff);
        rbrAction->Index = (uint32)pendingAsyncInfo.Index;
        rbrAction->Length = (uint32)(finalModPos - copyBuff);

        rbrAction->Data = (rbrAction->Length != 0) ? this->m_eventSlabAllocator.SlabAllocateArray<byte>(rbrAction->Length) : nullptr;
        js_memcpy_s(rbrAction->Data, rbrAction->Length, copyBuff, rbrAction->Length);

        actionPopper.InitializeWithEventAndEnter(evt);
    }

    void EventLog::RecordJsRTConstructCall(TTDJsRTActionResultAutoRecorder& actionPopper, Js::Var funcVar, uint32 argCount, Js::Var* args)
    {
        NSLogEvents::JsRTConstructCallAction* ccAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTConstructCallAction, NSLogEvents::EventKind::ConstructCallActionTag>(&ccAction);

        ccAction->ArgCount = argCount + 1;

        static_assert(sizeof(TTDVar) == sizeof(Js::Var), "These need to be the same size (and have same bit layout) for this to work!");

        ccAction->ArgArray = this->m_eventSlabAllocator.SlabAllocateArray<TTDVar>(ccAction->ArgCount);
        ccAction->ArgArray[0] = TTD_CONVERT_JSVAR_TO_TTDVAR(funcVar);
        js_memcpy_s(ccAction->ArgArray + 1, (ccAction->ArgCount - 1) * sizeof(TTDVar), args, argCount * sizeof(Js::Var));

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(ccAction->Result));
    }

    NSLogEvents::EventLogEntry* EventLog::RecordJsRTCodeParse(TTDJsRTActionResultAutoRecorder& actionPopper, LoadScriptFlag loadFlag, bool isUft8, const byte* script, uint32 scriptByteLength, uint64 sourceContextId, const char16* sourceUri)
    {
        NSLogEvents::JsRTCodeParseAction* cpAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTCodeParseAction, NSLogEvents::EventKind::CodeParseActionTag>(&cpAction);

        cpAction->BodyCtrId = 0; //initialize to known default -- should always update later or something is wrong

        cpAction->IsUtf8 = isUft8;
        cpAction->SourceByteLength = scriptByteLength;

        cpAction->SourceCode = this->m_eventSlabAllocator.SlabAllocateArray<byte>(cpAction->SourceByteLength);
        js_memcpy_s(cpAction->SourceCode, cpAction->SourceByteLength, script, scriptByteLength);

        this->m_eventSlabAllocator.CopyNullTermStringInto(sourceUri, cpAction->SourceUri);
        cpAction->SourceContextId = sourceContextId;

        cpAction->LoadFlag = loadFlag;

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cpAction->Result));

        return evt;
    }

    NSLogEvents::EventLogEntry* EventLog::RecordJsRTCallFunction(TTDJsRTActionResultAutoRecorder& actionPopper, int32 rootDepth, Js::Var funcVar, uint32 argCount, Js::Var* args)
    {
        NSLogEvents::JsRTCallFunctionAction* cAction = nullptr;
        NSLogEvents::EventLogEntry* evt = this->RecordGetInitializedEvent<NSLogEvents::JsRTCallFunctionAction, NSLogEvents::EventKind::CallExistingFunctionActionTag>(&cAction);

        int64 evtTime = this->GetLastEventTime();
        int64 topLevelCallTime = (rootDepth == 0) ? evtTime : this->m_topLevelCallbackEventTime;
        NSLogEvents::JsRTCallFunctionAction_ProcessArgs(evt, rootDepth, evtTime, funcVar, argCount, args, topLevelCallTime, this->m_eventSlabAllocator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        NSLogEvents::JsRTCallFunctionAction_ProcessDiagInfoPre(evt, funcVar, this->m_eventSlabAllocator);
#endif

        actionPopper.InitializeWithEventAndEnterWResult(evt, &(cAction->Result));

        return evt;
    }

    void EventLog::EmitLog(const char* emitUri, size_t emitUriLength)
    {
#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
        this->m_threadContext->TTDExecutionInfo->GetTraceLogger()->ForceFlush();
#endif

        TTDataIOInfo& iofp = this->m_threadContext->TTDContext->TTDataIOInfo;
        iofp.ActiveTTUriLength = emitUriLength;
        iofp.ActiveTTUri = emitUri;

        const char* logfilename = "ttdlog.log";
        JsTTDStreamHandle logHandle = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(logfilename), logfilename, false, true);
        TTDAssert(logHandle != nullptr, "Failed to initialize strem for writing TTD Log.");

        TTD_LOG_WRITER writer(logHandle, iofp.pfWriteBytesToStream, iofp.pfFlushAndCloseStream);

        writer.WriteRecordStart();
        writer.AdjustIndent(1);

        TTString archString;
#if defined(_M_IX86)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("x86"), archString);
#elif defined(_M_X64)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("x64"), archString);
#elif defined(_M_ARM)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("arm"), archString);
#elif defined(_M_ARM64)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("arm64"), archString);
#else
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("unknown"), archString);
#endif

        writer.WriteString(NSTokens::Key::arch, archString);

        TTString platformString;
#if defined(_WIN32)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("Windows"), platformString);
#elif defined(__APPLE__)
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("macOS"), platformString);
#else
        this->m_miscSlabAllocator.CopyNullTermStringInto(_u("Linux"), platformString);
#endif

        writer.WriteString(NSTokens::Key::platform, platformString, NSTokens::Separator::CommaSeparator);

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

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        JsUtil::Stack<int64, HeapAllocator> callNestingStack(&HeapAllocator::Instance);
#endif

        bool firstElem = true;

        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        writer.WriteSeperator(NSTokens::Separator::BigSpaceSeparator);
        for(auto iter = this->m_eventList.GetIteratorAtFirst(); iter.IsValid(); iter.MoveNext())
        {
            const NSLogEvents::EventLogEntry* evt = iter.Current();

            NSTokens::Separator sep = firstElem ? NSTokens::Separator::NoSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSLogEvents::EventLogEntry_Emit(evt, this->m_eventListVTable, &writer, this->m_threadContext, sep);

            firstElem = false;
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            bool isJsRTCall = (evt->EventKind == NSLogEvents::EventKind::CallExistingFunctionActionTag);
            bool isExternalCall = (evt->EventKind == NSLogEvents::EventKind::ExternalCallTag);
            bool isRegisterCall = (evt->EventKind == NSLogEvents::EventKind::ExternalCbRegisterCall);
            if(isJsRTCall | isExternalCall | isRegisterCall)
            {
                writer.WriteSequenceStart(NSTokens::Separator::BigSpaceSeparator);

                int64 lastNestedTime = -1;
                if(isJsRTCall)
                {
                    lastNestedTime = NSLogEvents::JsRTCallFunctionAction_GetLastNestedEventTime(evt);
                }
                else if(isExternalCall)
                {
                    lastNestedTime = NSLogEvents::ExternalCallEventLogEntry_GetLastNestedEventTime(evt);
                }
                else
                {
                    lastNestedTime = NSLogEvents::ExternalCbRegisterCallEventLogEntry_GetLastNestedEventTime(evt);
                }
                callNestingStack.Push(lastNestedTime);

                if(lastNestedTime != evt->EventTimeStamp)
                {
                    writer.AdjustIndent(1);

                    writer.WriteSeperator(NSTokens::Separator::BigSpaceSeparator);
                    firstElem = true;
                }
            }

            if(!callNestingStack.Empty() && evt->EventTimeStamp == callNestingStack.Peek())
            {
                int64 eTime = callNestingStack.Pop();

                if(!isJsRTCall & !isExternalCall & !isRegisterCall)
                {
                    writer.AdjustIndent(-1);
                    writer.WriteSeperator(NSTokens::Separator::BigSpaceSeparator);
                }
                writer.WriteSequenceEnd();

                while(!callNestingStack.Empty() && eTime == callNestingStack.Peek())
                {
                    callNestingStack.Pop();

                    writer.AdjustIndent(-1);
                    writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
                }
            }
#endif
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        //emit the properties
        writer.WriteLengthValue(this->m_propertyRecordPinSet->Count(), NSTokens::Separator::CommaSeparator);

        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        bool firstProperty = true;
        for(auto iter = this->m_propertyRecordPinSet->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSTokens::Separator sep = (!firstProperty) ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSSnapType::EmitPropertyRecordAsSnapPropertyRecord(iter.CurrentValue(), &writer, sep);

            firstProperty = false;
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        //do top level script processing here
        writer.WriteUInt32(NSTokens::Key::u32Val, this->m_sourceInfoCount, NSTokens::Separator::CommaSeparator);

        writer.WriteLengthValue(this->m_loadedTopLevelScripts.Count(), NSTokens::Separator::CommaSeparator);
        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        bool firstLoadScript = true;
        for(auto iter = this->m_loadedTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSTokens::Separator sep = (!firstLoadScript) ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSSnapValues::EmitTopLevelLoadedFunctionBodyInfo(iter.Current(), this->m_threadContext, &writer, sep);

            firstLoadScript = false;
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        writer.WriteLengthValue(this->m_newFunctionTopLevelScripts.Count(), NSTokens::Separator::CommaSeparator);
        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        bool firstNewScript = true;
        for(auto iter = this->m_newFunctionTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSTokens::Separator sep = (!firstNewScript) ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSSnapValues::EmitTopLevelNewFunctionBodyInfo(iter.Current(), this->m_threadContext, &writer, sep);

            firstNewScript = false;
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        writer.WriteLengthValue(this->m_evalTopLevelScripts.Count(), NSTokens::Separator::CommaSeparator);
        writer.WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        writer.AdjustIndent(1);
        bool firstEvalScript = true;
        for(auto iter = this->m_evalTopLevelScripts.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSTokens::Separator sep = (!firstEvalScript) ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;
            NSSnapValues::EmitTopLevelEvalFunctionBodyInfo(iter.Current(), this->m_threadContext, &writer, sep);

            firstEvalScript = false;
        }
        writer.AdjustIndent(-1);
        writer.WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
        //

        writer.AdjustIndent(-1);
        writer.WriteRecordEnd(NSTokens::Separator::BigSpaceSeparator);

        writer.FlushAndClose();

        iofp.ActiveTTUriLength = 0;
        iofp.ActiveTTUri = nullptr;
    }

    void EventLog::ParseLogInto(TTDataIOInfo& iofp, const char* parseUri, size_t parseUriLength)
    {
        iofp.ActiveTTUriLength = parseUriLength;
        iofp.ActiveTTUri = parseUri;

        const char* logfilename = "ttdlog.log";
        JsTTDStreamHandle logHandle = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(logfilename), logfilename, true, false);
        TTDAssert(logHandle != nullptr, "Failed to initialize strem for reading TTD Log.");

        TTD_LOG_READER reader(logHandle, iofp.pfReadBytesFromStream, iofp.pfFlushAndCloseStream);

        reader.ReadRecordStart();

        TTString archString;
        reader.ReadString(NSTokens::Key::arch, this->m_miscSlabAllocator, archString);

#if defined(_M_IX86)
        TTDAssert(wcscmp(_u("x86"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#elif defined(_M_X64)
        TTDAssert(wcscmp(_u("x64"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#elif defined(_M_ARM)
        TTDAssert(wcscmp(_u("arm64"), archString.Contents) == 0, "Mismatch in arch between record and replay!!!");
#else
        TTDAssert(false, "Unknown arch!!!");
#endif

        //This is informational only so just read off the value and ignore
        TTString platformString;
        reader.ReadString(NSTokens::Key::platform, this->m_miscSlabAllocator, platformString, true);

        bool diagEnabled = reader.ReadBool(NSTokens::Key::diagEnabled, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTDAssert(diagEnabled, "Diag was enabled in record so it shoud be in replay as well!!!");
#else
        TTDAssert(!diagEnabled, "Diag was *not* enabled in record so it shoud *not* be in replay either!!!");
#endif

        reader.ReadUInt64(NSTokens::Key::usedMemory, true);
        reader.ReadUInt64(NSTokens::Key::reservedMemory, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        JsUtil::Stack<int64, HeapAllocator> callNestingStack(&HeapAllocator::Instance);

        bool doSep = false;
#endif

        uint32 ecount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < ecount; ++i)
        {
            NSLogEvents::EventKind evtKind = NSLogEvents::EventLogEntry_ParseHeader(false, &reader);

            NSLogEvents::EventLogEntry* evt = this->m_eventList.GetNextAvailableEntry(this->m_eventListVTable[(uint32)evtKind].DataSize);
            evt->EventKind = evtKind;

            NSLogEvents::EventLogEntry_ParseRest(evt, this->m_eventListVTable, this->m_threadContext, &reader, this->m_eventSlabAllocator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            bool isJsRTCall = (evt->EventKind == NSLogEvents::EventKind::CallExistingFunctionActionTag);
            bool isExternalCall = (evt->EventKind == NSLogEvents::EventKind::ExternalCallTag);
            bool isRegisterCall = (evt->EventKind == NSLogEvents::EventKind::ExternalCbRegisterCall);
            if(isJsRTCall | isExternalCall | isRegisterCall)
            {
                reader.ReadSequenceStart(false);

                int64 lastNestedTime = -1;
                if(isJsRTCall)
                {
                    lastNestedTime = NSLogEvents::JsRTCallFunctionAction_GetLastNestedEventTime(evt);
                }
                else if(isExternalCall)
                {
                    lastNestedTime = NSLogEvents::ExternalCallEventLogEntry_GetLastNestedEventTime(evt);
                }
                else
                {
                    lastNestedTime = NSLogEvents::ExternalCbRegisterCallEventLogEntry_GetLastNestedEventTime(evt);
                }
                callNestingStack.Push(lastNestedTime);
            }

            doSep = (!isJsRTCall & !isExternalCall & !isRegisterCall);

            while(callNestingStack.Count() != 0 && evt->EventTimeStamp == callNestingStack.Peek())
            {
                callNestingStack.Pop();
                reader.ReadSequenceEnd();
            }
#endif
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

        //do top level script processing here
        this->m_sourceInfoCount = reader.ReadUInt32(NSTokens::Key::u32Val, true);

        uint32 loadedScriptCount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < loadedScriptCount; ++i)
        {
            NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo = this->m_loadedTopLevelScripts.NextOpenEntry();
            NSSnapValues::ParseTopLevelLoadedFunctionBodyInfo(fbInfo, i != 0, this->m_threadContext, &reader, this->m_miscSlabAllocator);
        }
        reader.ReadSequenceEnd();

        uint32 newScriptCount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < newScriptCount; ++i)
        {
            NSSnapValues::TopLevelNewFunctionBodyResolveInfo* fbInfo = this->m_newFunctionTopLevelScripts.NextOpenEntry();
            NSSnapValues::ParseTopLevelNewFunctionBodyInfo(fbInfo, i != 0, this->m_threadContext, &reader, this->m_miscSlabAllocator);
        }
        reader.ReadSequenceEnd();

        uint32 evalScriptCount = reader.ReadLengthValue(true);
        reader.ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < evalScriptCount; ++i)
        {
            NSSnapValues::TopLevelEvalFunctionBodyResolveInfo* fbInfo = this->m_evalTopLevelScripts.NextOpenEntry();
            NSSnapValues::ParseTopLevelEvalFunctionBodyInfo(fbInfo, i != 0, this->m_threadContext, &reader, this->m_miscSlabAllocator);
        }
        reader.ReadSequenceEnd();
        //

        reader.ReadRecordEnd();

        //After reading setup the previous event map
        this->m_eventList.InitializePreviousEventMap();
    }
}

#endif
