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
        void ExtractCompoundObject(NSSnapObjects::SnapObject* sobj, Js::RecyclableObject* obj, bool isWellKnown, bool isLogged, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc)
        {
            AssertMsg(!obj->CanHaveInterceptors(), "We are not prepared for custom external objects yet");

            sobj->ObjectPtrId = TTD_CONVERT_VAR_TO_PTR_ID(obj);
            sobj->SnapObjectTag = obj->GetSnapTag_TTD();
            sobj->OptWellKnownToken = isWellKnown ? obj->GetScriptContext()->ResolveKnownTokenForGeneralObject_TTD(obj) : TTD_INVALID_WELLKNOWN_TOKEN;

            Js::Type* objType = obj->GetType();
            sobj->SnapType = idToTypeMap.LookupKnownItem(TTD_CONVERT_TYPEINFO_TO_PTR_ID(objType));

            sobj->ObjectLogTag = isLogged ? obj->GetScriptContext()->GetThreadContext()->TTDInfo->LookupTagForObject(obj) : TTD_INVALID_LOG_TAG;

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
            snpObject->VarArrayCount = 0;
            snpObject->VarArray = nullptr;

            snpObject->OptIndexedObjectArray = TTD_INVALID_PTR_ID;

            //
            //TODO: I am not sure if we want to track identities of certain static type objects (maybe enumerators?)
            //      If so we need to add tags to them and extract here and add code to restore the special case tags in the SetObjectTrackingTagSnapAndInflate_TTD method.
            //
            snpObject->ObjectIdentityTag = TTD_INVALID_IDENTITY_TAG;

            snpObject->OptDependsOnInfo = nullptr;
            //AddtlSnapObjectInfo must be set later in type specific extract code
        }

        void StdPropertyExtract_DynamicType(SnapObject* snpObject, Js::DynamicObject* dynObj, SlabAllocator& alloc)
        {
            NSSnapType::SnapType* sType = snpObject->SnapType;

            if(sType->TypeHandlerInfo->MaxPropertyIndex == 0)
            {
                snpObject->VarArrayCount = 0;
                snpObject->VarArray = nullptr;
            }
            else
            {
                NSSnapType::SnapHandler* sHandler = sType->TypeHandlerInfo;

                snpObject->VarArrayCount = sHandler->MaxPropertyIndex;
                snpObject->VarArray = alloc.SlabAllocateArray<TTDVar>(snpObject->VarArrayCount);

                TTDVar* cpyBase = snpObject->VarArray;
                if(sHandler->InlineSlotCapacity != 0)
                {
                    Js::Var* inlineSlots = dynObj->GetInlineSlots_TTD();

                    //copy all the properties (if they all fit into the inline slots) otherwise just copy all the inline slot values
                    uint32 inlineSlotCount = min(sHandler->MaxPropertyIndex, sHandler->InlineSlotCapacity);
                    memcpy(cpyBase, inlineSlots, inlineSlotCount * sizeof(Js::Var));
                }

                if(sHandler->MaxPropertyIndex > sHandler->InlineSlotCapacity)
                {
                    cpyBase = cpyBase + sHandler->InlineSlotCapacity;
                    Js::Var* auxSlots = dynObj->GetAuxSlots_TTD();

                    //there are some values in aux slots (in addition to the inline slots) so copy them as well
                    uint32 auxSlotCount = (sHandler->MaxPropertyIndex - sHandler->InlineSlotCapacity);
                    memcpy(cpyBase, auxSlots, auxSlotCount * sizeof(Js::Var));
                }
            }

            Js::ArrayObject* parray = dynObj->GetObjectArray();
            snpObject->OptIndexedObjectArray = (parray == nullptr) ? TTD_INVALID_PTR_ID : TTD_CONVERT_VAR_TO_PTR_ID(parray);

#if ENABLE_TTD_IDENTITY_TRACING
            snpObject->ObjectIdentityTag = dynObj->TTDObjectIdentityTag;
#else
            snpObject->ObjectIdentityTag = TTD_INVALID_IDENTITY_TAG;
#endif

            snpObject->OptDependsOnInfo = nullptr;
            //AddtlSnapObjectInfo must be set later in type specific extract code
        }

        Js::DynamicObject* ReuseObjectCheckAndReset(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::RecyclableObject* robj = inflator->FindReusableObjectIfExists(snpObject->ObjectPtrId);
            if(robj == nullptr)
            {
                return nullptr;
            }
            AssertMsg(Js::DynamicType::Is(robj->GetTypeId()), "You should only do this for dynamic objects!!!");

            Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(robj);
            JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& propertyReset = inflator->GetPropertyResetSet();
            propertyReset.Clear();

            ////

            for(int32 i = 0; i < dynObj->GetPropertyCount(); i++)
            {
                Js::PropertyId pid = dynObj->GetPropertyId((Js::PropertyIndex)i);
                propertyReset.AddNew(pid);
            }

            const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;
            for(uint32 i = 0; i < handler->MaxPropertyIndex; ++i)
            {
                if(handler->PropertyInfoArray[i].DataKind != NSSnapType::SnapEntryDataKindTag::Clear)
                {
                    Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;
                    propertyReset.Remove(pid);
                }
            }

            if(propertyReset.Count() != 0)
            {
                for(auto iter = propertyReset.GetIterator(); iter.IsValid(); iter.MoveNext())
                {
                    dynObj->DeleteProperty(iter.CurrentValue(), Js::PropertyOperationFlags::PropertyOperation_Force);
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

        void StdPropertyRestore(const SnapObject* snpObject, Js::DynamicObject* obj, InflateMap* inflator)
        {
            if(snpObject->SnapType->PrototypeId != TTD_INVALID_PTR_ID)
            {
                Js::RecyclableObject* protoObj = Js::RecyclableObject::FromVar(inflator->LookupObject(snpObject->SnapType->PrototypeId));

                //Many protos are set at creation, don't mess with them if they are already correct
                if(obj->GetPrototype() != protoObj)
                {
                    AssertMsg(!Js::JavascriptProxy::Is(obj), "Why is proxy's prototype not the regular one?");

                    obj->SetPrototype(protoObj);
                }
            }

            //set all the standard properties
            const NSSnapType::SnapHandler* handler = snpObject->SnapType->TypeHandlerInfo;

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

                AssertMsg(!Js::JavascriptProxy::Is(obj), "I didn't think proxies could have real properties directly on them.");

                Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;
                TTDVar ttdVal = snpObject->VarArray[i];
                Js::Var pVal = inflator->InflateTTDVar(ttdVal);

                if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Data)
                {
                    if(!obj->HasOwnProperty(pid) || obj->IsWritable(pid))
                    {
                        obj->SetProperty(pid, pVal, Js::PropertyOperationFlags::PropertyOperation_Force, nullptr);
                    }
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
                        AssertMsg(false, "Don't know how to restore this accesstag!!");
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
            obj->GetDynamicType()->GetTypeHandler()->SetExtensibleFlag_TTD(handler->IsExtensibleFlag);
            obj->GetDynamicType()->SetHasNoEnumerableProperties(snpObject->SnapType->HasNoEnumerableProperties);
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

            writer->WriteLogTag(NSTokens::Key::logTag, snpObject->ObjectLogTag, NSTokens::Separator::CommaSeparator);
            writer->WriteIdentityTag(NSTokens::Key::identityTag, snpObject->ObjectIdentityTag, NSTokens::Separator::CommaSeparator);

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
                            Js::PropertyId pid = handler->PropertyInfoArray[i].PropertyRecordId;

                            writer->WriteRecordStart(varSep);
                            writer->WriteString(NSTokens::Key::name, threadContext->GetPropertyName(pid)->GetBuffer(), NSTokens::Separator::NoSeparator);
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
                snpObject->OptWellKnownToken = reader->ReadWellKnownToken(alloc, NSTokens::Key::wellKnownToken, true);
            }

            snpObject->SnapType = ptrIdToTypeMap.LookupKnownItem(reader->ReadAddr(NSTokens::Key::typeId, true));

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

            snpObject->ObjectLogTag = reader->ReadLogTag(NSTokens::Key::logTag, true);
            snpObject->ObjectIdentityTag = reader->ReadIdentityTag(NSTokens::Key::identityTag, true);

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
                        if(handler->PropertyInfoArray[i].DataKind == NSSnapType::SnapEntryDataKindTag::Clear)
                        {
                            reader->ReadNakedNull(i != 0);
                        }
                        else
                        {
                            bool readSeparator = (i != 0);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
                            reader->ReadRecordStart(readSeparator);
                            reader->ReadString(NSTokens::Key::name);
                            reader->ReadKey(NSTokens::Key::entry, true);

                            readSeparator = false;
#endif

                            snpObject->VarArray[i] = NSSnapValues::ParseTTDVar(readSeparator, reader);

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

        Js::RecyclableObject* DoObjectInflation_SnapDynamicObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
            if(rcObj != nullptr)
            {
                return rcObj;
            }
            else
            {
                Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
                return ctx->GetLibrary()->CreateObject();
            }
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapJSRTExternalInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
            if(rcObj != nullptr)
            {
                return rcObj;
            }
            else
            {
                Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

                //
                //TODO: we just pretend they are dynamic objects for now until we sort out what we want to do
                //
                return ctx->GetLibrary()->CreateObject();
            }
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapScriptFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
            if(rcObj != nullptr)
            {
                return rcObj;
            }
            else
            {
                Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
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

                func->SetHasInlineCaches(snapFuncInfo->HasInlineCaches);
                func->SetHasSuperReference(snapFuncInfo->HasSuperReference);
                func->SetIsDefaultConstructor(snapFuncInfo->IsDefaultConstructor);

                return func;
            }
        }

        void DoAddtlValueInstantiation_SnapScriptFunctionInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            Js::ScriptFunction* fobj = Js::ScriptFunction::FromVar(obj);
            SnapScriptFunctionInfo* snapFuncInfo = SnapObjectGetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject);

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

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            writer->WriteString(NSTokens::Key::name, snapFuncInfo->DebugFunctionName, NSTokens::Separator::CommaSeparator); //diagnostic only
#endif

            writer->WriteAddr(NSTokens::Key::scopeId, snapFuncInfo->ScopeId, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::ptrIdVal, snapFuncInfo->HomeObjId, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::nameInfo, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(snapFuncInfo->ComputedNameInfo, writer, NSTokens::Separator::NoSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, snapFuncInfo->HasInlineCaches, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, snapFuncInfo->HasSuperReference, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::boolVal, snapFuncInfo->IsDefaultConstructor, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapScriptFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapScriptFunctionInfo* snapFuncInfo = alloc.SlabAllocateStruct<SnapScriptFunctionInfo>();

            snapFuncInfo->BodyRefId = reader->ReadAddr(NSTokens::Key::functionBodyId, true);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            snapFuncInfo->DebugFunctionName = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::name, true));
#endif

            snapFuncInfo->ScopeId = reader->ReadAddr(NSTokens::Key::scopeId, true);
            snapFuncInfo->HomeObjId = reader->ReadAddr(NSTokens::Key::ptrIdVal, true);

            reader->ReadKey(NSTokens::Key::nameInfo, true);
            snapFuncInfo->ComputedNameInfo = NSSnapValues::ParseTTDVar(false, reader);

            snapFuncInfo->HasInlineCaches = reader->ReadBool(NSTokens::Key::boolVal, true);
            snapFuncInfo->HasSuperReference = reader->ReadBool(NSTokens::Key::boolVal, true);
            snapFuncInfo->IsDefaultConstructor = reader->ReadBool(NSTokens::Key::boolVal, true);

            SnapObjectSetAddtlInfoAs<SnapScriptFunctionInfo*, SnapObjectType::SnapScriptFunctionObject>(snpObject, snapFuncInfo);
        }

        Js::RecyclableObject* DoObjectInflation_SnapExternalFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            LPCWSTR snapName = SnapObjectGetAddtlInfoAs<LPCWSTR, SnapObjectType::SnapExternalFunctionObject>(snpObject);

            Js::JavascriptString* fname = Js::JavascriptString::NewCopyBuffer(snapName, wcslen(snapName), ctx);
            return ctx->GetLibrary()->CreateExternalFunction_TTD(fname);
        }

        void EmitAddtlInfo_SnapExternalFunctionInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            LPCWSTR snapName = SnapObjectGetAddtlInfoAs<LPCWSTR, SnapObjectType::SnapExternalFunctionObject>(snpObject);

            writer->WriteString(NSTokens::Key::name, snapName, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapExternalFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            LPCWSTR snapName = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::name, true));

            SnapObjectSetAddtlInfoAs<LPCWSTR, SnapObjectType::SnapExternalFunctionObject>(snpObject, snapName);
        }

        Js::RecyclableObject* DoObjectInflation_SnapRevokerFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

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

        Js::RecyclableObject* DoObjectInflation_SnapBoundFunctionInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Bound functions are not too common and have special internal state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            SnapBoundFunctionInfo* snapBoundInfo = SnapObjectGetAddtlInfoAs<SnapBoundFunctionInfo*, SnapObjectType::SnapBoundFunctionObject>(snpObject);

            Js::RecyclableObject* bFunction = inflator->LookupObject(snapBoundInfo->TargetFunction);
            Js::RecyclableObject* bThis = (snapBoundInfo->BoundThis != TTD_INVALID_PTR_ID) ? inflator->LookupObject(snapBoundInfo->BoundThis) : nullptr;

            Js::Var* bArgs = nullptr;
            if(snapBoundInfo->ArgCount != 0)
            {
                bArgs = RecyclerNewArray(ctx->GetRecycler(), Js::Var, snapBoundInfo->ArgCount);

                for(uint i = 0; i < snapBoundInfo->ArgCount; i++)
                {
                    bArgs[i] = inflator->InflateTTDVar(snapBoundInfo->ArgArray[i]);
                }
            }

            return ctx->GetLibrary()->CreateBoundFunction_TTD(bFunction, bThis, snapBoundInfo->ArgCount, bArgs);
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

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapActivationInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            return ctx->GetLibrary()->CreateActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapBlockActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            return ctx->GetLibrary()->CreateBlockActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapPseudoActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            return ctx->GetLibrary()->CreatePseudoActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapConsoleScopeActivationObject(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            return ctx->GetLibrary()->CreateConsoleScopeActivationObject();
        }

        Js::RecyclableObject* DoObjectInflation_SnapActivationExInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            //
            //TODO: I am going to assume the cached info is an optimization only so we just return a regular activation object here
            //

            return ctx->GetLibrary()->CreateActivationObject();
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapHeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            SnapHeapArgumentsInfo* argsInfo = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, SnapObjectType::SnapHeapArgumentsObject>(snpObject);

            Js::RecyclableObject* activationObj = (argsInfo->FrameObject != TTD_INVALID_PTR_ID) ? inflator->LookupObject(argsInfo->FrameObject) : nullptr;

            return  ctx->GetLibrary()->CreateHeapArguments_TTD(argsInfo->NumOfArguments, argsInfo->FormalCount, static_cast<Js::ActivationObject*>(activationObj), argsInfo->DeletedArgFlags);
        }

        void EmitAddtlInfo_SnapHeapArgumentsInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapHeapArgumentsInfo* argsInfo = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, SnapObjectType::SnapHeapArgumentsObject>(snpObject);

            writer->WriteUInt32(NSTokens::Key::numberOfArgs, argsInfo->NumOfArguments, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteAddr(NSTokens::Key::objectId, argsInfo->FrameObject, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteLengthValue(argsInfo->FormalCount, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::deletedArgs, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart();
            for(uint32 i = 0; i < argsInfo->FormalCount; ++i)
            {
               writer->WriteNakedByte(argsInfo->DeletedArgFlags[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        void ParseAddtlInfo_SnapHeapArgumentsInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapHeapArgumentsInfo* argsInfo = alloc.SlabAllocateStruct<SnapHeapArgumentsInfo>();

            argsInfo->NumOfArguments = reader->ReadUInt32(NSTokens::Key::numberOfArgs, true);

            argsInfo->FrameObject = reader->ReadAddr(NSTokens::Key::objectId, true);

            argsInfo->FormalCount = reader->ReadLengthValue(true);

            if(argsInfo->FormalCount == 0)
            {
                argsInfo->DeletedArgFlags = nullptr;
            }
            else
            {
                argsInfo->DeletedArgFlags = alloc.SlabAllocateArray<byte>(argsInfo->FormalCount);
            }

            reader->ReadKey(NSTokens::Key::deletedArgs, true);
            reader->ReadSequenceStart();
            for(uint32 i = 0; i < argsInfo->FormalCount; ++i)
            {
                argsInfo->DeletedArgFlags[i] = reader->ReadNakedByte(i != 0);
            }
            reader->ReadSequenceEnd();

            SnapObjectSetAddtlInfoAs<SnapHeapArgumentsInfo*, SnapObjectType::SnapHeapArgumentsObject>(snpObject, argsInfo);
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapBoxedValue(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Boxed values are not too common and have special internal state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            Js::JavascriptLibrary* jslib = ctx->GetLibrary();

            TTDVar snapBoxedVar = SnapObjectGetAddtlInfoAs<TTDVar, SnapObjectType::SnapBoxedValueObject>(snpObject);

            if(snapBoxedVar == nullptr)
            {
                switch(snpObject->SnapType->JsTypeId)
                {
                case Js::TypeIds_BooleanObject:
                    return jslib->CreateBooleanObject_TTD(nullptr);
                case Js::TypeIds_StringObject:
                    return jslib->CreateStringObject_TTD(nullptr);
                case Js::TypeIds_SymbolObject:
                    return jslib->CreateSymbolObject_TTD(nullptr);
                default:
                    AssertMsg(false, "Unsupported nullptr value boxed object.");
                    return nullptr;
                }
            }
            else
            {
                Js::Var jsvar = inflator->InflateTTDVar(snapBoxedVar);

                switch(snpObject->SnapType->JsTypeId)
                {
                case Js::TypeIds_BooleanObject:
                    return jslib->CreateBooleanObject_TTD(jsvar);
                case Js::TypeIds_NumberObject:
                    return jslib->CreateNumberObject_TTD(jsvar);
                case Js::TypeIds_StringObject:
                    return jslib->CreateStringObject_TTD(jsvar);
                case Js::TypeIds_SymbolObject:
                    return jslib->CreateSymbolObject_TTD(jsvar);
                default:
                    AssertMsg(false, "Unknown Js::TypeId for SnapBoxedValueObject");
                    return nullptr;
                }
            }
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

        Js::RecyclableObject* DoObjectInflation_SnapDate(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Dates are not too common and have some mutable state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
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

        Js::RecyclableObject* DoObjectInflation_SnapRegexInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Dates are not too common and have some mutable state so it seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            SnapRegexInfo* regexInfo = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject);

            return ctx->GetLibrary()->CreateRegex_TTD(regexInfo->RegexStr, regexInfo->Flags, regexInfo->LastIndexOrFlag);
        }

        void EmitAddtlInfo_SnapRegexInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapRegexInfo* regexInfo = SnapObjectGetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject);

            writer->WriteString(NSTokens::Key::stringVal, regexInfo->RegexStr, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteTag<UnifiedRegex::RegexFlags>(NSTokens::Key::attributeFlags, regexInfo->Flags, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::u32Val, regexInfo->LastIndexOrFlag, NSTokens::Separator::CommaSeparator);
        }

        void ParseAddtlInfo_SnapRegexInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapRegexInfo* regexInfo = alloc.SlabAllocateStruct<SnapRegexInfo>();

            regexInfo->RegexStr = alloc.CopyStringInto(reader->ReadString(NSTokens::Key::stringVal, true));

            regexInfo->Flags = reader->ReadTag<UnifiedRegex::RegexFlags>(NSTokens::Key::attributeFlags, true);
            regexInfo->LastIndexOrFlag = reader->ReadUInt32(NSTokens::Key::u32Val, true);

            SnapObjectSetAddtlInfoAs<SnapRegexInfo*, SnapObjectType::SnapRegexObject>(snpObject, regexInfo);
        }

        Js::RecyclableObject* DoObjectInflation_SnapError(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

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

        Js::RecyclableObject* DoObjectInflation_SnapArrayInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Arrays can change type on us so seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed and add checks for same type-ness.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Array)
            {
                return ctx->GetLibrary()->CreateArray();
            }
            else if(snpObject->SnapType->JsTypeId == Js::TypeIds_NativeIntArray)
            {
                return ctx->GetLibrary()->CreateNativeIntArray();
            }
            else if(snpObject->SnapType->JsTypeId == Js::TypeIds_NativeFloatArray)
            {
                return ctx->GetLibrary()->CreateNativeFloatArray();
            }
            else
            {
                AssertMsg(false, "Unknown array type!");
                return nullptr;
            }
        }

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapArrayBufferInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //ArrayBuffers can change on us so seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            SnapArrayBufferInfo* buffInfo = SnapObjectGetAddtlInfoAs<SnapArrayBufferInfo*, SnapObjectType::SnapArrayBufferObject>(snpObject);

            Js::ArrayBuffer* abuff = ctx->GetLibrary()->CreateArrayBuffer(buffInfo->Length);
            AssertMsg(abuff->GetByteLength() == buffInfo->Length, "Something is wrong with our sizes.");

            memcpy(abuff->GetBuffer(), buffInfo->Buff, buffInfo->Length);

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

        Js::RecyclableObject* DoObjectInflation_SnapTypedArrayInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //I am lazy and always re-create typed arrays.
            //We can re-evaluate this choice later if needed.

            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
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
                AssertMsg(false, "Not a typed array!");
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

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapSetInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Set)
            {
                return ctx->GetLibrary()->CreateSet_TTD();
            }
            else
            {
                //
                //TODO: we need to have weak sets keep track of when they GC/realease objects -- probably as part of log
                //
                AssertMsg(false, "WeakSet not supported yet!!!");
                return nullptr;
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
                AssertMsg(false, "WeakSet not supported yet!!!");
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

        Js::RecyclableObject* DoObjectInflation_SnapMapInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Map)
            {
                return ctx->GetLibrary()->CreateMap_TTD();
            }
            else
            {
                //
                //TODO: we need to have weak maps keep track of when they GC/realease objects -- probably as part of log
                //
                AssertMsg(false, "WeakMap not supported yet!!!");
                return nullptr;
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
                AssertMsg(false, "WeakMap not supported yet!!!");
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

        //////////////////

        Js::RecyclableObject* DoObjectInflation_SnapProxyInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);

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
    }
}

#endif
