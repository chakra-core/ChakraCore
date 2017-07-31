//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

#define TTD_EVENT_BASE_SIZE sizeof(TTD::NSLogEvents::EventLogEntry)
#define TTD_EVENT_PLUS_DATA_SIZE_DIRECT(S) TTD_WORD_ALIGN_ALLOC_SIZE(TTD_EVENT_BASE_SIZE + S)
#define TTD_EVENT_PLUS_DATA_SIZE(T) TTD_WORD_ALIGN_ALLOC_SIZE(TTD_EVENT_BASE_SIZE + sizeof(T))

//The limit on event times used as the default value
#define TTD_EVENT_MAXTIME INT64_MAX

//Values copied from ChakraCommon.h
#define TTD_REPLAY_JS_INVALID_REFERENCE nullptr
#define TTD_REPLAY_JsErrorInvalidArgument 65537
#define TTD_REPLAY_JsErrorArgumentNotObject 65548
#define TTD_REPLAY_JsErrorCategoryScript 196609
#define TTD_REPLAY_JsErrorScriptTerminated 196611


#define TTD_REPLAY_VALIDATE_JSREF(p) \
        if (p == TTD_REPLAY_JS_INVALID_REFERENCE) \
        { \
            return; \
        }

#define TTD_REPLAY_MARSHAL_OBJECT(p, scriptContext) \
        Js::RecyclableObject* __obj = Js::RecyclableObject::FromVar(p); \
        if (__obj->GetScriptContext() != scriptContext) \
        { \
            p = Js::CrossSite::MarshalVar(scriptContext, __obj); \
        }

#define TTD_REPLAY_VALIDATE_INCOMING_REFERENCE(p, scriptContext) \
        TTD_REPLAY_VALIDATE_JSREF(p); \
        if (Js::RecyclableObject::Is(p)) \
        { \
            TTD_REPLAY_MARSHAL_OBJECT(p, scriptContext) \
        }

#define TTD_REPLAY_VALIDATE_INCOMING_OBJECT(p, scriptContext) \
        { \
            TTD_REPLAY_VALIDATE_JSREF(p); \
            if(!Js::JavascriptOperators::IsObject(p)) \
            { \
                return; \
            } \
            TTD_REPLAY_MARSHAL_OBJECT(p, scriptContext) \
        }

#define TTD_REPLAY_VALIDATE_INCOMING_OBJECT_OR_NULL(p, scriptContext) \
        { \
            TTD_REPLAY_VALIDATE_JSREF(p); \
            if(!Js::JavascriptOperators::IsObjectOrNull(p)) \
            { \
                return; \
            } \
            TTD_REPLAY_MARSHAL_OBJECT(p, scriptContext) \
        }

#define TTD_REPLAY_VALIDATE_INCOMING_FUNCTION(p, scriptContext) \
        { \
            TTD_REPLAY_VALIDATE_JSREF(p); \
            if(!Js::JavascriptFunction::Is(p)) \
            { \
                return; \
            } \
            TTD_REPLAY_MARSHAL_OBJECT(p, scriptContext) \
        }

#define TTD_REPLAY_ACTIVE_CONTEXT(executeContext) \
        Js::ScriptContext* ctx = executeContext->GetActiveScriptContext(); \
        TTDAssert(ctx != nullptr, "This should be non-null!!!");

namespace TTD
{
    namespace NSLogEvents
    {
        //An enumeration of the event kinds in the system
        enum class EventKind : uint32
        {
            Invalid = 0x0,
            //Tags for internal engine events
            SnapshotTag,
            EventLoopYieldPointTag,
            TopLevelCodeTag,
            TelemetryLogTag,
            DoubleTag,
            StringTag,
            RandomSeedTag,
            PropertyEnumTag,
            SymbolCreationTag,
            WeakCollectionContainsTag,
            ExternalCbRegisterCall,
            ExternalCallTag,
            ExplicitLogWriteTag,
            //JsRTActionTag is a marker for where the JsRT actions begin
            JsRTActionTag,

            CreateScriptContextActionTag,
            SetActiveScriptContextActionTag,

#if !INT32VAR
            CreateIntegerActionTag,
#endif
            CreateNumberActionTag,
            CreateBooleanActionTag,
            CreateStringActionTag,
            CreateSymbolActionTag,

            CreateErrorActionTag,
            CreateRangeErrorActionTag,
            CreateReferenceErrorActionTag,
            CreateSyntaxErrorActionTag,
            CreateTypeErrorActionTag,
            CreateURIErrorActionTag,

            VarConvertToNumberActionTag,
            VarConvertToBooleanActionTag,
            VarConvertToStringActionTag,
            VarConvertToObjectActionTag,

            AddRootRefActionTag,
            AddWeakRootRefActionTag,
            //No remove weak root ref, this is handled when syncing at snapshot points

