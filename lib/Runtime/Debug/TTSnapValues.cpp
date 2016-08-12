//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

#include "ByteCode/ByteCodeSerializer.h"

namespace TTD
{
    namespace JsSupport
    {
        bool IsVarTaggedInline(Js::Var v)
        {
            return Js::TaggedNumber::Is(v);
        }

        bool IsVarPtrValued(Js::Var v)
        {
            return !Js::TaggedNumber::Is(v);
        }

        bool IsVarPrimitiveKind(Js::Var v)
        {
            if(Js::TaggedNumber::Is(v))
            {
                return false;
            }

            Js::TypeId tid = Js::RecyclableObject::FromVar(v)->GetTypeId();
            return tid <= Js::TypeIds_LastToPrimitiveType;
        }

        bool IsVarComplexKind(Js::Var v)
        {
            if(Js::TaggedNumber::Is(v))
            {
                return false;
            }

            Js::TypeId tid = Js::RecyclableObject::FromVar(v)->GetTypeId();
            return tid > Js::TypeIds_LastToPrimitiveType;
        }

        Js::FunctionBody* ForceAndGetFunctionBody(Js::ParseableFunctionInfo* pfi)
        {
            Js::FunctionBody* fb = nullptr;

            if(pfi->IsDeferredDeserializeFunction())
            {
                Js::DeferDeserializeFunctionInfo* deferDeserializeInfo = pfi->GetDeferDeserializeFunctionInfo();
                fb = deferDeserializeInfo->Deserialize();
            }
            else
            {
                if(pfi->IsDeferredParseFunction())
                {
                    fb = pfi->GetParseableFunctionInfo()->Parse();
                }
                else
                {
                    fb = pfi->GetFunctionBody();
                }
            }
            AssertMsg(fb != nullptr, "I just want a function body!!!");

            fb->EnsureDeserialized();
            return fb;
        }

        void CopyStringToHeapAllocatorWLength(const char16* string, uint32 length, TTString& into)
        {
            AssertMsg(string != nullptr, "Not allowed with string + length");

            into.Length = length;

            if(length == 0)
            {
                into.Contents = nullptr;
            }
            else
            {
                into.Contents = TT_HEAP_ALLOC_ARRAY(char16, into.Length + 1);
                js_memcpy_s(into.Contents, into.Length * sizeof(char16), string, length * sizeof(char16));
                into.Contents[into.Length] = _u('\0');
            }
        }

        void DeleteStringFromHeapAllocator(TTString& string)
        {
            if(string.Contents != nullptr)
            {
                TT_HEAP_FREE_ARRAY(char16, string.Contents, string.Length + 1);
            }
        }

        void WriteCodeToFile(ThreadContext* threadContext, bool fromEvent, DWORD_PTR docId, bool isUtf8Source, byte* sourceBuffer, uint32 length)
        {
            char asciiResourceName[64];
            sprintf_s(asciiResourceName, 64, "src%s_%I64u.js", (fromEvent ? "_ld" : ""), static_cast<uint64>(docId));

            JsTTDStreamHandle srcStream = threadContext->TTDStreamFunctions.pfGetResourceStream(threadContext->TTDUri.UriByteLength, threadContext->TTDUri.UriBytes, asciiResourceName, false, true);

            if(isUtf8Source)
            {
                byte byteOrderArray[3] = { 0xEF, 0xBB, 0xBF };
                size_t byteOrderCount = 0;
                bool okBOC = threadContext->TTDStreamFunctions.pfWriteBytesToStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                AssertMsg(okBOC && byteOrderCount == _countof(byteOrderArray), "Write Failed!!!");
            }
            else
            {
                byte byteOrderArray[2] = { 0xFF, 0xFE };
                size_t byteOrderCount = 0;
                bool okBOC = threadContext->TTDStreamFunctions.pfWriteBytesToStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                AssertMsg(okBOC && byteOrderCount == _countof(byteOrderArray), "Write Failed!!!");
            }

            size_t writtenCount = 0;
            bool ok = threadContext->TTDStreamFunctions.pfWriteBytesToStream(srcStream, sourceBuffer, length, &writtenCount);
            AssertMsg(ok && writtenCount == length, "Write Failed!!!");

            threadContext->TTDStreamFunctions.pfFlushAndCloseStream(srcStream, false, true);
        }

        void ReadCodeFromFile(ThreadContext* threadContext, bool fromEvent, DWORD_PTR docId, bool isUtf8Source, byte* sourceBuffer, uint32 length)
        {
            char asciiResourceName[64];
            sprintf_s(asciiResourceName, 64, "src%s_%I64u.js", (fromEvent ? "_ld" : ""), static_cast<uint64>(docId));

            JsTTDStreamHandle srcStream = threadContext->TTDStreamFunctions.pfGetResourceStream(threadContext->TTDUri.UriByteLength, threadContext->TTDUri.UriBytes, asciiResourceName, true, false);

            if(isUtf8Source)
            {
                byte byteOrderArray[3] = { 0x0, 0x0, 0x0 };
                size_t byteOrderCount = 0;
                bool okBOC = threadContext->TTDStreamFunctions.pfReadBytesFromStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                AssertMsg(okBOC && byteOrderCount == _countof(byteOrderArray) && byteOrderArray[0] == 0xEF && byteOrderArray[1] == 0xBB && byteOrderArray[2] == 0xBF, "Read Failed!!!");
            }
            else
            {
                byte byteOrderArray[2] = { 0x0, 0x0 };
                size_t byteOrderCount = 0;
                bool okBOC = threadContext->TTDStreamFunctions.pfReadBytesFromStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                AssertMsg(okBOC && byteOrderCount == _countof(byteOrderArray) && byteOrderArray[0] == 0xFF && byteOrderArray[1] == 0xFE, "Read Failed!!!");
            }

            size_t readCount = 0;
            bool ok = threadContext->TTDStreamFunctions.pfReadBytesFromStream(srcStream, sourceBuffer, length, &readCount);
            AssertMsg(ok && readCount == length, "Read Failed!!!");

            threadContext->TTDStreamFunctions.pfFlushAndCloseStream(srcStream, true, false);
        }
    }

    namespace NSSnapValues
    {
        void EmitTTDVar(TTDVar var, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            if(var == nullptr)
            {
                writer->WriteTag<TTDVarEmitTag>(NSTokens::Key::ttdVarTag, TTDVarEmitTag::TTDVarNull);
                writer->WriteNull(NSTokens::Key::nullVal, NSTokens::Separator::CommaSeparator);
            }
            else if(Js::TaggedNumber::Is(var))
            {
#if FLOATVAR
                if(Js::TaggedInt::Is(var))
                {
#endif
                    writer->WriteTag<TTDVarEmitTag>(NSTokens::Key::ttdVarTag, TTDVarEmitTag::TTDVarInt);
                    writer->WriteInt32(NSTokens::Key::i32Val, Js::TaggedInt::ToInt32(var), NSTokens::Separator::CommaSeparator);
#if FLOATVAR
                }
                else
                {
                    AssertMsg(Js::JavascriptNumber::Is_NoTaggedIntCheck(var), "Only other tagged value we support!!!");

                    writer->WriteTag<TTDVarEmitTag>(NSTokens::Key::ttdVarTag, TTDVarEmitTag::TTDVarDouble);
                    writer->WriteDouble(NSTokens::Key::doubleVal, Js::JavascriptNumber::GetValue(var), NSTokens::Separator::CommaSeparator);
                }
#endif
            }
            else
            {
                writer->WriteTag<TTDVarEmitTag>(NSTokens::Key::ttdVarTag, TTDVarEmitTag::TTDVarAddr);
                writer->WriteAddr(NSTokens::Key::ptrIdVal, TTD_CONVERT_VAR_TO_PTR_ID(var), NSTokens::Separator::CommaSeparator);
            }

            writer->WriteRecordEnd();
        }

        TTDVar ParseTTDVar(bool readSeperator, FileReader* reader)
        {
            reader->ReadRecordStart(readSeperator);

            TTDVar res = nullptr;
            TTDVarEmitTag tag = reader->ReadTag<TTDVarEmitTag>(NSTokens::Key::ttdVarTag);
            if(tag == TTDVarEmitTag::TTDVarNull)
            {
                reader->ReadNull(NSTokens::Key::nullVal, true);
                res = nullptr;
            }
            else if(tag == TTDVarEmitTag::TTDVarInt)
            {
                int32 intVal = reader->ReadInt32(NSTokens::Key::i32Val, true);
                res = Js::TaggedInt::ToVarUnchecked(intVal);
            }
#if FLOATVAR
            else if(tag == TTDVarEmitTag::TTDVarDouble)
            {
                double doubleVal = reader->ReadDouble(NSTokens::Key::doubleVal, true);
                res = Js::JavascriptNumber::NewInlined(doubleVal, nullptr);
            }
#endif
            else
            {
                AssertMsg(tag == TTDVarEmitTag::TTDVarAddr, "Is there something else?");

                TTD_PTR_ID addrVal = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
                res = TTD_COERCE_PTR_ID_TO_VAR(addrVal);
            }

            reader->ReadRecordEnd();

            return res;
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquivTTDVar_Helper(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next)
        {
            if(v1 == nullptr || v2 == nullptr)
            {
                compareMap.DiagnosticAssert(v1 == v2);
            }
            else if(Js::TaggedNumber::Is(v1) || Js::TaggedNumber::Is(v2))
            {
                compareMap.DiagnosticAssert(Js::TaggedNumber::Is(v1) && Js::TaggedNumber::Is(v2));

#if FLOATVAR
                if(Js::TaggedInt::Is(v1))
                {
#endif
                    compareMap.DiagnosticAssert(Js::TaggedInt::ToInt32(v1) == Js::TaggedInt::ToInt32(v2));
#if FLOATVAR
                }
                else
                {
                    if(Js::JavascriptNumber::IsNan(Js::JavascriptNumber::GetValue(v1)) || Js::JavascriptNumber::IsNan(Js::JavascriptNumber::GetValue(v2)))
                    {
                        compareMap.DiagnosticAssert(Js::JavascriptNumber::IsNan(Js::JavascriptNumber::GetValue(v1)) && Js::JavascriptNumber::IsNan(Js::JavascriptNumber::GetValue(v2)));
                    }
                    else
                    {
                        compareMap.DiagnosticAssert(Js::JavascriptNumber::GetValue(v1) == Js::JavascriptNumber::GetValue(v2));
                    }
                }
#endif
            }
            else
            {
                compareMap.CheckConsistentAndAddPtrIdMapping_Helper(TTD_CONVERT_VAR_TO_PTR_ID(v1), TTD_CONVERT_VAR_TO_PTR_ID(v2), stepKind, next);
            }
        }

        void AssertSnapEquivTTDVar_Property(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid)
        {
            TTDComparePath::PathEntry next{ (uint32)pid, nullptr };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::PropertyData, next);
        }

