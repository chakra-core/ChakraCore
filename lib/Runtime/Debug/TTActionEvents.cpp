//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    void JsRTActionLogEntry::JsRTBaseEmit(FileWriter* writer) const
    {
        writer->WriteLogTag(NSTokens::Key::ctxTag, this->m_ctxTag, NSTokens::Separator::CommaSeparator);
        writer->WriteTag<JsRTActionType>(NSTokens::Key::jsrtEventKind, this->m_actionTypeTag, NSTokens::Separator::CommaSeparator);
    }

    JsRTActionLogEntry::JsRTActionLogEntry(int64 eTime, TTD_LOG_TAG ctxTag, JsRTActionType actionTypeTag)
        : EventLogEntry(EventLogEntry::EventKind::JsRTActionTag, eTime), m_ctxTag(ctxTag), m_actionTypeTag(actionTypeTag)
    {
        ;
    }

    JsRTActionLogEntry::~JsRTActionLogEntry()
    {
        ;
    }

    Js::ScriptContext* JsRTActionLogEntry::GetScriptContextForAction(ThreadContext* threadContext) const
    {
        return threadContext->TTDInfo->LookupObjectForTag(this->m_ctxTag)->GetScriptContext();
    }

    JsRTActionLogEntry* JsRTActionLogEntry::As(EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag, "Not a JsRT action!");

        return static_cast<JsRTActionLogEntry*>(e);
    }

    JsRTActionType JsRTActionLogEntry::GetActionTypeTag() const
    {
        return this->m_actionTypeTag;
    }

    bool JsRTActionLogEntry::IsRootCall()
    {
        return (this->m_actionTypeTag == JsRTActionType::CallExistingFunction && JsRTCallFunctionAction::As(this)->GetCallDepth() == 0);
    }

    JsRTActionLogEntry* JsRTActionLogEntry::CompleteParse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc, int64 eTime)
    {
        TTD_LOG_TAG ctxTag = reader->ReadLogTag(NSTokens::Key::ctxTag, readSeperator);
        JsRTActionType actionTag = reader->ReadTag<JsRTActionType>(NSTokens::Key::jsrtEventKind, true);

        JsRTActionLogEntry* res = nullptr;
        switch(actionTag)
        {
        case JsRTActionType::AllocateNumber:
            res = JsRTNumberAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::VarConvert:
            res = JsRTVarConvertAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetAndClearException:
            res = JsRTGetAndClearExceptionAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetProperty:
            res = JsRTGetPropertyAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CallbackOp:
            res = JsRTCallbackAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CodeParse:
            res = JsRTCodeParseAction::CompleteParse(threadContext, reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CallExistingFunction:
            res = JsRTCallFunctionAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        default:
            AssertMsg(false, "Missing tag in case");
        }

        return res;
    }

    JsRTNumberAllocateAction::JsRTNumberAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool useIntRepresentation, int32 ival, double dval)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateNumber), m_useIntRepresentation(useIntRepresentation)
    {
        if(useIntRepresentation)
        {
            this->u_ival = ival;
        }
        else
        {
            this->u_dval = dval;
        }
    }

    JsRTNumberAllocateAction::~JsRTNumberAllocateAction()
    {
        ;
    }

    void JsRTNumberAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::Var number = this->m_useIntRepresentation ? Js::JavascriptNumber::ToVar(this->u_ival, execContext) : Js::JavascriptNumber::ToVarNoCheck(this->u_dval, execContext);

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(number));
    }

    void JsRTNumberAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_useIntRepresentation, NSTokens::Separator::CommaSeparator);
        if(this->m_useIntRepresentation)
        {
            writer->WriteInt32(NSTokens::Key::i32Val, this->u_ival, NSTokens::Separator::CommaSeparator);
        }
        else
        {
            writer->WriteDouble(NSTokens::Key::doubleVal, this->u_dval, NSTokens::Separator::CommaSeparator);
        }

        writer->WriteRecordEnd();
    }

    JsRTNumberAllocateAction* JsRTNumberAllocateAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool useIntRepresentation = reader->ReadBool(NSTokens::Key::boolVal, true);

        if(useIntRepresentation)
        { 
            int32 ival = reader->ReadInt32(NSTokens::Key::i32Val, true);
            return alloc.SlabNew<JsRTNumberAllocateAction>(eTime, ctxTag, true, ival, 0.0);
        }
        else
        {
            double dval = reader->ReadDouble(NSTokens::Key::doubleVal, true);
            return alloc.SlabNew<JsRTNumberAllocateAction>(eTime, ctxTag, false, 0, dval);
        }
    }

    JsRTVarConvertAction::JsRTVarConvertAction(int64 eTime, TTD_LOG_TAG ctxTag, bool toBool, bool toNumber, bool toString, NSLogValue::ArgRetValue* var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::VarConvert), m_toBool(toBool), m_toNumber(toNumber), m_toString(toString), m_var(var)
    {
        ;
    }

    JsRTVarConvertAction::~JsRTVarConvertAction()
    {
        ;
    }

    void JsRTVarConvertAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var value = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        if(this->m_toBool)
        {
            ; //we always tag the known bools so we don't need to allocate or tag anything here
        }
        else if(this->m_toNumber)
        {
            Js::Var numVal = Js::JavascriptOperators::ToNumber(value, execContext);
            if(TTD::JsSupport::IsVarPtrValued(numVal))
            {
                threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(numVal));

            }
        }
        else if(this->m_toString)
        {
            Js::JavascriptString* strVal = Js::JavascriptConversion::ToString(value, execContext);
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(strVal));
        }
        else
        {
            AssertMsg(false, "Unknown conversion!!!");
        }
    }

    void JsRTVarConvertAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_toBool, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_toNumber, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_toString, NSTokens::Separator::CommaSeparator);

        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTVarConvertAction* JsRTVarConvertAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool toBool = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool toNumber = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool toString = reader->ReadBool(NSTokens::Key::boolVal, true);

        NSLogValue::ArgRetValue* var = alloc.SlabAllocateStruct<NSLogValue::ArgRetValue>();
        NSLogValue::ParseArgRetValue(var, true, reader, alloc);

        return alloc.SlabNew<JsRTVarConvertAction>(eTime, ctxTag, toBool, toNumber, toString, var);
    }

    JsRTGetAndClearExceptionAction::JsRTGetAndClearExceptionAction(int64 eTime, TTD_LOG_TAG ctxTag)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetAndClearException)
    {
        ;
    }

    JsRTGetAndClearExceptionAction::~JsRTGetAndClearExceptionAction()
    {
        ;
    }

    void JsRTGetAndClearExceptionAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        HRESULT hr = S_OK;
        Js::JavascriptExceptionObject *recordedException = nullptr;

        BEGIN_TRANSLATE_OOM_TO_HRESULT
            recordedException = execContext->GetAndClearRecordedException();
        END_TRANSLATE_OOM_TO_HRESULT(hr)

        if(hr == E_OUTOFMEMORY)
        {
            //
            //TODO: we don't have support for OOM yet (and adding support will require a non-trivial amount of work)
            //
            AssertMsg(false, "OOM is not supported");
        }
        if(recordedException == nullptr)
        {
            return;
        }
        Js::Var exception = recordedException->GetThrownObject(nullptr);

        if(exception != nullptr)
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(exception));
        }
    }

    void JsRTGetAndClearExceptionAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteRecordEnd();
    }

    JsRTGetAndClearExceptionAction* JsRTGetAndClearExceptionAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        return alloc.SlabNew<JsRTGetAndClearExceptionAction>(eTime, ctxTag);
    }

    JsRTGetPropertyAction::JsRTGetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, NSLogValue::ArgRetValue* var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetProperty), m_propertyId(pid), m_var(var)
    {
        ;
    }

    JsRTGetPropertyAction::~JsRTGetPropertyAction()
    {
        ;
    }

    void JsRTGetPropertyAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var cvar = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        //if this throws then that is fine (it is what happens in JsRT)
        Js::Var value = Js::JavascriptOperators::OP_GetProperty(cvar, this->m_propertyId, execContext);

        //otherwise tag the return value in case we see it later
        if(TTD::JsSupport::IsVarPtrValued(value))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(value));
        }
    }

    void JsRTGetPropertyAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_propertyId, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTGetPropertyAction* JsRTGetPropertyAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ArgRetValue* var = alloc.SlabAllocateStruct<NSLogValue::ArgRetValue>();
        NSLogValue::ParseArgRetValue(var, false, reader, alloc);

        return alloc.SlabNew<JsRTGetPropertyAction>(eTime, ctxTag, pid, var);
    }

    JsRTCallbackAction::JsRTCallbackAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isCancel, bool isRepeating, int64 currentCallbackId, TTD_LOG_TAG callbackFunctionTag, int64 createdCallbackId)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CallbackOp), m_isCancel(isCancel), m_isRepeating(isRepeating), m_currentCallbackId(currentCallbackId), m_callbackFunctionTag(callbackFunctionTag), m_createdCallbackId(createdCallbackId)
