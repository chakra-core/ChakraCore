//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDBool16x8::JavascriptSIMDBool16x8(StaticType *type) : RecyclableObject(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool16x8);
    }

    JavascriptSIMDBool16x8::JavascriptSIMDBool16x8(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool16x8);
    }

    JavascriptSIMDBool16x8* JavascriptSIMDBool16x8::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool16x8 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool16x8, requestContext->GetLibrary()->GetSIMDBool16x8TypeStatic());
    }

    JavascriptSIMDBool16x8* JavascriptSIMDBool16x8::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool16x8 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool16x8, val, requestContext->GetLibrary()->GetSIMDBool16x8TypeStatic());
    }

    bool  JavascriptSIMDBool16x8::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDBool16x8;
    }

    JavascriptSIMDBool16x8* JavascriptSIMDBool16x8::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDBool16x8'");

        return reinterpret_cast<JavascriptSIMDBool16x8 *>(aValue);
    }


    BOOL JavascriptSIMDBool16x8::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);
    }

    BOOL JavascriptSIMDBool16x8::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDBool16x8::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDBool16x8::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDBool16x8::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        if (propertyId == PropertyIds::toString)
        {
            *value = requestContext->GetLibrary()->GetSIMDBool16x8ToStringFunction();
            return true;
        }
        return false;
    }

    // Entry Points

    Var JavascriptSIMDBool16x8::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !JavascriptSIMDBool16x8::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDBool32x4.toString");
        }

        JavascriptSIMDBool16x8 *instance = JavascriptSIMDBool16x8::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[1024];
        SIMDValue value = instance->GetValue();

        swprintf_s(stringBuffer, 1024, L"SIMD.Bool16x8(%s, %s, %s, %s, %s, %s, %s, %s)", \
            value.i16[0] ? L"true" : L"false", value.i16[1] ? L"true" : L"false", value.i16[2] ? L"true" : L"false", value.i16[3] ? L"true" : L"false", \
            value.i16[4] ? L"true" : L"false", value.i16[5] ? L"true" : L"false", value.i16[6] ? L"true" : L"false", value.i16[7] ? L"true" : L"false"
            );

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    // End Entry Points

    Var JavascriptSIMDBool16x8::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool16x8::New(&this->value, requestContext);
    }
}
