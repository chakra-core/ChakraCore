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

    JavascriptBigInt * JavascriptBigInt::CreateZero(ScriptContext * scriptContext)
    {
        JavascriptBigInt * bigintNew = RecyclerNew(scriptContext->GetRecycler(), JavascriptBigInt, scriptContext->GetLibrary()->GetBigIntTypeStatic());
        bigintNew->m_length = 1;
        bigintNew->m_isNegative = false;
        bigintNew->m_maxLength = 1;
        bigintNew->m_digits = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), digit_t, bigintNew->m_length);
        bigintNew->m_digits[0] = 0;
        
        return bigintNew;
    }

    JavascriptBigInt * JavascriptBigInt::New(JavascriptBigInt * pbi, ScriptContext * scriptContext)
    {
        JavascriptBigInt * bigintNew = RecyclerNew(scriptContext->GetRecycler(), JavascriptBigInt, scriptContext->GetLibrary()->GetBigIntTypeStatic());
        bigintNew->m_length = pbi->m_length;
        bigintNew->m_maxLength = pbi->m_maxLength;
        bigintNew->m_isNegative = pbi->m_isNegative;
        bigintNew->m_digits = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), digit_t, pbi->m_length);
        js_memcpy_s(bigintNew->m_digits, bigintNew->m_length * sizeof(digit_t), pbi->m_digits, bigintNew->m_length * sizeof(digit_t));
 
        return bigintNew;
    }

    RecyclableObject * JavascriptBigInt::CloneToScriptContext(ScriptContext* requestContext)
    {
        return JavascriptBigInt::New(this, requestContext);
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

    void JavascriptBigInt::Resize(digit_t length)
    {
        digit_t *digits;

        if (length <= m_maxLength)
        {
            return;
        }

        length += length;// double size
        if (SIZE_MAX / sizeof(digit_t) < length) // overflow 
        {
            JavascriptError::ThrowRangeError(this->GetScriptContext(), VBSERR_TypeMismatch, _u("Resize BigInt"));
        }

        digits = RecyclerNewArrayLeaf(this->GetScriptContext()->GetRecycler(), digit_t, length);
        if (NULL == digits)
        {
            JavascriptError::ThrowRangeError(this->GetScriptContext(), VBSERR_TypeMismatch, _u("Resize BigInt"));
        }

        if (0 < m_length) // in this case, we need to copy old data over
        {
            js_memcpy_s(digits, length * sizeof(digit_t), m_digits, m_length * sizeof(digit_t));
        }

        m_digits = digits;
        m_maxLength = length;
    }

    template <typename EncodedChar>
    void JavascriptBigInt::InitFromCharDigits(const EncodedChar *pChar, uint32 charLength, bool isNegative)
    {
        Assert(charLength >= 0);
        Assert(pChar != 0);

        const EncodedChar *pCharLimit = pChar + charLength - 1;//'n' at the end

        m_length = 0;
        m_digits = RecyclerNewArrayLeaf(this->GetScriptContext()->GetRecycler(), digit_t, m_maxLength);
        m_isNegative = isNegative;

        digit_t digitMul = 1;
        digit_t digitAdd = 0;
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

    // return low(a+b) and out carry
    digit_t JavascriptBigInt::AddDigit(digit_t a, digit_t b, digit_t* carry)
    {
        digit_t result = a + b;
        if (result < a)
        {
            *carry += 1;
        }
        return result;
    }

    // return low(a+b) and out carry
    digit_t JavascriptBigInt::SubDigit(digit_t a, digit_t b, digit_t* borrow)
    {
        digit_t result = a - b;
        if (result > a)
        {
            *borrow += 1;
        }
        return result;
    }

    bool JavascriptBigInt::IsZero(JavascriptBigInt * pbi)
    {
        return (pbi->m_length == 1 && pbi->m_digits[0] == 0);
    }

    void JavascriptBigInt::AbsoluteIncrement(JavascriptBigInt * pbi)
    {
        JavascriptBigInt* result = pbi;
        digit_t carry = 1;
        for (digit_t i = 0; i < result->m_length && carry > 0; i++)
        {
            digit_t tempCarry = 0;
            result->m_digits[i] = JavascriptBigInt::AddDigit(result->m_digits[i], carry, &tempCarry);
            carry = tempCarry;
        }
        if (carry > 0) //increase length
        {
            if (result->m_length >= result->m_maxLength)
            {
                result->Resize(result->m_length + 1);
            }
            result->m_digits[result->m_length++] = carry;
        }
    }

    void JavascriptBigInt::AbsoluteDecrement(JavascriptBigInt * pbi)
    {
        JavascriptBigInt* result = pbi;
        Assert(!JavascriptBigInt::IsZero(result));
        digit_t borrow = 1;
        for (digit_t i = 0; i < result->m_length && borrow > 0; i++)
        {
            digit_t tempBorrow = 0;
            result->m_digits[i] = JavascriptBigInt::SubDigit(result->m_digits[i], borrow, &tempBorrow);
            borrow = tempBorrow;
        }
        Assert(borrow == 0);
        // remove trailing zero
        if (result->m_digits[result->m_length-1] == 0)
        {
            result->m_length--;
        }
    }

    void JavascriptBigInt::Increment(JavascriptBigInt * pbi)
    {
        if (pbi->m_isNegative)
        {
            // return 0n for -1n
            if (pbi->m_length == 1 && pbi->m_digits[0] == 1)
            {
                JavascriptBigInt* result = pbi;
                result->m_digits[0] = 0;
                result->m_isNegative = false;
                return;
            }
            return JavascriptBigInt::AbsoluteDecrement(pbi);
        }
        return JavascriptBigInt::AbsoluteIncrement(pbi);
    }

    void JavascriptBigInt::Decrement(JavascriptBigInt * pbi)
    {
        if (pbi->m_isNegative)
        {
            return JavascriptBigInt::AbsoluteIncrement(pbi);
        }
        if (JavascriptBigInt::IsZero(pbi)) // return -1n for 0n
        {
            JavascriptBigInt* result = pbi;
            result->m_digits[0] = 1;
            result->m_isNegative = true;
            return;
        }
        return JavascriptBigInt::AbsoluteDecrement(pbi);
    }

    Var JavascriptBigInt::Increment(Var aRight)
    {
        JavascriptBigInt* rightBigInt = VarTo<JavascriptBigInt>(aRight);
        JavascriptBigInt* newBigInt = JavascriptBigInt::New(rightBigInt, rightBigInt->GetScriptContext());
        JavascriptBigInt::Increment(newBigInt);
        return newBigInt;
    }

    Var JavascriptBigInt::Decrement(Var aRight)
    {
        JavascriptBigInt* rightBigInt = VarTo<JavascriptBigInt>(aRight);
        JavascriptBigInt* newBigInt = JavascriptBigInt::New(rightBigInt, rightBigInt->GetScriptContext());
        JavascriptBigInt::Decrement(newBigInt);
        return newBigInt;
    }

    // return low(a*b) and out high
    digit_t JavascriptBigInt::MulDigit(digit_t a, digit_t b, digit_t* resultHigh)
    {
        // Multiply is performed in half chuck favor.
        // For inputs [AH AL]*[BH BL], the result is:
        //
        //            [AL*BL]  // rLow
        //    +    [AL*BH]     // rMid1
        //    +    [AH*BL]     // rMid2
        //    + [AH*BH]        // rHigh
        //    = [R1 R2 R3 R4]  // high = [R1 R2], low = [R3 R4]
        //

        digit_t kHalfDigitBits = sizeof(digit_t) * 4;
        digit_t kHalfDigitMask = ((digit_t)1 << kHalfDigitBits) - 1;

        digit_t aLow = a & kHalfDigitMask;
        digit_t aHigh = a >> kHalfDigitBits;
        digit_t bLow = b & kHalfDigitMask;
        digit_t bHigh = b >> kHalfDigitBits;

        digit_t rLow = aLow * bLow;
        digit_t rMid1 = aLow * bHigh;
        digit_t rMid2 = aHigh * bLow;
        digit_t rHigh = aHigh * bHigh;

        digit_t carry = 0;
        digit_t resultLow = JavascriptBigInt::AddDigit(rLow, rMid1 << kHalfDigitBits, &carry);
        resultLow = JavascriptBigInt::AddDigit(resultLow, rMid2 << kHalfDigitBits, &carry);
        *resultHigh = (rMid1 >> kHalfDigitBits) + (rMid2 >> kHalfDigitBits) + rHigh + carry;
        return resultLow;
    }

    bool JavascriptBigInt::MulThenAdd(digit_t digitMul, digit_t digitAdd)
    {
        Assert(digitMul != 0);

        digit_t carryDigit = 0;
        digit_t *pDigit = m_digits;
        digit_t *pDigitLimit = pDigit + m_length;

        for (; pDigit < pDigitLimit; pDigit++)
        {
            *pDigit = JavascriptBigInt::MulDigit(*pDigit, digitMul, &carryDigit);// return low Digit to digit, hight Digit to carry
            if (digitAdd > 0)
            {
                *pDigit = JavascriptBigInt::AddDigit(*pDigit, digitAdd, &carryDigit);// add carry to result
            }
            digitAdd = carryDigit;
        }
        if (0 < digitAdd) // length increase by 1
        {
            if (m_length >= m_maxLength)
            {
                Resize(m_length + 1);
            }
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

        int sign = m_isNegative ? -1 : 1;
        return sign * JavascriptBigInt::CompareAbsolute(pbi);
    }

    int JavascriptBigInt::CompareAbsolute(JavascriptBigInt *pbi)
    {
        digit_t index;

        if (m_length > pbi->m_length)
        {
            return 1;
        }
        if (m_length < pbi->m_length)
        {
            return -1;
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

        return (m_digits[index] > pbi->m_digits[index]) ? 1 : -1;
    }

    bool JavascriptBigInt::LessThan(Var aLeft, Var aRight)
    {
        AssertMsg(VarIs<JavascriptBigInt>(aLeft) && VarIs<JavascriptBigInt>(aRight), "BigInt LessThan");

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

    // pbi1 += pbi2 assume pbi1 has length no less than pbi2
    void JavascriptBigInt::AddAbsolute(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2)
    {
        Assert(pbi1->m_length >= pbi2->m_length);
        digit_t carryDigit = 0;
        digit_t *pDigit1 = pbi1->m_digits;
        digit_t *pDigit2 = pbi2->m_digits;
        digit_t i = 0;

        for (; i < pbi2->m_length; i++)
        {
            digit_t tempCarryDigit = 0;
            pDigit1[i] = JavascriptBigInt::AddDigit(pDigit1[i], pDigit2[i], &tempCarryDigit);
            pDigit1[i] = JavascriptBigInt::AddDigit(pDigit1[i], carryDigit, &tempCarryDigit);
            carryDigit = tempCarryDigit;
        }

        for (; i < pbi1->m_length && carryDigit > 0; i++)
        {
            digit_t tempCarryDigit = 0;
            pDigit1[i] = JavascriptBigInt::AddDigit(pDigit1[i], carryDigit, &tempCarryDigit);
            carryDigit = tempCarryDigit;
        }

        if (0 < carryDigit) // length increase by 1
        {
            if (pbi1->m_length >= pbi1->m_maxLength)
            {
                pbi1->Resize(pbi1->m_length + 1);
            }
            pbi1->m_digits[pbi1->m_length++] = carryDigit;
        }
    }

    // pbi1 -= pbi2 assume |pbi1| > |pbi2|
    void JavascriptBigInt::SubAbsolute(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2)
    {
        Assert(pbi1->CompareAbsolute(pbi2) == 1);
        digit_t carryDigit = 0;
        digit_t *pDigit1 = pbi1->m_digits;
        digit_t *pDigit2 = pbi2->m_digits;
        digit_t i = 0;

        for (; i < pbi2->m_length; i++)
        {
            digit_t tempCarryDigit = 0;
            pDigit1[i] = JavascriptBigInt::SubDigit(pDigit1[i], pDigit2[i], &tempCarryDigit);
            pDigit1[i] = JavascriptBigInt::SubDigit(pDigit1[i], carryDigit, &tempCarryDigit);
            carryDigit = tempCarryDigit;
        }

        for (; i < pbi1->m_length && carryDigit > 0; i++)
        {
            digit_t tempCarryDigit = 0;
            pDigit1[i] = JavascriptBigInt::SubDigit(pDigit1[i], carryDigit, &tempCarryDigit);
            carryDigit = tempCarryDigit;
        }
        // adjust length
        while ((pbi1->m_length>0) && (pbi1->m_digits[pbi1->m_length-1] == 0))
        {
            pbi1->m_length--;
        }
        Assert(pbi1->m_length > 0);
    }

    JavascriptBigInt * JavascriptBigInt::Sub(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2)
    {
        if (JavascriptBigInt::IsZero(pbi1))
        {
            pbi2->m_isNegative = !pbi2->m_isNegative;
            return pbi2;
        }
        if (JavascriptBigInt::IsZero(pbi2))
        {
            return pbi1;
        }

        if (pbi2->m_isNegative)
        {
            pbi2->m_isNegative = false;
            return JavascriptBigInt::Add(pbi1, pbi2);// a-(-b)=a+b
        }

        if (pbi1->m_isNegative) // -a-b=-(a+b)
        {
            if (pbi1->m_length >= pbi2->m_length)
            {
                JavascriptBigInt::AddAbsolute(pbi1, pbi2);
                return pbi1;
            }
            JavascriptBigInt::AddAbsolute(pbi2, pbi1);
            return pbi2;
        }
        else // both positive 
        {
            switch (pbi1->CompareAbsolute(pbi2))
            {
            case 0: // a -a = 0
                return JavascriptBigInt::CreateZero(pbi1->GetScriptContext());
            case 1: 
                JavascriptBigInt::SubAbsolute(pbi1, pbi2); // a - b > 0
                return pbi1;
            default:
                pbi2->m_isNegative = true;
                JavascriptBigInt::SubAbsolute(pbi2, pbi1); // a - b = - (b-a) < 0
                return pbi2;
            }
        }
    }

    JavascriptBigInt * JavascriptBigInt::Add(JavascriptBigInt * pbi1, JavascriptBigInt * pbi2)
    {
        if (JavascriptBigInt::IsZero(pbi1)) 
        {
            return pbi2;
        }
        if (JavascriptBigInt::IsZero(pbi2))
        {
            return pbi1;
        }

        if (pbi1->m_isNegative == pbi2->m_isNegative) // (-a)+(-b) = -(a+b)
        {
            if (pbi1->m_length >= pbi2->m_length)
            {
                JavascriptBigInt::AddAbsolute(pbi1, pbi2);
                return pbi1;
            }
            JavascriptBigInt::AddAbsolute(pbi2, pbi1);
            return pbi2;
        }
        else 
        {
            switch (pbi1->CompareAbsolute(pbi2))
            {
            case 0:
                return JavascriptBigInt::CreateZero(pbi1->GetScriptContext()); // a + (-a) = -a + a = 0
            case 1: 
                JavascriptBigInt::SubAbsolute(pbi1, pbi2); // a + (-b) = a - b or (-a) + b = -(a-b)
                return pbi1;
            default:
                JavascriptBigInt::SubAbsolute(pbi2, pbi1); // -a + b = b - a or a + (-b) = -(b-a)
                return pbi2;
            }
        }
    }

    Var JavascriptBigInt::Add(Var aLeft, Var aRight)
    {
        JavascriptBigInt *leftBigInt = VarTo<JavascriptBigInt>(aLeft);
        JavascriptBigInt *rightBigInt = VarTo<JavascriptBigInt>(aRight);
        return JavascriptBigInt::Add(JavascriptBigInt::New(leftBigInt, leftBigInt->GetScriptContext()), JavascriptBigInt::New(rightBigInt, rightBigInt->GetScriptContext()));
        // TODO: Consider deferring creation of new instances until we need them
    }

    Var JavascriptBigInt::Sub(Var aLeft, Var aRight)
    {
        JavascriptBigInt *leftBigInt = VarTo<JavascriptBigInt>(aLeft);
        JavascriptBigInt *rightBigInt = VarTo<JavascriptBigInt>(aRight);
        return JavascriptBigInt::Sub(JavascriptBigInt::New(leftBigInt, leftBigInt->GetScriptContext()), JavascriptBigInt::New(rightBigInt, rightBigInt->GetScriptContext()));
        // TODO: Consider deferring creation of new instances until we need them
    }

} // namespace Js