#if ENABLE_TTD_DEBUGGING
        , m_register_eventTime(-1), m_register_ftime(0), m_register_ltime(0), m_register_line(0), m_register_column(0), m_register_sourceId(0)
#endif
    {
        ;
    }

    JsRTCallbackAction::~JsRTCallbackAction()
    {
        ;
    }

    JsRTCallbackAction* JsRTCallbackAction::As(JsRTActionLogEntry* action)
    {
        AssertMsg(action->GetActionTypeTag() == JsRTActionType::CallbackOp, "Need to ensure this is true first");

        return static_cast<JsRTCallbackAction*>(action);
    }

    int64 JsRTCallbackAction::GetAssociatedHostCallbackId() const
    {
        return this->m_createdCallbackId;
    }

    bool JsRTCallbackAction::IsRepeatingOp() const
    {
        return this->m_isRepeating;
    }

    bool JsRTCallbackAction::IsCreateOp() const
    {
        return !this->m_isCancel;
    }

    bool JsRTCallbackAction::IsCancelOp() const
    {
        return this->m_isCancel;
    }

    bool JsRTCallbackAction::GetActionTimeInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const
    {
#if !ENABLE_TTD_DEBUGGING
        return false;
#else
        if(this->m_register_eventTime == -1)
        {
            return false; //we haven't been re-executed in replay so we don't have our info yet
        }

        *rootEventTime = this->m_register_eventTime;
        *ftime = this->m_register_ftime;
        *ltime = this->m_register_ltime;

        *line = this->m_register_line;
        *column = this->m_register_column;
        *sourceId = this->m_register_sourceId;

        return true;
#endif
    }

    void JsRTCallbackAction::ExecuteAction(ThreadContext* threadContext) const
    {
#if !ENABLE_TTD_DEBUGGING
        ; //we don't need to do anything
#else
        if(this->m_register_eventTime == -1)
        {
            threadContext->TTDLog->GetTimeAndPositionForDebugger(&this->m_register_eventTime, &this->m_register_ftime, &this->m_register_ltime, &this->m_register_line, &this->m_register_column, &this->m_register_sourceId);
        }
#endif
    }

    void JsRTCallbackAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_isCancel, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_isRepeating, NSTokens::Separator::CommaSeparator);

        writer->WriteInt64(NSTokens::Key::hostCallbackId, this->m_currentCallbackId, NSTokens::Separator::CommaSeparator);
        writer->WriteLogTag(NSTokens::Key::logTag, this->m_callbackFunctionTag, NSTokens::Separator::CommaSeparator);
        writer->WriteInt64(NSTokens::Key::newCallbackId, this->m_createdCallbackId, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTCallbackAction* JsRTCallbackAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isCancel = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool isRepeating = reader->ReadBool(NSTokens::Key::boolVal, true);

        int64 currentCallbackId = reader->ReadInt64(NSTokens::Key::hostCallbackId, true);
        TTD_LOG_TAG callbackFunctionTag = reader->ReadLogTag(NSTokens::Key::logTag, true);
        int64 createdCallbackId = reader->ReadInt64(NSTokens::Key::newCallbackId, true);

        return alloc.SlabNew<JsRTCallbackAction>(eTime, ctxTag, isCancel, isRepeating, currentCallbackId, callbackFunctionTag, createdCallbackId);
    }

    JsRTCodeParseAction::JsRTCodeParseAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isExpression, LPCWSTR sourceCode, DWORD_PTR optDocumentId, LPCWSTR optSourceUri, LPCWSTR srcDir)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CodeParse), m_isExpression(isExpression), m_sourceCode(sourceCode), m_optDocumentID(optDocumentId), m_optSourceUri(optSourceUri), m_srcDir(srcDir)
    {
        ;
    }

    JsRTCodeParseAction::~JsRTCodeParseAction()
    {
        ;
    }

    void JsRTCodeParseAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::JavascriptFunction* function = nullptr;

        DWORD_PTR sourceContext = this->m_optDocumentID;
        SourceContextInfo * sourceContextInfo = execContext->GetSourceContextInfo(sourceContext, nullptr);
        if(sourceContextInfo == nullptr)
        {
            sourceContextInfo = execContext->CreateSourceContextInfo(sourceContext, this->m_optSourceUri, wcslen(this->m_optSourceUri), nullptr);
        }

        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(wcslen(this->m_sourceCode)), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };

        Js::Utf8SourceInfo* utf8SourceInfo;
        CompileScriptException se;
        BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(execContext)
        {
            function = execContext->LoadScript(this->m_sourceCode, &si, &se, this->m_isExpression /*isExpression*/, false /*disableDeferredParse*/, false /*isByteCodeBufferForLibrary*/, &utf8SourceInfo, Js::Constants::GlobalCode);
        }
        END_LEAVE_SCRIPT_WITH_EXCEPTION(execContext);
        AssertMsg(function != nullptr, "Something went wrong");

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(function);
    }

    void JsRTCodeParseAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::isExpression, this->m_isExpression, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt64(NSTokens::Key::documentId, (uint64)this->m_optDocumentID, NSTokens::Separator::CommaSeparator);
        writer->WriteString(NSTokens::Key::uri, this->m_optSourceUri, NSTokens::Separator::CommaSeparator);
        writer->WriteString(NSTokens::Key::logDir, this->m_srcDir, NSTokens::Separator::CommaSeparator);

        LPCWSTR docId = writer->FormatNumber(this->m_optDocumentID);
        HANDLE srcStream = threadContext->TTDStreamFunctions.pfGetSrcCodeStream(this->m_srcDir, docId, this->m_optSourceUri, false, true);

        JSONWriter srcWriter(srcStream, threadContext->TTDStreamFunctions.pfWriteBytesToStream, threadContext->TTDStreamFunctions.pfFlushAndCloseStream);
        srcWriter.WriteRawString(this->m_sourceCode);

        writer->WriteRecordEnd();
    }

    JsRTCodeParseAction* JsRTCodeParseAction::CompleteParse(ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        LPCWSTR srcCode = nullptr;

        bool isExpression = reader->ReadBool(NSTokens::Key::isExpression, true);
        DWORD_PTR documentId = (DWORD_PTR)reader->ReadUInt64(NSTokens::Key::documentId, true);
        LPCWSTR srcUri = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::uri, true));
        LPCWSTR srcDir = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::logDir, true));

        LPCWSTR docId = reader->FormatNumber(documentId);
        HANDLE srcStream = threadContext->TTDStreamFunctions.pfGetSrcCodeStream(srcDir, docId, srcUri, true, false);

        JSONReader srcReader(srcStream, threadContext->TTDStreamFunctions.pfReadBytesFromStream, threadContext->TTDStreamFunctions.pfFlushAndCloseStream);
        srcCode = srcReader.ReadRawString(alloc);

        return alloc.SlabNew<JsRTCodeParseAction>(eTime, ctxTag, isExpression, srcCode, documentId, srcUri, srcDir);
    }

    JsRTCallFunctionAction::JsRTCallFunctionAction(int64 eTime, TTD_LOG_TAG ctxTag, int32 callbackDepth, int64 hostCallbackId, double beginTime, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CallExistingFunction), m_callbackDepth(callbackDepth), m_hostCallbackId(hostCallbackId), m_beginTime(beginTime), m_elapsedTime(0),
        m_functionTagId(functionTagId), m_argCount(argCount), m_argArray(argArray), m_execArgs(execArgs)
