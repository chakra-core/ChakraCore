//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTimeFunctionBody
{
public:
    JITTimeFunctionBody(FunctionBodyJITData * bodyData);

    uint GetFunctionNumber() const;
    uint GetLocalFunctionId() const;
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

    Js::PropertyId GetPropertyIdFromCacheId(uint cacheId) const;

    uint16 GetEnvDepth() const;
    uint16 GetProfiledIterations() const;
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
    bool HasImplicitArgIns() const;
    bool HasRestParameter() const;
    bool HasScopeObject() const;
    bool HasCachedScopePropIds() const;
    bool HasInlineCachesOnFunctionObject() const;
    bool DoInterruptProbe() const;

    const byte * GetByteCodeBuffer() const;
    Js::SmallSpanSequence * GetStatementMapSpanSequence();

    intptr_t GetConstantVar(Js::RegSlot location) const;
    intptr_t GetInlineCache(uint index) const;
    Js::TypeId GetConstantType(Js::RegSlot location) const;

private:
    Js::FunctionInfo::Attributes GetAttributes() const;
    Js::FunctionBody::FunctionBodyFlags GetFlags() const;

    void InitializeStatementMap();

    const FunctionBodyJITData * const m_bodyData;
    Js::SmallSpanSequence m_statementMap;
};
