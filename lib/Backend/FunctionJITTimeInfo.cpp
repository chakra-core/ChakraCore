//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

FunctionJITTimeInfo::FunctionJITTimeInfo(FunctionJITTimeDataIDL * data) : m_data(*data)
{
    // we will cast the data (i.e. midl struct) pointers into info pointers so we can extend with methods
    CompileAssert(sizeof(FunctionJITTimeDataIDL) == sizeof(FunctionJITTimeInfo));
}

/* static */
FunctionJITTimeDataIDL *
FunctionJITTimeInfo::BuildJITTimeData(ArenaAllocator * alloc, const Js::FunctionCodeGenJitTimeData * codeGenData, bool isInlinee)
{
    // using arena because we can't recycler allocate (may be on background), and heap freeing this is slightly complicated
    FunctionJITTimeDataIDL * jitData = AnewStructZ(alloc, FunctionJITTimeDataIDL);
    jitData->bodyData = codeGenData->GetJITBody();
    jitData->functionInfoAddr = (intptr_t)codeGenData->GetFunctionInfo();

    jitData->localFuncId = codeGenData->GetFunctionInfo()->GetLocalFunctionId();
    jitData->isAggressiveInliningEnabled = codeGenData->GetIsAggressiveInliningEnabled();
    jitData->isInlined = codeGenData->GetIsInlined();
    jitData->weakFuncRef = (intptr_t)codeGenData->GetWeakFuncRef();

    jitData->inlineesBv = (BVFixedIDL*)codeGenData->inlineesBv;

    if (codeGenData->GetFunctionInfo()->HasBody())
    {
        Js::FunctionBody * functionBody = codeGenData->GetFunctionBody();
        if (functionBody->HasDynamicProfileInfo())
        {
            Assert(jitData->bodyData != nullptr);
            ProfileDataIDL * profileData = AnewStruct(alloc, ProfileDataIDL);
            JITTimeProfileInfo::InitializeJITProfileData(functionBody->GetAnyDynamicProfileInfo(), functionBody, profileData);

            jitData->bodyData->profileData = profileData;

            if (isInlinee)
            {
                // if not inlinee, NativeCodeGenerator will provide the address
                // REVIEW: OOP JIT, for inlinees, is this actually necessary?
                Js::ProxyEntryPointInfo *defaultEntryPointInfo = functionBody->GetDefaultEntryPointInfo();
                Assert(defaultEntryPointInfo->IsFunctionEntryPointInfo());
                Js::FunctionEntryPointInfo *functionEntryPointInfo = static_cast<Js::FunctionEntryPointInfo*>(defaultEntryPointInfo);
                jitData->callsCountAddress = (intptr_t)&functionEntryPointInfo->callsCount;

                auto sharedGuards = functionEntryPointInfo->GetSharedPropertyGuards();
                if (sharedGuards != nullptr)
                {
                    jitData->sharedPropGuardCount = sharedGuards->Count();
                    jitData->sharedPropertyGuards = AnewArray(alloc, Js::PropertyId, sharedGuards->Count());
                    auto sharedGuardIter = sharedGuards->GetIterator();
                    uint i = 0;
                    while (sharedGuardIter.IsValid())
                    {
                        jitData->sharedPropertyGuards[i] = sharedGuardIter.CurrentKey();
                        sharedGuardIter.MoveNext();
                        ++i;
                    }
                }
            }
        }
        if (jitData->bodyData->profiledCallSiteCount > 0)
        {
            jitData->inlineeCount = jitData->bodyData->profiledCallSiteCount;
            jitData->inlinees = AnewArrayZ(alloc, FunctionJITTimeDataIDL*, jitData->bodyData->profiledCallSiteCount);
            for (Js::ProfileId i = 0; i < jitData->bodyData->profiledCallSiteCount; ++i)
            {
                const Js::FunctionCodeGenJitTimeData * inlinee = codeGenData->GetInlinee(i);
                if (inlinee != nullptr)
                {
                    jitData->inlinees[i] = BuildJITTimeData(alloc, inlinee);
                }
            }
        }
        if (jitData->bodyData->inlineCacheCount > 0)
        {
            jitData->ldFldInlineeCount = jitData->bodyData->inlineCacheCount;
            jitData->ldFldInlinees = AnewArrayZ(alloc, FunctionJITTimeDataIDL*, jitData->bodyData->inlineCacheCount);

            Js::ObjTypeSpecFldInfo ** objTypeSpecInfo = codeGenData->GetObjTypeSpecFldInfoArray() ? codeGenData->GetObjTypeSpecFldInfoArray()->GetInfoArray() : nullptr;
            if(objTypeSpecInfo)
            {
                jitData->objTypeSpecFldInfoCount = jitData->bodyData->inlineCacheCount;
                jitData->objTypeSpecFldInfoArray = AnewArrayZ(alloc, ObjTypeSpecFldIDL*, jitData->bodyData->inlineCacheCount);
            }
            for (Js::InlineCacheIndex i = 0; i < jitData->bodyData->inlineCacheCount; ++i)
            {
                const Js::FunctionCodeGenJitTimeData * inlinee = codeGenData->GetLdFldInlinee(i);
                if (inlinee != nullptr)
                {
                    jitData->ldFldInlinees[i] = BuildJITTimeData(alloc, inlinee);
                }
                if (objTypeSpecInfo != nullptr && objTypeSpecInfo[i] != nullptr)
                {
                    jitData->objTypeSpecFldInfoArray[i] = AnewStructZ(alloc, ObjTypeSpecFldIDL);
                    if (objTypeSpecInfo[i]->IsLoadedFromProto())
                    {
                        jitData->objTypeSpecFldInfoArray[i]->protoObjectAddr = (intptr_t)objTypeSpecInfo[i]->GetProtoObject();
                    }
                    jitData->objTypeSpecFldInfoArray[i]->propertyGuardValueAddr = (intptr_t)objTypeSpecInfo[i]->GetPropertyGuard()->GetAddressOfValue();
                    jitData->objTypeSpecFldInfoArray[i]->propertyId = objTypeSpecInfo[i]->GetPropertyId();
                    jitData->objTypeSpecFldInfoArray[i]->typeId = objTypeSpecInfo[i]->GetTypeId();
                    jitData->objTypeSpecFldInfoArray[i]->id = objTypeSpecInfo[i]->GetObjTypeSpecFldId();
                    jitData->objTypeSpecFldInfoArray[i]->flags = objTypeSpecInfo[i]->GetFlags();
                    jitData->objTypeSpecFldInfoArray[i]->slotIndex = objTypeSpecInfo[i]->GetSlotIndex();
                    jitData->objTypeSpecFldInfoArray[i]->fixedFieldCount = objTypeSpecInfo[i]->GetFixedFieldCount();

                    if (objTypeSpecInfo[i]->HasInitialType())
                    {
                        jitData->objTypeSpecFldInfoArray[i]->initialType = AnewStructZ(alloc, TypeIDL);
                        JITType::BuildFromJsType(objTypeSpecInfo[i]->GetInitialType(), (JITType*)jitData->objTypeSpecFldInfoArray[i]->initialType);
                    }

                    if (objTypeSpecInfo[i]->GetCtorCache() != nullptr)
                    {
                        jitData->objTypeSpecFldInfoArray[i]->ctorCache = objTypeSpecInfo[i]->GetCtorCache()->GetData();
                    }

                    Js::EquivalentTypeSet * equivTypeSet = objTypeSpecInfo[i]->GetEquivalentTypeSet();
                    if (equivTypeSet != nullptr)
                    {
                        jitData->objTypeSpecFldInfoArray[i]->typeSet = AnewStruct(alloc, EquivalentTypeSetIDL);
                        jitData->objTypeSpecFldInfoArray[i]->typeSet->sortedAndDuplicatesRemoved = equivTypeSet->GetSortedAndDuplicatesRemoved();
                        // REVIEW: OOP JIT, is this stable?
                        jitData->objTypeSpecFldInfoArray[i]->typeSet->count = equivTypeSet->GetCount();
                        jitData->objTypeSpecFldInfoArray[i]->typeSet->types = (TypeIDL**)equivTypeSet->GetTypes();
                    }

                    jitData->objTypeSpecFldInfoArray[i]->fixedFieldInfoArray = AnewArrayZ(alloc, FixedFieldIDL, objTypeSpecInfo[i]->GetFixedFieldCount());
                    Js::FixedFieldInfo * ffInfo = objTypeSpecInfo[i]->GetFixedFieldInfoArray();
                    for (uint16 j = 0; j < objTypeSpecInfo[i]->GetFixedFieldCount(); ++j)
                    {
                        jitData->objTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].fieldValue = (intptr_t)ffInfo[j].fieldValue;
                        jitData->objTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].nextHasSameFixedField = ffInfo[j].nextHasSameFixedField;
                        if(ffInfo[j].type != nullptr)
                        {
                            JITType::BuildFromJsType(ffInfo[j].type, (JITType*)&jitData->objTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].type);
                        }
                    }
                }
            }
        }
        if (codeGenData->GetGlobalObjTypeSpecFldInfoCount() > 0)
        {
            jitData->globalObjTypeSpecFldInfoCount = codeGenData->GetGlobalObjTypeSpecFldInfoCount();

            Js::ObjTypeSpecFldInfo ** globObjTypeSpecInfo = codeGenData->GetGlobalObjTypeSpecFldInfoArray();
            Assert(globObjTypeSpecInfo != nullptr);
            jitData->globalObjTypeSpecFldInfoArray = AnewArrayZ(alloc, ObjTypeSpecFldIDL*, jitData->globalObjTypeSpecFldInfoCount);
            for (uint i = 0; i < codeGenData->GetGlobalObjTypeSpecFldInfoCount(); ++i)
            {
                if (globObjTypeSpecInfo[i] == nullptr)
                {
                    continue;
                }
                jitData->globalObjTypeSpecFldInfoArray[i] = AnewStructZ(alloc, ObjTypeSpecFldIDL);
                if (globObjTypeSpecInfo[i]->IsLoadedFromProto())
                {
                    jitData->globalObjTypeSpecFldInfoArray[i]->protoObjectAddr = (intptr_t)globObjTypeSpecInfo[i]->GetProtoObject();
                }
                jitData->globalObjTypeSpecFldInfoArray[i]->propertyGuardValueAddr = (intptr_t)globObjTypeSpecInfo[i]->GetPropertyGuard()->GetAddressOfValue();
                jitData->globalObjTypeSpecFldInfoArray[i]->propertyId = globObjTypeSpecInfo[i]->GetPropertyId();
                jitData->globalObjTypeSpecFldInfoArray[i]->typeId = globObjTypeSpecInfo[i]->GetTypeId();
                jitData->globalObjTypeSpecFldInfoArray[i]->id = globObjTypeSpecInfo[i]->GetObjTypeSpecFldId();
                jitData->globalObjTypeSpecFldInfoArray[i]->flags = globObjTypeSpecInfo[i]->GetFlags();
                jitData->globalObjTypeSpecFldInfoArray[i]->slotIndex = globObjTypeSpecInfo[i]->GetSlotIndex();
                jitData->globalObjTypeSpecFldInfoArray[i]->fixedFieldCount = globObjTypeSpecInfo[i]->GetFixedFieldCount();

                if (globObjTypeSpecInfo[i]->HasInitialType())
                {
                    jitData->globalObjTypeSpecFldInfoArray[i]->initialType = AnewStructZ(alloc, TypeIDL);
                    JITType::BuildFromJsType(globObjTypeSpecInfo[i]->GetInitialType(), (JITType*)jitData->globalObjTypeSpecFldInfoArray[i]->initialType);
                }

                if (globObjTypeSpecInfo[i]->GetCtorCache() != nullptr)
                {
                    jitData->globalObjTypeSpecFldInfoArray[i]->ctorCache = globObjTypeSpecInfo[i]->GetCtorCache()->GetData();
                }

                Js::EquivalentTypeSet * equivTypeSet = globObjTypeSpecInfo[i]->GetEquivalentTypeSet();
                if (equivTypeSet != nullptr)
                {
                    jitData->globalObjTypeSpecFldInfoArray[i]->typeSet = AnewStruct(alloc, EquivalentTypeSetIDL);
                    jitData->globalObjTypeSpecFldInfoArray[i]->typeSet->sortedAndDuplicatesRemoved = equivTypeSet->GetSortedAndDuplicatesRemoved();
                    // REVIEW: OOP JIT, is this stable?
                    jitData->globalObjTypeSpecFldInfoArray[i]->typeSet->count = equivTypeSet->GetCount();
                    jitData->globalObjTypeSpecFldInfoArray[i]->typeSet->types = (TypeIDL**)equivTypeSet->GetTypes();
                }

                jitData->globalObjTypeSpecFldInfoArray[i]->fixedFieldInfoArray = AnewArrayZ(alloc, FixedFieldIDL, globObjTypeSpecInfo[i]->GetFixedFieldCount());
                Js::FixedFieldInfo * ffInfo = globObjTypeSpecInfo[i]->GetFixedFieldInfoArray();
                for (uint16 j = 0; j< globObjTypeSpecInfo[i]->GetFixedFieldCount(); ++j)
                {
                    jitData->globalObjTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].fieldValue = (intptr_t)ffInfo[j].fieldValue;
                    jitData->globalObjTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].nextHasSameFixedField = ffInfo[j].nextHasSameFixedField;
                    if (ffInfo[j].type != nullptr)
                    {
                        // TODO: OOP JIT, maybe type should be out of line? might not save anything on x64 though
                        JITType::BuildFromJsType(ffInfo[j].type, (JITType*)&jitData->globalObjTypeSpecFldInfoArray[i]->fixedFieldInfoArray[j].type);
                    }
                }
            }


        }

        auto codegenRuntimeData = functionBody->GetCodeGenRuntimeData();
        if (codegenRuntimeData)
        {
            jitData->bodyData->runtimeDataCount = jitData->bodyData->profiledCallSiteCount;
            Assert(jitData->bodyData->profiledCallSiteCount > 0);
            jitData->bodyData->profiledRuntimeData = AnewArrayZ(alloc, FunctionJITRuntimeIDL, jitData->bodyData->runtimeDataCount);
            // REVIEW: OOP JIT is this safe to be doing in background? I'm guessing probably not...
            for (uint i = 0; i < jitData->bodyData->runtimeDataCount; ++i)
            {
                if (codegenRuntimeData[i] && codegenRuntimeData[i]->ClonedInlineCaches()->HasInlineCaches())
                {
                    jitData->bodyData->profiledRuntimeData[i].clonedCacheCount = jitData->bodyData->inlineCacheCount;
                    for (uint j = 0; j < jitData->bodyData->inlineCacheCount; ++j)
                    {
                        // REVIEW: OOP JIT, what to do with WriteBarrierPtr?
                        jitData->bodyData->profiledRuntimeData[i].clonedInlineCaches = (intptr_t*)codegenRuntimeData[i]->ClonedInlineCaches()->GetInlineCache(j);
                    }
                }
            }
        }
    }
    if (codeGenData->GetNext() != nullptr)
    {
        jitData->next = BuildJITTimeData(alloc, codeGenData->GetNext());
    }
    return jitData;
}

