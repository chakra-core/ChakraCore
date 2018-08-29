//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptBigInt sealed : public RecyclableObject
    {

    private:
        Field(uint32*) m_digits;         // digits
        Field(uint32) m_length;          // length
        Field(bool) m_isNegative;

        // TODO: BigInt should be arbitrary-precision, need to resize space and add tagged BigInt
        static const uint32 MaxStringLength = 100; // Max String length
        static const uint32 MaxDigitLength = 20;  // Max Digit length 
        Field(uint32) m_reservedDigitsInit[MaxDigitLength]; // pre-defined space to store array

        DEFINE_VTABLE_CTOR(JavascriptBigInt, RecyclableObject);

    public:
        JavascriptBigInt(StaticType * type)
            : RecyclableObject(type), m_length(0), m_digits(nullptr), m_isNegative(false)
        {
            Assert(type->GetTypeId() == TypeIds_BigInt);
        }

        JavascriptBigInt(const char16 * content, charcount_t cchUseLength, bool isNegative, StaticType * type)
            : JavascriptBigInt(type)
        {
            Assert(type->GetTypeId() == TypeIds_BigInt);
            InitFromCharDigits(content, cchUseLength, isNegative);
        }


        static bool LessThan(Var aLeft, Var aRight);
        static bool Equals(Var aLeft, Var aRight);

        inline BOOL isNegative() { return m_isNegative; }

        static JavascriptBigInt * Create(const char16 * content, charcount_t cchUseLength, bool isNegative, ScriptContext * scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo ValueOf;
            static FunctionInfo ToString;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        virtual BOOL Equals(Var other, BOOL* value, ScriptContext * requestContext) override;

    private:
        template <typename EncodedChar>
        void InitFromCharDigits(const EncodedChar *prgch, uint32 cch, bool isNegative); // init from char of digits

        bool MulThenAdd(uint32 luMul, uint32 luAdd);
        int Compare(JavascriptBigInt * pbi);
        static BOOL Equals(JavascriptBigInt* left, Var right, BOOL* value, ScriptContext * requestContext);
    };

    template <> inline bool VarIsImpl<JavascriptBigInt>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_BigInt;
    }
}
