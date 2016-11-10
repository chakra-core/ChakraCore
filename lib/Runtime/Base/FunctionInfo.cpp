//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

namespace Js
{
    FunctionInfo::FunctionInfo(JavascriptMethod entryPoint, Attributes attributes, LocalFunctionId functionId, FunctionProxy* functionBodyImpl)
        : originalEntryPoint(entryPoint), attributes(attributes), functionBodyImpl(functionBodyImpl), functionId(functionId)
    {
#if !DYNAMIC_INTERPRETER_THUNK
        Assert(entryPoint != nullptr);
#endif
    }

    void FunctionInfo::VerifyOriginalEntryPoint() const
    {
        Assert(!this->HasBody() || this->IsDeferredParseFunction() || this->IsDeferredDeserializeFunction() || this->GetFunctionProxy()->HasValidEntryPoint());
    }

    JavascriptMethod
    FunctionInfo::GetOriginalEntryPoint() const
    {
        VerifyOriginalEntryPoint();
        return GetOriginalEntryPoint_Unchecked();
    }

    JavascriptMethod FunctionInfo::GetOriginalEntryPoint_Unchecked() const
    {
        return originalEntryPoint;
    }

    void FunctionInfo::SetOriginalEntryPoint(const JavascriptMethod originalEntryPoint)
    {
        Assert(originalEntryPoint);
        this->originalEntryPoint = originalEntryPoint;
    }

    FunctionBody *
    FunctionInfo::GetFunctionBody() const
    {
        return functionBodyImpl == nullptr ? nullptr : functionBodyImpl->GetFunctionBody();
    }

    FunctionInfo::Attributes FunctionInfo::GetAttributes(Js::RecyclableObject * function)
    {
        return function->GetTypeId() == Js::TypeIds_Function ?
            Js::JavascriptFunction::FromVar(function)->GetFunctionInfo()->GetAttributes() : Js::FunctionInfo::None;
    }
}
