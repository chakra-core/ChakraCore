//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSIMDBool16x8 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDBool16x8, JavascriptSIMDType);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ValueOf;
            static FunctionInfo SymbolToPrimitive;
        };


        JavascriptSIMDBool16x8(StaticType *type);
        JavascriptSIMDBool16x8(SIMDValue *val, StaticType *type);

        static JavascriptSIMDBool16x8* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDBool16x8* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDBool16x8* FromVar(Var aValue);
        static const wchar_t* GetFullBuiltinName(wchar_t** aBuffer, const wchar_t* name);
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool16x8, value); }
        static Var CallToLocaleString(RecyclableObject&, ScriptContext&, SIMDValue, const Var, uint, CallInfo) 
        { 
            Assert(UNREACHED);
            return nullptr;
        };
 
        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) wchar_t* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, L"SIMD.Bool16x8(%s, %s, %s, %s, %s, %s, %s, %s)", \
                value.i16[0] ? L"true" : L"false", value.i16[1] ? L"true" : L"false", value.i16[2] ? L"true" : L"false", value.i16[3] ? L"true" : L"false", \
                value.i16[4] ? L"true" : L"false", value.i16[5] ? L"true" : L"false", value.i16[6] ? L"true" : L"false", value.i16[7] ? L"true" : L"false"
                );
        }
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        __inline SIMDValue GetValue() { return value; }
        Var  Copy(ScriptContext* requestContext);
    };
}
