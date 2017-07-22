//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    namespace NSSnapObjects
    {
        void ExtractCompoundObject(NSSnapObjects::SnapObject* sobj, Js::RecyclableObject* obj, bool isWellKnown, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc)
        {
            TTDAssert(!obj->CanHaveInterceptors(), "We are not prepared for custom external objects yet");

            sobj->ObjectPtrId = TTD_CONVERT_VAR_TO_PTR_ID(obj);
            sobj->SnapObjectTag = obj->GetSnapTag_TTD();

            TTD_WELLKNOWN_TOKEN lookupToken = isWellKnown ? obj->GetScriptContext()->TTDWellKnownInfo->ResolvePathForKnownObject(obj) : TTD_INVALID_WELLKNOWN_TOKEN;
            sobj->OptWellKnownToken = alloc.CopyRawNullTerminatedStringInto(lookupToken);

            Js::Type* objType = obj->GetType();
            sobj->SnapType = idToTypeMap.LookupKnownItem(TTD_CONVERT_TYPEINFO_TO_PTR_ID(objType));

#if ENABLE_OBJECT_SOURCE_TRACKING
            InitializeDiagnosticOriginInformation(sobj->DiagOriginInfo);
#endif

            if(Js::StaticType::Is(objType->GetTypeId()))
            {
                NSSnapObjects::StdPropertyExtract_StaticType(sobj, obj);
            }
            else
            {
                NSSnapObjects::StdPropertyExtract_DynamicType(sobj, static_cast<Js::DynamicObject*>(obj), alloc);
            }

            obj->ExtractSnapObjectDataInto(sobj, alloc);
        }

        void StdPropertyExtract_StaticType(SnapObject* snpObject, Js::RecyclableObject* obj)
        {
            snpObject->IsCrossSite = FALSE;

            snpObject->VarArrayCount = 0;
            snpObject->VarArray = nullptr;

            snpObject->OptIndexedObjectArray = TTD_INVALID_PTR_ID;

            snpObject->OptDependsOnInfo = nullptr;
            //AddtlSnapObjectInfo must be set later in type specific extract code
        }

        void StdPropertyExtract_DynamicType(SnapObject* snpObject, Js::DynamicObject* dynObj, SlabAllocator& alloc)
        {
            NSSnapType::SnapType* sType = snpObject->SnapType;

            snpObject->IsCrossSite = dynObj->IsCrossSiteObject();

#if ENABLE_OBJECT_SOURCE_TRACKING
            CopyDiagnosticOriginInformation(snpObject->DiagOriginInfo, dynObj->TTDDiagOriginInfo);
#endif

            if(sType->TypeHandlerInfo->MaxPropertyIndex == 0)
            {
                snpObject->VarArrayCount = 0;
                snpObject->VarArray = nullptr;
            }
            else
            {
                NSSnapType::SnapHandler* sHandler = sType->TypeHandlerInfo;

                static_assert(sizeof(TTDVar) == sizeof(Js::Var), "These need to be the same size (and have same bit layout) for this to work!");

                snpObject->VarArrayCount = sHandler->MaxPropertyIndex;
                snpObject->VarArray = alloc.SlabAllocateArray<TTDVar>(snpObject->VarArrayCount);

                TTDVar* cpyBase = snpObject->VarArray;
                if(sHandler->InlineSlotCapacity != 0)
                {
                    Js::Var const* inlineSlots = dynObj->GetInlineSlots_TTD();

                    //copy all the properties (if they all fit into the inline slots) otherwise just copy all the inline slot values
                    uint32 inlineSlotCount = min(sHandler->MaxPropertyIndex, sHandler->InlineSlotCapacity);
                    js_memcpy_s(cpyBase, inlineSlotCount * sizeof(TTDVar), inlineSlots, inlineSlotCount * sizeof(Js::Var));
                }

                if(sHandler->MaxPropertyIndex > sHandler->InlineSlotCapacity)
                {
                    cpyBase = cpyBase + sHandler->InlineSlotCapacity;
                    Js::Var const* auxSlots = dynObj->GetAuxSlots_TTD();

                    //there are some values in aux slots (in addition to the inline slots) so copy them as well
                    uint32 auxSlotCount = (sHandler->MaxPropertyIndex - sHandler->InlineSlotCapacity);
                    js_memcpy_s(cpyBase, auxSlotCount * sizeof(TTDVar), auxSlots, auxSlotCount * sizeof(Js::Var));
                }
            }

            Js::ArrayObject* parray = dynObj->GetObjectArray();
            snpObject->OptIndexedObjectArray = (parray == nullptr) ? TTD_INVALID_PTR_ID : TTD_CONVERT_VAR_TO_PTR_ID(parray);

            snpObject->OptDependsOnInfo = nullptr;
            //AddtlSnapObjectInfo must be set later in type specific extract code
        }

        Js::DynamicObject* ReuseObjectCheckAndReset(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::RecyclableObject* robj = inflator->FindReusableObjectIfExists(snpObject->ObjectPtrId);
            if(robj == nullptr || Js::DynamicObject::FromVar(robj)->GetTypeId() != snpObject->SnapType->JsTypeId || Js::DynamicObject::FromVar(robj)->IsCrossSiteObject() != snpObject->IsCrossSite)
            {
                return nullptr;
            }
            TTDAssert(Js::DynamicType::Is(robj->GetTypeId()), "You should only do this for dynamic objects!!!");

            Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(robj);
            return ObjectPropertyReset_General(snpObject, dynObj, inflator);
        }

        bool DoesObjectBlockScriptContextReuse(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator)
        {
            TTDAssert(snpObject->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN, "Only well known objects can block re-use so check that before calling this.");

            JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& propertyReset = inflator->GetPropertyResetSet();
            propertyReset.Clear();

            ////
            for(int32 i = 0; i < dynObj->GetPropertyCount(); i++)
            {
                Js::PropertyId pid = dynObj->GetPropertyId((Js::PropertyIndex)i);
                if(pid != Js::Constants::NoProperty)
                {
                    propertyReset.AddNew(pid);
                }
            }

            const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;
            for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
            {
                BOOL willOverwriteLater = (handler->PropertyInfoArray[i].DataKind != NSSnapType::SnapEntryDataKindTag::Clear);
                BOOL isInternal = Js::IsInternalPropertyId(handler->PropertyInfoArray[i].PropertyRecordId);

                if(willOverwriteLater | isInternal)
                {
                    Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;
                    propertyReset.Remove(pid);
                }
            }

            if(propertyReset.Count() != 0)
            {
                for(auto iter = propertyReset.GetIterator(); iter.IsValid(); iter.MoveNext())
                {
                    Js::PropertyId pid = iter.CurrentValue();
                    TTDAssert(pid != Js::Constants::NoProperty, "This shouldn't happen!!!");

                    //We don't like trying to reset these
                    if(Js::IsInternalPropertyId(pid))
                    {
                        propertyReset.Clear();
                        return true; 
                    }

                    //someone added a property that is not simple to remove so let's just be safe an recreate contexts
                    if(!dynObj->IsConfigurable(pid))
                    {
                        propertyReset.Clear();
                        return true;
                    }
                }
            }

            propertyReset.Clear();
            ////

            return false;
        }

        Js::DynamicObject* ObjectPropertyReset_WellKnown(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator)
        {
            TTDAssert(snpObject->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN, "Should only call this on well known objects.");

            JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& propertyReset = inflator->GetPropertyResetSet();
            propertyReset.Clear();

            ////
            for(int32 i = 0; i < dynObj->GetPropertyCount(); i++)
            {
                Js::PropertyId pid = dynObj->GetPropertyId((Js::PropertyIndex)i);
                if(pid != Js::Constants::NoProperty)
                {
                    propertyReset.AddNew(pid);
                }
            }

            const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;
            for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
            {
                BOOL willOverwriteLater = (handler->PropertyInfoArray[i].DataKind != NSSnapType::SnapEntryDataKindTag::Clear);
                BOOL isInternal = Js::IsInternalPropertyId(handler->PropertyInfoArray[i].PropertyRecordId);

                if(willOverwriteLater | isInternal)
                {
                    Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;
                    propertyReset.Remove(pid);
                }
            }

            Js::Var undefined = dynObj->GetScriptContext()->GetLibrary()->GetUndefined();
            if(propertyReset.Count() != 0)
            {
                for(auto iter = propertyReset.GetIterator(); iter.IsValid(); iter.MoveNext())
                {
                    BOOL ok = FALSE;
                    Js::PropertyId pid = iter.CurrentValue();
                    TTDAssert(pid != Js::Constants::NoProperty && !Js::IsInternalPropertyId(pid), "This shouldn't happen!!!");

                    if(!dynObj->IsConfigurable(pid))
                    {
                        ok = dynObj->SetProperty(pid, undefined, Js::PropertyOperationFlags::PropertyOperation_Force, nullptr);
                    }
                    else
                    {
                        ok = dynObj->DeleteProperty(pid, Js::PropertyOperationFlags::PropertyOperation_Force);
                    }

                    TTDAssert(ok, "This property is stuck!!!");
                }
            }

            propertyReset.Clear();
            ////

            //always reset the index array as this is unusual and annoying to iterate over a bunch
            Js::ArrayObject* parray = dynObj->GetObjectArray();
            if(parray != nullptr)
            {
                Js::JavascriptArray* newArray = dynObj->GetLibrary()->CreateArray();
                dynObj->SetObjectArray(newArray);
            }

            return dynObj;
        }

        Js::DynamicObject* ObjectPropertyReset_General(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator)
        {
            TTDAssert(snpObject->OptWellKnownToken == TTD_INVALID_WELLKNOWN_TOKEN, "Should only call this on generic objects that we can fall back to re-allocating if we want.");

            if(!dynObj->GetDynamicType()->GetTypeHandler()->IsResetableForTTD(snpObject->SnapType->TypeHandlerInfo->MaxPropertyIndex))
            {
                return nullptr;
            }

            //always reset the index array as this is unusual and annoying to iterate over a bunch
            Js::ArrayObject* parray = dynObj->GetObjectArray();
            if(parray != nullptr)
            {
                Js::JavascriptArray* newArray = dynObj->GetLibrary()->CreateArray();
                dynObj->SetObjectArray(newArray);
            }

            return dynObj;
        }

        //
        //TODO: I still don't love this (and the reset above) as I feel it hits too much other execution machinery and can fail in odd cases.
        //          For the current time it is ok but we may want to look into adding specialized methods for resetting/restoring.
        //
        void StdPropertyRestore(const SnapObject* snpObject, Js::DynamicObject* obj, InflateMap* inflator)
        {
            //Many protos are set at creation, don't mess with them if they are already correct
            if(snpObject->SnapType->PrototypeVar != nullptr)
            {
                Js::RecyclableObject* protoObj = Js::RecyclableObject::FromVar(inflator->InflateTTDVar(snpObject->SnapType->PrototypeVar));
                if(obj->GetType()->GetPrototype() != protoObj)
                {
                    obj->SetPrototype(protoObj);
                }
            }

            //set all the standard properties
            const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;

#if ENABLE_OBJECT_SOURCE_TRACKING
            CopyDiagnosticOriginInformation(obj->TTDDiagOriginInfo, snpObject->DiagOriginInfo);
#endif

            //
            //We assume that placing properties back in the same order we read them out produces correct results.
            //This is not true for enumeration -- but we handle this by explicit logging
            //There may also be sensitivity in other cases -- e.g. activataion objects with arguments objects that use slot index values directly.
            //    Things look good in this case but future changes may require care and/or adding special case handling.
            //
            for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
            {
                //We have an empty (or uninteresting) slot for so there is nothing to restore
                if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Clear)
                {
                    continue;
                }

                TTDAssert(!Js::JavascriptProxy::Is(obj), "I didn't think proxies could have real properties directly on them.");

                Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;

                if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Uninitialized)
                {
                    TTDAssert(!obj->HasOwnProperty(pid), "Shouldn't have this defined, or we should have cleared it, and nothing more to do.");

                    BOOL success = obj->EnsureProperty(pid);

                    TTDAssert(success, "Failed to set property during restore!!!");
                }
                else
                {
                    TTDVar ttdVal = snpObject->VarArray[i];
                    Js::Var pVal = (ttdVal != nullptr) ? inflator->InflateTTDVar(ttdVal) : nullptr;

                    if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Data)
                    {
                        BOOL success = FALSE;
                        if(!obj->HasOwnProperty(pid))
                        {
                            //easy case just set the property
                            success = obj->SetPropertyWithAttributes(pid, pVal, PropertyDynamicTypeDefaults, nullptr);
                        }
                        else
                        {
                            //get the value to see if it is alreay ok
                            Js::Var currentValue = nullptr;
                            Js::JavascriptOperators::GetOwnProperty(obj, pid, &currentValue, obj->GetScriptContext());

                            if(currentValue == pVal)
                            {
                                //the right value is already there -- easy
                                success = TRUE;
                            }
                            else
                            {
                                //Ok so now we force set the property
                                success = obj->SetPropertyWithAttributes(pid, pVal, PropertyDynamicTypeDefaults, nullptr);
                            }
                        }

                        TTDAssert(success, "Failed to set property during restore!!!");
                    }
                    else
                    {
                        NSSnapType::SnapEntryDataKindTag ttag = handler->PropertyInfoArray[i].DataKind;
                        if(ttag == NSSnapType::SnapEntryDataKindTag::Getter)
                        {
                            obj->SetAccessors(pid, pVal, nullptr);
                        }
                        else if(ttag == NSSnapType::SnapEntryDataKindTag::Setter)
                        {
                            obj->SetAccessors(pid, nullptr, pVal);
                        }
                        else
                        {
                            TTDAssert(false, "Don't know how to restore this accesstag!!");
                        }
                    }
                }

                Js::PropertyAttributes pAttrib = (Js::PropertyAttributes)handler->PropertyInfoArray[i].AttributeInfo;
                if(obj->IsWritable(pid) && (pAttrib & PropertyWritable) == PropertyNone)
                {
                    obj->SetWritable(pid, FALSE);
                }

                if(obj->IsEnumerable(pid) && (pAttrib & PropertyEnumerable) == PropertyNone)
                {
                    obj->SetEnumerable(pid, FALSE);
                }

                if(obj->IsConfigurable(pid) && (pAttrib & PropertyConfigurable) == PropertyNone)
                {
                    obj->SetConfigurable(pid, FALSE);
                }
            }

            if(snpObject->OptIndexedObjectArray != TTD_INVALID_PTR_ID)
            {
                Js::Var objArray = inflator->LookupObject(snpObject->OptIndexedObjectArray);
                obj->SetObjectArray(Js::JavascriptArray::FromAnyArray(objArray));
            }

            //finally set the extensible flag
            if(handler->IsExtensibleFlag != Js::DynamicTypeHandler::IsExtensibleFlag)
            {
                //this automatically updates the type if needed
                obj->GetDynamicType()->GetTypeHandler()->PreventExtensions(obj);
            }
            else
            {
                if(!obj->GetIsExtensible())
                {
                    TTDAssert(!(obj->GetDynamicType()->GetIsShared() || obj->GetDynamicType()->GetTypeHandler()->GetIsShared()), "We are just changing the flag so if it is shared this might unexpectedly change another type!");

                    obj->GetDynamicType()->GetTypeHandler()->SetExtensible_TTD();
                }
            }

            if(snpObject->SnapType->HasNoEnumerableProperties != obj->GetDynamicType()->GetHasNoEnumerableProperties())
            {
                TTDAssert(!obj->GetDynamicType()->GetIsShared(), "This is shared so we are mucking something up.");

                obj->GetDynamicType()->SetHasNoEnumerableProperties(snpObject->SnapType->HasNoEnumerableProperties);
            }
        }

        void EmitObject(const SnapObject* snpObject, FileWriter* writer, NSTokens::Separator separator, const SnapObjectVTable* vtable, ThreadContext* threadContext)
        {
            writer->WriteRecordStart(separator);
            writer->AdjustIndent(1);

            writer->WriteAddr(NSTokens::Key::objectId, snpObject->ObjectPtrId);
            writer->WriteTag<SnapObjectType>(NSTokens::Key::objectType, snpObject->SnapObjectTag, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::isWellKnownToken, snpObject->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN, NSTokens::Separator::CommaSeparator);
            if(snpObject->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                writer->WriteWellKnownToken(NSTokens::Key::wellKnownToken, snpObject->OptWellKnownToken, NSTokens::Separator::CommaSeparator);
            }

            writer->WriteAddr(NSTokens::Key::typeId, snpObject->SnapType->TypePtrId, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::isCrossSite, !!snpObject->IsCrossSite, NSTokens::Separator::CommaSeparator);

#if ENABLE_OBJECT_SOURCE_TRACKING
            writer->WriteKey(NSTokens::Key::originInfo, NSTokens::Separator::CommaSeparator);
            EmitDiagnosticOriginInformation(snpObject->DiagOriginInfo, writer, NSTokens::Separator::NoSeparator);
#endif

            writer->WriteBool(NSTokens::Key::isDepOn, snpObject->OptDependsOnInfo != nullptr, NSTokens::Separator::CommaSeparator);
            if(snpObject->OptDependsOnInfo != nullptr)
            {
                writer->WriteLengthValue(snpObject->OptDependsOnInfo->DepOnCount, NSTokens::Separator::CommaSeparator);
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = 0; i < snpObject->OptDependsOnInfo->DepOnCount; ++i)
                {
                    writer->WriteNakedAddr(snpObject->OptDependsOnInfo->DepOnPtrArray[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
                }
                writer->WriteSequenceEnd();
            }

            if(Js::DynamicType::Is(snpObject->SnapType->JsTypeId))
            {
                const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;

                writer->WriteAddr(NSTokens::Key::objectId, snpObject->OptIndexedObjectArray, NSTokens::Separator::CommaSeparator);

                if(handler->MaxPropertyIndex == 0)
                {
                    writer->WriteLengthValue(snpObject->VarArrayCount, NSTokens::Separator::CommaSeparator);
                }
                else
                {
                    writer->WriteLengthValue(handler->MaxPropertyIndex, NSTokens::Separator::CommaAndBigSpaceSeparator);
                    writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                    writer->AdjustIndent(1);
                    for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
                    {
                        NSTokens::Separator varSep = i != 0 ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator;

                        if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Clear)
                        {
                            writer->WriteNakedNull(varSep);
                        }
                        else
                        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                            writer->WriteRecordStart(varSep);
                            writer->WriteUInt32(NSTokens::Key::pid, (uint32)handler->PropertyInfoArray[i].PropertyRecordId, NSTokens::Separator::NoSeparator);
                            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);

                            varSep = NSTokens::Separator::NoSeparator;
#endif

                            NSSnapValues::EmitTTDVar(snpObject->VarArray[i], writer, varSep);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                            writer->WriteRecordEnd();
#endif
                        }
                    }
                    writer->AdjustIndent(-1);
                    writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
                }
            }

            fPtr_EmitAddtlInfo addtlInfoEmit = vtable[(uint32)snpObject->SnapObjectTag].EmitAddtlInfoFunc;
            if(addtlInfoEmit != nullptr)
            {
                addtlInfoEmit(snpObject, writer);
            }

            writer->AdjustIndent(-1);
            writer->WriteRecordEnd(NSTokens::Separator::BigSpaceSeparator);
        }

        void ParseObject(SnapObject* snpObject, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const SnapObjectVTable* vtable, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& ptrIdToTypeMap)
        {
            reader->ReadRecordStart(readSeperator);

            snpObject->ObjectPtrId = reader->ReadAddr(NSTokens::Key::objectId);
            snpObject->SnapObjectTag = reader->ReadTag<SnapObjectType>(NSTokens::Key::objectType, true);

            bool hasWellKnownToken = reader->ReadBool(NSTokens::Key::isWellKnownToken, true);
            snpObject->OptWellKnownToken = TTD_INVALID_WELLKNOWN_TOKEN;
            if(hasWellKnownToken)
            {
                snpObject->OptWellKnownToken = reader->ReadWellKnownToken(NSTokens::Key::wellKnownToken, alloc, true);
            }

            snpObject->SnapType = ptrIdToTypeMap.LookupKnownItem(reader->ReadAddr(NSTokens::Key::typeId, true));

            snpObject->IsCrossSite = reader->ReadBool(NSTokens::Key::isCrossSite, true);

#if ENABLE_OBJECT_SOURCE_TRACKING
            reader->ReadKey(NSTokens::Key::originInfo, true);
            ParseDiagnosticOriginInformation(snpObject->DiagOriginInfo, false, reader);
#endif

            snpObject->OptDependsOnInfo = nullptr;
            bool isDepOn = reader->ReadBool(NSTokens::Key::isDepOn, true);
            if(isDepOn)
            {
                snpObject->OptDependsOnInfo = alloc.SlabAllocateStruct<DependsOnInfo>();

                snpObject->OptDependsOnInfo->DepOnCount = reader->ReadLengthValue(true);
                snpObject->OptDependsOnInfo->DepOnPtrArray = alloc.SlabAllocateArray<TTD_PTR_ID>(snpObject->OptDependsOnInfo->DepOnCount);

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < snpObject->OptDependsOnInfo->DepOnCount; ++i)
                {
                    snpObject->OptDependsOnInfo->DepOnPtrArray[i] = reader->ReadNakedAddr(i != 0);
                }
                reader->ReadSequenceEnd();
            }

            if(Js::DynamicType::Is(snpObject->SnapType->JsTypeId))
            {
                const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;

                snpObject->OptIndexedObjectArray = reader->ReadAddr(NSTokens::Key::objectId, true);

                snpObject->VarArrayCount = reader->ReadLengthValue(true);
                if(handler->MaxPropertyIndex == 0)
                {
                    snpObject->VarArray = nullptr;
                }
                else
                {
                    snpObject->VarArray = alloc.SlabAllocateArray<TTDVar>(snpObject->VarArrayCount);

                    reader->ReadSequenceStart_WDefaultKey(true);
                    for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
                    {
                        bool readVarSeparator = i != 0;

                        if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Clear)
                        {
                            reader->ReadNakedNull(readVarSeparator);
                        }
                        else
                        {
#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                            reader->ReadRecordStart(readVarSeparator);
                            reader->ReadUInt32(NSTokens::Key::pid);
                            reader->ReadKey(NSTokens::Key::entry, true);

                            readVarSeparator = false;
#endif

                            snpObject->VarArray[i] = NSSnapValues::ParseTTDVar(readVarSeparator, reader);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                            reader->ReadRecordEnd();
#endif
                        }
                    }
                    reader->ReadSequenceEnd();
                }
            }

            fPtr_ParseAddtlInfo addtlInfoParse = vtable[(uint32)snpObject->SnapObjectTag].ParseAddtlInfoFunc;
            if(addtlInfoParse != nullptr)
            {
                addtlInfoParse(snpObject, reader, alloc);
            }

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(sobj1->SnapObjectTag == sobj2->SnapObjectTag);
            compareMap.DiagnosticAssert(TTD_DIAGNOSTIC_COMPARE_WELLKNOWN_TOKENS(sobj1->OptWellKnownToken, sobj2->OptWellKnownToken));

            NSSnapType::AssertSnapEquiv(sobj1->SnapType, sobj2->SnapType, compareMap);

            //Depends on info is a function of the rest of the properties so we don't need to explicitly check it.
            //But for sanity assert same counts.
            compareMap.DiagnosticAssert((sobj1->OptDependsOnInfo == nullptr && sobj2->OptDependsOnInfo == nullptr) || (sobj1->OptDependsOnInfo->DepOnCount == sobj2->OptDependsOnInfo->DepOnCount));

            //we allow the replay in debug mode to be cross site even if orig was not (that is ok) but if record was x-site then replay must be as well
            if(compareMap.StrictCrossSite)
            {
                compareMap.DiagnosticAssert(sobj1->IsCrossSite == sobj2->IsCrossSite);
            }
            else
            {
                compareMap.DiagnosticAssert(!sobj1->IsCrossSite || sobj2->IsCrossSite);
            }

