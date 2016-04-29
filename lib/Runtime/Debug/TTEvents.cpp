//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    TTDebuggerAbortException::TTDebuggerAbortException(uint32 abortCode, int64 optEventTime, LPCWSTR staticAbortMessage)
        : m_abortCode(abortCode), m_optEventTime(optEventTime), m_staticAbortMessage(staticAbortMessage)
    {
        ;
    }

    TTDebuggerAbortException::~TTDebuggerAbortException()
    {
        ;
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateAbortEndOfLog(LPCWSTR staticMessage)
    {
        return TTDebuggerAbortException(1, -1, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateTopLevelAbortRequest(int64 targetEventTime, LPCWSTR staticMessage)
    {
        return TTDebuggerAbortException(2, targetEventTime, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateUncaughtExceptionAbortRequest(int64 targetEventTime, LPCWSTR staticMessage)
    {
        return TTDebuggerAbortException(3, targetEventTime, staticMessage);;
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

    LPCWSTR TTDebuggerAbortException::GetStaticAbortMessage() const
    {
        return this->m_staticAbortMessage;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation()
        : m_etime(-1), m_ftime(0), m_ltime(0), m_sourceFile(nullptr), m_docid(0), m_functionLine(0), m_functionColumn(0), m_line(0), m_column(0)
    {
        ;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other)
        : m_etime(other.m_etime), m_ftime(other.m_ftime), m_ltime(other.m_ltime), m_sourceFile(nullptr), m_docid(other.m_docid), m_functionLine(other.m_functionLine), m_functionColumn(other.m_functionColumn), m_line(other.m_line), m_column(other.m_column)
    {
        if(other.m_sourceFile != nullptr)
        {
            size_t wcharLength = wcslen(other.m_sourceFile) + 1;
            size_t byteLength = wcharLength * sizeof(wchar);

            this->m_sourceFile = new wchar[wcharLength];
            js_memcpy_s(this->m_sourceFile, byteLength, other.m_sourceFile, byteLength);
        }
    }

    TTDebuggerSourceLocation::~TTDebuggerSourceLocation()
    {
        this->Clear();
    }

    bool TTDebuggerSourceLocation::HasValue() const
    {
        return this->m_etime != -1;
    }

    void TTDebuggerSourceLocation::Clear()
    {
        this->m_etime = -1;
        this->m_ftime = 0;
        this->m_ltime = 0;

        this->m_docid = 0;

        this->m_functionLine = 0;
        this->m_functionColumn = 0;

        this->m_line = 0;
        this->m_column = 0;

        if(this->m_sourceFile != nullptr)
        {
            delete[] this->m_sourceFile;
        }
        this->m_sourceFile = nullptr;
    }

    void TTDebuggerSourceLocation::SetLocation(const TTDebuggerSourceLocation& other)
    {
        this->m_etime = other.m_etime;
        this->m_ftime = other.m_ftime;
        this->m_ltime = other.m_ltime;

        this->m_docid = other.m_docid;

        this->m_functionLine = other.m_functionLine;
        this->m_functionColumn = other.m_functionColumn;

        this->m_line = other.m_line;
        this->m_column = other.m_column;

        if(this->m_sourceFile != nullptr)
        {
            delete[] this->m_sourceFile;
        }
        this->m_sourceFile = nullptr;

        if(other.m_sourceFile != nullptr)
        {
            size_t wcharLength = wcslen(other.m_sourceFile) + 1;
            size_t byteLength = wcharLength * sizeof(wchar);

            this->m_sourceFile = new wchar[wcharLength];
            js_memcpy_s(this->m_sourceFile, byteLength, other.m_sourceFile, byteLength);
        }
    }

    void TTDebuggerSourceLocation::SetLocation(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column)
    {
        this->m_etime = etime;
        this->m_ftime = ftime;
        this->m_ltime = ltime;

        this->m_docid = body->GetUtf8SourceInfo()->GetSourceInfoId();

        this->m_functionLine = body->GetLineNumber();
        this->m_functionColumn = body->GetColumnNumber();

        this->m_line = (uint32)line;
        this->m_column = (uint32)column;

        if(this->m_sourceFile != nullptr)
        {
            delete[] this->m_sourceFile;
        }
        this->m_sourceFile = nullptr;

        LPCWSTR sourceFile = body->GetSourceContextInfo()->url;
        if(sourceFile != nullptr)
        {
            size_t wcharLength = wcslen(sourceFile) + 1;
            size_t byteLength = wcharLength * sizeof(wchar);

            this->m_sourceFile = new wchar[wcharLength];
            js_memcpy_s(this->m_sourceFile, byteLength, sourceFile, byteLength);
        }
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

    Js::FunctionBody* TTDebuggerSourceLocation::ResolveAssociatedSourceInfo(Js::ScriptContext* ctx)
    {
        Js::FunctionBody* resBody = ctx->FindFunctionBodyByFileName_TTD(this->m_sourceFile);

        while(true)
        {
            for(uint32 i = 0; i < resBody->GetNestedCount(); ++i)
            {
                Js::ParseableFunctionInfo* ipfi = resBody->GetNestedFunc(i)->EnsureDeserialized();
                Js::FunctionBody* ifb = JsSupport::ForceAndGetFunctionBody(ipfi);

                if(this->m_functionLine == ifb->GetLineNumber() && this->m_functionColumn == ifb->GetColumnNumber())
                {
                    return ifb;
                }

                //if it starts on a larger line or if same line but larger column then we don't contain the target
                AssertMsg(ifb->GetLineNumber() < this->m_functionLine || (ifb->GetLineNumber() == this->m_functionLine && ifb->GetColumnNumber() < this->m_functionColumn), "We went to far but didn't find our function??");

                uint32 endLine = UINT32_MAX;
                uint32 endColumn = UINT32_MAX;
                if(i + 1 < resBody->GetNestedCount())
                {
                    Js::ParseableFunctionInfo* ipfinext = resBody->GetNestedFunc(i + 1)->EnsureDeserialized();
                    Js::FunctionBody* ifbnext = JsSupport::ForceAndGetFunctionBody(ipfinext);

                    endLine = ifbnext->GetLineNumber();
                    endColumn = ifbnext->GetColumnNumber();
                }

                if(endLine > this->m_functionLine || (endLine == this->m_functionLine && endColumn > this->m_functionColumn))
                {
                    resBody = ifb;
                    break;
                }
            }
        }

        AssertMsg(false, "We should never get here!!!");
        return nullptr;
    }

    uint32 TTDebuggerSourceLocation::GetLine() const
    {
        return this->m_line;
    }

    uint32 TTDebuggerSourceLocation::GetColumn() const
    {
        return this->m_column;
    }

    //////////////////

    EventLogEntry::EventLogEntry(EventKind tag, int64 eventTimestamp)
        : m_eventKind(tag), m_eventTimestamp(eventTimestamp)
    {
        ;
    }

    void EventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        ;
    }

    void EventLogEntry::BaseStdEmit(FileWriter* writer, NSTokens::Separator separator) const
    {
        writer->WriteRecordStart(separator);

        writer->WriteTag<EventKind>(NSTokens::Key::eventKind, this->m_eventKind);
        writer->WriteInt64(NSTokens::Key::eventTime, this->m_eventTimestamp, NSTokens::Separator::CommaSeparator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        writer->WriteLogTag(NSTokens::Key::logTag, this->DiagnosticEventTagValue, NSTokens::Separator::CommaSeparator);
#endif
    }

    EventLogEntry::EventKind EventLogEntry::GetEventKind() const
    {
        return this->m_eventKind;
    }

    int64 EventLogEntry::GetEventTime() const
    {
        return this->m_eventTimestamp;
    }

    void EventLogEntry::UnloadSnapshot() const
    {
        ;
    }

    EventLogEntry* EventLogEntry::Parse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
    {
        reader->ReadRecordStart(readSeperator);

        EventKind kind = reader->ReadTag<EventKind>(NSTokens::Key::eventKind);
        int64 etime = reader->ReadInt64(NSTokens::Key::eventTime, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTD_LOG_TAG diagnosticTag = reader->ReadLogTag(NSTokens::Key::logTag, true);
#endif

        EventLogEntry* res = nullptr;
        switch(kind)
        {
        case EventKind::SnapshotTag:
            res = SnapshotEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::TopLevelCodeTag:
            res = CodeLoadEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::TelemetryLogTag:
            res = TelemetryEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::DoubleTag:
            res = DoubleEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::StringTag:
            res = StringValueEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::RandomSeedTag:
            res = RandomSeedEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::PropertyEnumTag:
            res = PropertyEnumStepEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::SymbolCreationTag:
            res = SymbolCreationEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::ExternalCallBeginTag:
            res = ExternalCallEventBeginLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::ExternalCallEndTag:
            res = ExternalCallEventEndLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::JsRTActionTag:
            res = JsRTActionLogEntry::CompleteParse(true, threadContext, reader, alloc, etime);
            break;
        default:
            AssertMsg(false, "Missing tag in case");
        }

        reader->ReadRecordEnd();

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->DiagnosticEventTagValue = diagnosticTag;
#endif

        return res;
    }

    //////////////////

    SnapshotEventLogEntry::SnapshotEventLogEntry(int64 eTime, SnapShot* snap, int64 restoreTimestamp, TTD_LOG_TAG restoreLogTag)
        : EventLogEntry(EventLogEntry::EventKind::SnapshotTag, eTime), m_restoreTimestamp(restoreTimestamp), m_restoreLogTag(restoreLogTag), m_snap(snap)
    {
        ;
    }

    void SnapshotEventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        this->UnloadSnapshot();
    }

    void SnapshotEventLogEntry::UnloadSnapshot() const
    {
        if(this->m_snap != nullptr)
        {
            HeapDelete(this->m_snap);
            this->m_snap = nullptr;
        }
    }

    SnapshotEventLogEntry* SnapshotEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::SnapshotTag, "Not a snapshot event!");

        return static_cast<SnapshotEventLogEntry*>(e);
    }

    int64 SnapshotEventLogEntry::GetRestoreEventTime() const
    {
        return this->m_restoreTimestamp;
    }

    TTD_LOG_TAG SnapshotEventLogEntry::GetRestoreLogTag() const
    {
        return this->m_restoreLogTag;
    }

    void SnapshotEventLogEntry::EnsureSnapshotDeserialized(LPCWSTR logContainerUri, ThreadContext* threadContext) const
    {
        if(this->m_snap == nullptr)
        {
            this->m_snap = SnapShot::Parse(logContainerUri, (DWORD)this->GetEventTime(), threadContext);
        }
    }

    const SnapShot* SnapshotEventLogEntry::GetSnapshot() const
    {
        return this->m_snap;
    }

    void SnapshotEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

        writer->WriteInt64(NSTokens::Key::restoreTime, this->m_restoreTimestamp, NSTokens::Separator::CommaSeparator);
        writer->WriteLogTag(NSTokens::Key::restoreLogTag, this->m_restoreLogTag, NSTokens::Separator::CommaSeparator);

        if(this->m_snap != nullptr)
        {
            this->m_snap->EmitSnapshot(logContainerUri, (DWORD)this->GetEventTime(), threadContext);
        }

        writer->WriteRecordEnd();
    }

    SnapshotEventLogEntry* SnapshotEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        int64 restoreTime = reader->ReadInt64(NSTokens::Key::restoreTime, true);
        TTD_LOG_TAG restoreLogTag = reader->ReadLogTag(NSTokens::Key::restoreLogTag, true);

        return alloc.SlabNew<SnapshotEventLogEntry>(eTime, nullptr, restoreTime, restoreLogTag);
    }

    //////////////////

    CodeLoadEventLogEntry::CodeLoadEventLogEntry(int64 eTime, uint64 bodyCtrId)
        : EventLogEntry(EventKind::TopLevelCodeTag, eTime), m_bodyCounterId(bodyCtrId)
    {
        ;
    }

    void CodeLoadEventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        ;
    }

    CodeLoadEventLogEntry* CodeLoadEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::TopLevelCodeTag, "Not a telemetry event!");

        return static_cast<CodeLoadEventLogEntry*>(e);
    }

    uint64 CodeLoadEventLogEntry::GetLoadCounterId() const
    {
        return this->m_bodyCounterId;
    }

    void CodeLoadEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

        writer->WriteUInt64(NSTokens::Key::u64Val, this->m_bodyCounterId, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    CodeLoadEventLogEntry* CodeLoadEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        uint64 bodyCtrId = reader->ReadUInt64(NSTokens::Key::u64Val, true);

        return alloc.SlabNew<CodeLoadEventLogEntry>(eTime, bodyCtrId);
    }

    TelemetryEventLogEntry::TelemetryEventLogEntry(int64 eTime, const TTString& infoString, bool doPrint)
        : EventLogEntry(EventKind::TelemetryLogTag, eTime), m_infoString(infoString), m_doPrint(doPrint)
    {
        ;
    }

    void TelemetryEventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        alloc.UnlinkString(this->m_infoString);
    }

    TelemetryEventLogEntry* TelemetryEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::TelemetryLogTag, "Not a telemetry event!");

        return static_cast<TelemetryEventLogEntry*>(e);
    }

    const TTString& TelemetryEventLogEntry::GetInfoString() const
    {
        return this->m_infoString;
    }

    bool TelemetryEventLogEntry::GetDoPrint() const
    {
        return this->m_doPrint;
    }

    void TelemetryEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

        writer->WriteString(NSTokens::Key::stringVal, this->m_infoString, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_doPrint, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    TelemetryEventLogEntry* TelemetryEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        TTString infoString;

        reader->ReadString(NSTokens::Key::stringVal, alloc, infoString, true);
        bool doPrint = reader->ReadBool(NSTokens::Key::boolVal, true);

        return alloc.SlabNew<TelemetryEventLogEntry>(eTime, infoString, doPrint);
    }

    //////////////////

    RandomSeedEventLogEntry::RandomSeedEventLogEntry(int64 eventTimestamp, uint64 seed0, uint64 seed1)
        : EventLogEntry(EventLogEntry::EventKind::RandomSeedTag, eventTimestamp), m_seed0(seed0), m_seed1(seed1)
    {
        ;
    }

    RandomSeedEventLogEntry* RandomSeedEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::RandomSeedTag, "Not a uint64 event!");

        return static_cast<RandomSeedEventLogEntry*>(e);
    }

    uint64 RandomSeedEventLogEntry::GetSeed0() const
    {
        return this->m_seed0;
    }

    uint64 RandomSeedEventLogEntry::GetSeed1() const
    {
        return this->m_seed1;
    }

    void RandomSeedEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteUInt64(NSTokens::Key::u64Val, this->m_seed0, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt64(NSTokens::Key::u64Val, this->m_seed1, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    RandomSeedEventLogEntry* RandomSeedEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        uint64 seed0 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
        uint64 seed1 = reader->ReadUInt64(NSTokens::Key::u64Val, true);

        return alloc.SlabNew<RandomSeedEventLogEntry>(eTime, seed0, seed1);
    }

    DoubleEventLogEntry::DoubleEventLogEntry(int64 eventTimestamp, double val)
        : EventLogEntry(EventLogEntry::EventKind::DoubleTag, eventTimestamp), m_doubleValue(val)
    {
        ;
    }

    DoubleEventLogEntry* DoubleEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::DoubleTag, "Not a double event!");

        return static_cast<DoubleEventLogEntry*>(e);
    }

    double DoubleEventLogEntry::GetDoubleValue() const
    {
        return this->m_doubleValue;
    }

    void DoubleEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteDouble(NSTokens::Key::doubleVal, this->m_doubleValue, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    DoubleEventLogEntry* DoubleEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        double val = reader->ReadDouble(NSTokens::Key::doubleVal, true);

        return alloc.SlabNew<DoubleEventLogEntry>(eTime, val);
    }

    StringValueEventLogEntry::StringValueEventLogEntry(int64 eventTimestamp, const TTString& val)
        : EventLogEntry(EventLogEntry::EventKind::StringTag, eventTimestamp), m_stringValue(val)
    {
        ;
    }

    void StringValueEventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        alloc.UnlinkString(this->m_stringValue);
    }

    StringValueEventLogEntry* StringValueEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::StringTag, "Not a string event!");

        return static_cast<StringValueEventLogEntry*>(e);
    }

    const TTString& StringValueEventLogEntry::GetStringValue() const
    {
        return this->m_stringValue;
    }

    void StringValueEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteString(NSTokens::Key::stringVal, this->m_stringValue, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    StringValueEventLogEntry* StringValueEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        TTString val;
        reader->ReadString(NSTokens::Key::stringVal, alloc, val, true);

        return alloc.SlabNew<StringValueEventLogEntry>(eTime, val);
    }

    //////////////////

    PropertyEnumStepEventLogEntry::PropertyEnumStepEventLogEntry(int64 eventTimestamp, BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, const TTString& propertyName)
        : EventLogEntry(EventLogEntry::EventKind::PropertyEnumTag, eventTimestamp), m_returnCode(returnCode), m_pid(pid), m_attributes(attributes), m_propertyString(propertyName)
    {
        ;
    }

    void PropertyEnumStepEventLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        if(!IsNullPtrTTString(this->m_propertyString))
        {
            alloc.UnlinkString(this->m_propertyString);
        }
    }

    PropertyEnumStepEventLogEntry* PropertyEnumStepEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::PropertyEnumTag, "Not a property enum event!");

        return static_cast<PropertyEnumStepEventLogEntry*>(e);
    }

    BOOL PropertyEnumStepEventLogEntry::GetReturnCode() const
    {
        return this->m_returnCode;
    }

    Js::PropertyId PropertyEnumStepEventLogEntry::GetPropertyId() const
    {
        return this->m_pid;
    }

    Js::PropertyAttributes PropertyEnumStepEventLogEntry::GetAttributes() const
    {
        return this->m_attributes;
    }

    void PropertyEnumStepEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_returnCode ? true : false, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_pid, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt32(NSTokens::Key::attributeFlags, this->m_attributes, NSTokens::Separator::CommaSeparator);

        if(this->m_returnCode)
        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            writer->WriteString(NSTokens::Key::stringVal, this->m_propertyString, NSTokens::Separator::CommaSeparator);
