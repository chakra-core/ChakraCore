//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSIMDUint32x4;

    class JavascriptSIMDInt32x4 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDInt32x4, JavascriptSIMDType);

    public:
        class EntryInfo
        {
        public:
            static FunctionInfo ToString;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ValueOf;
            static FunctionInfo SymbolToPrimitive;
            static FunctionInfo Bool;
        };

        JavascriptSIMDInt32x4(StaticType *type);
        JavascriptSIMDInt32x4(SIMDValue *val, StaticType *type);

        static JavascriptSIMDInt32x4* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDInt32x4* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDInt32x4* FromVar(Var aValue);
        static const char16* GetFullBuiltinName(char16** aBuffer, const char16* name);

        static JavascriptSIMDInt32x4* FromBool(SIMDValue *val, ScriptContext* requestContext);
        static JavascriptSIMDInt32x4* FromFloat64x2(JavascriptSIMDFloat64x2 *instance, ScriptContext* requestContext);
        static JavascriptSIMDInt32x4* FromFloat32x4(JavascriptSIMDFloat32x4   *instance, ScriptContext* requestContext);
        static Var CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue, const Var* args, uint numArgs, CallInfo callInfo);

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Int32x4(%d, %d, %d, %d)"), value.i32[SIMD_X], value.i32[SIMD_Y], value.i32[SIMD_Z], value.i32[SIMD_W]);
        }
        __inline SIMDValue GetValue() { return value; }

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDInt32x4, value); }

        Var  Copy(ScriptContext* requestContext);

    private:
        virtual bool GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext) override;
    };
}