#if ENABLE_OBJECT_SOURCE_TRACKING
            compareMap.DiagnosticAssert(sobj1->DiagOriginInfo.SourceLine == sobj2->DiagOriginInfo.SourceLine);
            compareMap.DiagnosticAssert(sobj1->DiagOriginInfo.EventTime == sobj2->DiagOriginInfo.EventTime);
            compareMap.DiagnosticAssert(sobj1->DiagOriginInfo.TimeHash == sobj2->DiagOriginInfo.TimeHash);
#endif

            compareMap.DiagnosticAssert(Js::DynamicType::Is(sobj1->SnapType->JsTypeId) == Js::DynamicType::Is(sobj2->SnapType->JsTypeId));
            if(Js::DynamicType::Is(sobj1->SnapType->JsTypeId))
            {
                compareMap.CheckConsistentAndAddPtrIdMapping_Special(sobj1->OptIndexedObjectArray, sobj2->OptIndexedObjectArray, _u("indexedObjectArray"));

                const NSSnapType::SnapHandler* handler1 = sobj1->SnapType->TypeHandlerInfo;
                JsUtil::BaseDictionary<int64, int32, HeapAllocator> sobj1PidMap(&HeapAllocator::Instance);
                for(uint32 i = 0; i < handler1->MaxPropertyIndex; ++i)
                {
                    const NSSnapType::SnapHandlerPropertyEntry spe = handler1->PropertyInfoArray[i];
                    if(spe.DataKind != NSSnapType::SnapEntryDataKindTag::Clear)
                    {
                        int64 locationTag = ComputeLocationTagForAssertCompare(spe);
                        sobj1PidMap.AddNew(locationTag, (int32)i);
                    }
                }

                const NSSnapType::SnapHandler* handler2 = sobj2->SnapType->TypeHandlerInfo;
                for(uint32 i = 0; i < handler2->MaxPropertyIndex; ++i)
                {
                    const NSSnapType::SnapHandlerPropertyEntry spe = handler2->PropertyInfoArray[i];
                    if(spe.DataKind != NSSnapType::SnapEntryDataKindTag::Clear && spe.DataKind != NSSnapType::SnapEntryDataKindTag::Uninitialized)
                    {
                        int64 locationTag = ComputeLocationTagForAssertCompare(spe);

                        int32 idx1 = sobj1PidMap.LookupWithKey(locationTag, -1);
                        compareMap.DiagnosticAssert(idx1 != -1);

                        TTDVar var1 = sobj1->VarArray[idx1];
                        TTDVar var2 = sobj2->VarArray[i];

                        if(spe.DataKind == NSSnapType::SnapEntryDataKindTag::Data)
                        {
                            NSSnapValues::AssertSnapEquivTTDVar_Property(var1, var2, compareMap, spe.PropertyRecordId);
                        }
                        else if(spe.DataKind == NSSnapType::SnapEntryDataKindTag::Getter)
                        {
                            NSSnapValues::AssertSnapEquivTTDVar_PropertyGetter(var1, var2, compareMap, spe.PropertyRecordId);
                        }
                        else
                        {
                            TTDAssert(spe.DataKind == NSSnapType::SnapEntryDataKindTag::Setter, "What other tags are there???");

                            NSSnapValues::AssertSnapEquivTTDVar_PropertySetter(var1, var2, compareMap, spe.PropertyRecordId);
                        }
                    }
                }
            }

            fPtr_AssertSnapEquivAddtlInfo equivCheck = compareMap.SnapObjCmpVTable[(int32)sobj1->SnapObjectTag];
            if(equivCheck != nullptr)
            {
                equivCheck(sobj1, sobj2, compareMap);
            }
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapDynamicObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
            if(rcObj != nullptr)
            {
                return rcObj;
            }
            else
            {
                Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
                return ctx->GetLibrary()->CreateObject();
            }
        }

        Js::RecyclableObject* DoObjectInflation_SnapExternalObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
            if(rcObj != nullptr)
            {
                return rcObj;
            }
            else
            {
                Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
                Js::Var res = nullptr;
                ctx->GetThreadContext()->TTDContext->TTDExternalObjectFunctions.pfCreateExternalObject(ctx, &res);

                return Js::RecyclableObject::FromVar(res);
            }
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapScriptFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            SnapScriptFunctionInfo* snapFuncInfo = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject);

            Js::FunctionBody* fbody = inflator->LookupFunctionBody(snapFuncInfo->BodyRefId);

            Js::ScriptFunction* func = nullptr;
            if(!fbody->GetInlineCachesOnFunctionObject())
            {
                func = ctx->GetLibrary()->CreateScriptFunction(fbody);
            }
            else
            {
                Js::ScriptFunctionWithInlineCache* ifunc = ctx->GetLibrary()->CreateScriptFunctionWithInlineCache(fbody);
                ifunc->CreateInlineCache();

                func = ifunc;
            }

            func->SetHasSuperReference(snapFuncInfo->HasSuperReference);

            return func;
        }

        void DoAddtlValueInstantiation_SnapScriptFunctionInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            Js::ScriptFunction* fobj = Js::ScriptFunction::FromVar(obj);
            SnapScriptFunctionInfo* snapFuncInfo = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject);

            if(snapFuncInfo->CachedScopeObjId != TTD_INVALID_PTR_ID)
            {
                fobj->SetCachedScope(Js::ActivationObjectEx::FromVar(inflator->LookupObject(snapFuncInfo->CachedScopeObjId)));
            }

            if(snapFuncInfo->HomeObjId != TTD_INVALID_PTR_ID)
            {
                fobj->SetHomeObj(inflator->LookupObject(snapFuncInfo->HomeObjId));
            }

            if(snapFuncInfo->ScopeId != TTD_INVALID_PTR_ID)
            {
                Js::FrameDisplay* environment = inflator->LookupEnvironment(snapFuncInfo->ScopeId);
                fobj->SetEnvironment(environment);
            }

            if(snapFuncInfo->ComputedNameInfo != nullptr)
            {
                Js::Var cNameVar = inflator->InflateTTDVar(snapFuncInfo->ComputedNameInfo);
                fobj->SetComputedNameVar(cNameVar);
            }
        }

        void EmitAddtlInfo_SnapScriptFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapScriptFunctionInfo* snapFuncInfo = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject);

            writer->WriteAddr(NSTokens::Key::functionBodyId, snapFuncInfo->BodyRefId, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteString(NSTokens::Key::name, snapFuncInfo->DebugFunctionName, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::cachedScopeObjId, snapFuncInfo->CachedScopeObjId, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::scopeId, snapFuncInfo->ScopeId, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::ptrIdVal, snapFuncInfo->HomeObjId, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::nameInfo, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(snapFuncInfo->ComputedNameInfo, writer, NSTokens::Separator::NoSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, snapFuncInfo->HasSuperReference, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapScriptFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapScriptFunctionInfo* snapFuncInfo = alloc.SlabAllocateStruct<SnapScriptFunctionInfo>();

            snapFuncInfo->BodyRefId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);

            reader->ReadString(NSTokens::Key::name, alloc, snapFuncInfo->DebugFunctionName, true);
            snapFuncInfo->CachedScopeObjId = reader->ReadAddr(NSTokens::Key::cachedScopeObjId, true);
            snapFuncInfo->ScopeId = reader->ReadAddr(NSTokens::Key::scopeId, true);
            snapFuncInfo->HomeObjId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);

            reader->ReadKey(NSTokens::Key::nameInfo, true);
            snapFuncInfo->ComputedNameInfo = NSSnapValues::ParseTTDVar(false, reader);

            snapFuncInfo->HasSuperReference = reader->ReadBool(NSTokens::Key::boolVal, true);

            SnapObjectSetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject, snapFuncInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapScriptFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapScriptFunctionInfo* snapFuncInfo1 = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(sobj1);
            const SnapScriptFunctionInfo* snapFuncInfo2 = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(sobj2);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(snapFuncInfo1->DebugFunctionName, snapFuncInfo2->DebugFunctionName));

            compareMap.CheckConsistentAndAddPtrIdMapping_FunctionBody(snapFuncInfo1->BodyRefId, snapFuncInfo2->BodyRefId);
            compareMap.CheckConsistentAndAddPtrIdMapping_Special(snapFuncInfo1->ScopeId, snapFuncInfo2->ScopeId, _u("scopes"));

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(snapFuncInfo1->CachedScopeObjId, snapFuncInfo2->CachedScopeObjId, _u("cachedScopeObj"));
            compareMap.CheckConsistentAndAddPtrIdMapping_Special(snapFuncInfo1->HomeObjId, snapFuncInfo2->HomeObjId, _u("homeObject"));

            NSSnapValues::AssertSnapEquivTTDVar_Special(snapFuncInfo1->ComputedNameInfo, snapFuncInfo2->ComputedNameInfo, compareMap, _u("computedName"));

            compareMap.DiagnosticAssert(snapFuncInfo1->HasSuperReference == snapFuncInfo2->HasSuperReference);
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapExternalFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            TTDVar snapVar = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapExternalFunctionObject>(snpObject);

            Js::Var fname = (snapVar != nullptr) ? inflator->InflateTTDVar(snapVar) : nullptr;
            return ctx->GetLibrary()->CreateExternalFunction_TTD(fname);
        }

        void EmitAddtlInfo_SnapExternalFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            TTDVar snapName = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapExternalFunctionObject>(snpObject);

            writer->WriteKey(NSTokens::Key::name, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(snapName, writer, NSTokens::Separator::NoSeparator);
        }

        void ParseAddtlInfo_SnapExternalFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadKey(NSTokens::Key::name, true);
            TTDVar snapName = NSSnapValues::ParseTTDVar(false, reader);

            SnapObjectSetAddtlInfoAs<TTDVar, SnapObjectType::SnapExternalFunctionObject>(snpObject, snapName);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapExternalFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            TTDVar snapName1 = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapExternalFunctionObject>(sobj1);
            TTDVar snapName2 = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapExternalFunctionObject>(sobj2);

            NSSnapValues::AssertSnapEquivTTDVar_Special(snapName1, snapName2, compareMap, _u("externalFunctionName"));
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapRevokerFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            TTD_PTR_ID* proxyId = SnapObjectGetAddtlInfoAs<TTD_PTR_ID*, SnapObjectType::SnapRuntimeRevokerFunctionObject>(snpObject);
            Js::RecyclableObject* proxyObj = nullptr;
            if(*proxyId == TTD_INVALID_PTR_ID)
            {
                proxyObj = ctx->GetLibrary()->GetNull();
            }
            else
            {
                proxyObj = inflator->LookupObject(*proxyId);
            }

            return ctx->GetLibrary()->CreateRevokeFunction_TTD(proxyObj);
        }

        void EmitAddtlInfo_SnapRevokerFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            TTD_PTR_ID* revokeTrgt = SnapObjectGetAddtlInfoAs<TTD_PTR_ID*, SnapObjectType::SnapRuntimeRevokerFunctionObject>(snpObject);

            writer->WriteAddr(NSTokens::Key::objectId, *revokeTrgt, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapRevokerFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            TTD_PTR_ID* revokerId = alloc.SlabAllocateStruct<TTD_PTR_ID>();
            *revokerId = reader->ReadAddr(NSTokens::Key::objectId, true);

            SnapObjectSetAddtlInfoAs<TTD_PTR_ID*, SnapObjectType::SnapRuntimeRevokerFunctionObject>(snpObject, revokerId);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapRevokerFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            TTD_PTR_ID* revokeTrgt1 = SnapObjectGetAddtlInfoAs<TTD_PTR_ID*, SnapObjectType::SnapRuntimeRevokerFunctionObject>(sobj1);
            TTD_PTR_ID* revokeTrgt2 = SnapObjectGetAddtlInfoAs<TTD_PTR_ID*, SnapObjectType::SnapRuntimeRevokerFunctionObject>(sobj2);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(*revokeTrgt1, *revokeTrgt2, _u("revokeTarget"));
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapBoundFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Bound functions are not too common and have special internal state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            SnapBoundFunctionInfo* snapBoundInfo = SnapObjectGetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(snpObject);

            Js::RecyclableObject* bFunction = inflator->LookupObject(snapBoundInfo->TargetFunction);
            Js::RecyclableObject* bThis = (snapBoundInfo->BoundThis != TTD_INVALID_PTR_ID) ? inflator->LookupObject(snapBoundInfo->BoundThis) : nullptr;

            Field(Js::Var)* bArgs = nullptr;
            if(snapBoundInfo->ArgCount != 0)
            {
                bArgs = RecyclerNewArray(ctx->GetRecycler(), Field(Js::Var), snapBoundInfo->ArgCount);

                for(uint i = 0; i < snapBoundInfo->ArgCount; i++)
                {
                    bArgs[i] = inflator->InflateTTDVar(snapBoundInfo->ArgArray[i]);
                }
            }

            return ctx->GetLibrary()->CreateBoundFunction_TTD(bFunction, bThis, snapBoundInfo->ArgCount, (Js::Var*)bArgs);
        }

        void EmitAddtlInfo_SnapBoundFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapBoundFunctionInfo* snapBoundInfo = SnapObjectGetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(snpObject);

            writer->WriteAddr(NSTokens::Key::boundFunction, snapBoundInfo->TargetFunction, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteAddr(NSTokens::Key::boundThis, snapBoundInfo->BoundThis, NSTokens::Separator::CommaSeparator);
            writer->WriteLengthValue(snapBoundInfo->ArgCount, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::boundArgs, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart();
            for(uint32 i = 0; i < snapBoundInfo->ArgCount; ++i)
            {
                NSSnapValues::EmitTTDVar(snapBoundInfo->ArgArray[i], writer, i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        void ParseAddtlInfo_SnapBoundFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapBoundFunctionInfo* snapBoundInfo = alloc.SlabAllocateStruct<SnapBoundFunctionInfo>();

            snapBoundInfo->TargetFunction = reader->ReadAddr(NSTokens::Key::boundFunction, true);
            snapBoundInfo->BoundThis = reader->ReadAddr(NSTokens::Key::boundThis, true);
            snapBoundInfo->ArgCount = reader->ReadLengthValue(true);

            if(snapBoundInfo->ArgCount == 0)
            {
                snapBoundInfo->ArgArray = nullptr;
            }
            else
            {
                snapBoundInfo->ArgArray = alloc.SlabAllocateArray<TTDVar>(snapBoundInfo->ArgCount);
            }

            reader->ReadKey(NSTokens::Key::boundArgs, true);
            reader->ReadSequenceStart();
            for(uint32 i = 0; i < snapBoundInfo->ArgCount; ++i)
            {
                snapBoundInfo->ArgArray[i] = NSSnapValues::ParseTTDVar(i != 0, reader);
            }
            reader->ReadSequenceEnd();

            SnapObjectSetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(snpObject, snapBoundInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapBoundFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapBoundFunctionInfo* snapBoundInfo1 = SnapObjectGetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(sobj1);
            SnapBoundFunctionInfo* snapBoundInfo2 = SnapObjectGetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(sobj2);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(snapBoundInfo1->TargetFunction, snapBoundInfo2->TargetFunction, _u("targetFunction"));
            compareMap.CheckConsistentAndAddPtrIdMapping_Special(snapBoundInfo1->BoundThis, snapBoundInfo2->BoundThis, _u("boundThis"));

            compareMap.DiagnosticAssert(snapBoundInfo1->ArgCount == snapBoundInfo2->ArgCount);
            for(uint32 i = 0; i < snapBoundInfo1->ArgCount; ++i)
            {
                NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(snapBoundInfo1->ArgArray[i], snapBoundInfo2->ArgArray[i], compareMap, _u("boundArgs"), i);
            }
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapActivationInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            return ctx->GetLibrary()->CreateActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapBlockActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            return ctx->GetLibrary()->CreateBlockActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapPseudoActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            return ctx->GetLibrary()->CreatePseudoActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapConsoleScopeActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            return ctx->GetLibrary()->CreateConsoleScopeActivationObject();
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapHeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            SnapHeapArgumentsInfo* argsInfo = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, SnapObjectType::SnapHeapArgumentsObject>(snpObject);

            Js::RecyclableObject* activationObj = nullptr;
            if(argsInfo->IsFrameNullPtr)
            {
                activationObj = nullptr;
            }
            else
            {
                TTDAssert(argsInfo->FrameObject != TTD_INVALID_PTR_ID, "That won't work!");

                activationObj = inflator->LookupObject(argsInfo->FrameObject);
            }

            return  ctx->GetLibrary()->CreateHeapArguments_TTD(argsInfo->NumOfArguments, argsInfo->FormalCount, static_cast<Js::ActivationObject*>(activationObj), argsInfo->DeletedArgFlags);
        }

        Js::RecyclableObject* DoObjectInflation_SnapES5HeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            SnapHeapArgumentsInfo* argsInfo = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, SnapObjectType::SnapES5HeapArgumentsObject>(snpObject);

            Js::RecyclableObject* activationObj = nullptr;
            if(argsInfo->IsFrameNullPtr)
            {
                activationObj = nullptr;
            }
            else
            {
                TTDAssert(argsInfo->FrameObject != TTD_INVALID_PTR_ID, "That won't work!");

                activationObj = inflator->LookupObject(argsInfo->FrameObject);
            }

            return  ctx->GetLibrary()->CreateES5HeapArguments_TTD(argsInfo->NumOfArguments, argsInfo->FormalCount, static_cast<Js::ActivationObject*>(activationObj), argsInfo->DeletedArgFlags);
        }

        //////////////////

        ////
        //Promise Info

        Js::RecyclableObject* DoObjectInflation_SnapPromiseInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            const SnapPromiseInfo* promiseInfo = SnapObjectGetAddtlInfoAs<SnapPromiseInfo*, SnapObjectType::SnapPromiseObject>(snpObject);
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            Js::Var result = (promiseInfo->Result != nullptr) ? inflator->InflateTTDVar(promiseInfo->Result) : nullptr;

            JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator> resolveReactions(&HeapAllocator::Instance);
            for(uint32 i = 0; i < promiseInfo->ResolveReactionCount; ++i)
            {
                Js::JavascriptPromiseReaction* reaction = NSSnapValues::InflatePromiseReactionInfo(promiseInfo->ResolveReactions + i, ctx, inflator);
                resolveReactions.Add(reaction);
            }

            JsUtil::List<Js::JavascriptPromiseReaction*, HeapAllocator> rejectReactions(&HeapAllocator::Instance);
            for(uint32 i = 0; i < promiseInfo->RejectReactionCount; ++i)
            {
                Js::JavascriptPromiseReaction* reaction = NSSnapValues::InflatePromiseReactionInfo(promiseInfo->RejectReactions + i, ctx, inflator);
                rejectReactions.Add(reaction);
            }

            Js::RecyclableObject* res = ctx->GetLibrary()->CreatePromise_TTD(promiseInfo->Status, result, resolveReactions, rejectReactions);

            return res;
        }

        void EmitAddtlInfo_SnapPromiseInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapPromiseInfo* promiseInfo = SnapObjectGetAddtlInfoAs<SnapPromiseInfo*, SnapObjectType::SnapPromiseObject>(snpObject);

            writer->WriteUInt32(NSTokens::Key::u32Val, promiseInfo->Status, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::resultValue, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(promiseInfo->Result, writer, NSTokens::Separator::NoSeparator);

            writer->WriteLengthValue(promiseInfo->ResolveReactionCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < promiseInfo->ResolveReactionCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                NSSnapValues::EmitPromiseReactionInfo(promiseInfo->ResolveReactions + i, writer, sep);
            }
            writer->WriteSequenceEnd();

            writer->WriteLengthValue(promiseInfo->RejectReactionCount, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < promiseInfo->RejectReactionCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                NSSnapValues::EmitPromiseReactionInfo(promiseInfo->RejectReactions + i, writer, sep);
            }
            writer->WriteSequenceEnd();
        }

        void ParseAddtlInfo_SnapPromiseInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapPromiseInfo* promiseInfo = alloc.SlabAllocateStruct<SnapPromiseInfo>();

            promiseInfo->Status = reader->ReadUInt32(NSTokens::Key::u32Val, true);

            reader->ReadKey(NSTokens::Key::resultValue, true);
            promiseInfo->Result = NSSnapValues::ParseTTDVar(false, reader);

            promiseInfo->ResolveReactionCount = reader->ReadLengthValue(true);
            promiseInfo->ResolveReactions = nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            if(promiseInfo->ResolveReactionCount != 0)
            {
                promiseInfo->ResolveReactions = alloc.SlabAllocateArray<NSSnapValues::SnapPromiseReactionInfo>(promiseInfo->ResolveReactionCount);

                for(uint32 i = 0; i < promiseInfo->ResolveReactionCount; ++i)
                {
                    NSSnapValues::ParsePromiseReactionInfo(promiseInfo->ResolveReactions + i, i != 0, reader, alloc);
                }
            }
            reader->ReadSequenceEnd();

            promiseInfo->RejectReactionCount = reader->ReadLengthValue(true);
            promiseInfo->RejectReactions = nullptr;
            reader->ReadSequenceStart_WDefaultKey(true);
            if(promiseInfo->RejectReactionCount != 0)
            {
                promiseInfo->RejectReactions = alloc.SlabAllocateArray<NSSnapValues::SnapPromiseReactionInfo>(promiseInfo->RejectReactionCount);

                for(uint32 i = 0; i < promiseInfo->RejectReactionCount; ++i)
                {
                    NSSnapValues::ParsePromiseReactionInfo(promiseInfo->RejectReactions + i, i != 0, reader, alloc);
                }
            }
            reader->ReadSequenceEnd();

            SnapObjectSetAddtlInfoAs<SnapPromiseInfo*, SnapObjectType::SnapPromiseObject>(snpObject, promiseInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapPromiseInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapPromiseInfo* promiseInfo1 = SnapObjectGetAddtlInfoAs<SnapPromiseInfo*, SnapObjectType::SnapPromiseObject>(sobj1);
            const SnapPromiseInfo* promiseInfo2 = SnapObjectGetAddtlInfoAs<SnapPromiseInfo*, SnapObjectType::SnapPromiseObject>(sobj2);

            NSSnapValues::AssertSnapEquivTTDVar_Special(promiseInfo1->Result, promiseInfo2->Result, compareMap, _u("result"));

            compareMap.DiagnosticAssert(promiseInfo1->ResolveReactionCount == promiseInfo2->ResolveReactionCount);
            for(uint32 i = 0; i < promiseInfo1->ResolveReactionCount; ++i)
            {
                NSSnapValues::AssertSnapEquiv(promiseInfo1->ResolveReactions + i, promiseInfo2->ResolveReactions + i, compareMap);
            }

            compareMap.DiagnosticAssert(promiseInfo1->RejectReactionCount == promiseInfo2->RejectReactionCount);
            for(uint32 i = 0; i < promiseInfo1->RejectReactionCount; ++i)
            {
                NSSnapValues::AssertSnapEquiv(promiseInfo1->RejectReactions + i, promiseInfo2->RejectReactions + i, compareMap);
            }
        }
#endif

        ////
        //PromiseResolveOrRejectFunction Info

        Js::RecyclableObject* DoObjectInflation_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            const SnapPromiseResolveOrRejectFunctionInfo* rrfInfo = SnapObjectGetAddtlInfoAs<SnapPromiseResolveOrRejectFunctionInfo*, SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(snpObject);
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            Js::RecyclableObject* promise = inflator->LookupObject(rrfInfo->PromiseId);

            if(!inflator->IsPromiseInfoDefined<Js::JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper>(rrfInfo->AlreadyResolvedWrapperId))
            {
                Js::JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* wrapper = ctx->GetLibrary()->CreateAlreadyDefinedWrapper_TTD(rrfInfo->AlreadyResolvedValue);
                inflator->AddInflatedPromiseInfo<Js::JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper>(rrfInfo->AlreadyResolvedWrapperId, wrapper);
            }
            Js::JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolved = inflator->LookupInflatedPromiseInfo<Js::JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper>(rrfInfo->AlreadyResolvedWrapperId);

            return ctx->GetLibrary()->CreatePromiseResolveOrRejectFunction_TTD(promise, rrfInfo->IsReject, alreadyResolved);
        }

        void EmitAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapPromiseResolveOrRejectFunctionInfo* rrfInfo = SnapObjectGetAddtlInfoAs<SnapPromiseResolveOrRejectFunctionInfo*, SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(snpObject);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, rrfInfo->PromiseId, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, rrfInfo->IsReject, NSTokens::Separator::CommaSeparator);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, rrfInfo->AlreadyResolvedWrapperId, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, rrfInfo->AlreadyResolvedValue, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapPromiseResolveOrRejectFunctionInfo* rrfInfo = alloc.SlabAllocateStruct<SnapPromiseResolveOrRejectFunctionInfo>();

            rrfInfo->PromiseId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
            rrfInfo->IsReject = reader->ReadBool(NSTokens::Key::boolVal, true);

            rrfInfo->AlreadyResolvedWrapperId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
            rrfInfo->AlreadyResolvedValue = reader->ReadBool(NSTokens::Key::boolVal, true);

            SnapObjectSetAddtlInfoAs<SnapPromiseResolveOrRejectFunctionInfo*, SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(snpObject, rrfInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapPromiseResolveOrRejectFunctionInfo* rrfInfo1 = SnapObjectGetAddtlInfoAs<SnapPromiseResolveOrRejectFunctionInfo*, SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(sobj1);
            SnapPromiseResolveOrRejectFunctionInfo* rrfInfo2 = SnapObjectGetAddtlInfoAs<SnapPromiseResolveOrRejectFunctionInfo*, SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(sobj2);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(rrfInfo1->PromiseId, rrfInfo2->PromiseId, _u("promise"));

            compareMap.DiagnosticAssert(rrfInfo1->IsReject == rrfInfo2->IsReject);
            compareMap.DiagnosticAssert(rrfInfo1->AlreadyResolvedValue == rrfInfo2->AlreadyResolvedValue);

            compareMap.CheckConsistentAndAddPtrIdMapping_NoEnqueue(rrfInfo1->AlreadyResolvedWrapperId, rrfInfo2->AlreadyResolvedWrapperId);
        }