        void AssertSnapEquivTTDVar_PropertyGetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid)
        {
            TTDComparePath::PathEntry next{ (uint32)pid, nullptr };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::PropertyGetter, next);
        }

        void AssertSnapEquivTTDVar_PropertySetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid)
        {
            TTDComparePath::PathEntry next{ (uint32)pid, nullptr };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::PropertySetter, next);
        }

        void AssertSnapEquivTTDVar_Array(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index)
        {
            TTDComparePath::PathEntry next{ index, nullptr };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::Array, next);
        }

        void AssertSnapEquivTTDVar_SlotArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index)
        {
            TTDComparePath::PathEntry next{ index, nullptr };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::SlotArray, next);
        }

        void AssertSnapEquivTTDVar_Special(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, const char16* specialField)
        {
            TTDComparePath::PathEntry next{ -1, specialField };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::Special, next);
        }

        void AssertSnapEquivTTDVar_SpecialArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, const char16* specialField, uint32 index)
        {
            TTDComparePath::PathEntry next{ index, specialField };
            AssertSnapEquivTTDVar_Helper(v1, v2, compareMap, TTDComparePath::StepKind::SpecialArray, next);
        }
#endif

        //////////////////

        void ExtractSnapPrimitiveValue(SnapPrimitiveValue* snapValue, Js::RecyclableObject* jsValue, bool isWellKnown, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc)
        {
            snapValue->PrimitiveValueId = TTD_CONVERT_VAR_TO_PTR_ID(jsValue);

            Js::Type* objType = jsValue->GetType();
            snapValue->SnapType = idToTypeMap.LookupKnownItem(TTD_CONVERT_TYPEINFO_TO_PTR_ID(objType));

            TTD_WELLKNOWN_TOKEN lookupToken = isWellKnown ? jsValue->GetScriptContext()->TTDWellKnownInfo->ResolvePathForKnownObject(jsValue) : TTD_INVALID_WELLKNOWN_TOKEN;
            snapValue->OptWellKnownToken = alloc.CopyRawNullTerminatedStringInto(lookupToken);

            if(snapValue->OptWellKnownToken == TTD_INVALID_WELLKNOWN_TOKEN)
            {
                Js::JavascriptLibrary* jslib = jsValue->GetLibrary();

                switch(snapValue->SnapType->JsTypeId)
                {
                case Js::TypeIds_Undefined:
                case Js::TypeIds_Null:
                    break;
                case Js::TypeIds_Boolean:
                    snapValue->u_boolValue = Js::JavascriptBoolean::FromVar(jsValue)->GetValue();
                    break;
                case Js::TypeIds_Number:
                    snapValue->u_doubleValue = Js::JavascriptNumber::GetValue(jsValue);
                    break;
                case Js::TypeIds_Int64Number:
                    snapValue->u_int64Value = Js::JavascriptInt64Number::FromVar(jsValue)->GetValue();
                    break;
                case Js::TypeIds_UInt64Number:
                    snapValue->u_uint64Value = Js::JavascriptUInt64Number::FromVar(jsValue)->GetValue();
                    break;
                case Js::TypeIds_String:
                    snapValue->u_stringValue = alloc.SlabAllocateStruct<TTString>();
                    alloc.CopyStringIntoWLength(Js::JavascriptString::FromVar(jsValue)->GetSz(), Js::JavascriptString::FromVar(jsValue)->GetLength(), *(snapValue->u_stringValue));
                    break;
                case Js::TypeIds_Symbol:
                    snapValue->u_propertyIdValue = jslib->ExtractPrimitveSymbolId_TTD(jsValue);
                    break;
                default:
                    AssertMsg(false, "These are supposed to be primitive values on the heap e.g., no pointers or properties.");
                    break;
                }
            }
        }

        void InflateSnapPrimitiveValue(const SnapPrimitiveValue* snapValue, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snapValue->SnapType->ScriptContextLogId);
            Js::JavascriptLibrary* jslib = ctx->GetLibrary();

            Js::Var res = nullptr;
            if(snapValue->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                res = ctx->TTDWellKnownInfo->LookupKnownObjectFromPath(snapValue->OptWellKnownToken);
            }
            else
            {
                Js::RecyclableObject* rObj = inflator->FindReusableObjectIfExists(snapValue->PrimitiveValueId);
                if(rObj != nullptr)
                {
                    res = rObj;
                }
                else
                {
                    switch(snapValue->SnapType->JsTypeId)
                    {
                    case Js::TypeIds_Undefined:
                        res = jslib->GetUndefined();
                        break;
                    case Js::TypeIds_Null:
                        res = jslib->GetNull();
                        break;
                    case Js::TypeIds_Boolean:
                        res = (snapValue->u_boolValue ? jslib->GetTrue() : jslib->GetFalse());
                        break;
                    case Js::TypeIds_Number:
                        res = Js::JavascriptNumber::ToVarWithCheck(snapValue->u_doubleValue, ctx);
                        break;
                    case Js::TypeIds_Int64Number:
                        res = Js::JavascriptInt64Number::ToVar(snapValue->u_int64Value, ctx);
                        break;
                    case Js::TypeIds_UInt64Number:
                        res = Js::JavascriptUInt64Number::ToVar(snapValue->u_uint64Value, ctx);
                        break;
                    case Js::TypeIds_String:
                        res = Js::JavascriptString::NewCopyBuffer(snapValue->u_stringValue->Contents, snapValue->u_stringValue->Length, ctx);
                        break;
                    case Js::TypeIds_Symbol:
                        res = jslib->CreatePrimitveSymbol_TTD(snapValue->u_propertyIdValue);
                        break;
                    default:
                        AssertMsg(false, "These are supposed to be primitive values e.g., no pointers or properties.");
                        res = nullptr;
                    }
                }
            }

            inflator->AddObject(snapValue->PrimitiveValueId, Js::RecyclableObject::FromVar(res));
        }

        void EmitSnapPrimitiveValue(const SnapPrimitiveValue* snapValue, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);
            writer->WriteAddr(NSTokens::Key::primitiveId, snapValue->PrimitiveValueId);
            writer->WriteAddr(NSTokens::Key::typeId, snapValue->SnapType->TypePtrId, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::isWellKnownToken, snapValue->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN, NSTokens::Separator::CommaSeparator);
            if(snapValue->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                writer->WriteWellKnownToken(NSTokens::Key::wellKnownToken, snapValue->OptWellKnownToken, NSTokens::Separator::CommaSeparator);
            }
            else
            {
                switch(snapValue->SnapType->JsTypeId)
                {
                case Js::TypeIds_Undefined:
                case Js::TypeIds_Null:
                    break;
                case Js::TypeIds_Boolean:
                    writer->WriteBool(NSTokens::Key::boolVal, snapValue->u_boolValue ? true : false, NSTokens::Separator::CommaSeparator);
                    break;
                case Js::TypeIds_Number:
                    writer->WriteDouble(NSTokens::Key::doubleVal, snapValue->u_doubleValue, NSTokens::Separator::CommaSeparator);
                    break;
                case Js::TypeIds_Int64Number:
                    writer->WriteInt64(NSTokens::Key::i64Val, snapValue->u_int64Value, NSTokens::Separator::CommaSeparator);
                    break;
                case Js::TypeIds_UInt64Number:
                    writer->WriteUInt64(NSTokens::Key::u64Val, snapValue->u_uint64Value, NSTokens::Separator::CommaSeparator);
                    break;
                case Js::TypeIds_String:
                    writer->WriteString(NSTokens::Key::stringVal, *(snapValue->u_stringValue), NSTokens::Separator::CommaSeparator);
                    break;
                case Js::TypeIds_Symbol:
                    writer->WriteInt32(NSTokens::Key::propertyId, snapValue->u_propertyIdValue, NSTokens::Separator::CommaSeparator);
                    break;
                default:
                    AssertMsg(false, "These are supposed to be primitive values e.g., no pointers or properties.");
                    break;
                }
            }
            writer->WriteRecordEnd();
        }

        void ParseSnapPrimitiveValue(SnapPrimitiveValue* snapValue, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& ptrIdToTypeMap)
        {
            reader->ReadRecordStart(readSeperator);
            snapValue->PrimitiveValueId = reader->ReadAddr(NSTokens::Key::primitiveId);

            TTD_PTR_ID snapTypeId = reader->ReadAddr(NSTokens::Key::typeId, true);
            snapValue->SnapType = ptrIdToTypeMap.LookupKnownItem(snapTypeId);

            bool isWellKnown = reader->ReadBool(NSTokens::Key::isWellKnownToken, true);
            if(isWellKnown)
            {
                snapValue->OptWellKnownToken = reader->ReadWellKnownToken(NSTokens::Key::wellKnownToken, alloc, true);

                //just clear it to help avoid any confusion later
                snapValue->u_uint64Value = 0;
            }
            else
            {
                snapValue->OptWellKnownToken = nullptr;

                switch(snapValue->SnapType->JsTypeId)
                {
                case Js::TypeIds_Undefined:
                case Js::TypeIds_Null:
                    break;
                case Js::TypeIds_Boolean:
                    snapValue->u_boolValue = reader->ReadBool(NSTokens::Key::boolVal, true);
                    break;
                case Js::TypeIds_Number:
                    snapValue->u_doubleValue = reader->ReadDouble(NSTokens::Key::doubleVal, true);
                    break;
                case Js::TypeIds_Int64Number:
                    snapValue->u_int64Value = reader->ReadInt64(NSTokens::Key::i64Val, true);
                    break;
                case Js::TypeIds_UInt64Number:
                    snapValue->u_uint64Value = reader->ReadUInt64(NSTokens::Key::u64Val, true);
                    break;
                case Js::TypeIds_String:
                    snapValue->u_stringValue = alloc.SlabAllocateStruct<TTString>();
                    reader->ReadString(NSTokens::Key::stringVal, alloc, *(snapValue->u_stringValue), true);
                    break;
                case Js::TypeIds_Symbol:
                    snapValue->u_propertyIdValue = (Js::PropertyId)reader->ReadInt32(NSTokens::Key::propertyId, true);
                    break;
                default:
                    AssertMsg(false, "These are supposed to be primitive values e.g., no pointers or properties.");
                    break;
                }
            }

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPrimitiveValue* v1, const SnapPrimitiveValue* v2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(v1->SnapType->JsTypeId == v2->SnapType->JsTypeId);

            NSSnapType::AssertSnapEquiv(v1->SnapType, v2->SnapType, compareMap);

            if(v1->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN || v2->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                compareMap.DiagnosticAssert(v1->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN && v2->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN);
                compareMap.DiagnosticAssert(TTD_DIAGNOSTIC_COMPARE_WELLKNOWN_TOKENS(v1->OptWellKnownToken, v2->OptWellKnownToken));
            }
            else
            {
                switch(v1->SnapType->JsTypeId)
                {
                case Js::TypeIds_Undefined:
                case Js::TypeIds_Null:
                    break;
                case Js::TypeIds_Boolean:
                    compareMap.DiagnosticAssert((v1->u_boolValue ? true : false) == (v2->u_boolValue ? true : false));
                    break;
                case Js::TypeIds_Number:
                    compareMap.DiagnosticAssert(v1->u_doubleValue == v2->u_doubleValue); //This may be problematic wrt. precise FP values
                    break;
                case Js::TypeIds_Int64Number:
                    compareMap.DiagnosticAssert(v1->u_int64Value == v2->u_int64Value);
                    break;
                case Js::TypeIds_UInt64Number:
                    compareMap.DiagnosticAssert(v1->u_uint64Value == v2->u_uint64Value);
                    break;
                case Js::TypeIds_String:
                    compareMap.DiagnosticAssert(TTStringEQForDiagnostics(*(v1->u_stringValue), *(v2->u_stringValue)));
                    break;
                case Js::TypeIds_Symbol:
                    compareMap.DiagnosticAssert(v1->u_propertyIdValue == v2->u_propertyIdValue);
                    break;
                default:
                    AssertMsg(false, "These are supposed to be primitive values e.g., no pointers or properties.");
                    break;
                }
            }
        }
