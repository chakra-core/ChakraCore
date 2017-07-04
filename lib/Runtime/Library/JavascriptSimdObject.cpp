//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    JavascriptSIMDObject::JavascriptSIMDObject(DynamicType * type)
        : DynamicObject(type), value(Js::TaggedInt::ToVarUnchecked(0))
    {
        Assert(type->GetTypeId() == TypeIds_SIMDObject);
    }

    JavascriptSIMDObject::JavascriptSIMDObject(Var value,  DynamicType * type, TypeId typeDescriptor)
        : DynamicObject(type), typeDescriptor(typeDescriptor), value(value)
    {
        Assert(type->GetTypeId() == TypeIds_SIMDObject);
        switch (typeDescriptor)
        {
        //typeDescriptor is TypeIds_SIMDObject only while initializing the SIMDObject prototypes.
        //Dynamically created wrapper objects must have a concrete typeDescriptor of a SIMDType.
        case TypeIds_SIMDObject: 
            numLanes = 0;
            break;
        case TypeIds_SIMDBool8x16:
        case TypeIds_SIMDInt8x16:
        case TypeIds_SIMDUint8x16:
            numLanes = 16;
            break;
        case TypeIds_SIMDBool16x8:
        case TypeIds_SIMDInt16x8:
        case TypeIds_SIMDUint16x8:
            numLanes = 8;
            break;
        case TypeIds_SIMDBool32x4:
        case TypeIds_SIMDInt32x4:
        case TypeIds_SIMDUint32x4:
        case TypeIds_SIMDFloat32x4:
            numLanes = 4;
            break;
        default:
            Assert(UNREACHED);
        }
    }
    void JavascriptSIMDObject::SetTypeDescriptor(TypeId tid)
    {
        Assert(tid != TypeIds_SIMDObject);

        typeDescriptor = tid;
        switch (typeDescriptor)
        {
        case TypeIds_SIMDBool8x16:
        case TypeIds_SIMDInt8x16:
        case TypeIds_SIMDUint8x16:
            numLanes = 16;
            break;
        case TypeIds_SIMDBool16x8:
        case TypeIds_SIMDInt16x8:
        case TypeIds_SIMDUint16x8:
            numLanes = 8;
            break;
        case TypeIds_SIMDBool32x4:
        case TypeIds_SIMDInt32x4:
        case TypeIds_SIMDUint32x4:
        case TypeIds_SIMDFloat32x4:
            numLanes = 4;
            break;
        default:
            Assert(UNREACHED);
        }
    }

    bool JavascriptSIMDObject::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_SIMDObject;
    }

    JavascriptSIMDObject* JavascriptSIMDObject::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptSIMD'");

        return static_cast<JavascriptSIMDObject *>(RecyclableObject::FromVar(aValue));
    }

    Var JavascriptSIMDObject::Unwrap() const
    {        
        return value;
    }

    Var JavascriptSIMDObject::ToString(ScriptContext* scriptContext) const
    {
        Assert(scriptContext);
        Assert(typeDescriptor != TypeIds_SIMDObject);

        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("fromCodePoint"));
        char16* stringBuffer = AnewArray(tempAllocator, char16, SIMD_STRING_BUFFER_MAX);
        SIMDValue simdValue;
        switch (typeDescriptor)
        {
        case TypeIds_SIMDBool8x16:
            simdValue = JavascriptSIMDBool8x16::FromVar(value)->GetValue();
            JavascriptSIMDBool8x16::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDInt8x16:
            simdValue = JavascriptSIMDInt8x16::FromVar(value)->GetValue();
            JavascriptSIMDInt8x16::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDUint8x16:
            simdValue = JavascriptSIMDUint8x16::FromVar(value)->GetValue();
            JavascriptSIMDUint8x16::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDBool16x8:
            simdValue = JavascriptSIMDBool16x8::FromVar(value)->GetValue();
            JavascriptSIMDBool16x8::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDInt16x8:
            simdValue = JavascriptSIMDInt16x8::FromVar(value)->GetValue();
            JavascriptSIMDInt16x8::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDUint16x8:
            simdValue = JavascriptSIMDUint16x8::FromVar(value)->GetValue();
            JavascriptSIMDUint16x8::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDBool32x4:
            simdValue = JavascriptSIMDBool32x4::FromVar(value)->GetValue();
            JavascriptSIMDBool32x4::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDInt32x4:
            simdValue = JavascriptSIMDInt32x4::FromVar(value)->GetValue();
            JavascriptSIMDInt32x4::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDUint32x4:
            simdValue = JavascriptSIMDUint32x4::FromVar(value)->GetValue();
            JavascriptSIMDUint32x4::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        case TypeIds_SIMDFloat32x4:
            simdValue = JavascriptSIMDFloat32x4::FromVar(value)->GetValue();
            JavascriptSIMDFloat32x4::ToStringBuffer(simdValue, stringBuffer, SIMD_STRING_BUFFER_MAX, scriptContext);
            break;
        default:
            Assert(UNREACHED);
        }
        JavascriptString* string = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());
        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);
        return string;
    }

    template <typename T, size_t N>
    Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString, const T (&laneValues)[N], 
        CallInfo* callInfo, ScriptContext* scriptContext) const
    {
        Assert(args);
        Assert(N == 4 || N == 8 || N == 16);
        if (typeDescriptor == TypeIds_SIMDBool8x16 ||
            typeDescriptor == TypeIds_SIMDBool16x8 ||
            typeDescriptor == TypeIds_SIMDBool32x4)
        {
            return ToString(scriptContext);   //Boolean types does not have toLocaleString.
        }

        // Clamp to the first 3 arguments - we'll ignore more.
        if (numArgs > 3)
        {
            numArgs = 3;
        }

        // Creating a new arguments list for the JavascriptNumber generated from each lane.The optional SIMDToLocaleString Args are
        //added to this argument list. 
        Var newArgs[3] = { nullptr, nullptr, nullptr };
        CallInfo newCallInfo((ushort)numArgs);

        if (numArgs > 1)
        {
            newArgs[1] = args[1];
        }
        if (numArgs > 2)
        {
            newArgs[2] = args[2];
        }

        //Locale specifc seperator?? 
        JavascriptString *seperator = JavascriptString::NewWithSz(_u(", "), scriptContext);
        uint idx = 0;
        Var laneVar = nullptr;
        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("fromCodePoint"));
        char16* stringBuffer = AnewArray(tempAllocator, char16, SIMD_STRING_BUFFER_MAX);
        JavascriptString *result = nullptr;

        swprintf_s(stringBuffer, SIMD_STRING_BUFFER_MAX, typeString);
        result = JavascriptString::NewCopySzFromArena(stringBuffer, scriptContext, scriptContext->GeneralAllocator());

        if (typeDescriptor == TypeIds_SIMDFloat32x4)
        {
            for (; idx < numLanes - 1; ++idx)
            {
                laneVar = JavascriptNumber::ToVarWithCheck(laneValues[idx], scriptContext);
                newArgs[0] = laneVar;
                JavascriptString *laneValue = JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext);
                result = JavascriptString::Concat(result, laneValue);
                result = JavascriptString::Concat(result, seperator);
            }
            laneVar = JavascriptNumber::ToVarWithCheck(laneValues[idx], scriptContext);
            newArgs[0] = laneVar;
            result = JavascriptString::Concat(result, JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext));
        }
        else if (typeDescriptor == TypeIds_SIMDInt8x16 || typeDescriptor == TypeIds_SIMDInt16x8 || typeDescriptor == TypeIds_SIMDInt32x4)
        {
            for (; idx < numLanes - 1; ++idx)
            {
                laneVar = JavascriptNumber::ToVar(static_cast<int>(laneValues[idx]), scriptContext); 
                newArgs[0] = laneVar;
                JavascriptString *laneValue = JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext);
                result = JavascriptString::Concat(result, laneValue);
                result = JavascriptString::Concat(result, seperator);
            }
            laneVar = JavascriptNumber::ToVar(static_cast<int>(laneValues[idx]), scriptContext);
            newArgs[0] = laneVar;
            result = JavascriptString::Concat(result, JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext));
        }
        else
        {
            Assert((typeDescriptor == TypeIds_SIMDUint8x16 || typeDescriptor == TypeIds_SIMDUint16x8 || typeDescriptor == TypeIds_SIMDUint32x4));
            for (; idx < numLanes - 1; ++idx)
            {
                laneVar = JavascriptNumber::ToVar(static_cast<uint>(laneValues[idx]), scriptContext); 
                newArgs[0] = laneVar;
                JavascriptString *laneValue = JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext);
                result = JavascriptString::Concat(result, laneValue);
                result = JavascriptString::Concat(result, seperator);
            }
            laneVar = JavascriptNumber::ToVar(static_cast<uint>(laneValues[idx]), scriptContext);
            newArgs[0] = laneVar;
            result = JavascriptString::Concat(result, JavascriptNumber::ToLocaleStringIntl(newArgs, newCallInfo, scriptContext));
        }
        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);
        return JavascriptString::Concat(result, JavascriptString::NewWithSz(_u(")"), scriptContext));
    }

    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString, 
        const float (&laneValues)[4], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const int(&laneValues)[4], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const int16(&laneValues)[8], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const int8(&laneValues)[16], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const uint(&laneValues)[4], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const uint16(&laneValues)[8], CallInfo* callInfo, ScriptContext* scriptContext) const;
    template Var JavascriptSIMDObject::ToLocaleString(const Var* args, uint numArgs, const char16 *typeString,
        const uint8(&laneValues)[16], CallInfo* callInfo, ScriptContext* scriptContext) const;

    Var JavascriptSIMDObject::GetValue() const
    {
        Assert(SIMDUtils::IsSimdType(value));
        return value;
    }
}
#endif