#endif

        ////
        //ReactionTaskFunction Info
        Js::RecyclableObject* DoObjectInflation_SnapPromiseReactionTaskFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            const SnapPromiseReactionTaskFunctionInfo* rInfo = SnapObjectGetAddtlInfoAs<SnapPromiseReactionTaskFunctionInfo*, SnapObjectType::SnapPromiseReactionTaskFunctionObject>(snpObject);
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            Js::JavascriptPromiseReaction* reaction = NSSnapValues::InflatePromiseReactionInfo(&rInfo->Reaction, ctx, inflator);
            Js::Var argument = inflator->InflateTTDVar(rInfo->Argument);

            return ctx->GetLibrary()->CreatePromiseReactionTaskFunction_TTD(reaction, argument);
        }

        void EmitAddtlInfo_SnapPromiseReactionTaskFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapPromiseReactionTaskFunctionInfo* rInfo = SnapObjectGetAddtlInfoAs<SnapPromiseReactionTaskFunctionInfo*, SnapObjectType::SnapPromiseReactionTaskFunctionObject>(snpObject);

            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(rInfo->Argument, writer, NSTokens::Separator::NoSeparator);

            writer->WriteKey(NSTokens::Key::reaction, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitPromiseReactionInfo(&rInfo->Reaction, writer, NSTokens::Separator::NoSeparator);
        }

        void ParseAddtlInfo_SnapPromiseReactionTaskFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapPromiseReactionTaskFunctionInfo* rInfo = alloc.SlabAllocateStruct<SnapPromiseReactionTaskFunctionInfo>();

            reader->ReadKey(NSTokens::Key::entry, true);
            rInfo->Argument = NSSnapValues::ParseTTDVar(false, reader);

            reader->ReadKey(NSTokens::Key::reaction, true);
            NSSnapValues::ParsePromiseReactionInfo(&rInfo->Reaction, false, reader, alloc);

            SnapObjectSetAddtlInfoAs<SnapPromiseReactionTaskFunctionInfo*, SnapObjectType::SnapPromiseReactionTaskFunctionObject>(snpObject, rInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapPromiseReactionTaskFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapPromiseReactionTaskFunctionInfo* rInfo1 = SnapObjectGetAddtlInfoAs<SnapPromiseReactionTaskFunctionInfo*, SnapObjectType::SnapPromiseReactionTaskFunctionObject>(sobj1);
            SnapPromiseReactionTaskFunctionInfo* rInfo2 = SnapObjectGetAddtlInfoAs<SnapPromiseReactionTaskFunctionInfo*, SnapObjectType::SnapPromiseReactionTaskFunctionObject>(sobj2);

            NSSnapValues::AssertSnapEquivTTDVar_Special(rInfo1->Argument, rInfo2->Argument, compareMap, _u("argument"));
            NSSnapValues::AssertSnapEquiv(&(rInfo1->Reaction), &(rInfo2->Reaction), compareMap);
        }