uint
FunctionJITTimeInfo::GetInlineeCount() const
{
    return m_data.inlineeCount;
}

bool
FunctionJITTimeInfo::IsLdFldInlineePresent() const
{
    return m_data.ldFldInlineeCount != 0;
}

bool
FunctionJITTimeInfo::HasSharedPropertyGuards() const
{
    return m_data.sharedPropGuardCount != 0;
}

bool
FunctionJITTimeInfo::HasSharedPropertyGuard(Js::PropertyId id) const
{
    for (uint i = 0; i < m_data.sharedPropGuardCount; ++i)
    {
        if (m_data.sharedPropertyGuards[i] == id)
        {
            return true;
        }
    }
    return false;
}

intptr_t
FunctionJITTimeInfo::GetFunctionInfoAddr() const
{
    return m_data.functionInfoAddr;
}

intptr_t
FunctionJITTimeInfo::GetWeakFuncRef() const
{
    return m_data.weakFuncRef;
}

uint
FunctionJITTimeInfo::GetLocalFunctionId() const
{
    return m_data.localFuncId;
}

bool
FunctionJITTimeInfo::IsAggressiveInliningEnabled() const
{
    return m_data.isAggressiveInliningEnabled != FALSE;
}

bool
FunctionJITTimeInfo::IsInlined() const
{
    return m_data.isInlined != FALSE;
}

