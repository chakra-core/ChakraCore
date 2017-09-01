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

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        bool AreInlineVarsEquiv(Js::Var v1, Js::Var v2)
        {
            if(v1 == v2)
            {
                return true; //same bit pattern so no problem
            }

            if(v1 == nullptr || v2 == nullptr)
            {
                return false; //then they should be the same per above
            }

            if(Js::TaggedNumber::Is(v1) != Js::TaggedNumber::Is(v2))
            {
                return false;
            }

            double v1val = Js::TaggedInt::Is(v1) ? Js::TaggedInt::ToInt32(v1) : Js::JavascriptNumber::GetValue(v1);
            double v2val = Js::TaggedInt::Is(v2) ? Js::TaggedInt::ToInt32(v2) : Js::JavascriptNumber::GetValue(v2);

            if(Js::JavascriptNumber::IsNan(v1val) != Js::JavascriptNumber::IsNan(v2val))
            {
                return false;
            }

            return v1val == v2val;
        }
#endif

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
            TTDAssert(fb != nullptr, "I just want a function body!!!");

            fb->EnsureDeserialized();
            return fb;
        }

        void WriteCodeToFile(ThreadContext* threadContext, bool fromEvent, uint32 bodyId, bool isUtf8Source, byte* sourceBuffer, uint32 length)
        {
            char asciiResourceName[64];
            sprintf_s(asciiResourceName, 64, "src%s_%I32u.js", (fromEvent ? "_ld" : ""), bodyId);

            TTDataIOInfo& iofp = threadContext->TTDContext->TTDataIOInfo;
            JsTTDStreamHandle srcStream = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(asciiResourceName), asciiResourceName, false, true);
            TTDAssert(srcStream != nullptr, "Failed to open code resource stream for writing.");

            if(isUtf8Source)
            {
                byte byteOrderArray[3] = { 0xEF, 0xBB, 0xBF };
                size_t byteOrderCount = 0;
                bool okBOC = iofp.pfWriteBytesToStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                TTDAssert(okBOC && byteOrderCount == _countof(byteOrderArray), "Write Failed!!!");
            }
            else
            {
                byte byteOrderArray[2] = { 0xFF, 0xFE };
                size_t byteOrderCount = 0;
                bool okBOC = iofp.pfWriteBytesToStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                TTDAssert(okBOC && byteOrderCount == _countof(byteOrderArray), "Write Failed!!!");
            }

            size_t writtenCount = 0;
            bool ok = iofp.pfWriteBytesToStream(srcStream, sourceBuffer, length, &writtenCount);
            TTDAssert(ok && writtenCount == length, "Write Failed!!!");

            iofp.pfFlushAndCloseStream(srcStream, false, true);
        }

        void ReadCodeFromFile(ThreadContext* threadContext, bool fromEvent, uint32 bodyId, bool isUtf8Source, byte* sourceBuffer, uint32 length)
        {
            char asciiResourceName[64];
            sprintf_s(asciiResourceName, 64, "src%s_%I32u.js", (fromEvent ? "_ld" : ""), bodyId);

            TTDataIOInfo& iofp = threadContext->TTDContext->TTDataIOInfo;
            JsTTDStreamHandle srcStream = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(asciiResourceName), asciiResourceName, true, false);
            TTDAssert(srcStream != nullptr, "Failed to open code resource stream for reading.");

            if(isUtf8Source)
            {
                byte byteOrderArray[3] = { 0x0, 0x0, 0x0 };
                size_t byteOrderCount = 0;
                bool okBOC = iofp.pfReadBytesFromStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                TTDAssert(okBOC && byteOrderCount == _countof(byteOrderArray) && byteOrderArray[0] == 0xEF && byteOrderArray[1] == 0xBB && byteOrderArray[2] == 0xBF, "Read Failed!!!");
            }
            else
            {
                byte byteOrderArray[2] = { 0x0, 0x0 };
                size_t byteOrderCount = 0;
                bool okBOC = iofp.pfReadBytesFromStream(srcStream, byteOrderArray, _countof(byteOrderArray), &byteOrderCount);
                TTDAssert(okBOC && byteOrderCount == _countof(byteOrderArray) && byteOrderArray[0] == 0xFF && byteOrderArray[1] == 0xFE, "Read Failed!!!");
            }

            size_t readCount = 0;
            bool ok = iofp.pfReadBytesFromStream(srcStream, sourceBuffer, length, &readCount);
            TTDAssert(ok && readCount == length, "Read Failed!!!");

            iofp.pfFlushAndCloseStream(srcStream, true, false);
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
                    TTDAssert(Js::JavascriptNumber::Is_NoTaggedIntCheck(var), "Only other tagged value we support!!!");

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
                TTDAssert(tag == TTDVarEmitTag::TTDVarAddr, "Is there something else?");

                TTD_PTR_ID addrVal = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
                res = TTD_COERCE_PTR_ID_TO_VAR(addrVal);
            }

            reader->ReadRecordEnd();

            return res;
        }

