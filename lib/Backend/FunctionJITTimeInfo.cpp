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
void
FunctionJITTimeInfo::BuildJITTimeData(
    __in ArenaAllocator * alloc,
    __in const Js::FunctionCodeGenJitTimeData * codeGenData,
    __in_opt const Js::FunctionCodeGenRuntimeData * runtimeData,
    __out FunctionJITTimeDataIDL * jitData,
    bool isInlinee,
    bool isForegroundJIT)
{
    jitData->functionInfoAddr = (intptr_t)codeGenData->GetFunctionInfo();
    jitData->localFuncId = codeGenData->GetFunctionInfo()->GetLocalFunctionId();
    jitData->isAggressiveInliningEnabled = codeGenData->GetIsAggressiveInliningEnabled();
    jitData->isInlined = codeGenData->GetIsInlined();
    jitData->weakFuncRef = (intptr_t)codeGenData->GetWeakFuncRef();
    jitData->inlineesBv = (BVFixedIDL*)(const BVFixed*)codeGenData->inlineesBv;

    if (!codeGenData->GetFunctionBody() || !codeGenData->GetFunctionBody()->GetByteCode())
    {
        // outermost function must have a body, but inlinees may not (if they are builtins)
        Assert(isInlinee);
        return;
    }

    Js::FunctionBody * body = codeGenData->GetFunctionInfo()->GetParseableFunctionInfo()->GetFunctionBody();
    jitData->bodyData = AnewStructZ(alloc, FunctionBodyDataIDL);
    JITTimeFunctionBody::InitializeJITFunctionData(alloc, body, jitData->bodyData);

    Assert(isInlinee == !!runtimeData);
    const Js::FunctionCodeGenRuntimeData * targetRuntimeData = nullptr;
    if (runtimeData)
    {
        // may be polymorphic, so seek the runtime data matching our JIT time data
        targetRuntimeData = runtimeData->GetForTarget(codeGenData->GetFunctionInfo()->GetFunctionBody());
    }
    Js::FunctionBody * functionBody = codeGenData->GetFunctionBody();
    if (functionBody->HasDynamicProfileInfo())
    {
        ProfileDataIDL * profileData = AnewStruct(alloc, ProfileDataIDL);
        JITTimeProfileInfo::InitializeJITProfileData(alloc, functionBody->GetAnyDynamicProfileInfo(), functionBody, profileData, isForegroundJIT);

        jitData->bodyData->profileData = profileData;

        if (isInlinee)
        {
            // if not inlinee, NativeCodeGenerator will provide the address
            // REVIEW: OOP JIT, for inlinees, is this actually necessary?
            Js::ProxyEntryPointInfo *defaultEntryPointInfo = functionBody->GetDefaultEntryPointInfo();
            Assert(defaultEntryPointInfo->IsFunctionEntryPointInfo());
            Js::FunctionEntryPointInfo *functionEntryPointInfo = static_cast<Js::FunctionEntryPointInfo*>(defaultEntryPointInfo);
            jitData->callsCountAddress = (intptr_t)&functionEntryPointInfo->callsCount;
                
            jitData->sharedPropertyGuards = codeGenData->sharedPropertyGuards;
            jitData->sharedPropGuardCount = codeGenData->sharedPropertyGuardCount;
        }
    }
    if (jitData->bodyData->profiledCallSiteCount > 0)
    {
        jitData->inlineeCount = jitData->bodyData->profiledCallSiteCount;
        // using arena because we can't recycler allocate (may be on background), and heap freeing this is slightly complicated
        jitData->inlinees = AnewArrayZ(alloc, FunctionJITTimeDataIDL*, jitData->bodyData->profiledCallSiteCount);
        jitData->inlineesRecursionFlags = AnewArrayZ(alloc, boolean, jitData->bodyData->profiledCallSiteCount);

        for (Js::ProfileId i = 0; i < jitData->bodyData->profiledCallSiteCount; ++i)
        {
            const Js::FunctionCodeGenJitTimeData * inlineeJITData = codeGenData->GetInlinee(i);
            if (inlineeJITData == codeGenData)
            {
                jitData->inlineesRecursionFlags[i] = TRUE;
            }
            else if (inlineeJITData != nullptr)
            {
                const Js::FunctionCodeGenRuntimeData * inlineeRuntimeData = nullptr;
                if (inlineeJITData->GetFunctionInfo()->HasBody())
                {
                    inlineeRuntimeData = isInlinee ? targetRuntimeData->GetInlinee(i) : functionBody->GetInlineeCodeGenRuntimeData(i);
                }
                jitData->inlinees[i] = AnewStructZ(alloc, FunctionJITTimeDataIDL);
                BuildJITTimeData(alloc, inlineeJITData, inlineeRuntimeData, jitData->inlinees[i], true, isForegroundJIT);
            }
        }
    }
    jitData->profiledRuntimeData = AnewStructZ(alloc, FunctionJITRuntimeIDL);
    if (isInlinee && targetRuntimeData->ClonedInlineCaches()->HasInlineCaches())
    {
        jitData->profiledRuntimeData->clonedCacheCount = jitData->bodyData->inlineCacheCount;
        jitData->profiledRuntimeData->clonedInlineCaches = AnewArray(alloc, intptr_t, jitData->profiledRuntimeData->clonedCacheCount);
        for (uint j = 0; j < jitData->bodyData->inlineCacheCount; ++j)
        {
            jitData->profiledRuntimeData->clonedInlineCaches[j] = (intptr_t)targetRuntimeData->ClonedInlineCaches()->GetInlineCache(j);
        }
    }
    if (jitData->bodyData->inlineCacheCount > 0)
    {
        jitData->ldFldInlineeCount = jitData->bodyData->inlineCacheCount;
        jitData->ldFldInlinees = AnewArrayZ(alloc, FunctionJITTimeDataIDL*, jitData->bodyData->inlineCacheCount);

        Field(ObjTypeSpecFldInfo*)* objTypeSpecInfo = codeGenData->GetObjTypeSpecFldInfoArray()->GetInfoArray();
        if(objTypeSpecInfo)
        {
            jitData->objTypeSpecFldInfoCount = jitData->bodyData->inlineCacheCount;
            jitData->objTypeSpecFldInfoArray = (ObjTypeSpecFldIDL**)objTypeSpecInfo;
        }
        for (Js::InlineCacheIndex i = 0; i < jitData->bodyData->inlineCacheCount; ++i)
        {
            const Js::FunctionCodeGenJitTimeData * inlineeJITData = codeGenData->GetLdFldInlinee(i);
            const Js::FunctionCodeGenRuntimeData * inlineeRuntimeData = isInlinee ? targetRuntimeData->GetLdFldInlinee(i) : functionBody->GetLdFldInlineeCodeGenRuntimeData(i);
            if (inlineeJITData != nullptr)
            {
                jitData->ldFldInlinees[i] = AnewStructZ(alloc, FunctionJITTimeDataIDL);
                BuildJITTimeData(alloc, inlineeJITData, inlineeRuntimeData, jitData->ldFldInlinees[i], true, isForegroundJIT);
            }
        }
    }
    if (!isInlinee && codeGenData->GetGlobalObjTypeSpecFldInfoCount() > 0)
    {
        Field(ObjTypeSpecFldInfo*)* globObjTypeSpecInfo = codeGenData->GetGlobalObjTypeSpecFldInfoArray();
        Assert(globObjTypeSpecInfo != nullptr);

        jitData->globalObjTypeSpecFldInfoCount = codeGenData->GetGlobalObjTypeSpecFldInfoCount();
        jitData->globalObjTypeSpecFldInfoArray = (ObjTypeSpecFldIDL**)globObjTypeSpecInfo;
    }
    const Js::FunctionCodeGenJitTimeData * nextJITData = codeGenData->GetNext();
    if (nextJITData != nullptr)
    {
        // only inlinee should be polymorphic
        Assert(isInlinee);
        jitData->next = AnewStructZ(alloc, FunctionJITTimeDataIDL);
        BuildJITTimeData(alloc, nextJITData, runtimeData, jitData->next, true, isForegroundJIT);
    }
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

const FunctionJITRuntimeInfo *
FunctionJITTimeInfo::GetInlineeForTargetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId, intptr_t inlineeFuncBodyAddr) const
{
    const FunctionJITTimeInfo *inlineeData = GetInlinee(profiledCallSiteId);
    while (inlineeData && inlineeData->GetBody()->GetAddr() != inlineeFuncBodyAddr)
    {
        inlineeData = inlineeData->GetNext();
    }
    __analysis_assume(inlineeData != nullptr);
    return inlineeData->GetRuntimeInfo();
}