#if ENABLE_TTD_DEBUGGING
        , m_rtrSnap(nullptr), m_rtrRestoreLogTag(TTD_INVALID_LOG_TAG), m_rtrRestoreIdentityTag(TTD_INITIAL_IDENTITY_TAG)
        , m_lastExecuted_valid(false), m_lastExecuted_ftime(0), m_lastExecuted_ltime(0), m_lastExecuted_line(0), m_lastExecuted_column(0), m_lastExecuted_sourceId(0)
#endif
    {
        ;
    }

    JsRTCallFunctionAction::~JsRTCallFunctionAction()
    {
#if ENABLE_TTD_DEBUGGING
        if(this->m_rtrSnap != nullptr)
        {
            HeapDelete(this->m_rtrSnap);

            this->m_rtrSnap = nullptr;
            this->m_rtrRestoreLogTag = TTD_INVALID_LOG_TAG;
            this->m_rtrRestoreIdentityTag = TTD_INVALID_IDENTITY_TAG;
        }
#endif
    }

    void JsRTCallFunctionAction::UnloadSnapshot() const
    {
#if ENABLE_TTD_DEBUGGING
        if(this->m_rtrSnap != nullptr)
        {
            HeapDelete(this->m_rtrSnap);

            this->m_rtrSnap = nullptr;
            this->m_rtrRestoreLogTag = TTD_INVALID_LOG_TAG;
            this->m_rtrRestoreIdentityTag = TTD_INVALID_IDENTITY_TAG;
        }
#endif
    }

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
    void JsRTCallFunctionAction::SetFunctionName(LPCWSTR fname)
    {
        this->m_functionName = fname;
    }
