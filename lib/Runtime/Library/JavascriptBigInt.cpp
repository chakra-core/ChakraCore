//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptBigInt * JavascriptBigInt::Create(const char16 * content, charcount_t cchUseLength, bool isNegative, ScriptContext * scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), JavascriptBigInt, content, cchUseLength, isNegative, scriptContext->GetLibrary()->GetBigIntTypeStatic());
    }

    Var JavascriptBigInt::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.HasArg(), "Should always have implicit 'this'");

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        Var result = nullptr;

        if (args.Info.Count > 1)
        {
            result = JavascriptConversion::ToBigInt(args[1], scriptContext);
        }
        else
        {
            // TODO:
            // v8 throw: cannot convert from undefined to bigint
            // we can consider creating a Zero BigInt
            AssertOrFailFast(false);
        }

        if (callInfo.Flags & CallFlags_New)
        {
            // TODO: handle new constructor
            // v8 throw: bigint is not a constructor
            AssertOrFailFast(false);
        }

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), UnsafeVarTo<RecyclableObject>(result), nullptr, scriptContext) :
            result;
    }

    BOOL JavascriptBigInt::Equals(Var other, BOOL* value, ScriptContext * requestContext)
    {
        return JavascriptBigInt::Equals(this, other, value, requestContext);
    }

    BOOL JavascriptBigInt::Equals(JavascriptBigInt* left, Var right, BOOL* value, ScriptContext * requestContext)
    {
        switch (JavascriptOperators::GetTypeId(right))
        {
        case TypeIds_BigInt:
            *value = JavascriptBigInt::Equals(left, right);
            break;
        default:
            AssertMsg(VarIs<JavascriptBigInt>(right), "do not support comparison with types other than BigInt");
            *value = FALSE;
            break;
        }
        return true;
    }

    template <typename EncodedChar>
    void JavascriptBigInt::InitFromCharDigits(const EncodedChar *pChar, uint32 charLength, bool isNegative)
    {
        Assert(charLength >= 0);
        Assert(pChar != 0);
        AssertMsg(charLength <= MaxStringLength, "do not support BigInt out of the range");

        const EncodedChar *pCharLimit = pChar + charLength - 1;//'n' at the end

        m_length = 0;
        m_digits = m_reservedDigitsInit;
        m_isNegative = isNegative;

        uint32 digitMul = 1;
        uint32 digitAdd = 0;
        bool check = true;
        for (; pChar < pCharLimit; pChar++)
        {
            Assert(NumberUtilities::IsDigit(*pChar));
            if (digitMul == 1e9)
            {
                check = MulThenAdd(digitMul, digitAdd);
                Assert(check);
                digitMul = 1;
                digitAdd = 0;
            }
            digitMul *= 10;
            digitAdd = digitAdd * 10 + *pChar - '0';
        }
        Assert(1 < digitMul);
        check = MulThenAdd(digitMul, digitAdd);
        Assert(check);

        // make sure this is no negative zero
        if (m_length == 0)
        {
            m_isNegative = false;
            m_length = 1;
            m_digits[0] = 0;
        }
    }

    bool JavascriptBigInt::MulThenAdd(uint32 digitMul, uint32 digitAdd)
    {
        Assert(digitMul != 0);

        uint32 carryDigit = 0;
        uint32 *pDigit = m_digits;
        uint32 *pDigitLimit = pDigit + m_length;

        for (; pDigit < pDigitLimit; pDigit++)
        {
            *pDigit = NumberUtilities::MulLu(*pDigit, digitMul, &carryDigit);// return low Digit to digit, hight Digit to carry
            if (digitAdd > 0)
            {
                carryDigit += NumberUtilities::AddLu(pDigit, digitAdd);// add carry to result
            }
            digitAdd = carryDigit;
        }
        if (0 < digitAdd) // length increase by 1
        {
            AssertOrFailFast(m_length < MaxDigitLength);// check out of bound
            m_digits[m_length++] = digitAdd;
        }
        return true;
    }

    int JavascriptBigInt::Compare(JavascriptBigInt *pbi)
    {
        if (m_isNegative != pbi->m_isNegative)
        {
            if (m_isNegative)
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }

        uint32 index;
        int sign = m_isNegative ? -1 : 1;

        if (m_length > pbi->m_length)
        {
            return 1 * sign;
        }
        if (m_length < pbi->m_length)
        {
            return -1 * sign;
        }
        if (0 == m_length)
        {
            return 0;
        }

#pragma prefast(suppress:__WARNING_LOOP_ONLY_EXECUTED_ONCE,"noise")
        for (index = m_length - 1; m_digits[index] == pbi->m_digits[index]; index--)
        {
            if (0 == index)
                return 0;
        }
        Assert(m_digits[index] != pbi->m_digits[index]);

        return sign*((m_digits[index] > pbi->m_digits[index]) ? 1 : -1);
    }

    bool JavascriptBigInt::LessThan(Var aLeft, Var aRight)
    {
        AssertMsg(VarIs<JavascriptBigInt>(aLeft) && VarIs<JavascriptBigInt>(aRight), "BigInt Equals");

        JavascriptBigInt *leftBigInt = VarTo<JavascriptBigInt>(aLeft);
        JavascriptBigInt *rightBigInt = VarTo<JavascriptBigInt>(aRight);

        return (leftBigInt->Compare(rightBigInt) < 0);
    }

    bool JavascriptBigInt::Equals(Var aLeft, Var aRight)
    {
        AssertMsg(VarIs<JavascriptBigInt>(aLeft) && VarIs<JavascriptBigInt>(aRight), "BigInt Equals");

        JavascriptBigInt *leftBigInt = VarTo<JavascriptBigInt>(aLeft);
        JavascriptBigInt *rightBigInt = VarTo<JavascriptBigInt>(aRight);

        return (leftBigInt->Compare(rightBigInt) == 0);
    }

    template void JavascriptBigInt::InitFromCharDigits<char16>(const char16 *pChar, uint32 charLength, bool isNegative);
    template void JavascriptBigInt::InitFromCharDigits<utf8char_t>(const utf8char_t *pChar, uint32 charLength, bool isNegative);

} // namespace Js