#endif

        //////////////////

        Js::Var* InflateSlotArrayInfo(const SlotArrayInfo* slotInfo, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(slotInfo->ScriptContextLogId);
            Js::Var* slotArray = RecyclerNewArray(ctx->GetRecycler(), Js::Var, slotInfo->SlotCount + Js::ScopeSlots::FirstSlotIndex);

            Js::ScopeSlots scopeSlots(slotArray);
            scopeSlots.SetCount(slotInfo->SlotCount);

            if(slotInfo->isFunctionBodyMetaData)
            {
                Js::FunctionBody* fbody = inflator->LookupFunctionBody(slotInfo->OptFunctionBodyId);
                scopeSlots.SetScopeMetadata(fbody);
            }
            else
            {
                //TODO: when we do better with debuggerscope we should set this to a real value
                scopeSlots.SetScopeMetadata(nullptr);
            }

            for(uint32 j = 0; j < slotInfo->SlotCount; j++)
            {
                Js::Var sval = inflator->InflateTTDVar(slotInfo->Slots[j]);
                scopeSlots.Set(j, sval);
            }

            return slotArray;
        }

        void EmitSlotArrayInfo(const SlotArrayInfo* slotInfo, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);
            writer->AdjustIndent(1);

            writer->WriteAddr(NSTokens::Key::slotId, slotInfo->SlotId, NSTokens::Separator::BigSpaceSeparator);
            writer->WriteLogTag(NSTokens::Key::ctxTag, slotInfo->ScriptContextLogId, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::isFunctionMetaData, slotInfo->isFunctionBodyMetaData, NSTokens::Separator::CommaSeparator);
            if(slotInfo->isFunctionBodyMetaData)
            {
                writer->WriteAddr(NSTokens::Key::functionBodyId, slotInfo->OptFunctionBodyId, NSTokens::Separator::CommaSeparator);
            }
            else
            {
                //TODO: emit debugger scope metadata
            }

            writer->WriteLengthValue(slotInfo->SlotCount, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->AdjustIndent(1);
            for(uint32 i = 0; i < slotInfo->SlotCount; ++i)
            {
                NSTokens::Separator sep = (i != 0 ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                writer->WriteRecordStart(sep);
                writer->WriteUInt32(NSTokens::Key::pid, slotInfo->DebugPIDArray[i], NSTokens::Separator::NoSeparator);
                writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);

                sep = NSTokens::Separator::NoSeparator;
#endif

                EmitTTDVar(slotInfo->Slots[i], writer, sep);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                writer->WriteRecordEnd();
#endif
            }
            writer->AdjustIndent(-1);
            writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

            writer->AdjustIndent(-1);
            writer->WriteRecordEnd(NSTokens::Separator::BigSpaceSeparator);
        }

        void ParseSlotArrayInfo(SlotArrayInfo* slotInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            slotInfo->SlotId = reader->ReadAddr(NSTokens::Key::slotId);
            slotInfo->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);

            slotInfo->isFunctionBodyMetaData = reader->ReadBool(NSTokens::Key::isFunctionMetaData, true);
            slotInfo->OptFunctionBodyId = TTD_INVALID_PTR_ID;
            if(slotInfo->isFunctionBodyMetaData)
            {
                slotInfo->OptFunctionBodyId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
            }
            else
            {
                //TODO: emit debugger scope metadata
            }

            slotInfo->SlotCount = reader->ReadLengthValue(true);
            reader->ReadSequenceStart_WDefaultKey(true);

            slotInfo->Slots = alloc.SlabAllocateArray<TTDVar>(slotInfo->SlotCount);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            slotInfo->DebugPIDArray = alloc.SlabAllocateArray<Js::PropertyId>(slotInfo->SlotCount);
#endif

            for(uint32 i = 0; i < slotInfo->SlotCount; ++i)
            {
                bool readSeparator = (i != 0);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                reader->ReadRecordStart(readSeparator);
                slotInfo->DebugPIDArray[i] = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::pid);
                reader->ReadKey(NSTokens::Key::entry, true);

                readSeparator = false;
#endif

                slotInfo->Slots[i] = ParseTTDVar(readSeparator, reader);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                reader->ReadRecordEnd();
#endif
            }
            reader->ReadSequenceEnd();

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SlotArrayInfo* sai1, const SlotArrayInfo* sai2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(sai1->ScriptContextLogId == sai2->ScriptContextLogId);

            compareMap.DiagnosticAssert(sai1->isFunctionBodyMetaData == sai2->isFunctionBodyMetaData);
            if(sai1->isFunctionBodyMetaData)
            {
                compareMap.CheckConsistentAndAddPtrIdMapping_FunctionBody(sai1->OptFunctionBodyId, sai2->OptFunctionBodyId);
            }
            else
            {
                //TODO: emit debugger scope metadata
            }

            compareMap.DiagnosticAssert(sai1->SlotCount == sai2->SlotCount);
            for(uint32 i = 0; i < sai1->SlotCount; ++i)
            {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                compareMap.DiagnosticAssert(sai1->DebugPIDArray[i] == sai2->DebugPIDArray[i]);
#endif

                AssertSnapEquivTTDVar_SlotArray(sai1->Slots[i], sai2->Slots[i], compareMap, i);
            }
        }
#endif

        Js::FrameDisplay* InflateScriptFunctionScopeInfo(const ScriptFunctionScopeInfo* funcScopeInfo, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(funcScopeInfo->ScriptContextLogId);

            Js::FrameDisplay* environment = RecyclerNewPlus(ctx->GetRecycler(), funcScopeInfo->ScopeCount * sizeof(Js::Var), Js::FrameDisplay, funcScopeInfo->ScopeCount);

            for(uint16 i = 0; i < funcScopeInfo->ScopeCount; ++i)
            {
                const ScopeInfoEntry& scp = funcScopeInfo->ScopeArray[i];

                switch(scp.Tag)
                {
                case Js::ScopeType::ScopeType_ActivationObject:
                case Js::ScopeType::ScopeType_WithScope:
                {
                    Js::Var sval = inflator->LookupObject(scp.IDValue);
                    environment->SetItem(i, sval);
                    break;
                }
                case Js::ScopeType::ScopeType_SlotArray:
                {
                    Js::Var* saval = inflator->LookupSlotArray(scp.IDValue);
                    environment->SetItem(i, saval);
                    break;
                }
                default:
                    AssertMsg(false, "Unknown scope kind");
                    break;
                }
            }

            return environment;
        }

        void EmitScriptFunctionScopeInfo(const ScriptFunctionScopeInfo* funcScopeInfo, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteAddr(NSTokens::Key::scopeId, funcScopeInfo->ScopeId);
            writer->WriteLogTag(NSTokens::Key::ctxTag, funcScopeInfo->ScriptContextLogId, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::count, funcScopeInfo->ScopeCount, NSTokens::Separator::CommaSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);

            for(uint32 i = 0; i < funcScopeInfo->ScopeCount; ++i)
            {
                writer->WriteRecordStart(i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);

                writer->WriteTag<Js::ScopeType>(NSTokens::Key::scopeType, funcScopeInfo->ScopeArray[i].Tag);
                writer->WriteAddr(NSTokens::Key::subscopeId, funcScopeInfo->ScopeArray[i].IDValue, NSTokens::Separator::CommaSeparator);

                writer->WriteRecordEnd();
            }

            writer->WriteSequenceEnd();

            writer->WriteRecordEnd();
        }

        void ParseScriptFunctionScopeInfo(ScriptFunctionScopeInfo* funcScopeInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            funcScopeInfo->ScopeId = reader->ReadAddr(NSTokens::Key::scopeId);
            funcScopeInfo->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);
            funcScopeInfo->ScopeCount = (uint16)reader->ReadUInt32(NSTokens::Key::count, true);

            reader->ReadSequenceStart_WDefaultKey(true);

            funcScopeInfo->ScopeArray = alloc.SlabAllocateArray<ScopeInfoEntry>(funcScopeInfo->ScopeCount);
            for(uint32 i = 0; i < funcScopeInfo->ScopeCount; ++i)
            {
                reader->ReadRecordStart(i != 0);

                funcScopeInfo->ScopeArray[i].Tag = reader->ReadTag<Js::ScopeType>(NSTokens::Key::scopeType);
                funcScopeInfo->ScopeArray[i].IDValue = reader->ReadAddr(NSTokens::Key::subscopeId, true);

                reader->ReadRecordEnd();
            }

            reader->ReadSequenceEnd();

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const ScriptFunctionScopeInfo* funcScopeInfo1, const ScriptFunctionScopeInfo* funcScopeInfo2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(funcScopeInfo1->ScriptContextLogId == funcScopeInfo2->ScriptContextLogId);

            compareMap.DiagnosticAssert(funcScopeInfo1->ScopeCount == funcScopeInfo2->ScopeCount);
            for(uint32 i = 0; i < funcScopeInfo1->ScopeCount; ++i)
            {
                compareMap.DiagnosticAssert(funcScopeInfo1->ScopeArray[i].Tag == funcScopeInfo2->ScopeArray[i].Tag);
                compareMap.CheckConsistentAndAddPtrIdMapping_Scope(funcScopeInfo1->ScopeArray[i].IDValue, funcScopeInfo2->ScopeArray[i].IDValue, i);
            }
        }
