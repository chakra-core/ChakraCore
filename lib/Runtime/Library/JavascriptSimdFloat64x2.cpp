//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    JavascriptSIMDFloat64x2::JavascriptSIMDFloat64x2(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDFloat64x2);
    }

    JavascriptSIMDFloat64x2* JavascriptSIMDFloat64x2::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDFloat64x2 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDFloat64x2, val, requestContext->GetLibrary()->GetSIMDFloat64x2TypeStatic());
    }

    bool  JavascriptSIMDFloat64x2::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDFloat64x2;
    }

    JavascriptSIMDFloat64x2* JavascriptSIMDFloat64x2::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDFloat64x2'");

        return reinterpret_cast<JavascriptSIMDFloat64x2 *>(aValue);
    }

    JavascriptSIMDFloat64x2* JavascriptSIMDFloat64x2::FromFloat32x4(JavascriptSIMDFloat32x4 *instance, ScriptContext* requestContext)
    {
        SIMDValue result = SIMDFloat64x2Operation::OpFromFloat32x4(instance->GetValue());
        return JavascriptSIMDFloat64x2::New(&result, requestContext);
    }

    JavascriptSIMDFloat64x2* JavascriptSIMDFloat64x2::FromInt32x4(JavascriptSIMDInt32x4   *instance, ScriptContext* requestContext)
    {
        SIMDValue result = SIMDFloat64x2Operation::OpFromInt32x4(instance->GetValue());
        return JavascriptSIMDFloat64x2::New(&result, requestContext);
    }

    PropertyQueryFlags JavascriptSIMDFloat64x2::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetPropertyBuiltIns(propertyId, value, requestContext));
    }

    RecyclableObject * JavascriptSIMDFloat64x2::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDFloat64x2::New(&value, requestContext);
    }

    PropertyQueryFlags JavascriptSIMDFloat64x2::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return PropertyQueryFlags::Property_Found;
        }

        *value = requestContext->GetMissingPropertyResult();
        return PropertyQueryFlags::Property_NotFound;
    }

    PropertyQueryFlags JavascriptSIMDFloat64x2::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDFloat64x2::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDFloat64x2::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        switch (propertyId)
        {
        case PropertyIds::toString:
            *value = requestContext->GetLibrary()->GetSIMDFloat64x2ToStringFunction();
            return true;
        }

        *value = requestContext->GetMissingPropertyResult();
        return false;
    }

    // Entry Points

    Var JavascriptSIMDFloat64x2::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || JavascriptOperators::GetTypeId(args[0]) != TypeIds_SIMDFloat64x2)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, _u("SIMDFloat64x2.toString"));
        }

        JavascriptSIMDFloat64x2 *instance = JavascriptSIMDFloat64x2::FromVar(args[0]);
        Assert(instance);

        char16 stringBuffer[SIMD_STRING_BUFFER_MAX];
        SIMDValue value = instance->GetValue();

        JavascriptSIMDFloat64x2::ToStringBuffer(value, stringBuffer, SIMD_STRING_BUFFER_MAX);

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    // End Entry Points

    Var JavascriptSIMDFloat64x2::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDFloat64x2::New(&this->value, requestContext);
    }
}
#endif
