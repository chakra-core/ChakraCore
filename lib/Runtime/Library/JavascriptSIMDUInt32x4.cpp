//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptSIMDUint32x4::JavascriptSIMDUint32x4(StaticType *type) : RecyclableObject(type)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint32x4);
    }

    JavascriptSIMDUint32x4::JavascriptSIMDUint32x4(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDUint32x4);
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::AllocUninitialized(ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint32x4, requestContext->GetLibrary()->GetSIMDUInt32x4TypeStatic());
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::New(SIMDValue *val, ScriptContext* requestContext)
    {
        return (JavascriptSIMDUint32x4 *)AllocatorNew(Recycler, requestContext->GetRecycler(), JavascriptSIMDUint32x4, val, requestContext->GetLibrary()->GetSIMDUInt32x4TypeStatic());
    }

    bool  JavascriptSIMDUint32x4::Is(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_SIMDUint32x4;
    }

    JavascriptSIMDUint32x4* JavascriptSIMDUint32x4::FromVar(Var aValue)
    {
        Assert(aValue);
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMDUint32x4'");

        return reinterpret_cast<JavascriptSIMDUint32x4 *>(aValue);
    }

    BOOL JavascriptSIMDUint32x4::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyBuiltIns(propertyId, value, requestContext);

    }

    BOOL JavascriptSIMDUint32x4::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, requestContext))
        {
            return true;
        }
        return false;
    }

    BOOL JavascriptSIMDUint32x4::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptSIMDUint32x4::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDUint32x4::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        if (propertyId == PropertyIds::toString)
        {
            *value = requestContext->GetLibrary()->GetSIMDUint32x4ToStringFunction();
            return true;
        }

        return false;
    }

    Var JavascriptSIMDUint32x4::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || JavascriptOperators::GetTypeId(args[0]) != TypeIds_SIMDUint32x4)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, L"SIMDUInt32x4.toString");
        }

        JavascriptSIMDUint32x4* instance = JavascriptSIMDUint32x4::FromVar(args[0]);
        Assert(instance);

        wchar_t stringBuffer[1024];
        SIMDValue value = instance->GetValue();

        swprintf_s(stringBuffer, 1024, L"SIMD.Uint32x4(%u, %u, %u, %u)", value.u32[SIMD_X], value.u32[SIMD_Y], value.u32[SIMD_Z], value.u32[SIMD_W]);

        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        return string;
    }

    Var JavascriptSIMDUint32x4::Copy(ScriptContext* requestContext)
    {
        return JavascriptSIMDUint32x4::New(&this->value, requestContext);
    }

}
