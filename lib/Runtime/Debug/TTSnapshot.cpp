//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    void SnapShot::EmitSnapshotToFile(FileWriter* writer, ThreadContext* threadContext) const
    {
        TTDTimer timer;
        double startWrite = timer.Now();

        writer->WriteRecordStart();
        writer->AdjustIndent(1);

        uint64 usedSpace = 0;
        uint64 reservedSpace = 0;
        this->ComputeSnapshotMemory(&usedSpace, &reservedSpace);

        writer->WriteDouble(NSTokens::Key::timeTotal, this->GCTime + this->MarkTime + this->ExtractTime);
        writer->WriteUInt64(NSTokens::Key::usedMemory, usedSpace, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt64(NSTokens::Key::reservedMemory, reservedSpace, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::timeGC, this->GCTime, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::timeMark, this->MarkTime, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::timeExtract, this->ExtractTime, NSTokens::Separator::CommaSeparator);

        writer->WriteLengthValue(this->m_ctxList.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->AdjustIndent(1);
        bool firstCtx = true;
        for(auto iter = this->m_ctxList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSSnapValues::EmitSnapContext(iter.Current(), writer, firstCtx ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);

            firstCtx = false;
        }
        writer->AdjustIndent(-1);
        writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        writer->WriteLengthValue(this->m_tcSymbolRegistrationMapContents.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
        bool firstTCSymbol = true;
        for(auto iter = this->m_tcSymbolRegistrationMapContents.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            writer->WriteNakedUInt32((uint32)*iter.Current(), firstTCSymbol ? NSTokens::Separator::NoSeparator : NSTokens::Separator::CommaSeparator);

            firstTCSymbol = false;
        }
        writer->WriteSequenceEnd();

        writer->WriteLogTag(NSTokens::Key::ctxTag, this->m_activeScriptContext, NSTokens::Separator::CommaAndBigSpaceSeparator);
        SnapShot::EmitListHelper(&SnapShot::SnapRootPinEntryEmit, this->m_rootList, writer);

        ////
        SnapShot::EmitListHelper(&NSSnapType::EmitSnapHandler, this->m_handlerList, writer);
        SnapShot::EmitListHelper(&NSSnapType::EmitSnapType, this->m_typeList, writer);

        ////
        writer->WriteLengthValue(this->m_functionBodyList.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->AdjustIndent(1);
        bool firstBody = true;
        for(auto iter = this->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSSnapValues::EmitFunctionBodyInfo(iter.Current(), writer, firstBody ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);

            firstBody = false;
        }
        writer->AdjustIndent(-1);
        writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        SnapShot::EmitListHelper(&NSSnapValues::EmitSnapPrimitiveValue, this->m_primitiveObjectList, writer);

        writer->WriteLengthValue(this->m_compoundObjectList.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->AdjustIndent(1);
        bool firstObj = true;
        for(auto iter = this->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSSnapObjects::EmitObject(iter.Current(), writer, firstObj ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator, this->m_snapObjectVTableArray, threadContext);

            firstObj = false;
        }
        writer->AdjustIndent(-1);
        writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

        ////
        SnapShot::EmitListHelper(&NSSnapValues::EmitScriptFunctionScopeInfo, this->m_scopeEntries, writer);
        SnapShot::EmitListHelper(&NSSnapValues::EmitSlotArrayInfo, this->m_slotArrayEntries, writer);

        ////
        double almostEndWrite = timer.Now();
        writer->WriteDouble(NSTokens::Key::timeWrite, (almostEndWrite - startWrite) / 1000.0, NSTokens::Separator::CommaAndBigSpaceSeparator);

        writer->AdjustIndent(-1);
        writer->WriteRecordEnd(NSTokens::Separator::BigSpaceSeparator);
    }

    SnapShot* SnapShot::ParseSnapshotFromFile(FileReader* reader)
    {
        reader->ReadRecordStart();

        reader->ReadDouble(NSTokens::Key::timeTotal);
        reader->ReadUInt64(NSTokens::Key::usedMemory, true);
        reader->ReadUInt64(NSTokens::Key::reservedMemory, true);
        reader->ReadDouble(NSTokens::Key::timeGC, true);
        reader->ReadDouble(NSTokens::Key::timeMark, true);
        reader->ReadDouble(NSTokens::Key::timeExtract, true);

        SnapShot* snap = TT_HEAP_NEW(SnapShot, 0.0);

        uint32 ctxCount = reader->ReadLengthValue(true);
        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < ctxCount; ++i)
        {
            NSSnapValues::SnapContext* snpCtx = snap->m_ctxList.NextOpenEntry();
            NSSnapValues::ParseSnapContext(snpCtx, i != 0, reader, snap->GetSnapshotSlabAllocator());
        }
        reader->ReadSequenceEnd();

        uint32 tcSymbolCount = reader->ReadLengthValue(true);
        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < tcSymbolCount; ++i)
        {
            Js::PropertyId* symid = snap->m_tcSymbolRegistrationMapContents.NextOpenEntry();
            *symid = reader->ReadNakedUInt32(i != 0);
        }
        reader->ReadSequenceEnd();

        snap->m_activeScriptContext = reader->ReadLogTag(NSTokens::Key::ctxTag, true);
        SnapShot::ParseListHelper(&SnapShot::SnapRootPinEntryParse, snap->m_rootList, reader, snap->GetSnapshotSlabAllocator());

        ////

        SnapShot::ParseListHelper(&NSSnapType::ParseSnapHandler, snap->m_handlerList, reader, snap->GetSnapshotSlabAllocator());

        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapHandler*> handlerMap;
        handlerMap.Initialize(snap->m_handlerList.Count());
        for(auto iter = snap->m_handlerList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            handlerMap.AddItem(iter.Current()->HandlerId, iter.Current());
        }

        SnapShot::ParseListHelper_WMap(&NSSnapType::ParseSnapType, snap->m_typeList, reader, snap->GetSnapshotSlabAllocator(), handlerMap);
        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*> typeMap;
        typeMap.Initialize(snap->m_typeList.Count());

        for(auto iter = snap->m_typeList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            typeMap.AddItem(iter.Current()->TypePtrId, iter.Current());
        }

        ////
        uint32 bodyCount = reader->ReadLengthValue(true);
        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < bodyCount; ++i)
        {
            NSSnapValues::FunctionBodyResolveInfo* into = snap->m_functionBodyList.NextOpenEntry();
            NSSnapValues::ParseFunctionBodyInfo(into, i != 0, reader, snap->GetSnapshotSlabAllocator());
        }
        reader->ReadSequenceEnd();

        SnapShot::ParseListHelper_WMap(&NSSnapValues::ParseSnapPrimitiveValue, snap->m_primitiveObjectList, reader, snap->GetSnapshotSlabAllocator(), typeMap);

        uint32 objCount = reader->ReadLengthValue(true);
        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < objCount; ++i)
        {
            NSSnapObjects::SnapObject* into = snap->m_compoundObjectList.NextOpenEntry();
            NSSnapObjects::ParseObject(into, i != 0, reader, snap->GetSnapshotSlabAllocator(), snap->m_snapObjectVTableArray, typeMap);
        }
        reader->ReadSequenceEnd();

        ////
        SnapShot::ParseListHelper(&NSSnapValues::ParseScriptFunctionScopeInfo, snap->m_scopeEntries, reader, snap->GetSnapshotSlabAllocator());
        SnapShot::ParseListHelper(&NSSnapValues::ParseSlotArrayInfo, snap->m_slotArrayEntries, reader, snap->GetSnapshotSlabAllocator());

        reader->ReadDouble(NSTokens::Key::timeWrite, true);

        reader->ReadRecordEnd();

        return snap;
    }

    void SnapShot::InflateSingleObject(const NSSnapObjects::SnapObject* snpObject, InflateMap* inflator, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapObjects::SnapObject*>& idToSnpObjectMap) const
    {
        if(inflator->IsObjectAlreadyInflated(snpObject->ObjectPtrId))
        {
            return;
        }

        if(snpObject->OptDependsOnInfo != nullptr)
        {
            for(uint32 i = 0; i < snpObject->OptDependsOnInfo->DepOnCount; ++i)
            {
                const NSSnapObjects::SnapObject* depOnObj = idToSnpObjectMap.LookupKnownItem(snpObject->OptDependsOnInfo->DepOnPtrArray[i]);

                //This is recursive but should be shallow
                this->InflateSingleObject(depOnObj, inflator, idToSnpObjectMap);
            }
        }

        Js::RecyclableObject* res = nullptr;
        if(snpObject->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);
            res = ctx->TTDWellKnownInfo->LookupKnownObjectFromPath(snpObject->OptWellKnownToken);

            //Well known objects may always be dirty (e.g. we are re-using a context) so we always want to clean them
            res = NSSnapObjects::ObjectPropertyReset_WellKnown(snpObject, Js::DynamicObject::FromVar(res), inflator);
            TTDAssert(res != nullptr, "Should always produce a result!!!");
        }
        else
        {
            //lookup the inflator function for this object and call it
            NSSnapObjects::fPtr_DoObjectInflation inflateFPtr = this->m_snapObjectVTableArray[(uint32)snpObject->SnapObjectTag].InflationFunc;
            TTDAssert(inflateFPtr != nullptr, "We probably forgot to update the vtable with a tag we added.");

            res = inflateFPtr(snpObject, inflator);
        }

        if(Js::DynamicType::Is(snpObject->SnapType->JsTypeId))
        {
            //Always ok to be x-site but if snap was x-site then we must be too
            Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(res);
            if(snpObject->IsCrossSite && !dynObj->IsCrossSiteObject())
            {
                Js::CrossSite::MarshalCrossSite_TTDInflate(dynObj);
            }
        }

        inflator->AddObject(snpObject->ObjectPtrId, res);
    }

    void SnapShot::ReLinkThreadContextInfo(InflateMap* inflator, ThreadContextTTD* intoCtx) const
    {
        for(auto iter = this->m_rootList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapRootInfoEntry* rootEntry = iter.Current();
            Js::RecyclableObject* rootObj = inflator->LookupObject(rootEntry->LogObject);

            intoCtx->ForceSetRootInfoInRestore(rootEntry->LogId, rootObj, rootEntry->MaybeLongLivedRoot);
        }

        if(this->m_activeScriptContext == TTD_INVALID_LOG_PTR_ID)
        {
            intoCtx->TTDExternalObjectFunctions.pfSetActiveJsRTContext(intoCtx->GetRuntimeHandle(), nullptr);
        }
        else
        {
            Js::ScriptContext* ctx = inflator->LookupScriptContext(this->m_activeScriptContext);
            intoCtx->TTDExternalObjectFunctions.pfSetActiveJsRTContext(intoCtx->GetRuntimeHandle(), ctx);
        }
    }

    void SnapShot::SnapRootPinEntryEmit(const NSSnapValues::SnapRootInfoEntry* spe, FileWriter* snapwriter, NSTokens::Separator separator)
    {
        snapwriter->WriteRecordStart(separator);
        snapwriter->WriteLogTag(NSTokens::Key::logTag, spe->LogId);
        snapwriter->WriteAddr(NSTokens::Key::objectId, spe->LogObject, NSTokens::Separator::CommaSeparator);
        snapwriter->WriteBool(NSTokens::Key::boolVal, spe->MaybeLongLivedRoot, NSTokens::Separator::CommaSeparator);
        snapwriter->WriteRecordEnd();
    }

    void SnapShot::SnapRootPinEntryParse(NSSnapValues::SnapRootInfoEntry* spe, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
    {
        reader->ReadRecordStart(readSeperator);
        spe->LogId = reader->ReadLogTag(NSTokens::Key::logTag);
        spe->LogObject = reader->ReadAddr(NSTokens::Key::objectId, true);
        spe->MaybeLongLivedRoot = reader->ReadBool(NSTokens::Key::boolVal, true);
        reader->ReadRecordEnd();
    }

    void SnapShot::ComputeSnapshotMemory(uint64* usedSpace, uint64* reservedSpace) const
    {
        return this->m_slabAllocator.ComputeMemoryUsed(usedSpace, reservedSpace);
    }

    SnapShot::SnapShot(double gcTime)
        : m_slabAllocator(TTD_SLAB_BLOCK_ALLOCATION_SIZE_LARGE),
        m_ctxList(&this->m_slabAllocator), m_tcSymbolRegistrationMapContents(&this->m_slabAllocator), m_activeScriptContext(TTD_INVALID_LOG_PTR_ID),
        m_rootList(&this->m_slabAllocator),
        m_handlerList(&this->m_slabAllocator), m_typeList(&this->m_slabAllocator),
        m_functionBodyList(&this->m_slabAllocator), m_primitiveObjectList(&this->m_slabAllocator), m_compoundObjectList(&this->m_slabAllocator),
        m_scopeEntries(&this->m_slabAllocator), m_slotArrayEntries(&this->m_slabAllocator),
        m_snapObjectVTableArray(nullptr),
        GCTime(gcTime), MarkTime(0.0), ExtractTime(0.0)
    {
        this->m_snapObjectVTableArray = this->m_slabAllocator.SlabAllocateArray<NSSnapObjects::SnapObjectVTable>((uint32)NSSnapObjects::SnapObjectType::Limit);
        memset(this->m_snapObjectVTableArray, 0, sizeof(NSSnapObjects::SnapObjectVTable) * (uint32)NSSnapObjects::SnapObjectType::Limit);

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::Invalid] = { nullptr, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapUnhandledObject] = { nullptr, nullptr, nullptr, nullptr };

        ////
        //For the objects that have inflators

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapDynamicObject] = { &NSSnapObjects::DoObjectInflation_SnapDynamicObject, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapExternalObject] = { &NSSnapObjects::DoObjectInflation_SnapExternalObject, nullptr, nullptr, nullptr };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapScriptFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapScriptFunctionInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapScriptFunctionInfo, &NSSnapObjects::EmitAddtlInfo_SnapScriptFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapScriptFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapRuntimeFunctionObject] = { nullptr, nullptr, nullptr, nullptr }; //should always be wellknown objects and the extra state is in the functionbody defs
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapExternalFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapExternalFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapExternalFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapExternalFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapRuntimeRevokerFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapRevokerFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapRevokerFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapRevokerFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapBoundFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapBoundFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapBoundFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapBoundFunctionInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapActivationObject] = { &NSSnapObjects::DoObjectInflation_SnapActivationInfo, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapBlockActivationObject] = { &NSSnapObjects::DoObjectInflation_SnapBlockActivationObject, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapPseudoActivationObject] = { &NSSnapObjects::DoObjectInflation_SnapPseudoActivationObject, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapConsoleScopeActivationObject] = { &NSSnapObjects::DoObjectInflation_SnapConsoleScopeActivationObject, nullptr, nullptr, nullptr };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject] = { &NSSnapObjects::DoObjectInflation_SnapHeapArgumentsInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject>, &NSSnapObjects::ParseAddtlInfo_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject] = { &NSSnapObjects::DoObjectInflation_SnapES5HeapArgumentsInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject>, &NSSnapObjects::ParseAddtlInfo_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject> };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapBoxedValueObject] = { &NSSnapObjects::DoObjectInflation_SnapBoxedValue, &NSSnapObjects::DoAddtlValueInstantiation_SnapBoxedValue, &NSSnapObjects::EmitAddtlInfo_SnapBoxedValue, &NSSnapObjects::ParseAddtlInfo_SnapBoxedValue };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapDateObject] = { &NSSnapObjects::DoObjectInflation_SnapDate, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapDate, &NSSnapObjects::ParseAddtlInfo_SnapDate };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapRegexObject] = { &NSSnapObjects::DoObjectInflation_SnapRegexInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapRegexInfo, &NSSnapObjects::ParseAddtlInfo_SnapRegexInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapErrorObject] = { &NSSnapObjects::DoObjectInflation_SnapError, nullptr, nullptr, nullptr };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo<Js::Var, NSSnapObjects::SnapObjectType::SnapArrayObject>, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<TTDVar, Js::Var, NSSnapObjects::SnapObjectType::SnapArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<TTDVar, NSSnapObjects::SnapObjectType::SnapArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<TTDVar, NSSnapObjects::SnapObjectType::SnapArrayObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo<int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<int32, int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<double, double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapES5ArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapES5ArrayInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapES5ArrayInfo, &NSSnapObjects::EmitAddtlInfo_SnapES5ArrayInfo, &NSSnapObjects::ParseAddtlInfo_SnapES5ArrayInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapArrayBufferObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayBufferInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapArrayBufferInfo, &NSSnapObjects::ParseAddtlInfo_SnapArrayBufferInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapTypedArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapTypedArrayInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapTypedArrayInfo, &NSSnapObjects::ParseAddtlInfo_SnapTypedArrayInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapSetObject] = { &NSSnapObjects::DoObjectInflation_SnapSetInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapSetInfo, &NSSnapObjects::EmitAddtlInfo_SnapSetInfo, &NSSnapObjects::ParseAddtlInfo_SnapSetInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapMapObject] = { &NSSnapObjects::DoObjectInflation_SnapMapInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapMapInfo, &NSSnapObjects::EmitAddtlInfo_SnapMapInfo, &NSSnapObjects::ParseAddtlInfo_SnapMapInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapProxyObject] = { &NSSnapObjects::DoObjectInflation_SnapProxyInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapProxyInfo, &NSSnapObjects::ParseAddtlInfo_SnapProxyInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapPromiseObject] = { &NSSnapObjects::DoObjectInflation_SnapPromiseInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapPromiseInfo, &NSSnapObjects::ParseAddtlInfo_SnapPromiseInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapPromiseResolveOrRejectFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapPromiseReactionTaskFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapPromiseReactionTaskFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapPromiseReactionTaskFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapPromiseReactionTaskFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapPromiseAllResolveElementFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapPromiseAllResolveElementFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapPromiseAllResolveElementFunctionInfo };

        ////
        //For the objects that are always well known

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapWellKnownObject] = { nullptr, nullptr, nullptr, nullptr };
    }

    SnapShot::~SnapShot()
    {
       ;
    }

    uint32 SnapShot::ContextCount() const
    {
        return this->m_ctxList.Count();
    }

    uint32 SnapShot::HandlerCount() const
    {
        return this->m_handlerList.Count();
    }

    uint32 SnapShot::TypeCount() const
    {
        return this->m_typeList.Count();
    }

    uint32 SnapShot::BodyCount() const
    {
        return this->m_functionBodyList.Count();
    }

    uint32 SnapShot::PrimitiveCount() const
    {
        return this->m_primitiveObjectList.Count();
    }

    uint32 SnapShot::ObjectCount() const
    {
        return this->m_compoundObjectList.Count();
    }

    uint32 SnapShot::EnvCount() const
    {
        return this->m_scopeEntries.Count();
    }

    uint32 SnapShot::SlotArrayCount() const
    {
        return this->m_slotArrayEntries.Count();
    }

    uint32 SnapShot::GetDbgScopeCountNonTopLevel() const
    {
        uint32 dbgScopeCount = 0;
        for(auto iter = this->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext()) 
        {
            dbgScopeCount += iter.Current()->ScopeChainInfo.ScopeCount;
        }

        return dbgScopeCount;
    }

    UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& SnapShot::GetContextList()
    {
        return this->m_ctxList;
    }

    const UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& SnapShot::GetContextList() const
    {
        return this->m_ctxList;
    }

    UnorderedArrayList<Js::PropertyId, TTD_ARRAY_LIST_SIZE_XSMALL>& SnapShot::GetTCSymbolMapInfoList()
    {
        return this->m_tcSymbolRegistrationMapContents;
    }

    TTD_LOG_PTR_ID SnapShot::GetActiveScriptContext() const
    {
        return this->m_activeScriptContext;
    }

    void SnapShot::SetActiveScriptContext(TTD_LOG_PTR_ID activeCtx)
    {
        this->m_activeScriptContext = activeCtx;
    }

    UnorderedArrayList<NSSnapValues::SnapRootInfoEntry, TTD_ARRAY_LIST_SIZE_MID>& SnapShot::GetRootList()
    {
        return this->m_rootList;
    }

    NSSnapType::SnapHandler* SnapShot::GetNextAvailableHandlerEntry()
    {
        return this->m_handlerList.NextOpenEntry();
    }

    NSSnapType::SnapType* SnapShot::GetNextAvailableTypeEntry()
    {
        return this->m_typeList.NextOpenEntry();
    }

    NSSnapValues::FunctionBodyResolveInfo* SnapShot::GetNextAvailableFunctionBodyResolveInfoEntry()
    {
        return this->m_functionBodyList.NextOpenEntry();
    }

    NSSnapValues::SnapPrimitiveValue* SnapShot::GetNextAvailablePrimitiveObjectEntry()
    {
        return this->m_primitiveObjectList.NextOpenEntry();
    }

    NSSnapObjects::SnapObject* SnapShot::GetNextAvailableCompoundObjectEntry()
    {
        return this->m_compoundObjectList.NextOpenEntry();
    }

    NSSnapValues::ScriptFunctionScopeInfo* SnapShot::GetNextAvailableFunctionScopeEntry()
    {
        return this->m_scopeEntries.NextOpenEntry();
    }

    NSSnapValues::SlotArrayInfo* SnapShot::GetNextAvailableSlotArrayEntry()
    {
        return this->m_slotArrayEntries.NextOpenEntry();
    }

    SlabAllocator& SnapShot::GetSnapshotSlabAllocator()
    {
        return this->m_slabAllocator;
    }

    bool SnapShot::AllWellKnownObjectsReusable(InflateMap* inflator) const
    {
        for(auto iter = this->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapObjects::SnapObject* snpObj = iter.Current();
            if(snpObj->OptWellKnownToken != TTD_INVALID_WELLKNOWN_TOKEN)
            {
                Js::RecyclableObject* rObj = inflator->FindReusableObject_WellKnowReuseCheck(snpObj->ObjectPtrId);
                bool blocking = NSSnapObjects::DoesObjectBlockScriptContextReuse(snpObj, Js::DynamicObject::FromVar(rObj), inflator);

                if(blocking)
                {
                    return false;
                }
            }
        }

        return true;
    }

    void SnapShot::Inflate(InflateMap* inflator, ThreadContextTTD* tCtx) const
    {
        //We assume the caller has inflated all of the ScriptContexts for us and we are just filling in the objects

        ////

        //set the map from all function body ids to their snap representations
        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapValues::FunctionBodyResolveInfo*> idToSnpBodyMap;
        idToSnpBodyMap.Initialize(this->m_functionBodyList.Count());

        for(auto iter = this->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            idToSnpBodyMap.AddItem(iter.Current()->FunctionBodyId, iter.Current());
        }

        //set the map from all compound object ids to their snap representations
        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapObjects::SnapObject*> idToSnpObjectMap;
        idToSnpObjectMap.Initialize(this->m_compoundObjectList.Count());

        for(auto iter = this->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            idToSnpObjectMap.AddItem(iter.Current()->ObjectPtrId, iter.Current());
        }

        ////

        //inflate all the function bodies
        for(auto iter = this->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::FunctionBodyResolveInfo* fbInfo = iter.Current();
            NSSnapValues::InflateFunctionBody(fbInfo, inflator, idToSnpBodyMap);
        }

        //inflate all the primitive objects
        for(auto iter = this->m_primitiveObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapPrimitiveValue* pSnap = iter.Current();
            NSSnapValues::InflateSnapPrimitiveValue(pSnap, inflator);
        }

        //inflate all the regular objects
        for(auto iter = this->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapObjects::SnapObject* sObj = iter.Current();
            this->InflateSingleObject(sObj, inflator, idToSnpObjectMap);
        }

        //take care of all the slot arrays
        for(auto iter = this->m_slotArrayEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SlotArrayInfo* sai = iter.Current();
            Js::Var* slots = NSSnapValues::InflateSlotArrayInfo(sai, inflator);

            inflator->AddSlotArray(sai->SlotId, slots);
        }

        //and the scope entries
        for(auto iter = this->m_scopeEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::ScriptFunctionScopeInfo* sfsi = iter.Current();
            Js::FrameDisplay* frame = NSSnapValues::InflateScriptFunctionScopeInfo(sfsi, inflator);

            inflator->AddEnvironment(sfsi->ScopeId, frame);
        }

        //Link up the object pointers
        for(auto iter = this->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapObjects::SnapObject* sobj = iter.Current();
            Js::RecyclableObject* iobj = inflator->LookupObject(sobj->ObjectPtrId);

            NSSnapObjects::fPtr_DoAddtlValueInstantiation addtlInstFPtr = this->m_snapObjectVTableArray[(uint32)sobj->SnapObjectTag].AddtlInstationationFunc;
            if(addtlInstFPtr != nullptr)
            {
                addtlInstFPtr(sobj, iobj, inflator);
            }

            if(Js::DynamicType::Is(sobj->SnapType->JsTypeId))
            {
                NSSnapObjects::StdPropertyRestore(sobj, Js::DynamicObject::FromVar(iobj), inflator);
            }
        }

        this->ReLinkThreadContextInfo(inflator, tCtx);

        for(auto iter = this->m_ctxList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapContext* snpCtx = iter.Current();
            Js::ScriptContext* sctx = inflator->LookupScriptContext(snpCtx->ScriptContextLogId);

            NSSnapValues::ResetPendingAsyncBufferModInfo(snpCtx, sctx, inflator);
        }

        //reset the threadContext symbol map
        JsUtil::BaseDictionary<Js::HashedCharacterBuffer<char16>*, const Js::PropertyRecord*, Recycler, PowerOf2SizePolicy, Js::PropertyRecordStringHashComparer>* tcSymbolRegistrationMap = tCtx->GetThreadContext()->GetSymbolRegistrationMap_TTD();
        tcSymbolRegistrationMap->Clear();

        for(auto iter = this->m_tcSymbolRegistrationMapContents.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::PropertyId pid = *iter.Current();
            const Js::PropertyRecord* pRecord = tCtx->GetThreadContext()->GetPropertyName(pid);
            Js::HashedCharacterBuffer<char16> * propertyName = RecyclerNew(tCtx->GetThreadContext()->GetRecycler(), Js::HashedCharacterBuffer<char16>, pRecord->GetBuffer(), pRecord->GetLength());

            tcSymbolRegistrationMap->Add(propertyName, pRecord);
        }
    }

    void SnapShot::EmitSnapshot(int64 snapId, ThreadContext* threadContext) const
    {
        char asciiResourceName[64];
        sprintf_s(asciiResourceName, 64, "snap_%I64i.snp", snapId);

        TTDataIOInfo& iofp = threadContext->TTDContext->TTDataIOInfo;
        JsTTDStreamHandle snapHandle = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(asciiResourceName), asciiResourceName, false, true);
        TTDAssert(snapHandle != nullptr, "Failed to open snapshot resource stream for writing.");

        TTD_SNAP_WRITER snapwriter(snapHandle, iofp.pfWriteBytesToStream, iofp.pfFlushAndCloseStream);

        this->EmitSnapshotToFile(&snapwriter, threadContext);
        snapwriter.FlushAndClose();
    }

    SnapShot* SnapShot::Parse(int64 snapId, ThreadContext* threadContext)
    {
        char asciiResourceName[64];
        sprintf_s(asciiResourceName, 64, "snap_%I64i.snp", snapId);

        TTDataIOInfo& iofp = threadContext->TTDContext->TTDataIOInfo;
        JsTTDStreamHandle snapHandle = iofp.pfOpenResourceStream(iofp.ActiveTTUriLength, iofp.ActiveTTUri, strlen(asciiResourceName), asciiResourceName, true, false);
        TTDAssert(snapHandle != nullptr, "Failed to open snapshot resource stream for reading.");

        TTD_SNAP_READER snapreader(snapHandle, iofp.pfReadBytesFromStream, iofp.pfFlushAndCloseStream);
        SnapShot* snap = SnapShot::ParseSnapshotFromFile(&snapreader);

        return snap;
    }

