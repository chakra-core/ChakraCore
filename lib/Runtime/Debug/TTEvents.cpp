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

    //////////////////

    EventLogEntry::EventLogEntry(EventKind tag, int64 eventTimestamp)
        : m_eventKind(tag), m_eventTimestamp(eventTimestamp), m_prev(nullptr), m_next(nullptr)
    {
        ;
    }

    EventLogEntry::~EventLogEntry()
    {
        ;
    }

    void EventLogEntry::BaseStdEmit(FileWriter* writer, NSTokens::Separator separator) const
    {
        writer->WriteRecordStart(separator);

        writer->WriteTag<EventKind>(NSTokens::Key::eventKind, this->m_eventKind);
        writer->WriteInt64(NSTokens::Key::eventTime, this->m_eventTimestamp, NSTokens::Separator::CommaSeparator);
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

    const EventLogEntry* EventLogEntry::GetPreviousEvent() const
    {
        return this->m_prev;
    }

    EventLogEntry* EventLogEntry::GetPreviousEvent()
    {
        return this->m_prev;
    }

    const EventLogEntry* EventLogEntry::GetNextEvent() const
    {
        return this->m_next;
    }

    EventLogEntry* EventLogEntry::GetNextEvent()
    {
        return this->m_next;
    }

    void EventLogEntry::SetPreviousEvent(EventLogEntry* previous)
    {
        this->m_prev = previous;
    }

    void EventLogEntry::SetNextEvent(EventLogEntry* next)
    {
        this->m_next = next;
    }

    EventLogEntry* EventLogEntry::Parse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
    {
        reader->ReadRecordStart(readSeperator);

        EventKind kind = reader->ReadTag<EventKind>(NSTokens::Key::eventKind);
        int64 etime = reader->ReadInt64(NSTokens::Key::eventTime, true);

        EventLogEntry* res = nullptr;
        switch(kind)
        {
        case EventKind::SnapshotTag:
            res = SnapshotEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::UInt64Tag:
            res = UInt64EventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::DoubleTag:
            res = DoubleEventLogEntry::CompleteParse(true, reader, alloc, etime);
            break;
        case EventKind::StringTag:
            res = StringValueEventLogEntry::CompleteParse(true, reader, alloc, etime);
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

        return res;
    }

    void EventLogEntry::EmitEventList(const EventLogEntry* eventList, LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator)
    {
        AssertMsg(eventList == nullptr || eventList->m_next == nullptr, "Not the last event in the list!!!");

        uint32 ecount = 0;
        const EventLogEntry* headEvent = nullptr;
        const EventLogEntry* currec = eventList;
        while(currec != nullptr)
        {
            ecount++;
            headEvent = currec;
            currec = currec->m_prev;
        }
        writer->WriteLengthValue(ecount, separator);

        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);

        bool first = true;
        writer->AdjustIndent(1);
        for(const EventLogEntry* curr = headEvent; curr != nullptr; curr = curr->m_next)
        {
            curr->EmitEvent(logContainerUri, writer, threadContext, !first ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);
            first = false;
        }
        writer->AdjustIndent(-1);
        writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
    }

    EventLogEntry* EventLogEntry::ParseEventList(bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
    {
        EventLogEntry* prev = nullptr;

        uint32 ecount = reader->ReadLengthValue(readSeperator);
        reader->ReadSequenceStart_WDefaultKey(true);

        for(uint32 i = 0; i < ecount; ++i)
        {
            EventLogEntry* curr = EventLogEntry::Parse(i != 0, threadContext, reader, alloc);

            curr->SetPreviousEvent(prev);
            if(prev != nullptr)
            {
                prev->SetNextEvent(curr);
            }

            prev = curr;
        }
        reader->ReadSequenceEnd();

        AssertMsg(prev == nullptr || prev->GetNextEvent() == nullptr, "This should be the last event in the list.");

        return prev;
    }

    //////////////////

    SnapshotEventLogEntry::SnapshotEventLogEntry(int64 eTime, SnapShot* snap, int64 restoreTimestamp, TTD_LOG_TAG restoreLogTag, TTD_IDENTITY_TAG restoreIdentityTag)
        : EventLogEntry(EventLogEntry::EventKind::SnapshotTag, eTime), m_restoreTimestamp(restoreTimestamp), m_restoreLogTag(restoreLogTag), m_restoreIdentityTag(restoreIdentityTag), m_snap(snap)
    {
        ;
    }

    SnapshotEventLogEntry::~SnapshotEventLogEntry()
    {
        if(this->m_snap != nullptr)
        {
            HeapDelete(this->m_snap);
            this->m_snap = nullptr;
        }
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

    TTD_IDENTITY_TAG SnapshotEventLogEntry::GetRestoreIdentityTag() const
    {
        return this->m_restoreIdentityTag;
    }

    void SnapshotEventLogEntry::EnsureSnapshotDeserialized(LPCWSTR logContainerUri, ThreadContext* threadContext) const
    {
        if(this->m_snap == nullptr)
        {
            this->m_snap = SnapShot::Parse(logContainerUri, (DWORD)this->GetEventTime(), threadContext, TTD_WRITE_JSON_OUTPUT, TTD_WRITE_BINARY_OUTPUT);
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
        writer->WriteIdentityTag(NSTokens::Key::restoreIdentityTag, this->m_restoreIdentityTag, NSTokens::Separator::CommaSeparator);

        if(this->m_snap != nullptr)
        {
            this->m_snap->EmitSnapshot(logContainerUri, (DWORD)this->GetEventTime(), threadContext, TTD_WRITE_JSON_OUTPUT, TTD_WRITE_BINARY_OUTPUT);
        }

        writer->WriteRecordEnd();
    }

    SnapshotEventLogEntry* SnapshotEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        int64 restoreTime = reader->ReadInt64(NSTokens::Key::restoreTime, true);
        TTD_LOG_TAG restoreLogTag = reader->ReadLogTag(NSTokens::Key::restoreLogTag, true);
        TTD_IDENTITY_TAG restoreIdentityTag = reader->ReadIdentityTag(NSTokens::Key::restoreIdentityTag, true);

        return alloc.SlabNew<SnapshotEventLogEntry>(eTime, nullptr, restoreTime, restoreLogTag, restoreIdentityTag);
    }

    //////////////////

    UInt64EventLogEntry::UInt64EventLogEntry(int64 eventTimestamp, uint64 val)
        : EventLogEntry(EventLogEntry::EventKind::UInt64Tag, eventTimestamp), m_uint64Value(val)
    {
        ;
    }

    UInt64EventLogEntry::~UInt64EventLogEntry()
    {
        ;
    }

    UInt64EventLogEntry* UInt64EventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::UInt64Tag, "Not a uint64 event!");

        return static_cast<UInt64EventLogEntry*>(e);
    }

    uint64 UInt64EventLogEntry::GetUInt64() const
    {
        return this->m_uint64Value;
    }

    void UInt64EventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteUInt64(NSTokens::Key::u64Val, this->m_uint64Value, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    UInt64EventLogEntry* UInt64EventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        uint64 val = reader->ReadUInt64(NSTokens::Key::u64Val, true);

        return alloc.SlabNew<UInt64EventLogEntry>(eTime, val);
    }

    DoubleEventLogEntry::DoubleEventLogEntry(int64 eventTimestamp, double val)
        : EventLogEntry(EventLogEntry::EventKind::DoubleTag, eventTimestamp), m_doubleValue(val)
    {
        ;
    }

    DoubleEventLogEntry::~DoubleEventLogEntry()
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

    DoubleEventLogEntry* DoubleEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        double val = reader->ReadDouble(NSTokens::Key::doubleVal, true);

        return alloc.SlabNew<DoubleEventLogEntry>(eTime, val);
    }

    StringValueEventLogEntry::StringValueEventLogEntry(int64 eventTimestamp, LPCWSTR val)
        : EventLogEntry(EventLogEntry::EventKind::StringTag, eventTimestamp), m_stringValue(val)
    {
        ;
    }

    StringValueEventLogEntry::~StringValueEventLogEntry()
    {
        ;
    }

    StringValueEventLogEntry* StringValueEventLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::StringTag, "Not a string event!");

        return static_cast<StringValueEventLogEntry*>(e);
    }

    LPCWSTR StringValueEventLogEntry::GetStringValue() const
    {
        return this->m_stringValue;
    }

    void StringValueEventLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        writer->WriteString(NSTokens::Key::stringVal, this->m_stringValue, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    StringValueEventLogEntry* StringValueEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        LPCWSTR val = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::stringVal, true));

        return alloc.SlabNew<StringValueEventLogEntry>(eTime, val);
    }

    //////////////////

    PropertyEnumStepEventLogEntry::PropertyEnumStepEventLogEntry(int64 eventTimestamp, BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attributes, LPCWSTR propertyName)
        : EventLogEntry(EventLogEntry::EventKind::PropertyEnumTag, eventTimestamp), m_returnCode(returnCode), m_pid(pid), m_attributes(attributes), m_propertyString(propertyName)
    {
        ;
    }

    PropertyEnumStepEventLogEntry::~PropertyEnumStepEventLogEntry()
    {
        ;
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

    PropertyEnumStepEventLogEntry* PropertyEnumStepEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        BOOL retCode = reader->ReadBool(NSTokens::Key::boolVal, true);
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);
        Js::PropertyAttributes attr = (Js::PropertyAttributes)reader->ReadUInt32(NSTokens::Key::attributeFlags, true);
        LPCWSTR pname = nullptr;

        if(retCode)
        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            pname = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::stringVal, true));
