//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Js
{
        Var JavascriptMath::Negate_Full(Var aRight, ScriptContext* scriptContext)
        {
            // Special case for zero. Must return -0
            if( aRight == TaggedInt::ToVarUnchecked(0) )
            {
                return scriptContext->GetLibrary()->GetNegativeZero();
            }

            double value = Negate_Helper(aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(value, scriptContext);
        }

        Var JavascriptMath::Negate_InPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            // Special case for zero. Must return -0
            if( aRight == TaggedInt::ToVarUnchecked(0) )
            {
                return scriptContext->GetLibrary()->GetNegativeZero();
            }

            double value = Negate_Helper(aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(value, scriptContext, result);
        }

        Var JavascriptMath::Not_Full(Var aRight, ScriptContext* scriptContext)
        {
#if _M_IX86
            AssertMsg(!TaggedInt::Is(aRight), "Should be detected");
#endif
            int nValue = JavascriptConversion::ToInt32(aRight, scriptContext);
            return JavascriptNumber::ToVar(~nValue, scriptContext);
        }

        Var JavascriptMath::Not_InPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            AssertMsg(!TaggedInt::Is(aRight), "Should be detected");

            int nValue = JavascriptConversion::ToInt32(aRight, scriptContext);
            return JavascriptNumber::ToVarInPlace(~nValue, scriptContext, result);
        }

        Var JavascriptMath::Increment_InPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if (TaggedInt::Is(aRight))
            {
                return TaggedInt::Increment(aRight, scriptContext);
            }

            double inc = Increment_Helper(aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(inc, scriptContext, result);
        }

        Var JavascriptMath::Increment_Full(Var aRight, ScriptContext* scriptContext)
        {
            if (TaggedInt::Is(aRight))
            {
                return TaggedInt::Increment(aRight, scriptContext);
            }

            double inc = Increment_Helper(aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(inc, scriptContext);
        }

        Var JavascriptMath::Decrement_InPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if (TaggedInt::Is(aRight))
            {
                return TaggedInt::Decrement(aRight, scriptContext);
            }

            double dec = Decrement_Helper(aRight,scriptContext);
            return JavascriptNumber::InPlaceNew(dec, scriptContext, result);
        }

        Var JavascriptMath::Decrement_Full(Var aRight, ScriptContext* scriptContext)
        {
            if (TaggedInt::Is(aRight))
            {
                return TaggedInt::Decrement(aRight, scriptContext);
            }

            double dec = Decrement_Helper(aRight,scriptContext);
            return JavascriptNumber::ToVarNoCheck(dec, scriptContext);
        }

        Var JavascriptMath::And_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            int32 value = And_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVar(value, scriptContext);
        }

        Var JavascriptMath::And_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            int32 value = And_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVarInPlace(value, scriptContext, result);
        }

        Var JavascriptMath::Or_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            int32 value = Or_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVar(value, scriptContext);
        }

        Var JavascriptMath::Or_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            int32 value = Or_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVarInPlace(value, scriptContext, result);
        }

        Var JavascriptMath::Xor_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            int32 nLeft = TaggedInt::Is(aLeft) ? TaggedInt::ToInt32(aLeft) : JavascriptConversion::ToInt32(aLeft, scriptContext);
            int32 nRight = TaggedInt::Is(aRight) ? TaggedInt::ToInt32(aRight) : JavascriptConversion::ToInt32(aRight, scriptContext);

            return JavascriptNumber::ToVar(nLeft ^ nRight,scriptContext);
        }

        Var JavascriptMath::Xor_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext,  JavascriptNumber* result)
        {
            int32 nLeft = TaggedInt::Is(aLeft) ? TaggedInt::ToInt32(aLeft) : JavascriptConversion::ToInt32(aLeft, scriptContext);
            int32 nRight = TaggedInt::Is(aRight) ? TaggedInt::ToInt32(aRight) : JavascriptConversion::ToInt32(aRight, scriptContext);

            return JavascriptNumber::ToVarInPlace(nLeft ^ nRight, scriptContext, result);
        }

        Var JavascriptMath::ShiftLeft_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            int32 nValue    = JavascriptConversion::ToInt32(aLeft, scriptContext);
            uint32 nShift   = JavascriptConversion::ToUInt32(aRight, scriptContext);
            int32 nResult   = nValue << (nShift & 0x1F);

            return JavascriptNumber::ToVar(nResult,scriptContext);
        }

        Var JavascriptMath::ShiftRight_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            int32 nValue    = JavascriptConversion::ToInt32(aLeft, scriptContext);
            uint32 nShift   = JavascriptConversion::ToUInt32(aRight, scriptContext);

            int32 nResult   = nValue >> (nShift & 0x1F);

            return JavascriptNumber::ToVar(nResult,scriptContext);
        }

        Var JavascriptMath::ShiftRightU_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            uint32 nValue   = JavascriptConversion::ToUInt32(aLeft, scriptContext);
            uint32 nShift   = JavascriptConversion::ToUInt32(aRight, scriptContext);

            uint32 nResult  = nValue >> (nShift & 0x1F);

            return JavascriptNumber::ToVar(nResult,scriptContext);
        }