#if ENABLE_SNAPSHOT_COMPARE
    void SnapShot::InitializeForSnapshotCompare(const SnapShot* snap1, const SnapShot* snap2, TTDCompareMap& compareMap)
    {
        ////
        //Initialize all of the maps

        //top-level functions
        for(auto iter = snap1->m_ctxList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapContext* ctx = iter.Current();

            for(uint32 i = 0; i < ctx->LoadedTopLevelScriptCount; ++i)
            {
                compareMap.H1FunctionTopLevelLoadMap.AddNew(ctx->LoadedTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->LoadedTopLevelScriptArray[i].TopLevelBodyCtr);
            }

            for(uint32 i = 0; i < ctx->NewFunctionTopLevelScriptCount; ++i)
            {
                compareMap.H1FunctionTopLevelNewMap.AddNew(ctx->NewFunctionTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->NewFunctionTopLevelScriptArray[i].TopLevelBodyCtr);
            }

            for(uint32 i = 0; i < ctx->EvalTopLevelScriptCount; ++i)
            {
                compareMap.H1FunctionTopLevelEvalMap.AddNew(ctx->EvalTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->EvalTopLevelScriptArray[i].TopLevelBodyCtr);
            }
        }

        for(auto iter = snap2->m_ctxList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const NSSnapValues::SnapContext* ctx = iter.Current();

            for(uint32 i = 0; i < ctx->LoadedTopLevelScriptCount; ++i)
            {
                compareMap.H2FunctionTopLevelLoadMap.AddNew(ctx->LoadedTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->LoadedTopLevelScriptArray[i].TopLevelBodyCtr);
            }

            for(uint32 i = 0; i < ctx->NewFunctionTopLevelScriptCount; ++i)
            {
                compareMap.H2FunctionTopLevelNewMap.AddNew(ctx->NewFunctionTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->NewFunctionTopLevelScriptArray[i].TopLevelBodyCtr);
            }

            for(uint32 i = 0; i < ctx->EvalTopLevelScriptCount; ++i)
            {
                compareMap.H2FunctionTopLevelEvalMap.AddNew(ctx->EvalTopLevelScriptArray[i].ContextSpecificBodyPtrId, ctx->EvalTopLevelScriptArray[i].TopLevelBodyCtr);
            }
        }

        //Values and things
        for(auto iter = snap1->m_primitiveObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H1ValueMap.AddNew(iter.Current()->PrimitiveValueId, iter.Current());
        }

        for(auto iter = snap2->m_primitiveObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H2ValueMap.AddNew(iter.Current()->PrimitiveValueId, iter.Current());
        }

        for(auto iter = snap1->m_slotArrayEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H1SlotArrayMap.AddNew(iter.Current()->SlotId, iter.Current());
        }

        for(auto iter = snap2->m_slotArrayEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H2SlotArrayMap.AddNew(iter.Current()->SlotId, iter.Current());
        }

        for(auto iter = snap1->m_scopeEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H1FunctionScopeInfoMap.AddNew(iter.Current()->ScopeId, iter.Current());
        }

        for(auto iter = snap2->m_scopeEntries.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H2FunctionScopeInfoMap.AddNew(iter.Current()->ScopeId, iter.Current());
        }

        //Bodies and objects
        for(auto iter = snap1->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H1FunctionBodyMap.AddNew(iter.Current()->FunctionBodyId, iter.Current());
        }

        for(auto iter = snap2->m_functionBodyList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H2FunctionBodyMap.AddNew(iter.Current()->FunctionBodyId, iter.Current());
        }

        for(auto iter = snap1->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H1ObjectMap.AddNew(iter.Current()->ObjectPtrId, iter.Current());
        }

        for(auto iter = snap2->m_compoundObjectList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            compareMap.H2ObjectMap.AddNew(iter.Current()->ObjectPtrId, iter.Current());
        }
    }

    void SnapShot::DoSnapshotCompare(const SnapShot* snap1, const SnapShot* snap2, TTDCompareMap& compareMap)
    {
        //compare the roots to kick things off
        compareMap.DiagnosticAssert(snap1->m_rootList.Count() == snap2->m_rootList.Count());

        JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator> allRootMap1(&HeapAllocator::Instance);
        JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator> allRootMap2(&HeapAllocator::Instance);

        for(auto iter1 = snap1->m_rootList.GetIterator(); iter1.IsValid(); iter1.MoveNext())
        {
            NSSnapValues::SnapRootInfoEntry* rootEntry1 = iter1.Current();
            allRootMap1.AddNew(rootEntry1->LogId, rootEntry1);
        }

        for(auto iter2 = snap2->m_rootList.GetIterator(); iter2.IsValid(); iter2.MoveNext())
        {
            NSSnapValues::SnapRootInfoEntry* rootEntry2 = iter2.Current();
            allRootMap2.AddNew(rootEntry2->LogId, rootEntry2);

            bool ll1 = allRootMap1.Item(rootEntry2->LogId)->MaybeLongLivedRoot;
            compareMap.DiagnosticAssert(ll1 == rootEntry2->MaybeLongLivedRoot);

            TTD_PTR_ID id1 = allRootMap1.Item(rootEntry2->LogId)->LogObject;
            compareMap.CheckConsistentAndAddPtrIdMapping_Root(id1, rootEntry2->LogObject, rootEntry2->LogId);
        }

        //Get the script contexts into the mix
        compareMap.DiagnosticAssert(snap1->m_activeScriptContext == snap2->m_activeScriptContext);
        compareMap.DiagnosticAssert(snap1->m_ctxList.Count() == snap2->m_ctxList.Count());
        for(auto iter1 = snap1->m_ctxList.GetIterator(); iter1.IsValid(); iter1.MoveNext())
        {
            const NSSnapValues::SnapContext* ctx1 = iter1.Current();
            const NSSnapValues::SnapContext* ctx2 = nullptr;
            for(auto iter2 = snap2->m_ctxList.GetIterator(); iter2.IsValid(); iter2.MoveNext())
            {
                if(ctx1->ScriptContextLogId == iter2.Current()->ScriptContextLogId)
                {
                    ctx2 = iter2.Current();
                    break;
                }
            }
            compareMap.DiagnosticAssert(ctx2 != nullptr);

            NSSnapValues::AssertSnapEquiv(ctx1, ctx2, allRootMap1, allRootMap2, compareMap);
        }

        //compare the contents of the two thread context symbol maps
        compareMap.DiagnosticAssert(snap1->m_tcSymbolRegistrationMapContents.Count() == snap2->m_tcSymbolRegistrationMapContents.Count());
        for(auto iter1 = snap1->m_tcSymbolRegistrationMapContents.GetIterator(); iter1.IsValid(); iter1.MoveNext())
        {
            const Js::PropertyId pid1 = *iter1.Current();
            bool match = false;
            for(auto iter2 = snap2->m_tcSymbolRegistrationMapContents.GetIterator(); iter2.IsValid(); iter2.MoveNext())
            {
                if(*iter2.Current() == pid1)
                {
                    match = true;
                    break;
                }
            }
            compareMap.DiagnosticAssert(match);
        }

        //Iterate on the worklist until we are done
        TTDCompareTag ctag = TTDCompareTag::Done;
        TTD_PTR_ID ptrId1 = TTD_INVALID_PTR_ID;
        TTD_PTR_ID ptrId2 = TTD_INVALID_PTR_ID;

        uint32 comparedSlotArrays = 0;
        uint32 comparedScopes = 0;
        uint32 comparedObjects = 0;

        compareMap.GetNextCompareInfo(&ctag, &ptrId1, &ptrId2);
        while(ctag != TTDCompareTag::Done)
        {
            if(ctag == TTDCompareTag::SlotArray)
            {
                const NSSnapValues::SlotArrayInfo* sai1 = nullptr;
                const NSSnapValues::SlotArrayInfo* sai2 = nullptr;
                compareMap.GetCompareValues(ctag, ptrId1, &sai1, ptrId2, &sai2);
                NSSnapValues::AssertSnapEquiv(sai1, sai2, compareMap);

                comparedSlotArrays++;
            }
            else if(ctag == TTDCompareTag::FunctionScopeInfo)
            {
                const NSSnapValues::ScriptFunctionScopeInfo* scope1 = nullptr;
                const NSSnapValues::ScriptFunctionScopeInfo* scope2 = nullptr;
                compareMap.GetCompareValues(ctag, ptrId1, &scope1, ptrId2, &scope2);
                NSSnapValues::AssertSnapEquiv(scope1, scope2, compareMap);

                comparedScopes++;
            }
            else if(ctag == TTDCompareTag::TopLevelLoadFunction)
            {
                uint64 fload1 = 0;
                uint64 fload2 = 0;
                compareMap.GetCompareValues(ctag, ptrId1, &fload1, ptrId2, &fload2);
                compareMap.DiagnosticAssert(fload1 == fload2);
            }
            else if(ctag == TTDCompareTag::TopLevelNewFunction)
            {
                uint64 fnew1 = 0;
                uint64 fnew2 = 0;
                compareMap.GetCompareValues(ctag, ptrId1, &fnew1, ptrId2, &fnew2);
                compareMap.DiagnosticAssert(fnew1 == fnew2);
            }
            else if(ctag == TTDCompareTag::TopLevelEvalFunction)
            {
                uint64 feval1 = 0;
                uint64 feval2 = 0;
                compareMap.GetCompareValues(ctag, ptrId1, &feval1, ptrId2, &feval2);
                compareMap.DiagnosticAssert(feval1 == feval2);
            }
            else if(ctag == TTDCompareTag::FunctionBody)
            {
                const NSSnapValues::FunctionBodyResolveInfo* fb1 = nullptr;
                const NSSnapValues::FunctionBodyResolveInfo* fb2 = nullptr;
                compareMap.GetCompareValues(ctag, ptrId1, &fb1, ptrId2, &fb2);
                NSSnapValues::AssertSnapEquiv(fb1, fb2, compareMap);
            }
            else if(ctag == TTDCompareTag::SnapObject)
            {
                const NSSnapObjects::SnapObject* obj1 = nullptr;
                const NSSnapObjects::SnapObject* obj2 = nullptr;
                compareMap.GetCompareValues(ctag, ptrId1, &obj1, ptrId2, &obj2);
                NSSnapObjects::AssertSnapEquiv(obj1, obj2, compareMap);

                comparedObjects++;
            }
            else
            {
                TTDAssert(false, "Missing tag in case list!!!");
            }

            compareMap.GetNextCompareInfo(&ctag, &ptrId1, &ptrId2);
        }

        //Make sure all objects/values have been matched -- well not quite since we don't align and treaverse WeakSet values we might not compare everything -- maybe improve later?
        compareMap.DiagnosticAssert(comparedSlotArrays <= snap1->m_slotArrayEntries.Count() && comparedSlotArrays <= snap2->m_slotArrayEntries.Count());
        compareMap.DiagnosticAssert(comparedScopes <= snap1->m_scopeEntries.Count() && comparedScopes <= snap2->m_scopeEntries.Count());
        compareMap.DiagnosticAssert(comparedObjects <= snap1->m_compoundObjectList.Count() && comparedObjects <= snap2->m_compoundObjectList.Count());

        compareMap.DiagnosticAssert(snap1->m_slotArrayEntries.Count() == snap2->m_slotArrayEntries.Count());
        compareMap.DiagnosticAssert(snap1->m_scopeEntries.Count() == snap2->m_scopeEntries.Count());
        compareMap.DiagnosticAssert(snap1->m_compoundObjectList.Count() == snap2->m_compoundObjectList.Count());

        //
        //TODO: if we missed something we may want to put code here to identify it
        //
    }
#endif
}

#endif