#if ENABLE_SNAPSHOT_COMPARE
        bool CheckSnapEquivTTDDouble(double d1, double d2)
        {
            if(Js::JavascriptNumber::IsNan(d1) || Js::JavascriptNumber::IsNan(d2))
            {
                return (Js::JavascriptNumber::IsNan(d1) && Js::JavascriptNumber::IsNan(d2));
            }
            else
            {
                return (d1 == d2);
            }
        }

        void AssertSnapEquivTTDVar_Helper(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next)
        {
            if(v1 == nullptr || v2 == nullptr)
            {
                compareMap.DiagnosticAssert(v1 == v2);
            }
            else if(Js::TaggedNumber::Is(v1) || Js::TaggedNumber::Is(v2))
            {
                compareMap.DiagnosticAssert(JsSupport::AreInlineVarsEquiv(v1, v2));
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
                    TTDAssert(false, "These are supposed to be primitive values on the heap e.g., no pointers or properties.");
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
                        TTDAssert(false, "These are supposed to be primitive values e.g., no pointers or properties.");
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
                    writer->WriteBool(NSTokens::Key::boolVal, !!snapValue->u_boolValue, NSTokens::Separator::CommaSeparator);
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
                    TTDAssert(false, "These are supposed to be primitive values e.g., no pointers or properties.");
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
                    TTDAssert(false, "These are supposed to be primitive values e.g., no pointers or properties.");
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
                    compareMap.DiagnosticAssert((!!v1->u_boolValue) == (!!v2->u_boolValue));
                    break;
                case Js::TypeIds_Number:
                    compareMap.DiagnosticAssert(CheckSnapEquivTTDDouble(v1->u_doubleValue, v2->u_doubleValue));
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
                    TTDAssert(false, "These are supposed to be primitive values e.g., no pointers or properties.");
                    break;
                }
            }
        }
