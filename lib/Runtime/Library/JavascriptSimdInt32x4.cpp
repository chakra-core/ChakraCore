//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDInt32x4::TypeName[] = _u("SIMD.Int32x4");

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
        return false;
    }

    const char16* JavascriptSIMDInt32x4::GetTypeName()
    {
        return JavascriptSIMDInt32x4::TypeName;
    }

    Var JavascriptSIMDInt32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::New(&this->value, requestContext);
    }

    Var JavascriptSIMDInt32x4::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        const char16 *typeString = _u("SIMD.Int32x4(");
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<int, 4>(args, numArgs, typeString,
            simdValue.i32, &callInfo, &requestContext);
    }
}
#endif
