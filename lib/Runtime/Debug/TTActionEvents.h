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
        void JsRTActionHandleResultForReplay(Js::ScriptContext* ctx, const EventLogEntry* evt, Js::Var result)
        {
            TTDVar origResult = GetInlineEventDataAs<T, tag>(evt)->Result;
            PassVarToHostInReplay(ctx, origResult, result);
        }

        //////////////////

        //A generic struct for actions that only need variables
        struct JsRTVarsArgumentAction
        {
            TTDVar Result;
            TTDVar Var1;
            TTDVar Var2;
            TTDVar Var3;
        };

        template <EventKind tag>
        void JsRTVarsArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTVarsArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var1, writer, NSTokens::Separator::NoSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var2, writer, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var3, writer, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTVarsArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTVarsArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            reader->ReadSequenceStart_WDefaultKey(true);
            vAction->Var1 = NSSnapValues::ParseTTDVar(false, reader);
            vAction->Var2 = NSSnapValues::ParseTTDVar(true, reader);
            vAction->Var3 = NSSnapValues::ParseTTDVar(true, reader);
            reader->ReadSequenceEnd();
        }

        //A generic struct for actions that only need variables and 1 (large) integral value (of known type from the union)
        struct JsRTVarsWithIntegralUnionArgumentAction
        {
            TTDVar Result;
            TTDVar Var1;
            TTDVar Var2;
            union { BOOL u_bVal; Js::PropertyId u_pid; int64 u_iVal; };
        };

        template <EventKind tag>
        void JsRTVarsWithIntegralUnionArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTVarsWithIntegralUnionArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsWithIntegralUnionArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteInt64(NSTokens::Key::i64Val, vAction->u_iVal, NSTokens::Separator::CommaSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var1, writer, NSTokens::Separator::NoSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var2, writer, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTVarsWithIntegralUnionArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTVarsWithIntegralUnionArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsWithIntegralUnionArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            vAction->u_iVal = reader->ReadInt64(NSTokens::Key::i64Val, true);

            reader->ReadSequenceStart_WDefaultKey(true);
            vAction->Var1 = NSSnapValues::ParseTTDVar(false, reader);
            vAction->Var2 = NSSnapValues::ParseTTDVar(true, reader);
            reader->ReadSequenceEnd();
        }

        //A generic struct for actions that only need variables and 2 (small) integral values
        struct JsRTVarsWithBoolAndPIDArgumentAction
        {
            TTDVar Result;
            TTDVar Var1;
            TTDVar Var2;
            Js::PropertyId Pid;
            BOOL BoolVal;
        };

        template <EventKind tag>
        void JsRTVarsWithBoolAndPIDArgumentAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTVarsWithBoolAndPIDArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsWithBoolAndPIDArgumentAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteUInt32(NSTokens::Key::u32Val, vAction->Pid, NSTokens::Separator::CommaSeparator);
            writer->WriteInt32(NSTokens::Key::i32Val, vAction->BoolVal, NSTokens::Separator::CommaSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var1, writer, NSTokens::Separator::NoSeparator);
            NSSnapValues::EmitTTDVar(vAction->Var2, writer, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTVarsWithBoolAndPIDArgumentAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTVarsWithBoolAndPIDArgumentAction* vAction = GetInlineEventDataAs<JsRTVarsWithBoolAndPIDArgumentAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            vAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            vAction->Pid = reader->ReadUInt32(NSTokens::Key::u32Val, true);
            vAction->BoolVal = reader->ReadInt32(NSTokens::Key::i32Val, true);

            reader->ReadSequenceStart_WDefaultKey(true);
            vAction->Var1 = NSSnapValues::ParseTTDVar(false, reader);
            vAction->Var2 = NSSnapValues::ParseTTDVar(true, reader);
            reader->ReadSequenceEnd();
        }

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

            alloc.UnlinkAllocation(bufferAction->Buffer);
        }

        template <EventKind tag>
        void JsRTByteBufferAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext)
        {
            const JsRTByteBufferAction* bufferAction = GetInlineEventDataAs<JsRTByteBufferAction, tag>(evt);

            writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(bufferAction->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteLengthValue(bufferAction->Length, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < bufferAction->Length; ++i)
            {
                writer->WriteNakedByte(bufferAction->Buffer[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        template <EventKind tag>
        void JsRTByteBufferAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc)
        {
            JsRTByteBufferAction* bufferAction = GetInlineEventDataAs<JsRTByteBufferAction, tag>(evt);

            reader->ReadKey(NSTokens::Key::argRetVal, true);
            bufferAction->Result = NSSnapValues::ParseTTDVar(false, reader);

            bufferAction->Length = reader->ReadLengthValue(true);
            bufferAction->Buffer = alloc.SlabAllocateArray<byte>(bufferAction->Length);

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < bufferAction->Length; ++i)
            {
                bufferAction->Buffer[i] = reader->ReadNakedByte(i != 0);
            }
            reader->ReadSequenceEnd();
        }
        
        //////////////////

#if !INT32VAR
        void CreateInt_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
#endif

        void CreateNumber_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void CreateBoolean_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void CreateString_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void CreateSymbol_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        Js::Var Execute_CreateErrorHelper(const JsRTVarsArgumentAction* errorData, Js::ScriptContext* ctx, EventKind eventKind);

        template<EventKind errorKind>
        void CreateError_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx)
        {
            const JsRTVarsArgumentAction* action = GetInlineEventDataAs<JsRTVarsArgumentAction, errorKind>(evt);

            Js::Var res = Execute_CreateErrorHelper(action, ctx, errorKind);

            JsRTActionHandleResultForReplay<JsRTVarsArgumentAction, errorKind>(ctx, evt, res);
        }

        void VarConvertToNumber_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void VarConvertToBoolean_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void VarConvertToString_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void VarConvertToObject_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void AddRootRef_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void RemoveRootRef_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void AllocateObject_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void AllocateExternalObject_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void AllocateArrayAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void AllocateArrayBufferAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void AllocateExternalArrayBufferAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void AllocateFunctionAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void HostProcessExitAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void GetAndClearExceptionAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void SetExceptionAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void GetPropertyAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void GetIndexAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void GetOwnPropertyInfoAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void GetOwnPropertyNamesInfoAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void GetOwnPropertySymbolsInfoAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void DefinePropertyAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void DeletePropertyAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void SetPrototypeAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void SetPropertyAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void SetIndexAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

        void GetTypedArrayInfoAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

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

        void RawBufferCopySync_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void RawBufferModifySync_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void RawBufferAsyncModificationRegister_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void RawBufferAsyncModifyComplete_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);

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

        void JsRTConstructCallAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void JsRTConstructCallAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTConstructCallAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTConstructCallAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct for correlating host callback ids that are registed/created/canceled by this call
        struct JsRTCallbackAction
        {
            //The id of the current callback (we should be able to recompute this but we store it to simplify later analysis)
            int64 CurrentCallbackId;

            //the function tag and name for the callback function and the id that we are associating it with
            int64 NewCallbackId;
            
            //Info on the time that this registration occours
            TTDebuggerSourceLocation* RegisterLocation;

            //true if this is a cancelation/repeating action -- otherwise this is a 
            bool IsCreate;
            bool IsCancel;
            bool IsRepeating;
        };

        void JsRTCallbackAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void JsRTCallbackAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTCallbackAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTCallbackAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);
        
        bool JsRTCallbackAction_GetActionTimeInfoForDebugger(const EventLogEntry* evt, TTDebuggerSourceLocation& sourceLocation);

        //A struct for additional info associated with calls to script parse
        struct JsRTCodeParseAction_AdditionalInfo
        {
            //Is the code utf8
            bool IsUtf8;

            //The actual source code and the length in bytes
            byte* SourceCode;
            uint32 SourceByteLength;

            //The URI the souce code was loaded from and the document id we gave it internally
            TTString SourceUri;
            DWORD_PTR DocumentID;

            //The flags for loading this script
            LoadScriptFlag LoadFlag;
        };

        //A struct for calls to script parse
        struct JsRTCodeParseAction
        {
            TTDVar Result;

            //The body counter id associated with this load
            uint64 BodyCtrId;

            //All of the additional source code load info
            JsRTCodeParseAction_AdditionalInfo* AdditionalInfo;
        };

        void JsRTCodeParseAction_SetBodyCtrId(EventLogEntry* parseEvent, uint64 bodyCtrId);

        void JsRTCodeParseAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void JsRTCodeParseAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTCodeParseAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTCodeParseAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //A struct for additional info associated with calls to script parse
        struct JsRTCallFunctionAction_AdditionalInfo
        {
            double BeginTime;
            double EndTime;

            //The actual event time associated with this call (is >= the TopLevelCallbackEventTime)
            int64 CallEventTime;

            //The event time that corresponds to the top-level event time around this call
            int64 TopLevelCallbackEventTime;

            //ready-to-run snapshot information -- null if not set and if we want to unload it we just throw it away
            SnapShot* RtRSnap;

            //A buffer we can use for the actual invocation
            Js::Var* ExecArgs;

            //Info on the last executed statement in this call
            bool MarkedAsJustMyCode;
            TTDebuggerSourceLocation LastExecutedLocation;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            //The last event time that is nested in this call
            int64 LastNestedEvent;

            //The name of the function
            TTString FunctionName;
#endif
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

            //Additional info associated with the Action
            JsRTCallFunctionAction_AdditionalInfo* AdditionalInfo;
        };

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        int64 JsRTCallFunctionAction_GetLastNestedEventTime(const EventLogEntry* evt);

        void JsRTCallFunctionAction_ProcessDiagInfoPre(EventLogEntry* evt, Js::JavascriptFunction* function, UnlinkableSlabAllocator& alloc);
        void JsRTCallFunctionAction_ProcessDiagInfoPost(EventLogEntry* evt, int64 lastNestedEvent);
