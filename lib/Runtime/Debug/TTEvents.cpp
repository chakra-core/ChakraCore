//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    TTDebuggerAbortException::TTDebuggerAbortException(uint32 abortCode, int64 optEventTime, int64 optMoveMode, const char16* staticAbortMessage)
        : m_abortCode(abortCode), m_optEventTime(optEventTime), m_optMoveMode(optMoveMode), m_staticAbortMessage(staticAbortMessage)
    {
        ;
    }

    TTDebuggerAbortException::~TTDebuggerAbortException()
    {
        ;
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateAbortEndOfLog(const char16* staticMessage)
    {
        return TTDebuggerAbortException(1, -1, 0, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateTopLevelAbortRequest(int64 targetEventTime, int64 moveMode, const char16* staticMessage)
    {
        return TTDebuggerAbortException(2, targetEventTime, moveMode, staticMessage);
    }

    TTDebuggerAbortException TTDebuggerAbortException::CreateUncaughtExceptionAbortRequest(int64 targetEventTime, const char16* staticMessage)
    {
        return TTDebuggerAbortException(3, targetEventTime, 0, staticMessage);
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

    int64 TTDebuggerAbortException::GetMoveMode() const
    {
        return this->m_optMoveMode;
    }

    const char16* TTDebuggerAbortException::GetStaticAbortMessage() const
    {
        return this->m_staticAbortMessage;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation()
        : m_etime(-1), m_ftime(0), m_ltime(0), m_sourceFile(nullptr), m_docid(0), m_functionLine(0), m_functionColumn(0), m_line(0), m_column(0)
    {
        ;
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation(const SingleCallCounter& callFrame)
        : m_etime(-1), m_ftime(0), m_ltime(0), m_sourceFile(nullptr), m_docid(0), m_functionLine(0), m_functionColumn(0), m_line(0), m_column(0)
    {
        this->SetLocation(callFrame);
    }

    TTDebuggerSourceLocation::TTDebuggerSourceLocation(const TTDebuggerSourceLocation& other)
        : m_etime(other.m_etime), m_ftime(other.m_ftime), m_ltime(other.m_ltime), m_sourceFile(nullptr), m_docid(other.m_docid), m_functionLine(other.m_functionLine), m_functionColumn(other.m_functionColumn), m_line(other.m_line), m_column(other.m_column)
    {
        if(other.m_sourceFile != nullptr)
        {
            size_t char16Length = wcslen(other.m_sourceFile) + 1;
            size_t byteLength = char16Length * sizeof(char16);

            this->m_sourceFile = new char16[char16Length];
            js_memcpy_s(this->m_sourceFile, byteLength, other.m_sourceFile, byteLength);
        }
    }

    TTDebuggerSourceLocation::~TTDebuggerSourceLocation()
    {
        this->Clear();
    }

    TTDebuggerSourceLocation& TTDebuggerSourceLocation::operator= (const TTDebuggerSourceLocation& other)
    {
        if(this != &other)
        {
            this->SetLocation(other);
        }

        return *this;
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void TTDebuggerSourceLocation::PrintToConsole(bool newline) const
    {
        if(!this->HasValue())
        {
            wprintf(_u("undef"));
        }
        else
        {
            wprintf(_u("%ls l:%I32u c:%I32u (%I64i, %I64i, %I64i)"), this->m_sourceFile, this->m_line, this->m_column, this->m_etime, this->m_ftime, this->m_ltime);
        }

        if(newline)
        {
            wprintf(_u("\n"));
        }
    }
#endif

    void TTDebuggerSourceLocation::Initialize()
    {
        this->m_etime = -1;
        this->m_ftime = 0;
        this->m_ltime = 0;

        this->m_sourceFile = nullptr;
        this->m_docid = 0;

        this->m_functionLine = 0;
        this->m_functionColumn = 0;
        this->m_line = 0;
        this->m_column = 0;
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
#if !ENABLE_TTD_DEBUGGING
        AssertMsg(false, "Debugger is not enabled so you shouldn't be calling this");
        this->Clear();
#else
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
            size_t char16Length = wcslen(other.m_sourceFile) + 1;
            size_t byteLength = char16Length * sizeof(char16);

            this->m_sourceFile = new char16[char16Length];
            js_memcpy_s(this->m_sourceFile, byteLength, other.m_sourceFile, byteLength);
        }
#endif
    }

    void TTDebuggerSourceLocation::SetLocation(const SingleCallCounter& callFrame)
    {
#if !ENABLE_TTD_DEBUGGING
        AssertMsg(false, "Debugger is not enabled so you shouldn't be calling this");
        this->Clear();
#else
        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = callFrame.Function->GetStatementStartOffset(callFrame.CurrentStatementIndex);
        callFrame.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcLine, &srcColumn);

        this->SetLocation(callFrame.EventTime, callFrame.FunctionTime, callFrame.LoopTime, callFrame.Function, (uint32)srcLine, (uint32)srcColumn);
#endif
    }

    void TTDebuggerSourceLocation::SetLocation(int64 etime, int64 ftime, int64 ltime, Js::FunctionBody* body, ULONG line, LONG column)
    {
#if !ENABLE_TTD_DEBUGGING
        AssertMsg(false, "Debugger is not enabled so you shouldn't be calling this");
        this->Clear();
#else
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

        const char16* sourceFile = body->GetSourceContextInfo()->url;
        if(sourceFile != nullptr)
        {
            size_t char16Length = wcslen(sourceFile) + 1;
            size_t byteLength = char16Length * sizeof(char16);

            this->m_sourceFile = new char16[char16Length];
            js_memcpy_s(this->m_sourceFile, byteLength, sourceFile, byteLength);
        }
#endif
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

    Js::FunctionBody* TTDebuggerSourceLocation::ResolveAssociatedSourceInfo(Js::ScriptContext* ctx) const
    {
        Js::FunctionBody* resBody = ctx->TTDContextInfo->FindFunctionBodyByFileName(this->m_sourceFile);

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

    bool TTDebuggerSourceLocation::IsBefore(const TTDebuggerSourceLocation& other) const
    {
        AssertMsg(this->m_ftime != -1 && other.m_ftime != -1, "These aren't orderable!!!");
        AssertMsg(this->m_ltime != -1 && other.m_ltime != -1, "These aren't orderable!!!");

        //first check the order of the time parts
        if(this->m_etime != other.m_etime)
        {
            return this->m_etime < other.m_etime;
        }

        if(this->m_ftime != other.m_ftime)
        {
            return this->m_ftime < other.m_ftime;
        }

        if(this->m_ltime != other.m_ltime)
        {
            return this->m_ltime < other.m_ltime;
        }

        //so all times are the same => min column/min row decide
        if(this->m_functionLine != other.m_functionLine)
        {
            return this->m_functionLine < other.m_functionLine;
        }

        if(this->m_functionColumn != other.m_functionColumn)
        {
            return this->m_functionColumn < other.m_functionColumn;
        }

        //they are refering to the same location so this is *not* stricly before
        return false;
    }

    //////////////////

    namespace NSLogEvents
    {
        void PassVarToHostInReplay(Js::ScriptContext* ctx, TTDVar origVar, Js::Var replayVar)
        {
            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "We assume the bit patterns on these types are the same!!!");

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            if(replayVar == nullptr || TTD::JsSupport::IsVarTaggedInline(replayVar))
            {
                AssertMsg(origVar == replayVar, "Should be same bit pattern.");
            }
#endif

            if(replayVar != nullptr && TTD::JsSupport::IsVarPtrValued(replayVar))
            {
                ctx->TTDContextInfo->AddLocalRoot(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(origVar), Js::RecyclableObject::FromVar(replayVar));
            }
        }

        Js::Var InflateVarInReplay(Js::ScriptContext* ctx, TTDVar origVar)
        {
            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "We assume the bit patterns on these types are the same!!!");

            if(origVar == nullptr || TTD::JsSupport::IsVarTaggedInline(origVar))
            {
                return TTD_CONVERT_TTDVAR_TO_JSVAR(origVar);
            }
            else
            {
                return ctx->TTDContextInfo->LookupObjectForLogID(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(origVar));
            }
        }

        void EventLogEntry_Initialize(EventLogEntry* evt, EventKind tag, int64 etime)
        {
            evt->EventKind = tag;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            evt->EventTimeStamp = etime;
#endif
        }

        void EventLogEntry_Emit(const EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteTag<EventKind>(NSTokens::Key::eventKind, evt->EventKind);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            writer->WriteInt64(NSTokens::Key::eventTime, evt->EventTimeStamp, NSTokens::Separator::CommaSeparator);
#endif

            auto emitFP = evtFPVTable[(uint32)evt->EventKind].EmitFP;
            if(emitFP != nullptr)
            {
                emitFP(evt, writer, threadContext);
            }

            writer->WriteRecordEnd();
        }

        void EventLogEntry_Parse(EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            evt->EventKind = reader->ReadTag<EventKind>(NSTokens::Key::eventKind);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            evt->EventTimeStamp = reader->ReadInt64(NSTokens::Key::eventTime, true);
#endif

            auto parseFP = evtFPVTable[(uint32)evt->EventKind].ParseFP;
            if(parseFP != nullptr)
            {
                parseFP(evt, threadContext, reader, alloc);
            }

            reader->ReadRecordEnd();
        }

        //////////////////

        void SnapshotEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            SnapshotEventLogEntry_UnloadSnapshot(evt);
        }

        void SnapshotEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            writer->WriteInt64(NSTokens::Key::restoreTime, snapEvt->RestoreTimestamp, NSTokens::Separator::CommaSeparator);

            if(snapEvt->Snap != nullptr)
            {
                snapEvt->Snap->EmitSnapshot(snapEvt->RestoreTimestamp, threadContext);
            }
        }

        void SnapshotEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            snapEvt->RestoreTimestamp = reader->ReadInt64(NSTokens::Key::restoreTime, true);
            snapEvt->Snap = nullptr;
        }

        void SnapshotEventLogEntry_EnsureSnapshotDeserialized(EventLogEntry* evt, ThreadContext* threadContext)
        {
            SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            if(snapEvt->Snap == nullptr)
            {
                snapEvt->Snap = SnapShot::Parse(snapEvt->RestoreTimestamp, threadContext);
            }
        }

        void SnapshotEventLogEntry_UnloadSnapshot(EventLogEntry* evt)
        {
            SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            if(snapEvt->Snap != nullptr)
            {
                TT_HEAP_DELETE(SnapShot, snapEvt->Snap);
                snapEvt->Snap = nullptr;
            }
        }

        void EventLoopYieldPointEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const EventLoopYieldPointEntry* ypEvt = GetInlineEventDataAs<EventLoopYieldPointEntry, EventKind::EventLoopYieldPointTag>(evt);

            writer->WriteUInt64(NSTokens::Key::eventTime, ypEvt->EventTimeStamp, NSTokens::Separator::CommaSeparator);
            writer->WriteDouble(NSTokens::Key::loopTime, ypEvt->EventWallTime, NSTokens::Separator::CommaSeparator);
        }

        void EventLoopYieldPointEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            EventLoopYieldPointEntry* ypEvt = GetInlineEventDataAs<EventLoopYieldPointEntry, EventKind::EventLoopYieldPointTag>(evt);

            ypEvt->EventTimeStamp = reader->ReadUInt64(NSTokens::Key::eventTime, true);
            ypEvt->EventWallTime = reader->ReadDouble(NSTokens::Key::loopTime, true);
        }

        //////////////////

        void CodeLoadEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const CodeLoadEventLogEntry* codeEvt = GetInlineEventDataAs<CodeLoadEventLogEntry, EventKind::TopLevelCodeTag>(evt);

            writer->WriteUInt64(NSTokens::Key::u64Val, codeEvt->BodyCounterId, NSTokens::Separator::CommaSeparator);
        }

        void CodeLoadEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            CodeLoadEventLogEntry* codeEvt = GetInlineEventDataAs<CodeLoadEventLogEntry, EventKind::TopLevelCodeTag>(evt);

            codeEvt->BodyCounterId = reader->ReadUInt64(NSTokens::Key::u64Val, true);
        }

        void TelemetryEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            TelemetryEventLogEntry* telemetryEvt = GetInlineEventDataAs<TelemetryEventLogEntry, EventKind::TelemetryLogTag>(evt);

            alloc.UnlinkString(telemetryEvt->InfoString);
        }

        void TelemetryEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const TelemetryEventLogEntry* telemetryEvt = GetInlineEventDataAs<TelemetryEventLogEntry, EventKind::TelemetryLogTag>(evt);

            writer->WriteString(NSTokens::Key::stringVal, telemetryEvt->InfoString, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, telemetryEvt->DoPrint, NSTokens::Separator::CommaSeparator);
        }

        void TelemetryEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            TelemetryEventLogEntry* telemetryEvt = GetInlineEventDataAs<TelemetryEventLogEntry, EventKind::TelemetryLogTag>(evt);

            reader->ReadString(NSTokens::Key::stringVal, alloc, telemetryEvt->InfoString, true);
            telemetryEvt->DoPrint = reader->ReadBool(NSTokens::Key::boolVal, true);
        }

        //////////////////

        void RandomSeedEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const RandomSeedEventLogEntry* rndEvt = GetInlineEventDataAs<RandomSeedEventLogEntry, EventKind::RandomSeedTag>(evt);

            writer->WriteUInt64(NSTokens::Key::u64Val, rndEvt->Seed0, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::u64Val, rndEvt->Seed1, NSTokens::Separator::CommaSeparator);

        }

        void RandomSeedEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            RandomSeedEventLogEntry* rndEvt = GetInlineEventDataAs<RandomSeedEventLogEntry, EventKind::RandomSeedTag>(evt);

            rndEvt->Seed0 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            rndEvt->Seed1 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
        }

        void DoubleEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const DoubleEventLogEntry* dblEvt = GetInlineEventDataAs<DoubleEventLogEntry, EventKind::DoubleTag>(evt);

            writer->WriteDouble(NSTokens::Key::doubleVal, dblEvt->DoubleValue, NSTokens::Separator::CommaSeparator);
        }

        void DoubleEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            DoubleEventLogEntry* dblEvt = GetInlineEventDataAs<DoubleEventLogEntry, EventKind::DoubleTag>(evt);

            dblEvt->DoubleValue = reader->ReadDouble(NSTokens::Key::doubleVal, true);
        }

        void StringValueEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            StringValueEventLogEntry* strEvt = GetInlineEventDataAs<StringValueEventLogEntry, EventKind::StringTag>(evt);

            alloc.UnlinkString(strEvt->StringValue);
        }

        void StringValueEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const StringValueEventLogEntry* strEvt = GetInlineEventDataAs<StringValueEventLogEntry, EventKind::StringTag>(evt);

            writer->WriteString(NSTokens::Key::stringVal, strEvt->StringValue, NSTokens::Separator::CommaSeparator);
        }

        void StringValueEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            StringValueEventLogEntry* strEvt = GetInlineEventDataAs<StringValueEventLogEntry, EventKind::StringTag>(evt);

            reader->ReadString(NSTokens::Key::stringVal, alloc, strEvt->StringValue, true);
        }

        //////////////////

        void PropertyEnumStepEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            PropertyEnumStepEventLogEntry* propertyEvt = GetInlineEventDataAs<PropertyEnumStepEventLogEntry, EventKind::PropertyEnumTag>(evt);

            if(!IsNullPtrTTString(propertyEvt->PropertyString))
            {
                alloc.UnlinkString(propertyEvt->PropertyString);
            }
        }

        void PropertyEnumStepEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const PropertyEnumStepEventLogEntry* propertyEvt = GetInlineEventDataAs<PropertyEnumStepEventLogEntry, EventKind::PropertyEnumTag>(evt);

            writer->WriteBool(NSTokens::Key::boolVal, propertyEvt->ReturnCode ? true : false, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::propertyId, propertyEvt->Pid, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::attributeFlags, propertyEvt->Attributes, NSTokens::Separator::CommaSeparator);

            if(propertyEvt->ReturnCode)
            {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                writer->WriteString(NSTokens::Key::stringVal, propertyEvt->PropertyString, NSTokens::Separator::CommaSeparator);
#else
                if(propertyEvt->Pid == Js::Constants::NoProperty)
                {
                    writer->WriteString(NSTokens::Key::stringVal, propertyEvt->PropertyString, NSTokens::Separator::CommaSeparator);
                }
#endif
            }
        }

        void PropertyEnumStepEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            PropertyEnumStepEventLogEntry* propertyEvt = GetInlineEventDataAs<PropertyEnumStepEventLogEntry, EventKind::PropertyEnumTag>(evt);

            propertyEvt->ReturnCode = reader->ReadBool(NSTokens::Key::boolVal, true);
            propertyEvt->Pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);
            propertyEvt->Attributes = (Js::PropertyAttributes)reader->ReadUInt32(NSTokens::Key::attributeFlags, true);

            InitializeAsNullPtrTTString(propertyEvt->PropertyString);

            if(propertyEvt->ReturnCode)
            {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                reader->ReadString(NSTokens::Key::stringVal, alloc, propertyEvt->PropertyString, true);
#else
                if(propertyEvt->Pid == Js::Constants::NoProperty)
                {
                    reader->ReadString(NSTokens::Key::stringVal, alloc, propertyEvt->PropertyString, true);
                }
#endif
            }
        }

        //////////////////

        void SymbolCreationEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const SymbolCreationEventLogEntry* symEvt = GetInlineEventDataAs<SymbolCreationEventLogEntry, EventKind::SymbolCreationTag>(evt);

            writer->WriteUInt32(NSTokens::Key::propertyId, symEvt->Pid, NSTokens::Separator::CommaSeparator);
        }

        void SymbolCreationEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            SymbolCreationEventLogEntry* symEvt = GetInlineEventDataAs<SymbolCreationEventLogEntry, EventKind::SymbolCreationTag>(evt);

            symEvt->Pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);
        }

        //////////////////

        int64 ExternalCbRegisterCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt)
        {
            const ExternalCbRegisterCallEventLogEntry* cbrEvt = GetInlineEventDataAs<ExternalCbRegisterCallEventLogEntry, EventKind::ExternalCbRegisterCall>(evt);

            return cbrEvt->LastNestedEventTime;
        }

        void ExternalCbRegisterCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const ExternalCbRegisterCallEventLogEntry* cbrEvt = GetInlineEventDataAs<ExternalCbRegisterCallEventLogEntry, EventKind::ExternalCbRegisterCall>(evt);

            NSSnapValues::EmitTTDVar(cbrEvt->CallbackFunction, writer, NSTokens::Separator::CommaSeparator);
            writer->WriteInt64(NSTokens::Key::i64Val, cbrEvt->LastNestedEventTime, NSTokens::Separator::CommaSeparator);
        }

        void ExternalCbRegisterCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            ExternalCbRegisterCallEventLogEntry* cbrEvt = GetInlineEventDataAs<ExternalCbRegisterCallEventLogEntry, EventKind::ExternalCbRegisterCall>(evt);

            cbrEvt->CallbackFunction = NSSnapValues::ParseTTDVar(true, reader);
            cbrEvt->LastNestedEventTime = reader->ReadInt64(NSTokens::Key::i64Val, true);
        }

        //////////////////

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        void ExternalCallEventLogEntry_ProcessDiagInfoPre(EventLogEntry* evt, Js::JavascriptFunction* function, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            Js::JavascriptString* displayName = function->GetDisplayName();
            alloc.CopyStringIntoWLength(displayName->GetSz(), displayName->GetLength(), callEvt->AdditionalInfo->FunctionName);
        }