#endif

        //////////////////

        Js::Var* InflateSlotArrayInfo(const SlotArrayInfo* slotInfo, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(slotInfo->ScriptContextLogId);
            Field(Js::Var)* slotArray = RecyclerNewArray(ctx->GetRecycler(), Field(Js::Var), slotInfo->SlotCount + Js::ScopeSlots::FirstSlotIndex);

            Js::ScopeSlots scopeSlots((Js::Var*)slotArray);
            scopeSlots.SetCount(slotInfo->SlotCount);

            Js::Var undef = ctx->GetLibrary()->GetUndefined();
            for(uint32 j = 0; j < slotInfo->SlotCount; j++)
            {
                scopeSlots.Set(j, undef);
            }

            if(slotInfo->isFunctionBodyMetaData)
            {
                Js::FunctionBody* fbody = inflator->LookupFunctionBody(slotInfo->OptFunctionBodyId);
                scopeSlots.SetScopeMetadata(fbody->GetFunctionInfo());

                //This is a doubly nested lookup so if the scope slot array is large this could be expensive
                Js::PropertyId* propertyIds = fbody->GetPropertyIdsForScopeSlotArray();
                for(uint32 j = 0; j < slotInfo->SlotCount; j++)
                {
                    Js::PropertyId trgtPid = slotInfo->PIDArray[j];
                    for(uint32 i = 0; i < fbody->GetScopeSlotArraySize(); i++)
                    {
                        if(trgtPid == propertyIds[i])
                        {
                            Js::Var sval = inflator->InflateTTDVar(slotInfo->Slots[j]);
                            scopeSlots.Set(i, sval);
                            break;
                        }
                    }
                }
            }
            else
            {
                Js::DebuggerScope* dbgScope = nullptr;
                if(slotInfo->OptWellKnownDbgScope != TTD_INVALID_WELLKNOWN_TOKEN)
                {
                    dbgScope = ctx->TTDWellKnownInfo->LookupKnownDebuggerScopeFromPath(slotInfo->OptWellKnownDbgScope);
                }
                else
                {
                    Js::FunctionBody* scopeBody = nullptr;
                    int32 scopeIndex = -1;
                    inflator->LookupInfoForDebugScope(slotInfo->OptDebugScopeId, &scopeBody, &scopeIndex);

                    dbgScope = scopeBody->GetScopeObjectChain()->pScopeChain->Item(scopeIndex);
                }

                scopeSlots.SetScopeMetadata(dbgScope);

                //This is a doubly nested lookup so if the scope slot array is large this could be expensive
                for(uint32 j = 0; j < slotInfo->SlotCount; j++)
                {
                    Js::PropertyId trgtPid = slotInfo->PIDArray[j];
                    for(uint32 i = 0; i < slotInfo->SlotCount; i++)
                    {
                        if(trgtPid == dbgScope->GetPropertyIdForSlotIndex_TTD(i))
                        {
                            Js::Var sval = inflator->InflateTTDVar(slotInfo->Slots[j]);
                            scopeSlots.Set(i, sval);
                            break;
                        }
                    }
                }
            }

            return (Js::Var*)slotArray;
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
                writer->WriteBool(NSTokens::Key::isWellKnownToken, slotInfo->OptWellKnownDbgScope != TTD_INVALID_WELLKNOWN_TOKEN, NSTokens::Separator::CommaSeparator);
                if(slotInfo->OptWellKnownDbgScope != TTD_INVALID_WELLKNOWN_TOKEN)
                {
                    writer->WriteWellKnownToken(NSTokens::Key::wellKnownToken, slotInfo->OptWellKnownDbgScope, NSTokens::Separator::CommaSeparator);
                }
                else
                {
                    writer->WriteAddr(NSTokens::Key::debuggerScopeId, slotInfo->OptDebugScopeId, NSTokens::Separator::CommaSeparator);
                }
            }

            writer->WriteLengthValue(slotInfo->SlotCount, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->AdjustIndent(1);
            for(uint32 i = 0; i < slotInfo->SlotCount; ++i)
            {
                writer->WriteRecordStart(i != 0 ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);
                writer->WriteUInt32(NSTokens::Key::pid, slotInfo->PIDArray[i], NSTokens::Separator::NoSeparator);
                writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);

                EmitTTDVar(slotInfo->Slots[i], writer, NSTokens::Separator::NoSeparator);

                writer->WriteRecordEnd();
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
            slotInfo->OptDebugScopeId = TTD_INVALID_PTR_ID;
            slotInfo->OptWellKnownDbgScope = TTD_INVALID_WELLKNOWN_TOKEN;

            if(slotInfo->isFunctionBodyMetaData)
            {
                slotInfo->OptFunctionBodyId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
            }
            else
            {
                bool isWellKnown = reader->ReadBool(NSTokens::Key::isWellKnownToken, true);
                if(isWellKnown)
                {
                    slotInfo->OptWellKnownDbgScope = reader->ReadWellKnownToken(NSTokens::Key::wellKnownToken, alloc, true);
                }
                else
                {
                    slotInfo->OptDebugScopeId = reader->ReadAddr(NSTokens::Key::debuggerScopeId, true);
                }
            }

            slotInfo->SlotCount = reader->ReadLengthValue(true);
            reader->ReadSequenceStart_WDefaultKey(true);

            slotInfo->Slots = alloc.SlabAllocateArray<TTDVar>(slotInfo->SlotCount);

            slotInfo->PIDArray = alloc.SlabAllocateArray<Js::PropertyId>(slotInfo->SlotCount);

            for(uint32 i = 0; i < slotInfo->SlotCount; ++i)
            {
                reader->ReadRecordStart(i != 0);
                slotInfo->PIDArray[i] = (Js::PropertyId)reader->ReadUInt32(NSTokens::Key::pid);
                reader->ReadKey(NSTokens::Key::entry, true);

                slotInfo->Slots[i] = ParseTTDVar(false, reader);

                reader->ReadRecordEnd();
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
                Js::PropertyId id1 = sai1->PIDArray[i];
                bool found = false;
                for(uint32 j = 0; j < sai1->SlotCount; ++j)
                {
                    if(id1 == sai2->PIDArray[j])
                    {
                        AssertSnapEquivTTDVar_SlotArray(sai1->Slots[i], sai2->Slots[j], compareMap, i);
                        found = true;
                        break;
                    }
                }
                //TODO: We see this hit in a case where record has all values in a slot array when replaying --replay-debug (but not --replay).
                //      In the debug version the propertyId is in a Js::DebuggerScopeProperty instead. So this needs to be investigated in both extract and inflate.
                compareMap.DiagnosticAssert(found);
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
                    TTDAssert(false, "Unknown scope kind");
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

        void ExtractSnapFunctionBodyScopeChain(bool isWellKnownFunction, SnapFunctionBodyScopeChain& scopeChain, Js::FunctionBody* fb, SlabAllocator& alloc)
        {
            scopeChain.ScopeCount = 0;
            scopeChain.ScopeArray = nullptr;

            if(!isWellKnownFunction && fb->GetScopeObjectChain() != nullptr)
            {
                Js::ScopeObjectChain* scChain = fb->GetScopeObjectChain();
                scopeChain.ScopeCount = (uint32)scChain->pScopeChain->Count();

                if(scopeChain.ScopeCount == 0)
                {
                    scopeChain.ScopeArray = nullptr;
                }
                else
                {
                    scopeChain.ScopeArray = alloc.SlabAllocateArray<TTD_PTR_ID>(scopeChain.ScopeCount);

                    for(int32 i = 0; i < scChain->pScopeChain->Count(); ++i)
                    {
                        Js::DebuggerScope* dbgScope = scChain->pScopeChain->Item(i);
                        scopeChain.ScopeArray[i] = TTD_CONVERT_DEBUGSCOPE_TO_PTR_ID(dbgScope);
                    }
                }
            }
        }

        void EmitSnapFunctionBodyScopeChain(const SnapFunctionBodyScopeChain& scopeChain, FileWriter* writer)
        {
            writer->WriteRecordStart();

            writer->WriteLengthValue(scopeChain.ScopeCount);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < scopeChain.ScopeCount; ++i)
            {
                writer->WriteNakedAddr(scopeChain.ScopeArray[i], (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();

            writer->WriteRecordEnd();
        }

        void ParseSnapFunctionBodyScopeChain(SnapFunctionBodyScopeChain& scopeChain, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart();

            scopeChain.ScopeCount = reader->ReadLengthValue();
            scopeChain.ScopeArray = (scopeChain.ScopeCount != 0) ? alloc.SlabAllocateArray<TTD_PTR_ID>(scopeChain.ScopeCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < scopeChain.ScopeCount; ++i)
            {
                scopeChain.ScopeArray[i] = reader->ReadNakedAddr(i != 0);
            }
            reader->ReadSequenceEnd();

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv(const SnapFunctionBodyScopeChain& chain1, const SnapFunctionBodyScopeChain& chain2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(chain1.ScopeCount == chain2.ScopeCount);

            //Not sure if there is a way to compare the pointer ids in the two scopes
        }
#endif

        //////////////////

        void ExtractTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, uint64 sourceContextId, bool isUtf8source, const byte* source, uint32 sourceLen, SlabAllocator& alloc)
        {
            fbInfo->ScriptContextLogId = fb->GetScriptContext()->ScriptContextLogTag;
            fbInfo->TopLevelBodyCtr = topLevelCtr;

            alloc.CopyNullTermStringInto(fb->GetDisplayName(), fbInfo->FunctionName);

            fbInfo->ModuleId = moduleId;
            fbInfo->SourceContextId = sourceContextId;
            alloc.CopyNullTermStringInto(fb->GetSourceContextInfo()->url, fbInfo->SourceUri);

            fbInfo->IsUtf8 = isUtf8source;
            fbInfo->ByteLength = sourceLen;
            fbInfo->SourceBuffer = alloc.SlabAllocateArray<byte>(fbInfo->ByteLength);
            js_memcpy_s(fbInfo->SourceBuffer, fbInfo->ByteLength, source, sourceLen);

            fbInfo->DbgSerializedBytecodeSize = 0;
            fbInfo->DbgSerializedBytecodeBuffer = nullptr;

            ExtractSnapFunctionBodyScopeChain(false, fbInfo->ScopeChainInfo, fb, alloc);
        }

        void EmitTopLevelCommonBodyResolveInfo(const TopLevelCommonBodyResolveInfo* fbInfo, bool emitInline, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);
            writer->WriteUInt32(NSTokens::Key::functionBodyId, fbInfo->TopLevelBodyCtr);
            writer->WriteLogTag(NSTokens::Key::ctxTag, fbInfo->ScriptContextLogId, NSTokens::Separator::CommaSeparator);

            writer->WriteString(NSTokens::Key::name, fbInfo->FunctionName, NSTokens::Separator::CommaSeparator);

            writer->WriteUInt64(NSTokens::Key::moduleId, fbInfo->ModuleId, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::sourceContextId, fbInfo->SourceContextId, NSTokens::Separator::CommaSeparator);
            writer->WriteString(NSTokens::Key::uri, fbInfo->SourceUri, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, fbInfo->IsUtf8, NSTokens::Separator::CommaSeparator);
            writer->WriteLengthValue(fbInfo->ByteLength, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::scopeChain, NSTokens::Separator::CommaSeparator);
            EmitSnapFunctionBodyScopeChain(fbInfo->ScopeChainInfo, writer);

            if(emitInline || IsNullPtrTTString(fbInfo->SourceUri))
            {
                TTDAssert(!fbInfo->IsUtf8, "Should only emit char16 encoded data in inline mode.");

                writer->WriteInlineCode((char16*)fbInfo->SourceBuffer, fbInfo->ByteLength / sizeof(char16), NSTokens::Separator::CommaSeparator);
            }
            else
            {
                JsSupport::WriteCodeToFile(threadContext, false, fbInfo->TopLevelBodyCtr, fbInfo->IsUtf8, fbInfo->SourceBuffer, fbInfo->ByteLength);
            }
        }

        void ParseTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, bool readSeperator, bool parseInline, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);
            fbInfo->TopLevelBodyCtr = reader->ReadUInt32(NSTokens::Key::functionBodyId);
            fbInfo->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);

            reader->ReadString(NSTokens::Key::name, alloc, fbInfo->FunctionName, true);

            fbInfo->ModuleId = (Js::ModuleID)reader->ReadUInt64(NSTokens::Key::moduleId, true);
            fbInfo->SourceContextId = reader->ReadUInt64(NSTokens::Key::sourceContextId, true);
            reader->ReadString(NSTokens::Key::uri, alloc, fbInfo->SourceUri, true);

            fbInfo->IsUtf8 = reader->ReadBool(NSTokens::Key::boolVal, true);
            fbInfo->ByteLength = reader->ReadLengthValue(true);
            fbInfo->SourceBuffer = alloc.SlabAllocateArray<byte>(fbInfo->ByteLength);

            reader->ReadKey(NSTokens::Key::scopeChain, true);
            ParseSnapFunctionBodyScopeChain(fbInfo->ScopeChainInfo, reader, alloc);

            if(parseInline || IsNullPtrTTString(fbInfo->SourceUri))
            {
                TTDAssert(!fbInfo->IsUtf8, "Should only emit char16 encoded data in inline mode.");

                reader->ReadInlineCode((char16*)fbInfo->SourceBuffer, fbInfo->ByteLength / sizeof(char16), true);
            }
            else
            {
                JsSupport::ReadCodeFromFile(threadContext, false, fbInfo->TopLevelBodyCtr, fbInfo->IsUtf8, fbInfo->SourceBuffer, fbInfo->ByteLength);
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

            compareMap.DiagnosticAssert(fbInfo1->ModuleId == fbInfo2->ModuleId);
            compareMap.DiagnosticAssert(fbInfo1->SourceContextId == fbInfo2->SourceContextId);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(fbInfo1->SourceUri, fbInfo2->SourceUri));

            compareMap.DiagnosticAssert(fbInfo1->IsUtf8 == fbInfo2->IsUtf8);
            compareMap.DiagnosticAssert(fbInfo1->ByteLength == fbInfo2->ByteLength);

            for(uint32 i = 0; i < fbInfo1->ByteLength; ++i)
            {
                compareMap.DiagnosticAssert(fbInfo1->SourceBuffer[i] == fbInfo2->SourceBuffer[i]);
            }

            AssertSnapEquiv(fbInfo1->ScopeChainInfo, fbInfo2->ScopeChainInfo, compareMap);
        }
