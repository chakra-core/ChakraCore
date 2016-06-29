//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// forward decls
class AsmJsJITInfo;
class JITTimeProfileInfo;
class FunctionJITRuntimeInfo;

class JITTimeFunctionBody
{
public:
    JITTimeFunctionBody(FunctionBodyDataIDL * bodyData);

    static void InitializeJITFunctionData(
        __in Recycler * recycler,
        __in Js::FunctionBody * functionBody,
        __out FunctionBodyDataIDL * jitBody);

    intptr_t GetAddr() const;

    uint GetFunctionNumber() const;
    uint GetSourceContextId() const;
    uint GetNestedCount() const;
    uint GetScopeSlotArraySize() const;
    uint GetByteCodeCount() const;
    uint GetByteCodeInLoopCount() const;
    uint GetNonLoadByteCodeCount() const;
    uint GetLoopCount() const;
    uint GetByteCodeLength() const;
    uint GetInnerScopeCount() const;
    uint GetInlineCacheCount() const;
    uint GetRecursiveCallSiteCount() const;
    Js::RegSlot GetLocalFrameDisplayReg() const;
    Js::RegSlot GetLocalClosureReg() const;
    Js::RegSlot GetEnvReg() const;
    Js::RegSlot GetFirstTmpReg() const;
    Js::RegSlot GetFirstInnerScopeReg() const;
    Js::RegSlot GetVarCount() const;
    Js::RegSlot GetConstCount() const;
    Js::RegSlot GetLocalsCount() const;
    Js::RegSlot GetTempCount() const;
    Js::RegSlot GetFuncExprScopeReg() const;
    Js::RegSlot GetThisRegForEventHandler() const;
    Js::RegSlot GetFirstNonTempLocalIndex() const;
    Js::RegSlot GetEndNonTempLocalIndex() const;
    Js::RegSlot GetNonTempLocalVarCount() const;
    Js::RegSlot GetRestParamRegSlot() const;

    Js::PropertyId GetPropertyIdFromCacheId(uint cacheId) const;
    Js::PropertyId GetReferencedPropertyId(uint index) const;

    uint16 GetEnvDepth() const;
    uint16 GetProfiledIterations() const;
    uint16 GetArgUsedForBranch() const;
    Js::ProfileId GetProfiledCallSiteCount() const;
    Js::ArgSlot GetInParamsCount() const;

    bool DoStackNestedFunc() const;
    bool DoStackClosure() const;
    bool DoBackendArgumentsOptimization() const;
    bool IsLibraryCode() const;
    bool HasTry() const;
    bool HasOrParentHasArguments() const;
    bool IsGenerator() const;
    bool IsAsmJsMode() const;
    bool IsStrictMode() const;
    bool IsEval() const;
    bool HasImplicitArgIns() const;
    bool HasRestParameter() const;
    bool HasScopeObject() const;
    bool HasCachedScopePropIds() const;
    bool HasInlineCachesOnFunctionObject() const;
    bool DoInterruptProbe() const;
    bool IsGlobalFunc() const;
    bool IsInlineApplyDisabled() const;
    bool IsNonTempLocalVar(uint32 varIndex) const;
    bool DoJITLoopBody() const;
    bool IsInlineSpreadDisabled() const;
    bool HasLoops() const;
    bool HasNonBuiltInCallee() const;
    bool HasNestedLoop() const;
    bool CanInlineRecursively(uint depth, bool tryAggressive = true) const;

    const byte * GetByteCodeBuffer() const;


    intptr_t GetNestedFuncRef(uint index) const;
    intptr_t GetConstantVar(Js::RegSlot location) const;
    intptr_t GetInlineCache(uint index) const;
    intptr_t GetIsInstInlineCache(uint index) const;
    Js::TypeId GetConstantType(Js::RegSlot location) const;
    void * GetConstTable() const;

    intptr_t GetRootObject() const;
    intptr_t GetLoopHeaderAddr(uint loopNum) const;
    const JITLoopHeaderIDL * GetLoopHeaderData(uint loopNum) const;

    intptr_t GetScriptIdAddr() const;
    intptr_t GetProbeCountAddr() const;
    intptr_t GetFlagsAddr() const;
    intptr_t GetRegAllocLoadCountAddr() const;
    intptr_t GetRegAllocStoreCountAddr() const;
    intptr_t GetCallCountStatsAddr() const;
    intptr_t GetObjectLiteralTypeRef(uint index) const;
    intptr_t GetLiteralRegexAddr(uint index) const;
    const AsmJsJITInfo * GetAsmJsInfo() const;
    const JITTimeProfileInfo * GetProfileInfo() const;
    bool HasProfileInfo() const;

    static bool LoopContains(const JITLoopHeaderIDL * loop1, const JITLoopHeaderIDL * loop2);

    wchar_t* GetDisplayName() const;

    intptr_t ReadAuxArray(uint offset) const;
    Js::PropertyIdArray * ReadAuxPropIds(uint offset) const;
    const FunctionJITRuntimeInfo *GetInlineeRuntimeData(const Js::ProfileId profiledCallSiteId) const;
    const FunctionJITRuntimeInfo *GetLdFldInlineeRuntimeData(const Js::InlineCacheIndex inlineCacheIndex) const;

    void InitializeStatementMap(__out Js::SmallSpanSequence * statementMap) const;
private:
    Js::FunctionInfo::Attributes GetAttributes() const;
    Js::FunctionBody::FunctionBodyFlags GetFlags() const;

    FunctionBodyDataIDL m_bodyData;
};
