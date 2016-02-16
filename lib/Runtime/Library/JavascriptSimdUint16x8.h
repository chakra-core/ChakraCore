//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
    class JavascriptSIMDUint16x8 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDUint16x8, JavascriptSIMDType);
    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ValueOf;
            static FunctionInfo SymbolToPrimitive;
        };

        JavascriptSIMDUint16x8(SIMDValue *val, StaticType *type);
        static JavascriptSIMDUint16x8* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDUint16x8* FromVar(Var aValue);
        static const wchar_t* GetFullBuiltinName(wchar_t** aBuffer, const wchar_t* name);
        static Var CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue, const Var* args, uint numArgs, CallInfo callInfo);

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) wchar_t* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, L"SIMD.Uint16x8(%u, %u, %u, %u, %u, %u, %u, %u)",
                value.u16[0], value.u16[1], value.u16[2], value.u16[3], value.u16[4], value.u16[5], value.u16[6], value.u16[7]);
        }

        __inline SIMDValue GetValue() { return value; }
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        Var  Copy(ScriptContext* requestContext);
    };
}