const BVFixed *
FunctionJITTimeInfo::GetInlineesBV() const
{
    return reinterpret_cast<const BVFixed *>(m_data.inlineesBv);
}

const FunctionJITTimeInfo *
FunctionJITTimeInfo::GetJitTimeDataFromFunctionInfoAddr(intptr_t polyFuncInfo) const
{
    const FunctionJITTimeInfo *next = this;
    while (next && next->GetFunctionInfoAddr() != polyFuncInfo)
    {
        next = next->GetNext();
    }
    return next;
}

JITObjTypeSpecFldInfo *
FunctionJITTimeInfo::GetObjTypeSpecFldInfo(uint index) const
{
    if (m_data.objTypeSpecFldInfoArray == nullptr) 
    {
        return nullptr;
    }

    Assert(index < GetBody()->GetInlineCacheCount());
    Assert(index < m_data.objTypeSpecFldInfoCount);
    return reinterpret_cast<JITObjTypeSpecFldInfo **>(m_data.objTypeSpecFldInfoArray)[index];
}

JITObjTypeSpecFldInfo *
FunctionJITTimeInfo::GetGlobalObjTypeSpecFldInfo(uint index) const
{
    Assert(index < m_data.globalObjTypeSpecFldInfoCount);
    return reinterpret_cast<JITObjTypeSpecFldInfo **>(m_data.globalObjTypeSpecFldInfoArray)[index];
}