#endif

        //////////////////

        Js::JavascriptPromiseCapability* InflatePromiseCapabilityInfo(const SnapPromiseCapabilityInfo* capabilityInfo, Js::ScriptContext* ctx, InflateMap* inflator)
        {
            if(!inflator->IsPromiseInfoDefined<Js::JavascriptPromiseCapability>(capabilityInfo->CapabilityId))
            {
                Js::Var promise = inflator->InflateTTDVar(capabilityInfo->PromiseVar);
                Js::Var resolve = inflator->InflateTTDVar(capabilityInfo->ResolveVar);
                Js::Var reject = inflator->InflateTTDVar(capabilityInfo->RejectVar);

                Js::JavascriptPromiseCapability* res = ctx->GetLibrary()->CreatePromiseCapability_TTD(promise, resolve, reject);
                inflator->AddInflatedPromiseInfo<Js::JavascriptPromiseCapability>(capabilityInfo->CapabilityId, res);
            }

            return inflator->LookupInflatedPromiseInfo<Js::JavascriptPromiseCapability>(capabilityInfo->CapabilityId);
        }

        void EmitPromiseCapabilityInfo(const SnapPromiseCapabilityInfo* capabilityInfo, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, capabilityInfo->CapabilityId);

            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            EmitTTDVar(capabilityInfo->PromiseVar, writer, NSTokens::Separator::NoSeparator);

            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            EmitTTDVar(capabilityInfo->ResolveVar, writer, NSTokens::Separator::NoSeparator);

            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            EmitTTDVar(capabilityInfo->RejectVar, writer, NSTokens::Separator::NoSeparator);

            writer->WriteRecordEnd();
        }

        void ParsePromiseCapabilityInfo(SnapPromiseCapabilityInfo* capabilityInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            capabilityInfo->CapabilityId = reader->ReadAddr(NSTokens::Key::ptrIdVal);

            reader->ReadKey(NSTokens::Key::entry, true);
            capabilityInfo->PromiseVar = ParseTTDVar(false, reader);

            reader->ReadKey(NSTokens::Key::entry, true);
            capabilityInfo->ResolveVar = ParseTTDVar(false, reader);

            reader->ReadKey(NSTokens::Key::entry, true);
            capabilityInfo->RejectVar = ParseTTDVar(false, reader);

            reader->ReadRecordEnd();
        }


#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPromiseCapabilityInfo* capabilityInfo1, const SnapPromiseCapabilityInfo* capabilityInfo2, TTDCompareMap& compareMap)
        {
            compareMap.CheckConsistentAndAddPtrIdMapping_NoEnqueue(capabilityInfo1->CapabilityId, capabilityInfo2->CapabilityId);

            AssertSnapEquivTTDVar_Special(capabilityInfo1->PromiseVar, capabilityInfo2->PromiseVar, compareMap, _u("promiseVar"));
            AssertSnapEquivTTDVar_Special(capabilityInfo1->ResolveVar, capabilityInfo2->ResolveVar, compareMap, _u("resolveObjId"));
            AssertSnapEquivTTDVar_Special(capabilityInfo1->RejectVar, capabilityInfo2->RejectVar, compareMap, _u("rejectObjId"));
        }
#endif

        Js::JavascriptPromiseReaction* InflatePromiseReactionInfo(const SnapPromiseReactionInfo* reactionInfo, Js::ScriptContext* ctx, InflateMap* inflator)
        {
            if(!inflator->IsPromiseInfoDefined<Js::JavascriptPromiseReaction>(reactionInfo->PromiseReactionId))
            {
                Js::RecyclableObject* handler = inflator->LookupObject(reactionInfo->HandlerObjId);
                Js::JavascriptPromiseCapability* capabilities = InflatePromiseCapabilityInfo(&reactionInfo->Capabilities, ctx, inflator);

                Js::JavascriptPromiseReaction* res = ctx->GetLibrary()->CreatePromiseReaction_TTD(handler, capabilities);
                inflator->AddInflatedPromiseInfo<Js::JavascriptPromiseReaction>(reactionInfo->PromiseReactionId, res);
            }

            return inflator->LookupInflatedPromiseInfo<Js::JavascriptPromiseReaction>(reactionInfo->PromiseReactionId);
        }

        void EmitPromiseReactionInfo(const SnapPromiseReactionInfo* reactionInfo, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, reactionInfo->PromiseReactionId);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, reactionInfo->HandlerObjId, NSTokens::Separator::CommaSeparator);
            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            EmitPromiseCapabilityInfo(&reactionInfo->Capabilities, writer, NSTokens::Separator::NoSeparator);

            writer->WriteRecordEnd();
        }

        void ParsePromiseReactionInfo(SnapPromiseReactionInfo* reactionInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            reactionInfo->PromiseReactionId = reader->ReadAddr(NSTokens::Key::ptrIdVal);

            reactionInfo->HandlerObjId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
            reader->ReadKey(NSTokens::Key::entry, true);
            ParsePromiseCapabilityInfo(&reactionInfo->Capabilities, false, reader, alloc);

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPromiseReactionInfo* reactionInfo1, const SnapPromiseReactionInfo* reactionInfo2, TTDCompareMap& compareMap)
        {
            compareMap.CheckConsistentAndAddPtrIdMapping_NoEnqueue(reactionInfo1->PromiseReactionId, reactionInfo2->PromiseReactionId);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(reactionInfo1->HandlerObjId, reactionInfo2->HandlerObjId, _u("handlerObjId"));
            AssertSnapEquiv(&(reactionInfo1->Capabilities), &(reactionInfo2->Capabilities), compareMap);
        }
#endif

        //////////////////

        void ExtractTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint64 topLevelCtr, Js::ModuleID moduleId, DWORD_PTR documentID, bool isUtf8source, const byte* source, uint32 sourceLen, SlabAllocator& alloc)
        {
            fbInfo->ScriptContextLogId = fb->GetScriptContext()->ScriptContextLogTag;
            fbInfo->TopLevelBodyCtr = topLevelCtr;

            alloc.CopyNullTermStringInto(fb->GetDisplayName(), fbInfo->FunctionName);

            fbInfo->ModuleId = moduleId;
            fbInfo->DocumentID = documentID;
            alloc.CopyNullTermStringInto(fb->GetSourceContextInfo()->url, fbInfo->SourceUri);

            fbInfo->IsUtf8 = isUtf8source;
            fbInfo->ByteLength = sourceLen;
            fbInfo->SourceBuffer = alloc.SlabAllocateArray<byte>(fbInfo->ByteLength);
            js_memcpy_s(fbInfo->SourceBuffer, fbInfo->ByteLength, source, sourceLen);

            fbInfo->DbgSerializedBytecodeSize = 0;
            fbInfo->DbgSerializedBytecodeBuffer = nullptr;
        }

        void EmitTopLevelCommonBodyResolveInfo(const TopLevelCommonBodyResolveInfo* fbInfo, bool emitInline, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);
            writer->WriteUInt64(NSTokens::Key::functionBodyId, fbInfo->TopLevelBodyCtr);
            writer->WriteLogTag(NSTokens::Key::ctxTag, fbInfo->ScriptContextLogId, NSTokens::Separator::CommaSeparator);

            writer->WriteString(NSTokens::Key::name, fbInfo->FunctionName, NSTokens::Separator::CommaSeparator);

            writer->WriteUInt64(NSTokens::Key::moduleId, fbInfo->ModuleId, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::documentId, fbInfo->DocumentID, NSTokens::Separator::CommaSeparator);
            writer->WriteString(NSTokens::Key::uri, fbInfo->SourceUri, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, fbInfo->IsUtf8, NSTokens::Separator::CommaSeparator);
            writer->WriteLengthValue(fbInfo->ByteLength, NSTokens::Separator::CommaSeparator);

            if(emitInline || IsNullPtrTTString(fbInfo->SourceUri))
            {
                AssertMsg(!fbInfo->IsUtf8, "Should only emit char16 encoded data in inline mode.");

                writer->WriteInlineCode((char16*)fbInfo->SourceBuffer, fbInfo->ByteLength / sizeof(char16), NSTokens::Separator::CommaSeparator);
            }
            else
            {
                JsSupport::WriteCodeToFile(threadContext, false, fbInfo->DocumentID, fbInfo->IsUtf8, fbInfo->SourceBuffer, fbInfo->ByteLength);
            }
        }

        void ParseTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, bool readSeperator, bool parseInline, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);
            fbInfo->TopLevelBodyCtr = reader->ReadUInt64(NSTokens::Key::functionBodyId);
            fbInfo->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);

            reader->ReadString(NSTokens::Key::name, alloc, fbInfo->FunctionName, true);

            fbInfo->ModuleId = (Js::ModuleID)reader->ReadUInt64(NSTokens::Key::moduleId, true);
            fbInfo->DocumentID = (DWORD_PTR)reader->ReadUInt64(NSTokens::Key::documentId, true);
            reader->ReadString(NSTokens::Key::uri, alloc, fbInfo->SourceUri, true);

            fbInfo->IsUtf8 = reader->ReadBool(NSTokens::Key::boolVal, true);
            fbInfo->ByteLength = reader->ReadLengthValue(true);
            fbInfo->SourceBuffer = alloc.SlabAllocateArray<byte>(fbInfo->ByteLength);

            if(parseInline || IsNullPtrTTString(fbInfo->SourceUri))
            {
                AssertMsg(!fbInfo->IsUtf8, "Should only emit char16 encoded data in inline mode.");

                reader->ReadInlineCode((char16*)fbInfo->SourceBuffer, fbInfo->ByteLength / sizeof(char16), true);
            }
            else
            {
                JsSupport::ReadCodeFromFile(threadContext, false, fbInfo->DocumentID, fbInfo->IsUtf8, fbInfo->SourceBuffer, fbInfo->ByteLength);
            }

            fbInfo->DbgSerializedBytecodeSize = 0;
            fbInfo->DbgSerializedBytecodeBuffer = nullptr;
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv(const TopLevelCommonBodyResolveInfo* fbInfo1, const TopLevelCommonBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(fbInfo1->ScriptContextLogId == fbInfo2->ScriptContextLogId);
            compareMap.DiagnosticAssert(fbInfo1->TopLevelBodyCtr == fbInfo2->TopLevelBodyCtr);
            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(fbInfo1->FunctionName, fbInfo2->FunctionName));

            //
            //TODO: we don't force documentids to be the same accross re-inflate yet but when we do we should check this as well
            //
            //TTD_DIAGNOSTIC_ASSERT(fbInfo1->DocumentID, fbInfo2->DocumentID);

            compareMap.DiagnosticAssert(fbInfo1->ModuleId == fbInfo2->ModuleId);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(fbInfo1->SourceUri, fbInfo2->SourceUri));

            compareMap.DiagnosticAssert(fbInfo1->IsUtf8 == fbInfo2->IsUtf8);
            compareMap.DiagnosticAssert(fbInfo1->ByteLength == fbInfo2->ByteLength);

            for(uint32 i = 0; i < fbInfo1->ByteLength; ++i)
            {
                compareMap.DiagnosticAssert(fbInfo1->SourceBuffer[i] == fbInfo2->SourceBuffer[i]);
            }
        }