#if FLOATVAR
        Var JavascriptMath::Add_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            Assert(aLeft != nullptr);
            Assert(aRight != nullptr);
            Assert(scriptContext != nullptr);

            Js::TypeId typeLeft = JavascriptOperators::GetTypeId(aLeft);
            Js::TypeId typeRight = JavascriptOperators::GetTypeId(aRight);

            if (typeRight == typeLeft)
            {
                // If both sides are numbers/string, then we can do the addition directly
                if(typeLeft == TypeIds_Number)
                {
                    double sum = JavascriptNumber::GetValue(aLeft) + JavascriptNumber::GetValue(aRight);
                    return JavascriptNumber::ToVarNoCheck(sum, scriptContext);
                }
                else if (typeLeft == TypeIds_Integer)
                {
                    __int64 sum = TaggedInt::ToInt64(aLeft) + TaggedInt::ToInt64(aRight);
                    return JavascriptNumber::ToVar(sum, scriptContext);
                }
                else if (typeLeft == TypeIds_String)
                {
                    return JavascriptString::Concat(JavascriptString::FromVar(aLeft), JavascriptString::FromVar(aRight));
                }
            }
            else if(typeLeft == TypeIds_Number && typeRight == TypeIds_Integer)
            {
                double sum = JavascriptNumber::GetValue(aLeft) + TaggedInt::ToDouble(aRight);
                return JavascriptNumber::ToVarNoCheck(sum, scriptContext);
            }
            else if(typeLeft == TypeIds_Integer && typeRight == TypeIds_Number)
            {
                double sum = TaggedInt::ToDouble(aLeft) + JavascriptNumber::GetValue(aRight);
                return JavascriptNumber::ToVarNoCheck(sum, scriptContext);
            }

            return Add_FullHelper_Wrapper(aLeft, aRight, scriptContext, nullptr, false);
         }
#else
        Var JavascriptMath::Add_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            Assert(aLeft != nullptr);
            Assert(aRight != nullptr);
            Assert(scriptContext != nullptr);

            Js::TypeId typeLeft = JavascriptOperators::GetTypeId(aLeft);
            Js::TypeId typeRight = JavascriptOperators::GetTypeId(aRight);

            // Handle combinations of TaggedInt and Number or String pairs directly,
            // otherwise call the helper.
            switch( typeLeft )
            {
                case TypeIds_Integer:
                {
                    switch( typeRight )
                    {
                        case TypeIds_Integer:
                        {

                            // Compute the sum using integer addition, then convert to double.
                            // That way there's only one int->float conversion.
#if INT32VAR
                            int64 sum = TaggedInt::ToInt64(aLeft) + TaggedInt::ToInt64(aRight);
#else
                            int32 sum = TaggedInt::ToInt32(aLeft) + TaggedInt::ToInt32(aRight);
#endif
                            return JavascriptNumber::ToVar(sum, scriptContext );
                        }

                        case TypeIds_Number:
                        {
                            double sum = TaggedInt::ToDouble(aLeft) + JavascriptNumber::GetValue(aRight);
                            return JavascriptNumber::NewInlined( sum, scriptContext );
                        }
                    }
                    break;
                }

                case TypeIds_Number:
                {
                    switch( typeRight )
                    {
                        case TypeIds_Integer:
                        {
                            double sum = JavascriptNumber::GetValue(aLeft) + TaggedInt::ToDouble(aRight);
                            return JavascriptNumber::NewInlined( sum, scriptContext );
                        }

                        case TypeIds_Number:
                        {
                            double sum = JavascriptNumber::GetValue(aLeft) + JavascriptNumber::GetValue(aRight);
                            return JavascriptNumber::NewInlined( sum, scriptContext );
                        }
                    }
                    break;
                }

                case TypeIds_String:
                {
                    if( typeRight == TypeIds_String )
                    {
                        JavascriptString* leftString = JavascriptString::FromVar(aLeft);
                        JavascriptString* rightString = JavascriptString::FromVar(aRight);
                        return JavascriptString::Concat(leftString, rightString);
                    }
                    break;
                }
            }

            return Add_FullHelper_Wrapper(aLeft, aRight, scriptContext, nullptr, false);
        }
