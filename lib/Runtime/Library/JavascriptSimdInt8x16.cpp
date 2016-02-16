//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDInt8x16::JavascriptSIMDInt8x16(SIMDValue *val, StaticType *type) : JavascriptSIMDType(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt8x16);
    }

    JavascriptSIMDInt8x16* JavascriptSIMDInt8x16::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt8x16, val, requestContext->GetLibrary()->GetSIMDInt8x16TypeStatic());
    }

    bool  JavascriptSIMDInt8x16::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDInt8x16;
    }

    JavascriptSIMDInt8x16* JavascriptSIMDInt8x16::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDInt8x16'");

        return reinterpret_cast<JavascriptSIMDInt8x16 *>(aValue);
    }

    Var JavascriptSIMDInt8x16::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        wchar_t *typeString = L"SIMD.Int8x16(";
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<int8, 16>(args, numArgs, typeString,
            simdValue.i8, &callInfo, &requestContext);
    }

    RecyclableObject * JavascriptSIMDInt8x16::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt8x16::New(&value, requestContext);
    }

    const wchar_t* JavascriptSIMDInt8x16::GetFullBuiltinName(wchar_t** aBuffer, const wchar_t* name)
    {
        Assert(aBuffer && *aBuffer);
        swprintf_s(*aBuffer, SIMD_STRING_BUFFER_MAX, L"SIMD.Int8x16.%s", name);
        return *aBuffer;
    }

    Var JavascriptSIMDInt8x16::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt8x16::New(&this->value, requestContext);
    }
}
