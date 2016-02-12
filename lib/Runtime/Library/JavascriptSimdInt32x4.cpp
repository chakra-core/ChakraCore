//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// Copyright (C) 2016 Intel Corporation.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDInt32x4::JavascriptSIMDInt32x4(StaticType *type) : RecyclableObject(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt32x4);
    }

    JavascriptSIMDInt32x4::JavascriptSIMDInt32x4(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDInt32x4);
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt32x4, requestContext->GetLibrary()->GetSIMDInt32x4TypeStatic());
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDInt32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDInt32x4, val, requestContext->GetLibrary()->GetSIMDInt32x4TypeStatic());
    }

    bool  JavascriptSIMDInt32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDInt32x4;
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDInt32x4'");

        return reinterpret_cast<JavascriptSIMDInt32x4 *>(aValue);
    }

    JavascriptSIMDInt32x4* JavascriptSIMDInt32x4::FromFloat64x2(JavascriptSIMDFloat64x2 *instance, ScriptContext* requestContext)
    {
        SIMDValue result = SIMDInt32x4Operation::OpFromFloat64x2(instance->GetValue());
        return JavascriptSIMDInt32x4::New(&result, requestContext);
    }

    RecyclableObject * JavascriptSIMDInt32x4::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::New(&value, requestContext);
    }

    BOOL JavascriptSIMDInt32x4::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);

    }

    BOOL JavascriptSIMDInt32x4::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDInt32x4::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDInt32x4::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        switch (propertyId)
        {
        case PropertyIds::toString:
            *value = requestContext->GetLibrary()->GetSIMDInt32x4ToStringFunction();
            return true;
        }

        return false;
    }

    Var JavascriptSIMDInt32x4::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || JavascriptOperators::GetTypeId(args[0]) != TypeIds_SIMDInt32x4)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDInt32x4.toString");
        }

        JavascriptSIMDInt32x4* instance = JavascriptSIMDInt32x4::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[1024];
        SIMDValue value = instance->GetValue();

        swprintf_s(stringBuffer, 1024, L"SIMD.Int32x4(%d, %d, %d, %d)", value.i32[SIMD_X], value.i32[SIMD_Y], value.i32[SIMD_Z], value.i32[SIMD_W]);

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    Var JavascriptSIMDInt32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDInt32x4::New(&this->value, requestContext);
    }
}
