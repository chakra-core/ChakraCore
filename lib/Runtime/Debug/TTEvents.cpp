//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    namespace NSLogEvents
    {
        void PassVarToHostInReplay(ThreadContextTTD* executeContext, TTDVar origVar, Js::Var replayVar)
        {
            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "We assume the bit patterns on these types are the same!!!");

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            if(replayVar == nullptr || TTD::JsSupport::IsVarTaggedInline(replayVar))
            {
                TTDAssert(TTD::JsSupport::AreInlineVarsEquiv(origVar, replayVar), "Should be same bit pattern.");
            }
#endif

            if(replayVar != nullptr && TTD::JsSupport::IsVarPtrValued(replayVar))
            {
                Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(replayVar);
                executeContext->AddLocalRoot(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(origVar), obj);
            }
        }

        Js::Var InflateVarInReplay(ThreadContextTTD* executeContext, TTDVar origVar)
        {
            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "We assume the bit patterns on these types are the same!!!");

            if(origVar == nullptr || TTD::JsSupport::IsVarTaggedInline(origVar))
            {
                return TTD_CONVERT_TTDVAR_TO_JSVAR(origVar);
            }
            else
            {
                return executeContext->LookupObjectForLogID(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(origVar));
            }
        }

        void EventLogEntry_Emit(const EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteTag<EventKind>(NSTokens::Key::eventKind, evt->EventKind);
            writer->WriteInt32(NSTokens::Key::eventResultStatus, evt->ResultStatus, NSTokens::Separator::CommaSeparator);

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

        EventKind EventLogEntry_ParseHeader(bool readSeperator, FileReader* reader)
        {
            reader->ReadRecordStart(readSeperator);

            return reader->ReadTag<EventKind>(NSTokens::Key::eventKind);
        }

        void EventLogEntry_ParseRest(EventLogEntry* evt, EventLogEntryVTableEntry* evtFPVTable, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            evt->ResultStatus = reader->ReadInt32(NSTokens::Key::eventResultStatus, true);

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

        bool EventFailsWithRuntimeError(const EventLogEntry* evt)
        {
            return !(EventDoesNotReturn(evt) || EventCompletesNormally(evt) || EventCompletesWithException(evt));
        }

        bool EventDoesNotReturn(const EventLogEntry* evt)
        {
            return evt->ResultStatus == -1;
        }

        bool EventCompletesNormally(const EventLogEntry* evt)
        {
            return (evt->ResultStatus == 0) || (evt->ResultStatus == TTD_REPLAY_JsErrorInvalidArgument) || (evt->ResultStatus == TTD_REPLAY_JsErrorArgumentNotObject);
        }

        bool EventCompletesWithException(const EventLogEntry* evt)
        {
            return (evt->ResultStatus == TTD_REPLAY_JsErrorCategoryScript) || (evt->ResultStatus == TTD_REPLAY_JsErrorScriptTerminated);
        }

        //////////////////

        void SnapshotEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            if(snapEvt->LiveContextCount != 0)
            {
                alloc.UnlinkAllocation(snapEvt->LiveContextIdArray);
            }

            if(snapEvt->LongLivedRefRootsCount != 0)
            {
                alloc.UnlinkAllocation(snapEvt->LongLivedRefRootsIdArray);
            }

            SnapshotEventLogEntry_UnloadSnapshot(evt);
        }

        void SnapshotEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            writer->WriteInt64(NSTokens::Key::restoreTime, snapEvt->RestoreTimestamp, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(snapEvt->LiveContextCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapEvt->LiveContextCount; ++i)
            {
                writer->WriteNakedLogTag(snapEvt->LiveContextIdArray[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapEvt->LongLivedRefRootsCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapEvt->LongLivedRefRootsCount; ++i)
            {
                writer->WriteNakedLogTag(snapEvt->LongLivedRefRootsIdArray[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();

            if(snapEvt->Snap != nullptr)
            {
                snapEvt->Snap->EmitSnapshot(snapEvt->RestoreTimestamp, threadContext);
            }
        }

        void SnapshotEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            SnapshotEventLogEntry* snapEvt = GetInlineEventDataAs<SnapshotEventLogEntry, EventKind::SnapshotTag>(evt);

            snapEvt->RestoreTimestamp = reader->ReadInt64(NSTokens::Key::restoreTime, true);

            snapEvt->LiveContextCount = reader->ReadLengthValue(true);
            snapEvt->LiveContextIdArray = (snapEvt->LiveContextCount != 0) ? alloc.SlabAllocateArray<TTD_LOG_PTR_ID>(snapEvt->LiveContextCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < snapEvt->LiveContextCount; ++i)
            {
                snapEvt->LiveContextIdArray[i] = reader->ReadNakedLogTag(i != 0);
            }
            reader->ReadSequenceEnd();

            snapEvt->LongLivedRefRootsCount = reader->ReadLengthValue(true);
            snapEvt->LongLivedRefRootsIdArray = (snapEvt->LongLivedRefRootsCount != 0) ? alloc.SlabAllocateArray<TTD_LOG_PTR_ID>(snapEvt->LongLivedRefRootsCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < snapEvt->LongLivedRefRootsCount; ++i)
            {
                snapEvt->LongLivedRefRootsIdArray[i] = reader->ReadNakedLogTag(i != 0);
            }
            reader->ReadSequenceEnd();

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

            writer->WriteUInt32(NSTokens::Key::u32Val, codeEvt->BodyCounterId, NSTokens::Separator::CommaSeparator);
        }

        void CodeLoadEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            CodeLoadEventLogEntry* codeEvt = GetInlineEventDataAs<CodeLoadEventLogEntry, EventKind::TopLevelCodeTag>(evt);

            codeEvt->BodyCounterId = reader->ReadUInt32(NSTokens::Key::u32Val, true);
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

            writer->WriteBool(NSTokens::Key::boolVal, !!propertyEvt->ReturnCode, NSTokens::Separator::CommaSeparator);
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

        void WeakCollectionContainsEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const WeakCollectionContainsEventLogEntry* wcEvt = GetInlineEventDataAs<WeakCollectionContainsEventLogEntry, EventKind::WeakCollectionContainsTag>(evt);

            writer->WriteBool(NSTokens::Key::boolVal, wcEvt->ContainsValue, NSTokens::Separator::CommaSeparator);
        }

        void WeakCollectionContainsEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            WeakCollectionContainsEventLogEntry* wcEvt = GetInlineEventDataAs<WeakCollectionContainsEventLogEntry, EventKind::WeakCollectionContainsTag>(evt);

            wcEvt->ContainsValue = reader->ReadBool(NSTokens::Key::boolVal, true);
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
            alloc.CopyStringIntoWLength(displayName->GetSz(), displayName->GetLength(), callEvt->FunctionName);
        }
#endif

        int64 ExternalCallEventLogEntry_GetLastNestedEventTime(const EventLogEntry* evt)
        {
            const ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            return callEvt->LastNestedEventTime;
        }

        void ExternalCallEventLogEntry_ProcessArgs(EventLogEntry* evt, int32 rootDepth, Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, bool checkExceptions, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            callEvt->RootNestingDepth = rootDepth;
            callEvt->ArgCount = argc + 1;

            static_assert(sizeof(TTDVar) == sizeof(Js::Var), "These need to be the same size (and have same bit layout) for this to work!");

            callEvt->ArgArray = alloc.SlabAllocateArray<TTDVar>(callEvt->ArgCount);
            callEvt->ArgArray[0] = static_cast<TTDVar>(function);
            js_memcpy_s(callEvt->ArgArray + 1, (callEvt->ArgCount - 1) * sizeof(TTDVar), argv, argc * sizeof(Js::Var));

            callEvt->ReturnValue = nullptr;
            callEvt->LastNestedEventTime = TTD_EVENT_MAXTIME;

            callEvt->CheckExceptionStatus = checkExceptions;
        }

        void ExternalCallEventLogEntry_ProcessReturn(EventLogEntry* evt, Js::Var res, int64 lastNestedEvent)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            callEvt->ReturnValue = TTD_CONVERT_JSVAR_TO_TTDVAR(res);
            callEvt->LastNestedEventTime = lastNestedEvent;
        }

        void ExternalCallEventLogEntry_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

            alloc.UnlinkAllocation(callEvt->ArgArray);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            alloc.UnlinkString(callEvt->FunctionName);
#endif
        }

        void ExternalCallEventLogEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            writer->WriteString(NSTokens::Key::name, callEvt->FunctionName, NSTokens::Separator::CommaSeparator);
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

            writer->WriteBool(NSTokens::Key::boolVal, callEvt->CheckExceptionStatus, NSTokens::Separator::CommaSeparator);

            writer->WriteInt64(NSTokens::Key::i64Val, callEvt->LastNestedEventTime, NSTokens::Separator::CommaSeparator);
        }

        void ExternalCallEventLogEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            ExternalCallEventLogEntry* callEvt = GetInlineEventDataAs<ExternalCallEventLogEntry, EventKind::ExternalCallTag>(evt);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            reader->ReadString(NSTokens::Key::name, alloc, callEvt->FunctionName, true);
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

            callEvt->CheckExceptionStatus = reader->ReadBool(NSTokens::Key::boolVal, true);

            callEvt->LastNestedEventTime = reader->ReadInt64(NSTokens::Key::i64Val, true);
        }

        //////////////////

        void ExplicitLogWriteEntry_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            ; //We don't track any extra data with this
        }

        void ExplicitLogWriteEntry_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            ; //We don't track any extra data with this
        }
    }
}

#endif
