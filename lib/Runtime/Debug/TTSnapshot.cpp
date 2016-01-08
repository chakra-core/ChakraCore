//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    void SnapShot::EmitSnapshotToFile(FileWriter* writer, IOStreamFunctions& streamFunctions, LPCWSTR srcDir, ThreadContext* threadContext) const
    {
        Js::HiResTimer timer;
        double startWrite = timer.Now();

        writer->WriteRecordStart();
        writer->AdjustIndent(1);

        uint64 usedSpace = 0;
        uint64 reservedSpace = 0;
        this->ComputeSnapshotMemory(&usedSpace, &reservedSpace);

        writer->WriteDouble(NSTokens::Key::timeTotal, this->MarkTime + this->ExtractTime);
        writer->WriteUInt64(NSTokens::Key::snapUsedMemory, usedSpace, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt64(NSTokens::Key::snapReservedMemory, reservedSpace, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::timeMark, this->MarkTime, NSTokens::Separator::CommaSeparator);
        writer->WriteDouble(NSTokens::Key::timeExtract, this->ExtractTime, NSTokens::Separator::CommaSeparator);

        writer->WriteLengthValue(this->m_ctxList.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
        writer->AdjustIndent(1);
        bool firstCtx = true;
        for(auto iter = this->m_ctxList.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSSnapValues::EmitSnapContext(iter.Current(), srcDir, streamFunctions, writer, firstCtx ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);

            firstCtx = false;
        }
        writer->AdjustIndent(-1);
        writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);

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
            NSSnapValues::EmitFunctionBodyInfo(iter.Current(), srcDir, streamFunctions, writer, firstBody ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);

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

    SnapShot* SnapShot::ParseSnapshotFromFile(FileReader* reader, IOStreamFunctions& streamFunctions, LPCWSTR srcDir)
    {
        reader->ReadRecordStart();

        reader->ReadDouble(NSTokens::Key::timeTotal);
        reader->ReadUInt64(NSTokens::Key::snapUsedMemory, true);
        reader->ReadUInt64(NSTokens::Key::snapReservedMemory, true);
        reader->ReadDouble(NSTokens::Key::timeMark, true);
        reader->ReadDouble(NSTokens::Key::timeExtract, true);

        SnapShot* snap = HeapNew(SnapShot);

        uint32 ctxCount = reader->ReadLengthValue(true);
        reader->ReadSequenceStart_WDefaultKey(true);
        for(uint32 i = 0; i < ctxCount; ++i)
        {
            NSSnapValues::SnapContext* snpCtx = snap->m_ctxList.NextOpenEntry();
            NSSnapValues::ParseSnapContext(snpCtx, i != 0, srcDir, streamFunctions, reader, snap->GetSnapshotSlabAllocator());
        }
        reader->ReadSequenceEnd();

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
            NSSnapValues::ParseFunctionBodyInfo(into, i != 0, srcDir, streamFunctions, reader, snap->GetSnapshotSlabAllocator());
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
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
            res = ctx->LookupGeneralObjectForKnownToken_TTD(snpObject->OptWellKnownToken);
        }
        else
        {
            //lookup the inflator function for this object and call it
            NSSnapObjects::fPtr_DoObjectInflation inflateFPtr = this->m_snapObjectVTableArray[(uint32)snpObject->SnapObjectTag].InflationFunc;
            AssertMsg(inflateFPtr != nullptr, "We probably forgot to update the vtable with a tag we added.");

            res = inflateFPtr(snpObject, inflator);
        }

        Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextTag);
        ctx->GetThreadContext()->TTDInfo->SetObjectTrackingTagSnapAndInflate_TTD(snpObject->ObjectLogTag, snpObject->ObjectIdentityTag, res);

        inflator->AddObject(snpObject->ObjectPtrId, res);
    }

    void SnapShot::ComputeSnapshotMemory(uint64* usedSpace, uint64* reservedSpace) const
    {
        return this->m_slabAllocator.ComputeMemoryUsed(usedSpace, reservedSpace);
    }

    SnapShot::SnapShot()
        : m_slabAllocator(),
        m_ctxList(&this->m_slabAllocator), m_handlerList(&this->m_slabAllocator), m_typeList(&this->m_slabAllocator),
        m_functionBodyList(&this->m_slabAllocator), m_primitiveObjectList(&this->m_slabAllocator), m_compoundObjectList(&this->m_slabAllocator),
        m_scopeEntries(&this->m_slabAllocator), m_slotArrayEntries(&this->m_slabAllocator),
        m_snapObjectVTableArray(nullptr),
        MarkTime(0.0), ExtractTime(0.0)
    {
        this->m_snapObjectVTableArray = this->m_slabAllocator.SlabAllocateArray<NSSnapObjects::SnapObjectVTable>((uint32)NSSnapObjects::SnapObjectType::Limit);
        memset(this->m_snapObjectVTableArray, 0, sizeof(NSSnapObjects::SnapObjectVTable) * (uint32)NSSnapObjects::SnapObjectType::Limit);

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::Invalid] = { nullptr, nullptr, nullptr, nullptr };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapUnhandledObject] = { nullptr, nullptr, nullptr, nullptr };

        ////
        //For the objects that have inflators

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapDynamicObject] = { &NSSnapObjects::DoObjectInflation_SnapDynamicObject, nullptr, nullptr, nullptr };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapScriptFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapScriptFunctionInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapScriptFunctionInfo, &NSSnapObjects::EmitAddtlInfo_SnapScriptFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapScriptFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapRuntimeFunctionObject] = { nullptr, nullptr, nullptr, nullptr }; //should always be wellknown objects and the extra state is in the functionbody defs
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapExternalFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapExternalFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapExternalFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapExternalFunctionInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapBoundFunctionObject] = { &NSSnapObjects::DoObjectInflation_SnapBoundFunctionInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapBoundFunctionInfo, &NSSnapObjects::ParseAddtlInfo_SnapBoundFunctionInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapBoxedValueObject] = { &NSSnapObjects::DoObjectInflation_SnapBoxedValue, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapBoxedValue, &NSSnapObjects::ParseAddtlInfo_SnapBoxedValue };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapDateObject] = { &NSSnapObjects::DoObjectInflation_SnapDate, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapDate, &NSSnapObjects::ParseAddtlInfo_SnapDate };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapRegexObject] = { &NSSnapObjects::DoObjectInflation_SnapRegexInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapRegexInfo, &NSSnapObjects::ParseAddtlInfo_SnapRegexInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapErrorObject] = { &NSSnapObjects::DoObjectInflation_SnapError, nullptr, nullptr, nullptr };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<TTDVar, Js::Var, NSSnapObjects::SnapObjectType::SnapArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<TTDVar, NSSnapObjects::SnapObjectType::SnapArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<TTDVar, NSSnapObjects::SnapObjectType::SnapArrayObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<int32, int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<int32, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject> };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapArrayInfo<double, double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>, &NSSnapObjects::EmitAddtlInfo_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>, &NSSnapObjects::ParseAddtlInfo_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject> };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapArrayBufferObject] = { &NSSnapObjects::DoObjectInflation_SnapArrayBufferInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapArrayBufferInfo, &NSSnapObjects::ParseAddtlInfo_SnapArrayBufferInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapTypedArrayObject] = { &NSSnapObjects::DoObjectInflation_SnapTypedArrayInfo, nullptr, &NSSnapObjects::EmitAddtlInfo_SnapTypedArrayInfo, &NSSnapObjects::ParseAddtlInfo_SnapTypedArrayInfo };

        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapSetObject] = { &NSSnapObjects::DoObjectInflation_SnapSetInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapSetInfo, &NSSnapObjects::EmitAddtlInfo_SnapSetInfo, &NSSnapObjects::ParseAddtlInfo_SnapSetInfo };
        this->m_snapObjectVTableArray[(uint32)NSSnapObjects::SnapObjectType::SnapMapObject] = { &NSSnapObjects::DoObjectInflation_SnapMapInfo, &NSSnapObjects::DoAddtlValueInstantiation_SnapMapInfo, &NSSnapObjects::EmitAddtlInfo_SnapMapInfo, &NSSnapObjects::ParseAddtlInfo_SnapMapInfo };

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

    UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_SMALL>& SnapShot::GetContextList()
    {
        return this->m_ctxList;
    }

    const UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_SMALL>& SnapShot::GetContextList() const
    {
        return this->m_ctxList;
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

    void SnapShot::Inflate(InflateMap* inflator, const NSSnapValues::SnapContext* sCtx) const
    {
        //We assume the caller has inflated all of the ScriptContexts for us and we are just filling in the objects

        //
        //TODO: fill in the typehandler and type inflate mappings here
        //

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

        Js::ScriptContext* tCtx = inflator->LookupScriptContext(sCtx->m_scriptContextTagId);
        NSSnapValues::ReLinkRoots(sCtx, tCtx, inflator);
    }

    void SnapShot::EmitSnapshot(LPCWSTR sourceDir, DWORD snapId, ThreadContext* threadContext, bool json, bool binary) const
    {
        wchar* snapIdString = HeapNewArrayZ(wchar, 64);
        swprintf_s(snapIdString, 64, L"%u", snapId);

        wchar* snapContainerURI = nullptr;
        HANDLE snapHandle = threadContext->TTDStreamFunctions.pfGetSnapshotStream(sourceDir, snapIdString, false, true, &snapContainerURI);

        if(json)
        {
            JSONWriter snapwriter(snapHandle, threadContext->TTDStreamFunctions.pfWriteBytesToStream, threadContext->TTDStreamFunctions.pfFlushAndCloseStream);

            this->EmitSnapshotToFile(&snapwriter, threadContext->TTDStreamFunctions, snapContainerURI, threadContext);
            snapwriter.FlushAndClose();
        }

        if(binary)
        {
            AssertMsg(false, "Not Implemented Yet!!!");
        }

        CoTaskMemFree(snapContainerURI);
        HeapDeleteArray(64, snapIdString);
    }

    SnapShot* SnapShot::Parse(LPCWSTR sourceDir, DWORD snapId, ThreadContext* threadContext, bool json, bool binary)
    {
        wchar* snapIdString = HeapNewArrayZ(wchar, 64);
        swprintf_s(snapIdString, 64, L"%u", snapId);

        wchar* snapContainerURI = nullptr;
        HANDLE snapHandle = threadContext->TTDStreamFunctions.pfGetSnapshotStream(sourceDir, snapIdString, true, false, &snapContainerURI);

        JSONReader snapreader(snapHandle, threadContext->TTDStreamFunctions.pfReadBytesFromStream, threadContext->TTDStreamFunctions.pfFlushAndCloseStream);
        SnapShot* snap = SnapShot::ParseSnapshotFromFile(&snapreader, threadContext->TTDStreamFunctions, snapContainerURI);

        CoTaskMemFree(snapContainerURI);
        HeapDeleteArray(64, snapIdString);

        return snap;
    }
}

#endif
