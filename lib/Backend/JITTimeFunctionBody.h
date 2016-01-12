//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTimeFunctionBody
{
public:
    JITTimeFunctionBody(const FunctionBodyJITData * const bodyData);

    uint GetFunctionNumber() const;
    uint GetLocalFunctionId() const;
    uint GetSourceContextId() const;
    uint GetNestedCount() const;
    uint GetScopeSlotArraySize() const;
    uint GetLengthInBytes() const;
    uint GetByteCodeCount() const;
    uint GetByteCodeInLoopCount() const;

    uint16 GetEnvDepth() const;
    Js::ProfileId GetProfiledCallSiteCount() const;

    bool DoStackNestedFunc() const;
    bool DoStackClosure() const;
    bool DoBackendArgumentsOptimization() const;
    bool IsLibraryCode() const;
    bool HasTry() const;
    bool HasOrParentHasArguments() const;
    bool IsGenerator() const;
    bool IsAsmJsMode() const;

private:
    Js::FunctionInfo::Attributes GetAttributes() const;
    Js::FunctionBody::FunctionBodyFlags GetFlags() const;

    const FunctionBodyJITData * const m_bodyData;
};
