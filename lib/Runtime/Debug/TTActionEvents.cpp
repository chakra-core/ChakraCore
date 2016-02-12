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

    Js::ScriptContext* JsRTActionLogEntry::GetScriptContextForAction(ThreadContext* threadContext) const
    {
        return threadContext->TTDInfo->LookupObjectForTag(this->m_ctxTag)->GetScriptContext();
    }

    const JsRTActionLogEntry* JsRTActionLogEntry::As(const EventLogEntry* e)
    {
        AssertMsg(e->GetEventKind() == EventLogEntry::EventKind::JsRTActionTag, "Not a JsRT action!");

        return static_cast<const JsRTActionLogEntry*>(e);
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

    bool JsRTActionLogEntry::IsRootCallBegin()
    {
        return (this->m_actionTypeTag == JsRTActionType::CallExistingFunctionBegin && JsRTCallFunctionBeginAction::As(this)->GetCallDepth() == 0);
    }

    bool JsRTActionLogEntry::IsRootCallEnd()
    {
        return (this->m_actionTypeTag == JsRTActionType::CallExistingFunctionEnd && JsRTCallFunctionBeginAction::As(this)->GetCallDepth() == 0);
    }

    bool JsRTActionLogEntry::IsExecutedInScriptWrapper() const
    {
        return true;
    }

    JsRTActionLogEntry* JsRTActionLogEntry::CompleteParse(bool readSeperator, ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime)
    {
        TTD_LOG_TAG ctxTag = reader->ReadLogTag(NSTokens::Key::ctxTag, readSeperator);
        JsRTActionType actionTag = reader->ReadTag<JsRTActionType>(NSTokens::Key::jsrtEventKind, true);

        JsRTActionLogEntry* res = nullptr;
        switch(actionTag)
        {
        case JsRTActionType::AllocateNumber:
            res = JsRTNumberAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateString:
            res = JsRTStringAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateSymbol:
            res = JsRTSymbolAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::VarConvert:
            res = JsRTVarConvertAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateObject:
            res = JsRTObjectAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateArray:
            res = JsRTArrayAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateArrayBuffer:
            res = JsRTArrayBufferAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::AllocateFunction:
            res = JsRTFunctionAllocateAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetAndClearException:
            res = JsRTGetAndClearExceptionAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetProperty:
            res = JsRTGetPropertyAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetIndex:
            res = JsRTGetIndexAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetOwnPropertyInfo:
            res = JsRTGetOwnPropertyInfoAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetOwnPropertiesInfo:
            res = JsRTGetOwnPropertiesInfoAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::DefineProperty:
            res = JsRTDefinePropertyAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::DeleteProperty:
            res = JsRTDeletePropertyAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::SetPrototype:
            res = JsRTSetPrototypeAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::SetProperty:
            res = JsRTSetPropertyAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::SetIndex:
            res = JsRTSetIndexAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::GetTypedArrayInfo:
            res = JsRTGetTypedArrayInfoAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::ConstructCall:
            res = JsRTConstructCallAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CallbackOp:
            res = JsRTCallbackAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CodeParse:
            res = JsRTCodeParseAction::CompleteParse(threadContext, reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CallExistingFunctionBegin:
            res = JsRTCallFunctionBeginAction::CompleteParse(reader, alloc, eTime, ctxTag);
            break;
        case JsRTActionType::CallExistingFunctionEnd:
            res = JsRTCallFunctionEndAction::CompleteParse(reader, alloc, eTime, ctxTag);
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

    JsRTNumberAllocateAction* JsRTNumberAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
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

    JsRTStringAllocateAction::JsRTStringAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, const TTString& stringValue)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateString), m_stringValue(stringValue)
    {
        ;
    }

    void JsRTStringAllocateAction::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        if(!IsNullPtrTTString(this->m_stringValue))
        {
            alloc.UnlinkString(this->m_stringValue);
        }
    }

    void JsRTStringAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::JavascriptString* str = Js::JavascriptString::NewCopyBuffer(this->m_stringValue.Contents, this->m_stringValue.Length, execContext);

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(str));
    }

    void JsRTStringAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteString(NSTokens::Key::stringVal, this->m_stringValue, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTStringAllocateAction* JsRTStringAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        TTString str;
        reader->ReadString(NSTokens::Key::stringVal, alloc, str, true);

        return alloc.SlabNew<JsRTStringAllocateAction>(eTime, ctxTag, str);
    }

    JsRTSymbolAllocateAction::JsRTSymbolAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& symbolDescription)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateSymbol), m_symbolDescription(symbolDescription)
    {
        ;
    }

    void JsRTSymbolAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var description = NSLogValue::InflateArgRetValueIntoVar(this->m_symbolDescription, execContext);

        Js::JavascriptString* descriptionString = nullptr;
        if(description != nullptr)
        {
            descriptionString = Js::JavascriptConversion::ToString(description, execContext);
        }
        else
        {
            descriptionString = execContext->GetLibrary()->GetEmptyString();
        }

        Js::RecyclableObject* sym = execContext->GetLibrary()->CreatePrimitveSymbol_TTD(descriptionString);

        threadContext->TTDInfo->TrackTagObject(sym);
    }

    void JsRTSymbolAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_symbolDescription, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTSymbolAllocateAction* JsRTSymbolAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue symDescription;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(symDescription, false, reader);

        return alloc.SlabNew<JsRTSymbolAllocateAction>(eTime, ctxTag, symDescription);
    }

    JsRTVarConvertAction::JsRTVarConvertAction(int64 eTime, TTD_LOG_TAG ctxTag, bool toBool, bool toNumber, bool toString, bool toObject, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::VarConvert), m_toBool(toBool), m_toNumber(toNumber), m_toString(toString), m_toObject(toObject), m_var(var)
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
        else if(this->m_toObject)
        {
            Js::Var objVal = Js::JavascriptOperators::ToObject((Js::Var)value, execContext);

            if(TTD::JsSupport::IsVarPtrValued(objVal))
            {
                threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(objVal));

            }
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
        writer->WriteBool(NSTokens::Key::boolVal, this->m_toObject, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTVarConvertAction* JsRTVarConvertAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool toBool = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool toNumber = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool toString = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool toObject = reader->ReadBool(NSTokens::Key::boolVal, true);

        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTVarConvertAction>(eTime, ctxTag, toBool, toNumber, toString, toObject, var);
    }

    JsRTObjectAllocateAction::JsRTObjectAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isRegularObject)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateObject), m_isRegularObject(isRegularObject)
    {
        ;
    }

    void JsRTObjectAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::RecyclableObject* res = nullptr;
        if(this->m_isRegularObject)
        {
            res = execContext->GetLibrary()->CreateObject();
        }
        else
        {
            //
            //In replay we put regular dynamic objects in place of external objects -- doesn't matter since we fake all external interaction
            //
            res = execContext->GetLibrary()->CreateObject();
        }

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(res);
    }

    void JsRTObjectAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_isRegularObject, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTObjectAllocateAction* JsRTObjectAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isRegularObject = reader->ReadBool(NSTokens::Key::boolVal, true);

        return alloc.SlabNew<JsRTObjectAllocateAction>(eTime, ctxTag, isRegularObject);
    }

    JsRTArrayAllocateAction::JsRTArrayAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::TypeId arrayType, uint32 length)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateArray), m_arrayType(arrayType), m_length(length)
    {
        ;
    }

    void JsRTArrayAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::Var res = nullptr;
        switch(this->m_arrayType)
        {
        case Js::TypeIds_Array:
            res = execContext->GetLibrary()->CreateArray(this->m_length);
            break;
        default:
            AssertMsg(false, "Unknown Array Kind for JsRT allocate array action!");
        }
        AssertMsg(res != nullptr, "Something went very wrong");

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(res));
    }

    void JsRTArrayAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteTag<Js::TypeId>(NSTokens::Key::jsTypeId, this->m_arrayType, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt32(NSTokens::Key::u32Val, this->m_length, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTArrayAllocateAction* JsRTArrayAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        Js::TypeId arrayType = reader->ReadTag<Js::TypeId>(NSTokens::Key::jsTypeId, true);
        uint32 length = reader->ReadUInt32(NSTokens::Key::u32Val, true);

        return alloc.SlabNew<JsRTArrayAllocateAction>(eTime, ctxTag, arrayType, length);
    }

    JsRTArrayBufferAllocateAction::JsRTArrayBufferAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, uint32 bufferSize, byte* bufferData)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateArrayBuffer), m_bufferSize(bufferSize), m_bufferData(bufferData)
    {
        ;
    }

    void JsRTArrayBufferAllocateAction::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        alloc.UnlinkAllocation(this->m_bufferData);
    }

    void JsRTArrayBufferAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::ArrayBuffer* abuff = execContext->GetLibrary()->CreateArrayBuffer(this->m_bufferSize);
        AssertMsg(abuff->GetByteLength() == this->m_bufferSize, "Something is wrong with our sizes.");

        js_memcpy_s(abuff->GetBuffer(), abuff->GetByteLength(), this->m_bufferData, this->m_bufferSize);

        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(abuff));
    }

    void JsRTArrayBufferAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteLengthValue(this->m_bufferSize, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        for(uint32 i = 0; i < this->m_bufferSize; ++i)
        {
            writer->WriteNakedByte(this->m_bufferData[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
        }
        writer->WriteSequenceEnd();

        writer->WriteRecordEnd();
    }

    JsRTArrayBufferAllocateAction* JsRTArrayBufferAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        uint32 bufferSize = reader->ReadLengthValue(true);
        byte* bufferData = alloc.SlabAllocateArray<byte>(bufferSize);
        reader->ReadRecordStart_WDefaultKey(true);
        for(uint32 i = 0; i < bufferSize; ++i)
        {
            bufferData[i] = reader->ReadNakedByte(i != 0);
        }
        reader->ReadSequenceEnd();

        return alloc.SlabNew<JsRTArrayBufferAllocateAction>(eTime, ctxTag, bufferSize, bufferData);
    }

    JsRTFunctionAllocateAction::JsRTFunctionAllocateAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isNamed, const NSLogValue::ArgRetValue& name)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::AllocateFunction), m_isNamed(isNamed), m_name(name)
    {
        ;
    }

    void JsRTFunctionAllocateAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::Var res = nullptr;

        if(!this->m_isNamed)
        {
            res = execContext->GetLibrary()->CreateStdCallExternalFunction(nullptr, 0, nullptr);
        }
        else
        {
            Js::Var nameVar = NSLogValue::InflateArgRetValueIntoVar(this->m_name, execContext);

            Js::JavascriptString* name = nullptr;
            if(nameVar != nullptr)
            {
                name = Js::JavascriptConversion::ToString(name, execContext);
            }
            else
            {
                name = execContext->GetLibrary()->GetEmptyString();
            }

            res = execContext->GetLibrary()->CreateStdCallExternalFunction(nullptr, Js::JavascriptString::FromVar(name), nullptr);
        }

        //since we tag in JsRT we need to tag here too
        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(res));
    }

    void JsRTFunctionAllocateAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_isNamed, NSTokens::Separator::CommaSeparator);
        if(this->m_isNamed)
        {
            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            NSLogValue::EmitArgRetValue(this->m_name, writer, NSTokens::Separator::NoSeparator);
        }

        writer->WriteRecordEnd();
    }

    JsRTFunctionAllocateAction* JsRTFunctionAllocateAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isNamed = reader->ReadBool(NSTokens::Key::boolVal, true);
        NSLogValue::ArgRetValue name;
        name.Tag = NSLogValue::ArgRetValueTag::Invalid;

        if(isNamed)
        {
            NSLogValue::ArgRetValue name;
            reader->ReadKey(NSTokens::Key::entry, true);
            NSLogValue::ParseArgRetValue(name, false, reader);
        }

        return alloc.SlabNew<JsRTFunctionAllocateAction>(eTime, ctxTag, isNamed, name);
    }

    JsRTGetAndClearExceptionAction::JsRTGetAndClearExceptionAction(int64 eTime, TTD_LOG_TAG ctxTag)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetAndClearException)
    {
        ;
    }

    bool JsRTGetAndClearExceptionAction::IsExecutedInScriptWrapper() const
    {
        return false;
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

    JsRTGetAndClearExceptionAction* JsRTGetAndClearExceptionAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        return alloc.SlabNew<JsRTGetAndClearExceptionAction>(eTime, ctxTag);
    }

    JsRTGetPropertyAction::JsRTGetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetProperty), m_propertyId(pid), m_var(var)
    {
        ;
    }

    void JsRTGetPropertyAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var cvar = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

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

    JsRTGetPropertyAction* JsRTGetPropertyAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTGetPropertyAction>(eTime, ctxTag, pid, var);
    }

    JsRTGetIndexAction::JsRTGetIndexAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& index, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetIndex), m_index(index), m_var(var)
    {
        ;
    }

    void JsRTGetIndexAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::Var index = NSLogValue::InflateArgRetValueIntoVar(this->m_index, execContext);
        Js::Var cvar = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        Js::Var value = Js::JavascriptOperators::OP_GetElementI(cvar, index, execContext);

        //otherwise tag the return value in case we see it later
        if(TTD::JsSupport::IsVarPtrValued(value))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(value));
        }
    }

    void JsRTGetIndexAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::index, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_index, writer, NSTokens::Separator::NoSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTGetIndexAction* JsRTGetIndexAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue index;
        NSLogValue::ArgRetValue var;

        reader->ReadKey(NSTokens::Key::index, true);
        NSLogValue::ParseArgRetValue(index, false, reader);

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTGetIndexAction>(eTime, ctxTag, index, var);
    }

    JsRTGetOwnPropertyInfoAction::JsRTGetOwnPropertyInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, Js::PropertyId pid, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetOwnPropertyInfo), m_propertyId(pid), m_var(var)
    {
        ;
    }

    void JsRTGetOwnPropertyInfoAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var cvar = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        Js::Var res = nullptr;
        Js::PropertyDescriptor propertyDescriptorValue;
        if(Js::JavascriptOperators::GetOwnPropertyDescriptor(Js::RecyclableObject::FromVar(cvar), this->m_propertyId, execContext, &propertyDescriptorValue))
        {
            res = Js::JavascriptOperators::FromPropertyDescriptor(propertyDescriptorValue, execContext);
        }
        else
        {
            res = execContext->GetLibrary()->GetUndefined();
        }

        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(res));
    }

    void JsRTGetOwnPropertyInfoAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_propertyId, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTGetOwnPropertyInfoAction* JsRTGetOwnPropertyInfoAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTGetOwnPropertyInfoAction>(eTime, ctxTag, pid, var);
    }

    JsRTGetOwnPropertiesInfoAction::JsRTGetOwnPropertiesInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isGetNames, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetOwnPropertiesInfo), m_isGetNames(isGetNames), m_var(var)
    {
        ;
    }

    void JsRTGetOwnPropertiesInfoAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var cvar = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        Js::Var res = nullptr;
        if(this->m_isGetNames)
        {
            res = Js::JavascriptOperators::GetOwnPropertyNames(cvar, execContext);
        }
        else
        {
            res = Js::JavascriptOperators::GetOwnPropertySymbols(cvar, execContext);
        }

        threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(res));
    }

    void JsRTGetOwnPropertiesInfoAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_isGetNames, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTGetOwnPropertiesInfoAction* JsRTGetOwnPropertiesInfoAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isGetNames = reader->ReadBool(NSTokens::Key::boolVal, true);

        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTGetOwnPropertiesInfoAction>(eTime, ctxTag, isGetNames, var);
    }

    JsRTDefinePropertyAction::JsRTDefinePropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, const NSLogValue::ArgRetValue& propertyDescriptor)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::DefineProperty), m_propertyId(pid), m_var(var), m_propertyDescriptor(propertyDescriptor)
    {
        ;
    }

    void JsRTDefinePropertyAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var object = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);
        Js::Var propertyDescriptor = NSLogValue::InflateArgRetValueIntoVar(this->m_propertyDescriptor, execContext);

        Js::PropertyDescriptor propertyDescriptorValue;
        Js::JavascriptOperators::ToPropertyDescriptor(propertyDescriptor, &propertyDescriptorValue, execContext);

        Js::JavascriptOperators::DefineOwnPropertyDescriptor(Js::RecyclableObject::FromVar(object), this->m_propertyId, propertyDescriptorValue, true, execContext);
    }

    void JsRTDefinePropertyAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_propertyId, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_propertyDescriptor, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTDefinePropertyAction* JsRTDefinePropertyAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue var;
        NSLogValue::ArgRetValue value;

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(value, false, reader);

        return alloc.SlabNew<JsRTDefinePropertyAction>(eTime, ctxTag, var, pid, value);
    }

    JsRTDeletePropertyAction::JsRTDeletePropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, bool useStrictRules)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::DeleteProperty), m_propertyId(pid), m_var(var), m_useStrictRules(useStrictRules)
    {
        ;
    }

    void JsRTDeletePropertyAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var var = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        Js::Var value = Js::JavascriptOperators::OP_DeleteProperty(var, this->m_propertyId, execContext, this->m_useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);

        //otherwise tag the return value in case we see it later
        if(TTD::JsSupport::IsVarPtrValued(value))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(value));
        }
    }

    void JsRTDeletePropertyAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_propertyId, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_useStrictRules, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTDeletePropertyAction* JsRTDeletePropertyAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        bool useStrictRules = reader->ReadBool(NSTokens::Key::boolVal, true);

        return alloc.SlabNew<JsRTDeletePropertyAction>(eTime, ctxTag, var, pid, useStrictRules);
    }

    JsRTSetPrototypeAction::JsRTSetPrototypeAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, const NSLogValue::ArgRetValue& proto)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::SetPrototype), m_var(var), m_proto(proto)
    {
        ;
    }

    void JsRTSetPrototypeAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var var = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);
        Js::Var proto = NSLogValue::InflateArgRetValueIntoVar(this->m_proto, execContext);

        Js::JavascriptObject::ChangePrototype(Js::RecyclableObject::FromVar(var), Js::RecyclableObject::FromVar(proto), true, execContext);
    }

    void JsRTSetPrototypeAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_proto, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTSetPrototypeAction* JsRTSetPrototypeAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue var;
        NSLogValue::ArgRetValue proto;

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        reader->ReadKey(NSTokens::Key::argRetVal, true);
        NSLogValue::ParseArgRetValue(proto, false, reader);

        return alloc.SlabNew<JsRTSetPrototypeAction>(eTime, ctxTag, var, proto);
    }

    JsRTSetPropertyAction::JsRTSetPropertyAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, Js::PropertyId pid, const NSLogValue::ArgRetValue& value, bool useStrictRules)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::SetProperty), m_propertyId(pid), m_var(var), m_value(value), m_useStrictRules(useStrictRules)
    {
        ;
    }

    void JsRTSetPropertyAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var var = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);
        Js::Var value = NSLogValue::InflateArgRetValueIntoVar(this->m_value, execContext);

        Js::JavascriptOperators::OP_SetProperty(var, this->m_propertyId, value, execContext, nullptr, this->m_useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);
    }

    void JsRTSetPropertyAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteUInt32(NSTokens::Key::propertyId, this->m_propertyId, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_value, writer, NSTokens::Separator::NoSeparator);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_useStrictRules, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTSetPropertyAction* JsRTSetPropertyAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue var;
        NSLogValue::ArgRetValue value;

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        Js::PropertyId pid = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::propertyId, true);

        reader->ReadKey(NSTokens::Key::argRetVal, true);
        NSLogValue::ParseArgRetValue(value, false, reader);

        bool useStrictRules = reader->ReadBool(NSTokens::Key::boolVal, true);

        return alloc.SlabNew<JsRTSetPropertyAction>(eTime, ctxTag, var, pid, value, useStrictRules);
    }

    JsRTSetIndexAction::JsRTSetIndexAction(int64 eTime, TTD_LOG_TAG ctxTag, const NSLogValue::ArgRetValue& var, const NSLogValue::ArgRetValue& index, const NSLogValue::ArgRetValue& val)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::SetIndex), m_var(var), m_index(index), m_value(val)
    {
        ;
    }

    void JsRTSetIndexAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var var = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);
        Js::Var index = NSLogValue::InflateArgRetValueIntoVar(this->m_index, execContext);
        Js::Var value = NSLogValue::InflateArgRetValueIntoVar(this->m_value, execContext);

        Js::JavascriptOperators::OP_SetElementI(var, index, value, execContext);
    }

    void JsRTSetIndexAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteKey(NSTokens::Key::index, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_index, writer, NSTokens::Separator::NoSeparator);

        writer->WriteKey(NSTokens::Key::argRetVal, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_value, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTSetIndexAction* JsRTSetIndexAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        NSLogValue::ArgRetValue var;
        NSLogValue::ArgRetValue index;
        NSLogValue::ArgRetValue value;

        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        reader->ReadKey(NSTokens::Key::index, true);
        NSLogValue::ParseArgRetValue(index, false, reader);

        reader->ReadKey(NSTokens::Key::argRetVal, true);
        NSLogValue::ParseArgRetValue(value, false, reader);

        return alloc.SlabNew<JsRTSetIndexAction>(eTime, ctxTag, var, index, value);
    }

    JsRTGetTypedArrayInfoAction::JsRTGetTypedArrayInfoAction(int64 eTime, TTD_LOG_TAG ctxTag, bool returnsArrayBuff, const NSLogValue::ArgRetValue& var)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::GetTypedArrayInfo), m_returnsArrayBuff(returnsArrayBuff), m_var(var)
    {
        ;
    }

    void JsRTGetTypedArrayInfoAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::Var var = NSLogValue::InflateArgRetValueIntoVar(this->m_var, execContext);

        if(this->m_returnsArrayBuff)
        {
            Js::TypedArrayBase* typedArrayBase = Js::TypedArrayBase::FromVar(var);
            threadContext->TTDInfo->TrackTagObject(typedArrayBase->GetArrayBuffer());
        }
    }

    void JsRTGetTypedArrayInfoAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteBool(NSTokens::Key::boolVal, this->m_returnsArrayBuff, NSTokens::Separator::CommaSeparator);

        writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
        NSLogValue::EmitArgRetValue(this->m_var, writer, NSTokens::Separator::NoSeparator);

        writer->WriteRecordEnd();
    }

    JsRTGetTypedArrayInfoAction* JsRTGetTypedArrayInfoAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool returnsArrayBuff = reader->ReadBool(NSTokens::Key::boolVal, true);

        NSLogValue::ArgRetValue var;
        reader->ReadKey(NSTokens::Key::entry, true);
        NSLogValue::ParseArgRetValue(var, false, reader);

        return alloc.SlabNew<JsRTGetTypedArrayInfoAction>(eTime, ctxTag, returnsArrayBuff, var);
    }

    JsRTConstructCallAction::JsRTConstructCallAction(int64 eTime, TTD_LOG_TAG ctxTag, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::ConstructCall), m_functionTagId(functionTagId), m_argCount(argCount), m_argArray(argArray), m_execArgs(execArgs)
    {
        ;
    }

    void JsRTConstructCallAction::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        if(this->m_argArray != nullptr)
        {
            alloc.UnlinkAllocation(this->m_argArray);
        }

        if(this->m_execArgs != nullptr)
        {
            alloc.UnlinkAllocation(this->m_execArgs);
        }
    }

    void JsRTConstructCallAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::RecyclableObject* fobj = execContext->GetThreadContext()->TTDInfo->LookupObjectForTag(this->m_functionTagId);
        Js::JavascriptFunction *jsFunction = Js::JavascriptFunction::FromVar(fobj);

        Js::CallInfo callInfo((ushort)this->m_argCount);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            const NSLogValue::ArgRetValue& aval = this->m_argArray[i];
            Js::Var avar = NSLogValue::InflateArgRetValueIntoVar(aval, execContext);

            this->m_execArgs[i] = avar;
        }
        Js::Arguments jsArgs(callInfo, this->m_execArgs);

        AssertMsg(!Js::ScriptFunction::Is(jsFunction), "This will cause user code to execute and we need to add support for that as a top-level call source!!!!");

        Js::Var value = Js::JavascriptFunction::CallAsConstructor(jsFunction, /* overridingNewTarget = */nullptr, jsArgs, execContext);

        //otherwise tag the return value in case we see it later
        if(value != nullptr && TTD::JsSupport::IsVarPtrValued(value))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(value));
        }
    }

    void JsRTConstructCallAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteLogTag(NSTokens::Key::logTag, this->m_functionTagId, NSTokens::Separator::CommaSeparator);
        writer->WriteLengthValue(this->m_argCount, NSTokens::Separator::CommaSeparator);

        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
            NSLogValue::EmitArgRetValue(this->m_argArray[i], writer, sep);
        }
        writer->WriteSequenceEnd();

        writer->WriteRecordEnd();
    }

    JsRTConstructCallAction* JsRTConstructCallAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        TTD_LOG_TAG ftag = reader->ReadLogTag(NSTokens::Key::logTag, true);

        uint32 argc = reader->ReadLengthValue(true);
        NSLogValue::ArgRetValue* args = (argc != 0) ? alloc.SlabAllocateArray<NSLogValue::ArgRetValue>(argc) : nullptr;

        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < argc; ++i)
        {
            NSLogValue::ParseArgRetValue(args[i], i != 0, reader);
        }
        reader->ReadSequenceEnd();

        Js::Var* execArgs = (argc != 0) ? alloc.SlabAllocateArray<Js::Var>(argc) : nullptr;

        return alloc.SlabNew<JsRTConstructCallAction>(eTime, ctxTag, ftag, argc, args, execArgs);
    }

    JsRTCallbackAction::JsRTCallbackAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isCancel, bool isRepeating, int64 currentCallbackId, TTD_LOG_TAG callbackFunctionTag, int64 createdCallbackId)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CallbackOp), m_isCancel(isCancel), m_isRepeating(isRepeating), m_currentCallbackId(currentCallbackId), m_callbackFunctionTag(callbackFunctionTag), m_createdCallbackId(createdCallbackId)
