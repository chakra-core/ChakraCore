//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDBool8x16::JavascriptSIMDBool8x16(StaticType *type) : RecyclableObject(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool8x16);
    }

    JavascriptSIMDBool8x16::JavascriptSIMDBool8x16(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDBool8x16);
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool8x16, requestContext->GetLibrary()->GetSIMDBool8x16TypeStatic());
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDBool8x16 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDBool8x16, val, requestContext->GetLibrary()->GetSIMDBool8x16TypeStatic());
    }

    bool  JavascriptSIMDBool8x16::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDBool8x16;
    }

    JavascriptSIMDBool8x16* JavascriptSIMDBool8x16::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDBool8x16'");

        return reinterpret_cast<JavascriptSIMDBool8x16 *>(aValue);
    }

    RecyclableObject * JavascriptSIMDBool8x16::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool8x16::New(&value, requestContext);
    }

    BOOL JavascriptSIMDBool8x16::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);
    }

    BOOL JavascriptSIMDBool8x16::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDBool8x16::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDBool8x16::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDBool8x16::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        if (propertyId == PropertyIds::toString)
        {
            *value = requestContext->GetLibrary()->GetSIMDBool8x16ToStringFunction();
            return true;
        }
        return false;
    }

    // Entry Points

    Var JavascriptSIMDBool8x16::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !JavascriptSIMDBool8x16::Is(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDBool8x16.toString");
        }

        JavascriptSIMDBool8x16 *instance = JavascriptSIMDBool8x16::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[SIMD_STRING_BUFFER_MAX];
        SIMDValue value = instance->GetValue();

        JavascriptSIMDBool8x16::ToStringBuffer(value, stringBuffer, SIMD_STRING_BUFFER_MAX);

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    // End Entry Points

    Var JavascriptSIMDBool8x16::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDBool8x16::New(&this->value, requestContext);
    }
}