uint
FunctionJITTimeInfo::GetSourceContextId() const
{
    Assert(HasBody());

    return GetBody()->GetSourceContextId();
}

const FunctionJITTimeInfo *
FunctionJITTimeInfo::GetLdFldInlinee(Js::InlineCacheIndex inlineCacheIndex) const
{
    Assert(inlineCacheIndex < m_data.bodyData->inlineCacheCount);
    if (!m_data.ldFldInlinees)
    {
        return nullptr;
    }
    Assert(inlineCacheIndex < m_data.ldFldInlineeCount);

    return reinterpret_cast<const FunctionJITTimeInfo *>(m_data.ldFldInlinees[inlineCacheIndex]);
}

const FunctionJITTimeInfo *
FunctionJITTimeInfo::GetInlinee(Js::ProfileId profileId) const
{
    Assert(profileId < m_data.bodyData->profiledCallSiteCount);
    if (!m_data.inlinees)
    {
        return nullptr;
    }
    Assert(profileId < m_data.inlineeCount);

    return reinterpret_cast<const FunctionJITTimeInfo *>(m_data.inlinees[profileId]);
}

const FunctionJITTimeInfo *
FunctionJITTimeInfo::GetNext() const
{
    return reinterpret_cast<const FunctionJITTimeInfo *>(m_data.next);
}