#if ENABLE_TTD_DEBUGGING
        , m_register_eventTime(-1), m_register_ftime(0), m_register_ltime(0), m_register_line(0), m_register_column(0), m_register_sourceId(0)
#endif
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

    JsRTCallbackAction* JsRTCallbackAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isCancel = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool isRepeating = reader->ReadBool(NSTokens::Key::boolVal, true);

        int64 currentCallbackId = reader->ReadInt64(NSTokens::Key::hostCallbackId, true);
        TTD_LOG_TAG callbackFunctionTag = reader->ReadLogTag(NSTokens::Key::logTag, true);
        int64 createdCallbackId = reader->ReadInt64(NSTokens::Key::newCallbackId, true);

        return alloc.SlabNew<JsRTCallbackAction>(eTime, ctxTag, isCancel, isRepeating, currentCallbackId, callbackFunctionTag, createdCallbackId);
    }

    JsRTCodeParseAction::JsRTCodeParseAction(int64 eTime, TTD_LOG_TAG ctxTag, bool isExpression, const TTString& sourceCode, DWORD_PTR documentId, const TTString& sourceUri, const TTString& srcDir, const TTString& sourceFile)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CodeParse), m_isExpression(isExpression), m_sourceCode(sourceCode), m_sourceUri(sourceUri), m_documentID(documentId), m_srcDir(srcDir), m_sourceFile(sourceFile)
    {
        ;
    }

    void JsRTCodeParseAction::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
        alloc.UnlinkString(this->m_sourceCode);

        if(!IsNullPtrTTString(this->m_sourceUri))
        {
            alloc.UnlinkString(this->m_sourceUri);
        }

        if(!IsNullPtrTTString(this->m_sourceFile))
        {
            alloc.UnlinkString(this->m_sourceFile);
        }

        if(!IsNullPtrTTString(this->m_srcDir))
        {
            alloc.UnlinkString(this->m_srcDir);
        }
    }

    void JsRTCodeParseAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);

        Js::JavascriptFunction* function = nullptr;

        DWORD_PTR sourceContext = this->m_documentID;
        SourceContextInfo * sourceContextInfo = execContext->GetSourceContextInfo(sourceContext, nullptr);
        if(sourceContextInfo == nullptr)
        {
            sourceContextInfo = execContext->CreateSourceContextInfo(sourceContext, this->m_sourceUri.Contents, this->m_sourceUri.Length, nullptr);
        }

        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(this->m_sourceCode.Length), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };

        Js::Utf8SourceInfo* utf8SourceInfo;
        CompileScriptException se;
        BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(execContext)
        {
            function = execContext->LoadScript(this->m_sourceCode.Contents, &si, &se, this->m_isExpression /*isExpression*/, false /*disableDeferredParse*/, false /*isByteCodeBufferForLibrary*/, &utf8SourceInfo, Js::Constants::GlobalCode);
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
        writer->WriteUInt64(NSTokens::Key::documentId, (uint64)this->m_documentID, NSTokens::Separator::CommaSeparator);
        writer->WriteString(NSTokens::Key::logDir, this->m_srcDir, NSTokens::Separator::CommaSeparator);
        writer->WriteString(NSTokens::Key::src, this->m_sourceFile, NSTokens::Separator::CommaSeparator);
        writer->WriteString(NSTokens::Key::uri, this->m_sourceUri, NSTokens::Separator::CommaSeparator);

        writer->WriteLengthValue(this->m_sourceCode.Length, NSTokens::Separator::CommaSeparator);

        LPCWSTR docId = writer->FormatNumber(this->m_documentID);
        JsSupport::WriteCodeToFile(threadContext->TTDStreamFunctions, this->m_srcDir.Contents, docId, this->m_sourceUri.Contents, this->m_sourceCode.Contents, this->m_sourceCode.Length);

        writer->WriteRecordEnd();
    }

    JsRTCodeParseAction* JsRTCodeParseAction::CompleteParse(ThreadContext* threadContext, FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        bool isExpression = reader->ReadBool(NSTokens::Key::isExpression, true);
        DWORD_PTR documentId = (DWORD_PTR)reader->ReadUInt64(NSTokens::Key::documentId, true);

        TTString srcDir;
        reader->ReadString(NSTokens::Key::logDir, alloc, srcDir, true);

        TTString sourceFile;
        reader->ReadString(NSTokens::Key::src, alloc, sourceFile, true);

        TTString srcUri;
        reader->ReadString(NSTokens::Key::uri, alloc, srcUri, true);

        TTString sourceCode;
        sourceCode.Length = reader->ReadLengthValue(true);
        sourceCode.Contents = alloc.SlabAllocateArray<wchar>(sourceCode.Length + 1);

        LPCWSTR docId = reader->FormatNumber(documentId);
        JsSupport::ReadCodeFromFile(threadContext->TTDStreamFunctions, srcDir.Contents, docId, srcUri.Contents, sourceCode.Contents, sourceCode.Length);

        return alloc.SlabNew<JsRTCodeParseAction>(eTime, ctxTag, isExpression, sourceCode, documentId, srcUri, srcDir, sourceFile);
    }

    JsRTCallFunctionBeginAction::JsRTCallFunctionBeginAction(int64 eTime, TTD_LOG_TAG ctxTag, int32 callbackDepth, int64 hostCallbackId, double beginTime, TTD_LOG_TAG functionTagId, uint32 argCount, NSLogValue::ArgRetValue* argArray, Js::Var* execArgs)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CallExistingFunctionBegin), m_callbackDepth(callbackDepth), m_hostCallbackId(hostCallbackId), m_beginTime(beginTime),
        m_functionTagId(functionTagId), m_argCount(argCount), m_argArray(argArray), m_execArgs(execArgs)