#endif

        ////
        //AllResolveElementFunctionObject Info
        Js::RecyclableObject* DoObjectInflation_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            const SnapPromiseAllResolveElementFunctionInfo* aInfo = SnapObjectGetAddtlInfoAs<SnapPromiseAllResolveElementFunctionInfo*, SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(snpObject);
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            Js::JavascriptPromiseCapability* capabilities = InflatePromiseCapabilityInfo(&aInfo->Capabilities, ctx, inflator);

            if(!inflator->IsPromiseInfoDefined<Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper>(aInfo->RemainingElementsWrapperId))
            {
                Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingWrapper = ctx->GetLibrary()->CreateRemainingElementsWrapper_TTD(ctx, aInfo->RemainingElementsValue);
                inflator->AddInflatedPromiseInfo(aInfo->RemainingElementsWrapperId, remainingWrapper);
            }
            Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* wrapper = inflator->LookupInflatedPromiseInfo<Js::JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper>(aInfo->RemainingElementsWrapperId);

            Js::RecyclableObject* values = inflator->LookupObject(aInfo->Values);

            return ctx->GetLibrary()->CreatePromiseAllResolveElementFunction_TTD(capabilities, aInfo->Index, wrapper, values, aInfo->AlreadyCalled);
        }

        void EmitAddtlInfo_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapPromiseAllResolveElementFunctionInfo* aInfo = SnapObjectGetAddtlInfoAs<SnapPromiseAllResolveElementFunctionInfo*, SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(snpObject);

            writer->WriteKey(NSTokens::Key::entry, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitPromiseCapabilityInfo(&aInfo->Capabilities, writer, NSTokens::Separator::NoSeparator);

            writer->WriteUInt32(NSTokens::Key::u32Val, aInfo->Index, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::ptrIdVal, aInfo->RemainingElementsWrapperId, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::u32Val, aInfo->RemainingElementsValue, NSTokens::Separator::CommaSeparator);

            writer->WriteAddr(NSTokens::Key::ptrIdVal, aInfo->Values, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, aInfo->AlreadyCalled, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapPromiseAllResolveElementFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapPromiseAllResolveElementFunctionInfo* aInfo = alloc.SlabAllocateStruct<SnapPromiseAllResolveElementFunctionInfo>();

            reader->ReadKey(NSTokens::Key::entry, true);
            NSSnapValues::ParsePromiseCapabilityInfo(&aInfo->Capabilities, false, reader, alloc);

            aInfo->Index = reader->ReadUInt32(NSTokens::Key::u32Val, true);
            aInfo->RemainingElementsWrapperId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);
            aInfo->RemainingElementsValue = reader->ReadUInt32(NSTokens::Key::u32Val, true);

            aInfo->Values = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);

            aInfo->AlreadyCalled = reader->ReadBool(NSTokens::Key::boolVal, true);

            SnapObjectSetAddtlInfoAs<SnapPromiseAllResolveElementFunctionInfo*, SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(snpObject, aInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapPromiseAllResolveElementFunctionInfo* aInfo1 = SnapObjectGetAddtlInfoAs<SnapPromiseAllResolveElementFunctionInfo*, SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(sobj1);
            SnapPromiseAllResolveElementFunctionInfo* aInfo2 = SnapObjectGetAddtlInfoAs<SnapPromiseAllResolveElementFunctionInfo*, SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(sobj2);

            NSSnapValues::AssertSnapEquiv(&aInfo1->Capabilities, &aInfo2->Capabilities, compareMap);

            compareMap.DiagnosticAssert(aInfo1->Index == aInfo2->Index);
            compareMap.DiagnosticAssert(aInfo1->RemainingElementsValue == aInfo2->RemainingElementsValue);
            compareMap.DiagnosticAssert(aInfo1->AlreadyCalled == aInfo2->AlreadyCalled);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(aInfo1->Values, aInfo2->Values, _u("values"));

            compareMap.CheckConsistentAndAddPtrIdMapping_NoEnqueue(aInfo1->RemainingElementsWrapperId, aInfo2->RemainingElementsWrapperId);
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapBoxedValue(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Boxed values are not too common and have special internal state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            return ctx->GetLibrary()->CreateDefaultBoxedObject_TTD(snpObject->SnapType->JsTypeId);
        }

        void DoAddtlValueInstantiation_SnapBoxedValue(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            TTDVar snapBoxedVar = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(snpObject);
            Js::Var jsvar = (snapBoxedVar != nullptr) ? inflator->InflateTTDVar(snapBoxedVar) : nullptr;

            ctx->GetLibrary()->SetBoxedObjectValue_TTD(obj, jsvar);
        }

        void EmitAddtlInfo_SnapBoxedValue(const SnapObject* snpObject, FileWriter* writer)
        {
            TTDVar snapBoxedVar = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(snpObject);

            writer->WriteKey(NSTokens::Key::boxedInfo, NSTokens::Separator::CommaAndBigSpaceSeparator);
            NSSnapValues::EmitTTDVar(snapBoxedVar, writer, NSTokens::Separator::NoSeparator);
        }

        void ParseAddtlInfo_SnapBoxedValue(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadKey(NSTokens::Key::boxedInfo, true);
            TTDVar snapVar = NSSnapValues::ParseTTDVar(false, reader);

            SnapObjectSetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(snpObject, snapVar);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapBoxedValue(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            TTDVar snapBoxedVar1 = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(sobj1);
            TTDVar snapBoxedVar2 = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(sobj2);

            NSSnapValues::AssertSnapEquivTTDVar_Special(snapBoxedVar1, snapBoxedVar2, compareMap, _u("boxedVar"));
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapDate(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Dates are not too common and have some mutable state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            double* dateInfo = SnapObjectGetAddtlInfoAs<double*, SnapObjectType::SnapDateObject>(snpObject);

            return ctx->GetLibrary()->CreateDate_TTD(*dateInfo);
        }

        void EmitAddtlInfo_SnapDate(const SnapObject* snpObject, FileWriter* writer)
        {
            double* dateInfo = SnapObjectGetAddtlInfoAs<double*, SnapObjectType::SnapDateObject>(snpObject);
            writer->WriteDouble(NSTokens::Key::doubleVal, *dateInfo, NSTokens::Separator::CommaAndBigSpaceSeparator);
        }

        void ParseAddtlInfo_SnapDate(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            double* dateInfo = alloc.SlabAllocateStruct<double>();
            *dateInfo = reader->ReadDouble(NSTokens::Key::doubleVal, true);

            SnapObjectSetAddtlInfoAs<double*, SnapObjectType::SnapDateObject>(snpObject, dateInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapDate(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const double* dateInfo1 = SnapObjectGetAddtlInfoAs<double*, SnapObjectType::SnapDateObject>(sobj1);
            const double* dateInfo2 = SnapObjectGetAddtlInfoAs<double*, SnapObjectType::SnapDateObject>(sobj2);

            compareMap.DiagnosticAssert(NSSnapValues::CheckSnapEquivTTDDouble(*dateInfo1, *dateInfo2));
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapRegexInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Regexes are not too common and have some mutable state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            SnapRegexInfo* regexInfo = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject);

            Js::Var lastVar = (regexInfo->LastIndexVar != nullptr) ? inflator->InflateTTDVar(regexInfo->LastIndexVar) : nullptr;

            return ctx->GetLibrary()->CreateRegex_TTD(regexInfo->RegexStr.Contents, regexInfo->RegexStr.Length, regexInfo->Flags, regexInfo->LastIndexOrFlag, lastVar);
        }

        void EmitAddtlInfo_SnapRegexInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapRegexInfo* regexInfo = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject);

            writer->WriteString(NSTokens::Key::stringVal, regexInfo->RegexStr, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteTag<UnifiedRegex::RegexFlags>(NSTokens::Key::attributeFlags, regexInfo->Flags, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::u32Val, regexInfo->LastIndexOrFlag, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::ttdVarTag, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(regexInfo->LastIndexVar, writer, NSTokens::Separator::NoSeparator);
        }

        void ParseAddtlInfo_SnapRegexInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapRegexInfo* regexInfo = alloc.SlabAllocateStruct<SnapRegexInfo>();

             reader->ReadString(NSTokens::Key::stringVal, alloc, regexInfo->RegexStr, true);

            regexInfo->Flags = reader->ReadTag<UnifiedRegex::RegexFlags>(NSTokens::Key::attributeFlags, true);
            regexInfo->LastIndexOrFlag = reader->ReadUInt32(NSTokens::Key::u32Val, true);

            reader->ReadKey(NSTokens::Key::ttdVarTag, true);
            regexInfo->LastIndexVar = NSSnapValues::ParseTTDVar(false, reader);

            SnapObjectSetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject, regexInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapRegexInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapRegexInfo* regexInfo1 = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(sobj1);
            const SnapRegexInfo* regexInfo2 = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(sobj2);

            compareMap.DiagnosticAssert(TTStringEQForDiagnostics(regexInfo1->RegexStr, regexInfo2->RegexStr));
            compareMap.DiagnosticAssert(regexInfo1->Flags == regexInfo2->Flags);
            compareMap.DiagnosticAssert(regexInfo1->LastIndexOrFlag == regexInfo2->LastIndexOrFlag);
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapError(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            //
            //TODO: This is not great we just create a generic error
            //
            return ctx->GetLibrary()->CreateError_TTD();
        }

        //////////////////

        int32 SnapArrayInfo_InflateValue(int32 value, InflateMap* inflator) { return value; }
        void SnapArrayInfo_EmitValue(int32 value, FileWriter* writer) { writer->WriteInt32(NSTokens::Key::i32Val, value, NSTokens::Separator::CommaSeparator); }
        void SnapArrayInfo_ParseValue(int32* into, FileReader* reader, SlabAllocator& alloc) { *into = reader->ReadInt32(NSTokens::Key::i32Val, true); }

        double SnapArrayInfo_InflateValue(double value, InflateMap* inflator) { return value; }
        void SnapArrayInfo_EmitValue(double value, FileWriter* writer) { writer->WriteDouble(NSTokens::Key::doubleVal, value, NSTokens::Separator::CommaSeparator); }
        void SnapArrayInfo_ParseValue(double* into, FileReader* reader, SlabAllocator& alloc) { *into = reader->ReadDouble(NSTokens::Key::doubleVal, true); }

        Js::Var SnapArrayInfo_InflateValue(TTDVar value, InflateMap* inflator)
        {
            return inflator->InflateTTDVar(value);
        }

        void SnapArrayInfo_EmitValue(TTDVar value, FileWriter* writer)
        {
            writer->WriteKey(NSTokens::Key::ptrIdVal, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(value, writer, NSTokens::Separator::NoSeparator);
        }

        void SnapArrayInfo_ParseValue(TTDVar* into, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadKey(NSTokens::Key::ptrIdVal, true);
            *into = NSSnapValues::ParseTTDVar(false, reader);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void SnapArrayInfo_EquivValue(int32 val1, int32 val2, TTDCompareMap& compareMap, int32 i)
        {
            compareMap.DiagnosticAssert(val1 == val2);
        }

        void SnapArrayInfo_EquivValue(double val1, double val2, TTDCompareMap& compareMap, int32 i)
        {
            compareMap.DiagnosticAssert(val1 == val2);
        }

        void SnapArrayInfo_EquivValue(TTDVar val1, TTDVar val2, TTDCompareMap& compareMap, int32 i)
        {
            NSSnapValues::AssertSnapEquivTTDVar_Array(val1, val2, compareMap, i);
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapES5ArrayInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            return ctx->GetLibrary()->CreateES5Array_TTD();
        }

        void DoAddtlValueInstantiation_SnapES5ArrayInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            SnapES5ArrayInfo* es5Info = SnapObjectGetAddtlInfoAs<SnapES5ArrayInfo*, SnapObjectType::SnapES5ArrayObject>(snpObject);

            Js::JavascriptArray* arrayObj = static_cast<Js::JavascriptArray*>(obj);
            DoAddtlValueInstantiation_SnapArrayInfoCore<TTDVar, Js::Var>(es5Info->BasicArrayData, arrayObj, inflator);

            for(uint32 i = 0; i < es5Info->GetterSetterCount; ++i)
            {
                const SnapES5ArrayGetterSetterEntry* entry = es5Info->GetterSetterEntries + i;

                Js::Var getter = nullptr;
                if(entry->Getter != nullptr)
                {
                    getter = inflator->InflateTTDVar(entry->Getter);
                }

                Js::Var setter = nullptr;
                if(entry->Setter != nullptr)
                {
                    setter = inflator->InflateTTDVar(entry->Setter);
                }

                if(getter != nullptr || setter != nullptr)
                {
                    arrayObj->SetItemAccessors(entry->Index, getter, setter);
                }

                arrayObj->SetItemAttributes(entry->Index, entry->Attributes);
            }

            //do length writable as needed
            Js::JavascriptLibrary::SetLengthWritableES5Array_TTD(arrayObj, es5Info->IsLengthWritable);
        }

        void EmitAddtlInfo_SnapES5ArrayInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapES5ArrayInfo* es5Info = SnapObjectGetAddtlInfoAs<SnapES5ArrayInfo*, SnapObjectType::SnapES5ArrayObject>(snpObject);

            writer->WriteLengthValue(es5Info->GetterSetterCount, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, es5Info->IsLengthWritable, NSTokens::Separator::CommaSeparator);

            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            for(uint32 i = 0; i < es5Info->GetterSetterCount; ++i)
            {
                NSTokens::Separator sep = (i != 0) ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator;
                const SnapES5ArrayGetterSetterEntry* entry = es5Info->GetterSetterEntries + i;

                writer->WriteRecordStart(sep);

                writer->WriteUInt32(NSTokens::Key::index, entry->Index);
                writer->WriteUInt32(NSTokens::Key::attributeFlags, entry->Attributes, NSTokens::Separator::CommaSeparator);

                writer->WriteKey(NSTokens::Key::getterEntry, NSTokens::Separator::CommaSeparator);
                NSSnapValues::EmitTTDVar(entry->Getter, writer, NSTokens::Separator::NoSeparator);

                writer->WriteKey(NSTokens::Key::setterEntry, NSTokens::Separator::CommaSeparator);
                NSSnapValues::EmitTTDVar(entry->Setter, writer, NSTokens::Separator::NoSeparator);

                writer->WriteRecordEnd();
            }
            writer->WriteSequenceEnd();

            EmitAddtlInfo_SnapArrayInfoCore<TTDVar>(es5Info->BasicArrayData, writer);
        }

        void ParseAddtlInfo_SnapES5ArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapES5ArrayInfo* es5Info = alloc.SlabAllocateStruct<SnapES5ArrayInfo>();

            es5Info->GetterSetterCount = reader->ReadLengthValue(true);
            es5Info->IsLengthWritable = reader->ReadBool(NSTokens::Key::boolVal, true);

            if(es5Info->GetterSetterCount == 0)
            {
                es5Info->GetterSetterEntries = nullptr;
            }
            else
            {
                es5Info->GetterSetterEntries = alloc.SlabAllocateArray<SnapES5ArrayGetterSetterEntry>(es5Info->GetterSetterCount);
            }

            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < es5Info->GetterSetterCount; ++i)
            {
                SnapES5ArrayGetterSetterEntry* entry = es5Info->GetterSetterEntries + i;

                reader->ReadRecordStart(i != 0);

                entry->Index = reader->ReadUInt32(NSTokens::Key::index);
                entry->Attributes = (Js::PropertyAttributes)reader->ReadUInt32(NSTokens::Key::attributeFlags, true);

                reader->ReadKey(NSTokens::Key::getterEntry, true);
                entry->Getter = NSSnapValues::ParseTTDVar(false, reader);

                reader->ReadKey(NSTokens::Key::setterEntry, true);
                entry->Setter = NSSnapValues::ParseTTDVar(false, reader);

                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            es5Info->BasicArrayData = ParseAddtlInfo_SnapArrayInfoCore<TTDVar>(reader, alloc);

            SnapObjectSetAddtlInfoAs<SnapES5ArrayInfo*, SnapObjectType::SnapES5ArrayObject>(snpObject, es5Info);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapES5ArrayInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapES5ArrayInfo* es5Info1 = SnapObjectGetAddtlInfoAs<SnapES5ArrayInfo*, SnapObjectType::SnapES5ArrayObject>(sobj1);
            SnapES5ArrayInfo* es5Info2 = SnapObjectGetAddtlInfoAs<SnapES5ArrayInfo*, SnapObjectType::SnapES5ArrayObject>(sobj2);

            compareMap.DiagnosticAssert(es5Info1->GetterSetterCount == es5Info2->GetterSetterCount);
            compareMap.DiagnosticAssert(es5Info1->IsLengthWritable == es5Info2->IsLengthWritable);

            for(uint32 i = 0; i < es5Info1->GetterSetterCount; ++i)
            {
                const SnapES5ArrayGetterSetterEntry* entry1 = es5Info1->GetterSetterEntries + i;
                const SnapES5ArrayGetterSetterEntry* entry2 = es5Info2->GetterSetterEntries + i;

                compareMap.DiagnosticAssert(entry1->Index == entry2->Index);
                compareMap.DiagnosticAssert(entry1->Attributes == entry2->Attributes);

                NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(entry1->Getter, entry2->Getter, compareMap, _u("es5Getter"), entry1->Index);
                NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(entry1->Setter, entry2->Setter, compareMap, _u("es5Setter"), entry1->Index);
            }

            compareMap.DiagnosticAssert(es5Info1->BasicArrayData->Length == es5Info2->BasicArrayData->Length);

            AssertSnapEquiv_SnapArrayInfoCore<TTDVar>(es5Info1->BasicArrayData->Data, es5Info2->BasicArrayData->Data, compareMap);
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapArrayBufferInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //ArrayBuffers can change on us so seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            SnapArrayBufferInfo* buffInfo = SnapObjectGetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(snpObject);

            Js::ArrayBuffer* abuff = ctx->GetLibrary()->CreateArrayBuffer(buffInfo->Length);
            TTDAssert(abuff->GetByteLength() == buffInfo->Length, "Something is wrong with our sizes.");

            js_memcpy_s(abuff->GetBuffer(), abuff->GetByteLength(), buffInfo->Buff, buffInfo->Length);

            return abuff;
        }

        void EmitAddtlInfo_SnapArrayBufferInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapArrayBufferInfo* buffInfo = SnapObjectGetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(snpObject);

            writer->WriteLengthValue(buffInfo->Length, NSTokens::Separator::CommaAndBigSpaceSeparator);

            if(buffInfo->Length > 0)
            {
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = 0; i < buffInfo->Length; ++i)
                {
                    writer->WriteNakedByte(buffInfo->Buff[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
                }
                writer->WriteSequenceEnd();
            }
        }

        void ParseAddtlInfo_SnapArrayBufferInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapArrayBufferInfo* buffInfo = alloc.SlabAllocateStruct<SnapArrayBufferInfo>();

            buffInfo->Length = reader->ReadLengthValue(true);

            if(buffInfo->Length == 0)
            {
                buffInfo->Buff = nullptr;
            }
            else
            {
                buffInfo->Buff = alloc.SlabAllocateArray<byte>(buffInfo->Length);

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < buffInfo->Length; ++i)
                {
                    buffInfo->Buff[i] = reader->ReadNakedByte(i != 0);
                }
                reader->ReadSequenceEnd();
            }

            SnapObjectSetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(snpObject, buffInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapArrayBufferInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapArrayBufferInfo* buffInfo1 = SnapObjectGetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(sobj1);
            const SnapArrayBufferInfo* buffInfo2 = SnapObjectGetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(sobj2);

            compareMap.DiagnosticAssert(buffInfo1->Length == buffInfo2->Length);

            //
            //Pending buffers cannot be accessed by the program until they are off the pending lists.
            //So we do not force the updates in closer sync than this and they may not be updated in sync so (as long as they are pending) in both.
            //
            if(compareMap.H1PendingAsyncModBufferSet.Contains(sobj1->ObjectPtrId) || compareMap.H2PendingAsyncModBufferSet.Contains(sobj2->ObjectPtrId))
            {
                compareMap.DiagnosticAssert(compareMap.H1PendingAsyncModBufferSet.Contains(sobj1->ObjectPtrId) && compareMap.H2PendingAsyncModBufferSet.Contains(sobj2->ObjectPtrId));
            }
            else
            {
                for(uint32 i = 0; i < buffInfo1->Length; ++i)
                {
                    compareMap.DiagnosticAssert(buffInfo1->Buff[i] == buffInfo2->Buff[i]);
                }
            }
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapTypedArrayInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //I am lazy and always re-create typed arrays.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            SnapTypedArrayInfo* typedArrayInfo = SnapObjectGetAddtlInfoAs<SnapTypedArrayInfo*, SnapObjectType::SnapTypedArrayObject>(snpObject);

            Js::JavascriptLibrary* jslib = ctx->GetLibrary();
            Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(inflator->LookupObject(typedArrayInfo->ArrayBufferAddr));

            Js::Var tab = nullptr;
            switch(snpObject->SnapType->JsTypeId)
            {
            case Js::TypeIds_Int8Array:
                tab = Js::Int8Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Uint8Array:
                tab = Js::Uint8Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Uint8ClampedArray:
                tab = Js::Uint8ClampedArray::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Int16Array:
                tab = Js::Int16Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Uint16Array:
                tab = Js::Uint16Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Int32Array:
                tab = Js::Int32Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Uint32Array:
                tab = Js::Uint32Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Float32Array:
                tab = Js::Float32Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Float64Array:
                tab = Js::Float64Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Int64Array:
                tab = Js::Int64Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_Uint64Array:
                tab = Js::Uint64Array::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_BoolArray:
                tab = Js::BoolArray::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            case Js::TypeIds_CharArray:
                tab = Js::CharArray::Create(arrayBuffer, typedArrayInfo->ByteOffset, typedArrayInfo->Length, jslib);
                break;
            default:
                TTDAssert(false, "Not a typed array!");
                break;
            }

            return Js::RecyclableObject::FromVar(tab);
        }

        void EmitAddtlInfo_SnapTypedArrayInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapTypedArrayInfo* typedArrayInfo = SnapObjectGetAddtlInfoAs<SnapTypedArrayInfo*, SnapObjectType::SnapTypedArrayObject>(snpObject);

            writer->WriteUInt32(NSTokens::Key::offset, typedArrayInfo->ByteOffset, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteLengthValue(typedArrayInfo->Length, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::objectId, typedArrayInfo->ArrayBufferAddr, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapTypedArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapTypedArrayInfo* typedArrayInfo = alloc.SlabAllocateStruct<SnapTypedArrayInfo>();

            typedArrayInfo->ByteOffset = reader->ReadUInt32(NSTokens::Key::offset, true);
            typedArrayInfo->Length = reader->ReadLengthValue(true);
            typedArrayInfo->ArrayBufferAddr = reader->ReadAddr(NSTokens::Key::objectId, true);

            SnapObjectSetAddtlInfoAs<SnapTypedArrayInfo*, SnapObjectType::SnapTypedArrayObject>(snpObject, typedArrayInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapTypedArrayInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapTypedArrayInfo* typedArrayInfo1 = SnapObjectGetAddtlInfoAs<SnapTypedArrayInfo*, SnapObjectType::SnapTypedArrayObject>(sobj1);
            const SnapTypedArrayInfo* typedArrayInfo2 = SnapObjectGetAddtlInfoAs<SnapTypedArrayInfo*, SnapObjectType::SnapTypedArrayObject>(sobj2);

            compareMap.DiagnosticAssert(typedArrayInfo1->ByteOffset == typedArrayInfo2->ByteOffset);
            compareMap.DiagnosticAssert(typedArrayInfo1->Length == typedArrayInfo2->Length);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(typedArrayInfo1->ArrayBufferAddr, typedArrayInfo2->ArrayBufferAddr, _u("arrayBuffer"));
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapSetInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Set)
            {
                return ctx->GetLibrary()->CreateSet_TTD();
            }
            else
            {
                return ctx->GetLibrary()->CreateWeakSet_TTD();
            }
        }

        void DoAddtlValueInstantiation_SnapSetInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            SnapSetInfo* setInfo = SnapObjectGetAddtlInfoAs<SnapSetInfo*, SnapObjectType::SnapSetObject>(snpObject);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Set)
            {
                Js::JavascriptSet* sobj = (Js::JavascriptSet*)obj;
                for(uint32 i = 0; i < setInfo->SetSize; ++i)
                {
                    Js::Var val = inflator->InflateTTDVar(setInfo->SetValueArray[i]);
                    Js::JavascriptLibrary::AddSetElementInflate_TTD(sobj, val);
                }
            }
            else
            {
                Js::JavascriptWeakSet* sobj = (Js::JavascriptWeakSet*)obj;
                for(uint32 i = 0; i < setInfo->SetSize; ++i)
                {
                    Js::Var val = inflator->InflateTTDVar(setInfo->SetValueArray[i]);
                    Js::JavascriptLibrary::AddWeakSetElementInflate_TTD(sobj, val);
                }
            }
        }

        void EmitAddtlInfo_SnapSetInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapSetInfo* setInfo = SnapObjectGetAddtlInfoAs<SnapSetInfo*, SnapObjectType::SnapSetObject>(snpObject);

            writer->WriteLengthValue(setInfo->SetSize, NSTokens::Separator::CommaAndBigSpaceSeparator);

            if(setInfo->SetSize > 0)
            {
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = 0; i < setInfo->SetSize; ++i)
                {
                    NSSnapValues::EmitTTDVar(setInfo->SetValueArray[i], writer, i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::BigSpaceSeparator);
                }
                writer->WriteSequenceEnd();
            }
        }

        void ParseAddtlInfo_SnapSetInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapSetInfo* setInfo = alloc.SlabAllocateStruct<SnapSetInfo>();

            setInfo->SetSize = reader->ReadLengthValue(true);
            if(setInfo->SetSize == 0)
            {
                setInfo->SetValueArray = nullptr;
            }
            else
            {
                setInfo->SetValueArray = alloc.SlabAllocateArray<TTDVar>(setInfo->SetSize);

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < setInfo->SetSize; ++i)
                {
                    setInfo->SetValueArray[i] = NSSnapValues::ParseTTDVar(i != 0, reader);
                }
                reader->ReadSequenceEnd();
            }

            SnapObjectSetAddtlInfoAs<SnapSetInfo*, SnapObjectType::SnapSetObject>(snpObject, setInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapSetInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapSetInfo* setInfo1 = SnapObjectGetAddtlInfoAs<SnapSetInfo*, SnapObjectType::SnapSetObject>(sobj1);
            const SnapSetInfo* setInfo2 = SnapObjectGetAddtlInfoAs<SnapSetInfo*, SnapObjectType::SnapSetObject>(sobj2);

            compareMap.DiagnosticAssert(setInfo1->SetSize == setInfo2->SetSize);

            //Treat weak set contents as ignorable
            if(sobj1->SnapType->JsTypeId == Js::TypeIds_Set)
            {
                for(uint32 i = 0; i < setInfo1->SetSize; ++i)
                {
                    NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(setInfo1->SetValueArray[i], setInfo2->SetValueArray[i], compareMap, _u("setValues"), i);
                }
            }
        }
