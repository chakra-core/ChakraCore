//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    const char16 JavascriptSIMDUint8x16::TypeName[] = _u("SIMD.Uint8x16");

    JavascriptSIMDUint8x16::JavascriptSIMDUint8x16(StaticType *type) : JavascriptSIMDType(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint8x16);
    }

    JavascriptSIMDUint8x16::JavascriptSIMDUint8x16(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint8x16);
    }

    JavascriptSIMDUint8x16* JavascriptSIMDUint8x16::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint8x16, val, requestContext->GetLibrary()->GetSIMDUint8x16TypeStatic());
    }

    bool  JavascriptSIMDUint8x16::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDUint8x16;
    }

    JavascriptSIMDUint8x16* JavascriptSIMDUint8x16::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDUint8x16'");

        return reinterpret_cast<JavascriptSIMDUint8x16 *>(aValue);
    }

    Var JavascriptSIMDUint8x16::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        const char16 *typeString = _u("SIMD.Uint8x16(");
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<uint8, 16>(args, numArgs, typeString,
            simdValue.u8, &callInfo, &requestContext);
    }

    RecyclableObject * JavascriptSIMDUint8x16::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint8x16::New(&value, requestContext);
    }

    const char16* JavascriptSIMDUint8x16::GetTypeName()
    {
        return JavascriptSIMDUint8x16::TypeName;
    }

    Var JavascriptSIMDUint8x16::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint8x16::New(&this->value, requestContext);
    }
}
#endif
