//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDBool8x16 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDBool8x16, JavascriptSIMDType);
        static const char16 TypeName[];
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
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool8x16, value); }
        static Var CallToLocaleString(RecyclableObject&, ScriptContext&, SIMDValue, const Var, uint, CallInfo)
        {
            Assert(UNREACHED);
            return nullptr;
        };

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        static const char16* GetTypeName();
        inline SIMDValue GetValue() { return value; }

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Bool8x16(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)"), \
                value.i8[0] ? _u("true") : _u("false"), value.i8[1] ? _u("true") : _u("false"), value.i8[2] ? _u("true") : _u("false"), value.i8[3] ? _u("true") : _u("false"), \
                value.i8[4] ? _u("true") : _u("false"), value.i8[5] ? _u("true") : _u("false"), value.i8[6] ? _u("true") : _u("false"), value.i8[7] ? _u("true") : _u("false"), \
                value.i8[8] ? _u("true") : _u("false"), value.i8[9] ? _u("true") : _u("false"), value.i8[10] ? _u("true") : _u("false"), value.i8[11] ? _u("true") : _u("false"), \
                value.i8[12] ? _u("true") : _u("false"), value.i8[13] ? _u("true") : _u("false"), value.i8[14] ? _u("true") : _u("false"), value.i8[15] ? _u("true") : _u("false")
                );
        }

        Var  Copy(ScriptContext* requestContext);
    };
}
#endif