#endif

    JsRTCallFunctionAction* JsRTCallFunctionAction::As(JsRTActionLogEntry* action)
    {
        AssertMsg(action->GetActionTypeTag() == JsRTActionType::CallExistingFunction, "Need to ensure this is true first");

        return static_cast<JsRTCallFunctionAction*>(action);
    }

    bool JsRTCallFunctionAction::HasReadyToRunSnapshotInfo() const
    {
#if !ENABLE_TTD_DEBUGGING
        return false;
#else
        return this->m_rtrSnap != nullptr;
#endif
    }

    void JsRTCallFunctionAction::GetReadyToRunSnapshotInfo(SnapShot** snap, TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const
    {
#if !ENABLE_TTD_DEBUGGING
        *snap = nullptr;
        *logTag = TTD_INVALID_LOG_TAG;
        *identityTag = TTD_INVALID_IDENTITY_TAG;
#else
        AssertMsg(this->m_rtrSnap != nullptr, "Does not have rtr info!!");

        *snap = this->m_rtrSnap;
        *logTag = this->m_rtrRestoreLogTag;
        *identityTag = this->m_rtrRestoreIdentityTag;
#endif
    }

    void JsRTCallFunctionAction::SetReadyToRunSnapshotInfo(SnapShot* snap, TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag) const
    {
#if ENABLE_TTD_DEBUGGING
        AssertMsg(this->m_rtrSnap == nullptr, "We already have a rtr snapshot!!!");

        this->m_rtrSnap = snap;
        this->m_rtrRestoreLogTag = logTag;
        this->m_rtrRestoreIdentityTag = identityTag;
#endif
    }

    void JsRTCallFunctionAction::ClearLastExecutedStatementAndFrameInfo() const
    {
#if ENABLE_TTD_DEBUGGING
        this->m_lastExecuted_valid = false;
        this->m_lastExecuted_ftime = 0;
        this->m_lastExecuted_ltime = 0;
        this->m_lastExecuted_line = 0;
        this->m_lastExecuted_column = 0;
        this->m_lastExecuted_sourceId = 0;
#endif
    }

    void JsRTCallFunctionAction::SetLastExecutedStatementAndFrameInfo(const SingleCallCounter& lastFrame) const
    {
#if ENABLE_TTD_DEBUGGING
        this->m_lastExecuted_valid = true;
        this->m_lastExecuted_ftime = lastFrame.FunctionTime;
        this->m_lastExecuted_ltime = lastFrame.LoopTime;

        LPCUTF8 srcBegin = nullptr;
        LPCUTF8 srcEnd = nullptr;
        ULONG srcLine = 0;
        LONG srcColumn = -1;
        uint32 startOffset = lastFrame.Function->GetStatementStartOffset(lastFrame.CurrentStatementIndex);
        lastFrame.Function->GetSourceLineFromStartOffset_TTD(startOffset, &srcBegin, &srcEnd, &srcLine, &srcColumn);

        this->m_lastExecuted_line = (uint32)srcLine;
        this->m_lastExecuted_column = (uint32)srcColumn;
        this->m_lastExecuted_sourceId = lastFrame.CurrentStatementIndex;
#endif
    }

    bool JsRTCallFunctionAction::GetLastExecutedStatementAndFrameInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const
    {
#if !ENABLE_TTD_DEBUGGING
        return false;
#else
        if(!this->m_lastExecuted_valid)
        {
            return false;
        }

        *rootEventTime = this->GetEventTime();
        *ftime = this->m_lastExecuted_ftime;
        *ltime = this->m_lastExecuted_ltime;

        *line = this->m_lastExecuted_line;
        *column = this->m_lastExecuted_column;
        *sourceId = this->m_lastExecuted_sourceId;

        return true;
#endif
    }

    int32 JsRTCallFunctionAction::GetCallDepth() const
    {
        return this->m_callbackDepth;
    }

    int64 JsRTCallFunctionAction::GetHostCallbackId() const
    {
        AssertMsg(this->m_callbackDepth == 0, "This should be a root call!!!");

        return this->m_hostCallbackId;
    }

    void JsRTCallFunctionAction::SetElapsedTime(double elapsedTime)
    {
        this->m_elapsedTime = elapsedTime;
    }

    void JsRTCallFunctionAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::RecyclableObject* fobj = execContext->GetThreadContext()->TTDInfo->LookupObjectForTag(this->m_functionTagId);
        Js::JavascriptFunction *jsFunction = Js::JavascriptFunction::FromVar(fobj);

        Js::CallInfo callInfo((ushort)this->m_argCount);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            const NSLogValue::ArgRetValue* aval = (this->m_argArray + i);
            Js::Var avar = NSLogValue::InflateArgRetValueIntoVar(aval, execContext);

            this->m_execArgs[i] = avar;
        }
        Js::Arguments jsArgs(callInfo, this->m_execArgs);
        if(this->m_callbackDepth == 0)
        {
            threadContext->TTDLog->ResetCallStackForTopLevelCall(this->GetEventTime(), this->m_hostCallbackId);
        }

        this->ClearLastExecutedStatementAndFrameInfo();

        bool abortIfTopLevel = false;
        Js::Var result = nullptr;
        try
        {
            result = jsFunction->CallRootFunction(jsArgs, execContext, true);
        }
        catch(Js::JavascriptExceptionObject*  exceptionObject)
        {
            AssertMsg(threadContext->GetRecordedException() == nullptr, "Not sure if this is true or not but seems like a reasonable requirement.");

            threadContext->SetRecordedException(exceptionObject);
            abortIfTopLevel = true;
        }
        catch(Js::ScriptAbortException)
        {
            AssertMsg(threadContext->GetRecordedException() == nullptr, "Not sure if this is true or not but seems like a reasonable requirement.");

            threadContext->SetRecordedException(threadContext->GetPendingTerminatedErrorObject());
            abortIfTopLevel = true;
        }
        catch(TTDebuggerAbortException)
        {
            throw; //re-throw my abort exception up to the top-level.
        }
        catch(...)
        {
            AssertMsg(false, "What else if dying here?");

            //not sure of our best strategy so just run for now
        }

