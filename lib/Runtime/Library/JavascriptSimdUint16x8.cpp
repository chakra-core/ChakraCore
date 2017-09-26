//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDUint16x8::TypeName[] = _u("SIMD.Uint16x8");

    JavascriptSIMDUint16x8::JavascriptSIMDUint16x8(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint16x8);
    }

    JavascriptSIMDUint16x8::JavascriptSIMDUint16x8(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint16x8);
    }

    JavascriptSIMDUint16x8* JavascriptSIMDUint16x8::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint16x8 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint16x8, val, requestContext->GetLibrary()->GetSIMDUint16x8TypeStatic());
    }

    bool  JavascriptSIMDUint16x8::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDUint16x8;
    }

    JavascriptSIMDUint16x8* JavascriptSIMDUint16x8::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDUint16x8'");

        return reinterpret_cast<JavascriptSIMDUint16x8 *>(aValue);
    }

    Var JavascriptSIMDUint16x8::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        const char16 *typeString = _u("SIMD.Uint16x8(");
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<uint16, 8>(args, numArgs, typeString,
            simdValue.u16, &callInfo, &requestContext);
    }

    RecyclableObject * JavascriptSIMDUint16x8::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint16x8::New(&value, requestContext);
    }

    const char16* JavascriptSIMDUint16x8::GetTypeName()
    {
        return JavascriptSIMDUint16x8::TypeName;
    }

    Var JavascriptSIMDUint16x8::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint16x8::New(&this->value, requestContext);
    }
}
#endif
