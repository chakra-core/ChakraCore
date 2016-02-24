//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSIMDBool16x8 sealed : public RecyclableObject
    {
    private:
        SIMDValue value;

        DEFINE_VTABLE_CTOR(JavascriptSIMDBool16x8, RecyclableObject);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;

        };


        JavascriptSIMDBool16x8(StaticType *type);
        JavascriptSIMDBool16x8(SIMDValue *val, StaticType *type);

        static JavascriptSIMDBool16x8* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDBool16x8* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDBool16x8* FromVar(Var aValue);

        __inline SIMDValue GetValue() { return value; }

        virtual BOOL GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool16x8, value); }


        // Entry Points
        /*
        There is one toString per SIMD type. The code is entrant from value objects (e.g. a.toString()) or on arithmetic operations.
        It will also be a property of SIMD.*.prototype for SIMD dynamic objects.
        */
        static Var EntryToString(RecyclableObject* function, CallInfo callInfo, ...);
        // End Entry Points

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) wchar_t* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, L"SIMD.Bool16x8(%s, %s, %s, %s, %s, %s, %s, %s)", \
                value.i16[0] ? L"true" : L"false", value.i16[1] ? L"true" : L"false", value.i16[2] ? L"true" : L"false", value.i16[3] ? L"true" : L"false", \
                value.i16[4] ? L"true" : L"false", value.i16[5] ? L"true" : L"false", value.i16[6] ? L"true" : L"false", value.i16[7] ? L"true" : L"false"
                );
        }

        Var  Copy(ScriptContext* requestContext);

    private:
        bool GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext);
    };
}
