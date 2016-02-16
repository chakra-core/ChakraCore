//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDInt32x4::JavascriptSIMDInt32x4(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt32x4);
    }

    JavascriptSIMDInt32x4::JavascriptSIMDInt32x4(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt32x4);
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt32x4, requestContext->GetLibrary()->GetSIMDInt32x4TypeStatic());
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt32x4, val, requestContext->GetLibrary()->GetSIMDInt32x4TypeStatic());
    }

    bool  JavascriptSIMDInt32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDInt32x4;
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDInt32x4'");

        return reinterpret_cast<JavascriptSIMDInt32x4 *>(aValue);
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::FromBool(SIMDValue *val, ScriptContext* requestContext)
    {
        SIMDValue result = SIMDInt32x4Operation::OpFromBool(*val);
        return JavascriptSIMDInt32x4::New(&result, requestContext);
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::FromFloat64x2(JavascriptSIMDFloat64x2 *instance, ScriptContext* requestContext)
    {
        SIMDValue result = SIMDInt32x4Operation::OpFromFloat64x2(instance->GetValue());
        return JavascriptSIMDInt32x4::New(&result, requestContext);
    }

    RecyclableObject * JavascriptSIMDInt32x4::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::New(&value, requestContext);
    }

    bool JavascriptSIMDInt32x4::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        switch (propertyId)
        {
        case PropertyIds::signMask:
            *value = GetSignMask();
            return true;

        case PropertyIds::flagX:
        case PropertyIds::flagY:
        case PropertyIds::flagZ:
        case PropertyIds::flagW:
            *value = GetLaneAsFlag(propertyId - PropertyIds::flagX, requestContext);
            return true;
        }

        return false;
    }

    const wchar_t* JavascriptSIMDInt32x4::GetFullBuiltinName(wchar_t** aBuffer, const wchar_t* name)
    {
        Assert(aBuffer && *aBuffer);
        swprintf_s(*aBuffer, SIMD_STRING_BUFFER_MAX, L"SIMD.Int32x4.%s", name);
        return *aBuffer;
    }

    Var JavascriptSIMDInt32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::New(&this->value, requestContext);
    }

    Var JavascriptSIMDInt32x4::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        wchar_t *typeString = L"SIMD.Int32x4(";
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<int, 4>(args, numArgs, typeString,
            simdValue.i32, &callInfo, &requestContext);
    }

    Var  JavascriptSIMDInt32x4::CopyAndSetLaneFlag(uint index, BOOL value, ScriptContext* requestContext)
    {
        AnalysisAssert(index < 4);
        Var instance = Copy(requestContext);
        JavascriptSIMDInt32x4 *insValue = JavascriptSIMDInt32x4::FromVar(instance);
        Assert(insValue);
        insValue->value.i32[index] = value ? -1 : 0;
        return instance;
    }

    __inline Var  JavascriptSIMDInt32x4::GetLaneAsFlag(uint index, ScriptContext* requestContext)
    {
        // convert value.i32[index] to TaggedInt
        AnalysisAssert(index < 4);
        return JavascriptNumber::ToVar(value.i32[index] != 0x0, requestContext);
    }

    __inline Var JavascriptSIMDInt32x4::GetSignMask()
    {
        int signMask = SIMDInt32x4Operation::OpGetSignMask(value);

        return TaggedInt::ToVarUnchecked(signMask);
    }
}
