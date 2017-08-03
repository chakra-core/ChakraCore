//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
class JavascriptSIMDFloat32x4;

namespace Js
{
    class JavascriptSIMDUint32x4 sealed : public JavascriptSIMDType
    {
    private:
        DEFINE_VTABLE_CTOR(JavascriptSIMDUint32x4, JavascriptSIMDType);
        static const char16 TypeName[];
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

        JavascriptSIMDUint32x4(StaticType *type);
        JavascriptSIMDUint32x4(SIMDValue *val, StaticType *type);

        static JavascriptSIMDUint32x4* AllocUninitialized(ScriptContext* requestContext);
        static JavascriptSIMDUint32x4* New(SIMDValue *val, ScriptContext* requestContext);
        static bool Is(Var instance);
        static JavascriptSIMDUint32x4* FromVar(Var aValue);
        static JavascriptSIMDUint32x4* FromFloat32x4(JavascriptSIMDFloat32x4   *instance, ScriptContext* requestContext);
        static size_t GetOffsetOfValue() { return offsetof(JavascriptSIMDUint32x4, value); }
        static Var CallToLocaleString(RecyclableObject& obj, ScriptContext& requestContext, SIMDValue simdValue, const Var* args, uint numArgs, CallInfo callInfo);

        static void ToStringBuffer(SIMDValue& value, __out_ecount(countBuffer) char16* stringBuffer, size_t countBuffer, ScriptContext* scriptContext = nullptr)
        {
            swprintf_s(stringBuffer, countBuffer, _u("SIMD.Uint32x4(%u, %u, %u, %u)"), value.u32[SIMD_X], value.u32[SIMD_Y], value.u32[SIMD_Z], value.u32[SIMD_W]);
        }

        static const char16* GetTypeName();
        inline SIMDValue GetValue() { return value; }

        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        Var  Copy(ScriptContext* requestContext);
    };
}
#endif
