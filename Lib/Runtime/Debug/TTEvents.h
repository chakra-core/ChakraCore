
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

        //An optional -- and static string message to include
        const LPCWSTR m_staticAbortMessage;

        TTDebuggerAbortException(uint32 abortCode, int64 optEventTime, LPCWSTR staticAbortMessage);

    public:
        ~TTDebuggerAbortException();

        static TTDebuggerAbortException CreateAbortEndOfLog(LPCWSTR staticMessage);
        static TTDebuggerAbortException CreateTopLevelAbortRequest(int64 targetEventTime, LPCWSTR staticMessage);
        static TTDebuggerAbortException CreateUncaughtExceptionAbortRequest(int64 targetEventTime, LPCWSTR staticMessage);

        bool IsEndOfLog() const;
        bool IsEventTimeMove() const;
        bool IsTopLevelException() const;

        int64 GetTargetEventTime() const;

        LPCWSTR GetStaticAbortMessage() const;
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
        TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other);
        ~TTDebuggerSourceLocation();

        bool HasValue() const;
        void Clear();
        void SetLocation(const TTDebuggerSourceLocation& other);
        void SetLocation(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column);

        int64 GetRootEventTime() const;
        int64 GetFunctionTime() const;
        int64 GetLoopTime() const;

        Js::FunctionBody* ResolveAssociatedSourceInfo(Js::ScriptContext* ctx);
        uint32 GetLine() const;
        uint32 GetColumn() const;

        void Emit(FileWriter* writer, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) const;
        static void ParseInto(TTDebuggerSourceLocation& into, FileReader* reader, bool readSeperator = false);
    };

    //////////////////

    //A base class for our event log entries
    class EventLogEntry
    {
    public:
        //An enumeration of the event kinds in the system
        enum class EventKind
        {
            SnapshotTag,
            TelemetryLogEntry,
            DoubleTag,
            StringTag,
            RandomSeedTag,
            PropertyEnumTag,
            SymbolCreationTag,
            ExternalCallBeginTag,
            ExternalCallEndTag,
            JsRTActionTag
        };

    private:
        //The kind of the event
        const EventKind m_eventKind;

        //The time at which this event occoured
        const int64 m_eventTimestamp;

    protected:
        EventLogEntry(EventKind tag, int64 eventTimestamp);

        //A helper for subclasses implementing EmitObject, this writes the initial tags and the standard data
        void BaseStdEmit(FileWriter* writer, NSTokens::Separator separator) const;

    public:
        //unload any memory this event uses (including any unlinkable memory)
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc);

        //Get the event kind tag
        EventKind GetEventKind() const;

        //The timestamp the event occoured
        int64 GetEventTime() const;

        //If this event may have a snapshot associated with it go ahead and unload it so we don't fill up memory with them
        virtual void UnloadSnapshot() const;

        //serialize the event data 
        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const = 0;

        //de-serialize an Event calling the correct completion vased on the EventKind
        //IMPORTANT: Each subclass should implement a static "CompleteParse" method which this will call to complete the parse and create the event
        static EventLogEntry* Parse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        //A debug value that tells us what the global event tag counter should be before this event completes
        TTD_LOG_TAG DiagnosticEventTagValue;
