//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDBool32x4 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDBool32x4, JavascriptSIMDType);
        static const char16 TypeName[];
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ValueOf;
            static FunctionInfo ToString;
            static FunctionInfo ToLocaleString;
            static FunctionInfo SymbolToPrimitive;
        };

        static bool Is(Var instance);
        static JavascriptSIMDBool32x4* FromVar(Var aValue);
        static JavascriptSIMDBool32x4* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDBool32x4* New(SIMDValue *val, ScriptContext* requestContext);
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDBool32x4, value); }
        static Var CallToLocaleString(RecyclableObject&, ScriptContext&, SIMDValue, const Var, uint, CallInfo) 
        {
            Assert(UNREACHED);
            return nullptr;
        };
        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Bool32x4(%s, %s, %s, %s)"), value.i32[SIMD_X] ? _u("true") : _u("false"),
                value.i32[SIMD_Y] ? _u("true") : _u("false"), value.i32[SIMD_Z] ? _u("true") : _u("false"), value.i32[SIMD_W] ? _u("true") : _u("false"));
        }

        JavascriptSIMDBool32x4(StaticType *type);
        JavascriptSIMDBool32x4(SIMDValue *val, StaticType *type);

        Var Copy(ScriptContext* requestContext);
        static const char16* GetTypeName();
        inline SIMDValue GetValue() { return value; }
        virtual RecyclableObject* CloneToScriptContext(ScriptContext* requestContext) override;

    };
}
#endif