            AllocateObjectActionTag,
            AllocateExternalObjectActionTag,
            AllocateArrayActionTag,
            AllocateArrayBufferActionTag,
            AllocateExternalArrayBufferActionTag,
            AllocateFunctionActionTag,

            HostExitProcessTag,
            GetAndClearExceptionWithMetadataActionTag,
            GetAndClearExceptionActionTag,
            SetExceptionActionTag,

            HasPropertyActionTag,
            InstanceOfActionTag,
            EqualsActionTag,

            GetPropertyIdFromSymbolTag,

            GetPrototypeActionTag,
            GetPropertyActionTag,
            GetIndexActionTag,
            GetOwnPropertyInfoActionTag,
            GetOwnPropertyNamesInfoActionTag,
            GetOwnPropertySymbolsInfoActionTag,

            DefinePropertyActionTag,
            DeletePropertyActionTag,
            SetPrototypeActionTag,
            SetPropertyActionTag,
            SetIndexActionTag,

            GetTypedArrayInfoActionTag,

            RawBufferCopySync,
            RawBufferModifySync,
            RawBufferAsyncModificationRegister,
            RawBufferAsyncModifyComplete,

            ConstructCallActionTag,
            CodeParseActionTag,
            CallExistingFunctionActionTag,

            HasOwnPropertyActionTag,
            GetDataViewInfoActionTag,

            Count
        };

        //Inflate an argument variable for an action during replay and record passing an value to the host
        void PassVarToHostInReplay(ThreadContextTTD* executeContext, TTDVar origVar, Js::Var replayVar);
        Js::Var InflateVarInReplay(ThreadContextTTD* executeContext, TTDVar var);

        //The kind of context that the replay code should execute in
        enum class ContextExecuteKind
        {
            None,
            GlobalAPIWrapper,
            ContextAPIWrapper,
            ContextAPINoScriptWrapper
        };

        typedef void(*fPtr_EventLogActionEntryInfoExecute)(const EventLogEntry* evt, ThreadContextTTD* execCtx);