#endif

        int64 ExternalCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt)
        {
            const ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            return callEvt->AdditionalInfo->LastNestedEventTime;
        }

        void ExternalCallEventLogEntry_ProcessArgs(EventLogEntry* evt, int32 rootDepth, Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);
            callEvt->AdditionalInfo = alloc.SlabAllocateStruct<ExternalCallEventLogEntry_AdditionalInfo>();

            callEvt->RootNestingDepth = rootDepth;
            callEvt->ArgCount = argc + 1;

            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "These need to be the same size (and have same bit layout) for this to work!");

            callEvt->ArgArray = alloc.SlabAllocateArray<TTDVar>(callEvt->ArgCount);
            callEvt->ArgArray[0] = static_cast<TTDVar>(function);
            js_memcpy_s(callEvt->ArgArray + 1, (callEvt->ArgCount - 1) * sizeof(TTDVar), argv, argc * sizeof(Js::Var));

            //Initialize this info in case we terminate without completing (e.g. exit(1))
            callEvt->ReturnValue = nullptr;
            callEvt->AdditionalInfo->HasScriptException = false;
            callEvt->AdditionalInfo->HasTerminiatingException = false;
            callEvt->AdditionalInfo->LastNestedEventTime = TTD_EVENT_MAXTIME;
        }

        void ExternalCallEventLogEntry_ProcessReturn(EventLogEntry* evt, Js::Var res, bool hasScriptException, bool hasTerminiatingException, int64 lastNestedEvent)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            callEvt->ReturnValue = TTD_CONVERT_JSVAR_TO_TTDVAR(res);
            callEvt->AdditionalInfo->HasScriptException = hasScriptException;
            callEvt->AdditionalInfo->HasTerminiatingException = hasTerminiatingException;
            callEvt->AdditionalInfo->LastNestedEventTime = lastNestedEvent;
        }

        void ExternalCallEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            alloc.UnlinkAllocation(callEvt->ArgArray);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            alloc.UnlinkString(callEvt->AdditionalInfo->FunctionName);