JITTimeFunctionBody *
FunctionJITTimeInfo::GetBody() const
{
    return reinterpret_cast<JITTimeFunctionBody *>(m_data.bodyData);
}

bool
FunctionJITTimeInfo::HasBody() const
{
    return m_data.bodyData != nullptr;
}

bool
FunctionJITTimeInfo::IsPolymorphicCallSite(Js::ProfileId profiledCallSiteId) const
{
    Assert(profiledCallSiteId < m_data.bodyData->profiledCallSiteCount);

    if (!m_data.inlinees)
    {
        return false;
    }
    Assert(profiledCallSiteId < m_data.inlineeCount);

    return m_data.inlinees[profiledCallSiteId]->next != nullptr;
}

bool
FunctionJITTimeInfo::ForceJITLoopBody() const
{
    return
        !PHASE_OFF(Js::JITLoopBodyPhase, this) &&
        !PHASE_OFF(Js::FullJitPhase, this) &&
        !GetBody()->IsGenerator() &&
        !GetBody()->HasTry() &&
        (
            PHASE_FORCE(Js::JITLoopBodyPhase, this)
#ifdef ENABLE_PREJIT
            || Js::Configuration::Global.flags.Prejit
#endif
            );
}


wchar_t*
FunctionJITTimeInfo::GetDisplayName() const
{
    return GetBody()->GetDisplayName();
}

wchar_t*
FunctionJITTimeInfo::GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const
{
    // (#%u.%u), #%u --> (source file Id . function Id) , function Number
    int len = swprintf_s(bufferToWriteTo, MAX_FUNCTION_BODY_DEBUG_STRING_SIZE, L" (#%d.%u), #%u",
        (int)GetSourceContextId(), GetLocalFunctionId(), GetBody()->GetFunctionNumber());
    Assert(len > 8);
    return bufferToWriteTo;
}