#endif

        ////
        //Regular script-load functions

        void ExtractTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint64 topLevelCtr, Js::ModuleID moduleId, DWORD_PTR documentID, bool isUtf8, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag, SlabAllocator& alloc)
        {
            NSSnapValues::ExtractTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, fb, topLevelCtr, moduleId, documentID, isUtf8, source, sourceLen, alloc);

            fbInfo->LoadFlag = loadFlag;
        }

        Js::FunctionBody* InflateTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx)
        {
            byte* script = fbInfo->TopLevelBase.SourceBuffer;
            uint32 scriptLength = fbInfo->TopLevelBase.ByteLength;
            DWORD_PTR sourceContext = fbInfo->TopLevelBase.DocumentID;

            AssertMsg(ctx->GetSourceContextInfo(sourceContext, nullptr) == nullptr, "On inflate we should either have clean ctxts or we want to optimize the inflate process by skipping redoing this work!!!");
            AssertMsg(fbInfo->TopLevelBase.IsUtf8 == ((fbInfo->LoadFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source), "Utf8 status is inconsistent!!!");

            SourceContextInfo * sourceContextInfo = ctx->CreateSourceContextInfo(sourceContext, fbInfo->TopLevelBase.SourceUri.Contents, fbInfo->TopLevelBase.SourceUri.Length, nullptr);

            AssertMsg(fbInfo->TopLevelBase.IsUtf8 || sizeof(wchar) == sizeof(char16), "Non-utf8 code only allowed on windows!!!");
            const int chsize = (fbInfo->LoadFlag & LoadScriptFlag_Utf8Source) ? sizeof(char) : sizeof(char16);
            SRCINFO si = {
                /* sourceContextInfo   */ sourceContextInfo,
                /* dlnHost             */ 0,
                /* ulColumnHost        */ 0,
                /* lnMinHost           */ 0,
                /* ichMinHost          */ 0,
                /* ichLimHost          */ static_cast<ULONG>(scriptLength / chsize), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
                /* ulCharOffset        */ 0,
                /* mod                 */ fbInfo->TopLevelBase.ModuleId,
                /* grfsi               */ 0
            };

            Js::Utf8SourceInfo* utf8SourceInfo = nullptr;
            CompileScriptException se;
            Js::JavascriptFunction* scriptFunction = nullptr;
            Js::FunctionBody* globalBody = nullptr;

            if(fbInfo->TopLevelBase.DbgSerializedBytecodeSize != 0)
            {
                //
                //TODO: Bytecode serializer does not support debug bytecode (StatementMaps vs Positions) so add this to serializer code.
                //      Then we can add code do optimized bytecode reload here.
                //
            }
            else
            {
                BEGIN_LEAVE_SCRIPT_WITH_EXCEPTION(ctx)
                {
                    // TODO: We should use the utf8 source here if possible
                    scriptFunction = ctx->LoadScript(script, scriptLength, &si, &se, &utf8SourceInfo, Js::Constants::GlobalCode, fbInfo->LoadFlag);
                }
                END_LEAVE_SCRIPT_WITH_EXCEPTION(ctx);
                AssertMsg(scriptFunction != nullptr, "Something went wrong");

                globalBody = TTD::JsSupport::ForceAndGetFunctionBody(scriptFunction->GetParseableFunctionInfo());

                //
                //TODO: Bytecode serializer does not suppper debug bytecode (StatementMaps vs Positions) so add this to serializer code.
                //      Then we can add code to do optimized bytecode reload here.
                //
            }

            ////
            //We don't do this automatically in the load script helper so do it here
            ctx->TTDContextInfo->ProcessFunctionBodyOnLoad(globalBody, nullptr);
            ctx->TTDContextInfo->RegisterLoadedScript(globalBody, fbInfo->TopLevelBase.TopLevelBodyCtr);

            bool isLibraryCode = ((fbInfo->LoadFlag & LoadScriptFlag_LibraryCode) == LoadScriptFlag_LibraryCode);
            const HostScriptContextCallbackFunctor& hostFunctor = ctx->TTDHostCallbackFunctor;
            if(hostFunctor.pfOnScriptLoadCallback != nullptr && !isLibraryCode)
            {
                hostFunctor.pfOnScriptLoadCallback(hostFunctor.HostData, scriptFunction, utf8SourceInfo, &se);
            }
            ////

            return globalBody;
        }

        void EmitTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator)
        {
            NSSnapValues::EmitTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, false, threadContext, writer, separator);

            writer->WriteTag<LoadScriptFlag>(NSTokens::Key::loadFlag, fbInfo->LoadFlag, NSTokens::Separator::CommaSeparator);

            writer->WriteRecordEnd();
        }

        void ParseTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
        {
            NSSnapValues::ParseTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, readSeperator, false, threadContext, reader, alloc);

            fbInfo->LoadFlag = reader->ReadTag<LoadScriptFlag>(NSTokens::Key::loadFlag, true);

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo1, const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(fbInfo1->LoadFlag == fbInfo2->LoadFlag);

            AssertSnapEquiv(&(fbInfo1->TopLevelBase), &(fbInfo2->TopLevelBase), compareMap);
        }
#endif

        ////
        //'new Function(...)' functions

        void ExtractTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint64 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, SlabAllocator& alloc)
        {
            NSSnapValues::ExtractTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, fb, topLevelCtr, moduleId, 0, false, (const byte*)source, sourceLen * sizeof(char16), alloc);
        }

        Js::FunctionBody* InflateTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx)
        {
            // Bug 1105479. Get the module id from the caller
            Js::ModuleID moduleID = kmodGlobal;
            BOOL strictMode = FALSE;

            char16* source = (char16*)fbInfo->TopLevelBase.SourceBuffer;
            int32 length = (int32)(fbInfo->TopLevelBase.ByteLength / sizeof(char16));

            Js::JavascriptFunction* pfuncScript = ctx->GetGlobalObject()->EvalHelper(ctx, source, length, moduleID, fscrNil, Js::Constants::FunctionCode, TRUE, TRUE, strictMode);
            AssertMsg(pfuncScript != nullptr, "Something went wrong!!!");

            // Indicate that this is a top-level function. We don't pass the fscrGlobalCode flag to the eval helper,
            // or it will return the global function that wraps the declared function body, as though it were an eval.
            // But we want, for instance, to be able to verify that we did the right amount of deferred parsing.
            Js::ParseableFunctionInfo* functionInfo = pfuncScript->GetParseableFunctionInfo();
            functionInfo->SetGrfscr(functionInfo->GetGrfscr() | fscrGlobalCode);

            Js::EvalMapString key(source, length, moduleID, strictMode, /* isLibraryCode = */ false);
            ctx->AddToNewFunctionMap(key, functionInfo);

            Js::FunctionBody* fb = JsSupport::ForceAndGetFunctionBody(pfuncScript->GetParseableFunctionInfo());

            //We don't do this automatically in the eval helper so do it here
            ctx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
            ctx->TTDContextInfo->RegisterNewScript(fb, fbInfo->TopLevelBase.TopLevelBodyCtr);

            return fb;
        }

        void EmitTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator)
        {
            NSSnapValues::EmitTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, true, threadContext, writer, separator);

            writer->WriteRecordEnd();
        }

        void ParseTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
        {
            NSSnapValues::ParseTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, readSeperator, false, threadContext, reader, alloc);

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelNewFunctionBodyResolveInfo* fbInfo1, const TopLevelNewFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap)
        {
            AssertSnapEquiv(&(fbInfo1->TopLevelBase), &(fbInfo2->TopLevelBase), compareMap);
        }