const FunctionJITRuntimeInfo *
FunctionJITTimeInfo::GetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId) const
{
    return GetInlinee(profiledCallSiteId) ? GetInlinee(profiledCallSiteId)->GetRuntimeInfo() : nullptr;
}

const FunctionJITRuntimeInfo *
FunctionJITTimeInfo::GetLdFldInlineeRuntimeData(const Js::InlineCacheIndex inlineCacheIndex) const
{
    return GetLdFldInlinee(inlineCacheIndex) ? GetLdFldInlinee(inlineCacheIndex)->GetRuntimeInfo() : nullptr;
}

const FunctionJITRuntimeInfo *
FunctionJITTimeInfo::GetRuntimeInfo() const
{
    return reinterpret_cast<const FunctionJITRuntimeInfo*>(m_data.profiledRuntimeData);
}

ObjTypeSpecFldInfo *
FunctionJITTimeInfo::GetObjTypeSpecFldInfo(uint index) const
{
    Assert(index < GetBody()->GetInlineCacheCount());
    if (m_data.objTypeSpecFldInfoArray == nullptr)
    {
        return nullptr;
    }

    return reinterpret_cast<ObjTypeSpecFldInfo *>(m_data.objTypeSpecFldInfoArray[index]);
}

ObjTypeSpecFldInfo *
FunctionJITTimeInfo::GetGlobalObjTypeSpecFldInfo(uint index) const
{
    Assert(index < m_data.globalObjTypeSpecFldInfoCount);

    return reinterpret_cast<ObjTypeSpecFldInfo *>(m_data.globalObjTypeSpecFldInfoArray[index]);
}