#endif

        Js::RecyclableObject* DoObjectInflation_SnapMapInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Map)
            {
                return ctx->GetLibrary()->CreateMap_TTD();
            }
            else
            {
                return ctx->GetLibrary()->CreateWeakMap_TTD();
            }
        }

        void DoAddtlValueInstantiation_SnapMapInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            SnapMapInfo* mapInfo = SnapObjectGetAddtlInfoAs<SnapMapInfo*, SnapObjectType::SnapMapObject>(snpObject);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Map)
            {
                Js::JavascriptMap* mobj = (Js::JavascriptMap*)obj;
                for(uint32 i = 0; i < mapInfo->MapSize; i+=2)
                {
                    Js::Var key = inflator->InflateTTDVar(mapInfo->MapKeyValueArray[i]);
                    Js::Var data = inflator->InflateTTDVar(mapInfo->MapKeyValueArray[i + 1]);
                    Js::JavascriptLibrary::AddMapElementInflate_TTD(mobj, key, data);
                }
            }
            else
            {
                Js::JavascriptWeakMap* mobj = (Js::JavascriptWeakMap*)obj;
                for(uint32 i = 0; i < mapInfo->MapSize; i += 2)
                {
                    Js::Var key = inflator->InflateTTDVar(mapInfo->MapKeyValueArray[i]);
                    Js::Var data = inflator->InflateTTDVar(mapInfo->MapKeyValueArray[i + 1]);
                    Js::JavascriptLibrary::AddWeakMapElementInflate_TTD(mobj, key, data);
                }
            }
        }

        void EmitAddtlInfo_SnapMapInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapMapInfo* mapInfo = SnapObjectGetAddtlInfoAs<SnapMapInfo*, SnapObjectType::SnapMapObject>(snpObject);

            writer->WriteLengthValue(mapInfo->MapSize, NSTokens::Separator::CommaAndBigSpaceSeparator);

            if(mapInfo->MapSize > 0)
            {
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = 0; i < mapInfo->MapSize; i+=2)
                {
                    writer->WriteSequenceStart(i != 0 ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);
                    NSSnapValues::EmitTTDVar(mapInfo->MapKeyValueArray[i], writer, NSTokens::Separator::NoSeparator);
                    NSSnapValues::EmitTTDVar(mapInfo->MapKeyValueArray[i + 1], writer, NSTokens::Separator::CommaSeparator);
                    writer->WriteSequenceEnd();
                }
                writer->WriteSequenceEnd();
            }
        }

        void ParseAddtlInfo_SnapMapInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapMapInfo* mapInfo = alloc.SlabAllocateStruct<SnapMapInfo>();

            mapInfo->MapSize = reader->ReadLengthValue(true);
            if(mapInfo->MapSize == 0)
            {
                mapInfo->MapKeyValueArray = nullptr;
            }
            else
            {
                mapInfo->MapKeyValueArray = alloc.SlabAllocateArray<TTDVar>(mapInfo->MapSize);

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < mapInfo->MapSize; i+=2)
                {
                    reader->ReadSequenceStart(i != 0);
                    mapInfo->MapKeyValueArray[i] = NSSnapValues::ParseTTDVar(false, reader);
                    mapInfo->MapKeyValueArray[i + 1] = NSSnapValues::ParseTTDVar(true, reader);
                    reader->ReadSequenceEnd();
                }
                reader->ReadSequenceEnd();
            }

            SnapObjectSetAddtlInfoAs<SnapMapInfo*, SnapObjectType::SnapMapObject>(snpObject, mapInfo);
        }