#endif

        ////
        //'eval(...)' functions

        void ExtractTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint64 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode, SlabAllocator& alloc)
        {
            NSSnapValues::ExtractTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, fb, topLevelCtr, moduleId, 0, false, (const byte*)source, sourceLen * sizeof(char16), alloc);

            fbInfo->EvalFlags = grfscr;
            fbInfo->RegisterDocument = registerDocument;
            fbInfo->IsIndirect = isIndirect ? true : false;
            fbInfo->IsStrictMode = strictMode ? true : false;
        }

        Js::FunctionBody* InflateTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx)
        {
            uint32 grfscr = ((uint32)fbInfo->EvalFlags) | fscrReturnExpression | fscrEval | fscrEvalCode | fscrGlobalCode;

            char16* source = (char16*)fbInfo->TopLevelBase.SourceBuffer;
            int32 sourceLen = (int32)(fbInfo->TopLevelBase.ByteLength / sizeof(char16));
            Js::ScriptFunction* pfuncScript = ctx->GetLibrary()->GetGlobalObject()->EvalHelper(ctx, source, sourceLen, fbInfo->TopLevelBase.ModuleId, grfscr, Js::Constants::EvalCode, fbInfo->RegisterDocument, fbInfo->IsIndirect, fbInfo->IsStrictMode);
            Assert(!pfuncScript->GetFunctionInfo()->IsGenerator());

            Js::FastEvalMapString key(source, sourceLen, fbInfo->TopLevelBase.ModuleId, fbInfo->IsStrictMode, false);
            ctx->AddToEvalMap(key, fbInfo->IsIndirect, pfuncScript);

            Js::FunctionBody* fb = JsSupport::ForceAndGetFunctionBody(pfuncScript->GetParseableFunctionInfo());

            //We don't do this automatically ing the eval helper so do it here
            ctx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
            ctx->TTDContextInfo->RegisterEvalScript(fb, fbInfo->TopLevelBase.TopLevelBodyCtr);

            return fb;
        }

        void EmitTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator)
        {
            NSSnapValues::EmitTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, true, threadContext, writer, separator);

            writer->WriteUInt64(NSTokens::Key::u64Val, fbInfo->EvalFlags, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, fbInfo->RegisterDocument, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, fbInfo->IsIndirect, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, fbInfo->IsStrictMode, NSTokens::Separator::CommaSeparator);
            writer->WriteRecordEnd();
        }

        void ParseTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
        {
            NSSnapValues::ParseTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, readSeperator, false, threadContext, reader, alloc);

            fbInfo->EvalFlags = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            fbInfo->RegisterDocument = reader->ReadBool(NSTokens::Key::boolVal, true);
            fbInfo->IsIndirect = reader->ReadBool(NSTokens::Key::boolVal, true);
            fbInfo->IsStrictMode = reader->ReadBool(NSTokens::Key::boolVal, true);
            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelEvalFunctionBodyResolveInfo* fbInfo1, const TopLevelEvalFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(fbInfo1->EvalFlags == fbInfo2->EvalFlags);
            compareMap.DiagnosticAssert(fbInfo1->RegisterDocument == fbInfo2->RegisterDocument);
            compareMap.DiagnosticAssert(fbInfo1->IsIndirect == fbInfo2->IsIndirect);
            compareMap.DiagnosticAssert(fbInfo1->IsStrictMode == fbInfo2->IsStrictMode);

            AssertSnapEquiv(&(fbInfo1->TopLevelBase), &(fbInfo2->TopLevelBase), compareMap);
        }
#endif

        void ExtractFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, bool isWellKnown, SlabAllocator& alloc)
        {
            fbInfo->FunctionBodyId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(fb);
            fbInfo->ScriptContextLogId = fb->GetScriptContext()->ScriptContextLogTag;

            alloc.CopyStringIntoWLength(fb->GetDisplayName(), fb->GetDisplayNameLength(), fbInfo->FunctionName);
            AssertMsg(wcscmp(fbInfo->FunctionName.Contents, Js::Constants::GlobalCode) != 0, "Why are we snapshotting global code??");

            if(isWellKnown)
            {
                fbInfo->OptKnownPath = alloc.CopyRawNullTerminatedStringInto(fb->GetScriptContext()->TTDWellKnownInfo->ResolvePathForKnownFunctionBody(fb));

                fbInfo->OptParentBodyId = TTD_INVALID_PTR_ID;
                fbInfo->OptLine = -1;
                fbInfo->OptColumn = -1;
            }
            else
            {
                fbInfo->OptKnownPath = TTD_INVALID_WELLKNOWN_TOKEN;

                Js::FunctionBody* parentBody = fb->GetScriptContext()->TTDContextInfo->ResolveParentBody(fb);
                AssertMsg(parentBody != nullptr, "We missed something!!!");

                fbInfo->OptParentBodyId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(parentBody);
                fbInfo->OptLine = fb->GetLineNumber();
                fbInfo->OptColumn = fb->GetColumnNumber();
            }
        }

        void InflateFunctionBody(const FunctionBodyResolveInfo* fbInfo, InflateMap* inflator, const TTDIdentifierDictionary<TTD_PTR_ID, FunctionBodyResolveInfo*>& idToFbResolveMap)
        {
            if(inflator->IsFunctionBodyAlreadyInflated(fbInfo->FunctionBodyId))
            {
                return;
            }

            Js::ScriptContext* ctx = inflator->LookupScriptContext(fbInfo->ScriptContextLogId);

            Js::FunctionBody* resfb = nullptr;
            if(fbInfo->OptKnownPath != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                resfb = ctx->TTDWellKnownInfo->LookupKnownFunctionBodyFromPath(fbInfo->OptKnownPath);
            }
            else
            {
                resfb = inflator->FindReusableFunctionBodyIfExists(fbInfo->FunctionBodyId);
                if(resfb == nullptr)
                {
                    //Find and inflate the parent body
                    if(!inflator->IsFunctionBodyAlreadyInflated(fbInfo->OptParentBodyId))
                    {
                        const FunctionBodyResolveInfo* pBodyInfo = idToFbResolveMap.LookupKnownItem(fbInfo->OptParentBodyId);
                        InflateFunctionBody(pBodyInfo, inflator, idToFbResolveMap);
                    }

                    Js::FunctionBody* parentBody = inflator->LookupFunctionBody(fbInfo->OptParentBodyId);

                    int32 imin = 0;
                    int32 imax = parentBody->GetNestedCount();
                    while(imin < imax)
                    {
                        int imid = (imin + imax) / 2;
                        Js::ParseableFunctionInfo* pfiMid = parentBody->GetNestedFunc(imid)->EnsureDeserialized();
                        Js::FunctionBody* currfb = JsSupport::ForceAndGetFunctionBody(pfiMid);

                        int32 cmpval = 0;
                        if(fbInfo->OptLine != currfb->GetLineNumber())
                        {
                            cmpval = (fbInfo->OptLine < currfb->GetLineNumber()) ? -1 : 1;
                        }
                        else
                        {
                            if(fbInfo->OptColumn != currfb->GetColumnNumber())
                            {
                                cmpval = (fbInfo->OptColumn < currfb->GetColumnNumber()) ? -1 : 1;
                            }
                        }

                        if(cmpval == 0)
                        {
                            resfb = currfb;
                            break;
                        }

                        if(cmpval > 0)
                        {
                            imin = imid + 1;
                        }
                        else
                        {
                            imax = imid;
                        }
                    }
                    AssertMsg(resfb != nullptr && fbInfo->OptLine == resfb->GetLineNumber() && fbInfo->OptColumn == resfb->GetColumnNumber(), "We are missing something");
                    AssertMsg(resfb != nullptr && (wcscmp(fbInfo->FunctionName.Contents, resfb->GetDisplayName()) == 0 || wcscmp(_u("get"), resfb->GetDisplayName()) == 0 || wcscmp(_u("set"), resfb->GetDisplayName()) == 0), "We are missing something");
                }
            }

            bool updateName = false;
            if(fbInfo->FunctionName.Length != resfb->GetDisplayNameLength())
            {
                updateName = true;
            }
            else
            {
                const char16* fname = resfb->GetDisplayName();
                for(uint32 i = 0; i < fbInfo->FunctionName.Length; ++i)
                {
                    updateName |= (fbInfo->FunctionName.Contents[i] != fname[i]);
                }
            }

            if(updateName)
            {
                AssertMsg(wcsstr(fbInfo->FunctionName.Contents, _u("get ")) == 0 || wcsstr(fbInfo->FunctionName.Contents, _u("set ")) == 0, "Does not start with get or set");

                resfb->SetDisplayName(fbInfo->FunctionName.Contents,fbInfo->FunctionName.Length, 3, Js::FunctionProxy::SetDisplayNameFlagsRecyclerAllocated);
            }

            inflator->AddInflationFunctionBody(fbInfo->FunctionBodyId, resfb);
        }

        void EmitFunctionBodyInfo(const FunctionBodyResolveInfo* fbInfo, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);
            writer->WriteAddr(NSTokens::Key::functionBodyId, fbInfo->FunctionBodyId);
            writer->WriteLogTag(NSTokens::Key::ctxTag, fbInfo->ScriptContextLogId, NSTokens::Separator::CommaSeparator);
            writer->WriteString(NSTokens::Key::name, fbInfo->FunctionName, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::isWellKnownToken, fbInfo->OptKnownPath != nullptr, NSTokens::Separator::CommaSeparator);
            if(fbInfo->OptKnownPath != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                writer->WriteWellKnownToken(NSTokens::Key::wellKnownToken, fbInfo->OptKnownPath, NSTokens::Separator::CommaSeparator);
            }
            else
            {
                writer->WriteAddr(NSTokens::Key::parentBodyId, fbInfo->OptParentBodyId, NSTokens::Separator::CommaSeparator);
                writer->WriteInt64(NSTokens::Key::line, fbInfo->OptLine, NSTokens::Separator::CommaSeparator);
                writer->WriteInt64(NSTokens::Key::column, fbInfo->OptColumn, NSTokens::Separator::CommaSeparator);
            }

            writer->WriteRecordEnd();
        }

        void ParseFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            fbInfo->FunctionBodyId = reader->ReadAddr(NSTokens::Key::functionBodyId);
            fbInfo->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);
            reader->ReadString(NSTokens::Key::name, alloc, fbInfo->FunctionName, true);

            bool isWellKnown = reader->ReadBool(NSTokens::Key::isWellKnownToken, true);
            if(isWellKnown)
            {
                fbInfo->OptKnownPath = reader->ReadWellKnownToken(NSTokens::Key::wellKnownToken, alloc, true);

                fbInfo->OptParentBodyId = TTD_INVALID_PTR_ID;
                fbInfo->OptLine = -1;
                fbInfo->OptColumn = -1;
            }
            else
            {
                fbInfo->OptKnownPath = TTD_INVALID_WELLKNOWN_TOKEN;

                fbInfo->OptParentBodyId = reader->ReadAddr(NSTokens::Key::parentBodyId, true);
                fbInfo->OptLine = reader->ReadInt64(NSTokens::Key::line, true);
                fbInfo->OptColumn = reader->ReadInt64(NSTokens::Key::column, true);
            }

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const FunctionBodyResolveInfo* fbInfo1, const FunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(fbInfo1->ScriptContextLogId == fbInfo2->ScriptContextLogId);
            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(fbInfo1->FunctionName, fbInfo2->FunctionName));

            compareMap.DiagnosticAssert(TTD_DIAGNOSTIC_COMPARE_WELLKNOWN_TOKENS(fbInfo1->OptKnownPath, fbInfo2->OptKnownPath));
            if(fbInfo1->OptKnownPath == TTD_INVALID_WELLKNOWN_TOKEN)
            {
                compareMap.CheckConsistentAndAddPtrIdMapping_FunctionBody(fbInfo1->OptParentBodyId, fbInfo2->OptParentBodyId);

                compareMap.DiagnosticAssert(fbInfo1->OptLine == fbInfo2->OptLine);
                compareMap.DiagnosticAssert(fbInfo1->OptColumn == fbInfo2->OptColumn);
            }
        }