#endif
        Var JavascriptMath::Add_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            Assert(aLeft != nullptr);
            Assert(aRight != nullptr);
            Assert(scriptContext != nullptr);
            Assert(result != nullptr);

            // If both sides are numbers, then we can do the addition directly, otherwise
            // we need to call the helper.
            if( TaggedInt::Is(aLeft) )
            {
                if( TaggedInt::Is(aRight) )
                {
                    // Compute the sum using integer addition, then convert to double.
                    // That way there's only one int->float conversion.
#if INT32VAR
                    int64 sum = TaggedInt::ToInt64(aLeft) + TaggedInt::ToInt64(aRight);
#else
                    int32 sum = TaggedInt::ToInt32(aLeft) + TaggedInt::ToInt32(aRight);
#endif

                    return JavascriptNumber::ToVarInPlace(sum, scriptContext, result);
                }
                else if( JavascriptNumber::Is_NoTaggedIntCheck(aRight) )
                {
                    double sum = TaggedInt::ToDouble(aLeft) + JavascriptNumber::GetValue(aRight);
                    return JavascriptNumber::InPlaceNew( sum, scriptContext, result );
                }
            }
            else if( TaggedInt::Is(aRight) )
            {
                if( JavascriptNumber::Is_NoTaggedIntCheck(aLeft) )
                {
                    double sum = JavascriptNumber::GetValue(aLeft) + TaggedInt::ToDouble(aRight);
                    return JavascriptNumber::InPlaceNew( sum, scriptContext, result );
                }
            }
            else if( JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight) )
            {
                double sum = JavascriptNumber::GetValue(aLeft) + JavascriptNumber::GetValue(aRight);
                return JavascriptNumber::InPlaceNew( sum, scriptContext, result );
            }

            return Add_FullHelper_Wrapper(aLeft, aRight, scriptContext, result, false);
        }

        Var JavascriptMath::AddLeftDead(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber *result)
        {
            if (JavascriptOperators::GetTypeId(aLeft) == TypeIds_String)
            {
                JavascriptString* leftString = JavascriptString::FromVar(aLeft);
                JavascriptString* rightString;
                TypeId rightType = JavascriptOperators::GetTypeId(aRight);
                switch(rightType)
                {
                    case TypeIds_String:
                        rightString = JavascriptString::FromVar(aRight);

StringCommon:
                        return leftString->ConcatDestructive(rightString);

                    case TypeIds_Integer:
                        rightString = scriptContext->GetIntegerString(aRight);
                        goto StringCommon;

                    case TypeIds_Number:
                        rightString = JavascriptNumber::ToStringRadix10(JavascriptNumber::GetValue(aRight), scriptContext);
                        goto StringCommon;
                }
            }

            if (TaggedInt::Is(aLeft))
            {
                if (TaggedInt::Is(aRight))
                {
                    return TaggedInt::Add(aLeft, aRight, scriptContext);
                }
                else if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
                {
                    return JavascriptNumber::ToVarMaybeInPlace(TaggedInt::ToDouble(aLeft) + JavascriptNumber::GetValue(aRight), scriptContext, result);
                }
            }
            else if (TaggedInt::Is(aRight))
            {
                if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
                {
                    return JavascriptNumber::ToVarMaybeInPlace(JavascriptNumber::GetValue(aLeft) + TaggedInt::ToDouble(aRight), scriptContext, result);
                }
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::ToVarMaybeInPlace(JavascriptNumber::GetValue(aLeft) + JavascriptNumber::GetValue(aRight), scriptContext, result);
            }
            return Add_FullHelper_Wrapper(aLeft, aRight, scriptContext, result, true);
        }

        Var JavascriptMath::Add_FullHelper_Wrapper(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result, bool leftIsDead)
        {
            Var aLeftToPrim = JavascriptConversion::ToPrimitive(aLeft, JavascriptHint::None, scriptContext);
            Var aRightToPrim = JavascriptConversion::ToPrimitive(aRight, JavascriptHint::None, scriptContext);
            return Add_FullHelper(aLeftToPrim, aRightToPrim, scriptContext, result, leftIsDead);
        }

        Var JavascriptMath::Add_FullHelper(Var primLeft, Var primRight, ScriptContext* scriptContext, JavascriptNumber *result, bool leftIsDead)
        {
            // If either side is a string, then the result is also a string
            if (JavascriptOperators::GetTypeId(primLeft) == TypeIds_String)
            {
                JavascriptString* stringLeft = JavascriptString::FromVar(primLeft);
                JavascriptString* stringRight = nullptr;

                if (JavascriptOperators::GetTypeId(primRight) == TypeIds_String)
                {
                    stringRight = JavascriptString::FromVar(primRight);
                }
                else
                {
                    stringRight = JavascriptConversion::ToString(primRight, scriptContext);
                }

                if(leftIsDead)
                {
                    return stringLeft->ConcatDestructive(stringRight);
                }
                return JavascriptString::Concat(stringLeft, stringRight);
            }

            if (JavascriptOperators::GetTypeId(primRight) == TypeIds_String)
            {
                JavascriptString* stringLeft = JavascriptConversion::ToString(primLeft, scriptContext);
                JavascriptString* stringRight = JavascriptString::FromVar(primRight);

                if(leftIsDead)
                {
                    return stringLeft->ConcatDestructive(stringRight);
                }
                return JavascriptString::Concat(stringLeft, stringRight);
            }

            double sum = Add_Helper(primLeft, primRight, scriptContext);
            return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
        }

        Var JavascriptMath::MulAddLeft(Var mulLeft, Var mulRight, Var addLeft, ScriptContext* scriptContext,  JavascriptNumber* result)
        {
            if(TaggedInt::Is(mulLeft))
            {
                if(TaggedInt::Is(mulRight))
                {
                    // Compute the sum using integer addition, then convert to double.
                    // That way there's only one int->float conversion.
                    JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
                    Var mulResult = TaggedInt::MultiplyInPlace(mulLeft, mulRight, scriptContext, &mulTemp);

                    if (result)
                    {
                        return JavascriptMath::Add_InPlace(addLeft, mulResult, scriptContext, result);
                    }
                    else
                    {
                        return JavascriptMath::Add_Full(addLeft, mulResult, scriptContext);
                    }
                }
                else if(JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
                {
                    double mulResult = TaggedInt::ToDouble(mulLeft) * JavascriptNumber::GetValue(mulRight);

                    return JavascriptMath::Add_DoubleHelper(addLeft, mulResult, scriptContext, result);
                }
            }
            else if(TaggedInt::Is(mulRight))
            {
                if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft))
                {
                    double mulResult = JavascriptNumber::GetValue(mulLeft) * TaggedInt::ToDouble(mulRight);

                    return JavascriptMath::Add_DoubleHelper(addLeft, mulResult, scriptContext, result);
                }
            }
            else if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft) && JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
            {
                double mulResult = JavascriptNumber::GetValue(mulLeft) * JavascriptNumber::GetValue(mulRight);

                return JavascriptMath::Add_DoubleHelper(addLeft, mulResult, scriptContext, result);
            }

            Var aMul;
            JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
            aMul = JavascriptMath::Multiply_InPlace(mulLeft, mulRight, scriptContext, &mulTemp);
            if (result)
            {
                return JavascriptMath::Add_InPlace(addLeft, aMul, scriptContext, result);
            }
            else
            {
                return JavascriptMath::Add_Full(addLeft, aMul, scriptContext);
            }
        }

        Var JavascriptMath::MulAddRight(Var mulLeft, Var mulRight, Var addRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if(TaggedInt::Is(mulLeft))
            {
                if(TaggedInt::Is(mulRight))
                {
                    // Compute the sum using integer addition, then convert to double.
                    // That way there's only one int->float conversion.
                    JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
                    Var mulResult = TaggedInt::MultiplyInPlace(mulLeft, mulRight, scriptContext, &mulTemp);

                    if (result)
                    {
                        return JavascriptMath::Add_InPlace(mulResult, addRight, scriptContext, result);
                    }
                    else
                    {
                        return JavascriptMath::Add_Full(mulResult, addRight, scriptContext);
                    }
                }
                else if(JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
                {
                    double mulResult = TaggedInt::ToDouble(mulLeft) * JavascriptNumber::GetValue(mulRight);

                    return JavascriptMath::Add_DoubleHelper(mulResult, addRight, scriptContext, result);
                }
            }
            else if(TaggedInt::Is(mulRight))
            {
                if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft))
                {
                    double mulResult = JavascriptNumber::GetValue(mulLeft) * TaggedInt::ToDouble(mulRight);

                    return JavascriptMath::Add_DoubleHelper(mulResult, addRight, scriptContext, result);
                }
            }
            else if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft) && JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
            {
                double mulResult = JavascriptNumber::GetValue(mulLeft) * JavascriptNumber::GetValue(mulRight);

                return JavascriptMath::Add_DoubleHelper(mulResult, addRight, scriptContext, result);
            }

            Var aMul;
            JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
            aMul = JavascriptMath::Multiply_InPlace(mulLeft, mulRight, scriptContext, &mulTemp);
            if (result)
            {
                return JavascriptMath::Add_InPlace(aMul, addRight, scriptContext, result);
            }
            else
            {
                return JavascriptMath::Add_Full(aMul, addRight, scriptContext);
            }
        }

        Var JavascriptMath::MulSubLeft(Var mulLeft, Var mulRight, Var subLeft, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if(TaggedInt::Is(mulLeft))
            {
                if(TaggedInt::Is(mulRight))
                {
                    // Compute the sum using integer addition, then convert to double.
                    // That way there's only one int->float conversion.
                    JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
                    Var mulResult = TaggedInt::MultiplyInPlace(mulLeft, mulRight, scriptContext, &mulTemp);

                    if (result)
                    {
                        return JavascriptMath::Subtract_InPlace(subLeft, mulResult, scriptContext, result);
                    }
                    else
                    {
                        return JavascriptMath::Subtract_Full(subLeft, mulResult, scriptContext);
                    }
                }
                else if(JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
                {
                    double mulResult = TaggedInt::ToDouble(mulLeft) * JavascriptNumber::GetValue(mulRight);

                    return JavascriptMath::Subtract_DoubleHelper(subLeft, mulResult, scriptContext, result);
                }
            }
            else if(TaggedInt::Is(mulRight))
            {
                if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft))
                {
                    double mulResult = JavascriptNumber::GetValue(mulLeft) * TaggedInt::ToDouble(mulRight);

                    return JavascriptMath::Subtract_DoubleHelper(subLeft, mulResult, scriptContext, result);
                }
            }
            else if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft) && JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
            {
                double mulResult = JavascriptNumber::GetValue(mulLeft) * JavascriptNumber::GetValue(mulRight);

                return JavascriptMath::Subtract_DoubleHelper(subLeft, mulResult, scriptContext, result);
            }

            Var aMul;
            JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
            aMul = JavascriptMath::Multiply_InPlace(mulLeft, mulRight, scriptContext, &mulTemp);
            if (result)
            {
                return JavascriptMath::Subtract_InPlace(subLeft, aMul, scriptContext, result);
            }
            else
            {
                return JavascriptMath::Subtract_Full(subLeft, aMul, scriptContext);
            }
        }

        Var JavascriptMath::MulSubRight(Var mulLeft, Var mulRight, Var subRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if(TaggedInt::Is(mulLeft))
            {
                if(TaggedInt::Is(mulRight))
                {
                    // Compute the sum using integer addition, then convert to double.
                    // That way there's only one int->float conversion.
                    JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
                    Var mulResult = TaggedInt::MultiplyInPlace(mulLeft, mulRight, scriptContext, &mulTemp);

                    if (result)
                    {
                        return JavascriptMath::Subtract_InPlace(mulResult, subRight, scriptContext, result);
                    }
                    else
                    {
                        return JavascriptMath::Subtract_Full(mulResult, subRight, scriptContext);
                    }
                }
                else if(JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
                {
                    double mulResult = TaggedInt::ToDouble(mulLeft) * JavascriptNumber::GetValue(mulRight);

                    return JavascriptMath::Subtract_DoubleHelper(mulResult, subRight, scriptContext, result);
                }
            }
            else if(TaggedInt::Is(mulRight))
            {
                if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft))
                {
                    double mulResult = JavascriptNumber::GetValue(mulLeft) * TaggedInt::ToDouble(mulRight);

                    return JavascriptMath::Subtract_DoubleHelper(mulResult, subRight, scriptContext, result);
                }
            }
            else if(JavascriptNumber::Is_NoTaggedIntCheck(mulLeft) && JavascriptNumber::Is_NoTaggedIntCheck(mulRight))
            {
                double mulResult = JavascriptNumber::GetValue(mulLeft) * JavascriptNumber::GetValue(mulRight);

                return JavascriptMath::Subtract_DoubleHelper(mulResult, subRight, scriptContext, result);
            }

            Var aMul;
            JavascriptNumber mulTemp(0, scriptContext->GetLibrary()->GetNumberTypeStatic());
            aMul = JavascriptMath::Multiply_InPlace(mulLeft, mulRight, scriptContext, &mulTemp);
            if (result)
            {
                return JavascriptMath::Subtract_InPlace(aMul, subRight, scriptContext, result);
            }
            else
            {
                return JavascriptMath::Subtract_Full(aMul, subRight, scriptContext);
            }
        }

        Var inline JavascriptMath::Add_DoubleHelper(double dblLeft, Var addRight, ScriptContext* scriptContext, JavascriptNumber*result)
        {
            if (TaggedInt::Is(addRight))
            {
                double sum =  dblLeft + TaggedInt::ToDouble(addRight);

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(addRight))
            {
                double sum = dblLeft + JavascriptNumber::GetValue(addRight);

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else
            {
                Var aLeft = JavascriptNumber::ToVarMaybeInPlace(dblLeft, scriptContext, result);

                return Add_Full(aLeft, addRight, scriptContext);
            }
        }

        Var inline JavascriptMath::Add_DoubleHelper(Var addLeft, double dblRight, ScriptContext* scriptContext, JavascriptNumber*result)
        {
            if (TaggedInt::Is(addLeft))
            {
                double sum =  TaggedInt::ToDouble(addLeft) + dblRight;

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(addLeft))
            {
                double sum = JavascriptNumber::GetValue(addLeft) + dblRight;

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else
            {
                Var aRight = JavascriptNumber::ToVarMaybeInPlace(dblRight, scriptContext, result);

                return Add_Full(addLeft, aRight, scriptContext);
            }
        }

        Var inline JavascriptMath::Subtract_DoubleHelper(double dblLeft, Var subRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if (TaggedInt::Is(subRight))
            {
                double sum =  dblLeft - TaggedInt::ToDouble(subRight);

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(subRight))
            {
                double sum = dblLeft - JavascriptNumber::GetValue(subRight);

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else
            {
                Var aLeft = JavascriptNumber::ToVarMaybeInPlace(dblLeft, scriptContext, result);

                return Subtract_Full(aLeft, subRight, scriptContext);
            }
        }

        Var inline JavascriptMath::Subtract_DoubleHelper(Var subLeft, double dblRight, ScriptContext* scriptContext, JavascriptNumber*result)
        {
            if (TaggedInt::Is(subLeft))
            {
                double sum =  TaggedInt::ToDouble(subLeft) - dblRight;

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(subLeft))
            {
                double sum = JavascriptNumber::GetValue(subLeft) - dblRight;

                return JavascriptNumber::ToVarMaybeInPlace(sum, scriptContext, result);
            }
            else
            {
                Var aRight = JavascriptNumber::ToVarMaybeInPlace(dblRight, scriptContext, result);

                return Subtract_Full(subLeft, aRight, scriptContext);
            }
        }

        Var JavascriptMath::Subtract_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            double difference = Subtract_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(difference, scriptContext);
        }

        Var JavascriptMath::Subtract_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            double difference = Subtract_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(difference, scriptContext, result);
        }

        Var JavascriptMath::Divide_Full(Var aLeft,Var aRight, ScriptContext* scriptContext)
        {
            // If both arguments are TaggedInt, then try to do integer division
            // This case is not handled by the lowerer.
            if (TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::Divide(aLeft, aRight, scriptContext);
            }

            return JavascriptNumber::NewInlined( Divide_Helper(aLeft, aRight, scriptContext), scriptContext );
        }

        Var JavascriptMath::Exponentiation_Full(Var aLeft, Var aRight, ScriptContext *scriptContext)
        {
            double x = JavascriptConversion::ToNumber(aLeft, scriptContext);
            double y = JavascriptConversion::ToNumber(aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(Math::Pow(x, y), scriptContext);
        }

        Var JavascriptMath::Exponentiation_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            // The IEEE 754 floating point spec ensures that NaNs are preserved in all operations
            double dblLeft = JavascriptConversion::ToNumber(aLeft, scriptContext);
            double dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);

            return JavascriptNumber::InPlaceNew(Math::Pow(dblLeft, dblRight), scriptContext, result);
        }

        Var JavascriptMath::Multiply_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            Assert(aLeft != nullptr);
            Assert(aRight != nullptr);
            Assert(scriptContext != nullptr);

            if(JavascriptNumber::Is(aLeft))
            {
                if(JavascriptNumber::Is(aRight))
                {
                    double product = JavascriptNumber::GetValue(aLeft) * JavascriptNumber::GetValue(aRight);
                    return JavascriptNumber::ToVarNoCheck(product, scriptContext);
                }
                else if(TaggedInt::Is(aRight))
                {
                    double product = TaggedInt::ToDouble(aRight) * JavascriptNumber::GetValue(aLeft);
                    return JavascriptNumber::ToVarNoCheck(product, scriptContext);
                }
            }
            else if(JavascriptNumber::Is(aRight))
            {
                if(TaggedInt::Is(aLeft))
                {
                    double product = TaggedInt::ToDouble(aLeft) * JavascriptNumber::GetValue(aRight);
                    return JavascriptNumber::ToVarNoCheck(product, scriptContext);
                }
            }
            else if(TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::Multiply(aLeft, aRight, scriptContext);
            }
            double product = Multiply_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(product, scriptContext);
        }

        Var JavascriptMath::Multiply_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            if(JavascriptNumber::Is(aLeft))
            {
                if(JavascriptNumber::Is(aRight))
                {
                    return JavascriptNumber::ToVarInPlace(
                        JavascriptNumber::GetValue(aLeft) * JavascriptNumber::GetValue(aRight), scriptContext, result);
                }
                else if (TaggedInt::Is(aRight))
                {
                    return JavascriptNumber::ToVarInPlace(
                        JavascriptNumber::GetValue(aLeft) * TaggedInt::ToDouble(aRight), scriptContext, result);
                }
            }
            else if(JavascriptNumber::Is(aRight))
            {
                if(TaggedInt::Is(aLeft))
                {
                    return JavascriptNumber::ToVarInPlace(
                        TaggedInt::ToDouble(aLeft) * JavascriptNumber::GetValue(aRight), scriptContext, result);
                }
            }
            else if(TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::MultiplyInPlace(aLeft, aRight, scriptContext, result);
            }

            double product = Multiply_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(product, scriptContext, result);
        }

        Var JavascriptMath::Divide_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            // If both arguments are TaggedInt, then try to do integer division
            // This case is not handled by the lowerer.
            if (TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::DivideInPlace(aLeft, aRight, scriptContext, result);
            }

            double quotient = Divide_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(quotient, scriptContext, result);
        }

        Var JavascriptMath::Modulus_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
        {
            // If both arguments are TaggedInt, then try to do integer modulus.
            // This case is not handled by the lowerer.
            if (TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::Modulus(aLeft, aRight, scriptContext);
            }

            double remainder = Modulus_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::ToVarNoCheck(remainder, scriptContext);
        }

        Var JavascriptMath::Modulus_InPlace(Var aLeft, Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
        {
            Assert(aLeft != nullptr);
            Assert(aRight != nullptr);
            Assert(scriptContext != nullptr);

            // If both arguments are TaggedInt, then try to do integer division
            // This case is not handled by the lowerer.
            if (TaggedInt::IsPair(aLeft, aRight))
            {
                return TaggedInt::Modulus(aLeft, aRight, scriptContext);
            }

            double remainder = Modulus_Helper(aLeft, aRight, scriptContext);
            return JavascriptNumber::InPlaceNew(remainder, scriptContext, result);
        }


        Var JavascriptMath::FinishOddDivByPow2(int32 value, ScriptContext *scriptContext)
        {
            return JavascriptNumber::New((double)(value + 0.5), scriptContext);
        }

        Var JavascriptMath::FinishOddDivByPow2_InPlace(int32 value, ScriptContext *scriptContext, JavascriptNumber* result)
        {
            return JavascriptNumber::InPlaceNew((double)(value + 0.5), scriptContext, result);
        }

        Var JavascriptMath::MaxInAnArray(RecyclableObject * function, CallInfo callInfo, ...)
        {
            PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

            ARGUMENTS(args, callInfo);
            Assert(args.Info.Count == 2);
            Var thisArg = args[0];
            Var arrayArg = args[1];

            ScriptContext * scriptContext = function->GetScriptContext();

            TypeId typeId = JavascriptOperators::GetTypeId(arrayArg);
            if (!JavascriptNativeArray::Is(typeId) && !(TypedArrayBase::Is(typeId) && typeId != TypeIds_CharArray && typeId != TypeIds_BoolArray))
            {
                if (JavascriptArray::IsVarArray(typeId) && JavascriptArray::FromVar(arrayArg)->GetLength() == 0)
                {
                    return scriptContext->GetLibrary()->GetNegativeInfinite();
                }
                return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
            }

            if (JavascriptNativeArray::Is(typeId))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arrayArg);
#endif
                JavascriptNativeArray * argsArray = JavascriptNativeArray::FromVar(arrayArg);
                uint len = argsArray->GetLength();
                if (len == 0)
                {
                    return scriptContext->GetLibrary()->GetNegativeInfinite();
                }

                if (argsArray->GetHead()->next != nullptr || !argsArray->HasNoMissingValues() ||
                    argsArray->GetHead()->length != len)
                {
                    return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
                }

                return argsArray->FindMinOrMax(scriptContext, true /*findMax*/);
            }
            else
            {
                TypedArrayBase * argsArray = TypedArrayBase::FromVar(arrayArg);
                uint len = argsArray->GetLength();
                if (len == 0)
                {
                    return scriptContext->GetLibrary()->GetNegativeInfinite();
                }
                Var max = argsArray->FindMinOrMax(scriptContext, typeId, true /*findMax*/);
                if (max == nullptr)
                {
                    return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
                }
                return max;
            }
        }

        Var JavascriptMath::MinInAnArray(RecyclableObject * function, CallInfo callInfo, ...)
        {
            PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

            ARGUMENTS(args, callInfo);
            Assert(args.Info.Count == 2);
            Var thisArg = args[0];
            Var arrayArg = args[1];

            ScriptContext * scriptContext = function->GetScriptContext();

            TypeId typeId = JavascriptOperators::GetTypeId(arrayArg);
            if (!JavascriptNativeArray::Is(typeId) && !(TypedArrayBase::Is(typeId) && typeId != TypeIds_CharArray && typeId != TypeIds_BoolArray))
            {
                if (JavascriptArray::Is(typeId) && JavascriptArray::FromVar(arrayArg)->GetLength() == 0)
                {
                    return scriptContext->GetLibrary()->GetPositiveInfinite();
                }
                return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
            }

            if (JavascriptNativeArray::Is(typeId))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arrayArg);
