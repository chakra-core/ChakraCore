//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

FunctionJITTimeInfo::FunctionJITTimeInfo(FunctionJITTimeData * data) : m_data(*data)
{
    // we will cast the data (i.e. midl struct) pointers into info pointers so we can extend with methods
    CompileAssert(sizeof(FunctionJITTimeData) == sizeof(FunctionJITTimeInfo));
}

/* static */
FunctionJITTimeData *
FunctionJITTimeInfo::BuildJITTimeData(ArenaAllocator * alloc, const Js::FunctionCodeGenJitTimeData * codeGenData, bool isInlinee)
{
    // using arena because we can't recycler allocate (may be on background), and heap freeing this is slightly complicated
    FunctionJITTimeData * jitData = AnewStructZ(alloc, FunctionJITTimeData);
    jitData->bodyData = codeGenData->GetJITBody();
    jitData->functionInfoAddr = (intptr_t)codeGenData->GetFunctionInfo();

    jitData->localFuncId = codeGenData->GetFunctionInfo()->GetLocalFunctionId();
    jitData->isAggressiveInliningEnabled = codeGenData->GetIsAggressiveInliningEnabled();
    jitData->isInlined = codeGenData->GetIsInlined();
    jitData->weakFuncRef = (intptr_t)codeGenData->GetWeakFuncRef();

    jitData->inlineesBv = (BVFixedData*)codeGenData->inlineesBv;

    if (codeGenData->GetFunctionInfo()->HasBody())
    {
        Js::FunctionBody * functionBody = codeGenData->GetFunctionBody();
        if (functionBody->HasDynamicProfileInfo())
        {
            Assert(jitData->bodyData != nullptr);
            ProfileData * profileData = AnewStruct(alloc, ProfileData);
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
            }
        }
        if (jitData->bodyData->profiledCallSiteCount > 0)
        {
            jitData->inlineeCount = jitData->bodyData->profiledCallSiteCount;
            jitData->inlinees = AnewArrayZ(alloc, FunctionJITTimeData*, jitData->bodyData->profiledCallSiteCount);
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
            jitData->ldFldInlinees = AnewArrayZ(alloc, FunctionJITTimeData*, jitData->bodyData->inlineCacheCount);
            for (Js::InlineCacheIndex i = 0; i < jitData->bodyData->inlineCacheCount; ++i)
            {
                const Js::FunctionCodeGenJitTimeData * inlinee = codeGenData->GetLdFldInlinee(i);
                if (inlinee != nullptr)
                {
                    jitData->ldFldInlinees[i] = BuildJITTimeData(alloc, inlinee);
                }
            }
            CompileAssert(sizeof(ObjTypeSpecFldData) == sizeof(Js::ObjTypeSpecFldInfo));
            jitData->objTypeSpecFldInfoArray = reinterpret_cast<ObjTypeSpecFldData**>(codeGenData->GetObjTypeSpecFldInfoArray()->GetInfoArray());
        }

        auto codegenRuntimeData = functionBody->GetCodeGenRuntimeData();
        if (codegenRuntimeData)
        {
            jitData->bodyData->runtimeDataCount = jitData->bodyData->profiledCallSiteCount;
            Assert(jitData->bodyData->profiledCallSiteCount > 0);
            jitData->bodyData->profiledRuntimeData = AnewArrayZ(alloc, FunctionJITRuntimeData, jitData->bodyData->runtimeDataCount);
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

Js::ObjTypeSpecFldInfo *
FunctionJITTimeInfo::GetObjTypeSpecFldInfo(uint index) const
{
    Assert(index < GetBody()->GetInlineCacheCount());
    return reinterpret_cast<Js::ObjTypeSpecFldInfo **>(m_data.objTypeSpecFldInfoArray)[index];
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