#else
            if(this->m_pid == Js::Constants::NoProperty)
            {
                writer->WriteString(NSTokens::Key::stringVal, this->m_propertyString, NSTokens::Separator::CommaSeparator);
            }
#endif
        }
        writer->WriteRecordEnd();
    }

    PropertyEnumStepEventLogEntry* PropertyEnumStepEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        BOOL retCode = reader->ReadBool(NSTokens::Key::boolVal, true);
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);
        Js::PropertyAttributes attr = (Js::PropertyAttributes)reader->ReadUInt32(NSTokens::Key::attributeFlags, true);

        TTString pname;
        InitializeAsNullPtrTTString(pname);

        if(retCode)
        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            reader->ReadString(NSTokens::Key::stringVal, alloc, pname, true);
#else
            if(pid == Js::Constants::NoProperty)
            {
                reader->ReadString(NSTokens::Key::stringVal, alloc, pname, true);
            }
#endif
        }

        return alloc.SlabNew<PropertyEnumStepEventLogEntry>(eTime, retCode, pid, attr, pname);
    }

    //////////////////

    SymbolCreationEventLogEntry::SymbolCreationEventLogEntry(int64 eventTimestamp, Js::PropertyId pid)
        : EventLogEntry(EventLogEntry::EventKind::SymbolCreationTag, eventTimestamp), m_pid(pid)
    {
        ;
    }

    SymbolCreationEventLogEntry* SymbolCreationEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::SymbolCreationTag, "Not a property enum event!");

        return static_cast<SymbolCreationEventLogEntry*>(e);
    }

    Js::PropertyId SymbolCreationEventLogEntry::GetPropertyId() const
    {
        return this->m_pid;
    }

    void SymbolCreationEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_pid, NSTokens::Separator::CommaSeparator);
        
        writer->WriteRecordEnd();
    }

    SymbolCreationEventLogEntry* SymbolCreationEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        return alloc.SlabNew<SymbolCreationEventLogEntry>(eTime, pid);
    }

    //////////////////

    namespace NSLogValue
    {
        void UnloadData(const ArgRetValue& val, UnlinkableSlabAllocator& alloc)
        {
            if(val.Tag == ArgRetValueTag::ChakraString)
            {
                //Unlink the string contents and the string object
                alloc.UnlinkString(*(val.m_optStringContents));
                alloc.UnlinkAllocation(val.m_optStringContents);
            }
        }

        void InitializeArgRetValueAsInvalid(ArgRetValue& val)
        {
            val.Tag = ArgRetValueTag::Invalid;
        }

        void ExtractArgRetValueFromVar(Js::Var var, ArgRetValue& val, UnlinkableSlabAllocator& alloc)
        {
            val.u_uint64Val = 0;
            val.m_optStringContents = nullptr;

            if(var == nullptr)
            {
                val.Tag = ArgRetValueTag::RawNull;
            }
            else
            {
                Js::TypeId tid = Js::JavascriptOperators::GetTypeId(var);
                switch(tid)
                {
                case Js::TypeIds_Undefined:
                    val.Tag = ArgRetValueTag::ChakraUndefined;
                    break;
                case Js::TypeIds_Null:
                    val.Tag = ArgRetValueTag::ChakraNull;
                    break;
                case Js::TypeIds_Boolean:
                    val.Tag = ArgRetValueTag::ChakraBool;
                    val.u_boolVal = Js::JavascriptBoolean::FromVar(var)->GetValue();
                    break;
                case Js::TypeIds_Integer:
                    val.Tag = ArgRetValueTag::ChakraInteger;
                    val.u_int64Val = Js::TaggedInt::ToInt64(var);
                    break;
                case Js::TypeIds_Number:
                    val.Tag = ArgRetValueTag::ChakraNumber;
                    val.u_doubleVal = Js::JavascriptNumber::GetValue(var);
                    break;
                case Js::TypeIds_Int64Number:
                    val.Tag = ArgRetValueTag::ChakraInt64;
                    val.u_int64Val = Js::JavascriptInt64Number::FromVar(var)->GetValue();
                    break;
                case Js::TypeIds_UInt64Number:
                    val.Tag = ArgRetValueTag::ChakraUInt64;
                    val.u_uint64Val = Js::JavascriptUInt64Number::FromVar(var)->GetValue();
                    break;
                case Js::TypeIds_String:
                {
                    val.Tag = ArgRetValueTag::ChakraString;
                    val.m_optStringContents = alloc.SlabAllocateStruct<TTString>();
                    alloc.CopyStringIntoWLength(Js::JavascriptString::FromVar(var)->GetSz(), Js::JavascriptString::FromVar(var)->GetLength(), *(val.m_optStringContents));
                    break;
                }
                case Js::TypeIds_Symbol:
                {
                    val.Tag = ArgRetValueTag::ChakraSymbol;
                    val.u_propertyId = Js::RecyclableObject::FromVar(var)->GetScriptContext()->GetLibrary()->ExtractPrimitveSymbolId_TTD(var);
                    break;
                }
                default:
                {
                    val.Tag = ArgRetValueTag::ChakraLoggedObject;

                    Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(var);
                    TTD_LOG_TAG logTag = obj->GetScriptContext()->GetThreadContext()->TTDInfo->LookupTagForObject(obj);
                    AssertMsg(logTag != TTD_INVALID_LOG_TAG, "Object was not logged previously!!!");

                    val.u_objectTag = logTag;
                    break;
                }
                }
            }
        }

        Js::Var InflateArgRetValueIntoVar(const ArgRetValue& val, Js::ScriptContext* ctx)
        {
            Js::Var res = nullptr;

            switch(val.Tag)
            {
            case ArgRetValueTag::RawNull:
                res = nullptr;
                break;
            case ArgRetValueTag::ChakraUndefined:
                res = ctx->GetLibrary()->GetUndefined();
                break;
            case ArgRetValueTag::ChakraNull:
                res = ctx->GetLibrary()->GetNull();
                break;
            case ArgRetValueTag::ChakraBool:
                res = (val.u_boolVal ? ctx->GetLibrary()->GetTrue() : ctx->GetLibrary()->GetFalse());
                break;
            case ArgRetValueTag::ChakraInteger:
                res = Js::TaggedInt::ToVarUnchecked((int)val.u_int64Val);
                break;
            case ArgRetValueTag::ChakraInt64:
                res = Js::JavascriptInt64Number::ToVar(val.u_int64Val, ctx);
                break;
            case ArgRetValueTag::ChakraUInt64:
                res = Js::JavascriptUInt64Number::ToVar(val.u_uint64Val, ctx);
                break;
            case ArgRetValueTag::ChakraNumber:
                res = Js::JavascriptNumber::ToVarWithCheck(val.u_doubleVal, ctx);
                break;
            case ArgRetValueTag::ChakraString:
                res = Js::JavascriptString::NewCopyBuffer(val.m_optStringContents->Contents, val.m_optStringContents->Length, ctx);
                break;
            case ArgRetValueTag::ChakraSymbol:
                res = ctx->GetLibrary()->CreatePrimitveSymbol_TTD(val.u_propertyId);
                break;
            case ArgRetValueTag::ChakraLoggedObject:
                res = ctx->GetThreadContext()->TTDInfo->LookupObjectForTag(val.u_objectTag);
                break;
            default:
                AssertMsg(false, "Missing case??");
                break;
            }

            return res;
        }

        void EmitArgRetValue(const ArgRetValue& val, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteTag<ArgRetValueTag>(NSTokens::Key::argRetValueType, val.Tag);
            switch(val.Tag)
            {
            case ArgRetValueTag::RawNull:
            case ArgRetValueTag::ChakraUndefined:
            case ArgRetValueTag::ChakraNull:
                break;
            case ArgRetValueTag::ChakraBool:
                writer->WriteInt32(NSTokens::Key::boolVal, val.u_boolVal, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraInteger:
                writer->WriteInt64(NSTokens::Key::i64Val, val.u_int64Val, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraInt64:
                writer->WriteInt64(NSTokens::Key::i64Val, val.u_int64Val, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraUInt64:
                writer->WriteInt64(NSTokens::Key::u64Val, val.u_uint64Val, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraNumber:
                writer->WriteDouble(NSTokens::Key::doubleVal, val.u_doubleVal, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraString:
                writer->WriteString(NSTokens::Key::stringVal, *(val.m_optStringContents), NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraSymbol:
                writer->WriteInt32(NSTokens::Key::propertyId, val.u_propertyId, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraLoggedObject:
                writer->WriteLogTag(NSTokens::Key::tagVal, val.u_objectTag, NSTokens::Separator::CommaSeparator);
                break;
            default:
                AssertMsg(false, "Missing case??");
                break;
            }

            writer->WriteRecordEnd();
        }

        void ParseArgRetValue(ArgRetValue& val, bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            val.u_uint64Val = 0;
            val.m_optStringContents = nullptr;

            val.Tag = reader->ReadTag<ArgRetValueTag>(NSTokens::Key::argRetValueType);
            switch(val.Tag)
            {
            case ArgRetValueTag::RawNull:
            case ArgRetValueTag::ChakraUndefined:
            case ArgRetValueTag::ChakraNull:
                break;
            case ArgRetValueTag::ChakraBool:
                val.u_int64Val = reader->ReadInt32(NSTokens::Key::boolVal, true);
                break;
            case ArgRetValueTag::ChakraInteger:
                val.u_int64Val = reader->ReadInt64(NSTokens::Key::i64Val, true);
                break;
            case ArgRetValueTag::ChakraInt64:
                val.u_int64Val = reader->ReadInt64(NSTokens::Key::i64Val, true);
                break;
            case ArgRetValueTag::ChakraUInt64:
                val.u_uint64Val = reader->ReadInt64(NSTokens::Key::u64Val, true);
                break;
            case ArgRetValueTag::ChakraNumber:
                val.u_doubleVal = reader->ReadDouble(NSTokens::Key::doubleVal, true);
                break;
            case ArgRetValueTag::ChakraString:
                val.m_optStringContents = alloc.SlabAllocateStruct<TTString>();
                reader->ReadString(NSTokens::Key::stringVal, alloc, *(val.m_optStringContents), true);
                break;
            case ArgRetValueTag::ChakraSymbol:
                val.u_propertyId = (Js::PropertyId)reader->ReadInt32(NSTokens::Key::propertyId, true);
                break;
            case ArgRetValueTag::ChakraLoggedObject:
                val.u_objectTag = reader->ReadLogTag(NSTokens::Key::tagVal, true);
                break;
            default:
                AssertMsg(false, "Missing case??");
                break;
            }

            reader->ReadRecordEnd();
        }
    }

    //////////////////

    ExternalCallEventBeginLogEntry::ExternalCallEventBeginLogEntry(int64 eTime, int32 rootNestingDepth, double callBeginTime)
        : EventLogEntry(EventLogEntry::EventKind::ExternalCallBeginTag, eTime), m_rootNestingDepth(rootNestingDepth), m_callBeginTime(callBeginTime)
    {
        ;
    }

    void ExternalCallEventBeginLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        alloc.UnlinkString(this->m_functionName);
#endif
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void ExternalCallEventBeginLogEntry::SetFunctionName(const TTString& fname)
    {
        this->m_functionName = fname;
    }
#endif

    ExternalCallEventBeginLogEntry* ExternalCallEventBeginLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::ExternalCallBeginTag, "Not an external call event!");

        return static_cast<ExternalCallEventBeginLogEntry*>(e);
    }

    int32 ExternalCallEventBeginLogEntry::GetRootNestingDepth() const
    {
        return this->m_rootNestingDepth;
    }

    void ExternalCallEventBeginLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        writer->WriteString(NSTokens::Key::name, this->m_functionName, NSTokens::Separator::CommaSeparator);
#endif

        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_rootNestingDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::beginTime, this->m_callBeginTime, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    ExternalCallEventBeginLogEntry* ExternalCallEventBeginLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString fname;
        reader->ReadString(NSTokens::Key::name, alloc, fname, true);
#endif

        int32 nestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        double beginTime = reader->ReadDouble(NSTokens::Key::beginTime, true);

        ExternalCallEventBeginLogEntry* res = alloc.SlabNew<ExternalCallEventBeginLogEntry>(eTime, nestingDepth, beginTime);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->SetFunctionName(fname);
#endif

        return res;
    }

    ExternalCallEventEndLogEntry::ExternalCallEventEndLogEntry(int64 eTime, int64 matchingBeginTime, int32 rootNestingDepth, bool hasScriptException, bool hasTerminatingException, double endTime, const NSLogValue::ArgRetValue& returnVal)
        : EventLogEntry(EventLogEntry::EventKind::ExternalCallEndTag, eTime), m_matchingBeginTime(matchingBeginTime), m_rootNestingDepth(rootNestingDepth), m_hasTerminiatingException(false), m_hasScriptException(false), m_callEndTime(endTime), m_returnVal(returnVal)
    {
        ;
    }

    void ExternalCallEventEndLogEntry::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        NSLogValue::UnloadData(this->m_returnVal, alloc);
    }

    ExternalCallEventEndLogEntry* ExternalCallEventEndLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::ExternalCallEndTag, "Not an external call event!");

        return static_cast<ExternalCallEventEndLogEntry*>(e);
    }

    bool ExternalCallEventEndLogEntry::HasTerminatingException() const
    {
        return this->m_hasTerminiatingException;
    }

    bool ExternalCallEventEndLogEntry::HasScriptException() const
    {
        return this->m_hasScriptException;
    }

    int64 ExternalCallEventEndLogEntry::GetMatchingCallBegin() const
    {
        return this->m_matchingBeginTime;
    }

    int32 ExternalCallEventEndLogEntry::GetRootNestingDepth() const
    {
        return this->m_rootNestingDepth;
    }

    const NSLogValue::ArgRetValue& ExternalCallEventEndLogEntry::GetReturnValue() const
    {
        return this->m_returnVal;
    }

    void ExternalCallEventEndLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

        writer->WriteInt64(NSTokens::Key::matchingCallBegin, this->m_matchingBeginTime, NSTokens::Separator::CommaSeparator);
        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_rootNestingDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_hasScriptException, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_hasTerminiatingException, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_returnVal, writer, NSTokens::Separator::NoSeparator);

        writer->WriteDouble(NSTokens::Key::endTime, this->m_callEndTime, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    ExternalCallEventEndLogEntry* ExternalCallEventEndLogEntry::CompleteParse(bool readSeperator, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        int64 matchingBegin = reader->ReadInt64(NSTokens::Key::matchingCallBegin, true);
        int32 nestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        bool hasScriptException = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool hasTerminatingException = reader->ReadBool(NSTokens::Key::boolVal, true);

        NSLogValue::ArgRetValue retVal;
        reader->ReadKey(NSTokens::Key::argRetVal, true);
        NSLogValue::ParseArgRetValue(retVal, false, reader, alloc);

        double endTime = reader->ReadDouble(NSTokens::Key::endTime, true);

        return alloc.SlabNew<ExternalCallEventEndLogEntry>(eTime, matchingBegin, nestingDepth, hasScriptException, hasTerminatingException, endTime, retVal);
    }
}

#endif