#if ENABLE_TTD_DEBUGGING
        , m_rtrSnap(nullptr), m_rtrRestoreLogTag(TTD_INVALID_LOG_TAG), m_rtrRestoreIdentityTag(TTD_INITIAL_IDENTITY_TAG)
#endif
    {
        ;
    }

    void JsRTCallFunctionBeginAction::UnloadEventMemory(UnlinkableSlabAllocator& alloc)
    {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        alloc.UnlinkString(this->m_functionName);
#endif

        if(this->m_argArray != nullptr)
        {
            alloc.UnlinkAllocation(this->m_argArray);
        }

        if(this->m_execArgs != nullptr)
        {
            alloc.UnlinkAllocation(this->m_execArgs);
        }

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

    void JsRTCallFunctionBeginAction::UnloadSnapshot() const
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
    void JsRTCallFunctionBeginAction::SetFunctionName(const TTString& fname)
    {
        this->m_functionName = fname;
    }
#endif

    JsRTCallFunctionBeginAction* JsRTCallFunctionBeginAction::As(JsRTActionLogEntry* action)
    {
        AssertMsg(action->GetActionTypeTag() == JsRTActionType::CallExistingFunctionBegin, "Need to ensure this is true first");

        return static_cast<JsRTCallFunctionBeginAction*>(action);
    }

    bool JsRTCallFunctionBeginAction::HasReadyToRunSnapshotInfo() const
    {
#if !ENABLE_TTD_DEBUGGING
        return false;
#else
        return this->m_rtrSnap != nullptr;
#endif
    }

    void JsRTCallFunctionBeginAction::GetReadyToRunSnapshotInfo(SnapShot** snap, TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const
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

    void JsRTCallFunctionBeginAction::SetReadyToRunSnapshotInfo(SnapShot* snap, TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag) const
    {
#if ENABLE_TTD_DEBUGGING
        AssertMsg(this->m_rtrSnap == nullptr, "We already have a rtr snapshot!!!");

        this->m_rtrSnap = snap;
        this->m_rtrRestoreLogTag = logTag;
        this->m_rtrRestoreIdentityTag = identityTag;
#endif
    }

    int32 JsRTCallFunctionBeginAction::GetCallDepth() const
    {
        return this->m_callbackDepth;
    }

    int64 JsRTCallFunctionBeginAction::GetHostCallbackId() const
    {
        AssertMsg(this->m_callbackDepth == 0, "This should be a root call!!!");

        return this->m_hostCallbackId;
    }

    void JsRTCallFunctionBeginAction::ExecuteAction(ThreadContext* threadContext) const
    {
        Js::ScriptContext* execContext = this->GetScriptContextForAction(threadContext);
        Js::RecyclableObject* fobj = execContext->GetThreadContext()->TTDInfo->LookupObjectForTag(this->m_functionTagId);
        Js::JavascriptFunction *jsFunction = Js::JavascriptFunction::FromVar(fobj);

        Js::CallInfo callInfo((ushort)this->m_argCount);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            const NSLogValue::ArgRetValue& aval = this->m_argArray[i];
            Js::Var avar = NSLogValue::InflateArgRetValueIntoVar(aval, execContext);

            this->m_execArgs[i] = avar;
        }
        Js::Arguments jsArgs(callInfo, this->m_execArgs);

        if(this->m_callbackDepth == 0)
        {
            threadContext->TTDLog->ResetCallStackForTopLevelCall(this->GetEventTime(), this->m_hostCallbackId);
        }

        Js::Var result = nullptr;
        try
        {
            result = jsFunction->CallRootFunction(jsArgs, execContext, true);
        }
        catch(Js::JavascriptExceptionObject*  exceptionObject)
        {
            AssertMsg(threadContext->GetRecordedException() == nullptr, "Not sure if this is true or not but seems like a reasonable requirement.");

            threadContext->SetRecordedException(exceptionObject);
        }
        catch(Js::ScriptAbortException)
        {
            AssertMsg(threadContext->GetRecordedException() == nullptr, "Not sure if this is true or not but seems like a reasonable requirement.");

            threadContext->SetRecordedException(threadContext->GetPendingTerminatedErrorObject());
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

        //since we tag in JsRT we need to tag here too
        if(result != nullptr && JsSupport::IsVarPtrValued(result))
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(result));
        }
    }

    void JsRTCallFunctionBeginAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_callbackDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteInt64(NSTokens::Key::hostCallbackId, this->m_hostCallbackId, NSTokens::Separator::CommaSeparator);
        writer->WriteLogTag(NSTokens::Key::logTag, this->m_functionTagId, NSTokens::Separator::CommaSeparator);

        writer->WriteDouble(NSTokens::Key::beginTime, this->m_beginTime, NSTokens::Separator::CommaSeparator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        writer->WriteString(NSTokens::Key::name, this->m_functionName, NSTokens::Separator::CommaSeparator);
#endif

        writer->WriteLengthValue(this->m_argCount, NSTokens::Separator::CommaSeparator);

        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        for(uint32 i = 0; i < this->m_argCount; ++i)
        {
            NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
            NSLogValue::EmitArgRetValue(this->m_argArray[i], writer, sep);
        }
        writer->WriteSequenceEnd();

        writer->WriteRecordEnd();
    }

    JsRTCallFunctionBeginAction* JsRTCallFunctionBeginAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        int32 callbackDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        int64 hostCallbackId = reader->ReadInt64(NSTokens::Key::hostCallbackId, true);
        TTD_LOG_TAG ftag = reader->ReadLogTag(NSTokens::Key::logTag, true);

        double beginTime = reader->ReadDouble(NSTokens::Key::beginTime, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        TTString fname;
        reader->ReadString(NSTokens::Key::name, alloc, fname, true);
#endif

        uint32 argc = reader->ReadLengthValue(true);
        NSLogValue::ArgRetValue* args = (argc != 0) ? alloc.SlabAllocateArray<NSLogValue::ArgRetValue>(argc) : nullptr;

        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < argc; ++i)
        {
            NSLogValue::ParseArgRetValue(args[i], i != 0, reader);
        }
        reader->ReadSequenceEnd();

        Js::Var* execArgs = (argc != 0) ? alloc.SlabAllocateArray<Js::Var>(argc) : nullptr;

        JsRTCallFunctionBeginAction* res = alloc.SlabNew<JsRTCallFunctionBeginAction>(eTime, ctxTag, callbackDepth, hostCallbackId, beginTime, ftag, argc, args, execArgs);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        res->SetFunctionName(fname);