#endif
                JavascriptNativeArray * argsArray = JavascriptNativeArray::FromVar(arrayArg);
                uint len = argsArray->GetLength();
                if (len == 0)
                {
                    return scriptContext->GetLibrary()->GetPositiveInfinite();
                }

                if (argsArray->GetHead()->next != nullptr || !argsArray->HasNoMissingValues() ||
                    argsArray->GetHead()->length != len)
                {
                    return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
                }

                return argsArray->FindMinOrMax(scriptContext, false /*findMax*/);
            }
            else
            {
                TypedArrayBase * argsArray = TypedArrayBase::FromVar(arrayArg);
                uint len = argsArray->GetLength();
                if (len == 0)
                {
                    return scriptContext->GetLibrary()->GetPositiveInfinite();
                }
                Var min = argsArray->FindMinOrMax(scriptContext, typeId, false /*findMax*/);
                if (min == nullptr)
                {
                    return JavascriptFunction::CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, arrayArg, scriptContext);
                }
                return min;
            }
        }

        void InitializeRandomSeeds(uint64 *seed0, uint64 *seed1, ScriptContext *scriptContext)
        {
#if DBG
            if (CONFIG_FLAG(PRNGSeed0) && CONFIG_FLAG(PRNGSeed1))
            {
                *seed0 = CONFIG_FLAG(PRNGSeed0);
                *seed1 = CONFIG_FLAG(PRNGSeed1);
            }
            else
#endif
            {
                LARGE_INTEGER s0;
                LARGE_INTEGER s1;

                if (!rand_s(reinterpret_cast<unsigned int*>(&s0.LowPart)) &&
                    !rand_s(reinterpret_cast<unsigned int*>(&s0.HighPart)) &&
                    !rand_s(reinterpret_cast<unsigned int*>(&s1.LowPart)) &&
                    !rand_s(reinterpret_cast<unsigned int*>(&s1.HighPart)))
                {
                    *seed0 = s0.QuadPart;
                    *seed1 = s1.QuadPart;
                }
                else
                {
                    AssertMsg(false, "Unable to initialize PRNG seeds with rand_s. Revert to using entropy.");
#ifdef ENABLE_CUSTOM_ENTROPY
                    ThreadContext *threadContext = scriptContext->GetThreadContext();

                    threadContext->GetEntropy().AddThreadCycleTime();
                    threadContext->GetEntropy().AddIoCounters();
                    *seed0 = threadContext->GetEntropy().GetRand();

                    threadContext->GetEntropy().AddThreadCycleTime();
                    threadContext->GetEntropy().AddIoCounters();
                    *seed1 = threadContext->GetEntropy().GetRand();
#endif
                }
            }
        }

        double ConvertRandomSeedsToDouble(const uint64 seed0, const uint64 seed1)
        {
            const uint64 mExp  = 0x3FF0000000000000;
            const uint64 mMant = 0x000FFFFFFFFFFFFF;

            // Take lower 52 bits of the sum of two seeds to make a double
            // Subtract 1.0 to negate the implicit integer bit of 1. Final range: [0.0, 1.0)
            // See IEEE754 Double-precision floating-point format for details
            //   https://en.wikipedia.org/wiki/Double-precision_floating-point_format
            uint64 resplusone_ui64 = ((seed0 + seed1) & mMant) | mExp;
            double res = *(reinterpret_cast<double*>(&resplusone_ui64)) - 1.0;
            return res;
        }

        void Xorshift128plus(uint64 *seed0, uint64 *seed1)
        {
            uint64 s1 = *seed0;
            uint64 s0 = *seed1;
            *seed0 = s0;
            s1 ^= s1 << 23;
            s1 ^= s1 >> 17;
            s1 ^= s0;
            s1 ^= s0 >> 26;
            *seed1 = s1;
        }

        double JavascriptMath::Random(ScriptContext *scriptContext)
        {
            uint64 seed0;
            uint64 seed1;

            if (!scriptContext->GetLibrary()->IsPRNGSeeded())
            {
                InitializeRandomSeeds(&seed0, &seed1, scriptContext);
#if DBG_DUMP
                OUTPUT_TRACE(Js::PRNGPhase, _u("[PRNG:%x] INIT %I64x %I64x\n"), scriptContext, seed0, seed1);
#endif
                scriptContext->GetLibrary()->SetIsPRNGSeeded(true);

#if ENABLE_TTD
                if(scriptContext->ShouldPerformReplayAction())
                {
                    scriptContext->GetThreadContext()->TTDLog->ReplayExternalEntropyRandomEvent(&seed0, &seed1);
                }
                else if(scriptContext->ShouldPerformRecordAction())
                {
                    scriptContext->GetThreadContext()->TTDLog->RecordExternalEntropyRandomEvent(seed0, seed1);
                }
                else
                {
                    ;
                }
#endif
            }
            else
            {
                seed0 = scriptContext->GetLibrary()->GetRandSeed0();
                seed1 = scriptContext->GetLibrary()->GetRandSeed1();
            }

#if DBG_DUMP
            OUTPUT_TRACE(Js::PRNGPhase, _u("[PRNG:%x] SEED %I64x %I64x\n"), scriptContext, seed0, seed1);
#endif

            Xorshift128plus(&seed0, &seed1);

            //update the seeds in script context
            scriptContext->GetLibrary()->SetRandSeed0(seed0);
            scriptContext->GetLibrary()->SetRandSeed1(seed1);

            double res = ConvertRandomSeedsToDouble(seed0, seed1);
#if DBG_DUMP
            OUTPUT_TRACE(Js::PRNGPhase, _u("[PRNG:%x] RAND %I64x\n"), scriptContext, *((uint64 *)&res));
#endif
            return res;
        }

        uint32 JavascriptMath::ToUInt32(double T1)
        {
            // Same as doing ToInt32 and reinterpret the bits as uint32
            return (uint32)ToInt32Core(T1);
        }

        int32 JavascriptMath::ToInt32(double T1)
        {
            return JavascriptMath::ToInt32Core(T1);
        }

        int32 JavascriptMath::ToInt32_Full(Var aValue, ScriptContext* scriptContext)
        {
            AssertMsg(!TaggedInt::Is(aValue), "Should be detected");

            // This is used when TaggedInt's overflow but remain under int32
            // so Number is our most critical case:

            TypeId typeId = JavascriptOperators::GetTypeId(aValue);

            if (typeId == TypeIds_Number)
            {
                return JavascriptMath::ToInt32Core(JavascriptNumber::GetValue(aValue));
            }

            return JavascriptConversion::ToInt32_Full(aValue, scriptContext);
        }
}