#endif

            alloc.UnlinkAllocation(callEvt->AdditionalInfo);
        }

        void ExternalCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            writer->WriteString(NSTokens::Key::name, callEvt->AdditionalInfo->FunctionName, NSTokens::Separator::CommaSeparator);
#endif

            writer->WriteInt32(NSTokens::Key::rootNestingDepth, callEvt->RootNestingDepth, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(callEvt->ArgCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < callEvt->ArgCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                NSSnapValues::EmitTTDVar(callEvt->ArgArray[i], writer, sep);
            }
            writer->WriteSequenceEnd();

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(callEvt->ReturnValue, writer, NSTokens::Separator::NoSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, callEvt->AdditionalInfo->HasScriptException, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, callEvt->AdditionalInfo->HasTerminiatingException, NSTokens::Separator::CommaSeparator);
            writer->WriteInt64(NSTokens::Key::i64Val, callEvt->AdditionalInfo->LastNestedEventTime, NSTokens::Separator::CommaSeparator);
        }

        void ExternalCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);
            callEvt->AdditionalInfo = alloc.SlabAllocateStruct<ExternalCallEventLogEntry_AdditionalInfo>();

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            reader->ReadString(NSTokens::Key::name, alloc, callEvt->AdditionalInfo->FunctionName, true);
#endif

            callEvt->RootNestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);

            callEvt->ArgCount = reader->ReadLengthValue(true);
            callEvt->ArgArray = alloc.SlabAllocateArray<TTDVar>(callEvt->ArgCount);

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < callEvt->ArgCount; ++i)
            {
                callEvt->ArgArray[i] = NSSnapValues::ParseTTDVar(i != 0, reader);
            }
            reader->ReadSequenceEnd();

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            callEvt->ReturnValue = NSSnapValues::ParseTTDVar(false, reader);

            callEvt->AdditionalInfo->HasScriptException = reader->ReadBool(NSTokens::Key::boolVal, true);
            callEvt->AdditionalInfo->HasTerminiatingException = reader->ReadBool(NSTokens::Key::boolVal, true);
            callEvt->AdditionalInfo->LastNestedEventTime = reader->ReadInt64(NSTokens::Key::i64Val, true);
        }
    }
}

#endif
