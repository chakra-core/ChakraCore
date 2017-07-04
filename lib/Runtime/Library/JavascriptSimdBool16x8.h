//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDBool16x8 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDBool16x8, JavascriptSIMDType);
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


        JavascriptSIMDBool16x8(StaticType *type);
        JavascriptSIMDBool16x8(SIMDValue *val, StaticType *type);

        static JavascriptSIMDBool16x8* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDBool16x8* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDBool16x8* FromVar(Var aValue);
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool16x8, value); }
        static Var CallToLocaleString(RecyclableObject&, ScriptContext&, SIMDValue, const Var, uint, CallInfo) 
        { 
            Assert(UNREACHED);
            return nullptr;
        };
 
        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Bool16x8(%s, %s, %s, %s, %s, %s, %s, %s)"), \
                value.i16[0] ? _u("true") : _u("false"), value.i16[1] ? _u("true") : _u("false"), value.i16[2] ? _u("true") : _u("false"), value.i16[3] ? _u("true") : _u("false"), \
                value.i16[4] ? _u("true") : _u("false"), value.i16[5] ? _u("true") : _u("false"), value.i16[6] ? _u("true") : _u("false"), value.i16[7] ? _u("true") : _u("false")
                );
        }
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        static const char16* GetTypeName();
        inline SIMDValue GetValue() { return value; }
        Var  Copy(ScriptContext* requestContext);
    };
}
#endif