#endif

        //////////////////

        void ExtractScriptContext(SnapContext* snapCtx, Js::ScriptContext* ctx, SlabAllocator& alloc)
        {
            snapCtx->m_scriptContextLogId = ctx->ScriptContextLogTag;

            snapCtx->m_isPNRGSeeded = ctx->GetLibrary()->IsPRNGSeeded();
            snapCtx->m_randomSeed0 = ctx->GetLibrary()->GetRandSeed0();
            snapCtx->m_randomSeed1 = ctx->GetLibrary()->GetRandSeed1();
            alloc.CopyNullTermStringInto(ctx->GetUrl(), snapCtx->m_contextSRC);

            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelScriptLoad(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelNewFunction(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelEval(&HeapAllocator::Instance);

            ctx->TTDContextInfo->GetLoadedSources(topLevelScriptLoad, topLevelNewFunction, topLevelEval);

            snapCtx->m_loadedTopLevelScriptCount = topLevelScriptLoad.Count();
            snapCtx->m_loadedTopLevelScriptArray = (snapCtx->m_loadedTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->m_loadedTopLevelScriptCount) : nullptr;
            for(int32 i = 0; i < topLevelScriptLoad.Count(); ++i)
            {
                snapCtx->m_loadedTopLevelScriptArray[i] = topLevelScriptLoad.Item(i);
            }

            snapCtx->m_newFunctionTopLevelScriptCount = topLevelNewFunction.Count();
            snapCtx->m_newFunctionTopLevelScriptArray = (snapCtx->m_newFunctionTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->m_newFunctionTopLevelScriptCount) : nullptr;
            for(int32 i = 0; i < topLevelNewFunction.Count(); ++i)
            {
                snapCtx->m_newFunctionTopLevelScriptArray[i] = topLevelNewFunction.Item(i);
            }

            snapCtx->m_evalTopLevelScriptCount = topLevelEval.Count();
            snapCtx->m_evalTopLevelScriptArray = (snapCtx->m_evalTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->m_evalTopLevelScriptCount) : nullptr;
            for(int32 i = 0; i < topLevelEval.Count(); ++i)
            {
                snapCtx->m_evalTopLevelScriptArray[i] = topLevelEval.Item(i);
            }

            //invert the root map for extracting

            JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator> objToLogIdMap(&HeapAllocator::Instance);
            ctx->TTDContextInfo->LoadInvertedRootMap(objToLogIdMap);

            //Extract global roots
            snapCtx->m_globalRootCount = ctx->TTDContextInfo->GetRootSet()->Count();
            snapCtx->m_globalRootArray = (snapCtx->m_globalRootCount != 0) ? alloc.SlabAllocateArray<SnapRootPinEntry>(snapCtx->m_globalRootCount) : nullptr;

            int32 i = 0;
            for(auto iter = ctx->TTDContextInfo->GetRootSet()->GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                AssertMsg(objToLogIdMap.ContainsKey(iter.CurrentValue()), "We are missing a value mapping!!!");

                snapCtx->m_globalRootArray[i].LogObject = TTD_CONVERT_VAR_TO_PTR_ID(iter.CurrentValue());
                snapCtx->m_globalRootArray[i].LogId = objToLogIdMap.LookupWithKey(iter.CurrentValue(), TTD_INVALID_PTR_ID);

                i++;
            }

            //Extract local roots
            if(ctx->TTDContextInfo->GetLocalRootSet()->Count() == 0)
            {
                snapCtx->m_localRootCount = 0;
                snapCtx->m_localRootArray = nullptr;
            }
            else
            {
                snapCtx->m_localRootCount = 0;
                snapCtx->m_localRootArray = alloc.SlabReserveArraySpace<SnapRootPinEntry>(ctx->TTDContextInfo->GetLocalRootSet()->Count());

                for(auto iter = ctx->TTDContextInfo->GetLocalRootSet()->GetIterator(); iter.IsValid(); iter.MoveNext())
                {
                    if(objToLogIdMap.ContainsKey(iter.CurrentValue()))
                    {
                        snapCtx->m_localRootArray[snapCtx->m_localRootCount].LogObject = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(iter.CurrentValue());
                        snapCtx->m_localRootArray[snapCtx->m_localRootCount].LogId = objToLogIdMap.LookupWithKey(iter.CurrentValue(), TTD_INVALID_LOG_PTR_ID);

                        snapCtx->m_localRootCount++;
                    }
                }

                if(snapCtx->m_localRootCount != 0)
                {
                    alloc.SlabCommitArraySpace<SnapRootPinEntry>(snapCtx->m_localRootCount, ctx->TTDContextInfo->GetLocalRootSet()->Count());
                }
                else
                {
                    alloc.SlabAbortArraySpace<SnapRootPinEntry>(ctx->TTDContextInfo->GetLocalRootSet()->Count());
                }
            }
            //Extract pending async modification info
            const JsUtil::List<TTDPendingAsyncBufferModification, HeapAllocator>& pendingAsyncList = ctx->TTDContextInfo->GetPendingAsyncModListForSnapshot();
            snapCtx->m_pendingAsyncModCount = pendingAsyncList.Count();
            snapCtx->m_pendingAsyncModArray = (snapCtx->m_pendingAsyncModCount != 0) ? alloc.SlabAllocateArray<SnapPendingAsyncBufferModification>(snapCtx->m_pendingAsyncModCount) : nullptr;

            for(int32 k = 0; k < pendingAsyncList.Count(); ++k)
            {
                const TTDPendingAsyncBufferModification& pk = pendingAsyncList.Item(k);
                snapCtx->m_pendingAsyncModArray[k].LogId = objToLogIdMap.LookupWithKey(Js::RecyclableObject::FromVar(pk.ArrayBufferVar), TTD_INVALID_LOG_PTR_ID);
                snapCtx->m_pendingAsyncModArray[k].Index = pk.Index;
            }
        }

        void InflateScriptContext(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator, 
            const TTDIdentifierDictionary<uint64, TopLevelScriptLoadFunctionBodyResolveInfo*>& topLevelLoadScriptMap,
            const TTDIdentifierDictionary<uint64, TopLevelNewFunctionBodyResolveInfo*>& topLevelNewScriptMap, 
            const TTDIdentifierDictionary<uint64, TopLevelEvalFunctionBodyResolveInfo*>& topLevelEvalScriptMap)
        {
            AssertMsg(wcscmp(snpCtx->m_contextSRC.Contents, intoCtx->GetUrl()) == 0, "Make sure the src uri values are the same.");

            intoCtx->GetLibrary()->SetIsPRNGSeeded(snpCtx->m_isPNRGSeeded);
            intoCtx->GetLibrary()->SetRandSeed0(snpCtx->m_randomSeed0);
            intoCtx->GetLibrary()->SetRandSeed1(snpCtx->m_randomSeed1);
            inflator->AddScriptContext(snpCtx->m_scriptContextLogId, intoCtx);

            intoCtx->TTDContextInfo->ClearLoadedSourcesForSnapshotRestore();

            for(uint32 i = 0; i < snpCtx->m_loadedTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation& cri = snpCtx->m_loadedTopLevelScriptArray[i];

                Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                if(fb != nullptr)
                {
                    intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                    intoCtx->TTDContextInfo->RegisterLoadedScript(fb, cri.TopLevelBodyCtr);
                }
                else
                {
                    const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo = topLevelLoadScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);
                    fb = NSSnapValues::InflateTopLevelLoadedFunctionBodyInfo(fbInfo, intoCtx);
                }
                inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
            }

            for(uint32 i = 0; i < snpCtx->m_newFunctionTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation& cri = snpCtx->m_newFunctionTopLevelScriptArray[i];

                Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                if(fb != nullptr)
                {
                    intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                    intoCtx->TTDContextInfo->RegisterNewScript(fb, cri.TopLevelBodyCtr);
                }
                else
                {
                    const TopLevelNewFunctionBodyResolveInfo* fbInfo = topLevelNewScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);
                    fb = NSSnapValues::InflateTopLevelNewFunctionBodyInfo(fbInfo, intoCtx);
                }
                inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
            }

            for(uint32 i = 0; i < snpCtx->m_evalTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation& cri = snpCtx->m_evalTopLevelScriptArray[i];

                Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                if(fb != nullptr)
                {
                    intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                    intoCtx->TTDContextInfo->RegisterEvalScript(fb, cri.TopLevelBodyCtr);
                }
                else
                {
                    const TopLevelEvalFunctionBodyResolveInfo* fbInfo = topLevelEvalScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);
                    fb = NSSnapValues::InflateTopLevelEvalFunctionBodyInfo(fbInfo, intoCtx);
                }
                inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
            }
        }

        void ReLinkRoots(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator)
        {
            intoCtx->TTDContextInfo->ClearRootsForSnapRestore();

            for(uint32 i = 0; i < snpCtx->m_globalRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry = snpCtx->m_globalRootArray[i];
                Js::RecyclableObject* rootObj = inflator->LookupObject(rootEntry.LogObject);

                intoCtx->TTDContextInfo->AddTrackedRoot(rootEntry.LogId, rootObj);
            }

            for(uint32 i = 0; i < snpCtx->m_localRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry = snpCtx->m_localRootArray[i];
                Js::RecyclableObject* rootObj = inflator->LookupObject(rootEntry.LogObject);

                intoCtx->TTDContextInfo->AddLocalRoot(rootEntry.LogId, rootObj);
            }
        }

        void ResetPendingAsyncBufferModInfo(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator)
        {
            intoCtx->TTDContextInfo->ClearPendingAsyncModListForSnapRestore();

            for(uint32 i = 0; i < snpCtx->m_pendingAsyncModCount; ++i)
            {
                Js::RecyclableObject* buff = inflator->LookupObject(snpCtx->m_pendingAsyncModArray[i].LogId);
                uint32 index = snpCtx->m_pendingAsyncModArray[i].Index;

                AssertMsg(Js::ArrayBuffer::Is(buff), "Not an ArrayBuffer!!!");
                intoCtx->TTDContextInfo->AddToAsyncPendingList(Js::ArrayBuffer::FromVar(buff), index);
            }
        }

        void EmitSnapContext(const SnapContext* snapCtx, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteLogTag(NSTokens::Key::ctxTag, snapCtx->m_scriptContextLogId);
            writer->WriteBool(NSTokens::Key::boolVal, snapCtx->m_isPNRGSeeded, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::u64Val, snapCtx->m_randomSeed0, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::u64Val, snapCtx->m_randomSeed1, NSTokens::Separator::CommaSeparator);
            writer->WriteString(NSTokens::Key::ctxUri, snapCtx->m_contextSRC, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(snapCtx->m_loadedTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_loadedTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->m_loadedTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt64(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->m_newFunctionTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_newFunctionTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->m_newFunctionTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt64(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->m_evalTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_evalTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->m_evalTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt64(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->m_globalRootCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_globalRootCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                writer->WriteRecordStart(sep);
                writer->WriteLogTag(NSTokens::Key::logTag, snapCtx->m_globalRootArray[i].LogId);
                writer->WriteAddr(NSTokens::Key::objectId, snapCtx->m_globalRootArray[i].LogObject, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->m_localRootCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_localRootCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                writer->WriteRecordStart(sep);
                writer->WriteLogTag(NSTokens::Key::logTag, snapCtx->m_localRootArray[i].LogId);
                writer->WriteAddr(NSTokens::Key::objectId, snapCtx->m_localRootArray[i].LogObject, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->m_pendingAsyncModCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->m_pendingAsyncModCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                writer->WriteRecordStart(sep);
                writer->WriteLogTag(NSTokens::Key::logTag, snapCtx->m_pendingAsyncModArray[i].LogId);
                writer->WriteUInt32(NSTokens::Key::u32Val, snapCtx->m_pendingAsyncModArray[i].Index, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteRecordEnd();
        }

        void ParseSnapContext(SnapContext* intoCtx, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            intoCtx->m_scriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag);
            intoCtx->m_isPNRGSeeded = reader->ReadBool(NSTokens::Key::boolVal, true);
            intoCtx->m_randomSeed0 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            intoCtx->m_randomSeed1 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            reader->ReadString(NSTokens::Key::ctxUri, alloc, intoCtx->m_contextSRC, true);

            intoCtx->m_loadedTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->m_loadedTopLevelScriptArray = (intoCtx->m_loadedTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->m_loadedTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_loadedTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->m_loadedTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt64(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->m_newFunctionTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->m_newFunctionTopLevelScriptArray = (intoCtx->m_newFunctionTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->m_newFunctionTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_newFunctionTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->m_newFunctionTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt64(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->m_evalTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->m_evalTopLevelScriptArray = (intoCtx->m_evalTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->m_evalTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_evalTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->m_evalTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt64(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->m_globalRootCount = reader->ReadLengthValue(true);
            intoCtx->m_globalRootArray = (intoCtx->m_globalRootCount != 0) ? alloc.SlabAllocateArray<SnapRootPinEntry>(intoCtx->m_globalRootCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_globalRootCount; ++i)
            {
                reader->ReadRecordStart(i != 0);
                intoCtx->m_globalRootArray[i].LogId = reader->ReadLogTag(NSTokens::Key::logTag);
                intoCtx->m_globalRootArray[i].LogObject = reader->ReadAddr(NSTokens::Key::objectId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->m_localRootCount = reader->ReadLengthValue(true);
            intoCtx->m_localRootArray = (intoCtx->m_localRootCount != 0) ? alloc.SlabAllocateArray<SnapRootPinEntry>(intoCtx->m_localRootCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_localRootCount; ++i)
            {
                reader->ReadRecordStart(i != 0);
                intoCtx->m_localRootArray[i].LogId = reader->ReadLogTag(NSTokens::Key::logTag);
                intoCtx->m_localRootArray[i].LogObject = reader->ReadAddr(NSTokens::Key::objectId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->m_pendingAsyncModCount = reader->ReadLengthValue(true);
            intoCtx->m_pendingAsyncModArray = (intoCtx->m_pendingAsyncModCount != 0) ? alloc.SlabAllocateArray<SnapPendingAsyncBufferModification>(intoCtx->m_pendingAsyncModCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->m_pendingAsyncModCount; ++i)
            {
                reader->ReadRecordStart(i != 0);
                intoCtx->m_pendingAsyncModArray[i].LogId = reader->ReadLogTag(NSTokens::Key::logTag);
                intoCtx->m_pendingAsyncModArray[i].Index = reader->ReadUInt32(NSTokens::Key::u32Val, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapContext* snapCtx1, const SnapContext* snapCtx2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(snapCtx1->m_scriptContextLogId == snapCtx2->m_scriptContextLogId);

            compareMap.DiagnosticAssert(snapCtx1->m_isPNRGSeeded == snapCtx2->m_isPNRGSeeded);
            compareMap.DiagnosticAssert(snapCtx1->m_randomSeed0 == snapCtx2->m_randomSeed0);
            compareMap.DiagnosticAssert(snapCtx1->m_randomSeed1 == snapCtx2->m_randomSeed1);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(snapCtx1->m_contextSRC, snapCtx2->m_contextSRC));

            //
            //TODO: Once loaded script has a unique identifier we can match (e.g. documentId) then we should match here.
            //      For now just sanity check the number of top-level functions and let the FunctionBody matching drive any matching.
            //

            compareMap.DiagnosticAssert(snapCtx1->m_loadedTopLevelScriptCount == snapCtx2->m_loadedTopLevelScriptCount);
            //TopLevelScriptLoadFunctionBodyResolveInfo* m_loadedScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->m_newFunctionTopLevelScriptCount == snapCtx2->m_newFunctionTopLevelScriptCount);
            //TopLevelNewFunctionBodyResolveInfo* m_newScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->m_evalTopLevelScriptCount == snapCtx2->m_evalTopLevelScriptCount);
            //TopLevelEvalFunctionBodyResolveInfo* m_evalScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->m_globalRootCount == snapCtx2->m_globalRootCount);

            JsUtil::BaseDictionary<TTD_LOG_PTR_ID, TTD_PTR_ID, HeapAllocator> allRootMap1(&HeapAllocator::Instance);
            JsUtil::BaseDictionary<TTD_LOG_PTR_ID, TTD_PTR_ID, HeapAllocator> allRootMap2(&HeapAllocator::Instance);

            JsUtil::BaseDictionary<TTD_LOG_PTR_ID, TTD_PTR_ID, HeapAllocator> globalRootMap1(&HeapAllocator::Instance);
            for(uint32 i = 0; i < snapCtx1->m_globalRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry1 = snapCtx1->m_globalRootArray[i];
                allRootMap1.AddNew(rootEntry1.LogId, rootEntry1.LogObject);

                globalRootMap1.AddNew(rootEntry1.LogId, rootEntry1.LogObject);
            }

            for(uint32 i = 0; i < snapCtx2->m_globalRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry2 = snapCtx2->m_globalRootArray[i];
                allRootMap2.AddNew(rootEntry2.LogId, rootEntry2.LogObject);

                TTD_PTR_ID id1 = globalRootMap1.LookupWithKey(rootEntry2.LogId, TTD_INVALID_PTR_ID);
                compareMap.CheckConsistentAndAddPtrIdMapping_Root(id1, rootEntry2.LogObject, rootEntry2.LogId);
            }

            compareMap.DiagnosticAssert(snapCtx1->m_localRootCount == snapCtx2->m_localRootCount);

            JsUtil::BaseDictionary<TTD_LOG_PTR_ID, TTD_PTR_ID, HeapAllocator> localRootMap1(&HeapAllocator::Instance);
            for(uint32 i = 0; i < snapCtx1->m_localRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry1 = snapCtx1->m_localRootArray[i];
                if(!allRootMap1.ContainsKey(rootEntry1.LogId))
                {
                    allRootMap1.AddNew(rootEntry1.LogId, rootEntry1.LogObject);
                }

                localRootMap1.AddNew(rootEntry1.LogId, rootEntry1.LogObject);
            }

            for(uint32 i = 0; i < snapCtx2->m_localRootCount; ++i)
            {
                const SnapRootPinEntry& rootEntry2 = snapCtx2->m_localRootArray[i];
                if(!allRootMap2.ContainsKey(rootEntry2.LogId))
                {
                    allRootMap2.AddNew(rootEntry2.LogId, rootEntry2.LogObject);
                }

                TTD_PTR_ID id1 = localRootMap1.LookupWithKey(rootEntry2.LogId, TTD_INVALID_PTR_ID);
                compareMap.CheckConsistentAndAddPtrIdMapping_Root(id1, rootEntry2.LogObject, rootEntry2.LogId);
            }

            compareMap.DiagnosticAssert(snapCtx1->m_pendingAsyncModCount == snapCtx2->m_pendingAsyncModCount);

            for(uint32 i = 0; i < snapCtx1->m_pendingAsyncModCount; ++i)
            {
                const SnapPendingAsyncBufferModification& pendEntry1 = snapCtx1->m_pendingAsyncModArray[i];
                const SnapPendingAsyncBufferModification& pendEntry2 = snapCtx2->m_pendingAsyncModArray[i];

                compareMap.DiagnosticAssert(pendEntry1.LogId == pendEntry2.LogId && pendEntry1.Index == pendEntry2.Index);

                compareMap.H1PendingAsyncModBufferSet.AddNew(allRootMap1.LookupWithKey(pendEntry1.LogId, TTD_INVALID_PTR_ID));
                compareMap.H2PendingAsyncModBufferSet.AddNew(allRootMap2.LookupWithKey(pendEntry2.LogId, TTD_INVALID_PTR_ID));
            }
        }
#endif
    }
}

#endif