uint
FunctionJITTimeInfo::GetGlobalObjTypeSpecFldInfoCount() const
{
    return m_data.globalObjTypeSpecFldInfoCount;
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


    return reinterpret_cast<const FunctionJITTimeInfo*>(m_data.ldFldInlinees[inlineCacheIndex]);
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

    auto inlinee = reinterpret_cast<const FunctionJITTimeInfo *>(m_data.inlinees[profileId]);
    if (inlinee == nullptr && m_data.inlineesRecursionFlags[profileId])
    {
        inlinee = this;
    }
    return inlinee;
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

    return ((FunctionJITTimeDataIDL*)this->GetInlinee(profiledCallSiteId))->next != nullptr;
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


char16*
FunctionJITTimeInfo::GetDisplayName() const
{
    return GetBody()->GetDisplayName();
}

char16*
FunctionJITTimeInfo::GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const
{
    // (#%u.%u), #%u --> (source file Id . function Id) , function Number
    int len = swprintf_s(bufferToWriteTo, MAX_FUNCTION_BODY_DEBUG_STRING_SIZE, _u(" (#%d.%u), #%u"),
        (int)GetSourceContextId(), GetLocalFunctionId(), GetBody()->GetFunctionNumber());
    Assert(len > 8);
    return bufferToWriteTo;
}