#endif
    };

    //////////////////

    //A class that represents snapshot events
    class SnapshotEventLogEntry : public EventLogEntry
    {
    private:
        //The timestamp we should restore to 
        const int64 m_restoreTimestamp;

        //The tag and identity we should restore to 
        const TTD_LOG_TAG m_restoreLogTag;

        //The snapshot (we many persist this to disk and inflate back in later)
        mutable SnapShot* m_snap;

    public:
        SnapshotEventLogEntry(int64 eTime, SnapShot* snap, int64 restoreTimestamp, TTD_LOG_TAG restoreLogTag);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        virtual void UnloadSnapshot() const override;

        //Get the event as a snapshot event (and do tag checking for consistency)
        static SnapshotEventLogEntry* As(EventLogEntry* e);

        //Get the event time and tag to restore to
        int64 GetRestoreEventTime() const;
        TTD_LOG_TAG GetRestoreLogTag() const;
        void EnsureSnapshotDeserialized(LPCWSTR logContainerUri, ThreadContext* threadContext) const;
        const SnapShot* GetSnapshot() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static SnapshotEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //////////////////

    //A class that represents telemetry events from the user code
    class TelemetryEventLogEntry : public EventLogEntry
    {
    private:
        //A string that contains all of the info that is logged and if we should print it
        const TTString m_infoString;
        const bool m_shouldPrint;

        //An id the user can provide for this event so we can vector clock with whatever is being logged in the code
        const int64 m_optUserEventId;

        //if we should break at this point during replay and the source line info
        const bool m_shouldBreak;
        TTDebuggerSourceLocation m_sourceLocation;

    public:
        TelemetryEventLogEntry(int64 eTime, const TTString& infoString, bool shouldPrint, int64 optUserEventId, bool shouldBreak, const TTDebuggerSourceLocation& sourceLocation);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        //Get the event as a snapshot event (and do tag checking for consistency)
        static TelemetryEventLogEntry* As(EventLogEntry* e);

        //Get the info about the event
        const TTString& GetInfoString() const;
        bool ShouldPrint() const;
        int64 GetOptUserEventId() const;

        //Get info about associated breakpoints
        bool ShouldBreak() const;
        void GetBreakSourceLocation(TTDebuggerSourceLocation& sourceLocation) const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static TelemetryEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //////////////////

    //A class that represents the generation of random seeds
    class RandomSeedEventLogEntry : public EventLogEntry
    {
    private:
        //The values associated with the event
        const uint64 m_seed0;
        const uint64 m_seed1;

    public:
        RandomSeedEventLogEntry(int64 eventTimestamp, uint64 seed0, uint64 seed1);

        static RandomSeedEventLogEntry* As(EventLogEntry* e);

        uint64 GetSeed0() const;
        uint64 GetSeed1() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static RandomSeedEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //A class that represents a simple event that needs a double value (e.g. date values)
    class DoubleEventLogEntry : public EventLogEntry
    {
    private:
        //The value associated with the event
        const double m_doubleValue;

    public:
        DoubleEventLogEntry(int64 eventTimestamp, double val);

        static DoubleEventLogEntry* As(EventLogEntry* e);

        double GetDoubleValue() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static DoubleEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //A class that represents a simple event that needs a string value (e.g. date values)
    class StringValueEventLogEntry : public EventLogEntry
    {
    private:
        //The value associated with the event
        const TTString m_stringValue;

    public:
        StringValueEventLogEntry(int64 eventTimestamp, const TTString& val);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        static StringValueEventLogEntry* As(EventLogEntry* e);

        const TTString& GetStringValue() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static StringValueEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //////////////////

    //A class that represents a single enumeration step for properties on a dynamic object
    class PropertyEnumStepEventLogEntry : public EventLogEntry
    {
    private:
        //The return code, property id, and attributes returned
        const BOOL m_returnCode;
        const Js::PropertyId m_pid;
        const Js::PropertyAttributes m_attributes;

        //Optional property name string (may need to actually use later if pid can be Constants::NoProperty)
        //Always set if if doing extra diagnostics otherwise only as needed
        const TTString m_propertyString;

    public:
        PropertyEnumStepEventLogEntry(int64 eventTimestamp, BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, const TTString& propertyName);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        static PropertyEnumStepEventLogEntry* As(EventLogEntry* e);

        BOOL GetReturnCode() const;
        Js::PropertyId GetPropertyId() const;
        Js::PropertyAttributes GetAttributes() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static PropertyEnumStepEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //////////////////

    //A class that represents the creation of a symbol (which we need to make sure gets the correct property id)
    class SymbolCreationEventLogEntry : public EventLogEntry
    {
    private:
        //The property id of the created symbol
        const Js::PropertyId m_pid;

    public:
        SymbolCreationEventLogEntry(int64 eventTimestamp, Js::PropertyId pid);

        static SymbolCreationEventLogEntry* As(EventLogEntry* e);

        Js::PropertyId GetPropertyId() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static SymbolCreationEventLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //////////////////

    namespace NSLogValue
    {
        enum class ArgRetValueTag
        {
            Invalid = 0x0,

            RawNull,
            ChakraUndefined,
            ChakraNull,
            ChakraBool,
            ChakraInteger,
            ChakraInt64,
            ChakraUInt64,
            ChakraNumber,
            ChakraString,
            ChakraSymbol,
            ChakraLoggedObject
        };

        //A struct that represents a cross Host/Chakra call argument/return value 
        struct ArgRetValue
        {
            ArgRetValueTag Tag;

            union
            {
                BOOL u_boolVal;
                int64 u_int64Val;
                uint64 u_uint64Val;
                double u_doubleVal;
                Js::PropertyId u_propertyId;
                TTD_LOG_TAG u_objectTag;
            };

            TTString* m_optStringContents;
        };

        //Unload any data that is needed from this ArgRetValue
        void UnloadData(const ArgRetValue& val, UnlinkableSlabAllocator& alloc);

        //Initialize as invalid
        void InitializeArgRetValueAsInvalid(ArgRetValue& val);

        //Extract a ArgRetValue 
        void ExtractArgRetValueFromVar(Js::Var var, ArgRetValue& val, UnlinkableSlabAllocator& alloc);

        //Convert the ArgRetValue into the appropriate value
        Js::Var InflateArgRetValueIntoVar(const ArgRetValue& val, Js::ScriptContext* ctx);

        //serialize the SnapPrimitiveValue
        void EmitArgRetValue(const ArgRetValue& val, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the SnapPrimitiveValue
        void ParseArgRetValue(ArgRetValue& val, bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc);
    }

    //////////////////

    //A class for logging calls from Chakra to an external function (e.g., record start of external execution and later any argument information)
    class ExternalCallEventBeginLogEntry : public EventLogEntry
    {
    private:
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        //the function name for the function that is invoked
        TTString m_functionName;
#endif

        //The root nesting depth
        const int32 m_rootNestingDepth;

        //The time at which the external call began
        const double m_callBeginTime;

    public:
        ExternalCallEventBeginLogEntry(int64 eTime, int32 rootNestingDepth, double callBeginTime);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void SetFunctionName(const TTString& fname);
#endif

        //Get the event as a external call event (and do tag checking for consistency)
        static ExternalCallEventBeginLogEntry* As(EventLogEntry* e);

        //Return the root nesting depth
        int32 GetRootNestingDepth() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static ExternalCallEventBeginLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };

    //A class for logging calls from Chakra to an external function (e.g., record the return value)
    class ExternalCallEventEndLogEntry : public EventLogEntry
    {
    private:
        //The corresponding begin action time
        int64 m_matchingBeginTime;

        //The time at which the external call ended
        const double m_callEndTime;

        //
        //TODO: later we should record more detail on the script exception for inflation if needed
        //
        const bool m_hasScriptException;
        const bool m_hasTerminiatingException;

        //The root nesting depth
        const int32 m_rootNestingDepth;

        //the value returned by the external call
        const NSLogValue::ArgRetValue m_returnVal;

    public:
        ExternalCallEventEndLogEntry(int64 eTime, int64 matchingBeginTime, int32 rootNestingDepth, bool hasScriptException, bool hasTerminatingException, double endTime, const NSLogValue::ArgRetValue& returnVal);
        virtual void UnloadEventMemory(UnlinkableSlabAllocator& alloc) override;

        //Get the event as a external call event (and do tag checking for consistency)
        static ExternalCallEventEndLogEntry* As(EventLogEntry* e);

        bool HasTerminatingException() const;
        bool HasScriptException() const;

        //Get the event time for the matching call begin event
        int64 GetMatchingCallBegin() const;

        //Return the root nesting depth
        int32 GetRootNestingDepth() const;

        //Get the return value argument
        const NSLogValue::ArgRetValue& GetReturnValue() const;

        virtual void EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const override;
        static ExternalCallEventEndLogEntry* CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime);
    };
}

#endif