#endif

        ////
        //Regular script-load functions

        void ExtractTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, uint64 sourceContextId, bool isUtf8, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag, SlabAllocator& alloc)
        {
            NSSnapValues::ExtractTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, fb, topLevelCtr, moduleId, sourceContextId, isUtf8, source, sourceLen, alloc);

            fbInfo->LoadFlag = loadFlag;
        }

        Js::FunctionBody* InflateTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx)
        {
            byte* script = fbInfo->TopLevelBase.SourceBuffer;
            uint32 scriptLength = fbInfo->TopLevelBase.ByteLength;
            uint64 sourceContext = fbInfo->TopLevelBase.SourceContextId;

            TTDAssert(ctx->GetSourceContextInfo((DWORD_PTR)sourceContext, nullptr) == nullptr, "On inflate we should either have clean ctxts or we want to optimize the inflate process by skipping redoing this work!!!");
            TTDAssert(fbInfo->TopLevelBase.IsUtf8 == ((fbInfo->LoadFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source), "Utf8 status is inconsistent!!!");

            const char16* srcUri = fbInfo->TopLevelBase.SourceUri.Contents;
            uint32 srcUriLength = fbInfo->TopLevelBase.SourceUri.Length;

            SourceContextInfo * sourceContextInfo = ctx->CreateSourceContextInfo((DWORD_PTR)sourceContext, srcUri, srcUriLength, nullptr);

            TTDAssert(fbInfo->TopLevelBase.IsUtf8 || sizeof(wchar) == sizeof(char16), "Non-utf8 code only allowed on windows!!!");
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
                TTDAssert(false, "Not implemented yet -- this branch should never be taken.");
            }
            else
            {
                scriptFunction = ctx->LoadScript(script, scriptLength, &si, &se, &utf8SourceInfo, Js::Constants::GlobalCode, fbInfo->LoadFlag, nullptr);
                TTDAssert(scriptFunction != nullptr, "Something went wrong");

                globalBody = TTD::JsSupport::ForceAndGetFunctionBody(scriptFunction->GetParseableFunctionInfo());

                //
                //TODO: Bytecode serializer does not suppper debug bytecode (StatementMaps vs Positions) so add this to serializer code.
                //      Then we can add code to do optimized bytecode reload here.
                //
            }

            ////
            //We don't do this automatically in the load script helper so do it here
            BEGIN_JS_RUNTIME_CALL(ctx);
            {
                ctx->TTDContextInfo->ProcessFunctionBodyOnLoad(globalBody, nullptr);
                ctx->TTDContextInfo->RegisterLoadedScript(globalBody, fbInfo->TopLevelBase.TopLevelBodyCtr);

                globalBody->GetUtf8SourceInfo()->SetSourceInfoForDebugReplay_TTD(fbInfo->TopLevelBase.TopLevelBodyCtr);
            }
            END_JS_RUNTIME_CALL(ctx);

            bool isLibraryCode = ((fbInfo->LoadFlag & LoadScriptFlag_LibraryCode) == LoadScriptFlag_LibraryCode);
            if(ctx->GetThreadContext()->TTDExecutionInfo != nullptr && !isLibraryCode)
            {
                ctx->GetThreadContext()->TTDExecutionInfo->ProcessScriptLoad(ctx, fbInfo->TopLevelBase.TopLevelBodyCtr, globalBody, utf8SourceInfo, &se);
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

        void ExtractTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, SlabAllocator& alloc)
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
            TTDAssert(pfuncScript != nullptr, "Something went wrong!!!");

            // Indicate that this is a top-level function. We don't pass the fscrGlobalCode flag to the eval helper,
            // or it will return the global function that wraps the declared function body, as though it were an eval.
            // But we want, for instance, to be able to verify that we did the right amount of deferred parsing.
            Js::ParseableFunctionInfo* functionInfo = pfuncScript->GetParseableFunctionInfo();
            functionInfo->SetGrfscr(functionInfo->GetGrfscr() | fscrGlobalCode);

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

        void ExtractTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode, SlabAllocator& alloc)
        {
            NSSnapValues::ExtractTopLevelCommonBodyResolveInfo(&fbInfo->TopLevelBase, fb, topLevelCtr, moduleId, 0, false, (const byte*)source, sourceLen * sizeof(char16), alloc);

            fbInfo->EvalFlags = grfscr;
            fbInfo->RegisterDocument = registerDocument;
            fbInfo->IsIndirect = !!isIndirect;
            fbInfo->IsStrictMode = !!strictMode;
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
            TTDAssert(wcscmp(fbInfo->FunctionName.Contents, Js::Constants::GlobalCode) != 0, "Why are we snapshotting global code??");

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
                TTDAssert(parentBody != nullptr, "We missed something!!!");

                fbInfo->OptParentBodyId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(parentBody);
                fbInfo->OptLine = fb->GetLineNumber();
                fbInfo->OptColumn = fb->GetColumnNumber();
            }

            ExtractSnapFunctionBodyScopeChain(fbInfo->OptKnownPath != TTD_INVALID_WELLKNOWN_TOKEN, fbInfo->ScopeChainInfo, fb, alloc);
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

                    //
                    //TODO: this is a potentially expensive linear search (but needed since classes dump implicit functions out-of-text order).
                    //      May want to add sort and save in inflator or our shaddow info in script context if this is looking expensive.
                    //
                    uint32 blength = parentBody->GetNestedCount();
                    for(uint32 i = 0; i < blength; ++i)
                    {
                        Js::ParseableFunctionInfo* pfi = parentBody->GetNestedFunctionForExecution(i);
                        Js::FunctionBody* currfb = JsSupport::ForceAndGetFunctionBody(pfi);

                        if(fbInfo->OptLine == currfb->GetLineNumber() && fbInfo->OptColumn == currfb->GetColumnNumber())
                        {
                            resfb = currfb;
                            break;
                        }
                    }

                    TTDAssert(resfb != nullptr && fbInfo->OptLine == resfb->GetLineNumber() && fbInfo->OptColumn == resfb->GetColumnNumber(), "We are missing something");
                    TTDAssert(resfb != nullptr && (wcscmp(fbInfo->FunctionName.Contents, resfb->GetDisplayName()) == 0 || wcscmp(_u("get"), resfb->GetDisplayName()) == 0 || wcscmp(_u("set"), resfb->GetDisplayName()) == 0), "We are missing something");
                }

                //Make sure to register any scopes the found function body has (but *not* for well known functions)
                inflator->UpdateFBScopes(fbInfo->ScopeChainInfo, resfb);
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
                uint32 suffixWDotPos = (fbInfo->FunctionName.Length - 4);
                uint32 suffixPos = (fbInfo->FunctionName.Length - 3);

                TTDAssert(wcsstr(fbInfo->FunctionName.Contents, _u(".get")) == (fbInfo->FunctionName.Contents + suffixWDotPos) || wcsstr(fbInfo->FunctionName.Contents, _u(".set")) == (fbInfo->FunctionName.Contents + suffixWDotPos), "Does not start with get or set");

                resfb->SetDisplayName(fbInfo->FunctionName.Contents, fbInfo->FunctionName.Length, suffixPos, Js::FunctionProxy::SetDisplayNameFlagsRecyclerAllocated);
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

            writer->WriteKey(NSTokens::Key::scopeChain, NSTokens::Separator::CommaSeparator);
            EmitSnapFunctionBodyScopeChain(fbInfo->ScopeChainInfo, writer);

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

            reader->ReadKey(NSTokens::Key::scopeChain, true);
            ParseSnapFunctionBodyScopeChain(fbInfo->ScopeChainInfo, reader, alloc);

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

            AssertSnapEquiv(fbInfo1->ScopeChainInfo, fbInfo1->ScopeChainInfo, compareMap);
        }
