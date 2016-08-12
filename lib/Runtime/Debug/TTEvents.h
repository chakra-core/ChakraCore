//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

//Ideally we want this to be a multiple of 8 when added to the tag sizes in the EventLogEntry struct.
//It should also be of a size that allows us to inline the event data for the most common events without being too wasteful on other events.
#define EVENT_INLINE_DATA_BYTE_COUNT 36

//The limit on event times used as the default value
#define TTD_EVENT_MAXTIME INT64_MAX

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

        uint64 EventTime; //The event time when the function was called
        uint64 FunctionTime; //The function time when the function was called
        uint64 LoopTime; //The current loop taken time for the function

#if ENABLE_TTD_STACK_STMTS
        int32 LastStatementIndex; //The previously executed statement
        uint64 LastStatementLoopTime; //The previously executed statement

        int32 CurrentStatementIndex; //The currently executing statement
        uint64 CurrentStatementLoopTime; //The currently executing statement

        //bytecode range of the current stmt
        uint32 CurrentStatementBytecodeMin;
        uint32 CurrentStatementBytecodeMax;
#endif
    };

    //A class to represent a source location
    class TTDebuggerSourceLocation
    {
    private:
        //The time aware parts of this location
        int64 m_etime;  //-1 indicates an INVALID location
        int64 m_ftime;  //-1 indicates any ftime is OK
        int64 m_ltime;  //-1 indicates any ltime is OK

        //The document
        wchar* m_sourceFile; //temp use until we make docid stable
        uint32 m_docid;

        //The position of the function in the document
        uint32 m_functionLine;
        uint32 m_functionColumn;

        //The location in the fnuction
        uint32 m_line;
        uint32 m_column;

    public:
        TTDebuggerSourceLocation();
        TTDebuggerSourceLocation(const SingleCallCounter& callFrame);
        TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other);
        ~TTDebuggerSourceLocation();

        TTDebuggerSourceLocation& operator= (const TTDebuggerSourceLocation& other);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void PrintToConsole(bool newline) const;
#endif

        void Initialize();

        bool HasValue() const;
        void Clear();
        void SetLocation(const TTDebuggerSourceLocation& other);
        void SetLocation(const SingleCallCounter& callFrame);
        void SetLocation(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column);

        int64 GetRootEventTime() const;
        int64 GetFunctionTime() const;
        int64 GetLoopTime() const;

        Js::FunctionBody* ResolveAssociatedSourceInfo(Js::ScriptContext* ctx) const;
        uint32 GetLine() const;
        uint32 GetColumn() const;

        //return true if this comes strictly before other in execution order
        bool IsBefore(const TTDebuggerSourceLocation& other) const;
    };

    //////////////////

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
            ExternalCbRegisterCall,
            ExternalCallTag,
            //JsRTActionTag is a marker for where the JsRT actions begin
            JsRTActionTag,

#if !INT32VAR
            CreateIntegerActionTag,
#endif
            CreateNumberActionTag,
            CreateBooleanActionTag,
            CreateStringActionTag,
            CreateSymbolActionTag,

            VarConvertToNumberActionTag,
            VarConvertToBooleanActionTag,
            VarConvertToStringActionTag,
            VarConvertToObjectActionTag,

            AddRootRefActionTag,
            RemoveRootRefActionTag,

            AllocateObjectActionTag,
            AllocateExternalObjectActionTag,
            AllocateArrayActionTag,
            AllocateArrayBufferActionTag,
            AllocateExternalArrayBufferActionTag,
            AllocateFunctionActionTag,

            HostExitProcessTag,
            GetAndClearExceptionActionTag,

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
            CallbackOpActionTag,
            CodeParseActionTag,
            CallExistingFunctionActionTag,

            Count
        };

        //Inflate an argument variable for an action during replay and record passing an value to the host
        void PassVarToHostInReplay(Js::ScriptContext* ctx, TTDVar origVar, Js::Var replayVar);
        Js::Var InflateVarInReplay(Js::ScriptContext* ctx, TTDVar var);

        typedef void(*fPtr_EventLogActionEntryInfoExecute)(const EventLogEntry* evt, Js::ScriptContext* ctx);

        typedef void(*fPtr_EventLogEntryInfoUnload)(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        typedef void(*fPtr_EventLogEntryInfoEmit)(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        typedef void(*fPtr_EventLogEntryInfoParse)(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct that we use for our pseudo v-table on the EventLogEntry data
        struct EventLogEntryVTableEntry
        {
            fPtr_EventLogActionEntryInfoExecute ExecuteFP;

            fPtr_EventLogEntryInfoUnload UnloadFP;
            fPtr_EventLogEntryInfoEmit EmitFP;
            fPtr_EventLogEntryInfoParse ParseFP;
        };

        //A base struct for our event log entries -- we will use the kind tags as v-table values 
        struct EventLogEntry
        {
            byte EventData[EVENT_INLINE_DATA_BYTE_COUNT];

            //The kind of the event
            EventKind EventKind;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //The event time for this event
            int64 EventTimeStamp;
#endif
        };

        template <typename T, EventKind tag>
        const T* GetInlineEventDataAs(const EventLogEntry* evt)
        {
            static_assert(sizeof(T) < EVENT_INLINE_DATA_BYTE_COUNT, "Data is too large for inline representation!!!");
            AssertMsg(evt->EventKind == tag, "Bad tag match!");

            return reinterpret_cast<const T*>(evt->EventData);
        }

        template <typename T, EventKind tag>
        T* GetInlineEventDataAs(EventLogEntry* evt)
        {
            static_assert(sizeof(T) < EVENT_INLINE_DATA_BYTE_COUNT, "Data is too large for inline representation!!!");
            AssertMsg(evt->EventKind == tag, "Bad tag match!");

            return reinterpret_cast<T*>(evt->EventData);
        }

        //Helpers for initializing, emitting and parsing the basic event data
        void EventLogEntry_Initialize(EventLogEntry* evt, EventKind tag, int64 etime);
        void EventLogEntry_Emit(const EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator);
        void EventLogEntry_Parse(EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //////////////////

        //A struct that represents snapshot events
        struct SnapshotEventLogEntry
        {
            //The timestamp we should restore to 
            int64 RestoreTimestamp;

            //The snapshot (we many persist this to disk and inflate back in later)
            SnapShot* Snap;
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
            uint64 BodyCounterId;
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


        //A struct containing additional information on the external call
        struct ExternalCallEventLogEntry_AdditionalInfo
        {
            //
            //TODO: later we should record more detail on the script exception for inflation if needed
            //
            bool HasScriptException;
            bool HasTerminiatingException;

            //The last event time that is nested in this external call
            int64 LastNestedEventTime;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //the function name for the function that is invoked
            TTString FunctionName;
#endif
        };

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

            ExternalCallEventLogEntry_AdditionalInfo* AdditionalInfo;
        };

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void ExternalCallEventLogEntry_ProcessDiagInfoPre(EventLogEntry* evt, Js::JavascriptFunction* function, UnlinkableSlabAllocator& alloc);
#endif

        int64 ExternalCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt);

        void ExternalCallEventLogEntry_ProcessArgs(EventLogEntry* evt, int32 rootDepth, Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, UnlinkableSlabAllocator& alloc);
        void ExternalCallEventLogEntry_ProcessReturn(EventLogEntry* evt, Js::Var res, bool hasScriptException, bool hasTerminiatingException, int64 lastNestedEvent);

        void ExternalCallEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void ExternalCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void ExternalCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);
    }
}

#endif
