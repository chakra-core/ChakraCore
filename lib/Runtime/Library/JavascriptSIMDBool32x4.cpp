//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"


namespace Js
{
    JavascriptSIMDBool32x4::JavascriptSIMDBool32x4(StaticType *type) : RecyclableObject(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool32x4);
    }

    JavascriptSIMDBool32x4::JavascriptSIMDBool32x4(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool32x4);
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool32x4, requestContext->GetLibrary()->GetSIMDBool32x4TypeStatic());
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool32x4, val, requestContext->GetLibrary()->GetSIMDBool32x4TypeStatic());
    }

    bool  JavascriptSIMDBool32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDBool32x4;
    }

    JavascriptSIMDBool32x4* JavascriptSIMDBool32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDBool32x4'");

        return reinterpret_cast<JavascriptSIMDBool32x4 *>(aValue);
    }


    BOOL JavascriptSIMDBool32x4::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);
    }

    BOOL JavascriptSIMDBool32x4::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDBool32x4::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDBool32x4::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDBool32x4::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        if (propertyId == PropertyIds::toString)
        {
            *value = requestContext->GetLibrary()->GetSIMDBool32x4ToStringFunction();
            return true;
        }
        return false;
    }

    // Entry Points

    Var JavascriptSIMDBool32x4::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !JavascriptSIMDBool32x4::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDBool32x4.toString");
        }

        JavascriptSIMDBool32x4 *instance = JavascriptSIMDBool32x4::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[1024];
        SIMDValue value = instance->GetValue();

        swprintf_s(stringBuffer, 1024, L"SIMD.Bool32x4(%s, %s, %s, %s)", value.i32[SIMD_X] ? L"true" : L"false", value.i32[SIMD_Y] ? L"true" : L"false", value.i32[SIMD_Z] ? L"true" : L"false", value.i32[SIMD_W] ? L"true" : L"false");

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    // End Entry Points

    Var JavascriptSIMDBool32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool32x4::New(&this->value, requestContext);
    }
}
