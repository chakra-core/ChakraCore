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
        Field(digit_t*) m_digits;         // digits
        Field(digit_t) m_length;          // length
        Field(digit_t) m_maxLength;          // max length without resize
        Field(bool) m_isNegative;

        static const digit_t InitDigitLength = 2;  // Max Digit length 

        DEFINE_VTABLE_CTOR(JavascriptBigInt, RecyclableObject);

    public:
        JavascriptBigInt(StaticType * type)
            : RecyclableObject(type), m_length(0), m_digits(nullptr), m_isNegative(false), m_maxLength(InitDigitLength)
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
        static Var Increment(Var aRight);
        static Var Add(Var aLeft, Var aRight);
        static Var Sub(Var aLeft, Var aRight);
        static Var Decrement(Var aRight);

        inline BOOL isNegative() { return m_isNegative; }

        static JavascriptBigInt * CreateZero(ScriptContext * scriptContext);
        static JavascriptBigInt * Create(const char16 * content, charcount_t cchUseLength, bool isNegative, ScriptContext * scriptContext);
        virtual RecyclableObject * CloneToScriptContext(ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo ValueOf;
            static FunctionInfo ToString;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);

        virtual BOOL Equals(Var other, BOOL* value, ScriptContext * requestContext) override;

        static digit_t AddDigit(digit_t a, digit_t b, digit_t * carry);
        static digit_t SubDigit(digit_t a, digit_t b, digit_t * borrow);
        static digit_t MulDigit(digit_t a, digit_t b, digit_t * high);

    private:
        template <typename EncodedChar>
        void InitFromCharDigits(const EncodedChar *prgch, uint32 cch, bool isNegative); // init from char of digits

        bool MulThenAdd(digit_t luMul, digit_t luAdd);
        static bool IsZero(JavascriptBigInt * pbi);
        static void AbsoluteIncrement(JavascriptBigInt * pbi);
        static void AbsoluteDecrement(JavascriptBigInt * pbi);
        static void Increment(JavascriptBigInt * aValue);
        static void Decrement(JavascriptBigInt * pbi);
        static JavascriptBigInt * Add(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2);
        static void AddAbsolute(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2);
        static JavascriptBigInt * Sub(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2);
        static void SubAbsolute(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2);
        int Compare(JavascriptBigInt * pbi);
        int CompareAbsolute(JavascriptBigInt * pbi);
        static BOOL Equals(JavascriptBigInt* left, Var right, BOOL* value, ScriptContext * requestContext);
        void Resize(digit_t length);

        static JavascriptBigInt * New(JavascriptBigInt * pbi, ScriptContext * scriptContext);
    };
    
    template <> inline bool VarIsImpl<JavascriptBigInt>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_BigInt;
    }
}
