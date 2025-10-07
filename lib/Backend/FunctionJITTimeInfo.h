//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class FunctionJITTimeInfo
{
public:
    FunctionJITTimeInfo(FunctionJITTimeDataIDL * data);
    static void BuildJITTimeData(
        _In_ ArenaAllocator * alloc,
        _In_ const Js::FunctionCodeGenJitTimeData * codeGenData,
        __in_opt const Js::FunctionCodeGenRuntimeData * runtimeData,
        _Out_ FunctionJITTimeDataIDL * jitData,
        bool isInlinee,
        bool isForegroundJIT);

    uint GetInlineeCount() const;
    bool IsLdFldInlineePresent() const;

    const FunctionJITTimeInfo * GetCallbackInlinee(Js::ProfileId profileId) const;
    const FunctionJITTimeInfo * GetCallApplyTargetInlinee(Js::ProfileId profileId) const;
    const Js::ProfileId GetCallApplyCallSiteIdForCallSiteId(Js::ProfileId profiledCallSiteId) const;
    const FunctionJITTimeInfo * GetLdFldInlinee(Js::InlineCacheIndex inlineCacheIndex) const;
    const FunctionJITTimeInfo * GetInlinee(Js::ProfileId profileId) const;
    const FunctionJITTimeInfo * GetNext() const;
    JITTimeFunctionBody * GetBody() const;
    bool IsPolymorphicCallSite(Js::ProfileId profiledCallSiteId) const;
    intptr_t GetFunctionInfoAddr() const;
    intptr_t GetEntryPointInfoAddr() const;
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
    void ClearObjTypeSpecFldInfo(uint index);
    ObjTypeSpecFldInfo * GetGlobalObjTypeSpecFldInfo(uint index) const;
    uint GetGlobalObjTypeSpecFldInfoCount() const;
    const FunctionJITRuntimeInfo * GetInlineeForTargetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId, intptr_t inlineeFuncBodyAddr) const;
    const FunctionJITRuntimeInfo *GetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId) const;
    const FunctionJITRuntimeInfo *GetLdFldInlineeRuntimeData(const Js::InlineCacheIndex inlineCacheIndex) const;
    const FunctionJITRuntimeInfo * GetCallbackInlineeRuntimeData(const Js::ProfileId profiledCallSiteId) const;
    const FunctionJITRuntimeInfo * GetInlineeForCallbackInlineeRuntimeData(const Js::ProfileId profiledCallSiteId, intptr_t inlineeFuncBodyAddr) const;
    const FunctionJITRuntimeInfo * GetCallApplyTargetInlineeRuntimeData(const Js::ProfileId callApplyCallSiteId) const;
    bool ForceJITLoopBody() const;
    bool HasSharedPropertyGuards() const;
    bool HasSharedPropertyGuard(Js::PropertyId id) const;
    bool IsJsBuiltInForceInline() const;

    char16* GetDisplayName() const;
    char16* GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const;
private:
    FunctionJITTimeDataIDL m_data;
};