#endif

        //////////////////

        void ExtractScriptContext(SnapContext* snapCtx, Js::ScriptContext* ctx, const JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator>& objToLogIdMap, const JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies, SlabAllocator& alloc)
        {
            snapCtx->ScriptContextLogId = ctx->ScriptContextLogTag;

            snapCtx->IsPNRGSeeded = ctx->GetLibrary()->IsPRNGSeeded();
            snapCtx->RandomSeed0 = ctx->GetLibrary()->GetRandSeed0();
            snapCtx->RandomSeed1 = ctx->GetLibrary()->GetRandSeed1();
            alloc.CopyNullTermStringInto(ctx->GetUrl(), snapCtx->ContextSRC);

            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelScriptLoad(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelNewFunction(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelEval(&HeapAllocator::Instance);

            ctx->TTDContextInfo->GetLoadedSources(&liveTopLevelBodies, topLevelScriptLoad, topLevelNewFunction, topLevelEval);

            snapCtx->LoadedTopLevelScriptCount = topLevelScriptLoad.Count();
            if(snapCtx->LoadedTopLevelScriptCount == 0)
            {
                snapCtx->LoadedTopLevelScriptArray = nullptr;
            }
            else
            {
                snapCtx->LoadedTopLevelScriptArray = alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->LoadedTopLevelScriptCount);
                for(int32 i = 0; i < topLevelScriptLoad.Count(); ++i)
                {
                    snapCtx->LoadedTopLevelScriptArray[i] = topLevelScriptLoad.Item(i);
                }
            }

            snapCtx->NewFunctionTopLevelScriptCount = topLevelNewFunction.Count();
            if(snapCtx->NewFunctionTopLevelScriptCount == 0)
            {
                snapCtx->NewFunctionTopLevelScriptArray = nullptr;
            }
            else
            {
                snapCtx->NewFunctionTopLevelScriptArray = alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->NewFunctionTopLevelScriptCount);
                for(int32 i = 0; i < topLevelNewFunction.Count(); ++i)
                {
                    snapCtx->NewFunctionTopLevelScriptArray[i] = topLevelNewFunction.Item(i);
                }
            }

            snapCtx->EvalTopLevelScriptCount = topLevelEval.Count();
            if(snapCtx->EvalTopLevelScriptCount == 0)
            {
                snapCtx->EvalTopLevelScriptArray = nullptr;
            }
            else
            {
                snapCtx->EvalTopLevelScriptArray = alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(snapCtx->EvalTopLevelScriptCount);
                for(int32 i = 0; i < topLevelEval.Count(); ++i)
                {
                    snapCtx->EvalTopLevelScriptArray[i] = topLevelEval.Item(i);
                }
            }

            //Extract pending async modification info
            const JsUtil::List<TTDPendingAsyncBufferModification, HeapAllocator>& pendingAsyncList = ctx->TTDContextInfo->GetPendingAsyncModListForSnapshot();
            snapCtx->PendingAsyncModCount = pendingAsyncList.Count();
            if(snapCtx->PendingAsyncModCount == 0)
            {
                snapCtx->PendingAsyncModArray = nullptr;
            }
            else
            {
                snapCtx->PendingAsyncModArray = alloc.SlabAllocateArray<SnapPendingAsyncBufferModification>(snapCtx->PendingAsyncModCount);

                for(int32 k = 0; k < pendingAsyncList.Count(); ++k)
                {
                    const TTDPendingAsyncBufferModification& pk = pendingAsyncList.Item(k);
                    snapCtx->PendingAsyncModArray[k].LogId = objToLogIdMap.Item(Js::RecyclableObject::FromVar(pk.ArrayBufferVar));
                    snapCtx->PendingAsyncModArray[k].Index = pk.Index;
                }
            }
        }

        void InflateScriptContext(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator,
            const TTDIdentifierDictionary<uint64, TopLevelScriptLoadFunctionBodyResolveInfo*>& topLevelLoadScriptMap,
            const TTDIdentifierDictionary<uint64, TopLevelNewFunctionBodyResolveInfo*>& topLevelNewScriptMap,
            const TTDIdentifierDictionary<uint64, TopLevelEvalFunctionBodyResolveInfo*>& topLevelEvalScriptMap)
        {
            TTDAssert(wcscmp(snpCtx->ContextSRC.Contents, intoCtx->GetUrl()) == 0, "Make sure the src uri values are the same.");

            intoCtx->GetLibrary()->SetIsPRNGSeeded(snpCtx->IsPNRGSeeded);
            intoCtx->GetLibrary()->SetRandSeed0(snpCtx->RandomSeed0);
            intoCtx->GetLibrary()->SetRandSeed1(snpCtx->RandomSeed1);
            inflator->AddScriptContext(snpCtx->ScriptContextLogId, intoCtx);

            intoCtx->TTDContextInfo->ClearLoadedSourcesForSnapshotRestore();

            if(intoCtx->HasRecordedException())
            {
                intoCtx->GetAndClearRecordedException(nullptr);
            }

            for(uint32 i = 0; i < snpCtx->LoadedTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation& cri = snpCtx->LoadedTopLevelScriptArray[i];

                Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo = topLevelLoadScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);

                if(fb == nullptr)
                {
                    fb = NSSnapValues::InflateTopLevelLoadedFunctionBodyInfo(fbInfo, intoCtx);
                }
                else
                {
                    intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                    intoCtx->TTDContextInfo->RegisterLoadedScript(fb, cri.TopLevelBodyCtr);

                    intoCtx->GetThreadContext()->TTDExecutionInfo->ProcessScriptLoad_InflateReuseBody(cri.TopLevelBodyCtr, fb);
                }

                inflator->UpdateFBScopes(fbInfo->TopLevelBase.ScopeChainInfo, fb);
                inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
            }

            //The inflation code for NewFunction and Eval uses the paths in the runtime (which assume they are in script) -- so enter here to make them happy
            BEGIN_ENTER_SCRIPT(intoCtx, true, true, true)
            {
                for(uint32 i = 0; i < snpCtx->NewFunctionTopLevelScriptCount; ++i)
                {
                    const TopLevelFunctionInContextRelation& cri = snpCtx->NewFunctionTopLevelScriptArray[i];

                    Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                    const TopLevelNewFunctionBodyResolveInfo* fbInfo = topLevelNewScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);

                    if(fb == nullptr)
                    {
                        fb = NSSnapValues::InflateTopLevelNewFunctionBodyInfo(fbInfo, intoCtx);
                    }
                    else
                    {
                        intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                        intoCtx->TTDContextInfo->RegisterNewScript(fb, cri.TopLevelBodyCtr);

                        intoCtx->GetThreadContext()->TTDExecutionInfo->ProcessScriptLoad_InflateReuseBody(cri.TopLevelBodyCtr, fb);
                    }

                    inflator->UpdateFBScopes(fbInfo->TopLevelBase.ScopeChainInfo, fb);
                    inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
                }

                for(uint32 i = 0; i < snpCtx->EvalTopLevelScriptCount; ++i)
                {
                    const TopLevelFunctionInContextRelation& cri = snpCtx->EvalTopLevelScriptArray[i];

                    Js::FunctionBody* fb = inflator->FindReusableFunctionBodyIfExists(cri.ContextSpecificBodyPtrId);
                    const TopLevelEvalFunctionBodyResolveInfo* fbInfo = topLevelEvalScriptMap.LookupKnownItem(cri.TopLevelBodyCtr);

                    if(fb == nullptr)
                    {
                        fb = NSSnapValues::InflateTopLevelEvalFunctionBodyInfo(fbInfo, intoCtx);
                    }
                    else
                    {
                        intoCtx->TTDContextInfo->ProcessFunctionBodyOnLoad(fb, nullptr);
                        intoCtx->TTDContextInfo->RegisterEvalScript(fb, cri.TopLevelBodyCtr);

                        intoCtx->GetThreadContext()->TTDExecutionInfo->ProcessScriptLoad_InflateReuseBody(cri.TopLevelBodyCtr, fb);
                    }

                    inflator->UpdateFBScopes(fbInfo->TopLevelBase.ScopeChainInfo, fb);
                    inflator->AddInflationFunctionBody(cri.ContextSpecificBodyPtrId, fb);
                }
            }
            END_ENTER_SCRIPT
        }

        void ResetPendingAsyncBufferModInfo(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator)
        {
            intoCtx->TTDContextInfo->ClearPendingAsyncModListForSnapRestore();

            for(uint32 i = 0; i < snpCtx->PendingAsyncModCount; ++i)
            {
                Js::RecyclableObject* buff = intoCtx->GetThreadContext()->TTDContext->LookupObjectForLogID(snpCtx->PendingAsyncModArray[i].LogId);
                uint32 index = snpCtx->PendingAsyncModArray[i].Index;

                TTDAssert(Js::ArrayBuffer::Is(buff), "Not an ArrayBuffer!!!");
                intoCtx->TTDContextInfo->AddToAsyncPendingList(Js::ArrayBuffer::FromVar(buff), index);
            }
        }

        void EmitSnapContext(const SnapContext* snapCtx, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteLogTag(NSTokens::Key::ctxTag, snapCtx->ScriptContextLogId);
            writer->WriteBool(NSTokens::Key::boolVal, snapCtx->IsPNRGSeeded, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::u64Val, snapCtx->RandomSeed0, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt64(NSTokens::Key::u64Val, snapCtx->RandomSeed1, NSTokens::Separator::CommaSeparator);
            writer->WriteString(NSTokens::Key::ctxUri, snapCtx->ContextSRC, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(snapCtx->LoadedTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->LoadedTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->LoadedTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt32(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->NewFunctionTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->NewFunctionTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->NewFunctionTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt32(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->EvalTopLevelScriptCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->EvalTopLevelScriptCount; ++i)
            {
                const TopLevelFunctionInContextRelation* cri = snapCtx->EvalTopLevelScriptArray + i;
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;

                writer->WriteRecordStart(sep);
                writer->WriteUInt32(NSTokens::Key::bodyCounterId, cri->TopLevelBodyCtr);
                writer->WriteAddr(NSTokens::Key::functionBodyId, cri->ContextSpecificBodyPtrId, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(snapCtx->PendingAsyncModCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < snapCtx->PendingAsyncModCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                writer->WriteRecordStart(sep);
                writer->WriteLogTag(NSTokens::Key::logTag, snapCtx->PendingAsyncModArray[i].LogId);
                writer->WriteUInt32(NSTokens::Key::u32Val, snapCtx->PendingAsyncModArray[i].Index, NSTokens::Separator::CommaSeparator);
                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            writer->WriteRecordEnd();
        }

        void ParseSnapContext(SnapContext* intoCtx, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            intoCtx->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag);
            intoCtx->IsPNRGSeeded = reader->ReadBool(NSTokens::Key::boolVal, true);
            intoCtx->RandomSeed0 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            intoCtx->RandomSeed1 = reader->ReadUInt64(NSTokens::Key::u64Val, true);
            reader->ReadString(NSTokens::Key::ctxUri, alloc, intoCtx->ContextSRC, true);

            intoCtx->LoadedTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->LoadedTopLevelScriptArray = (intoCtx->LoadedTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->LoadedTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->LoadedTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->LoadedTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt32(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->NewFunctionTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->NewFunctionTopLevelScriptArray = (intoCtx->NewFunctionTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->NewFunctionTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->NewFunctionTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->NewFunctionTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt32(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->EvalTopLevelScriptCount = reader->ReadLengthValue(true);
            intoCtx->EvalTopLevelScriptArray = (intoCtx->EvalTopLevelScriptCount != 0) ? alloc.SlabAllocateArray<TopLevelFunctionInContextRelation>(intoCtx->EvalTopLevelScriptCount) : nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->EvalTopLevelScriptCount; ++i)
            {
                TopLevelFunctionInContextRelation* cri = intoCtx->EvalTopLevelScriptArray + i;

                reader->ReadRecordStart(i != 0);
                cri->TopLevelBodyCtr = reader->ReadUInt32(NSTokens::Key::bodyCounterId);
                cri->ContextSpecificBodyPtrId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            intoCtx->PendingAsyncModCount = reader->ReadLengthValue(true);
            intoCtx->PendingAsyncModArray = (intoCtx->PendingAsyncModCount != 0) ? alloc.SlabAllocateArray<SnapPendingAsyncBufferModification>(intoCtx->PendingAsyncModCount) : nullptr;

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < intoCtx->PendingAsyncModCount; ++i)
            {
                reader->ReadRecordStart(i != 0);
                intoCtx->PendingAsyncModArray[i].LogId = reader->ReadLogTag(NSTokens::Key::logTag);
                intoCtx->PendingAsyncModArray[i].Index = reader->ReadUInt32(NSTokens::Key::u32Val, true);
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapContext* snapCtx1, const SnapContext* snapCtx2, const JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator>& allRootMap1, const JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator>& allRootMap2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(snapCtx1->ScriptContextLogId == snapCtx2->ScriptContextLogId);

            compareMap.DiagnosticAssert(snapCtx1->IsPNRGSeeded == snapCtx2->IsPNRGSeeded);
            compareMap.DiagnosticAssert(snapCtx1->RandomSeed0 == snapCtx2->RandomSeed0);
            compareMap.DiagnosticAssert(snapCtx1->RandomSeed1 == snapCtx2->RandomSeed1);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(snapCtx1->ContextSRC, snapCtx2->ContextSRC));

            //
            //TODO: For now just sanity check the number of top-level functions and let the FunctionBody matching drive any matching.
            //

            compareMap.DiagnosticAssert(snapCtx1->LoadedTopLevelScriptCount == snapCtx2->LoadedTopLevelScriptCount);
            //TopLevelScriptLoadFunctionBodyResolveInfo* m_loadedScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->NewFunctionTopLevelScriptCount == snapCtx2->NewFunctionTopLevelScriptCount);
            //TopLevelNewFunctionBodyResolveInfo* m_newScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->EvalTopLevelScriptCount == snapCtx2->EvalTopLevelScriptCount);
            //TopLevelEvalFunctionBodyResolveInfo* m_evalScriptArray;

            compareMap.DiagnosticAssert(snapCtx1->PendingAsyncModCount == snapCtx2->PendingAsyncModCount);

            for(uint32 i = 0; i < snapCtx1->PendingAsyncModCount; ++i)
            {
                const SnapPendingAsyncBufferModification& pendEntry1 = snapCtx1->PendingAsyncModArray[i];
                const SnapPendingAsyncBufferModification& pendEntry2 = snapCtx2->PendingAsyncModArray[i];

                compareMap.DiagnosticAssert(pendEntry1.LogId == pendEntry2.LogId && pendEntry1.Index == pendEntry2.Index);

                compareMap.H1PendingAsyncModBufferSet.AddNew(allRootMap1.Item(pendEntry1.LogId)->LogObject);
                compareMap.H2PendingAsyncModBufferSet.AddNew(allRootMap2.Item(pendEntry2.LogId)->LogObject);
            }
        }
#endif
    }
}

#endif