        typedef void(*fPtr_EventLogEntryInfoUnload)(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        typedef void(*fPtr_EventLogEntryInfoEmit)(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        typedef void(*fPtr_EventLogEntryInfoParse)(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct that we use for our pseudo v-table on the EventLogEntry data
        struct EventLogEntryVTableEntry
        {
            ContextExecuteKind ContextKind;

            fPtr_EventLogActionEntryInfoExecute ExecuteFP;

            fPtr_EventLogEntryInfoUnload UnloadFP;
            fPtr_EventLogEntryInfoEmit EmitFP;
            fPtr_EventLogEntryInfoParse ParseFP;

            size_t DataSize;
        };

        //A base struct for our event log entries -- we will use the kind tags as v-table values
        //Data is stored in buffer memory immediately following this struct
        struct EventLogEntry
        {
            //The kind of the event
            EventKind EventKind;

            //The result status code
            int32 ResultStatus;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //The event time for this event
            int64 EventTimeStamp;
#endif
        };

        template <typename T, EventKind tag>
        const T* GetInlineEventDataAs(const EventLogEntry* evt)
        {
            TTDAssert(evt->EventKind == tag, "Bad tag match!");

            return reinterpret_cast<const T*>(((byte*)evt) + TTD_EVENT_BASE_SIZE);
        }

        template <typename T, EventKind tag>
        T* GetInlineEventDataAs(EventLogEntry* evt)
        {
            TTDAssert(evt->EventKind == tag, "Bad tag match!");

            return reinterpret_cast<T*>(((byte*)evt) + TTD_EVENT_BASE_SIZE);
        }

        template<EventKind tag>
        void EventLogEntry_Initialize(EventLogEntry* evt, int64 etime)
        {
            evt->EventKind = tag;
            evt->ResultStatus = -1;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            static_assert(TTD_EVENT_BASE_SIZE % 4 == 0, "This should always be word aligned.");
            AssertMsg(((uint64)evt) % 4 == 0, "We want this word aligned for performance so who messed it up.");

            evt->EventTimeStamp = etime;
#endif
        }

        //Helpers for initializing, emitting and parsing the basic event data
        void EventLogEntry_Emit(const EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator);
        EventKind EventLogEntry_ParseHeader(bool readSeperator, FileReader* reader);
        void EventLogEntry_ParseRest(EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        bool EventFailsWithRuntimeError(const EventLogEntry* evt);
        bool EventDoesNotReturn(const EventLogEntry* evt);
        bool EventCompletesNormally(const EventLogEntry* evt);
        bool EventCompletesWithException(const EventLogEntry* evt);

        //////////////////

        //A struct that represents snapshot events
        struct SnapshotEventLogEntry
        {
            //The timestamp we should restore to 
            int64 RestoreTimestamp;

            //The snapshot (we many persist this to disk and inflate back in later)
            SnapShot* Snap;

            //The logids of live contexts
            uint32 LiveContextCount;
            TTD_LOG_PTR_ID* LiveContextIdArray;

            //The logids of live roots (with non-zero weak references)
            uint32 LongLivedRefRootsCount;
            TTD_LOG_PTR_ID* LongLivedRefRootsIdArray;
        };

        void SnapshotEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void SnapshotEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void SnapshotEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        void SnapshotEventLogEntry_EnsureSnapshotDeserialized(EventLogEntry* evt, ThreadContext* threadContext);
        void SnapshotEventLogEntry_UnloadSnapshot(EventLogEntry* evt);

        //A struct that represents snapshot events
        struct EventLoopYieldPointEntry
        {
            //The timestamp of this yieldpoint
            int64 EventTimeStamp;

            //The wall clock time when this point is reached
            double EventWallTime;
        };

        void EventLoopYieldPointEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void EventLoopYieldPointEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that represents a top level code load event
        struct CodeLoadEventLogEntry
        {
            //The code counter id for the TopLevelFunctionBodyInfo
            uint32 BodyCounterId;
        };

        void CodeLoadEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void CodeLoadEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct that represents telemetry events from the user code
        struct TelemetryEventLogEntry
        {
            //A string that contains all of the info that is logged
            TTString InfoString;

            //Do we want to print the msg or just record it internally
            bool DoPrint;
        };

        void TelemetryEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void TelemetryEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void TelemetryEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that represents the generation of random seeds
        struct RandomSeedEventLogEntry
        {
            //The values associated with the event
            uint64 Seed0;
            uint64 Seed1;
        };

        void RandomSeedEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void RandomSeedEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct that represents a simple event that needs a double value (e.g. date values)
        struct DoubleEventLogEntry
        {
            //The value associated with the event
            double DoubleValue;
        };

        void DoubleEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void DoubleEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct that represents a simple event that needs a string value (e.g. date values)
        struct StringValueEventLogEntry
        {
            //The value associated with the event
            TTString StringValue;
        };

        void StringValueEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void StringValueEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void StringValueEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that represents a single enumeration step for properties on a dynamic object
        struct PropertyEnumStepEventLogEntry
        {
            //The return code, property id, and attributes returned
            BOOL ReturnCode;
            Js::PropertyId Pid;
            Js::PropertyAttributes Attributes;

            //Optional property name string (may need to actually use later if pid can be Constants::NoProperty)
            //Always set if if doing extra diagnostics otherwise only as needed
            TTString PropertyString;
        };

        void PropertyEnumStepEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void PropertyEnumStepEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void PropertyEnumStepEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that represents the creation of a symbol (which we need to make sure gets the correct property id)
        struct SymbolCreationEventLogEntry
        {
            //The property id of the created symbol
            Js::PropertyId Pid;
        };

        void SymbolCreationEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void SymbolCreationEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that tracks if a weak key value is in a weak collection
        struct WeakCollectionContainsEventLogEntry
        {
            bool ContainsValue;
        };

        void WeakCollectionContainsEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void WeakCollectionContainsEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct for logging the invocation of the host callback registration function
        struct ExternalCbRegisterCallEventLogEntry
        {
            //the number of arguments and the argument array -- function is always argument[0]
            TTDVar CallbackFunction;

            //The last event time that is nested in this external call
            int64 LastNestedEventTime;
        };

        int64 ExternalCbRegisterCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt);

        void ExternalCbRegisterCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void ExternalCbRegisterCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct for logging calls from Chakra to an external function (e.g., record start of external execution and later any argument information)
        struct ExternalCallEventLogEntry
        {
            //The root nesting depth
            int32 RootNestingDepth;

            //the number of arguments and the argument array -- function is always argument[0]
            uint32 ArgCount;
            TTDVar* ArgArray;

            //The return value of the external call
            TTDVar ReturnValue;

            //The last event time that is nested in this external call
            int64 LastNestedEventTime;

            //if we need to check exception information
            bool CheckExceptionStatus;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //the function name for the function that is invoked
            TTString FunctionName;
#endif
        };

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void ExternalCallEventLogEntry_ProcessDiagInfoPre(EventLogEntry* evt, Js::JavascriptFunction* function, UnlinkableSlabAllocator& alloc);
#endif

        int64 ExternalCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt);

        void ExternalCallEventLogEntry_ProcessArgs(EventLogEntry* evt, int32 rootDepth, Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool checkExceptions, UnlinkableSlabAllocator& alloc);
        void ExternalCallEventLogEntry_ProcessReturn(EventLogEntry* evt, Js::Var res, int64 lastNestedEvent);

        void ExternalCallEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void ExternalCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void ExternalCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct for when we explicitly write a log entry -- currently empty placeholder
        struct ExplicitLogWriteEventLogEntry
        {
            ;
        };

        void ExplicitLogWriteEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void ExplicitLogWriteEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);
    }
}

#endif