#endif

        return res;
    }

    JsRTCallFunctionEndAction::JsRTCallFunctionEndAction(int64 eTime, TTD_LOG_TAG ctxTag, int64 matchingBeginTime, bool hasScriptException, bool hasTerminatingException, int32 callbackDepth, double endTime)
        : JsRTActionLogEntry(eTime, ctxTag, JsRTActionType::CallExistingFunctionEnd), m_matchingBeginTime(matchingBeginTime), m_hasScriptException(hasScriptException), m_hasTerminiatingException(hasTerminatingException), m_callbackDepth(callbackDepth), m_endTime(endTime)
#if ENABLE_TTD_DEBUGGING
        , m_lastExecuted_valid(false), m_lastExecuted_ftime(0), m_lastExecuted_ltime(0), m_lastExecuted_line(0), m_lastExecuted_column(0), m_lastExecuted_sourceId(0)
#endif
    {
        ;
    }

    JsRTCallFunctionEndAction* JsRTCallFunctionEndAction::As(JsRTActionLogEntry* action)
    {
        AssertMsg(action->GetActionTypeTag() == JsRTActionType::CallExistingFunctionEnd, "Need to ensure this is true first");

        return static_cast<JsRTCallFunctionEndAction*>(action);
    }

    void JsRTCallFunctionEndAction::SetLastExecutedStatementAndFrameInfo(const SingleCallCounter& lastFrame) const
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

    bool JsRTCallFunctionEndAction::GetLastExecutedStatementAndFrameInfoForDebugger(int64* rootEventTime, uint64* ftime, uint64* ltime, uint32* line, uint32* column, uint32* sourceId) const
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

    int64 JsRTCallFunctionEndAction::GetMatchingCallBegin() const
    {
        return this->m_matchingBeginTime;
    }

    int32 JsRTCallFunctionEndAction::GetCallDepth() const
    {
        return this->m_callbackDepth;
    }

    void JsRTCallFunctionEndAction::ExecuteAction(ThreadContext* threadContext) const
    {
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

            if(this->m_hasScriptException || this->m_hasTerminiatingException)
            {
                throw TTDebuggerAbortException::CreateUncaughtExceptionAbortRequest(threadContext->TTDLog->GetCurrentTopLevelEventTime(), L"Uncaught exception -- Propagate to top-level.");
            }
        }
