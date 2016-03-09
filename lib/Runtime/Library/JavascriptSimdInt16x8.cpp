//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"


namespace Js
{
    JavascriptSIMDInt16x8::JavascriptSIMDInt16x8(SIMDValue *val, StaticType *type) : JavascriptSIMDType(val, type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt16x8);
    }

    JavascriptSIMDInt16x8* JavascriptSIMDInt16x8::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt16x8 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt16x8, val, requestContext->GetLibrary()->GetSIMDInt16x8TypeStatic());
    }

    bool  JavascriptSIMDInt16x8::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDInt16x8;
    }

    JavascriptSIMDInt16x8* JavascriptSIMDInt16x8::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDInt16x8'");

        return reinterpret_cast<JavascriptSIMDInt16x8 *>(aValue);
    }

    Var JavascriptSIMDInt16x8::CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue,
        const Var* args, uint numArgs, CallInfo callInfo)
    {
        char16 *typeString = _u("SIMD.Int16x8(");
        return JavascriptSIMDObject::FromVar(&obj)->ToLocaleString<int16, 8>(args, numArgs, typeString,
            simdValue.i16, &callInfo, &requestContext);
    }

    RecyclableObject * JavascriptSIMDInt16x8::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt16x8::New(&value, requestContext);
    }

    const char16* JavascriptSIMDInt16x8::GetFullBuiltinName(char16** aBuffer, const char16* name)
    {
        Assert(aBuffer && *aBuffer);
        swprintf_s(*aBuffer, SIMD_STRING_BUFFER_MAX, _u("SIMD.Int16x8.%s"), name);
        return *aBuffer;
    }

    Var JavascriptSIMDInt16x8::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt16x8::New(&this->value, requestContext);
    }

    Var JavascriptSIMDInt16x8::CopyAndSetLane(uint index, int value, ScriptContext* requestContext)
    {
        AssertMsg(index < 8, "Out of range lane index");
        Var instance = Copy(requestContext);
        JavascriptSIMDInt16x8 *insValue = JavascriptSIMDInt16x8::FromVar(instance);
        Assert(insValue);
        insValue->value.i16[index] = (short)value;
        return instance;
    }

    __inline Var  JavascriptSIMDInt16x8::GetLaneAsNumber(uint index, ScriptContext* requestContext)
    {
        // convert value.i32[index] to TaggedInt
        AssertMsg(index < 8, "Out of range lane index");
        return JavascriptNumber::ToVar(value.i16[index], requestContext);
    }

    __inline Var  JavascriptSIMDInt16x8::GetLaneAsFlag(uint index, ScriptContext* requestContext)
    {
        // convert value.i32[index] to TaggedInt
        AssertMsg(index < 8, "Out of range lane index");
        return JavascriptNumber::ToVar(value.i16[index] != 0x0, requestContext);
    }
}

