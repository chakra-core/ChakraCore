//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class JavascriptSIMDFloat32x4;
class JavascriptSIMDInt32x4;
class JavascriptSIMDInt16x8;

namespace Js
{
    class JavascriptSIMDInt8x16 sealed : public RecyclableObject
    {
    private:
        SIMDValue value;

        DEFINE_VTABLE_CTOR(JavascriptSIMDInt8x16, RecyclableObject);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;
        };

        JavascriptSIMDInt8x16(SIMDValue *val, StaticType *type);
        static JavascriptSIMDInt8x16* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDInt8x16* FromVar(Var aValue);

        __inline SIMDValue GetValue() { return value; }

        virtual BOOL GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        // Entry Points
        /*
        There is one toString per SIMD type. The code is entrant from value objects explicitly (e.g. a.toString()) or on overloaded operations.
        It will also be a property of SIMD.int8x16.prototype for SIMD dynamic objects.
        */

        static Var EntryToString(RecyclableObject* function, CallInfo callInfo, ...);
        // End Entry Points

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) wchar_t* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, L"SIMD.Int8x16(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
                value.i8[0], value.i8[1], value.i8[2], value.i8[3], value.i8[4], value.i8[5], value.i8[6], value.i8[7],
                value.i8[8], value.i8[9], value.i8[10], value.i8[11], value.i8[12], value.i8[13], value.i8[14], value.i8[15]);
        }

        Var  Copy(ScriptContext* requestContext);

    private:
        bool GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext);
    };
}
