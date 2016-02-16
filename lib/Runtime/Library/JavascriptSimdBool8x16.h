//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSIMDBool8x16 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDBool8x16, JavascriptSIMDType);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ValueOf;
            static FunctionInfo SymbolToPrimitive;
        };
        JavascriptSIMDBool8x16(StaticType *type);
        JavascriptSIMDBool8x16(SIMDValue *val, StaticType *type);

        static JavascriptSIMDBool8x16* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDBool8x16* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDBool8x16* FromVar(Var aValue);
        static const wchar_t* GetFullBuiltinName(wchar_t** aBuffer, const wchar_t* name);
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool8x16, value); }
        static Var CallToLocaleString(RecyclableObject&, ScriptContext&, SIMDValue, const Var, uint, CallInfo)
        {
            Assert(UNREACHED);
            return nullptr;
        };

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        __inline SIMDValue GetValue() { return value; }

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) wchar_t* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, L"SIMD.Bool8x16(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)", \
                value.i8[0] ? L"true" : L"false", value.i8[1] ? L"true" : L"false", value.i8[2] ? L"true" : L"false", value.i8[3] ? L"true" : L"false", \
                value.i8[4] ? L"true" : L"false", value.i8[5] ? L"true" : L"false", value.i8[6] ? L"true" : L"false", value.i8[7] ? L"true" : L"false", \
                value.i8[8] ? L"true" : L"false", value.i8[9] ? L"true" : L"false", value.i8[10] ? L"true" : L"false", value.i8[11] ? L"true" : L"false", \
                value.i8[12] ? L"true" : L"false", value.i8[13] ? L"true" : L"false", value.i8[14] ? L"true" : L"false", value.i8[15] ? L"true" : L"false"
                );
        }

        Var  Copy(ScriptContext* requestContext);
    };
}

