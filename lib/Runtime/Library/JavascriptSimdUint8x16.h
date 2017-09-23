//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDUint8x16 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDUint8x16, JavascriptSIMDType);
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

        JavascriptSIMDUint8x16(StaticType *type);
        JavascriptSIMDUint8x16(SIMDValue *val, StaticType *type);
        static JavascriptSIMDUint8x16* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDUint8x16* FromVar(Var aValue);
        static Var CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue, const Var* args, uint numArgs, CallInfo callInfo);

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Uint8x16(%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)"),
                value.u8[0], value.u8[1], value.u8[2], value.u8[3], value.u8[4], value.u8[5], value.u8[6], value.u8[7],
                value.u8[8], value.u8[9], value.u8[10], value.u8[11], value.u8[12], value.u8[13], value.u8[14], value.u8[15]);
        }

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        inline SIMDValue GetValue() { return value; }
        static const char16* GetTypeName();
        Var  Copy(ScriptContext* requestContext);
    };
}
#endif