#endif
    }

    void JsRTCallFunctionEndAction::EmitEvent(LPCWSTR logContainerUri, FileWriter* writer, ThreadContext* threadContext, NSTokens::Separator separator) const
    {
        this->BaseStdEmit(writer, separator);
        this->JsRTBaseEmit(writer);

        writer->WriteInt64(NSTokens::Key::matchingCallBegin, this->m_matchingBeginTime, NSTokens::Separator::CommaSeparator);
        writer->WriteInt32(NSTokens::Key::rootNestingDepth, this->m_callbackDepth, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_hasScriptException, NSTokens::Separator::CommaSeparator);
        writer->WriteBool(NSTokens::Key::boolVal, this->m_hasTerminiatingException, NSTokens::Separator::CommaSeparator);

        writer->WriteDouble(NSTokens::Key::endTime, this->m_endTime, NSTokens::Separator::CommaSeparator);

        writer->WriteRecordEnd();
    }

    JsRTCallFunctionEndAction* JsRTCallFunctionEndAction::CompleteParse(FileReader* reader, UnlinkableSlabAllocator& alloc, int64 eTime, TTD_LOG_TAG ctxTag)
    {
        int64 matchingBegin = reader->ReadInt64(NSTokens::Key::matchingCallBegin, true);
        int32 nestingDepth = reader->ReadInt32(NSTokens::Key::rootNestingDepth, true);
        bool hasScriptException = reader->ReadBool(NSTokens::Key::boolVal, true);
        bool hasTerminatingException = reader->ReadBool(NSTokens::Key::boolVal, true);

        double endTime = reader->ReadDouble(NSTokens::Key::endTime, true);

        return alloc.SlabNew<JsRTCallFunctionEndAction>(eTime, ctxTag, matchingBegin, hasScriptException, hasTerminatingException, nestingDepth, endTime);
    }
}

#endif
