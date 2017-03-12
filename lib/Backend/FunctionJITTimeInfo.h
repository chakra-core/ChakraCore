//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class FunctionJITTimeInfo
{
public:
    FunctionJITTimeInfo(FunctionJITTimeDataIDL * data);
    static void BuildJITTimeData(
        __in ArenaAllocator * alloc,
        __in const Js::FunctionCodeGenJitTimeData * codeGenData,
        __in_opt const Js::FunctionCodeGenRuntimeData * runtimeData,
        __out FunctionJITTimeDataIDL * jitData,
        bool isInlinee,
        bool isForegroundJIT);

    uint GetInlineeCount() const;
    bool IsLdFldInlineePresent() const;

    const FunctionJITTimeInfo * GetLdFldInlinee(Js::InlineCacheIndex inlineCacheIndex) const;
    const FunctionJITTimeInfo * GetInlinee(Js::ProfileId profileId) const;
    const FunctionJITTimeInfo * GetNext() const;
    JITTimeFunctionBody * GetBody() const;
    bool IsPolymorphicCallSite(Js::ProfileId profiledCallSiteId) const;
    intptr_t GetFunctionInfoAddr() const;
    intptr_t GetWeakFuncRef() const;
    uint GetLocalFunctionId() const;
    uint GetSourceContextId() const;
    bool HasBody() const;
    bool IsAggressiveInliningEnabled() const;
    bool IsInlined() const;
    const FunctionJITRuntimeInfo * GetRuntimeInfo() const;
    const BVFixed * GetInlineesBV() const;
    const FunctionJITTimeInfo * GetJitTimeDataFromFunctionInfoAddr(intptr_t polyFuncInfo) const;
    ObjTypeSpecFldInfo * GetObjTypeSpecFldInfo(uint index) const;
    ObjTypeSpecFldInfo * GetGlobalObjTypeSpecFldInfo(uint index) const;
    uint GetGlobalObjTypeSpecFldInfoCount() const;
    const FunctionJITRuntimeInfo * GetInlineeForTargetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId, intptr_t inlineeFuncBodyAddr) const;
    const FunctionJITRuntimeInfo *GetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId) const;
    const FunctionJITRuntimeInfo *GetLdFldInlineeRuntimeData(const Js::InlineCacheIndex inlineCacheIndex) const;
    bool ForceJITLoopBody() const;
    bool HasSharedPropertyGuards() const;
    bool HasSharedPropertyGuard(Js::PropertyId id) const;

    char16* GetDisplayName() const;
    char16* GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const;
private:
    FunctionJITTimeDataIDL m_data;
};