#else
            if(pid == Js::Constants::NoProperty)
            {
                pname = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::stringVal, true));
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

    SymbolCreationEventLogEntry::~SymbolCreationEventLogEntry()
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

    SymbolCreationEventLogEntry* SymbolCreationEventLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        return alloc.SlabNew<SymbolCreationEventLogEntry>(eTime, pid);
    }

    //////////////////

    namespace NSLogValue
    {
        void ExtractArgRetValueFromVar(Js::Var var, ArgRetValue* val, SlabAllocator& alloc)
        {
            if(var == nullptr)
            {
                val->Tag = ArgRetValueTag::RawNull;
                val->ExtraData = nullptr;
            }
            else
            {
                if(JsSupport::IsVarTaggedInline(var))
                {
                    val->Tag = ArgRetValueTag::ChakraTaggedInteger;
                    val->u_int64Val = Js::TaggedInt::ToInt32(var);
                }
                else
                {
                    val->Tag = ArgRetValueTag::ChakraLoggedObject;

                    Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(var);
                    TTD_LOG_TAG logTag = obj->GetScriptContext()->GetThreadContext()->TTDInfo->LookupTagForObject(obj);
                    AssertMsg(logTag != TTD_INVALID_LOG_TAG, "Object was not logged previously!!!");

                    val->u_objectTag = logTag;
                }
            }
        }

        void ExtractArgRetValueFromPropertyId(Js::PropertyId pid, ArgRetValue* val)
        {
            val->Tag = ArgRetValueTag::ChakraPropertyId;
            val->u_propertyId = pid;

            val->ExtraData = nullptr;
        }

        void ExtractArgRetValueFromUInt(unsigned int uval, ArgRetValue* val)
        {
            val->Tag = ArgRetValueTag::RawUIntValue;
            val->u_uint64Val = uval;

            val->ExtraData = nullptr;
        }

        void ExtractArgRetValueFromBytePtr(byte* buff, unsigned int size, ArgRetValue* val, SlabAllocator& alloc)
        {
            val->Tag = ArgRetValueTag::RawUIntValue;
            val->u_uint64Val = size;

            val->ExtraData = (size != 0) ? alloc.SlabAllocateArray<byte>(size) : nullptr;

            if(size != 0)
            {
                memcpy(val->ExtraData, buff, size);
            }
        }

        Js::Var InflateArgRetValueIntoVar(const ArgRetValue* val, Js::ScriptContext* ctx)
        {
            Js::Var res = nullptr;

            if(val->Tag == ArgRetValueTag::RawNull)
            {
                res = nullptr;
            }
            else
            {
                if(val->Tag == ArgRetValueTag::ChakraTaggedInteger)
                {
                    res = Js::TaggedInt::ToVarUnchecked((int32)val->u_int64Val);
                }
                else
                {
                    res = ctx->GetThreadContext()->TTDInfo->LookupObjectForTag(val->u_objectTag);
                }
            }

            return res;
        }

        Js::PropertyId InflateArgRetValueIntoPropertyId(const ArgRetValue* val)
        {
            return val->u_propertyId;
        }

        unsigned int InflateArgRetValueIntoUInt(const ArgRetValue* val)
        {
            return (unsigned int)val->u_uint64Val;
        }

        void InflateArgRetValueIntoBytePtr(byte* buff, unsigned int* size, const ArgRetValue* val)
        {
            AssertMsg(buff != nullptr, "We want to copy data into this!!!");

            *size = (unsigned int)val->u_uint64Val;
            if(*size != 0)
            {
                memcpy(buff, (byte*)val->ExtraData, *size);
            }
        }

        void EmitArgRetValue(const ArgRetValue* val, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteTag<ArgRetValueTag>(NSTokens::Key::argRetValueType, val->Tag);
            switch(val->Tag)
            {
            case ArgRetValueTag::RawNull:
                writer->WriteNull(NSTokens::Key::nullVal, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraTaggedInteger:
                writer->WriteInt32(NSTokens::Key::i32Val, (int32)val->u_int64Val, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraLoggedObject:
                writer->WriteLogTag(NSTokens::Key::tagVal, val->u_objectTag, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::ChakraPropertyId:
            case ArgRetValueTag::RawUIntValue:
            case ArgRetValueTag::RawEnumValue:
                writer->WriteUInt32(NSTokens::Key::u32Val, (uint32)val->u_uint64Val, NSTokens::Separator::CommaSeparator);
                break;
            case ArgRetValueTag::RawBytePtr:
            {
                writer->WriteLengthValue((uint32)val->u_uint64Val, NSTokens::Separator::CommaSeparator);
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = 0; i < (uint32)val->u_uint64Val; ++i)
                {
                    writer->WriteNakedByte(((byte*)val->ExtraData)[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
                }
                writer->WriteSequenceEnd();
                break;
            }
            default:
                AssertMsg(false, "Missing case??");
                break;
            }

            writer->WriteRecordEnd();
        }

        void ParseArgRetValue(ArgRetValue* val, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            val->Tag = reader->ReadTag<ArgRetValueTag>(NSTokens::Key::argRetValueType);
            val->ExtraData = nullptr;

            switch(val->Tag)
            {
            case ArgRetValueTag::RawNull:
                break;
            case ArgRetValueTag::ChakraTaggedInteger:
                val->u_int64Val = reader->ReadInt32(NSTokens::Key::i32Val, true);
                break;
            case ArgRetValueTag::ChakraLoggedObject:
                val->u_objectTag = reader->ReadLogTag(NSTokens::Key::tagVal, true);
                break;
            case ArgRetValueTag::ChakraPropertyId:
            case ArgRetValueTag::RawUIntValue:
            case ArgRetValueTag::RawEnumValue:
                val->u_uint64Val = reader->ReadUInt32(NSTokens::Key::u32Val, true);
                break;
            case ArgRetValueTag::RawBytePtr:
            {
                val->u_uint64Val = reader->ReadLengthValue(true);
                val->ExtraData = (val->u_uint64Val != 0) ? alloc.SlabAllocateArray<byte>((size_t)val->u_uint64Val) : nullptr;

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < (uint32)val->u_uint64Val; ++i)
                {
                    ((byte*)val->ExtraData)[i] = reader->ReadNakedByte(i != 0);
                }
                reader->ReadSequenceEnd();
                break;
            }
            default:
                AssertMsg(false, "Missing case??");
                break;
            }

            reader->ReadRecordEnd();
        }
    }

    //////////////////

    ExternalCallEventBeginLogEntry::ExternalCallEventBeginLogEntry(int64 eTime, int32 rootNestingDepth, double callBeginTime)
        : EventLogEntry(EventLogEntry::EventKind::ExternalCallBeginTag, eTime), m_rootNestingDepth(rootNestingDepth), m_callBeginTime(callBeginTime), m_elapsedTime(0)
    {
        ;
    }

    ExternalCallEventBeginLogEntry::~ExternalCallEventBeginLogEntry()
    {
        ;
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void ExternalCallEventBeginLogEntry::SetFunctionName(LPCWSTR fname)
    {
        this->m_functionName = fname;
    }
#endif

    void ExternalCallEventBeginLogEntry::SetElapsedTime(double elapsedTime)
    {
        this->m_elapsedTime = elapsedTime;
    }

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
        writer->WriteDouble(NSTokens::Key::elapsedTime, this->m_elapsedTime, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    ExternalCallEventBeginLogEntry* ExternalCallEventBeginLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        LPCWSTR fname = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::name, true));
#endif

        int32 nestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        double beginTime = reader->ReadDouble(NSTokens::Key::beginTime, true);
        double elapsedTime = reader->ReadDouble(NSTokens::Key::elapsedTime, true);

        ExternalCallEventBeginLogEntry* res = alloc.SlabNew<ExternalCallEventBeginLogEntry>(eTime, nestingDepth, beginTime);
        res->SetElapsedTime(elapsedTime);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->SetFunctionName(fname);
#endif

        return res;
    }

    ExternalCallEventEndLogEntry::ExternalCallEventEndLogEntry(int64 eTime, int32 rootNestingDepth, NSLogValue::ArgRetValue* returnVal)
        : EventLogEntry(EventLogEntry::EventKind::ExternalCallEndTag, eTime), m_hasTerminiatingException(false), m_hasScriptException(false), m_rootNestingDepth(rootNestingDepth), m_returnVal(returnVal)
    {
        ;
    }

    ExternalCallEventEndLogEntry::~ExternalCallEventEndLogEntry()
    {
        ;
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void ExternalCallEventEndLogEntry::SetFunctionName(LPCWSTR fname)
    {
        this->m_functionName = fname;
    }
#endif

    ExternalCallEventEndLogEntry* ExternalCallEventEndLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::ExternalCallEndTag, "Not an external call event!");

        return static_cast<ExternalCallEventEndLogEntry*>(e);
    }

    void ExternalCallEventEndLogEntry::SetTerminatingException()
    {
        this->m_hasTerminiatingException = true;
    }

    void ExternalCallEventEndLogEntry::SetScriptException()
    {
        this->m_hasScriptException = true;
    }

    bool ExternalCallEventEndLogEntry::HasTerminatingException() const
    {
        return this->m_hasTerminiatingException;
    }

    bool ExternalCallEventEndLogEntry::HasScriptException() const
    {
        return this->m_hasScriptException;
    }

    int32 ExternalCallEventEndLogEntry::GetRootNestingDepth() const
    {
        return this->m_rootNestingDepth;
    }

    const NSLogValue::ArgRetValue* ExternalCallEventEndLogEntry::GetReturnValue() const
    {
        return this->m_returnVal;
    }

    void ExternalCallEventEndLogEntry::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        writer->WriteString(NSTokens::Key::name, this->m_functionName, NSTokens::Separator::CommaSeparator);
#endif

        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_rootNestingDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_returnVal, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    ExternalCallEventEndLogEntry* ExternalCallEventEndLogEntry::CompleteParse(bool readSeperator, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        LPCWSTR fname = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::name, true));
#endif

        int32 nestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        NSLogValue::ArgRetValue* retVal = alloc.SlabAllocateStruct<NSLogValue::ArgRetValue>();
        reader->ReadKey(NSTokens::Key::argRetVal, true);
        NSLogValue::ParseArgRetValue(retVal, false, reader, alloc);

        ExternalCallEventEndLogEntry* res = alloc.SlabNew<ExternalCallEventEndLogEntry>(eTime, nestingDepth, retVal);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->SetFunctionName(fname);
#endif

        return res;
    }
}

#endif
