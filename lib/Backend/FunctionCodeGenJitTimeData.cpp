//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

#if ENABLE_NATIVE_CODEGEN
namespace Js
{
    FunctionCodeGenJitTimeData::FunctionCodeGenJitTimeData(FunctionInfo *const functionInfo, EntryPointInfo *const entryPoint, Var globalThis, uint16 profiledIterations, bool isInlined) :
        functionInfo(functionInfo), entryPointInfo(entryPoint), globalObjTypeSpecFldInfoCount(0), globalObjTypeSpecFldInfoArray(nullptr),
        weakFuncRef(nullptr), inlinees(nullptr), inlineeCount(0), ldFldInlineeCount(0), isInlined(isInlined), isAggressiveInliningEnabled(false),
#ifdef FIELD_ACCESS_STATS
        inlineCacheStats(nullptr),
#endif
        next(nullptr),
        ldFldInlinees(nullptr),
        globalThisObject(globalThis),
        profiledIterations(profiledIterations),
        sharedPropertyGuards(nullptr),
        sharedPropertyGuardCount(0)
    {
    }

    FunctionCodeGenJitTimeData* FunctionCodeGenJitTimeData::New(Recycler* recycler, FunctionInfo *const functionInfo, EntryPointInfo *const entryPoint, bool isInlined)
    {
        Var globalThis = nullptr;
        uint16 profiledIterations = 0;
        FunctionProxy *proxy = functionInfo->GetFunctionProxy();
        if (proxy && proxy->IsFunctionBody())
        {
            FunctionBody* functionBody = proxy->GetFunctionBody();
            if (functionBody)
            {
                if (functionBody->GetByteCode())
                {
                    globalThis = functionBody->GetScriptContext()->GetLibrary()->GetGlobalObject()->ToThis();
                    profiledIterations = functionBody->GetProfiledIterations();
                }

                DebugOnly(functionBody->LockDownCounters());
            }
        }

        return RecyclerNew(recycler, FunctionCodeGenJitTimeData, functionInfo, entryPoint, globalThis, profiledIterations, isInlined);
    }


    uint16 FunctionCodeGenJitTimeData::GetProfiledIterations() const
    {
        return profiledIterations;
    }

    FunctionInfo *FunctionCodeGenJitTimeData::GetFunctionInfo() const
    {
        return this->functionInfo;
    }

    FunctionBody *FunctionCodeGenJitTimeData::GetFunctionBody() const
    {
        FunctionProxy *proxy = this->functionInfo->GetFunctionProxy();
        return proxy && proxy->IsFunctionBody() ? proxy->GetFunctionBody() : nullptr;
    }

    Var FunctionCodeGenJitTimeData::GetGlobalThisObject() const
    {
        return this->globalThisObject;
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
            inlinees = RecyclerNewArrayZ(recycler, Field(FunctionCodeGenJitTimeData *), functionBody->GetProfiledCallSiteCount());
        }

        FunctionCodeGenJitTimeData *inlineeData = nullptr;
        if (!inlinees[profiledCallSiteId])
        {
            inlineeData = FunctionCodeGenJitTimeData::New(recycler, inlinee, nullptr /* entryPoint */, isInlined);
            inlinees[profiledCallSiteId] = inlineeData;
            if (++inlineeCount == 0)
            {
                Js::Throw::OutOfMemory();
            }
        }
        else
        {
            inlineeData = FunctionCodeGenJitTimeData::New(recycler, inlinee, nullptr /* entryPoint */, isInlined);
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
            ldFldInlinees = RecyclerNewArrayZ(recycler, Field(FunctionCodeGenJitTimeData*), GetFunctionBody()->GetInlineCacheCount());
        }

        const auto inlineeData = FunctionCodeGenJitTimeData::New(recycler, inlinee, nullptr);
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
