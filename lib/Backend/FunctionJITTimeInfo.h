//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class FunctionJITTimeInfo
{
public:
    FunctionJITTimeInfo(FunctionJITTimeData * data);
    static FunctionJITTimeData * BuildJITTimeData(ArenaAllocator * alloc, const Js::FunctionCodeGenJitTimeData * codeGenData, bool isInlinee = true);

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
    const BVFixed * GetInlineesBV() const;
    const FunctionJITTimeInfo * GetJitTimeDataFromFunctionInfoAddr(intptr_t polyFuncInfo) const;
    Js::ObjTypeSpecFldInfo * GetObjTypeSpecFldInfo(uint index) const;
    bool ForceJITLoopBody() const;

    wchar_t* GetDisplayName() const;
    wchar_t* GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const;
private:
    FunctionJITTimeData m_data;
};
