//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_NATIVE_CODEGEN
// forward ref
struct FunctionBodyDataIDL;

namespace Js
{
    // - Data generated for jitting purposes
    // - Recycler-allocated, lifetime is from when a code gen work item is added to the jit queue, to when jitting is complete
    //     - Also keeps the function body and inlinee function bodies alive while jitting.
    class FunctionCodeGenJitTimeData
    {
    private:
        FunctionBodyDataIDL * bodyData;
        FunctionInfo *const functionInfo;

        // Point's to an entry point if the work item needs the entry point alive- null for cases where the entry point isn't used
        EntryPointInfo *const entryPointInfo;

        // These cloned inline caches are guaranteed to have stable data while jitting, but will be collectible after jitting
        ObjTypeSpecFldInfoArray objTypeSpecFldInfoArray;

        // Globally ordered list of all object type specialized property access information (monomorphic and polymorphic caches combined).
        uint globalObjTypeSpecFldInfoCount;
        ObjTypeSpecFldInfo** globalObjTypeSpecFldInfoArray;

        // There will be a non-null entry for each profiled call site where a function is to be inlined
        FunctionCodeGenJitTimeData **inlinees;
        FunctionCodeGenJitTimeData **ldFldInlinees;
        RecyclerWeakReference<FunctionBody> *weakFuncRef;

        PolymorphicInlineCacheInfoIDL* inlineeInfo;
        PolymorphicInlineCacheInfoIDL* selfInfo;
        PolymorphicInlineCacheIDL* polymorphicInlineCaches;

        // Number of functions that are to be inlined (this is not the length of the 'inlinees' array above, includes getter setter inlinee count)
        uint inlineeCount;
        // Number of counts of getter setter to be inlined. This is not an exact count as inline caches are shared and we have no way of knowing
        // accurate count.
        uint ldFldInlineeCount;

        // For polymorphic call site we will have linked list of FunctionCodeGenJitTimeData
        // Each is differentiated by id starting from 0, 1
        FunctionCodeGenJitTimeData *next;
        bool isInlined;

        // This indicates the function is aggressively Inlined(see NativeCodeGenerator::TryAggressiveInlining) .
        bool isAggressiveInliningEnabled;

#ifdef FIELD_ACCESS_STATS
    public:
        FieldAccessStatsPtr inlineCacheStats;

        void EnsureInlineCacheStats(Recycler* recycler);
        void AddInlineeInlineCacheStats(FunctionCodeGenJitTimeData* inlineeJitTimeData);
#endif

    public:
        FunctionCodeGenJitTimeData(FunctionInfo *const functionInfo, EntryPointInfo *const entryPoint, bool isInlined = true);

    public:
        BVFixed *inlineesBv;

        FunctionInfo *GetFunctionInfo() const;
        FunctionBody *GetFunctionBody() const;
        FunctionBodyDataIDL *GetJITBody() const;
        FunctionCodeGenJitTimeData *GetNext() const { return next; }

        const ObjTypeSpecFldInfoArray* GetObjTypeSpecFldInfoArray() const { return &this->objTypeSpecFldInfoArray; }
        ObjTypeSpecFldInfoArray* GetObjTypeSpecFldInfoArray() { return &this->objTypeSpecFldInfoArray; }
        EntryPointInfo* GetEntryPointInfo() const { return this->entryPointInfo; }

    public:
        const FunctionCodeGenJitTimeData *GetInlinee(const ProfileId profiledCallSiteId) const;
        FunctionCodeGenJitTimeData ** GetInlinees();
        const FunctionCodeGenJitTimeData *GetLdFldInlinee(const InlineCacheIndex inlineCacheIndex) const;
        FunctionCodeGenJitTimeData ** GetLdFldInlinees();
        FunctionCodeGenJitTimeData *AddInlinee(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId,
            FunctionInfo *const inlinee,
            bool isInlined = true);
        uint InlineeCount() const;
        uint LdFldInlineeCount() const;
        bool IsLdFldInlineePresent() const { return ldFldInlineeCount != 0; }

        RecyclerWeakReference<FunctionBody> *GetWeakFuncRef() const { return this->weakFuncRef; }
        void SetWeakFuncRef(RecyclerWeakReference<FunctionBody> *weakFuncRef)
        {
            Assert(this->weakFuncRef == nullptr || weakFuncRef == nullptr || this->weakFuncRef == weakFuncRef);
            this->weakFuncRef = weakFuncRef;
        }

        void SetPolymorphicInlineInfo(PolymorphicInlineCacheInfoIDL* inlineeInfo, PolymorphicInlineCacheInfoIDL* selfInfo, PolymorphicInlineCacheIDL* polymorphicInlineCaches)
        {
            this->inlineeInfo = inlineeInfo;
            this->selfInfo = selfInfo;
            this->polymorphicInlineCaches = polymorphicInlineCaches;
        }

        FunctionCodeGenJitTimeData *AddLdFldInlinee(
            Recycler *const recycler,
            const InlineCacheIndex inlineCacheIndex,
            FunctionInfo *const inlinee);

        bool IsPolymorphicCallSite(const ProfileId profiledCallSiteId) const;
        // This function walks all the chained jittimedata and returns the one which match the functionInfo.
        // This can return null, if the functionInfo doesn't match.
        const FunctionCodeGenJitTimeData *GetJitTimeDataFromFunctionInfo(FunctionInfo *polyFunctionInfo) const;

        uint GetGlobalObjTypeSpecFldInfoCount() const { return this->globalObjTypeSpecFldInfoCount; }
        ObjTypeSpecFldInfo** GetGlobalObjTypeSpecFldInfoArray() const {return this->globalObjTypeSpecFldInfoArray; }

        ObjTypeSpecFldInfo* GetGlobalObjTypeSpecFldInfo(uint propertyInfoId) const
        {
            Assert(this->globalObjTypeSpecFldInfoArray != nullptr && propertyInfoId < this->globalObjTypeSpecFldInfoCount);
            return this->globalObjTypeSpecFldInfoArray[propertyInfoId];
        }

        void SetGlobalObjTypeSpecFldInfo(uint propertyInfoId, ObjTypeSpecFldInfo* info) const
        {
            Assert(this->globalObjTypeSpecFldInfoArray != nullptr && propertyInfoId < this->globalObjTypeSpecFldInfoCount);
            this->globalObjTypeSpecFldInfoArray[propertyInfoId] = info;
        }

        void SetGlobalObjTypeSpecFldInfoArray(ObjTypeSpecFldInfo** array, uint count)
        {
            Assert(array != nullptr);
            this->globalObjTypeSpecFldInfoArray = array;
            this->globalObjTypeSpecFldInfoCount = count;
        }

        bool GetIsInlined() const
        {
            return isInlined;
        }
        bool GetIsAggressiveInliningEnabled() const
        {
            return isAggressiveInliningEnabled;
        }
        void SetIsAggressiveInliningEnabled()
        {
            isAggressiveInliningEnabled = true;
        }

        void SetupRecursiveInlineeChain(
            Recycler *const recycler,
            const ProfileId profiledCallSiteId)
        {
            if (!inlinees)
            {
                inlinees = RecyclerNewArrayZ(recycler, FunctionCodeGenJitTimeData *, GetFunctionBody()->GetProfiledCallSiteCount());
            }
            inlinees[profiledCallSiteId] = this;
            inlineeCount++;
        }

        PREVENT_COPY(FunctionCodeGenJitTimeData)
    };
}
#endif

