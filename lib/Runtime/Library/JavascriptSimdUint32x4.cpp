//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDUint32x4::TypeName[] = _u("SIMD.Uint32x4");

    JavascriptSIMDUint32x4::JavascriptSIMDUint32x4(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint32x4);
    }

    JavascriptSIMDUint32x4::JavascriptSIMDUint32x4(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint32x4);
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint32x4, requestContext->GetLibrary()->GetSIMDUInt32x4TypeStatic());
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint32x4, val, requestContext->GetLibrary()->GetSIMDUInt32x4TypeStatic());
    }

    bool  JavascriptSIMDUint32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDUint32x4;
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDUint32x4'");

        return reinterpret_cast<JavascriptSIMDUint32x4 *>(aValue);
    }

    Var JavascriptSIMDUint32x4::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        const char16 *typeString = _u("SIMD.Uint32x4(");
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<int32, 4>(args, numArgs, typeString,
            simdValue.i32, &callInfo, &requestContext);
    }

    RecyclableObject * JavascriptSIMDUint32x4::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint32x4::New(&value, requestContext);
    }

    const char16* JavascriptSIMDUint32x4::GetTypeName()
    {
        return JavascriptSIMDUint32x4::TypeName;
    }

    Var JavascriptSIMDUint32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint32x4::New(&this->value, requestContext);
    }

}
#endif
