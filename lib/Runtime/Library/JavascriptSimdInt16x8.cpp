//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"


namespace Js
{
    JavascriptSIMDInt16x8::JavascriptSIMDInt16x8(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
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

    RecyclableObject * JavascriptSIMDInt16x8::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt16x8::New(&value, requestContext);
    }

    BOOL JavascriptSIMDInt16x8::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);

    }

    BOOL JavascriptSIMDInt16x8::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDInt16x8::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDInt16x8::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDInt16x8::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        switch (propertyId)
        {
        case PropertyIds::toString:
            *value = requestContext->GetLibrary()->GetSIMDInt16x8ToStringFunction();
            return true;
        }

        return false;
    }

    Var JavascriptSIMDInt16x8::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || JavascriptOperators::GetTypeId(args[0]) != TypeIds_SIMDInt16x8)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDInt16x8.toString");
        }

        JavascriptSIMDInt16x8* instance = JavascriptSIMDInt16x8::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[SIMD_STRING_BUFFER_MAX];
        SIMDValue value = instance->GetValue();

        JavascriptSIMDInt16x8::ToStringBuffer(value, stringBuffer, SIMD_STRING_BUFFER_MAX);

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
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

