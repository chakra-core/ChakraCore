//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    namespace NSLogEvents
    {
        //true if this is a root call function
        bool IsJsRTActionRootCall(const EventLogEntry* evt);

        //We have a number of loops where we look for a snapshot or root with a given time value -- this encapsulates the access logic
        int64 AccessTimeInRootCallOrSnapshot(const EventLogEntry* evt, bool& isSnap, bool& isRoot, bool& hasRtrSnap);
        bool TryGetTimeFromRootCallOrSnapshot(const EventLogEntry* evt, int64& res);
        int64 GetTimeFromRootCallOrSnapshot(const EventLogEntry* evt);

        //Handle the replay of the result of an action for the the given action type and tag
        template <typename T, EventKind tag>
        void JsRTActionHandleResultForReplay(ThreadContextTTD* executeContext, const EventLogEntry* evt, Js::Var result)
        {
            TTDVar origResult = GetInlineEventDataAs<T, tag>(evt)->Result;
            PassVarToHostInReplay(executeContext, origResult, result);
        }

        //////////////////

        //A generic struct for actions that only need a result value
        struct JsRTResultOnlyAction
        {
            TTDVar Result;
        };

        template <EventKind tag>
        void JsRTResultOnlyAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTResultOnlyAction* vAction = GetInlineEventDataAs<JsRTResultOnlyAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);
        }

        template <EventKind tag>
        void JsRTResultOnlyAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTResultOnlyAction* vAction = GetInlineEventDataAs<JsRTResultOnlyAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);
        }

        //A generic struct for actions that only need a single integral value
        struct JsRTIntegralArgumentAction
        {
            TTDVar Result;
            int64 Scalar;
        };

        template <EventKind tag>
        void JsRTIntegralArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTIntegralArgumentAction* vAction = GetInlineEventDataAs<JsRTIntegralArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteInt64(NSTokens::Key::i64Val, vAction->Scalar, NSTokens::Separator::CommaSeparator);
        }

        template <EventKind tag>
        void JsRTIntegralArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTIntegralArgumentAction* vAction = GetInlineEventDataAs<JsRTIntegralArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            vAction->Scalar = reader->ReadInt64(NSTokens::Key::i64Val, true);
        }

        //A generic struct for actions that only need variables
        template <size_t count>
        struct JsRTVarsArgumentAction_InternalUse
        {
            TTDVar Result;
            TTDVar VarArray[count];
        };

        template <EventKind tag, size_t count>
        void JsRTVarsArgumentAction_InternalUse_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTVarsArgumentAction_InternalUse<count>* vAction = GetInlineEventDataAs<JsRTVarsArgumentAction_InternalUse<count>, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            if(count == 1)
            {
                NSSnapValues::EmitTTDVar(vAction->VarArray[0], writer, NSTokens::Separator::CommaSeparator);
            }
            else
            {
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(size_t i = 0; i < count; ++i)
                {
                    NSSnapValues::EmitTTDVar(vAction->VarArray[i], writer, i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
                }
                writer->WriteSequenceEnd();
            }
        }

        template <EventKind tag, size_t count>
        void JsRTVarsArgumentAction_InternalUse_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTVarsArgumentAction_InternalUse<count>* vAction = GetInlineEventDataAs<JsRTVarsArgumentAction_InternalUse<count>, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            if(count == 1)
            {
                vAction->VarArray[0] = NSSnapValues::ParseTTDVar(true, reader);
            }
            else
            {
                reader->ReadSequenceStart_WDefaultKey(true);
                for(size_t i = 0; i < count; ++i)
                {
                    vAction->VarArray[i] = NSSnapValues::ParseTTDVar(i != 0, reader);
                }
                reader->ReadSequenceEnd();
            }
        }

        template <size_t count, size_t index>
        TTDVar GetVarItem_InternalUse(const JsRTVarsArgumentAction_InternalUse<count>* argAction)
        {
            static_assert(index < count, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            return argAction->VarArray[index];
        }
        template <size_t count> TTDVar GetVarItem_0(const JsRTVarsArgumentAction_InternalUse<count>* argAction) { return GetVarItem_InternalUse<count, 0>(argAction); }
        template <size_t count> TTDVar GetVarItem_1(const JsRTVarsArgumentAction_InternalUse<count>* argAction) { return GetVarItem_InternalUse<count, 1>(argAction); }
        template <size_t count> TTDVar GetVarItem_2(const JsRTVarsArgumentAction_InternalUse<count>* argAction) { return GetVarItem_InternalUse<count, 2>(argAction); }

        template <size_t count, size_t index>
        void SetVarItem_InternalUse(JsRTVarsArgumentAction_InternalUse<count>* argAction, TTDVar value)
        {
            static_assert(index < count, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            argAction->VarArray[index] = value;
        }
        template <size_t count> void SetVarItem_0(JsRTVarsArgumentAction_InternalUse<count>* argAction, TTDVar value) { return SetVarItem_InternalUse<count, 0>(argAction, value); }
        template <size_t count> void SetVarItem_1(JsRTVarsArgumentAction_InternalUse<count>* argAction, TTDVar value) { return SetVarItem_InternalUse<count, 1>(argAction, value); }
        template <size_t count> void SetVarItem_2(JsRTVarsArgumentAction_InternalUse<count>* argAction, TTDVar value) { return SetVarItem_InternalUse<count, 2>(argAction, value); }

        typedef JsRTVarsArgumentAction_InternalUse<1> JsRTSingleVarArgumentAction;
        template <EventKind tag> void JsRTSingleVarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarsArgumentAction_InternalUse_Emit<tag, 1>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTSingleVarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarsArgumentAction_InternalUse_Parse<tag, 1>(evt, threadContext, reader, alloc); }

        typedef JsRTVarsArgumentAction_InternalUse<2> JsRTDoubleVarArgumentAction;
        template <EventKind tag> void JsRTDoubleVarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarsArgumentAction_InternalUse_Emit<tag, 2>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTDoubleVarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarsArgumentAction_InternalUse_Parse<tag, 2>(evt, threadContext, reader, alloc); }

        typedef JsRTVarsArgumentAction_InternalUse<3> JsRTTrippleVarArgumentAction;
        template <EventKind tag> void JsRTTrippleVarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarsArgumentAction_InternalUse_Emit<tag, 3>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTTrippleVarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarsArgumentAction_InternalUse_Parse<tag, 3>(evt, threadContext, reader, alloc); }

        //A generic struct for actions that need variables and scalar data
        template <size_t vcount, size_t icount>
        struct JsRTVarAndIntegralArgumentsAction_InternalUse
        {
            TTDVar Result;
            TTDVar VarArray[vcount];
            int64 ScalarArray[icount];
        };

        template <EventKind tag, size_t vcount, size_t icount>
        void JsRTVarAndIntegralArgumentsAction_InternalUse_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* vAction = GetInlineEventDataAs<JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(size_t i = 0; i < vcount; ++i)
            {
                NSSnapValues::EmitTTDVar(vAction->VarArray[i], writer, i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }

            for(size_t i = 0; i < icount; ++i)
            {
                writer->WriteNakedInt64(vAction->ScalarArray[i], vcount + i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        template <EventKind tag, size_t vcount, size_t icount>
        void JsRTVarAndIntegralArgumentsAction_InternalUse_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* vAction = GetInlineEventDataAs<JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            reader->ReadSequenceStart_WDefaultKey(true);
            for(size_t i = 0; i < vcount; ++i)
            {
                vAction->VarArray[i] = NSSnapValues::ParseTTDVar(i != 0, reader);
            }

            for(size_t i = 0; i < icount; ++i)
            {
                vAction->ScalarArray[i] = reader->ReadNakedInt64(vcount + i != 0);
            }
            reader->ReadSequenceEnd();
        }

        template <size_t vcount, size_t icount, size_t index>
        TTDVar GetVarItem_InternalUse(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction)
        {
            static_assert(index < vcount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            return argAction->VarArray[index];
        }
        template <size_t vcount, size_t icount> TTDVar GetVarItem_0(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction) { return GetVarItem_InternalUse<vcount, icount, 0>(argAction); }
        template <size_t vcount, size_t icount> TTDVar GetVarItem_1(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction) { return GetVarItem_InternalUse<vcount, icount, 1>(argAction); }

        template <size_t vcount, size_t icount, size_t index>
        void SetVarItem_InternalUse(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, TTDVar value)
        {
            static_assert(index < vcount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            argAction->VarArray[index] = value;
        }
        template <size_t vcount, size_t icount> void SetVarItem_0(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, TTDVar value) { return SetVarItem_InternalUse<vcount, icount, 0>(argAction, value); }
        template <size_t vcount, size_t icount> void SetVarItem_1(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, TTDVar value) { return SetVarItem_InternalUse<vcount, icount, 1>(argAction, value); }

        template <size_t vcount, size_t icount, size_t index>
        int64 GetScalarItem_InternalUse(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction)
        {
            static_assert(index < icount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            return argAction->ScalarArray[index];
        }
        template <size_t vcount, size_t icount> int64 GetScalarItem_0(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction) { return GetScalarItem_InternalUse<vcount, icount, 0>(argAction); }
        template <size_t vcount, size_t icount> int64 GetScalarItem_1(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction) { return GetScalarItem_InternalUse<vcount, icount, 1>(argAction); }

        template <size_t vcount, size_t icount, size_t index>
        void SetScalarItem_InternalUse(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, int64 value)
        {
            static_assert(index < icount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            argAction->ScalarArray[index] = value;
        }
        template <size_t vcount, size_t icount> void SetScalarItem_0(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, int64 value) { return SetScalarItem_InternalUse<vcount, icount, 0>(argAction, value); }
        template <size_t vcount, size_t icount> void SetScalarItem_1(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, int64 value) { return SetScalarItem_InternalUse<vcount, icount, 1>(argAction, value); }

        template <size_t vcount, size_t icount>
        Js::PropertyId GetPropertyIdItem(const JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction)
        {
            static_assert(0 < icount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            return (Js::PropertyId)argAction->ScalarArray[0];
        }

        template <size_t vcount, size_t icount>
        void SetPropertyIdItem(JsRTVarAndIntegralArgumentsAction_InternalUse<vcount, icount>* argAction, Js::PropertyId value)
        {
            AssertMsg(0 < icount, "Index out of bounds in JsRTVarAndIntegralArgumentsAction.");
            argAction->ScalarArray[0] = (int64)value;
        }

        typedef JsRTVarAndIntegralArgumentsAction_InternalUse<1, 1> JsRTSingleVarScalarArgumentAction;
        template <EventKind tag> void JsRTSingleVarScalarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarAndIntegralArgumentsAction_InternalUse_Emit<tag, 1, 1>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTSingleVarScalarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarAndIntegralArgumentsAction_InternalUse_Parse<tag, 1, 1>(evt, threadContext, reader, alloc); }

        typedef JsRTVarAndIntegralArgumentsAction_InternalUse<2, 1> JsRTDoubleVarSingleScalarArgumentAction;
        template <EventKind tag> void JsRTDoubleVarSingleScalarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarAndIntegralArgumentsAction_InternalUse_Emit<tag, 2, 1>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTDoubleVarSingleScalarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarAndIntegralArgumentsAction_InternalUse_Parse<tag, 2, 1>(evt, threadContext, reader, alloc); }

        typedef JsRTVarAndIntegralArgumentsAction_InternalUse<1, 2> JsRTSingleVarDoubleScalarArgumentAction;
        template <EventKind tag> void JsRTSingleVarDoubleScalarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarAndIntegralArgumentsAction_InternalUse_Emit<tag, 1, 2>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTSingleVarDoubleScalarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarAndIntegralArgumentsAction_InternalUse_Parse<tag, 1, 2>(evt, threadContext, reader, alloc); }

        typedef JsRTVarAndIntegralArgumentsAction_InternalUse<2, 2> JsRTDoubleVarDoubleScalarArgumentAction;
        template <EventKind tag> void JsRTDoubleVarDoubleScalarArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext) { JsRTVarAndIntegralArgumentsAction_InternalUse_Emit<tag, 2, 2>(evt, writer, threadContext); }
        template <EventKind tag> void JsRTDoubleVarDoubleScalarArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc) { JsRTVarAndIntegralArgumentsAction_InternalUse_Parse<tag, 2, 2>(evt, threadContext, reader, alloc); }

        //A struct for actions that are definied by their tag and a double
        struct JsRTDoubleArgumentAction
        {
            TTDVar Result;
            double DoubleValue;
        };

        template <EventKind tag>
        void JsRTDoubleArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTDoubleArgumentAction* dblAction = GetInlineEventDataAs<JsRTDoubleArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(dblAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteDouble(NSTokens::Key::doubleVal, dblAction->DoubleValue, NSTokens::Separator::CommaSeparator);
        }

        template <EventKind tag>
        void JsRTDoubleArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTDoubleArgumentAction* dblAction = GetInlineEventDataAs<JsRTDoubleArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            dblAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            dblAction->DoubleValue = reader->ReadDouble(NSTokens::Key::doubleVal, true);
        }

        //A struct for actions that are definied by their tag and a single string
        struct JsRTStringArgumentAction
        {
            TTDVar Result;
            TTString StringValue;
        };

        template <EventKind tag>
        void JsRTStringArgumentAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            JsRTStringArgumentAction* strAction = GetInlineEventDataAs<JsRTStringArgumentAction, tag>(evt);

            alloc.UnlinkString(strAction->StringValue);
        }

        template <EventKind tag>
        void JsRTStringArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTStringArgumentAction* strAction = GetInlineEventDataAs<JsRTStringArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(strAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteString(NSTokens::Key::stringVal, strAction->StringValue, NSTokens::Separator::CommaSeparator);
        }

        template <EventKind tag>
        void JsRTStringArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTStringArgumentAction* strAction = GetInlineEventDataAs<JsRTStringArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            strAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            reader->ReadString(NSTokens::Key::stringVal, alloc, strAction->StringValue, true);
        }

        //A struct for actions that are definied by a raw byte* + length
        struct JsRTByteBufferAction
        {
            TTDVar Result;
            byte* Buffer;
            uint32 Length;
        };

        template <EventKind tag>
        void JsRTByteBufferAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            JsRTByteBufferAction* bufferAction = GetInlineEventDataAs<JsRTByteBufferAction, tag>(evt);

            if(bufferAction->Buffer != nullptr)
            {
                alloc.UnlinkAllocation(bufferAction->Buffer);
            }
        }

        template <EventKind tag>
        void JsRTByteBufferAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTByteBufferAction* bufferAction = GetInlineEventDataAs<JsRTByteBufferAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(bufferAction->Result, writer, NSTokens::Separator::NoSeparator);

            bool badValue = (bufferAction->Buffer == nullptr && bufferAction->Length > 0);
            writer->WriteBool(NSTokens::Key::boolVal, badValue, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(bufferAction->Length, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            if(!badValue)
            {
                for(uint32 i = 0; i < bufferAction->Length; ++i)
                {
                    writer->WriteNakedByte(bufferAction->Buffer[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
                }
            }
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTByteBufferAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTByteBufferAction* bufferAction = GetInlineEventDataAs<JsRTByteBufferAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            bufferAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            bool badValue = reader->ReadBool(NSTokens::Key::boolVal, true);

            bufferAction->Length = reader->ReadLengthValue(true);
            bufferAction->Buffer = nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            if(!badValue)
            {
                bufferAction->Buffer = (bufferAction->Length != 0) ? alloc.SlabAllocateArray<byte>(bufferAction->Length) : nullptr;

                for(uint32 i = 0; i < bufferAction->Length; ++i)
                {
                    bufferAction->Buffer[i] = reader->ReadNakedByte(i != 0);
                }
                reader->ReadSequenceEnd();
            }
        }
        
        //////////////////

        //A class for constructor calls
        struct JsRTCreateScriptContextAction_KnownObjects
        {
            TTD_LOG_PTR_ID UndefinedObject;
            TTD_LOG_PTR_ID NullObject;
            TTD_LOG_PTR_ID TrueObject;
            TTD_LOG_PTR_ID FalseObject;
        };

        struct JsRTCreateScriptContextAction
        {
            TTD_LOG_PTR_ID GlobalObject;

            JsRTCreateScriptContextAction_KnownObjects* KnownObjects;
        };

        void CreateScriptContext_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void CreateScriptContext_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void CreateScriptContext_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void CreateScriptContext_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        void SetActiveScriptContext_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

#if !INT32VAR
        void CreateInt_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
#endif

        void CreateNumber_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void CreateBoolean_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void CreateString_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void CreateSymbol_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void Execute_CreateErrorHelper(const JsRTSingleVarArgumentAction* errorData, ThreadContextTTD* executeContext, Js::ScriptContext* ctx, EventKind eventKind, Js::Var* res);

        template<EventKind errorKind>
        void CreateError_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext)
        {
            const JsRTSingleVarArgumentAction* action = GetInlineEventDataAs<JsRTSingleVarArgumentAction, errorKind>(evt);

            Js::Var res = nullptr;
            Execute_CreateErrorHelper(action, executeContext, executeContext->GetActiveScriptContext(), errorKind, &res);

            if(res != nullptr)
            {
                JsRTActionHandleResultForReplay<JsRTSingleVarArgumentAction, errorKind>(executeContext, evt, res);
            }
        }

        void VarConvertToNumber_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void VarConvertToBoolean_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void VarConvertToString_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void VarConvertToObject_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void AddRootRef_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AddWeakRootRef_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void AllocateObject_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AllocateExternalObject_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AllocateArrayAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AllocateArrayBufferAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AllocateExternalArrayBufferAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void AllocateFunctionAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void HostProcessExitAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetAndClearExceptionWithMetadataAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetAndClearExceptionAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void SetExceptionAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void HasPropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void HasOwnPropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void InstanceOfAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void EqualsAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void GetPropertyIdFromSymbolAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void GetPrototypeAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetPropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetIndexAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetOwnPropertyInfoAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetOwnPropertyNamesInfoAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetOwnPropertySymbolsInfoAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void DefinePropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void DeletePropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void SetPrototypeAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void SetPropertyAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void SetIndexAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        void GetTypedArrayInfoAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void GetDataViewInfoAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        //////////////////

        //A generic struct for the naked buffer copy action
        struct JsRTRawBufferCopyAction
        {
            TTDVar Dst;
            TTDVar Src;
            uint32 DstIndx;
            uint32 SrcIndx;
            uint32 Count;
        };

        void JsRTRawBufferCopyAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTRawBufferCopyAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A generic struct for the naked buffer modify action (with buffer data)
        struct JsRTRawBufferModifyAction
        {
            TTDVar Trgt;
            byte* Data;
            uint32 Index;
            uint32 Length;
        };

        template <EventKind tag>
        void JsRTRawBufferModifyAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc)
        {
            JsRTRawBufferModifyAction* bufferAction = GetInlineEventDataAs<JsRTRawBufferModifyAction, tag>(evt);

            if(bufferAction->Data != nullptr)
            {
                alloc.UnlinkAllocation(bufferAction->Data);
            }
        }

        template <EventKind tag>
        void JsRTRawBufferModifyAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTRawBufferModifyAction* bufferAction = GetInlineEventDataAs<JsRTRawBufferModifyAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(bufferAction->Trgt, writer, NSTokens::Separator::NoSeparator);

            writer->WriteUInt32(NSTokens::Key::index, bufferAction->Index, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(bufferAction->Length, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < bufferAction->Length; ++i)
            {
                writer->WriteNakedByte(bufferAction->Data[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTRawBufferModifyAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTRawBufferModifyAction* bufferAction = GetInlineEventDataAs<JsRTRawBufferModifyAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            bufferAction->Trgt = NSSnapValues::ParseTTDVar(false, reader);

            bufferAction->Index = reader->ReadUInt32(NSTokens::Key::index, true);
            bufferAction->Length = reader->ReadUInt32(NSTokens::Key::count, true);

            bufferAction->Data = (bufferAction->Length != 0) ? alloc.SlabAllocateArray<byte>(bufferAction->Length) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < bufferAction->Length; ++i)
            {
                bufferAction->Data[i] = reader->ReadNakedByte(i != 0);
            }
            reader->ReadSequenceEnd();
        }

        void RawBufferCopySync_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void RawBufferModifySync_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void RawBufferAsyncModificationRegister_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void RawBufferAsyncModifyComplete_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);

        //////////////////

        //A class for constructor calls
        struct JsRTConstructCallAction
        {
            TTDVar Result;

            //The arguments info (constructor function is always args[0])
            uint32 ArgCount;
            TTDVar* ArgArray;

            //A buffer we can use for the actual invocation
            Js::Var* ExecArgs;
        };

        void JsRTConstructCallAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void JsRTConstructCallAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTConstructCallAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTConstructCallAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct for calls to script parse
        struct JsRTCodeParseAction
        {
            TTDVar Result;

            //The body counter id associated with this load
            uint32 BodyCtrId;

            //Is the code utf8
            bool IsUtf8;

            //The actual source code and the length in bytes
            byte* SourceCode;
            uint32 SourceByteLength;

            //The URI the souce code was loaded from and the source context id
            TTString SourceUri;
            uint64 SourceContextId;

            //The flags for loading this script
            LoadScriptFlag LoadFlag;
        };

        void JsRTCodeParseAction_SetBodyCtrId(EventLogEntry* parseEvent, uint32 bodyCtrId);

        void JsRTCodeParseAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void JsRTCodeParseAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTCodeParseAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTCodeParseAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct for the replay-debugger info associated with a JsRTCallFunctionAction
        struct JsRTCallFunctionAction_ReplayAdditionalInfo
        {
            //ready-to-run snapshot information -- null if not set and if we want to unload it we just throw it away
            SnapShot* RtRSnap;

            //A buffer we can use for the actual invocation
            Js::Var* ExecArgs;

            //Info on the last executed statement in this call
            TTDebuggerSourceLocation LastExecutedLocation;
        };

        //A struct for calls to that execute existing functions
        struct JsRTCallFunctionAction
        {
            TTDVar Result;

            //The re-entry depth we are at when this happens
            int32 CallbackDepth;

            //the number of arguments and the argument array -- function is always argument[0]
            uint32 ArgCount;
            TTDVar* ArgArray;

            //The actual event time associated with this call (is >= the TopLevelCallbackEventTime)
            int64 CallEventTime;

            //The event time that corresponds to the top-level event time around this call
            int64 TopLevelCallbackEventTime;

            //The additional info assocated with this action that we use in replay/debug but doesn't matter for record
            JsRTCallFunctionAction_ReplayAdditionalInfo* AdditionalReplayInfo;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //The last event time that is nested in this call
            int64 LastNestedEvent;

            //The name of the function
            TTString FunctionName;
#endif
        };

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        int64 JsRTCallFunctionAction_GetLastNestedEventTime(const EventLogEntry* evt);

        void JsRTCallFunctionAction_ProcessDiagInfoPre(EventLogEntry* evt, Js::Var funcVar, UnlinkableSlabAllocator& alloc);
        void JsRTCallFunctionAction_ProcessDiagInfoPost(EventLogEntry* evt, int64 lastNestedEvent);
#endif

        void JsRTCallFunctionAction_ProcessArgs(EventLogEntry* evt, int32 rootDepth, int64 callEventTime, Js::Var funcVar, uint32 argc, Js::Var* argv, int64 topLevelCallbackEventTime, UnlinkableSlabAllocator& alloc);

        void JsRTCallFunctionAction_Execute(const EventLogEntry* evt, ThreadContextTTD* executeContext);
        void JsRTCallFunctionAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTCallFunctionAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTCallFunctionAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //Unload the snapshot
        void JsRTCallFunctionAction_UnloadSnapshot(EventLogEntry* evt);

        //Set the last executed statement and frame (in debugging mode -- nops for replay mode)
        void JsRTCallFunctionAction_SetLastExecutedStatementAndFrameInfo(EventLogEntry* evt, const TTDebuggerSourceLocation& lastSourceLocation);
        bool JsRTCallFunctionAction_GetLastExecutedStatementAndFrameInfoForDebugger(const EventLogEntry* evt, TTDebuggerSourceLocation& lastSourceInfo);
    }
}

#endif