#endif

        void JsRTCallFunctionAction_ProcessArgs(EventLogEntry* evt, int32 rootDepth, int64 callEventTime, Js::JavascriptFunction* function, uint32 argc, Js::Var* argv, double wallTime, int64 topLevelCallbackEventTime, UnlinkableSlabAllocator& alloc);

        void JsRTCallFunctionAction_Execute(const EventLogEntry* evt, Js::ScriptContext* ctx);
        void JsRTCallFunctionAction_UnloadEventMemory(EventLogEntry* evt, UnlinkableSlabAllocator& alloc);
        void JsRTCallFunctionAction_Emit(const EventLogEntry* evt, FileWriter* writer, ThreadContext* threadContext);
        void JsRTCallFunctionAction_Parse(EventLogEntry* evt, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc);

        //Unload the snapshot
        void JsRTCallFunctionAction_UnloadSnapshot(EventLogEntry* evt);

        //Set the last executed statement and frame (in debugging mode -- nops for replay mode)
        void JsRTCallFunctionAction_SetLastExecutedStatementAndFrameInfo(EventLogEntry* evt, bool markedAsJustMyCode, const TTDebuggerSourceLocation& lastSourceLocation);
        bool JsRTCallFunctionAction_GetLastExecutedStatementAndFrameInfoForDebugger(const EventLogEntry* evt, bool* markedAsJustMyCode, TTDebuggerSourceLocation& lastSourceInfo);
    }
}

#endif