#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapMapInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapMapInfo* mapInfo1 = SnapObjectGetAddtlInfoAs<SnapMapInfo*, SnapObjectType::SnapMapObject>(sobj1);
            const SnapMapInfo* mapInfo2 = SnapObjectGetAddtlInfoAs<SnapMapInfo*, SnapObjectType::SnapMapObject>(sobj2);

            compareMap.DiagnosticAssert(mapInfo1->MapSize == mapInfo2->MapSize);

            //Treat weak map contents as ignorable
            if(sobj1->SnapType->JsTypeId == Js::TypeIds_Map)
            {
                for(uint32 i = 0; i < mapInfo1->MapSize; i += 2)
                {
                    NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(mapInfo1->MapKeyValueArray[i], mapInfo2->MapKeyValueArray[i], compareMap, _u("mapKeys"), i);
                    NSSnapValues::AssertSnapEquivTTDVar_SpecialArray(mapInfo1->MapKeyValueArray[i + 1], mapInfo2->MapKeyValueArray[i + 1], compareMap, _u("mapValues"), i);
                }
            }
        }
#endif

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapProxyInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            SnapProxyInfo* proxyInfo = SnapObjectGetAddtlInfoAs<SnapProxyInfo*, SnapObjectType::SnapProxyObject>(snpObject);

            Js::RecyclableObject* handlerObj = (proxyInfo->HandlerId != TTD_INVALID_PTR_ID) ? inflator->LookupObject(proxyInfo->HandlerId) : nullptr;
            Js::RecyclableObject* targetObj = (proxyInfo->TargetId != TTD_INVALID_PTR_ID) ? inflator->LookupObject(proxyInfo->TargetId) : nullptr;

            return  ctx->GetLibrary()->CreateProxy_TTD(handlerObj, targetObj);
        }

        void EmitAddtlInfo_SnapProxyInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapProxyInfo* proxyInfo = SnapObjectGetAddtlInfoAs<SnapProxyInfo*, SnapObjectType::SnapProxyObject>(snpObject);

            writer->WriteAddr(NSTokens::Key::handlerId, proxyInfo->HandlerId, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::objectId, proxyInfo->TargetId, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapProxyInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapProxyInfo* proxyInfo = alloc.SlabAllocateStruct<SnapProxyInfo>();

            proxyInfo->HandlerId = reader->ReadAddr(NSTokens::Key::handlerId, true);
            proxyInfo->TargetId = reader->ReadAddr(NSTokens::Key::objectId, true);

            SnapObjectSetAddtlInfoAs<SnapProxyInfo*, SnapObjectType::SnapProxyObject>(snpObject, proxyInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapProxyInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapProxyInfo* proxyInfo1 = SnapObjectGetAddtlInfoAs<SnapProxyInfo*, SnapObjectType::SnapProxyObject>(sobj1);
            const SnapProxyInfo* proxyInfo2 = SnapObjectGetAddtlInfoAs<SnapProxyInfo*, SnapObjectType::SnapProxyObject>(sobj2);

            compareMap.CheckConsistentAndAddPtrIdMapping_Special(proxyInfo1->HandlerId, proxyInfo2->HandlerId, _u("handlerId"));
            compareMap.CheckConsistentAndAddPtrIdMapping_Special(proxyInfo1->TargetId, proxyInfo2->TargetId, _u("targetId"));
        }
#endif
    }
}

#endif
