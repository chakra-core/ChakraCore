//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    // Constructors
    JavascriptSIMDType::JavascriptSIMDType(StaticType *type) : RecyclableObject(type)
    { }
    JavascriptSIMDType::JavascriptSIMDType(SIMDValue *val, StaticType *type) : RecyclableObject(type), value(*val)
    { }

    // Entry Points
    template <typename SIMDType>
    Var JavascriptSIMDType::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !SIMDType::Is(args[0]))
        {
            char16 buffer[SIMD_STRING_BUFFER_MAX] = _u("");
            swprintf_s(buffer, SIMD_STRING_BUFFER_MAX, _u("%s.toString()"), SIMDType::GetTypeName());
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, buffer);
        }

        JavascriptSIMDType* instance = SIMDType::FromVar(args[0]);
        return JavascriptConversion::ToString(instance, scriptContext);
    }

    template<typename SIMDType>
    Var JavascriptSIMDType::EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !SIMDType::Is(args[0]))
        {
            char16 buffer[SIMD_STRING_BUFFER_MAX] = _u("");
            swprintf_s(buffer, SIMD_STRING_BUFFER_MAX, _u("%s.toLocaleString()"), SIMDType::GetTypeName());
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, buffer);
        }

        //SPEC: The meanings of the optional parameters to this method are defined in the ECMA - 402 specification; 
        //implementations that do not include ECMA - 402 support must not use those parameter positions for anything else.
        //args[1] and args[2] are optional reserved parameters.
        RecyclableObject *obj = nullptr;
        if (!JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            char16 buffer[SIMD_STRING_BUFFER_MAX] = _u("");
            swprintf_s(buffer, SIMD_STRING_BUFFER_MAX, _u("%s.toLocaleString()"), SIMDType::GetTypeName());
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedSimd, buffer);
        }

        if (JavascriptSIMDBool32x4::Is(args[0]) || JavascriptSIMDBool16x8::Is(args[0]) || JavascriptSIMDBool8x16::Is(args[0]))
        {   //Boolean types  are independent of locale.
            return JavascriptSIMDObject::FromVar(obj)->ToString(scriptContext);
        }

        //For all other SIMD types, call helper. 
        SIMDValue simdValue = SIMDType::FromVar(args[0])->GetValue();
        return SIMDType::CallToLocaleString(*obj, *scriptContext, simdValue, args.Values, args.Info.Count, callInfo);
    }

    // SIMDType.prototype[@@toPrimitive] as described in SIMD spec 
    template<typename SIMDType>
    Var JavascriptSIMDType::EntrySymbolToPrimitive(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // One argument given will be hint
        //The allowed values for hint are "default", "number", and "string"
        if (args.Info.Count == 2)
        {
            if (JavascriptString::Is(args[1]))
            {
                JavascriptString* StringObject = JavascriptString::FromVar(args[1]);

                if (wcscmp(StringObject->UnsafeGetBuffer(), _u("default")) == 0 ||
                    wcscmp(StringObject->UnsafeGetBuffer(), _u("number")) == 0 ||
                    wcscmp(StringObject->UnsafeGetBuffer(), _u("string")) == 0)
                {
                    // The hint values are validated when provided but ignored for simd types.
                    if (SIMDType::Is(args[0]))
                    {
                        return SIMDType::FromVar(args[0]);
                    }
                    else if (JavascriptSIMDObject::Is(args[0]))
                    {
                        return SIMDType::FromVar(JavascriptSIMDObject::FromVar(args[0])->GetValue());
                    }
                }
            }
        }
        char16 buffer[SIMD_STRING_BUFFER_MAX] = _u("");
        swprintf_s(buffer, SIMD_STRING_BUFFER_MAX, _u("%s.[Symbol.toPrimitive]"), SIMDType::GetTypeName());
        JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_Invalid, buffer);
    }

    template<typename SIMDType>
    Var JavascriptSIMDType::EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || SIMDType::Is(args[0]))
        {
            return SIMDType::FromVar(args[0]);
        }
        else if (JavascriptSIMDObject::Is(args[0]))
        {
            return SIMDType::FromVar((JavascriptSIMDObject::FromVar(args[0]))->GetValue());
        }
        char16 buffer[SIMD_STRING_BUFFER_MAX] = _u("");
        swprintf_s(buffer, SIMD_STRING_BUFFER_MAX, _u("%s.valueOf()"), SIMDType::GetTypeName());
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SIMDConversion, buffer);
    }
    // End Entry Points

    //Shared utility methods.
    PropertyQueryFlags JavascriptSIMDType::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(GetPropertyBuiltIns(propertyId, value, requestContext));
    }

    PropertyQueryFlags JavascriptSIMDType::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
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

    PropertyQueryFlags JavascriptSIMDType::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    bool JavascriptSIMDType::GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        *value = requestContext->GetMissingPropertyResult();
        return false;
    }

    //For all inheriting simd types.
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDBool32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDBool16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDBool8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDInt32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDInt16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDInt8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDUint32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDUint16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDUint8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToString<JavascriptSIMDFloat32x4>(RecyclableObject* function, CallInfo callInfo, ...);

    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDBool32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDBool16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDBool8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDInt32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDInt16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDInt8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDUint32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDUint16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDUint8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryToLocaleString<JavascriptSIMDFloat32x4>(RecyclableObject* function, CallInfo callInfo, ...);

    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDBool32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDBool16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDBool8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDInt32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDInt16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDInt8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDUint32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDUint16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDUint8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntrySymbolToPrimitive<JavascriptSIMDFloat32x4>(RecyclableObject* function, CallInfo callInfo, ...);

    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDBool32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDBool16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDBool8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDInt32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDInt16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDInt8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDUint32x4>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDUint16x8>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDUint8x16>(RecyclableObject* function, CallInfo callInfo, ...);
    template Var JavascriptSIMDType::EntryValueOf<JavascriptSIMDFloat32x4>(RecyclableObject* function, CallInfo callInfo, ...);
}
#endif