#if ENABLE_TTD_DEBUGGING
        if(this->m_callbackDepth == 0)
        {
            if(threadContext->TTDLog->HasImmediateReturnFrame())
            {
                this->SetLastExecutedStatementAndFrameInfo(threadContext->TTDLog->GetImmediateReturnFrame());
            }
            else
            {
                this->SetLastExecutedStatementAndFrameInfo(threadContext->TTDLog->GetImmediateExceptionFrame());
            }

            if(abortIfTopLevel)
            {
                throw TTDebuggerAbortException::CreateUncaughtExceptionAbortRequest(threadContext->TTDLog->GetCurrentTopLevelEventTime(), L"Uncaught exception -- Propagate to top-level.");
            }
        }
#endif

        //since we tag in JsRT we need to tag here too
        if(result != nullptr && JsSupport::IsVarPtrValued(result))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(result));
        }
    }

    void JsRTCallFunctionAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_callbackDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteInt64(NSTokens::Key::hostCallbackId, this->m_hostCallbackId, NSTokens::Separator::CommaSeparator);
        writer->WriteLogTag(NSTokens::Key::logTag, this->m_functionTagId, NSTokens::Separator::CommaSeparator);

        writer->WriteDouble(NSTokens::Key::beginTime, this->m_beginTime, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::elapsedTime, this->m_elapsedTime, NSTokens::Separator::CommaSeparator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        writer->WriteString(NSTokens::Key::name, this->m_functionName, NSTokens::Separator::CommaSeparator);
#endif

        writer->WriteLengthValue(this->m_argCount, NSTokens::Separator::CommaSeparator);

        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
            NSLogValue::EmitArgRetValue(this->m_argArray + i, writer, sep);
        }
        writer->WriteSequenceEnd();

        writer->WriteRecordEnd();
    }

    JsRTCallFunctionAction* JsRTCallFunctionAction::CompleteParse(FileReader* reader, SlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        int32 callbackDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        int64 hostCallbackId = reader->ReadInt64(NSTokens::Key::hostCallbackId, true);
        TTD_LOG_TAG ftag = reader->ReadLogTag(NSTokens::Key::logTag, true);

        double beginTime = reader->ReadDouble(NSTokens::Key::beginTime, true);
        double elapsedTime = reader->ReadDouble(NSTokens::Key::elapsedTime, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        LPCWSTR fname = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::name, true));
#endif

        uint32 argc = reader->ReadLengthValue(true);
        NSLogValue::ArgRetValue* args = (argc != 0) ? alloc.SlabAllocateArray<NSLogValue::ArgRetValue>(argc) : nullptr;

        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < argc; ++i)
        {
            NSLogValue::ParseArgRetValue(args + i, i != 0, reader, alloc);
        }
        reader->ReadSequenceEnd();

        Js::Var* execArgs = (argc != 0) ? alloc.SlabAllocateArray<Js::Var>(argc) : nullptr;

        JsRTCallFunctionAction* res = alloc.SlabNew<JsRTCallFunctionAction>(eTime, ctxTag, callbackDepth, hostCallbackId, beginTime, ftag, argc, args, execArgs);
        res->SetElapsedTime(elapsedTime);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->SetFunctionName(fname);
#endif

        return res;
    }
}

#endif
