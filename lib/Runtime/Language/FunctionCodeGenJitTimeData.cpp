//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"
#include "JITTimeFunctionBody.h"

#if ENABLE_NATIVE_CODEGEN
namespace Js
{
    FunctionCodeGenJitTimeData::FunctionCodeGenJitTimeData(FunctionInfo *const functionInfo, EntryPointInfo *const entryPoint, bool isInlined) :
        functionInfo(functionInfo), entryPointInfo(entryPoint), globalObjTypeSpecFldInfoCount(0), globalObjTypeSpecFldInfoArray(nullptr),
        weakFuncRef(nullptr), inlinees(nullptr), inlineeCount(0), ldFldInlineeCount(0), isInlined(isInlined), isAggressiveInliningEnabled(false),
#ifdef FIELD_ACCESS_STATS
        inlineCacheStats(nullptr),
#endif
        next(0),
        ldFldInlinees(nullptr),
        bodyData(nullptr)
    {
        if (GetFunctionInfo()->HasBody())
        {
            Recycler * recycler = functionInfo->GetFunctionProxy()->GetScriptContext()->GetRecycler();
            this->bodyData = RecyclerNewStructZ(recycler, FunctionBodyDataIDL);
            JITTimeFunctionBody::InitializeJITFunctionData(recycler, GetFunctionBody(), this->bodyData);
        }
    }

    FunctionInfo *FunctionCodeGenJitTimeData::GetFunctionInfo() const
    {
        return this->functionInfo;
    }

    FunctionBodyDataIDL *FunctionCodeGenJitTimeData::GetJITBody() const
    {
        return this->bodyData;
    }

    FunctionBody *FunctionCodeGenJitTimeData::GetFunctionBody() const
    {
        return this->functionInfo->GetFunctionBody();
    }

    bool FunctionCodeGenJitTimeData::IsPolymorphicCallSite(const ProfileId profiledCallSiteId) const
    {
        Assert(GetFunctionBody());
        Assert(profiledCallSiteId < GetFunctionBody()->GetProfiledCallSiteCount());

        return inlinees ? inlinees[profiledCallSiteId]->next != nullptr : false;
    }

    const FunctionCodeGenJitTimeData *FunctionCodeGenJitTimeData::GetInlinee(const ProfileId profiledCallSiteId) const
    {
        Assert(GetFunctionBody());
        Assert(profiledCallSiteId < GetFunctionBody()->GetProfiledCallSiteCount());

        return inlinees ? inlinees[profiledCallSiteId] : nullptr;
    }

    FunctionCodeGenJitTimeData ** FunctionCodeGenJitTimeData::GetInlinees()
    {
        return inlinees;
    }

    const FunctionCodeGenJitTimeData *FunctionCodeGenJitTimeData::GetJitTimeDataFromFunctionInfo(FunctionInfo *polyFunctionInfo) const
    {
        const FunctionCodeGenJitTimeData *next = this;
        while (next && next->functionInfo != polyFunctionInfo)
        {
            next = next->next;
        }
        return next;
    }

    const FunctionCodeGenJitTimeData *FunctionCodeGenJitTimeData::GetLdFldInlinee(const InlineCacheIndex inlineCacheIndex) const
    {
        Assert(GetFunctionBody());
        Assert(inlineCacheIndex < GetFunctionBody()->GetInlineCacheCount());

        return ldFldInlinees ? ldFldInlinees[inlineCacheIndex] : nullptr;
    }

    FunctionCodeGenJitTimeData ** FunctionCodeGenJitTimeData::GetLdFldInlinees()
    {
        return ldFldInlinees;
    }

    FunctionCodeGenJitTimeData *FunctionCodeGenJitTimeData::AddInlinee(
        Recycler *const recycler,
        const ProfileId profiledCallSiteId,
        FunctionInfo *const inlinee,
        bool isInlined)
    {
        Assert(recycler);
        const auto functionBody = GetFunctionBody();
        Assert(functionBody);
        Assert(profiledCallSiteId < functionBody->GetProfiledCallSiteCount());
        Assert(inlinee);

        if (!inlinees)
        {
            inlinees = RecyclerNewArrayZ(recycler, FunctionCodeGenJitTimeData *, functionBody->GetProfiledCallSiteCount());
        }

        FunctionCodeGenJitTimeData *inlineeData = nullptr;
        if (!inlinees[profiledCallSiteId])
        {
            inlineeData = RecyclerNew(recycler, FunctionCodeGenJitTimeData, inlinee, nullptr /* entryPoint */, isInlined);
            inlinees[profiledCallSiteId] = inlineeData;
            if (++inlineeCount == 0)
            {
                Js::Throw::OutOfMemory();
            }
        }
        else
        {
            inlineeData = RecyclerNew(recycler, FunctionCodeGenJitTimeData, inlinee, nullptr /* entryPoint */, isInlined);
            // This is polymorphic, chain the data.
            inlineeData->next = inlinees[profiledCallSiteId];
            inlinees[profiledCallSiteId] = inlineeData;
        }
        return inlineeData;
    }

    FunctionCodeGenJitTimeData *FunctionCodeGenJitTimeData::AddLdFldInlinee(
        Recycler *const recycler,
        const InlineCacheIndex inlineCacheIndex,
        FunctionInfo *const inlinee)
    {
        Assert(recycler);
        const auto functionBody = GetFunctionBody();
        Assert(functionBody);
        Assert(inlineCacheIndex < GetFunctionBody()->GetInlineCacheCount());
        Assert(inlinee);

        if (!ldFldInlinees)
        {
            ldFldInlinees = RecyclerNewArrayZ(recycler, FunctionCodeGenJitTimeData *, GetFunctionBody()->GetInlineCacheCount());
        }

        const auto inlineeData = RecyclerNew(recycler, FunctionCodeGenJitTimeData, inlinee, nullptr);
        Assert(!ldFldInlinees[inlineCacheIndex]);
        ldFldInlinees[inlineCacheIndex] = inlineeData;
        if (++ldFldInlineeCount == 0)
        {
            Js::Throw::OutOfMemory();
        }
        return inlineeData;
    }

    uint FunctionCodeGenJitTimeData::InlineeCount() const
    {
        return inlineeCount;
    }

    uint FunctionCodeGenJitTimeData::LdFldInlineeCount() const
    {
        return ldFldInlineeCount;
    }

#ifdef FIELD_ACCESS_STATS
    void FunctionCodeGenJitTimeData::EnsureInlineCacheStats(Recycler* recycler)
    {
        this->inlineCacheStats = RecyclerNew(recycler, FieldAccessStats);
    }

    void FunctionCodeGenJitTimeData::AddInlineeInlineCacheStats(FunctionCodeGenJitTimeData* inlineeJitTimeData)
    {
        Assert(this->inlineCacheStats != nullptr);
        Assert(inlineeJitTimeData != nullptr && inlineeJitTimeData->inlineCacheStats != nullptr);
        this->inlineCacheStats->Add(inlineeJitTimeData->inlineCacheStats);
    }
#endif
}
#endif
