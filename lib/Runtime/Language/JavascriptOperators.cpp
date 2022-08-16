//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLanguagePch.h"

#include "Types/PathTypeHandler.h"
#include "Types/PropertyIndexRanges.h"
#include "Types/UnscopablesWrapperObject.h"
#include "Types/SpreadArgument.h"
#include "Library/JavascriptPromise.h"
#include "Library/Regex/JavascriptRegularExpression.h"
#include "Library/ThrowErrorObject.h"
#include "Library/Generators/JavascriptGeneratorFunction.h"
#include "Library/Generators/JavascriptAsyncFunction.h"

#include "Library/ForInObjectEnumerator.h"
#include "Library/Array/ES5Array.h"
#include "Types/SimpleDictionaryPropertyDescriptor.h"
#include "Types/SimpleDictionaryTypeHandler.h"
#include "Language/ModuleNamespace.h"

#ifndef SCRIPT_DIRECT_TYPE
typedef enum JsNativeValueType: int
{
    JsInt8Type,
    JsUint8Type,
    JsInt16Type,
    JsUint16Type,
    JsInt32Type,
    JsUint32Type,
    JsInt64Type,
    JsUint64Type,
    JsFloatType,
    JsDoubleType,
    JsNativeStringType
} JsNativeValueType;

typedef struct JsNativeString
{
    unsigned int length;
    LPCWSTR str;
} JsNativeString;

#endif

using namespace Js;

    DEFINE_RECYCLER_TRACKER_ARRAY_PERF_COUNTER(Var);
    DEFINE_RECYCLER_TRACKER_PERF_COUNTER(FrameDisplay);

    enum IndexType
    {
        IndexType_Number,
        IndexType_PropertyId,
        IndexType_JavascriptString
    };

    IndexType GetIndexTypeFromString(char16 const * propertyName, charcount_t propertyLength, ScriptContext* scriptContext, uint32* index, PropertyRecord const** propertyRecord, bool createIfNotFound)
    {
        if (JavascriptOperators::TryConvertToUInt32(propertyName, propertyLength, index) &&
            (*index != JavascriptArray::InvalidIndex))
        {
            return IndexType_Number;
        }
        else
        {
            if (createIfNotFound)
            {
                scriptContext->GetOrAddPropertyRecord(propertyName, propertyLength, propertyRecord);
            }
            else
            {
                scriptContext->FindPropertyRecord(propertyName, propertyLength, propertyRecord);
            }
            return IndexType_PropertyId;
        }
    }

    IndexType GetIndexTypeFromPrimitive(Var indexVar, ScriptContext* scriptContext, uint32* index, PropertyRecord const ** propertyRecord, JavascriptString ** propertyNameString, bool createIfNotFound, bool preferJavascriptStringOverPropertyRecord)
    {
        // CONSIDER: Only OP_SetElementI and OP_GetElementI use and take advantage of the
        // IndexType_JavascriptString result. Consider modifying other callers of GetIndexType to take
        // advantage of non-interned property strings where appropriate.
        if (TaggedInt::Is(indexVar))
        {
            int indexInt = TaggedInt::ToInt32(indexVar);
            if (indexInt >= 0)
            {
                *index = (uint)indexInt;
                return IndexType_Number;
            }
            else
            {
                char16 stringBuffer[22];

                int pos = TaggedInt::ToBuffer(indexInt, stringBuffer, _countof(stringBuffer));
                charcount_t length = (_countof(stringBuffer) - 1) - pos;
                if (createIfNotFound || preferJavascriptStringOverPropertyRecord)
                {
                    // When preferring JavascriptString objects, just return a PropertyRecord instead
                    // of creating temporary JavascriptString objects for every negative integer that
                    // comes through here.
                    scriptContext->GetOrAddPropertyRecord(stringBuffer + pos, length, propertyRecord);
                }
                else
                {
                    scriptContext->FindPropertyRecord(stringBuffer + pos, length, propertyRecord);
                }
                return IndexType_PropertyId;
            }
        }

        if (JavascriptNumber::Is_NoTaggedIntCheck(indexVar))
        {
            // If this double can be a positive integer index, convert it.
            int32 value = 0;
            bool isInt32 = false;
            if (JavascriptNumber::TryGetInt32OrUInt32Value(JavascriptNumber::GetValue(indexVar), &value, &isInt32)
                && !isInt32
                && static_cast<uint32>(value) < JavascriptArray::InvalidIndex)
            {
                *index = static_cast<uint32>(value);
                return IndexType_Number;
            }

            // Fall through to slow string conversion.
        }

        JavascriptSymbol * symbol = JavascriptOperators::TryFromVar<JavascriptSymbol>(indexVar);
        if (symbol)
        {
            // JavascriptSymbols cannot add a new PropertyRecord - they correspond to one and only one existing PropertyRecord.
            // We already know what the PropertyRecord is since it is stored in the JavascriptSymbol itself so just return it.

            *propertyRecord = symbol->GetValue();
            return IndexType_PropertyId;
        }
        else
        {
            JavascriptString* indexStr = JavascriptConversion::ToString(indexVar, scriptContext);

            char16 const * propertyName = indexStr->GetString();
            charcount_t const propertyLength = indexStr->GetLength();

            if (!createIfNotFound && preferJavascriptStringOverPropertyRecord)
            {
                if (JavascriptOperators::TryConvertToUInt32(propertyName, propertyLength, index) &&
                    (*index != JavascriptArray::InvalidIndex))
                {
                    return IndexType_Number;
                }

                *propertyNameString = indexStr;
                return IndexType_JavascriptString;
            }
            return GetIndexTypeFromString(propertyName, propertyLength, scriptContext, index, propertyRecord, createIfNotFound);
        }
    }

    IndexType GetIndexTypeFromPrimitive(Var indexVar, ScriptContext* scriptContext, uint32* index, PropertyRecord const ** propertyRecord, bool createIfNotFound)
    {
        return GetIndexTypeFromPrimitive(indexVar, scriptContext, index, propertyRecord, nullptr, createIfNotFound, false);
    }

    IndexType GetIndexType(Var& indexVar, ScriptContext* scriptContext, uint32* index, PropertyRecord const ** propertyRecord, JavascriptString ** propertyNameString, bool createIfNotFound, bool preferJavascriptStringOverPropertyRecord)
    {
        indexVar = JavascriptConversion::ToPrimitive<JavascriptHint::HintString>(indexVar, scriptContext);
        return GetIndexTypeFromPrimitive(indexVar, scriptContext, index, propertyRecord, propertyNameString, createIfNotFound, preferJavascriptStringOverPropertyRecord);
    }

    IndexType GetIndexType(Var& indexVar, ScriptContext* scriptContext, uint32* index, PropertyRecord const ** propertyRecord, bool createIfNotFound)
    {
        return GetIndexType(indexVar, scriptContext, index, propertyRecord, nullptr, createIfNotFound, false);
    }

    BOOL FEqualDbl(double dbl1, double dbl2)
    {
        // If the low ulongs don't match, they can't be equal.
        if (Js::NumberUtilities::LuLoDbl(dbl1) != Js::NumberUtilities::LuLoDbl(dbl2))
            return FALSE;

        // If the high ulongs don't match, they can be equal iff one is -0 and
        // the other is +0.
        if (Js::NumberUtilities::LuHiDbl(dbl1) != Js::NumberUtilities::LuHiDbl(dbl2))
        {
            return 0x80000000 == (Js::NumberUtilities::LuHiDbl(dbl1) | Js::NumberUtilities::LuHiDbl(dbl2)) &&
                0 == Js::NumberUtilities::LuLoDbl(dbl1);
        }

        // The bit patterns match. They are equal iff they are not Nan.
        return !Js::NumberUtilities::IsNan(dbl1);
    }

    Var JavascriptOperators::OP_ApplyArgs(Var func, Var instance, __in_xcount(8) void** stackPtr, CallInfo callInfo, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_OP_ApplyArgs);
        int argCount = callInfo.Count;
        ///
        /// Check func has internal [[Call]] property
        /// If not, throw TypeError
        ///
        if (!JavascriptConversion::IsCallable(func)) {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }

        // Fix callInfo: expect result/value, and none of other flags are currently applicable.
        //   OP_ApplyArgs expects a result. Neither of {jit, interpreted} mode sends correct callFlags:
        //   LdArgCnt -- jit sends whatever was passed to current function, interpreter always sends 0.
        //   See Win8 bug 490489.
        callInfo.Flags = CallFlags_Value;

        RecyclableObject *funcPtr = UnsafeVarTo<RecyclableObject>(func);
        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault + argCount * 4);

        JavascriptMethod entryPoint = funcPtr->GetEntryPoint();
        Var ret;

        switch (argCount) {
        case 0:
            Assert(false);
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo);
            break;
        case 1:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance);
            break;
        case 2:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0]);
            break;
        case 3:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0], stackPtr[1]);
            break;
        case 4:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0], stackPtr[1], stackPtr[2]);
            break;
        case 5:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0], stackPtr[1], stackPtr[2], stackPtr[3]);
            break;
        case 6:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0], stackPtr[1], stackPtr[2], stackPtr[3], stackPtr[4]);
            break;
        case 7:
            ret = CALL_ENTRYPOINT_NOASSERT(entryPoint, funcPtr, callInfo, instance, stackPtr[0], stackPtr[1], stackPtr[2], stackPtr[3], stackPtr[4], stackPtr[5]);
            break;
        default:
        {
            // Don't need stack probe here- we just did so above
            Arguments args(callInfo, stackPtr - 1);
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                ret = JavascriptFunction::CallFunction<false>(funcPtr, entryPoint, args);
            }
            END_SAFE_REENTRANT_CALL
            break;
        }
        }
        return ret;
        JIT_HELPER_END(Op_OP_ApplyArgs);
    }

#ifdef _M_IX86
    // Alias for overloaded JavascriptNumber::ToVar so it can be called unambiguously from native code
    Var JavascriptOperators::Int32ToVar(int32 value, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_Int32ToAtom);
        return JavascriptNumber::ToVar(value, scriptContext);
        JIT_HELPER_END(Op_Int32ToAtom);
    }

    // Alias for overloaded JavascriptNumber::ToVar so it can be called unambiguously from native code
    Var JavascriptOperators::Int32ToVarInPlace(int32 value, ScriptContext* scriptContext, JavascriptNumber* result)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_Int32ToAtomInPlace);
        return JavascriptNumber::ToVarInPlace(value, scriptContext, result);
        JIT_HELPER_END(Op_Int32ToAtomInPlace);
    }

    // Alias for overloaded JavascriptNumber::ToVar so it can be called unambiguously from native code
    Var JavascriptOperators::UInt32ToVar(uint32 value, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_UInt32ToAtom);
        return JavascriptNumber::ToVar(value, scriptContext);
        JIT_HELPER_END(Op_UInt32ToAtom);
    }

    // Alias for overloaded JavascriptNumber::ToVar so it can be called unambiguously from native code
    Var JavascriptOperators::UInt32ToVarInPlace(uint32 value, ScriptContext* scriptContext, JavascriptNumber* result)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_UInt32ToAtomInPlace);
        return JavascriptNumber::ToVarInPlace(value, scriptContext, result);
        JIT_HELPER_END(Op_UInt32ToAtomInPlace);
    }
#endif

    Var JavascriptOperators::OP_FinishOddDivBy2(uint32 value, ScriptContext *scriptContext)
    {
        return JavascriptNumber::New((double)(value + 0.5), scriptContext);
    }

    Var JavascriptOperators::ToNumberInPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_ConvNumberInPlace);
        if (TaggedInt::Is(aRight) || JavascriptNumber::Is_NoTaggedIntCheck(aRight))
        {
            return aRight;
        }

        return JavascriptNumber::ToVarInPlace(JavascriptConversion::ToNumber(aRight, scriptContext), scriptContext, result);
        JIT_HELPER_END(Op_ConvNumberInPlace);
    }

    Var JavascriptOperators::ToNumericInPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result)
    {
        if (JavascriptOperators::GetTypeId(aRight) == TypeIds_BigInt)
        {
            return aRight;
        }
        return JavascriptOperators::ToNumberInPlace(aRight, scriptContext, result);
    }

    Var JavascriptOperators::Typeof(Var var, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Typeof);
        switch (JavascriptOperators::GetTypeId(var))
        {
        case TypeIds_Undefined:
            return scriptContext->GetLibrary()->GetUndefinedDisplayString();
        case TypeIds_Null:
            //null
            return scriptContext->GetLibrary()->GetObjectTypeDisplayString();
        case TypeIds_Integer:
        case TypeIds_Number:
        case TypeIds_Int64Number:
        case TypeIds_UInt64Number:
            return scriptContext->GetLibrary()->GetNumberTypeDisplayString();
        default:
            // Falsy objects are typeof 'undefined'.
            if (VarTo<RecyclableObject>(var)->GetType()->IsFalsy())
            {
                return scriptContext->GetLibrary()->GetUndefinedDisplayString();
            }
            else
            {
                return VarTo<RecyclableObject>(var)->GetTypeOfString(scriptContext);
            }
        }
        JIT_HELPER_END(Op_Typeof);
    }


    Var JavascriptOperators::TypeofFld(Var instance, PropertyId propertyId, ScriptContext* scriptContext)
    {
        return TypeofFld_Internal(instance, false, propertyId, scriptContext);
    }

    Var JavascriptOperators::TypeofRootFld(Var instance, PropertyId propertyId, ScriptContext* scriptContext)
    {
        return TypeofFld_Internal(instance, true, propertyId, scriptContext);
    }

    Var JavascriptOperators::TypeofFld_Internal(Var instance, const bool isRoot, PropertyId propertyId, ScriptContext* scriptContext)
    {
        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined , scriptContext->GetPropertyName(propertyId)->GetBuffer());
        }

        Var value = nullptr;
        try
        {
            Js::JavascriptExceptionOperators::AutoCatchHandlerExists autoCatchHandlerExists(scriptContext);

            // In edge mode, spec compat is more important than backward compat. Use spec/web behavior here
            if (isRoot
                    ? !JavascriptOperators::GetRootProperty(instance, propertyId, &value, scriptContext)
                    : !JavascriptOperators::GetProperty(instance, object, propertyId, &value, scriptContext))
            {
                return scriptContext->GetLibrary()->GetUndefinedDisplayString();
            }
            if (!scriptContext->IsUndeclBlockVar(value))
            {
                return JavascriptOperators::Typeof(value, scriptContext);
            }
        }
        catch(const JavascriptException& err)
        {
            err.GetAndClear();  // discard exception object
            return scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }

        Assert(scriptContext->IsUndeclBlockVar(value));
        JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
    }


    Var JavascriptOperators::TypeofElem_UInt32(Var instance, uint32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_TypeofElem_UInt32);
        if (JavascriptOperators::IsNumberFromNativeArray(instance, index, scriptContext))
            return scriptContext->GetLibrary()->GetNumberTypeDisplayString();

#if FLOATVAR
        return TypeofElem(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return TypeofElem(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_TypeofElem_UInt32);
    }

    Var JavascriptOperators::TypeofElem_Int32(Var instance, int32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_TypeofElem_Int32);
        if (JavascriptOperators::IsNumberFromNativeArray(instance, index, scriptContext))
            return scriptContext->GetLibrary()->GetNumberTypeDisplayString();

#if FLOATVAR
        return TypeofElem(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return TypeofElem(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_TypeofElem_Int32);
    }

    Js::JavascriptString* GetPropertyDisplayNameForError(Var prop, ScriptContext* scriptContext)
    {
        JavascriptString* str;
        JavascriptSymbol *symbol = JavascriptOperators::TryFromVar<JavascriptSymbol>(prop);
        if (symbol)
        {
            str = JavascriptSymbol::ToString(symbol->GetValue(), scriptContext);
        }
        else
        {
            str = JavascriptConversion::ToString(prop, scriptContext);
        }

        return str;
    }

    Var JavascriptOperators::TypeofElem(Var instance, Var index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_TypeofElem);
        RecyclableObject* object = nullptr;

        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined, GetPropertyDisplayNameForError(index, scriptContext));
        }

        Var member = nullptr;
        uint32 indexVal;
        PropertyRecord const * propertyRecord = nullptr;

        ThreadContext* threadContext = scriptContext->GetThreadContext();
        ImplicitCallFlags savedImplicitCallFlags = threadContext->GetImplicitCallFlags();
        threadContext->ClearImplicitCallFlags();

        try
        {
            Js::JavascriptExceptionOperators::AutoCatchHandlerExists autoCatchHandlerExists(scriptContext);
            IndexType indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, false);

            // For JS Objects, don't create the propertyId if not already added
            if (indexType == IndexType_Number)
            {
                // In edge mode, we don't need to worry about the special "unknown" behavior. If the item is not available from Get,
                // just return undefined.
                if (!JavascriptOperators::GetItem(instance, object, indexVal, &member, scriptContext))
                {
                    // If the instance doesn't have the item, typeof result is "undefined".
                    threadContext->CheckAndResetImplicitCallAccessorFlag();
                    threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
                    return scriptContext->GetLibrary()->GetUndefinedDisplayString();
                }
            }
            else
            {
                Assert(indexType == IndexType_PropertyId);
                if (propertyRecord == nullptr && !JavascriptOperators::CanShortcutOnUnknownPropertyName(object))
                {
                    indexType = GetIndexTypeFromPrimitive(index, scriptContext, &indexVal, &propertyRecord, true);
                    Assert(indexType == IndexType_PropertyId);
                    Assert(propertyRecord != nullptr);
                }

                if (propertyRecord != nullptr)
                {
                    if (!JavascriptOperators::GetProperty(instance, object, propertyRecord->GetPropertyId(), &member, scriptContext))
                    {
                        // If the instance doesn't have the property, typeof result is "undefined".
                        threadContext->CheckAndResetImplicitCallAccessorFlag();
                        threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
                        return scriptContext->GetLibrary()->GetUndefinedDisplayString();
                    }
                }
                else
                {
#if DBG
                    JavascriptString* indexStr = JavascriptConversion::ToString(index, scriptContext);
                    PropertyRecord const * debugPropertyRecord;
                    scriptContext->GetOrAddPropertyRecord(indexStr, &debugPropertyRecord);
                    AssertMsg(!JavascriptOperators::GetProperty(instance, object, debugPropertyRecord->GetPropertyId(), &member, scriptContext), "how did this property come? See OS Bug 2727708 if you see this come from the web");
#endif

                    // If the instance doesn't have the property, typeof result is "undefined".
                    threadContext->CheckAndResetImplicitCallAccessorFlag();
                    threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
                    return scriptContext->GetLibrary()->GetUndefinedDisplayString();
                }
            }
            threadContext->CheckAndResetImplicitCallAccessorFlag();
            threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
            return JavascriptOperators::Typeof(member, scriptContext);
        }
        catch(const JavascriptException& err)
        {
            err.GetAndClear();  // discard exception object
            threadContext->CheckAndResetImplicitCallAccessorFlag();
            threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
            return scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        JIT_HELPER_END(Op_TypeofElem);
    }

    //
    // Delete the given Var
    //
    Var JavascriptOperators::Delete(Var var, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_Delete);
        return scriptContext->GetLibrary()->GetTrue();
        JIT_HELPER_END(Op_Delete);
    }

    BOOL JavascriptOperators::Equal_Full(Var aLeft, Var aRight, ScriptContext* requestContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Equal_Full);
        //
        // Fast-path SmInts and paired Number combinations.
        //

        if (aLeft == aRight)
        {
            if (JavascriptNumber::Is(aLeft) && JavascriptNumber::IsNan(JavascriptNumber::GetValue(aLeft)))
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        BOOL result = false;

        if (TaggedInt::Is(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                // If aLeft == aRight, we would already have returned true above.
                return false;
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return TaggedInt::ToDouble(aLeft) == JavascriptNumber::GetValue(aRight);
            }
            else
            {
                BOOL res = UnsafeVarTo<RecyclableObject>(aRight)->Equals(aLeft, &result, requestContext);
                AssertMsg(res, "Should have handled this");
                return result;
            }
        }
        else if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                return TaggedInt::ToDouble(aRight) == JavascriptNumber::GetValue(aLeft);
            }
            else if(JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::GetValue(aLeft) == JavascriptNumber::GetValue(aRight);
            }
            else
            {
                BOOL res = UnsafeVarTo<RecyclableObject>(aRight)->Equals(aLeft, &result, requestContext);
                AssertMsg(res, "Should have handled this");
                return result;
            }
        }

        if (UnsafeVarTo<RecyclableObject>(aLeft)->Equals(aRight, &result, requestContext))
        {
            return result;
        }
        else
        {
            return false;
        }
        JIT_HELPER_END(Op_Equal_Full);
    }

    BOOL JavascriptOperators::Greater_Full(Var aLeft,Var aRight,ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Greater_Full);
        return RelationalComparisonHelper(aRight, aLeft, scriptContext, false, false);
        JIT_HELPER_END(Op_Greater_Full);
    }

    BOOL JavascriptOperators::Less_Full(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        return RelationalComparisonHelper(aLeft, aRight, scriptContext, true, false);
    }

    BOOL JavascriptOperators::RelationalComparisonHelper(Var aLeft, Var aRight, ScriptContext* scriptContext, bool leftFirst, bool undefinedAs)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aLeft);

        if (typeId == TypeIds_Null)
        {
            aLeft=TaggedInt::ToVarUnchecked(0);
        }
        else if (typeId == TypeIds_Undefined)
        {
            aLeft=scriptContext->GetLibrary()->GetNaN();
        }

        typeId = JavascriptOperators::GetTypeId(aRight);

        if (typeId == TypeIds_Null)
        {
            aRight=TaggedInt::ToVarUnchecked(0);
        }
        else if (typeId == TypeIds_Undefined)
        {
            aRight=scriptContext->GetLibrary()->GetNaN();
        }

        double dblLeft, dblRight;
        TypeId leftType = JavascriptOperators::GetTypeId(aLeft);
        TypeId rightType = JavascriptOperators::GetTypeId(aRight);

        if ((leftType == TypeIds_BigInt) || (rightType == TypeIds_BigInt))
        {
            // TODO: support comparison with types other than BigInt
            AssertOrFailFastMsg(leftType == rightType, "do not support comparison with types other than BigInt");
            return JavascriptBigInt::LessThan(aLeft, aRight);
        }

        switch (leftType)
        {
        case TypeIds_Integer:
            dblLeft = TaggedInt::ToDouble(aLeft);
            switch (rightType)
            {
            case TypeIds_Integer:
                dblRight = TaggedInt::ToDouble(aRight);
                break;
            case TypeIds_Number:
                dblRight = JavascriptNumber::GetValue(aRight);
                break;
            default:
                dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
                break;
            }
            break;
        case TypeIds_Number:
            dblLeft = JavascriptNumber::GetValue(aLeft);
            switch (rightType)
            {
            case TypeIds_Integer:
                dblRight = TaggedInt::ToDouble(aRight);
                break;
            case TypeIds_Number:
                dblRight = JavascriptNumber::GetValue(aRight);
                break;
            default:
                dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
                break;
            }
            break;
        case TypeIds_Int64Number:
            {
                switch (rightType)
                {
                case TypeIds_Int64Number:
                    {
                        __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                        __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                        return leftValue < rightValue;
                    }
                    break;
                case TypeIds_UInt64Number:
                    {
                        __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                        unsigned __int64 rightValue = UnsafeVarTo<JavascriptUInt64Number>(aRight)->GetValue();
                        if (rightValue <= INT_MAX && leftValue >= 0)
                        {
                            return leftValue < (__int64)rightValue;
                        }
                    }
                    break;
                }
                dblLeft = (double)UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
            }
            break;

        // we cannot do double conversion between 2 int64 numbers as we can get wrong result after conversion
        // i.e., two different numbers become the same after losing precision. We'll continue dbl comparison
        // if either number is not an int64 number.
        case TypeIds_UInt64Number:
            {
                switch (rightType)
                {
                case TypeIds_Int64Number:
                    {
                        unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                        __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                        if (leftValue < INT_MAX && rightValue >= 0)
                        {
                            return (__int64)leftValue < rightValue;
                        }
                    }
                    break;
                case TypeIds_UInt64Number:
                    {
                        unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                        unsigned __int64 rightValue = UnsafeVarTo<JavascriptUInt64Number>(aRight)->GetValue();
                        return leftValue < rightValue;
                    }
                    break;
                }
                dblLeft = (double)UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
            }
            break;

        case TypeIds_String:
            switch (rightType)
            {
            case TypeIds_Integer:
            case TypeIds_Number:
            case TypeIds_Boolean:
                break;
            default:
                aRight = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(aRight, scriptContext);
                rightType = JavascriptOperators::GetTypeId(aRight);
                if (rightType != TypeIds_String)
                {
                    dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
                    break;
                }
            case TypeIds_String:
                return JavascriptString::LessThan(aLeft, aRight);
            }
            dblLeft = JavascriptConversion::ToNumber(aLeft, scriptContext);
            dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
            break;
        case TypeIds_Boolean:
        case TypeIds_Null:
        case TypeIds_Undefined:
        case TypeIds_Symbol:
            dblLeft = JavascriptConversion::ToNumber(aLeft, scriptContext);
            dblRight = JavascriptConversion::ToNumber(aRight, scriptContext);
            break;
        default:
            if (leftFirst)
            {
                aLeft = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(aLeft, scriptContext);
                aRight = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(aRight, scriptContext);
            }
            else
            {
                aRight = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(aRight, scriptContext);
                aLeft = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(aLeft, scriptContext);
            }
            //BugFix: When @@ToPrimitive of an object is overridden with a function that returns null/undefined
            //this helper will fall into a inescapable goto loop as the checks for null/undefined were outside of the path
            return RelationalComparisonHelper(aLeft, aRight, scriptContext, leftFirst, undefinedAs);
        }

        //
        // And +0,-0 that is not implemented fully
        //

        if (JavascriptNumber::IsNan(dblLeft) || JavascriptNumber::IsNan(dblRight))
        {
            return undefinedAs;
        }

        // this will succeed for -0.0 == 0.0 case as well
        if (dblLeft == dblRight)
        {
            return false;
        }

        return dblLeft < dblRight;
    }

    BOOL JavascriptOperators::StrictEqualString(Var aLeft, JavascriptString* aRight)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_StrictEqualString);
        JIT_HELPER_SAME_ATTRIBUTES(Op_StrictEqualString, Op_StrictEqual);

        JavascriptString* leftStr = TryFromVar<JavascriptString>(aLeft);
        if (!leftStr)
        {
            return false;
        }
        JIT_HELPER_REENTRANT_HEADER(Op_StrictEqualString);
        JIT_HELPER_SAME_ATTRIBUTES(Op_StrictEqualString, Op_StrictEqual);
        return JavascriptString::Equals(leftStr, aRight);
        JIT_HELPER_END(Op_StrictEqualString);
    }

    BOOL JavascriptOperators::StrictEqualEmptyString(Var aLeft)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_StrictEqualEmptyString);
        JavascriptString * string = JavascriptOperators::TryFromVar<JavascriptString>(aLeft);
        if (!string)
        {
            return false;
        }

        Assert(string);
        return string->GetLength() == 0;
        JIT_HELPER_END(Op_StrictEqualEmptyString);
    }

#ifdef _CHAKRACOREBUILD
    BOOL JavascriptOperators::StrictEqualNumberType(Var aLeft, Var aRight, TypeId leftType, TypeId rightType, ScriptContext *requestContext)
    {
        double dblLeft, dblRight;

        switch (leftType)
        {
        case TypeIds_Integer:
            switch (rightType)
            {
            case TypeIds_Integer:
                return aLeft == aRight;
                // we don't need to worry about int64: it cannot equal as we create
                // JavascriptInt64Number only in overflow scenarios.
            case TypeIds_Number:
                dblLeft = TaggedInt::ToDouble(aLeft);
                dblRight = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;

        case TypeIds_Int64Number:
            switch (rightType)
            {
            case TypeIds_Int64Number:
            {
                __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                return leftValue == rightValue;
            }
            case TypeIds_UInt64Number:
            {
                __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                unsigned __int64 rightValue = VarTo<JavascriptUInt64Number>(aRight)->GetValue();
                return ((unsigned __int64)leftValue == rightValue);
            }
            case TypeIds_Number:
                dblLeft = (double)UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                dblRight = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;

        case TypeIds_UInt64Number:
            switch (rightType)
            {
            case TypeIds_Int64Number:
            {
                unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                return (leftValue == (unsigned __int64)rightValue);
            }
            case TypeIds_UInt64Number:
            {
                unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                unsigned __int64 rightValue = VarTo<JavascriptUInt64Number>(aRight)->GetValue();
                return leftValue == rightValue;
            }
            case TypeIds_Number:
                dblLeft = (double)UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                dblRight = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;

        case TypeIds_Number:
            switch (rightType)
            {
            case TypeIds_Integer:
                dblLeft = JavascriptNumber::GetValue(aLeft);
                dblRight = TaggedInt::ToDouble(aRight);
                goto CommonNumber;
            case TypeIds_Int64Number:
                dblLeft = JavascriptNumber::GetValue(aLeft);
                dblRight = (double)VarTo<JavascriptInt64Number>(aRight)->GetValue();
                goto CommonNumber;
            case TypeIds_UInt64Number:
                dblLeft = JavascriptNumber::GetValue(aLeft);
                dblRight = (double)UnsafeVarTo<JavascriptUInt64Number>(aRight)->GetValue();
                goto CommonNumber;
            case TypeIds_Number:
                dblLeft = JavascriptNumber::GetValue(aLeft);
                dblRight = JavascriptNumber::GetValue(aRight);
            CommonNumber:
                return FEqualDbl(dblLeft, dblRight);
            }
            return FALSE;
        }

        Assert(0 && "Unreachable Code");
        return FALSE;
    }

    BOOL JavascriptOperators::StrictEqual(Var aLeft, Var aRight, ScriptContext* requestContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_StrictEqual);
        TypeId rightType, leftType;
        leftType = JavascriptOperators::GetTypeId(aLeft);

        // Because NaN !== NaN, we may not return TRUE when typeId is Number
        if (aLeft == aRight && leftType != TypeIds_Number) return TRUE;

        rightType = JavascriptOperators::GetTypeId(aRight);

        if (leftType == TypeIds_String)
        {
            if (rightType == TypeIds_String)
            {
                return JavascriptString::Equals(UnsafeVarTo<JavascriptString>(aLeft), UnsafeVarTo<JavascriptString>(aRight));
            }
            return FALSE;
        }
        else if (leftType >= TypeIds_Integer && leftType <= TypeIds_UInt64Number)
        {
            return JavascriptOperators::StrictEqualNumberType(aLeft, aRight, leftType, rightType, requestContext);
        }
        else if (leftType == TypeIds_GlobalObject)
        {
            BOOL result;
            if (UnsafeVarTo<RecyclableObject>(aLeft)->StrictEquals(aRight, &result, requestContext))
            {
                return result;
            }
            return false;
        }
        else if (leftType == TypeIds_BigInt)
        {
            if (rightType == TypeIds_BigInt)
            {
                return JavascriptBigInt::Equals(aLeft, aRight);
            }
            return FALSE;
        }

        return aLeft == aRight;
        JIT_HELPER_END(Op_StrictEqual);
    }
 #else
    BOOL JavascriptOperators::StrictEqual(Var aLeft, Var aRight, ScriptContext* requestContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_StrictEqual);
        double dblLeft, dblRight;
        TypeId rightType, leftType;
        leftType = JavascriptOperators::GetTypeId(aLeft);

        // Because NaN !== NaN, we may not return TRUE when typeId is Number
        if (aLeft == aRight && leftType != TypeIds_Number) return TRUE;

        rightType = JavascriptOperators::GetTypeId(aRight);

        switch (leftType)
        {
        case TypeIds_String:
            switch (rightType)
            {
            case TypeIds_String:
                return JavascriptString::Equals(UnsafeVarTo<JavascriptString>(aLeft), UnsafeVarTo<JavascriptString>(aRight));
            }
            return FALSE;
        case TypeIds_Integer:
            switch (rightType)
            {
            case TypeIds_Integer:
                return aLeft == aRight;
            // we don't need to worry about int64: it cannot equal as we create
            // JavascriptInt64Number only in overflow scenarios.
            case TypeIds_Number:
                dblLeft     = TaggedInt::ToDouble(aLeft);
                dblRight    = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;
        case TypeIds_Int64Number:
            switch (rightType)
            {
            case TypeIds_Int64Number:
                {
                    __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                    __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                    return leftValue == rightValue;
                }
            case TypeIds_UInt64Number:
                {
                    __int64 leftValue = UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                    unsigned __int64 rightValue = VarTo<JavascriptUInt64Number>(aRight)->GetValue();
                    return ((unsigned __int64)leftValue == rightValue);
                }
            case TypeIds_Number:
                dblLeft     = (double)UnsafeVarTo<JavascriptInt64Number>(aLeft)->GetValue();
                dblRight    = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;
        case TypeIds_UInt64Number:
            switch (rightType)
            {
            case TypeIds_Int64Number:
                {
                    unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                    __int64 rightValue = UnsafeVarTo<JavascriptInt64Number>(aRight)->GetValue();
                    return (leftValue == (unsigned __int64)rightValue);
                }
            case TypeIds_UInt64Number:
                {
                    unsigned __int64 leftValue = UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                    unsigned __int64 rightValue = VarTo<JavascriptUInt64Number>(aRight)->GetValue();
                    return leftValue == rightValue;
                }
            case TypeIds_Number:
                dblLeft     = (double)UnsafeVarTo<JavascriptUInt64Number>(aLeft)->GetValue();
                dblRight    = JavascriptNumber::GetValue(aRight);
                goto CommonNumber;
            }
            return FALSE;

        case TypeIds_Number:
            switch (rightType)
            {
            case TypeIds_Integer:
                dblLeft     = JavascriptNumber::GetValue(aLeft);
                dblRight    = TaggedInt::ToDouble(aRight);
                goto CommonNumber;
            case TypeIds_Int64Number:
                dblLeft     = JavascriptNumber::GetValue(aLeft);
                dblRight = (double)VarTo<JavascriptInt64Number>(aRight)->GetValue();
                goto CommonNumber;
            case TypeIds_UInt64Number:
                dblLeft     = JavascriptNumber::GetValue(aLeft);
                dblRight = (double)UnsafeVarTo<JavascriptUInt64Number>(aRight)->GetValue();
                goto CommonNumber;
            case TypeIds_Number:
                dblLeft     = JavascriptNumber::GetValue(aLeft);
                dblRight    = JavascriptNumber::GetValue(aRight);
CommonNumber:
                return FEqualDbl(dblLeft, dblRight);
            }
            return FALSE;

        case TypeIds_BigInt:
            switch (rightType)
            {
            case TypeIds_BigInt:
                return JavascriptBigInt::Equals(aLeft, aRight);
            }
            return FALSE;
        case TypeIds_Boolean:
            switch (rightType)
            {
            case TypeIds_Boolean:
                return aLeft == aRight;
            }
            return FALSE;

        case TypeIds_Undefined:
            return rightType == TypeIds_Undefined;

        case TypeIds_Null:
            return rightType == TypeIds_Null;

        case TypeIds_Array:
            return (rightType == TypeIds_Array && aLeft == aRight);
#if DBG
        case TypeIds_Symbol:
            if (rightType == TypeIds_Symbol)
            {
                    const PropertyRecord* leftValue = UnsafeVarTo<JavascriptSymbol>(aLeft)->GetValue();
                    const PropertyRecord* rightValue = UnsafeVarTo<JavascriptSymbol>(aRight)->GetValue();
                    Assert(leftValue != rightValue);
            }
            break;
#endif
        case TypeIds_GlobalObject:
        case TypeIds_HostDispatch:
            switch (rightType)
            {
                case TypeIds_HostDispatch:
                case TypeIds_GlobalObject:
                {
                    BOOL result;
                    if(UnsafeVarTo<RecyclableObject>(aLeft)->StrictEquals(aRight, &result, requestContext))
                    {
                        return result;
                    }
                    return false;
                }
            }
            break;
        }

        if (VarTo<RecyclableObject>(aLeft)->IsExternal())
        {
            BOOL result;
            if (VarTo<RecyclableObject>(aLeft)->StrictEquals(aRight, &result, requestContext))
            {
                if (result)
                {
                    return TRUE;
                }
            }
        }

        if (!TaggedNumber::Is(aRight) && VarTo<RecyclableObject>(aRight)->IsExternal())
        {
            BOOL result;
            if (VarTo<RecyclableObject>(aRight)->StrictEquals(aLeft, &result, requestContext))
            {
                if (result)
                {
                    return TRUE;
                }
            }
        }

        return aLeft == aRight;
        JIT_HELPER_END(Op_StrictEqual);
    }
#endif

    BOOL JavascriptOperators::HasOwnProperty(
        Var instance,
        PropertyId propertyId,
        _In_ ScriptContext* requestContext,
        _In_opt_ PropertyString* propString)
    {
        if (TaggedNumber::Is(instance))
        {
            return FALSE;
        }
        RecyclableObject* object = UnsafeVarTo<RecyclableObject>(instance);

        if (VarIs<JavascriptProxy>(instance))
        {
            PropertyDescriptor desc;
            return GetOwnPropertyDescriptor(object, propertyId, requestContext, &desc);
        }

        // If we have a PropertyString, attempt to shortcut the lookup by using its caches
        if (propString != nullptr)
        {
            PropertyCacheOperationInfo info;
            if (propString->GetLdElemInlineCache()->PretendTryGetProperty(object->GetType(), &info))
            {
                switch (info.cacheType)
                {
                case CacheType_Local:
                    Assert(object->HasOwnProperty(propertyId));
                    return TRUE;
                case CacheType_Proto:
                    Assert(!object->HasOwnProperty(propertyId));
                    return FALSE;
                default:
                    // We had a cache hit, but cache doesn't tell us if we have an own property
                    break;
                }
            }
            if (propString->GetStElemInlineCache()->PretendTrySetProperty(object->GetType(), object->GetType(), &info))
            {
                switch (info.cacheType)
                {
                case CacheType_Local:
                    Assert(object->HasOwnProperty(propertyId));
                    return TRUE;
                case CacheType_LocalWithoutProperty:
                    Assert(!object->HasOwnProperty(propertyId));
                    return FALSE;
                default:
                    // We had a cache hit, but cache doesn't tell us if we have an own property
                    break;
                }
            }
        }

        return object && object->HasOwnProperty(propertyId);
    }

    BOOL JavascriptOperators::GetOwnAccessors(Var instance, PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext)
    {
        BOOL result;
        if (TaggedNumber::Is(instance))
        {
            result = false;
        }
        else
        {
            RecyclableObject* object = UnsafeVarTo<RecyclableObject>(instance);
            result = object && object->GetAccessors(propertyId, getter, setter, requestContext);
        }
        return result;
    }

    JavascriptArray* JavascriptOperators::GetOwnPropertyNames(Var instance, ScriptContext *scriptContext)
    {
        RecyclableObject *object = ToObject(instance, scriptContext);
        AssertOrFailFast(VarIsCorrectType(object)); // Consider moving this check into ToObject
        JavascriptProxy * proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instance);
        if (proxy)
        {
            return proxy->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::GetOwnPropertyNamesKind, scriptContext);
        }

        return JavascriptObject::CreateOwnStringPropertiesHelper(object, scriptContext);
    }

    JavascriptArray* JavascriptOperators::GetOwnPropertySymbols(Var instance, ScriptContext *scriptContext)
    {
        RecyclableObject *object = ToObject(instance, scriptContext);
        AssertOrFailFast(VarIsCorrectType(object));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Object_Constructor_getOwnPropertySymbols);

        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instance);
        if (proxy)
        {
            return proxy->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::GetOwnPropertySymbolKind, scriptContext);
        }

        return JavascriptObject::CreateOwnSymbolPropertiesHelper(object, scriptContext);
    }

    JavascriptArray* JavascriptOperators::GetOwnPropertyKeys(Var instance, ScriptContext* scriptContext)
    {
        RecyclableObject *object = ToObject(instance, scriptContext);
        AssertOrFailFast(VarIsCorrectType(object));

        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instance);
        if (proxy)
        {
            return proxy->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::KeysKind, scriptContext);
        }

        return JavascriptObject::CreateOwnStringSymbolPropertiesHelper(object, scriptContext);
    }

    JavascriptArray* JavascriptOperators::GetOwnEnumerablePropertyNames(RecyclableObject* object, ScriptContext* scriptContext)
    {
        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(object);
        if (proxy)
        {
            JavascriptArray* proxyResult = proxy->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::GetOwnPropertyNamesKind, scriptContext);
            JavascriptArray* proxyResultToReturn = scriptContext->GetLibrary()->CreateArray(0);

            // filter enumerable keys
            uint32 resultLength = proxyResult->GetLength();
            Var element;
            const Js::PropertyRecord *propertyRecord = nullptr;
            uint32 index = 0;
            for (uint32 i = 0; i < resultLength; i++)
            {
                element = proxyResult->DirectGetItem(i);

                Assert(!VarIs<JavascriptSymbol>(element));

                PropertyDescriptor propertyDescriptor;
                JavascriptConversion::ToPropertyKey(element, scriptContext, &propertyRecord, nullptr);
                if (JavascriptOperators::GetOwnPropertyDescriptor(object, propertyRecord->GetPropertyId(), scriptContext, &propertyDescriptor))
                {
                    if (propertyDescriptor.IsEnumerable())
                    {
                        proxyResultToReturn->DirectSetItemAt(index++, CrossSite::MarshalVar(scriptContext, element));
                    }
                }
            }
            return proxyResultToReturn;
        }

        return JavascriptObject::CreateOwnEnumerableStringPropertiesHelper(object, scriptContext);
    }

    JavascriptArray* JavascriptOperators::GetOwnEnumerablePropertyNamesSymbols(RecyclableObject* object, ScriptContext* scriptContext)
    {
        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(object);
        if (proxy)
        {
            return proxy->PropertyKeysTrap(JavascriptProxy::KeysTrapKind::KeysKind, scriptContext);
        }

        return JavascriptObject::CreateOwnEnumerableStringSymbolPropertiesHelper(object, scriptContext);
    }

    BOOL JavascriptOperators::GetOwnProperty(Var instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo * propertyValueInfo)
    {
        BOOL result;
        if (TaggedNumber::Is(instance))
        {
            result = false;
        }
        else
        {
            RecyclableObject* object = VarTo<RecyclableObject>(instance);
            result = object && object->GetProperty(object, propertyId, value, propertyValueInfo, requestContext);

            if (propertyValueInfo && result)
            {
                // We can only update the cache in case a property was found, because if it wasn't found, we don't know if it is missing or on a prototype
                CacheOperators::CachePropertyRead(instance, object, false /* isRoot */, propertyId, false /* isMissing */, propertyValueInfo, requestContext);
            }
        }
        return result;
    }

    BOOL JavascriptOperators::GetOwnPropertyDescriptor(RecyclableObject* obj, JavascriptString* propertyKey, ScriptContext* scriptContext, PropertyDescriptor* propertyDescriptor)
    {
        return JavascriptOperators::GetOwnPropertyDescriptor(obj, JavascriptOperators::GetPropertyId(propertyKey, scriptContext), scriptContext, propertyDescriptor);
    }

    // ES5's [[GetOwnProperty]].
    // Return value:
    //   FALSE means "undefined" PD.
    //   TRUE means success. The propertyDescriptor parameter gets the descriptor.
    //
    BOOL JavascriptOperators::GetOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propertyId, ScriptContext* scriptContext, PropertyDescriptor* propertyDescriptor)
    {
        Assert(obj);
        Assert(scriptContext);
        Assert(propertyDescriptor);

        if (VarIs<JavascriptProxy>(obj))
        {
            return JavascriptProxy::GetOwnPropertyDescriptor(obj, propertyId, scriptContext, propertyDescriptor);
        }
        Var getter, setter;
        if (false == JavascriptOperators::GetOwnAccessors(obj, propertyId, &getter, &setter, scriptContext))
        {
            Var value = nullptr;
            if (false == JavascriptOperators::GetOwnProperty(obj, propertyId, &value, scriptContext, nullptr))
            {
                return FALSE;
            }
            if (nullptr != value)
            {
                propertyDescriptor->SetValue(value);
            }

            //CONSIDER : Its expensive to query for each flag from type system. Combine this with the GetOwnProperty to get all the flags
            //at once. This will require a new API from type system and override in all the types which overrides IsEnumerable etc.
            //Currently there is no performance tuning for ES5. This should be ok.
            propertyDescriptor->SetWritable(FALSE != obj->IsWritable(propertyId));
        }
        else
        {
            if (nullptr == getter)
            {
                getter = scriptContext->GetLibrary()->GetUndefined();
            }
            propertyDescriptor->SetGetter(getter);

            if (nullptr == setter)
            {
                setter = scriptContext->GetLibrary()->GetUndefined();
            }
            propertyDescriptor->SetSetter(setter);
        }

        propertyDescriptor->SetConfigurable(FALSE != obj->IsConfigurable(propertyId));
        propertyDescriptor->SetEnumerable(FALSE != obj->IsEnumerable(propertyId));
        return TRUE;
    }

    inline RecyclableObject* JavascriptOperators::GetPrototypeNoTrap(RecyclableObject* instance)
    {
        Type* type = instance->GetType();
        if (type->HasSpecialPrototype())
        {
            if (type->GetTypeId() == TypeIds_Proxy)
            {
                // get back null
                Assert(type->GetPrototype() == instance->GetScriptContext()->GetLibrary()->GetNull());
                return type->GetPrototype();
            }
            else
            {
                return instance->GetPrototypeSpecial();
            }
        }
        return type->GetPrototype();
    }

    BOOL JavascriptOperators::IsRemoteArray(RecyclableObject* instance)
    {
        TypeId remoteTypeId = TypeIds_Limit;
        return (JavascriptOperators::GetRemoteTypeId(instance, &remoteTypeId) &&
            DynamicObject::IsAnyArrayTypeId(remoteTypeId));
    }

    bool JavascriptOperators::IsArray(_In_ JavascriptProxy * instance)
    {
        // If it is a proxy, follow to the end of the proxy chain before checking if it is an array again.
        JavascriptProxy * proxy = instance;
        while (true)
        {
            RecyclableObject * targetInstance = proxy->GetTarget();
            proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(targetInstance);
            if (proxy == nullptr)
            {
                return DynamicObject::IsAnyArray(targetInstance) || IsRemoteArray(targetInstance);
            }
        }
    }

    bool JavascriptOperators::IsArray(_In_ RecyclableObject* instance)
    {
        if (DynamicObject::IsAnyArray(instance))
        {
            return TRUE;
        }

        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instance);
        if (proxy)
        {
            return IsArray(proxy);
        }

        return IsRemoteArray(instance);
    }

    bool JavascriptOperators::IsArray(_In_ Var instanceVar)
    {
        RecyclableObject* instanceObj = TryFromVar<RecyclableObject>(instanceVar);
        return instanceObj && IsArray(instanceObj);
    }

    bool JavascriptOperators::IsConstructor(_In_ JavascriptProxy * instance)
    {
        // If it is a proxy, follow to the end of the proxy chain before checking if it is a constructor again.
        JavascriptProxy * proxy = instance;
        while (true)
        {
            RecyclableObject* targetInstance = proxy->GetTarget();
            proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(targetInstance);
            if (proxy == nullptr)
            {
                JavascriptFunction* function = JavascriptOperators::TryFromVar<JavascriptFunction>(targetInstance);
                return function && function->IsConstructor();
            }
        }
    }

    bool JavascriptOperators::IsConstructor(_In_ RecyclableObject* instanceObj)
    {
        JavascriptProxy* proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instanceObj);
        if (proxy)
        {
            return IsConstructor(proxy);
        }

        JavascriptFunction* function = JavascriptOperators::TryFromVar<JavascriptFunction>(instanceObj);
        return function && function->IsConstructor();
    }

    bool JavascriptOperators::IsConstructor(_In_ Var instanceVar)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_IsConstructor);
        RecyclableObject* instanceObj = TryFromVar<RecyclableObject>(instanceVar);
        return instanceObj && IsConstructor(instanceObj);
        JIT_HELPER_END(Op_IsConstructor);
    }

    BOOL JavascriptOperators::IsConcatSpreadable(Var instanceVar)
    {
        // an object is spreadable under two condition, either it is a JsArray
        // or you define an isconcatSpreadable flag on it.
        if (!JavascriptOperators::IsObject(instanceVar))
        {
            return false;
        }

        RecyclableObject* instance = UnsafeVarTo<RecyclableObject>(instanceVar);
        ScriptContext* scriptContext = instance->GetScriptContext();

        if (!PHASE_OFF1(IsConcatSpreadableCachePhase))
        {
            BOOL retVal = FALSE;
            Type *instanceType = instance->GetType();
            IsConcatSpreadableCache *isConcatSpreadableCache = scriptContext->GetThreadContext()->GetIsConcatSpreadableCache();

            if (isConcatSpreadableCache->TryGetIsConcatSpreadable(instanceType, &retVal))
            {
                OUTPUT_TRACE(Phase::IsConcatSpreadableCachePhase, _u("IsConcatSpreadableCache hit: %p\n"), instanceType);
                return retVal;
            }

            Var spreadable = nullptr;
            BOOL hasUserDefinedSpreadable = JavascriptOperators::GetProperty(instance, instance, PropertyIds::_symbolIsConcatSpreadable, &spreadable, scriptContext);

            if (hasUserDefinedSpreadable && spreadable != scriptContext->GetLibrary()->GetUndefined())
            {
                return JavascriptConversion::ToBoolean(spreadable, scriptContext);
            }

            retVal = JavascriptOperators::IsArray(instance);

            if (!hasUserDefinedSpreadable)
            {
                OUTPUT_TRACE(Phase::IsConcatSpreadableCachePhase, _u("IsConcatSpreadableCache saved: %p\n"), instanceType);
                isConcatSpreadableCache->CacheIsConcatSpreadable(instanceType, retVal);
            }

            return retVal;
        }

        Var spreadable = JavascriptOperators::GetProperty(instance, PropertyIds::_symbolIsConcatSpreadable, scriptContext);
        if (spreadable != scriptContext->GetLibrary()->GetUndefined())
        {
            return JavascriptConversion::ToBoolean(spreadable, scriptContext);
        }

        return JavascriptOperators::IsArray(instance);
    }

    bool JavascriptOperators::IsConstructorSuperCall(Arguments args)
    {
        Var newTarget = args.GetNewTarget();
        return args.IsNewCall() && newTarget != nullptr
                && !JavascriptOperators::IsUndefined(newTarget);
    }

    bool JavascriptOperators::GetAndAssertIsConstructorSuperCall(Arguments args)
    {
        bool isCtorSuperCall = JavascriptOperators::IsConstructorSuperCall(args);
        Assert(isCtorSuperCall || !args.IsNewCall()
                || args[0] == nullptr || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);
        return isCtorSuperCall;
    }

    Var JavascriptOperators::OP_LdCustomSpreadIteratorList(Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_ToSpreadedFunctionArgument);
#if ENABLE_COPYONACCESS_ARRAY
        // We know we're going to read from this array. Do the conversion before we try to perform checks on the head segment.
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray(aRight);
#endif

#ifdef ENABLE_JS_BUILTINS
        scriptContext->GetLibrary()->EnsureArrayBuiltInsAreReady();
#endif

        RecyclableObject* function = GetIteratorFunction(aRight, scriptContext);
        JavascriptMethod method = function->GetEntryPoint();
        if (((JavascriptArray::IsNonES5Array(aRight) &&
              (
                  JavascriptLibrary::IsDefaultArrayValuesFunction(function, scriptContext)
                  // Verify that the head segment of the array covers all elements with no gaps.
                  // Accessing an element on the prototype could have side-effects that would invalidate the optimization.
                  && UnsafeVarTo<JavascriptArray>(aRight)->GetHead()->next == nullptr
                  && UnsafeVarTo<JavascriptArray>(aRight)->GetHead()->left == 0
                  && UnsafeVarTo<JavascriptArray>(aRight)->GetHead()->length == VarTo<JavascriptArray>(aRight)->GetLength()
                  && UnsafeVarTo<JavascriptArray>(aRight)->HasNoMissingValues()
                  && !UnsafeVarTo<JavascriptArray>(aRight)->IsCrossSiteObject()
              )) ||
             (VarIs<TypedArrayBase>(aRight) && method == TypedArrayBase::EntryInfo::Values.GetOriginalEntryPoint()))
            // We can't optimize away the iterator if the array iterator prototype is user defined.
            && !JavascriptLibrary::ArrayIteratorPrototypeHasUserDefinedNext(scriptContext))
        {
            return RecyclerNew(scriptContext->GetRecycler(), SpreadArgument, aRight, true /*useDirectCall*/, scriptContext->GetLibrary()->GetSpreadArgumentType());
        }

        ThreadContext *threadContext = scriptContext->GetThreadContext();

        Var iteratorVar =
            threadContext->ExecuteImplicitCall(function, ImplicitCall_Accessor, [=]() -> Var
            {
                return CALL_FUNCTION(threadContext, function, CallInfo(Js::CallFlags_Value, 1), aRight);
            });

        if (!JavascriptOperators::IsObject(iteratorVar))
        {
            if (!threadContext->RecordImplicitException())
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
        }

        return RecyclerNew(scriptContext->GetRecycler(), SpreadArgument, iteratorVar, false /*useDirectCall*/, scriptContext->GetLibrary()->GetSpreadArgumentType());
        JIT_HELPER_END(Op_ToSpreadedFunctionArgument);
    }

    BOOL JavascriptOperators::IsPropertyUnscopable(Var instanceVar, JavascriptString *propertyString)
    {
        // This never gets called.
        Throw::InternalError();
    }

    BOOL JavascriptOperators::IsPropertyUnscopable(Var instanceVar, PropertyId propertyId)
    {
        RecyclableObject* instance = VarTo<RecyclableObject>(instanceVar);
        ScriptContext * scriptContext = instance->GetScriptContext();

        Var unscopables = JavascriptOperators::GetProperty(instance, PropertyIds::_symbolUnscopables, scriptContext);
        if (JavascriptOperators::IsObject(unscopables))
        {
            DynamicObject *unscopablesList = VarTo<DynamicObject>(unscopables);
            Var value = nullptr;
            //8.1.1.2.1.9.c If blocked is not undefined
            if (JavascriptOperators::GetProperty(unscopablesList, propertyId, &value, scriptContext))
            {
                return JavascriptConversion::ToBoolean(value, scriptContext);
            }
        }

        return false;
    }

    BOOL JavascriptOperators::HasProperty(RecyclableObject* instance, PropertyId propertyId)
    {
        while (!JavascriptOperators::IsNull(instance))
        {
            PropertyQueryFlags result = instance->HasPropertyQuery(propertyId, nullptr /*info*/);
            if (result != PropertyQueryFlags::Property_NotFound)
            {
                return JavascriptConversion::PropertyQueryFlagsToBoolean(result); // return false if instance is typed array and HasPropertyQuery() returns PropertyQueryFlags::Property_Found_Undefined
            }

            instance = JavascriptOperators::GetPrototypeNoTrap(instance);
        }
        return false;
    }

    BOOL JavascriptOperators::HasPropertyUnscopables(RecyclableObject* instance, PropertyId propertyId)
    {
        return JavascriptOperators::HasProperty(instance, propertyId)
            && !IsPropertyUnscopable(instance, propertyId);
    }

    BOOL JavascriptOperators::HasRootProperty(RecyclableObject* instance, PropertyId propertyId)
    {
        Assert(VarIs<RootObjectBase>(instance));

        RootObjectBase* rootObject = static_cast<RootObjectBase*>(instance);
        if (rootObject->HasRootProperty(propertyId))
        {
            return true;
        }
        instance = instance->GetPrototype();

        return HasProperty(instance, propertyId);
    }

    BOOL JavascriptOperators::HasProxyOrPrototypeInlineCacheProperty(RecyclableObject* instance, PropertyId propertyId)
    {
        TypeId typeId;
        typeId = JavascriptOperators::GetTypeId(instance);
        if (typeId == Js::TypeIds_Proxy)
        {
            // let's be more aggressive to disable inline prototype cache when proxy is presented in the prototypechain
            return true;
        }
        do
        {
            instance = instance->GetPrototype();
            typeId = JavascriptOperators::GetTypeId(instance);
            if (typeId == Js::TypeIds_Proxy)
            {
                // let's be more aggressive to disable inline prototype cache when proxy is presented in the prototypechain
                return true;
            }
            if (typeId == TypeIds_Null)
            {
                break;
            }
            /* We can rule out object with deferred type handler, because they would have expanded if they are in the cache */
            if (!instance->HasDeferredTypeHandler() && instance->HasProperty(propertyId)) { return true; }
        } while (typeId != TypeIds_Null);
        return false;
    }

    BOOL JavascriptOperators::OP_HasProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_HasProperty);
        RecyclableObject* object = TaggedNumber::Is(instance) ?
            scriptContext->GetLibrary()->GetNumberPrototype() :
            VarTo<RecyclableObject>(instance);
        BOOL result = HasProperty(object, propertyId);
        return result;
        JIT_HELPER_END(Op_HasProperty);
    }

    BOOL JavascriptOperators::OP_HasOwnProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext, _In_opt_ PropertyString * propString)
    {
        RecyclableObject* object = TaggedNumber::Is(instance) ?
            scriptContext->GetLibrary()->GetNumberPrototype() :
            VarTo<RecyclableObject>(instance);
        BOOL result = HasOwnProperty(object, propertyId, scriptContext, propString);
        return result;
    }

    // CONSIDER: Have logic similar to HasOwnPropertyNoHostObjectForHeapEnum
    BOOL JavascriptOperators::HasOwnPropertyNoHostObject(Var instance, PropertyId propertyId)
    {
        AssertMsg(!TaggedNumber::Is(instance), "HasOwnPropertyNoHostObject int passed");

        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        return object && object->HasOwnPropertyNoHostObject(propertyId);
    }

    // CONSIDER: Remove HasOwnPropertyNoHostObjectForHeapEnum and use GetOwnPropertyNoHostObjectForHeapEnum in its place by changing it
    // to return BOOL, true or false with whether the property exists or not, and return the value if not getter/setter as an out param.
    BOOL JavascriptOperators::HasOwnPropertyNoHostObjectForHeapEnum(Var instance, PropertyId propertyId, ScriptContext* requestContext, Var& getter, Var& setter)
    {
        AssertMsg(!TaggedNumber::Is(instance), "HasOwnPropertyNoHostObjectForHeapEnum int passed");

        RecyclableObject * object = VarTo<RecyclableObject>(instance);
        if (StaticType::Is(object->GetTypeId()))
        {
            return FALSE;
        }
        getter = setter = NULL;
        DynamicObject* dynamicObject = VarTo<DynamicObject>(instance);
        Assert(dynamicObject->GetScriptContext()->IsHeapEnumInProgress());
        if (dynamicObject->UseDynamicObjectForNoHostObjectAccess())
        {
            if (!dynamicObject->DynamicObject::GetAccessors(propertyId, &getter, &setter, requestContext))
            {
                Var value = nullptr;
                if (!JavascriptConversion::PropertyQueryFlagsToBoolean(dynamicObject->DynamicObject::GetPropertyQuery(instance, propertyId, &value, NULL, requestContext)) ||
                    (requestContext->IsUndeclBlockVar(value) && (VarIs<ActivationObject>(instance) || VarIs<RootObjectBase>(instance))))
                {
                    return FALSE;
                }
            }
        }
        else
        {
            if (!object->GetAccessors(propertyId, &getter, &setter, requestContext))
            {
                Var value = nullptr;
                if (!object->GetProperty(instance, propertyId, &value, NULL, requestContext) ||
                    (requestContext->IsUndeclBlockVar(value) && (VarIs<ActivationObject>(instance) || VarIs<RootObjectBase>(instance))))
                {
                    return FALSE;
                }
            }
        }
        return TRUE;
    }

    Var JavascriptOperators::GetOwnPropertyNoHostObjectForHeapEnum(Var instance, PropertyId propertyId, ScriptContext* requestContext, Var& getter, Var& setter)
    {
        AssertMsg(!TaggedNumber::Is(instance), "GetDataPropertyNoHostObject int passed");
        Assert(HasOwnPropertyNoHostObjectForHeapEnum(instance, propertyId, requestContext, getter, setter) || getter || setter);
        DynamicObject* dynamicObject = VarTo<DynamicObject>(instance);
        getter = setter = NULL;
        if (NULL == dynamicObject)
        {
            return requestContext->GetLibrary()->GetUndefined();
        }
        Var returnVar = requestContext->GetLibrary()->GetUndefined();
        BOOL result = FALSE;
        if (dynamicObject->UseDynamicObjectForNoHostObjectAccess())
        {
            if (! dynamicObject->DynamicObject::GetAccessors(propertyId, &getter, &setter, requestContext))
            {
                result = JavascriptConversion::PropertyQueryFlagsToBoolean((dynamicObject->DynamicObject::GetPropertyQuery(instance, propertyId, &returnVar, NULL, requestContext)));
            }
        }
        else
        {
            if (! dynamicObject->GetAccessors(propertyId, &getter, &setter, requestContext))
            {
                result = dynamicObject->GetProperty(instance, propertyId, &returnVar, NULL, requestContext);
            }
        }

        if (result)
        {
            return returnVar;
        }
        return requestContext->GetLibrary()->GetUndefined();
    }


    BOOL JavascriptOperators::OP_HasOwnPropScoped(Var scope, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext)
    {
        AssertMsg(scope == scriptContext->GetLibrary()->GetNull() || JavascriptArray::IsNonES5Array(scope),
                  "Invalid scope chain pointer passed - should be null or an array");

        JavascriptArray* arrScope = JavascriptArray::TryVarToNonES5Array(scope);
        if (arrScope)
        {
            Var instance = arrScope->DirectGetItem(0);
            return JavascriptOperators::OP_HasOwnProperty(instance, propertyId, scriptContext);
        }
        return JavascriptOperators::OP_HasOwnProperty(defaultInstance, propertyId, scriptContext);
    }

    BOOL JavascriptOperators::GetPropertyUnscopable(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return GetProperty_Internal<true>(instance, propertyObject, false, propertyId, value, requestContext, info);
    }

    BOOL JavascriptOperators::GetProperty(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return GetProperty_Internal<false>(instance, propertyObject, false, propertyId, value, requestContext, info);
    }

    BOOL JavascriptOperators::GetRootProperty(Var instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return GetProperty_Internal<false>(instance, VarTo<RecyclableObject>(instance), true, propertyId, value, requestContext, info);
    }

    BOOL JavascriptOperators::GetProperty_InternalSimple(Var instance, RecyclableObject* object, PropertyId propertyId, _Outptr_result_maybenull_ Var* value, ScriptContext* requestContext)
    {
        BOOL foundProperty = FALSE;
        Assert(value != nullptr);

        while (!JavascriptOperators::IsNull(object))
        {
            PropertyQueryFlags result = object->GetPropertyQuery(instance, propertyId, value, nullptr, requestContext);
            if (result != PropertyQueryFlags::Property_NotFound)
            {
                foundProperty = JavascriptConversion::PropertyQueryFlagsToBoolean(result);
                break;
            }

            if (object->SkipsPrototype())
            {
                break;
            }

            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }

        if (!foundProperty)
        {
            *value = requestContext->GetMissingPropertyResult();
        }

        return foundProperty;
    }

    template <bool unscopables>
    BOOL JavascriptOperators::GetProperty_Internal(Var instance, RecyclableObject* propertyObject, const bool isRoot, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        if (TaggedNumber::Is(instance))
        {
            PropertyValueInfo::ClearCacheInfo(info);
        }
        RecyclableObject* object = propertyObject;
        BOOL foundProperty = FALSE;
        if (isRoot)
        {
            Assert(VarIs<RootObjectBase>(object));

            RootObjectBase* rootObject = static_cast<RootObjectBase*>(object);
            foundProperty = rootObject->GetRootProperty(instance, propertyId, value, info, requestContext);
        }

        while (!foundProperty && !JavascriptOperators::IsNull(object))
        {
            if (unscopables && IsPropertyUnscopable(object, propertyId))
            {
                break;
            }
            else
            {
                PropertyQueryFlags result = object->GetPropertyQuery(instance, propertyId, value, info, requestContext);
                if (result != PropertyQueryFlags::Property_NotFound)
                {
                    foundProperty = JavascriptConversion::PropertyQueryFlagsToBoolean(result);
                    break;
                }
            }

            if (object->SkipsPrototype())
            {
                break;
            }

            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }

        if (foundProperty)
        {
#if ENABLE_FIXED_FIELDS && DBG
            if (DynamicObject::IsBaseDynamicObject(object))
            {
                DynamicObject* dynamicObject = (DynamicObject*)object;
                DynamicTypeHandler* dynamicTypeHandler = dynamicObject->GetDynamicType()->GetTypeHandler();
                Var property;
                if (dynamicTypeHandler->CheckFixedProperty(requestContext->GetPropertyName(propertyId), &property, requestContext))
                {
                    bool skipAssert = false;
                    if (value != nullptr && Js::VarIs<Js::RecyclableObject>(property))
                    {
                        Js::RecyclableObject* pObject = Js::VarTo<Js::RecyclableObject>(property);
                        Js::RecyclableObject* pValue = Js::VarTo<Js::RecyclableObject>(*value);

                        if (pValue->GetScriptContext() != pObject->GetScriptContext())
                        {
                            // value was marshaled. skip check
                            skipAssert = true;
                        }
                    }
                    Assert(skipAssert || value == nullptr || *value == property);
                }
            }
#endif
            // Don't cache the information if the value is undecl block var
            // REVIEW: We might want to only check this if we need to (For LdRootFld or ScopedLdFld)
            //         Also we might want to throw here instead of checking it again in the caller
            if (value && !requestContext->IsUndeclBlockVar(*value) && !VarIs<UnscopablesWrapperObject>(object))
            {
                CacheOperators::CachePropertyRead(propertyObject, object, isRoot, propertyId, false, info, requestContext);
            }
#ifdef TELEMETRY_JSO
            if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
            {
                requestContext->GetTelemetry().GetOpcodeTelemetry().GetProperty(instance, propertyId, value, /*successful: */true);
            }
#endif

            return TRUE;
        }

        else
        {
#ifdef MISSING_PROPERTY_STATS
            if (PHASE_STATS1(MissingPropertyCachePhase))
            {
                requestContext->RecordMissingPropertyMiss();
            }
#endif
            if (PHASE_TRACE1(MissingPropertyCachePhase))
            {
                Output::Print(_u("MissingPropertyCaching: Missing property %d on slow path.\n"), propertyId);
            }

            TryCacheMissingProperty(instance, propertyObject, isRoot, propertyId, requestContext, info);

#if defined(TELEMETRY_JSO) || defined(TELEMETRY_AddToCache) // enabled for `TELEMETRY_AddToCache`, because this is the property-not-found codepath where the normal TELEMETRY_AddToCache code wouldn't be executed.
            if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
            {
                if (info && info->AllowResizingPolymorphicInlineCache()) // If in interpreted mode, not JIT.
                {
                    requestContext->GetTelemetry().GetOpcodeTelemetry().GetProperty(instance, propertyId, nullptr);
                }
            }
#endif
            *value = requestContext->GetMissingPropertyResult();
            return FALSE;
        }
    }

    // If the given instance is a type where we can cache missing properties, then cache that the given property ID is missing.
    // cacheInstance is used as startingObject in CachePropertyRead, and might be instance's proto if we are fetching a super property (see #3064).
    void JavascriptOperators::TryCacheMissingProperty(Var instance, Var cacheInstance, bool isRoot, PropertyId propertyId, ScriptContext* requestContext, _Inout_ PropertyValueInfo * info)
    {
        // Here, any well-behaved subclasses of DynamicObject can opt in to getting included in the missing property cache.
        // For now, we only include basic objects and arrays. 
        if (PHASE_OFF1(MissingPropertyCachePhase) || isRoot || !(DynamicObject::IsBaseDynamicObject(instance) || DynamicObject::IsAnyArray(instance)))
        {
            return;
        }

        // CustomExternalObject in particular is problematic because in some cases it can report missing when implicit callsare disabled.
        // See CustomExternalObject::GetPropertyQuery for an example.
        if (UnsafeVarTo<DynamicObject>(instance)->GetType()->IsJsrtExternal() && requestContext->GetThreadContext()->IsDisableImplicitCall())
        {
            return;
        }

        DynamicTypeHandler* handler = UnsafeVarTo<DynamicObject>(instance)->GetDynamicType()->GetTypeHandler();

        // Only cache missing property lookups for non-root field loads on objects that have PathTypeHandlers, because only these types have the right behavior
        // when the missing property is later added. DictionaryTypeHandler's introduce the possibility that a stale TypePropertyCache entry with isMissing==true can
        // be left in the cache after the property has been installed in the object's prototype chain. Other changes to optimize accesses to objects that don't
        // override special symbols make it unnecessary to introduce an invalidation scheme to deal with DictionaryTypeHandler's.

        if (!handler->IsPathTypeHandler())
        {
            return;
        }

#ifdef MISSING_PROPERTY_STATS
        if (PHASE_STATS1(MissingPropertyCachePhase))
        {
            requestContext->RecordMissingPropertyCacheAttempt();
        }
#endif
        if (PHASE_TRACE1(MissingPropertyCachePhase))
        {
            Output::Print(_u("MissingPropertyCache: Caching missing property for property %d.\n"), propertyId);
        }

        PropertyValueInfo::Set(info, requestContext->GetLibrary()->GetMissingPropertyHolder(), 0);
        CacheOperators::CachePropertyRead(cacheInstance, requestContext->GetLibrary()->GetMissingPropertyHolder(), isRoot, propertyId, true /*isMissing*/, info, requestContext);
    }

    template<bool OutputExistence, typename PropertyKeyType> PropertyQueryFlags QueryGetOrHasProperty(
        Var originalInstance, RecyclableObject* object, PropertyKeyType propertyKey, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
    template<> PropertyQueryFlags QueryGetOrHasProperty<false /*OutputExistence*/, PropertyId /*PropertyKeyType*/>(
        Var originalInstance, RecyclableObject* object, PropertyId propertyKey, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return object->GetPropertyQuery(originalInstance, propertyKey, value, info, requestContext);
    }
    template<> PropertyQueryFlags QueryGetOrHasProperty<false /*OutputExistence*/, JavascriptString* /*PropertyKeyType*/>(
        Var originalInstance, RecyclableObject* object, JavascriptString* propertyKey, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return object->GetPropertyQuery(originalInstance, propertyKey, value, info, requestContext);
    }
    template<> PropertyQueryFlags QueryGetOrHasProperty<true /*OutputExistence*/, PropertyId /*PropertyKeyType*/>(
        Var originalInstance, RecyclableObject* object, PropertyId propertyKey, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        PropertyQueryFlags result = object->HasPropertyQuery(propertyKey, info);
        *value = JavascriptBoolean::ToVar(JavascriptConversion::PropertyQueryFlagsToBoolean(result), requestContext);
        return result;
    }

    template<bool OutputExistence, typename PropertyKeyType>
    BOOL JavascriptOperators::GetPropertyWPCache(Var instance, RecyclableObject* propertyObject, PropertyKeyType propertyKey, Var* value, ScriptContext* requestContext, _Inout_ PropertyValueInfo * info)
    {
        Assert(value);
        RecyclableObject* object = propertyObject;
        while (!JavascriptOperators::IsNull(object))
        {
            PropertyQueryFlags result = QueryGetOrHasProperty<OutputExistence>(instance, object, propertyKey, value, info, requestContext);

            if (result != PropertyQueryFlags::Property_NotFound)
            {
                if (!VarIs<UnscopablesWrapperObject>(object) && info->GetPropertyRecordUsageCache())
                {
                    PropertyId propertyId = info->GetPropertyRecordUsageCache()->GetPropertyRecord()->GetPropertyId();
                    CacheOperators::CachePropertyRead(instance, object, false, propertyId, false, info, requestContext);
                }
                return JavascriptConversion::PropertyQueryFlagsToBoolean(result);
            }

            // SkipsPrototype refers only to the Get operation, not Has. Some objects like CustomExternalObject respond
            // to HasPropertyQuery with info only about the object itself and GetPropertyQuery with info about its prototype chain.
            // For consistency with the behavior of JavascriptOperators::HasProperty, don't skip prototypes when outputting existence.
            if (!OutputExistence && object->SkipsPrototype())
            {
                break;
            }
            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }
        if (info->GetPropertyRecordUsageCache())
        {
            TryCacheMissingProperty(instance, instance, false /*isRoot*/, info->GetPropertyRecordUsageCache()->GetPropertyRecord()->GetPropertyId(), requestContext, info);
        }

        *value = OutputExistence
            ? requestContext->GetLibrary()->GetFalse()
            : requestContext->GetMissingPropertyResult();
        return FALSE;
    }

    bool JavascriptOperators::GetPropertyObjectForElementAccess(
        _In_ Var instance,
        _In_ Var index,
        _In_ ScriptContext* scriptContext,
        _Out_ RecyclableObject** propertyObject,
        _In_ rtErrors error)
    {
        BOOL isNullOrUndefined = !GetPropertyObject(instance, scriptContext, propertyObject);
        Assert(*propertyObject == instance || TaggedNumber::Is(instance));

        if (isNullOrUndefined)
        {
            if (!scriptContext->GetThreadContext()->RecordImplicitException())
            {
                return false;
            }

            JavascriptError::ThrowTypeError(scriptContext, error, GetPropertyDisplayNameForError(index, scriptContext));
        }
        return true;
    }

    bool JavascriptOperators::GetPropertyObjectForSetElementI(
        _In_ Var instance,
        _In_ Var index,
        _In_ ScriptContext* scriptContext,
        _Out_ RecyclableObject** propertyObject)
    {
        return GetPropertyObjectForElementAccess(instance, index, scriptContext, propertyObject, JSERR_Property_CannotSet_NullOrUndefined);
    }

    bool JavascriptOperators::GetPropertyObjectForGetElementI(
        _In_ Var instance,
        _In_ Var index,
        _In_ ScriptContext* scriptContext,
        _Out_ RecyclableObject** propertyObject)
    {
        return GetPropertyObjectForElementAccess(instance, index, scriptContext, propertyObject, JSERR_Property_CannotGet_NullOrUndefined);
    }

    BOOL JavascriptOperators::GetPropertyObject(Var instance, ScriptContext * scriptContext, RecyclableObject** propertyObject)
    {
        Assert(propertyObject);
        if (TaggedNumber::Is(instance))
        {
            *propertyObject = scriptContext->GetLibrary()->GetNumberPrototype();
            return TRUE;
        }
        RecyclableObject* object = UnsafeVarTo<RecyclableObject>(instance);
        *propertyObject = object;
        if (JavascriptOperators::IsUndefinedOrNull(object))
        {
            return FALSE;
        }
        return TRUE;
    }

#if DBG
    BOOL JavascriptOperators::IsPropertyObject(RecyclableObject * instance)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(instance);
        return (typeId != TypeIds_Integer && typeId != TypeIds_Null && typeId != TypeIds_Undefined);
    }
#endif

    Var JavascriptOperators::OP_GetProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetProperty);
        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined, scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        Var result = JavascriptOperators::GetPropertyNoCache(instance, object, propertyId, scriptContext);
        AssertMsg(result != nullptr, "result null in OP_GetProperty");
        return result;
        JIT_HELPER_END(Op_GetProperty);
    }

    Var JavascriptOperators::OP_GetRootProperty(Var instance, PropertyId propertyId, PropertyValueInfo * info, ScriptContext* scriptContext)
    {
        AssertMsg(VarIs<RootObjectBase>(instance), "Root must be an object!");

        Var value = nullptr;
        if (JavascriptOperators::GetRootProperty(VarTo<RecyclableObject>(instance), propertyId, &value, scriptContext, info))
        {
            if (scriptContext->IsUndeclBlockVar(value) && scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
            }
            return value;
        }

        const char16* propertyName = scriptContext->GetPropertyName(propertyId)->GetBuffer();

        JavascriptFunction * caller = nullptr;
        if (JavascriptStackWalker::GetCaller(&caller, scriptContext))
        {
            FunctionBody * callerBody = caller->GetFunctionBody();
            if (callerBody && callerBody->GetUtf8SourceInfo()->GetIsXDomain())
            {
                propertyName = nullptr;
            }
        }

        // Don't error if we disabled implicit calls
        if (scriptContext->GetThreadContext()->RecordImplicitException())
        {
            JavascriptError::ThrowReferenceError(scriptContext, JSERR_UndefVariable, propertyName);
        }

        return scriptContext->GetMissingPropertyResult();
    }

    Var JavascriptOperators::OP_GetThisScoped(FrameDisplay *pScope, Var defaultInstance, ScriptContext* scriptContext)
    {
        // NOTE: If changes are made to this logic be sure to update the debuggers as well
        int length = pScope->GetLength();

        for (int i = 0; i < length; i += 1)
        {
            Var value = nullptr;
            RecyclableObject *obj = VarTo<RecyclableObject>(pScope->GetItem(i));
            if (JavascriptOperators::GetProperty(obj, Js::PropertyIds::_this, &value, scriptContext))
            {
                return value;
            }
        }

        return defaultInstance;
    }

    Var JavascriptOperators::OP_UnwrapWithObj(Var aValue)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_UnwrapWithObj);
        return VarTo<UnscopablesWrapperObject>(aValue)->GetWrappedObject();
        JIT_HELPER_END(Op_UnwrapWithObj);
    }
    Var JavascriptOperators::OP_GetInstanceScoped(FrameDisplay *pScope, PropertyId propertyId, Var rootObject, Var* thisVar, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetInstanceScoped);
        // Similar to GetPropertyScoped, but instead of returning the property value, we return the instance that
        // owns it, or the global object if no instance is found.

        int i;
        int length = pScope->GetLength();

        for (i = 0; i < length; i++)
        {
            RecyclableObject *obj = (RecyclableObject*)pScope->GetItem(i);

            if (JavascriptOperators::HasProperty(obj, propertyId))
            {
                // HasProperty will call UnscopablesWrapperObject's HasProperty which will do the filtering
                // All we have to do here is unwrap the object hence the api call

                return obj->GetThisAndUnwrappedInstance(thisVar);
            }
        }

        *thisVar = scriptContext->GetLibrary()->GetUndefined();
        if (rootObject != scriptContext->GetGlobalObject())
        {
            if (JavascriptOperators::OP_HasProperty(rootObject, propertyId, scriptContext))
            {
                return rootObject;
            }
        }

        return scriptContext->GetGlobalObject();
        JIT_HELPER_END(Op_GetInstanceScoped);
    }

    Var JavascriptOperators::GetPropertyReference(RecyclableObject *instance, PropertyId propertyId, ScriptContext* requestContext)
    {
        Var value = nullptr;
        PropertyValueInfo info;
        if (JavascriptOperators::GetPropertyReference(instance, propertyId, &value, requestContext, &info))
        {
            Assert(value != nullptr);
            return value;
        }
        return requestContext->GetMissingPropertyResult();
    }

    BOOL JavascriptOperators::GetPropertyReference(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return GetPropertyReference_Internal(instance, propertyObject, false, propertyId, value, requestContext, info);
    }

    BOOL JavascriptOperators::GetRootPropertyReference(RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return GetPropertyReference_Internal(instance, instance, true, propertyId, value, requestContext, info);
    }

    BOOL JavascriptOperators::PropertyReferenceWalkUnscopable(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return PropertyReferenceWalk_Impl<true>(instance, propertyObject, propertyId, value, info, requestContext);
    }

    BOOL JavascriptOperators::PropertyReferenceWalk(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return PropertyReferenceWalk_Impl<false>(instance, propertyObject, propertyId, value, info, requestContext);
    }

    template <bool unscopables>
    BOOL JavascriptOperators::PropertyReferenceWalk_Impl(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL foundProperty = false;
        RecyclableObject* object = *propertyObject;
        while (!foundProperty && !JavascriptOperators::IsNull(object))
        {
            if (unscopables && JavascriptOperators::IsPropertyUnscopable(object, propertyId))
            {
                break;
            }
            else
            {
                PropertyQueryFlags result = object->GetPropertyReferenceQuery(instance, propertyId, value, info, requestContext);
                if (result != PropertyQueryFlags::Property_NotFound)
                {
                    foundProperty = JavascriptConversion::PropertyQueryFlagsToBoolean(result);
                    break;
                }
            }

            if (object->SkipsPrototype())
            {
                break; // will return false
            }

            object = JavascriptOperators::GetPrototypeNoTrap(object);

        }
        *propertyObject = object;
        return foundProperty;
    }

    BOOL JavascriptOperators::GetPropertyReference_Internal(Var instance, RecyclableObject* propertyObject, const bool isRoot, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        if (TaggedNumber::Is(instance))
        {
            PropertyValueInfo::ClearCacheInfo(info);
        }
        BOOL foundProperty = FALSE;
        RecyclableObject* object = propertyObject;

        if (isRoot)
        {
            foundProperty = VarTo<RootObjectBase>(object)->GetRootPropertyReference(instance, propertyId, value, info, requestContext);
        }
        if (!foundProperty)
        {
            foundProperty = PropertyReferenceWalk(instance, &object, propertyId, value, info, requestContext);
        }

        if (!foundProperty)
        {
#if defined(TELEMETRY_JSO) || defined(TELEMETRY_AddToCache) // enabled for `TELEMETRY_AddToCache`, because this is the property-not-found codepath where the normal TELEMETRY_AddToCache code wouldn't be executed.
            if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
            {
                if (info && info->AllowResizingPolymorphicInlineCache()) // If in interpreted mode, not JIT.
                {
                    requestContext->GetTelemetry().GetOpcodeTelemetry().GetProperty(instance, propertyId, nullptr);
                }
            }
#endif
            *value = requestContext->GetMissingPropertyResult();
            return foundProperty;
        }

        if (requestContext->IsUndeclBlockVar(*value))
        {
            JavascriptError::ThrowReferenceError(requestContext, JSERR_UseBeforeDeclaration);
        }

#if ENABLE_FIXED_FIELDS && DBG
        if (DynamicObject::IsBaseDynamicObject(object))
        {
            DynamicObject* dynamicObject = (DynamicObject*)object;
            DynamicTypeHandler* dynamicTypeHandler = dynamicObject->GetDynamicType()->GetTypeHandler();
            Var property = nullptr;
            if (dynamicTypeHandler->CheckFixedProperty(requestContext->GetPropertyName(propertyId), &property, requestContext))
            {
                Assert(value == nullptr || *value == property);
            }
        }
#endif

        CacheOperators::CachePropertyRead(instance, object, isRoot, propertyId, false, info, requestContext);
        return TRUE;
    }

    template <typename PropertyKeyType, bool unscopable>
    DescriptorFlags JavascriptOperators::GetterSetter_Impl(RecyclableObject* instance, PropertyKeyType propertyKey, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        DescriptorFlags flags = None;
        RecyclableObject* object = instance;
        while (flags == None && !JavascriptOperators::IsNull(object))
        {
            if (unscopable && IsPropertyUnscopable(object, propertyKey))
            {
                break;
            }
            else
            {
                flags = object->GetSetter(propertyKey, setterValue, info, scriptContext);
                if (flags != None)
                {
                    break;
                }
            }
            // CONSIDER: we should add SkipsPrototype support. DOM has no ES 5 concepts built in that aren't
            // already part of our prototype objects which are chakra objects.
            object = object->GetPrototype();
        }
        return flags;
    }

    DescriptorFlags JavascriptOperators::GetterSetterUnscopable(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        return GetterSetter_Impl<PropertyId, true>(instance, propertyId, setterValue, info, scriptContext);
    }

    DescriptorFlags JavascriptOperators::GetterSetter(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        return GetterSetter_Impl<PropertyId, false>(instance, propertyId, setterValue, info, scriptContext);
    }

    DescriptorFlags JavascriptOperators::GetterSetter(RecyclableObject* instance, JavascriptString * propertyName, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        return GetterSetter_Impl<JavascriptString*, false>(instance, propertyName, setterValue, info, scriptContext);
    }

    void JavascriptOperators::OP_InvalidateProtoCaches(PropertyId propertyId, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(InvalidateProtoCaches, reentrancylock, scriptContext->GetThreadContext());
        scriptContext->InvalidateProtoCaches(propertyId);
        JIT_HELPER_END(InvalidateProtoCaches);
    }

    // Checks to see if any object in the prototype chain has a property descriptor for the given index
    // that specifies either an accessor or a non-writable attribute.
    // If TRUE, check flags for details.
    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritableItem(RecyclableObject* instance, uint32 index,
        Var* setterValue, DescriptorFlags *flags, ScriptContext* scriptContext, BOOL skipPrototypeCheck /* = FALSE */)
    {
        Assert(setterValue);
        Assert(flags);

        // Do a quick walk up the prototype chain to see if any of the prototypes has ever had ANY setter or non-writable property.
        if (CheckIfObjectAndPrototypeChainHasOnlyWritableDataProperties(instance))
        {
            return FALSE;
        }

        RecyclableObject* object = instance;
        while (!JavascriptOperators::IsNull(object))
        {
            *flags = object->GetItemSetter(index, setterValue, scriptContext);
            if (*flags != None || skipPrototypeCheck)
            {
                break;
            }
            object = object->GetPrototype();
        }

        return ((*flags & Accessor) == Accessor) || ((*flags & Proxy) == Proxy) || ((*flags & Data) == Data && (*flags & Writable) == None);
    }

    BOOL JavascriptOperators::SetGlobalPropertyNoHost(char16 const * propertyName, charcount_t propertyLength, Var value, ScriptContext * scriptContext)
    {
        GlobalObject * globalObject = scriptContext->GetGlobalObject();
        uint32 index;
        PropertyRecord const * propertyRecord = nullptr;
        IndexType indexType = GetIndexTypeFromString(propertyName, propertyLength, scriptContext, &index, &propertyRecord, true);

        if (indexType == IndexType_Number)
        {
            return globalObject->DynamicObject::SetItem(index, value, PropertyOperation_None);
        }
        return globalObject->DynamicObject::SetProperty(propertyRecord->GetPropertyId(), value, PropertyOperation_None, NULL);
    }

    template<typename PropertyKeyType>
    BOOL JavascriptOperators::SetPropertyWPCache(Var receiver, RecyclableObject* object, PropertyKeyType propertyKey, Var newValue, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags, _Inout_ PropertyValueInfo * info)
    {
        if (receiver)
        {
            AnalysisAssert(object);
            Assert(!TaggedNumber::Is(receiver));
            Var setterValueOrProxy = nullptr;
            DescriptorFlags flags = None;
            if (JavascriptOperators::CheckPrototypesForAccessorOrNonWritableProperty(object, propertyKey, &setterValueOrProxy, &flags, info, requestContext))
            {
                if ((flags & Accessor) == Accessor)
                {
                    if (JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, setterValueOrProxy, requestContext))
                    {
                        return TRUE;
                    }
                    if (setterValueOrProxy)
                    {
                        if (VarIs<UnscopablesWrapperObject>(receiver))
                        {
                            receiver = (UnsafeVarTo<UnscopablesWrapperObject>(receiver))->GetWrappedObject();
                        }
                        else if (info->GetPropertyRecordUsageCache() && !JavascriptOperators::IsUndefinedAccessor(setterValueOrProxy, requestContext))
                        {
                            CacheOperators::CachePropertyWrite(VarTo<RecyclableObject>(receiver), false, object->GetType(), info->GetPropertyRecordUsageCache()->GetPropertyRecord()->GetPropertyId(), info, requestContext);
                        }
                        RecyclableObject* func = VarTo<RecyclableObject>(setterValueOrProxy);

                        JavascriptOperators::CallSetter(func, receiver, newValue, requestContext);
                    }
                    return TRUE;
                }
                else if ((flags & Proxy) == Proxy)
                {
                    Assert(VarIs<JavascriptProxy>(setterValueOrProxy));
                    JavascriptProxy* proxy = VarTo<JavascriptProxy>(setterValueOrProxy);
                    auto fn = [&](RecyclableObject* target) -> BOOL {
                        return JavascriptOperators::SetPropertyWPCache(receiver, target, propertyKey, newValue, requestContext, propertyOperationFlags, info);
                    };
                    if (info->GetPropertyRecordUsageCache())
                    {
                        PropertyValueInfo::SetNoCache(info, proxy);
                        PropertyValueInfo::DisablePrototypeCache(info, proxy);
                    }
                    return proxy->SetPropertyTrap(receiver, JavascriptProxy::SetPropertyTrapKind::SetPropertyWPCacheKind, propertyKey, newValue, requestContext, propertyOperationFlags);
                }
                else
                {
                    Assert((flags & Data) == Data && (flags & Writable) == None);

                    requestContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
                    JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
                    return FALSE;
                }
            }
            else if (!JavascriptOperators::IsObject(receiver))
            {
                JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
                return FALSE;
            }

            RecyclableObject* receiverObject = VarTo<RecyclableObject>(receiver);
            if (receiver != object)
            {
                // If the receiver object has the property and it is an accessor then return false
                PropertyDescriptor existingDesc;
                if (JavascriptOperators::GetOwnPropertyDescriptor(receiverObject, propertyKey, requestContext, &existingDesc)
                    && existingDesc.IsAccessorDescriptor())
                {
                    return FALSE;
                }
            }

            Type *typeWithoutProperty = object->GetType();
            // in 9.1.9, step 5, we should return false if receiver is not object, and that will happen in default RecyclableObject operation anyhow.
            if (receiverObject->SetProperty(propertyKey, newValue, propertyOperationFlags, info))
            {
                if (!VarIs<JavascriptProxy>(receiver) && info->GetPropertyRecordUsageCache() && info->GetFlags() != InlineCacheSetterFlag && !object->IsExternal())
                {
                    CacheOperators::CachePropertyWrite(VarTo<RecyclableObject>(receiver), false, typeWithoutProperty, info->GetPropertyRecordUsageCache()->GetPropertyRecord()->GetPropertyId(), info, requestContext);

                    if (info->GetInstance() == receiverObject)
                    {
                        PropertyValueInfo::SetCacheInfo(info, info->GetPropertyRecordUsageCache()->GetLdElemInlineCache(), info->AllowResizingPolymorphicInlineCache());
                        CacheOperators::CachePropertyRead(object, receiverObject, false, info->GetPropertyRecordUsageCache()->GetPropertyRecord()->GetPropertyId(), false, info, requestContext);
                    }
                }
                return TRUE;
            }
        }

        return FALSE;
    }

    BOOL JavascriptOperators::SetItemOnTaggedNumber(Var receiver, RecyclableObject* object, uint32 index, Var newValue, ScriptContext* requestContext,
        PropertyOperationFlags propertyOperationFlags)
    {
        Assert(TaggedNumber::Is(receiver));

        if (requestContext->optimizationOverrides.GetSideEffects() & SideEffects_Accessor)
        {
            Var setterValueOrProxy = nullptr;
            DescriptorFlags flags = None;
            if (object == nullptr)
            {
                GetPropertyObject(receiver, requestContext, &object);
            }
            if (JavascriptOperators::CheckPrototypesForAccessorOrNonWritableItem(object, index, &setterValueOrProxy, &flags, requestContext))
            {
                if ((flags & Accessor) == Accessor)
                {
                    if (JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, setterValueOrProxy, requestContext))
                    {
                        return TRUE;
                    }
                    if (setterValueOrProxy)
                    {
                        RecyclableObject* func = VarTo<RecyclableObject>(setterValueOrProxy);
                        JavascriptOperators::CallSetter(func, receiver, newValue, requestContext);
                        return TRUE;
                    }
                }
                else if ((flags & Proxy) == Proxy)
                {
                    Assert(VarIs<JavascriptProxy>(setterValueOrProxy));
                    JavascriptProxy* proxy = VarTo<JavascriptProxy>(setterValueOrProxy);
                    const PropertyRecord* propertyRecord = nullptr;
                    proxy->PropertyIdFromInt(index, &propertyRecord);
                    return proxy->SetPropertyTrap(receiver, JavascriptProxy::SetPropertyTrapKind::SetItemOnTaggedNumberKind, propertyRecord->GetPropertyId(), newValue, requestContext, propertyOperationFlags);
                }
                else
                {
                    Assert((flags & Data) == Data && (flags & Writable) == None);
                    JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
                }
            }
        }

        JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
        return FALSE;
    }

    BOOL JavascriptOperators::SetPropertyOnTaggedNumber(Var receiver, RecyclableObject* object, PropertyId propertyId, Var newValue, ScriptContext* requestContext,
        PropertyOperationFlags propertyOperationFlags)
    {
        Assert (TaggedNumber::Is(receiver));

        if (requestContext->optimizationOverrides.GetSideEffects() & SideEffects_Accessor)
        {
            Var setterValueOrProxy = nullptr;
            PropertyValueInfo info;
            DescriptorFlags flags = None;
            if (object == nullptr)
            {
                GetPropertyObject(receiver, requestContext, &object);
            }
            if (JavascriptOperators::CheckPrototypesForAccessorOrNonWritableProperty(object, propertyId, &setterValueOrProxy, &flags, &info, requestContext))
            {
                if ((flags & Accessor) == Accessor)
                {
                    if (JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, setterValueOrProxy, requestContext))
                    {
                        return TRUE;
                    }
                    if (setterValueOrProxy)
                    {
                        RecyclableObject* func = VarTo<RecyclableObject>(setterValueOrProxy);
                        Assert(info.GetFlags() == InlineCacheSetterFlag || info.GetPropertyIndex() == Constants::NoSlot);
                        JavascriptOperators::CallSetter(func, receiver, newValue, requestContext);
                        return TRUE;
                    }
                }
                else if ((flags & Proxy) == Proxy)
                {
                    Assert(VarIs<JavascriptProxy>(setterValueOrProxy));
                    JavascriptProxy* proxy = VarTo<JavascriptProxy>(setterValueOrProxy);
                    return proxy->SetPropertyTrap(receiver, JavascriptProxy::SetPropertyTrapKind::SetPropertyOnTaggedNumberKind, propertyId, newValue, requestContext, propertyOperationFlags);
                }
                else
                {
                    Assert((flags & Data) == Data && (flags & Writable) == None);
                    JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
                }
            }
        }

        // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
        requestContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
        return FALSE;
    }

    BOOL JavascriptOperators::SetPropertyUnscopable(Var instance, RecyclableObject* receiver, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        return SetProperty_Internal<true>(instance, receiver, false, propertyId, newValue, info, requestContext, propertyOperationFlags);
    }

    BOOL JavascriptOperators::SetProperty(Var receiver, RecyclableObject* object, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        return SetProperty_Internal<false>(receiver, object, false, propertyId, newValue, info, requestContext, propertyOperationFlags);
    }

    BOOL JavascriptOperators::SetRootProperty(RecyclableObject* instance, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        return SetProperty_Internal<false>(instance, instance, true, propertyId, newValue, info, requestContext, propertyOperationFlags);
    }

    // Returns true if a result was written.
    bool JavascriptOperators::SetAccessorOrNonWritableProperty(
        Var receiver,
        RecyclableObject* object,
        PropertyId propertyId,
        Var newValue,
        PropertyValueInfo * info,
        ScriptContext* requestContext,
        PropertyOperationFlags propertyOperationFlags,
        bool isRoot,
        bool allowUndecInConsoleScope,
        BOOL *result)
    {
        *result = FALSE;
        Var setterValueOrProxy = nullptr;
        DescriptorFlags flags = None;
        bool receiverNonWritable = false;

        if (receiver != object && !isRoot)
        {
            Var receiverSetter = nullptr;
            PropertyValueInfo receiverInfo;
            DescriptorFlags receiverFlags = VarTo<RecyclableObject>(receiver)->GetSetter(propertyId, &receiverSetter, &receiverInfo, requestContext);
            receiverNonWritable = ((receiverFlags & Data) == Data && (receiverFlags & Writable) == None);
        }

        if ((isRoot && JavascriptOperators::CheckPrototypesForAccessorOrNonWritableRootProperty(object, propertyId, &setterValueOrProxy, &flags, info, requestContext)) ||
            (!isRoot && (JavascriptOperators::CheckPrototypesForAccessorOrNonWritableProperty(object, propertyId, &setterValueOrProxy, &flags, info, requestContext) ||
            receiverNonWritable)))
        {
            if ((flags & Accessor) == Accessor)
            {
                if (JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, setterValueOrProxy, requestContext) ||
                    JavascriptError::ThrowIfNotExtensibleUndefinedSetter(propertyOperationFlags, setterValueOrProxy, requestContext))
                {
                    *result = TRUE;
                    return true;
                }
                if (setterValueOrProxy)
                {
                    RecyclableObject* func = VarTo<RecyclableObject>(setterValueOrProxy);
                    Assert(!info || info->GetFlags() == InlineCacheSetterFlag || info->GetPropertyIndex() == Constants::NoSlot);

                    if (VarIs<UnscopablesWrapperObject>(receiver))
                    {
                        receiver = (UnsafeVarTo<UnscopablesWrapperObject>(receiver))->GetWrappedObject();
                    }
                    else if (!JavascriptOperators::IsUndefinedAccessor(setterValueOrProxy, requestContext))
                    {
                        CacheOperators::CachePropertyWrite(VarTo<RecyclableObject>(receiver), isRoot, object->GetType(), propertyId, info, requestContext);
                    }
#ifdef ENABLE_MUTATION_BREAKPOINT
                    if (MutationBreakpoint::IsFeatureEnabled(requestContext))
                    {
                        MutationBreakpoint::HandleSetProperty(requestContext, object, propertyId, newValue);
                    }
#endif
                    JavascriptOperators::CallSetter(func, receiver, newValue, requestContext);
                }
                *result = TRUE;
                return true;
            }
            else if ((flags & Proxy) == Proxy)
            {
                Assert(VarIs<JavascriptProxy>(setterValueOrProxy));
                JavascriptProxy* proxy = VarTo<JavascriptProxy>(setterValueOrProxy);
                // We can't cache the property at this time. both target and handler can be changed outside of the proxy, so the inline cache needs to be
                // invalidate when target, handler, or handler prototype has changed. We don't have a way to achieve this yet.
                PropertyValueInfo::SetNoCache(info, proxy);
                PropertyValueInfo::DisablePrototypeCache(info, proxy); // We can't cache prototype property either

                *result = proxy->SetPropertyTrap(receiver, JavascriptProxy::SetPropertyTrapKind::SetPropertyKind, propertyId, newValue, requestContext, propertyOperationFlags);
                return true;
            }
            else
            {
                Assert(((flags & Data) == Data && (flags & Writable) == None) || receiverNonWritable);
                if (!allowUndecInConsoleScope)
                {
                    if (flags & Const)
                    {
                        JavascriptError::ThrowTypeError(requestContext, ERRAssignmentToConst);
                    }

                    JavascriptError::ThrowCantAssign(propertyOperationFlags, requestContext, propertyId);
                    JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);

                    *result = FALSE;
                    return true;
                }
            }
        }
        return false;
    }

     template <bool unscopables>
    BOOL JavascriptOperators::SetProperty_Internal(Var receiver, RecyclableObject* object, const bool isRoot, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        if (receiver == nullptr)
        {
            return FALSE;
        }

        Assert(!TaggedNumber::Is(receiver));
        BOOL setAccessorResult = FALSE;
        if (SetAccessorOrNonWritableProperty(receiver, object, propertyId, newValue, info, requestContext, propertyOperationFlags, isRoot, false, &setAccessorResult))
        {
            return setAccessorResult;
        }
        else if (!JavascriptOperators::IsObject(receiver))
        {
            JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, requestContext);
            return FALSE;
        }

#ifdef ENABLE_MUTATION_BREAKPOINT
        // Break on mutation if needed
        bool doNotUpdateCacheForMbp = MutationBreakpoint::IsFeatureEnabled(requestContext) ?
            MutationBreakpoint::HandleSetProperty(requestContext, object, propertyId, newValue) : false;
#endif

        // Get the original type before setting the property
        Type *typeWithoutProperty = object->GetType();
        BOOL didSetProperty = false;
        if (isRoot)
        {
            AssertMsg(JavascriptOperators::GetTypeId(receiver) == TypeIds_GlobalObject
                || JavascriptOperators::GetTypeId(receiver) == TypeIds_ModuleRoot,
                "Root must be a global object!");

            RootObjectBase* rootObject = static_cast<RootObjectBase*>(receiver);
            didSetProperty = rootObject->SetRootProperty(propertyId, newValue, propertyOperationFlags, info);
        }
        else
        {
            RecyclableObject* instanceObject = VarTo<RecyclableObject>(receiver);
            while (!JavascriptOperators::IsNull(instanceObject))
            {
                if (unscopables && JavascriptOperators::IsPropertyUnscopable(instanceObject, propertyId))
                {
                    break;
                }
                else
                {
                    didSetProperty = instanceObject->SetProperty(propertyId, newValue, propertyOperationFlags, info);
                    if (didSetProperty || !unscopables)
                    {
                        break;
                    }
                }
                instanceObject = JavascriptOperators::GetPrototypeNoTrap(instanceObject);
            }
        }

        if (didSetProperty)
        {
            bool updateCache = true;
#ifdef ENABLE_MUTATION_BREAKPOINT
            updateCache = updateCache && !doNotUpdateCacheForMbp;
#endif

            if (updateCache)
            {
                if (!VarIs<JavascriptProxy>(receiver))
                {
                    CacheOperators::CachePropertyWrite(VarTo<RecyclableObject>(receiver), isRoot, typeWithoutProperty, propertyId, info, requestContext);
                }
            }
            return TRUE;
        }

        return FALSE;
    }

    BOOL JavascriptOperators::IsNumberFromNativeArray(Var instance, uint32 index, ScriptContext* scriptContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        Js::TypeId instanceType = JavascriptOperators::GetTypeId(instance);
        // Fast path for native and typed arrays.
        bool isNativeArray = instanceType == TypeIds_NativeIntArray || instanceType == TypeIds_NativeFloatArray;
        bool isTypedArray = instanceType >= TypeIds_Int8Array && instanceType <= TypeIds_Uint64Array;
        if (isNativeArray || isTypedArray)
        {
            // Check if the typed array is detached to prevent an exception in GetOwnItem
            if (isTypedArray && TypedArrayBase::IsDetachedTypedArray(instance))
            {
                return FALSE;
            }
            RecyclableObject* object = VarTo<RecyclableObject>(instance);
            Var member = nullptr;

            // If the item is found in the array own body, then it is a number
            if (JavascriptOperators::GetOwnItem(object, index, &member, scriptContext)
                && !JavascriptOperators::IsUndefined(member))
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    BOOL _Check_return_ _Success_(return) JavascriptOperators::GetAccessors(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, _Out_ Var* getter, _Out_ Var* setter)
    {
        RecyclableObject* object = instance;
        while (!JavascriptOperators::IsNull(object))
        {
            if (object->GetAccessors(propertyId, getter, setter, requestContext))
            {
                *getter = JavascriptOperators::CanonicalizeAccessor(*getter, requestContext);
                *setter = JavascriptOperators::CanonicalizeAccessor(*setter, requestContext);
                return TRUE;
            }

            if (object->SkipsPrototype())
            {
                break;
            }
            object = JavascriptOperators::GetPrototype(object);
        }
        return FALSE;
    }

    BOOL JavascriptOperators::SetAccessors(RecyclableObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        BOOL result = instance && instance->SetAccessors(propertyId, getter, setter, flags);
        return result;
    }

    BOOL JavascriptOperators::OP_SetProperty(Var instance, PropertyId propertyId, Var newValue, ScriptContext* scriptContext, PropertyValueInfo * info, PropertyOperationFlags flags, Var thisInstance)
    {
        // The call into ToObject(dynamicObject) is avoided here by checking for null and undefined and doing nothing when dynamicObject is a primitive value.
        if (thisInstance == nullptr)
        {
            thisInstance = instance;
        }
        TypeId typeId = JavascriptOperators::GetTypeId(instance);

        if (JavascriptOperators::IsUndefinedOrNullType(typeId))
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotSet_NullOrUndefined, scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            return TRUE;
        }

        if (!TaggedNumber::Is(instance) && !TaggedNumber::Is(thisInstance))
        {
            return JavascriptOperators::SetProperty(UnsafeVarTo<RecyclableObject>(thisInstance), UnsafeVarTo<RecyclableObject>(instance), propertyId, newValue, info, scriptContext, flags);
        }

        JavascriptError::ThrowCantAssignIfStrictMode(flags, scriptContext);
        return false;
    }

    BOOL JavascriptOperators::OP_StFunctionExpression(Var obj, PropertyId propertyId, Var newValue)
    {
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_StFunctionExpression, reentrancylock, instance->GetScriptContext()->GetThreadContext());

        instance->SetProperty(propertyId, newValue, PropertyOperation_None, NULL);
        instance->SetWritable(propertyId, FALSE);
        instance->SetConfigurable(propertyId, FALSE);

        return TRUE;
        JIT_HELPER_END(Op_StFunctionExpression);
    }

    BOOL JavascriptOperators::OP_InitClassMember(Var obj, PropertyId propertyId, Var newValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_InitClassMember);
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);

        PropertyOperationFlags flags = PropertyOperation_None;
        PropertyAttributes attributes = PropertyClassMemberDefaults;

        instance->SetPropertyWithAttributes(propertyId, newValue, attributes, NULL, flags);

        return TRUE;
        JIT_HELPER_END(Op_InitClassMember);
    }

    BOOL JavascriptOperators::OP_InitLetProperty(Var obj, PropertyId propertyId, Var newValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_InitLetFld);
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);

        PropertyOperationFlags flags = instance->GetScriptContext()->IsUndeclBlockVar(newValue) ? PropertyOperation_SpecialValue : PropertyOperation_None;
        PropertyAttributes attributes = PropertyLetDefaults;

        if (VarIs<RootObjectBase>(instance))
        {
            attributes |= PropertyLetConstGlobal;
        }

        instance->SetPropertyWithAttributes(propertyId, newValue, attributes, NULL, (PropertyOperationFlags)(flags | PropertyOperation_AllowUndecl));

        return TRUE;
        JIT_HELPER_END(Op_InitLetFld);
    }

    BOOL JavascriptOperators::OP_InitConstProperty(Var obj, PropertyId propertyId, Var newValue)
    {
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);
        JIT_HELPER_REENTRANT_HEADER(Op_InitConstFld);

        PropertyOperationFlags flags = instance->GetScriptContext()->IsUndeclBlockVar(newValue) ? PropertyOperation_SpecialValue : PropertyOperation_None;
        PropertyAttributes attributes = PropertyConstDefaults;

        if (VarIs<RootObjectBase>(instance))
        {
            attributes |= PropertyLetConstGlobal;
        }

        instance->SetPropertyWithAttributes(propertyId, newValue, attributes, NULL, (PropertyOperationFlags)(flags | PropertyOperation_AllowUndecl));

        return TRUE;
        JIT_HELPER_END(Op_InitConstFld);
    }

    BOOL JavascriptOperators::OP_InitUndeclRootLetProperty(Var obj, PropertyId propertyId)
    {
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_InitUndeclRootLetFld, reentrancylock, instance->GetScriptContext()->GetThreadContext());

        PropertyOperationFlags flags = static_cast<PropertyOperationFlags>(PropertyOperation_SpecialValue | PropertyOperation_AllowUndecl);
        PropertyAttributes attributes = PropertyLetDefaults | PropertyLetConstGlobal;

        instance->SetPropertyWithAttributes(propertyId, instance->GetLibrary()->GetUndeclBlockVar(), attributes, NULL, flags);

        return TRUE;
        JIT_HELPER_END(Op_InitUndeclRootLetFld);
    }

    BOOL JavascriptOperators::OP_InitUndeclRootConstProperty(Var obj, PropertyId propertyId)
    {
        RecyclableObject* instance = VarTo<RecyclableObject>(obj);
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_InitUndeclRootConstFld, reentrancylock, instance->GetScriptContext()->GetThreadContext());

        PropertyOperationFlags flags = static_cast<PropertyOperationFlags>(PropertyOperation_SpecialValue | PropertyOperation_AllowUndecl);
        PropertyAttributes attributes = PropertyConstDefaults | PropertyLetConstGlobal;

        instance->SetPropertyWithAttributes(propertyId, instance->GetLibrary()->GetUndeclBlockVar(), attributes, NULL, flags);

        return TRUE;
        JIT_HELPER_END(Op_InitUndeclRootConstFld);
    }

    BOOL JavascriptOperators::OP_InitUndeclConsoleLetProperty(Var obj, PropertyId propertyId)
    {
        FrameDisplay *pScope = (FrameDisplay*)obj;
        AssertMsg(VarIs<ConsoleScopeActivationObject>((DynamicObject*)pScope->GetItem(pScope->GetLength() - 1)), "How come we got this opcode without ConsoleScopeActivationObject?");
        RecyclableObject* instance = VarTo<RecyclableObject>(pScope->GetItem(0));
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_InitUndeclConsoleLetFld, reentrancylock, instance->GetScriptContext()->GetThreadContext());

        PropertyOperationFlags flags = static_cast<PropertyOperationFlags>(PropertyOperation_SpecialValue | PropertyOperation_AllowUndecl);
        PropertyAttributes attributes = PropertyLetDefaults;
        instance->SetPropertyWithAttributes(propertyId, instance->GetLibrary()->GetUndeclBlockVar(), attributes, NULL, flags);
        return TRUE;
        JIT_HELPER_END(Op_InitUndeclConsoleLetFld);
    }

    BOOL JavascriptOperators::OP_InitUndeclConsoleConstProperty(Var obj, PropertyId propertyId)
    {
        FrameDisplay *pScope = (FrameDisplay*)obj;
        AssertMsg(VarIs<ConsoleScopeActivationObject>((DynamicObject*)pScope->GetItem(pScope->GetLength() - 1)), "How come we got this opcode without ConsoleScopeActivationObject?");
        RecyclableObject* instance = VarTo<RecyclableObject>(pScope->GetItem(0));
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_InitUndeclConsoleConstFld, reentrancylock, instance->GetScriptContext()->GetThreadContext());

        PropertyOperationFlags flags = static_cast<PropertyOperationFlags>(PropertyOperation_SpecialValue | PropertyOperation_AllowUndecl);
        PropertyAttributes attributes = PropertyConstDefaults;
        instance->SetPropertyWithAttributes(propertyId, instance->GetLibrary()->GetUndeclBlockVar(), attributes, NULL, flags);
        return TRUE;
        JIT_HELPER_END(Op_InitUndeclConsoleConstFld);
    }

    BOOL JavascriptOperators::InitProperty(RecyclableObject* instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        return instance && instance->InitProperty(propertyId, newValue, flags);
    }

    BOOL JavascriptOperators::OP_InitProperty(Var instance, PropertyId propertyId, Var newValue)
    {
        if(TaggedNumber::Is(instance)) { return false; }
        return JavascriptOperators::InitProperty(VarTo<RecyclableObject>(instance), propertyId, newValue);
    }

    BOOL JavascriptOperators::DeleteProperty(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags)
    {
        return DeleteProperty_Impl<false>(instance, propertyId, propertyOperationFlags);
    }

    bool JavascriptOperators::ShouldTryDeleteProperty(RecyclableObject* instance, JavascriptString *propertyNameString, PropertyRecord const **pPropertyRecord)
    {
        PropertyRecord const *propertyRecord = nullptr;
        if (!JavascriptOperators::CanShortcutOnUnknownPropertyName(instance))
        {
            instance->GetScriptContext()->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
        }
        else
        {
            instance->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);
        }

        if (propertyRecord == nullptr)
        {
            return false;
        }
        *pPropertyRecord = propertyRecord;
        return true;
    }

    BOOL JavascriptOperators::DeleteProperty(RecyclableObject* instance, JavascriptString *propertyNameString, PropertyOperationFlags propertyOperationFlags)
    {
#ifdef ENABLE_MUTATION_BREAKPOINT
        ScriptContext *scriptContext = instance->GetScriptContext();
        if (MutationBreakpoint::IsFeatureEnabled(scriptContext)
            && scriptContext->HasMutationBreakpoints())
        {
            MutationBreakpoint::HandleDeleteProperty(scriptContext, instance, propertyNameString);
        }
#endif
        return instance->DeleteProperty(propertyNameString, propertyOperationFlags);
    }

    BOOL JavascriptOperators::DeletePropertyUnscopables(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags)
    {
        return DeleteProperty_Impl<true>(instance, propertyId, propertyOperationFlags);
    }
    template<bool unscopables>
    BOOL JavascriptOperators::DeleteProperty_Impl(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags)
    {
        if (unscopables && JavascriptOperators::IsPropertyUnscopable(instance, propertyId))
        {
            return false;
        }
#ifdef ENABLE_MUTATION_BREAKPOINT
        ScriptContext *scriptContext = instance->GetScriptContext();
        if (MutationBreakpoint::IsFeatureEnabled(scriptContext)
            && scriptContext->HasMutationBreakpoints())
        {
            MutationBreakpoint::HandleDeleteProperty(scriptContext, instance, propertyId);
        }
#endif
         // !unscopables will hit the return statement on the first iteration
         return instance->DeleteProperty(propertyId, propertyOperationFlags);
    }

    Var JavascriptOperators::OP_DeleteProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_DeleteProperty);
        if(TaggedNumber::Is(instance))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }

        RecyclableObject* recyclableObject = VarTo<RecyclableObject>(instance);
        if (JavascriptOperators::IsUndefinedOrNull(recyclableObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotDelete_NullOrUndefined,
                scriptContext->GetPropertyName(propertyId)->GetBuffer());
        }

        return scriptContext->GetLibrary()->CreateBoolean(
            JavascriptOperators::DeleteProperty(recyclableObject, propertyId, propertyOperationFlags));
        JIT_HELPER_END(Op_DeleteProperty);
    }

    Var JavascriptOperators::OP_DeleteRootProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        // In Edge the root is an External Object which can call Dispose and thus, can have reentrancy.
        JIT_HELPER_REENTRANT_HEADER(Op_DeleteRootProperty);
        AssertMsg(VarIs<RootObjectBase>(instance), "Root must be a global object!");
        RootObjectBase* rootObject = static_cast<RootObjectBase*>(instance);

        return scriptContext->GetLibrary()->CreateBoolean(
            rootObject->DeleteRootProperty(propertyId, propertyOperationFlags));
        JIT_HELPER_END(Op_DeleteRootProperty);
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchSetPropertyScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchSetPropertyScoped);
        // Set the property using a scope stack rather than an individual instance.
        // Walk the stack until we find an instance that has the property and store
        // the new value there.

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        uint16 length = pDisplay->GetLength();
        RecyclableObject *object;

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);

        bool allowUndecInConsoleScope = (propertyOperationFlags & PropertyOperation_AllowUndeclInConsoleScope) == PropertyOperation_AllowUndeclInConsoleScope;
        bool isLexicalThisSlotSymbol = (propertyId == PropertyIds::_this);

        for (uint16 i = 0; i < length; i++)
        {
            object = UnsafeVarTo<RecyclableObject>(pDisplay->GetItem(i));

            AssertMsg(!VarIs<ConsoleScopeActivationObject>(object) || (i == length - 1), "Invalid location for ConsoleScopeActivationObject");

            Type* type = object->GetType();
            if (CacheOperators::TrySetProperty<true, true, true, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
                    object, false, propertyId, newValue, scriptContext, propertyOperationFlags, nullptr, &info))
            {
                return;
            }

            // In scoped set property, we need to set the property when it is available; it could be a setter
            // or normal property. we need to check setter first, and if no setter is available, but HasProperty
            // is true, this must be a normal property.
            // TODO: merge OP_HasProperty and GetSetter in one pass if there is perf problem. In fastDOM we have quite
            // a lot of setters so separating the two might be actually faster.
            BOOL setAccessorResult = FALSE;
            if (SetAccessorOrNonWritableProperty(object, object, propertyId, newValue, &info, scriptContext, propertyOperationFlags, false, allowUndecInConsoleScope, &setAccessorResult))
            {
                return;
            }
            else if (!JavascriptOperators::IsObject(object))
            {
                JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, scriptContext);
            }

            // Need to do a "get" of the current value (if any) to make sure that we're not writing to
            // let/const before declaration, but we need to disable implicit calls around the "get",
            // so we need to do a "has" first to make sure the "get" is valid (e.g., "get" on a HostDispatch
            // with implicit calls disabled will always "succeed").
            if (JavascriptOperators::HasProperty(object, propertyId))
            {
                DisableImplicitFlags disableImplicitFlags = scriptContext->GetThreadContext()->GetDisableImplicitFlags();
                scriptContext->GetThreadContext()->SetDisableImplicitFlags(DisableImplicitCallAndExceptionFlag);

                Var value;
                BOOL result = JavascriptOperators::GetProperty(object, propertyId, &value, scriptContext, nullptr);

                scriptContext->GetThreadContext()->SetDisableImplicitFlags(disableImplicitFlags);

                if (result && scriptContext->IsUndeclBlockVar(value) && !allowUndecInConsoleScope && !isLexicalThisSlotSymbol)
                {
                    JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
                }

                PropertyValueInfo info2;
                PropertyValueInfo::SetCacheInfo(&info2, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
                PropertyOperationFlags setPropertyOpFlags = allowUndecInConsoleScope ? PropertyOperation_AllowUndeclInConsoleScope : PropertyOperation_None;
                object->SetProperty(propertyId, newValue, setPropertyOpFlags, &info2);

#if DBG_DUMP
                if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
                {
                    CacheOperators::TraceCache(inlineCache, _u("PatchSetPropertyScoped"), propertyId, scriptContext, object);
                }
#endif
                if (!VarIs<JavascriptProxy>(object) && !allowUndecInConsoleScope)
                {
                    CacheOperators::CachePropertyWrite(object, false, type, propertyId, &info2, scriptContext);
                }

                return;
            }
        }

        Assert(!isLexicalThisSlotSymbol);

        // If we have console scope and no one in the scope had the property add it to console scope
        if ((length > 0) && VarIs<ConsoleScopeActivationObject>(pDisplay->GetItem(length - 1)))
        {
            // CheckPrototypesForAccessorOrNonWritableProperty does not check for const in global object. We should check it here.
            if (length > 1)
            {
                Js::GlobalObject * globalObject = JavascriptOperators::TryFromVar<Js::GlobalObject>(pDisplay->GetItem(length - 2));
                if (globalObject)
                {
                    Var setterValue = nullptr;

                    DescriptorFlags flags = JavascriptOperators::GetRootSetter(globalObject, propertyId, &setterValue, &info, scriptContext);
                    Assert((flags & Accessor) != Accessor);
                    Assert((flags & Proxy) != Proxy);
                    if ((flags & Data) == Data && (flags & Writable) == None)
                    {
                        if (!allowUndecInConsoleScope)
                        {
                            if (flags & Const)
                            {
                                JavascriptError::ThrowTypeError(scriptContext, ERRAssignmentToConst);
                            }
                            Assert(!isLexicalThisSlotSymbol);
                            return;
                        }
                    }
                }
            }

            RecyclableObject* obj = VarTo<RecyclableObject>(pDisplay->GetItem(length - 1));
            OUTPUT_TRACE(Js::ConsoleScopePhase, _u("Adding property '%s' to console scope object\n"), scriptContext->GetPropertyName(propertyId)->GetBuffer());
            JavascriptOperators::SetProperty(obj, obj, propertyId, newValue, scriptContext, propertyOperationFlags);
            return;
        }

        // No one in the scope stack has the property, so add it to the default instance provided by the caller.
        AssertMsg(!TaggedNumber::Is(defaultInstance), "Root object is an int or tagged float?");
        Assert(defaultInstance != nullptr);
        RecyclableObject* obj = VarTo<RecyclableObject>(defaultInstance);
        {
            //SetPropertyScoped does not use inline cache for default instance
            PropertyValueInfo info2;
            JavascriptOperators::SetRootProperty(obj, propertyId, newValue, &info2, scriptContext, (PropertyOperationFlags)(propertyOperationFlags | PropertyOperation_Root));
        }
        JIT_HELPER_END(Op_PatchSetPropertyScoped);
    }
    JIT_HELPER_TEMPLATE(Op_PatchSetPropertyScoped, Op_ConsolePatchSetPropertyScoped)
    template void JavascriptOperators::PatchSetPropertyScoped<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags propertyOperationFlags);
    template void JavascriptOperators::PatchSetPropertyScoped<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags propertyOperationFlags);
    template void JavascriptOperators::PatchSetPropertyScoped<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags propertyOperationFlags);
    template void JavascriptOperators::PatchSetPropertyScoped<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags propertyOperationFlags);

    BOOL JavascriptOperators::OP_InitFuncScoped(FrameDisplay *pScope, PropertyId propertyId, Var newValue, Var defaultInstance, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_InitFuncScoped, reentrancylock, scriptContext->GetThreadContext());
        int i;
        int length = pScope->GetLength();
        DynamicObject *obj;

        for (i = 0; i < length; i++)
        {
            obj = (DynamicObject*)pScope->GetItem(i);

            if (obj->InitFuncScoped(propertyId, newValue))
            {
                return TRUE;
            }
        }

        AssertMsg(!TaggedNumber::Is(defaultInstance), "Root object is an int or tagged float?");
        return VarTo<RecyclableObject>(defaultInstance)->InitFuncScoped(propertyId, newValue);
        JIT_HELPER_END(Op_InitFuncScoped);
    }

    BOOL JavascriptOperators::OP_InitPropertyScoped(FrameDisplay *pScope, PropertyId propertyId, Var newValue, Var defaultInstance, ScriptContext* scriptContext)
    {
        int i;
        int length = pScope->GetLength();
        DynamicObject *obj;

        for (i = 0; i < length; i++)
        {
            obj = (DynamicObject*)pScope->GetItem(i);
            if (obj->InitPropertyScoped(propertyId, newValue))
            {
                return TRUE;
            }
        }

        AssertMsg(!TaggedNumber::Is(defaultInstance), "Root object is an int or tagged float?");
        return VarTo<RecyclableObject>(defaultInstance)->InitPropertyScoped(propertyId, newValue);
    }

    Var JavascriptOperators::OP_DeletePropertyScoped(
        FrameDisplay *pScope,
        PropertyId propertyId,
        Var defaultInstance,
        ScriptContext* scriptContext,
        PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_DeletePropertyScoped);
        JIT_HELPER_SAME_ATTRIBUTES(Op_DeleteRootProperty, Op_DeletePropertyScoped);
        int i;
        int length = pScope->GetLength();

        for (i = 0; i < length; i++)
        {
            DynamicObject *obj = (DynamicObject*)pScope->GetItem(i);
            if (JavascriptOperators::HasProperty(obj, propertyId))
            {
                return scriptContext->GetLibrary()->CreateBoolean(JavascriptOperators::DeleteProperty(obj, propertyId, propertyOperationFlags));
            }
        }

        return JavascriptOperators::OP_DeleteRootProperty(VarTo<RecyclableObject>(defaultInstance), propertyId, scriptContext, propertyOperationFlags);
        JIT_HELPER_END(Op_DeletePropertyScoped);
    }

    Var JavascriptOperators::OP_TypeofPropertyScoped(FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_TypeofPropertyScoped);
        int i;
        int length = pScope->GetLength();

        for (i = 0; i < length; i++)
        {
            DynamicObject *obj = (DynamicObject*)pScope->GetItem(i);
            if (JavascriptOperators::HasProperty(obj, propertyId))
            {
                return JavascriptOperators::TypeofFld(obj, propertyId, scriptContext);
            }
        }

        return JavascriptOperators::TypeofRootFld(VarTo<RecyclableObject>(defaultInstance), propertyId, scriptContext);
        JIT_HELPER_END(Op_TypeofPropertyScoped);
    }

    BOOL JavascriptOperators::HasOwnItem(RecyclableObject* object, uint32 index)
    {
        return object->HasOwnItem(index);
    }

    BOOL JavascriptOperators::HasItem(RecyclableObject* object, uint64 index)
    {
        PropertyRecord const * propertyRecord = nullptr;
        ScriptContext* scriptContext = object->GetScriptContext();
        JavascriptOperators::GetPropertyIdForInt(index, scriptContext, &propertyRecord);
        return JavascriptOperators::HasProperty(object, propertyRecord->GetPropertyId());
    }

    BOOL JavascriptOperators::HasItem(RecyclableObject* object, uint32 index)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(object);
#endif
        while (!JavascriptOperators::IsNull(object))
        {
            PropertyQueryFlags result;
            if ((result = object->HasItemQuery(index)) != PropertyQueryFlags::Property_NotFound)
            {
                return JavascriptConversion::PropertyQueryFlagsToBoolean(result);
            }
            // CONSIDER: Numeric property values shouldn't be on the prototype for now but if this changes
            // we should add SkipsPrototype support here as well
            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }
        return false;
    }

    BOOL JavascriptOperators::GetOwnItem(RecyclableObject* object, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return object->GetItem(object, index, value, requestContext);
    }

    BOOL JavascriptOperators::GetItem(Var instance, RecyclableObject* propertyObject, uint32 index, Var* value, ScriptContext* requestContext)
    {
        RecyclableObject* object = propertyObject;
        while (!JavascriptOperators::IsNull(object))
        {
            PropertyQueryFlags result;
            if ((result = object->GetItemQuery(instance, index, value, requestContext)) != PropertyQueryFlags::Property_NotFound)
            {
                return JavascriptConversion::PropertyQueryFlagsToBoolean(result);
            }
            if (object->SkipsPrototype())
            {
                break;
            }
            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }
        *value = requestContext->GetMissingItemResult();
        return false;
    }

    BOOL JavascriptOperators::GetItemReference(Var instance, RecyclableObject* propertyObject, uint32 index, Var* value, ScriptContext* requestContext)
    {
        RecyclableObject* object = propertyObject;
        while (!JavascriptOperators::IsNull(object))
        {
            PropertyQueryFlags result;
            if ((result = object->GetItemReferenceQuery(instance, index, value, requestContext)) != PropertyQueryFlags::Property_NotFound)
            {
                return JavascriptConversion::PropertyQueryFlagsToBoolean(result);
            }
            if (object->SkipsPrototype())
            {
                break;
            }
            object = JavascriptOperators::GetPrototypeNoTrap(object);
        }
        *value = requestContext->GetMissingItemResult();
        return false;
    }

    BOOL JavascriptOperators::SetItem(Var receiver, RecyclableObject* object, uint64 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptOperators::GetPropertyIdForInt(index, scriptContext, &propertyRecord);
        return JavascriptOperators::SetProperty(receiver, object, propertyRecord->GetPropertyId(), value, scriptContext, propertyOperationFlags);
    }

    BOOL JavascriptOperators::SetItem(Var receiver, RecyclableObject* object, uint32 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags, BOOL skipPrototypeCheck /* = FALSE */)
    {
        Var setterValueOrProxy = nullptr;
        DescriptorFlags flags = None;
        Assert(!TaggedNumber::Is(receiver));
        if (JavascriptOperators::CheckPrototypesForAccessorOrNonWritableItem(object, index, &setterValueOrProxy, &flags, scriptContext, skipPrototypeCheck))
        {
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
            if ((flags & Accessor) == Accessor)
            {
                if (JavascriptError::ThrowIfStrictModeUndefinedSetter(propertyOperationFlags, setterValueOrProxy, scriptContext) ||
                    JavascriptError::ThrowIfNotExtensibleUndefinedSetter(propertyOperationFlags, setterValueOrProxy, scriptContext))
                {
                    return TRUE;
                }
                if (setterValueOrProxy)
                {
                    RecyclableObject* func = VarTo<RecyclableObject>(setterValueOrProxy);
                    JavascriptOperators::CallSetter(func, receiver, value, scriptContext);
                }
                return TRUE;
            }
            else if ((flags & Proxy) == Proxy)
            {
                Assert(VarIs<JavascriptProxy>(setterValueOrProxy));
                JavascriptProxy* proxy = VarTo<JavascriptProxy>(setterValueOrProxy);
                const PropertyRecord* propertyRecord = nullptr;
                proxy->PropertyIdFromInt(index, &propertyRecord);
                return proxy->SetPropertyTrap(receiver, JavascriptProxy::SetPropertyTrapKind::SetItemKind, propertyRecord->GetPropertyId(), value, scriptContext, propertyOperationFlags, skipPrototypeCheck);
            }
            else
            {
                Assert((flags & Data) == Data && (flags & Writable) == None);
                if ((propertyOperationFlags & PropertyOperationFlags::PropertyOperation_ThrowIfNotExtensible) == PropertyOperationFlags::PropertyOperation_ThrowIfNotExtensible)
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NonExtensibleObject);
                }

                JavascriptError::ThrowCantAssign(propertyOperationFlags, scriptContext, index);
                JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, scriptContext);
                return FALSE;
            }
        }
        else if (!JavascriptOperators::IsObject(receiver))
        {
            JavascriptError::ThrowCantAssignIfStrictMode(propertyOperationFlags, scriptContext);
            return FALSE;
        }

        return (VarTo<RecyclableObject>(receiver))->SetItem(index, value, propertyOperationFlags);
    }

    BOOL JavascriptOperators::DeleteItem(RecyclableObject* object, uint32 index, PropertyOperationFlags propertyOperationFlags)
    {
        return object->DeleteItem(index, propertyOperationFlags);
    }
    BOOL JavascriptOperators::DeleteItem(RecyclableObject* object, uint64 index, PropertyOperationFlags propertyOperationFlags)
    {
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptOperators::GetPropertyIdForInt(index, object->GetScriptContext(), &propertyRecord);
        return JavascriptOperators::DeleteProperty(object, propertyRecord->GetPropertyId(), propertyOperationFlags);
    }

    BOOL JavascriptOperators::OP_HasItem(Var instance, Var index, ScriptContext* scriptContext)
    {
        RecyclableObject* object = TaggedNumber::Is(instance) ?
            scriptContext->GetLibrary()->GetNumberPrototype() :
            VarTo<RecyclableObject>(instance);

        uint32 indexVal;
        PropertyRecord const * propertyRecord = nullptr;
        IndexType indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, false);

        if (indexType == IndexType_Number)
        {
            return HasItem(object, indexVal);
        }
        else
        {
            Assert(indexType == IndexType_PropertyId);
            if (propertyRecord == nullptr && !JavascriptOperators::CanShortcutOnUnknownPropertyName(object))
            {
                indexType = GetIndexTypeFromPrimitive(index, scriptContext, &indexVal, &propertyRecord, true);
                Assert(indexType == IndexType_PropertyId);
                Assert(propertyRecord != nullptr);
            }

            if (propertyRecord != nullptr)
            {
                return HasProperty(object, propertyRecord->GetPropertyId());
            }
            else
            {
#if DBG
                JavascriptString* indexStr = JavascriptConversion::ToString(index, scriptContext);
                PropertyRecord const * debugPropertyRecord;
                scriptContext->GetOrAddPropertyRecord(indexStr, &debugPropertyRecord);
                AssertMsg(!JavascriptOperators::HasProperty(object, debugPropertyRecord->GetPropertyId()), "how did this property come? See OS Bug 2727708 if you see this come from the web");
#endif

                return FALSE;
            }
        }
    }

#if ENABLE_PROFILE_INFO
    void JavascriptOperators::UpdateNativeArrayProfileInfoToCreateVarArray(Var instance, const bool expectingNativeFloatArray, const bool expectingVarArray)
    {
        Assert(instance);
        Assert(expectingNativeFloatArray ^ expectingVarArray);

        JavascriptNativeArray * nativeArr = JavascriptOperators::TryFromVar<JavascriptNativeArray>(instance);
        if (!nativeArr)
        {
            return;
        }

        ArrayCallSiteInfo *const arrayCallSiteInfo = nativeArr->GetArrayCallSiteInfo();
        if (!arrayCallSiteInfo)
        {
            return;
        }

        if (expectingNativeFloatArray)
        {
            // Profile data is expecting a native float array. Ensure that at the array's creation site, that a native int array
            // is not created, such that the profiled array type would be correct.
            arrayCallSiteInfo->SetIsNotNativeIntArray();
        }
        else
        {
            // Profile data is expecting a var array. Ensure that at the array's creation site, that a native array is not
            // created, such that the profiled array type would be correct.
            Assert(expectingVarArray);
            arrayCallSiteInfo->SetIsNotNativeArray();
        }
    }

    bool JavascriptOperators::SetElementMayHaveImplicitCalls(ScriptContext *const scriptContext)
    {
        return
            scriptContext->optimizationOverrides.GetArraySetElementFastPathVtable() ==
                ScriptContextOptimizationOverrideInfo::InvalidVtable;
    }
#endif

    RecyclableObject *JavascriptOperators::GetCallableObjectOrThrow(const Var callee, ScriptContext *const scriptContext)
    {
        Assert(callee);
        Assert(scriptContext);

        if (TaggedNumber::Is(callee))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction /* TODO-ERROR: get arg name - aFunc */);
        }
        return UnsafeVarTo<RecyclableObject>(callee);
    }

    Var JavascriptOperators::OP_GetElementI_JIT(Var instance, Var index, ScriptContext *scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetElementI);
#if ENABLE_NATIVE_CODEGEN
        Assert(Js::JavascriptStackWalker::ValidateTopJitFrame(scriptContext));
#endif
        return OP_GetElementI(instance, index, scriptContext);
        JIT_HELPER_END(Op_GetElementI);
    }

    Var JavascriptOperators::OP_GetElementI_UInt32(Var instance, uint32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetElementI_UInt32);
#if FLOATVAR
        return OP_GetElementI_JIT(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetElementI_JIT(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_GetElementI_UInt32);
    }

    Var JavascriptOperators::OP_GetElementI_Int32(Var instance, int32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetElementI_Int32);
#if FLOATVAR
        return OP_GetElementI_JIT(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetElementI_JIT(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_GetElementI_Int32);
    }

    BOOL JavascriptOperators::GetItemFromArrayPrototype(JavascriptArray * arr, int32 indexInt, Var * result, ScriptContext * scriptContext)
    {
        // try get from Array prototype
        RecyclableObject* prototype = arr->GetPrototype();
        if (JavascriptOperators::GetTypeId(prototype) != TypeIds_Array) //This can be TypeIds_ES5Array (or any other object changed through __proto__).
        {
            return false;
        }

        JavascriptArray* arrayPrototype = UnsafeVarTo<JavascriptArray>(prototype); //Prototype must be Array.prototype (unless changed through __proto__)
        if (arrayPrototype->GetLength() && arrayPrototype->GetItem(arrayPrototype, (uint32)indexInt, result, scriptContext))
        {
            return true;
        }

        prototype = arrayPrototype->GetPrototype(); //Its prototype must be Object.prototype (unless changed through __proto__)
        if (prototype->GetScriptContext()->GetLibrary()->GetObjectPrototype() != prototype)
        {
            return false;
        }

        if (VarTo<DynamicObject>(prototype)->HasNonEmptyObjectArray())
        {
            if (prototype->GetItem(arr, (uint32)indexInt, result, scriptContext))
            {
                return true;
            }
        }

        *result = scriptContext->GetMissingItemResult();
        return true;
    }

    Var JavascriptOperators::GetElementIIntIndex(_In_ Var instance, _In_ Var index, _In_ ScriptContext* scriptContext)
    {
        Assert(TaggedInt::Is(index));

        switch (JavascriptOperators::GetTypeId(instance))
        {
        case TypeIds_Array: //fast path for array
        {
            Var result;
            if (OP_GetElementI_ArrayFastPath(UnsafeVarTo<JavascriptArray>(instance), TaggedInt::ToInt32(index), &result, scriptContext))
            {
                return result;
            }
            break;
        }
        case TypeIds_NativeIntArray:
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
            Var result;
            if (OP_GetElementI_ArrayFastPath(UnsafeVarTo<JavascriptNativeIntArray>(instance), TaggedInt::ToInt32(index), &result, scriptContext))
            {
                return result;
            }
            break;
        }
        case TypeIds_NativeFloatArray:
        {
            Var result;
            if (OP_GetElementI_ArrayFastPath(UnsafeVarTo<JavascriptNativeFloatArray>(instance), TaggedInt::ToInt32(index), &result, scriptContext))
            {
                return result;
            }
            break;
        }

        case TypeIds_String: // fast path for string
        {
            charcount_t indexInt = TaggedInt::ToUInt32(index);
            JavascriptString* string = UnsafeVarTo<JavascriptString>(instance);
            Var result;
            if (JavascriptConversion::PropertyQueryFlagsToBoolean(string->JavascriptString::GetItemQuery(instance, indexInt, &result, scriptContext)))
            {
                return result;
            }
            break;
        }

        case TypeIds_Int8Array:
        {
            // The typed array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Int8VirtualArray>::HasVirtualTable(instance))
            {
                Int8VirtualArray* int8Array = UnsafeVarTo<Int8VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return int8Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Int8Array>::HasVirtualTable(instance))
            {
                Int8Array* int8Array = UnsafeVarTo<Int8Array>(instance);
                if (indexInt >= 0)
                {
                    return int8Array->DirectGetItem(indexInt);
                }
            }
            break;
        }

        case TypeIds_Uint8Array:
        {
            // The typed array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Uint8VirtualArray>::HasVirtualTable(instance))
            {
                Uint8VirtualArray* uint8Array = UnsafeVarTo<Uint8VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return uint8Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Uint8Array>::HasVirtualTable(instance))
            {
                Uint8Array* uint8Array = UnsafeVarTo<Uint8Array>(instance);
                if (indexInt >= 0)
                {
                    return uint8Array->DirectGetItem(indexInt);
                }
            }
            break;
        }

        case TypeIds_Uint8ClampedArray:
        {
            // The typed array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Uint8ClampedVirtualArray>::HasVirtualTable(instance))
            {
                Uint8ClampedVirtualArray* uint8ClampedArray = UnsafeVarTo<Uint8ClampedVirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return uint8ClampedArray->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Uint8ClampedArray>::HasVirtualTable(instance))
            {
                Uint8ClampedArray* uint8ClampedArray = UnsafeVarTo<Uint8ClampedArray>(instance);
                if (indexInt >= 0)
                {
                    return uint8ClampedArray->DirectGetItem(indexInt);
                }
            }
            break;
        }

        case TypeIds_Int16Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);

            if (VirtualTableInfo<Int16VirtualArray>::HasVirtualTable(instance))
            {
                Int16VirtualArray* int16Array = UnsafeVarTo<Int16VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return int16Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Int16Array>::HasVirtualTable(instance))
            {
                Int16Array* int16Array = UnsafeVarTo<Int16Array>(instance);
                if (indexInt >= 0)
                {
                    return int16Array->DirectGetItem(indexInt);
                }
            }
            break;
        }

        case TypeIds_Uint16Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);

            if (VirtualTableInfo<Uint16VirtualArray>::HasVirtualTable(instance))
            {
                Uint16VirtualArray* uint16Array = UnsafeVarTo<Uint16VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return uint16Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Uint16Array>::HasVirtualTable(instance))
            {
                Uint16Array* uint16Array = UnsafeVarTo<Uint16Array>(instance);
                if (indexInt >= 0)
                {
                    return uint16Array->DirectGetItem(indexInt);
                }
            }
            break;
        }
        case TypeIds_Int32Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Int32VirtualArray>::HasVirtualTable(instance))
            {
                Int32VirtualArray* int32Array = UnsafeVarTo<Int32VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return int32Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Int32Array>::HasVirtualTable(instance))
            {
                Int32Array* int32Array = UnsafeVarTo<Int32Array>(instance);
                if (indexInt >= 0)
                {
                    return int32Array->DirectGetItem(indexInt);
                }
            }
            break;

        }
        case TypeIds_Uint32Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Uint32VirtualArray>::HasVirtualTable(instance))
            {
                Uint32VirtualArray* uint32Array = UnsafeVarTo<Uint32VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return uint32Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Uint32Array>::HasVirtualTable(instance))
            {
                Uint32Array* uint32Array = UnsafeVarTo<Uint32Array>(instance);
                if (indexInt >= 0)
                {
                    return uint32Array->DirectGetItem(indexInt);
                }
            }
            break;
        }
        case TypeIds_Float32Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);

            if (VirtualTableInfo<Float32VirtualArray>::HasVirtualTable(instance))
            {
                Float32VirtualArray* float32Array = UnsafeVarTo<Float32VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return float32Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Float32Array>::HasVirtualTable(instance))
            {
                Float32Array* float32Array = UnsafeVarTo<Float32Array>(instance);
                if (indexInt >= 0)
                {
                    return float32Array->DirectGetItem(indexInt);
                }
            }
            break;
        }
        case TypeIds_Float64Array:
        {
            // The type array will deal with all possible values for the index
            int32 indexInt = TaggedInt::ToInt32(index);
            if (VirtualTableInfo<Float64VirtualArray>::HasVirtualTable(instance))
            {
                Float64VirtualArray* float64Array = UnsafeVarTo<Float64VirtualArray>(instance);
                if (indexInt >= 0)
                {
                    return float64Array->DirectGetItem(indexInt);
                }
            }
            else if (VirtualTableInfo<Float64Array>::HasVirtualTable(instance))
            {
                Float64Array* float64Array = UnsafeVarTo<Float64Array>(instance);
                if (indexInt >= 0)
                {
                    return float64Array->DirectGetItem(indexInt);
                }
            }
            break;
        }

        default:
            break;
        }
        return JavascriptOperators::GetElementIHelper(instance, index, instance, scriptContext);
    }

    template <typename T>
    BOOL JavascriptOperators::OP_GetElementI_ArrayFastPath(T * arr, int indexInt, Var * result, ScriptContext * scriptContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arr);
#endif
        if (indexInt >= 0)
        {
            if (!CrossSite::IsCrossSiteObjectTyped(arr))
            {
                if (arr->T::DirectGetVarItemAt((uint32)indexInt, result, scriptContext))
                {
                    return true;
                }
            }
            else
            {
                if (arr->GetItem(arr, (uint32)indexInt, result, scriptContext))
                {
                    return true;
                }
            }
            return GetItemFromArrayPrototype(arr, indexInt, result, scriptContext);
        }
        return false;
    }

    Var JavascriptOperators::OP_GetElementI(Var instance, Var index, ScriptContext* scriptContext)
    {
#ifdef ENABLE_SPECTRE_RUNTIME_MITIGATIONS
        instance = BreakSpeculation(instance);
#endif
        if (TaggedInt::Is(index))
        {
            return GetElementIIntIndex(instance, index, scriptContext);
        }

        if (JavascriptNumber::Is_NoTaggedIntCheck(index))
        {
            uint32 uint32Index = JavascriptConversion::ToUInt32(index, scriptContext);

            if ((double)uint32Index == JavascriptNumber::GetValue(index) && !TaggedInt::IsOverflow(uint32Index))
            {
                index = TaggedInt::ToVarUnchecked(uint32Index);
                return GetElementIIntIndex(instance, index, scriptContext);
            }
        }
        else if (VarIs<RecyclableObject>(instance))
        {
            RecyclableObject* cacheOwner;
            PropertyRecordUsageCache* propertyRecordUsageCache;
            if (GetPropertyRecordUsageCache(index, scriptContext, &propertyRecordUsageCache, &cacheOwner))
            {
                return GetElementIWithCache<false /* ReturnOperationInfo */>(instance, cacheOwner, propertyRecordUsageCache, scriptContext, nullptr);
            }
        }

        return JavascriptOperators::GetElementIHelper(instance, index, instance, scriptContext);
    }

    _Success_(return) bool JavascriptOperators::GetPropertyRecordUsageCache(Var index, ScriptContext* scriptContext, _Outptr_ PropertyRecordUsageCache** propertyRecordUsageCache, _Outptr_ RecyclableObject** cacheOwner)
    {
        JavascriptString* string = JavascriptOperators::TryFromVar<JavascriptString>(index);
        if (string)
        {
            PropertyString * propertyString = nullptr;
            if (VirtualTableInfo<Js::PropertyString>::HasVirtualTable(string))
            {
                propertyString = (PropertyString*)string;
            }
            else if (VirtualTableInfo<Js::LiteralStringWithPropertyStringPtr>::HasVirtualTable(string))
            {
                LiteralStringWithPropertyStringPtr * strWithPtr = (LiteralStringWithPropertyStringPtr *)string;
                if (!strWithPtr->HasPropertyRecord())
                {
                    PropertyRecord const * propertyRecord;
                    strWithPtr->GetPropertyRecord(&propertyRecord); // lookup-cache propertyRecord
                }
                else
                {
                    propertyString = strWithPtr->GetOrAddPropertyString();
                    // this is the second time this property string is used
                    // we already had created the propertyRecord..
                    // now create the propertyString!
                }
            }

            if (propertyString != nullptr)
            {
                *propertyRecordUsageCache = propertyString->GetPropertyRecordUsageCache();
                *cacheOwner = propertyString;
                return true;
            }
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            if (PHASE_TRACE1(PropertyCachePhase))
            {
                Output::Print(_u("PropertyCache: GetElem No property string for '%s'\n"), string->GetString());
            }
#endif
#if DBG_DUMP
            scriptContext->forinNoCache++;
#endif
        }

        JavascriptSymbol* symbol = JavascriptOperators::TryFromVar<JavascriptSymbol>(index);
        if (symbol)
        {
            *propertyRecordUsageCache = symbol->GetPropertyRecordUsageCache();
            *cacheOwner = symbol;
            return true;
        }

        return false;
    }

    bool JavascriptOperators::SetElementIOnTaggedNumber(
        _In_ Var receiver,
        _In_ RecyclableObject* object,
        _In_ Var index,
        _In_ Var value,
        _In_ ScriptContext* requestContext,
        _In_ PropertyOperationFlags propertyOperationFlags)
    {
        Assert(TaggedNumber::Is(receiver));

        uint32 indexVal = 0;
        PropertyRecord const * propertyRecord = nullptr;
        IndexType indexType = GetIndexType(index, requestContext, &indexVal, &propertyRecord, true);
        if (indexType == IndexType_Number)
        {
            return  JavascriptOperators::SetItemOnTaggedNumber(receiver, object, indexVal, value, requestContext, propertyOperationFlags);
        }
        else
        {
            return  JavascriptOperators::SetPropertyOnTaggedNumber(receiver, object, propertyRecord->GetPropertyId(), value, requestContext, propertyOperationFlags);
        }
    }

    template <bool ReturnOperationInfo>
    bool JavascriptOperators::SetElementIWithCache(
        _In_ Var receiver,
        _In_ RecyclableObject* object,
        _In_ RecyclableObject* index,
        _In_ Var value,
        _In_ PropertyRecordUsageCache* propertyRecordUsageCache,
        _In_ ScriptContext* scriptContext,
        _In_ PropertyOperationFlags flags,
        _Inout_opt_ PropertyCacheOperationInfo* operationInfo)
    {
        if (TaggedNumber::Is(receiver))
        {
            return JavascriptOperators::SetElementIOnTaggedNumber(receiver, object, index, value, scriptContext, flags);
        }

        PropertyRecord const * propertyRecord = propertyRecordUsageCache->GetPropertyRecord();
        if (propertyRecord->IsNumeric())
        {
            return JavascriptOperators::SetItem(receiver, object, propertyRecord->GetNumericValue(), value, scriptContext, flags);
        }
        PropertyValueInfo info;
        if (receiver == object)
        {
            if (propertyRecordUsageCache->TrySetPropertyFromCache<ReturnOperationInfo>(object, value, scriptContext, flags, &info, index, operationInfo))
            {
                return true;
            }
        }
        PropertyId propId = propertyRecord->GetPropertyId();
        if (propId == PropertyIds::NaN || propId == PropertyIds::Infinity)
        {
            // As we no longer convert o[x] into o.x for NaN and Infinity, we need to follow SetProperty convention for these,
            // which would check for read-only properties, strict mode, etc.
            // Note that "-Infinity" does not qualify as property name, so we don't have to take care of it.
            return JavascriptOperators::SetProperty(receiver, object, propId, value, scriptContext, flags);
        }
        return JavascriptOperators::SetPropertyWPCache(receiver, object, propId, value, scriptContext, flags, &info);
    }
    template bool JavascriptOperators::SetElementIWithCache<false>(Var receiver, RecyclableObject* object, RecyclableObject* index, Var value, PropertyRecordUsageCache* propertyRecordUsageCache, ScriptContext* scriptContext, PropertyOperationFlags flags, PropertyCacheOperationInfo* operationInfo);
    template bool JavascriptOperators::SetElementIWithCache<true>(Var receiver, RecyclableObject* object, RecyclableObject* index, Var value, PropertyRecordUsageCache* propertyRecordUsageCache, ScriptContext* scriptContext, PropertyOperationFlags flags, PropertyCacheOperationInfo* operationInfo);

    template <bool ReturnOperationInfo>
    Var JavascriptOperators::GetElementIWithCache(
        _In_ Var instance,
        _In_ RecyclableObject* index,
        _In_ PropertyRecordUsageCache* propertyRecordUsageCache,
        _In_ ScriptContext* scriptContext,
        _Inout_opt_ PropertyCacheOperationInfo* operationInfo)
    {
        RecyclableObject* object = nullptr;
        if (!JavascriptOperators::GetPropertyObjectForGetElementI(instance, index, scriptContext, &object))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        PropertyRecord const * propertyRecord = propertyRecordUsageCache->GetPropertyRecord();
        Var value;

        if (propertyRecord->IsNumeric())
        {
            if (JavascriptOperators::GetItem(instance, object, propertyRecord->GetNumericValue(), &value, scriptContext))
            {
                return value;
            }
        }
        else
        {
            PropertyValueInfo info;
            if (propertyRecordUsageCache->TryGetPropertyFromCache<false /* OwnPropertyOnly */, false /* OutputExistence */, ReturnOperationInfo>(instance, object, &value, scriptContext, &info, index, operationInfo))
            {
                return value;
            }
            if (JavascriptOperators::GetPropertyWPCache<false /* OutputExistence */>(instance, object, propertyRecord->GetPropertyId(), &value, scriptContext, &info))
            {
                return value;
            }
        }
        return scriptContext->GetLibrary()->GetUndefined();
    }
    template Var JavascriptOperators::GetElementIWithCache<false>(Var instance, RecyclableObject* index, PropertyRecordUsageCache* propertyRecordUsageCache, ScriptContext* scriptContext, PropertyCacheOperationInfo* operationInfo);
    template Var JavascriptOperators::GetElementIWithCache<true>(Var instance, RecyclableObject* index, PropertyRecordUsageCache* propertyRecordUsageCache, ScriptContext* scriptContext, PropertyCacheOperationInfo* operationInfo);

    Var JavascriptOperators::GetElementIHelper(Var instance, Var index, Var receiver, ScriptContext* scriptContext)
    {
        RecyclableObject* object = nullptr;
        if (!JavascriptOperators::GetPropertyObjectForGetElementI(instance, index, scriptContext, &object))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        uint32 indexVal;
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptString * propertyNameString = nullptr;
        Var value = nullptr;

        IndexType indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, &propertyNameString, false, true);

        if (indexType == IndexType_Number)
        {
            if (JavascriptOperators::GetItem(receiver, object, indexVal, &value, scriptContext))
            {
                return value;
            }
        }
        else if (indexType == IndexType_JavascriptString)
        {
            PropertyValueInfo info;
            if (JavascriptOperators::GetPropertyWPCache<false /* OutputExistence */>(receiver, object, propertyNameString, &value, scriptContext, &info))
            {
                return value;
            }
        }
        else
        {
            Assert(indexType == IndexType_PropertyId);
            if (propertyRecord == nullptr && !JavascriptOperators::CanShortcutOnUnknownPropertyName(object))
            {
                indexType = GetIndexTypeFromPrimitive(index, scriptContext, &indexVal, &propertyRecord, &propertyNameString, true, true);
                Assert(indexType == IndexType_PropertyId);
                Assert(propertyRecord != nullptr);
            }

            if (propertyRecord != nullptr)
            {
                PropertyValueInfo info;
                if (JavascriptOperators::GetPropertyWPCache<false /* OutputExistence */>(receiver, object, propertyRecord->GetPropertyId(), &value, scriptContext, &info))
                {
                    return value;
                }
            }
#if DBG
            else
            {
                JavascriptString* indexStr = JavascriptConversion::ToString(index, scriptContext);
                PropertyRecord const * debugPropertyRecord;
                scriptContext->GetOrAddPropertyRecord(indexStr, &debugPropertyRecord);
                AssertMsg(!JavascriptOperators::GetProperty(receiver, object, debugPropertyRecord->GetPropertyId(), &value, scriptContext), "how did this property come? See OS Bug 2727708 if you see this come from the web");
            }
#endif
        }

        return scriptContext->GetMissingItemResult();
    }

    int32 JavascriptOperators::OP_GetNativeIntElementI(Var instance, Var index)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeIntElementI);
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        if (TaggedInt::Is(index))
        {
            int32 indexInt = TaggedInt::ToInt32(index);
            if (indexInt < 0)
            {
                return JavascriptNativeIntArray::MissingItem;
            }
            JavascriptArray * arr = VarTo<JavascriptArray>(instance);
            int32 result;
            if (arr->DirectGetItemAt((uint32)indexInt, &result))
            {
                return result;
            }
        }
        else if (JavascriptNumber::Is_NoTaggedIntCheck(index))
        {
            int32 indexInt;
            bool isInt32;
            double dIndex = JavascriptNumber::GetValue(index);
            if (JavascriptNumber::TryGetInt32OrUInt32Value(dIndex, &indexInt, &isInt32))
            {
                if (isInt32 && indexInt < 0)
                {
                    return JavascriptNativeIntArray::MissingItem;
                }
                JavascriptArray * arr = VarTo<JavascriptArray>(instance);
                int32 result;
                if (arr->DirectGetItemAt((uint32)indexInt, &result))
                {
                    return result;
                }
            }
        }
        else
        {
            AssertMsg(false, "Non-numerical index in this helper?");
        }

        return JavascriptNativeIntArray::MissingItem;
        JIT_HELPER_END(Op_GetNativeIntElementI);
    }

    int32 JavascriptOperators::OP_GetNativeIntElementI_UInt32(Var instance, uint32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeIntElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetNativeIntElementI_UInt32, Op_GetNativeIntElementI);
#if FLOATVAR
        return OP_GetNativeIntElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext));
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetNativeIntElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer));
#endif
        JIT_HELPER_END(Op_GetNativeIntElementI_UInt32);
    }

    int32 JavascriptOperators::OP_GetNativeIntElementI_Int32(Var instance, int32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeIntElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetNativeIntElementI_Int32, Op_GetNativeIntElementI);
#if FLOATVAR
        return OP_GetNativeIntElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext));
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetNativeIntElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer));
#endif
        JIT_HELPER_END(Op_GetNativeIntElementI_Int32);
    }

    double JavascriptOperators::OP_GetNativeFloatElementI(Var instance, Var index)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeFloatElementI);
        double result = 0;

        if (TaggedInt::Is(index))
        {
            int32 indexInt = TaggedInt::ToInt32(index);
            if (indexInt < 0)
            {
                result = JavascriptNativeFloatArray::MissingItem;
            }
            else
            {
                JavascriptArray * arr = VarTo<JavascriptArray>(instance);
                if (!arr->DirectGetItemAt((uint32)indexInt, &result))
                {
                    result = JavascriptNativeFloatArray::MissingItem;
                }
            }
        }
        else if (JavascriptNumber::Is_NoTaggedIntCheck(index))
        {
            int32 indexInt;
            bool isInt32;
            double dIndex = JavascriptNumber::GetValue(index);
            if (JavascriptNumber::TryGetInt32OrUInt32Value(dIndex, &indexInt, &isInt32))
            {
                if (isInt32 && indexInt < 0)
                {
                    result = JavascriptNativeFloatArray::MissingItem;
                }
                else
                {
                    JavascriptArray * arr = VarTo<JavascriptArray>(instance);
                    if (!arr->DirectGetItemAt((uint32)indexInt, &result))
                    {
                        result = JavascriptNativeFloatArray::MissingItem;
                    }
                }
            }
        }
        else
        {
            AssertMsg(false, "Non-numerical index in this helper?");
        }

        return result;
        JIT_HELPER_END(Op_GetNativeFloatElementI);
    }

    double JavascriptOperators::OP_GetNativeFloatElementI_UInt32(Var instance, uint32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeFloatElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetNativeFloatElementI_UInt32, Op_GetNativeFloatElementI);
#if FLOATVAR
        return OP_GetNativeFloatElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext));
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetNativeFloatElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer));
#endif
        JIT_HELPER_END(Op_GetNativeFloatElementI_UInt32);
    }

    double JavascriptOperators::OP_GetNativeFloatElementI_Int32(Var instance, int32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_GetNativeFloatElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetNativeFloatElementI_Int32, Op_GetNativeFloatElementI);
#if FLOATVAR
        return OP_GetNativeFloatElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext));
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetNativeFloatElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer));
#endif
        JIT_HELPER_END(Op_GetNativeFloatElementI_Int32);
    }

    Var JavascriptOperators::OP_GetMethodElement_UInt32(Var instance, uint32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetMethodElement_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetMethodElement_UInt32, Op_GetMethodElement);
#if FLOATVAR
        return OP_GetMethodElement(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetMethodElement(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_GetMethodElement_UInt32);
    }

    Var JavascriptOperators::OP_GetMethodElement_Int32(Var instance, int32 index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetMethodElement_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_GetMethodElement_Int32, Op_GetMethodElement);
#if FLOATVAR
        return OP_GetElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_GetMethodElement(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext);
#endif
        JIT_HELPER_END(Op_GetMethodElement_Int32);
    }

    Var JavascriptOperators::OP_GetMethodElement(Var instance, Var index, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GetMethodElement);
        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined, GetPropertyDisplayNameForError(index, scriptContext));
        }

        ThreadContext* threadContext = scriptContext->GetThreadContext();
        ImplicitCallFlags savedImplicitCallFlags = threadContext->GetImplicitCallFlags();
        threadContext->ClearImplicitCallFlags();

        uint32 indexVal;
        PropertyRecord const * propertyRecord = nullptr;
        Var value = NULL;
        BOOL hasProperty = FALSE;
        IndexType indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, false);

        if (indexType == IndexType_Number)
        {
            hasProperty = JavascriptOperators::GetItemReference(instance, object, indexVal, &value, scriptContext);
        }
        else
        {
            Assert(indexType == IndexType_PropertyId);

            if (propertyRecord == nullptr && !JavascriptOperators::CanShortcutOnUnknownPropertyName(object))
            {
                indexType = GetIndexTypeFromPrimitive(index, scriptContext, &indexVal, &propertyRecord, true);
                Assert(indexType == IndexType_PropertyId);
                Assert(propertyRecord != nullptr);
            }

            if (propertyRecord != nullptr)
            {
                hasProperty = JavascriptOperators::GetPropertyReference(instance, object, propertyRecord->GetPropertyId(), &value, scriptContext, NULL);
            }
#if DBG
            else
            {
                JavascriptString* indexStr = JavascriptConversion::ToString(index, scriptContext);
                PropertyRecord const * debugPropertyRecord;
                scriptContext->GetOrAddPropertyRecord(indexStr, &debugPropertyRecord);
                AssertMsg(!JavascriptOperators::GetPropertyReference(instance, object, debugPropertyRecord->GetPropertyId(), &value, scriptContext, NULL),
                          "how did this property come? See OS Bug 2727708 if you see this come from the web");
            }
#endif
        }

        if (!hasProperty)
        {
            JavascriptString* varName = nullptr;
            if (indexType == IndexType_PropertyId && propertyRecord != nullptr && propertyRecord->IsSymbol())
            {
                varName = JavascriptSymbol::ToString(propertyRecord, scriptContext);
            }
            else
            {
                varName = JavascriptConversion::ToString(index, scriptContext);
            }

            // ES5 11.2.3 #2: We evaluate the call target but don't throw yet if target member is missing. We need to evaluate argList
            // first (#3). Postpone throwing error to invoke time.
            value = ThrowErrorObject::CreateThrowTypeErrorObject(scriptContext, VBSERR_OLENoPropOrMethod, varName);
        }
        else if(!JavascriptConversion::IsCallable(value))
        {
            // ES5 11.2.3 #2: We evaluate the call target but don't throw yet if target member is missing. We need to evaluate argList
            // first (#3). Postpone throwing error to invoke time.
            JavascriptString* varName = JavascriptConversion::ToString(index, scriptContext);
            value = ThrowErrorObject::CreateThrowTypeErrorObject(scriptContext, JSERR_Property_NeedFunction, varName);
        }

        threadContext->CheckAndResetImplicitCallAccessorFlag();
        threadContext->AddImplicitCallFlags(savedImplicitCallFlags);
        return value;
        JIT_HELPER_END(Op_GetMethodElement);
    }

    BOOL JavascriptOperators::OP_SetElementI_UInt32(Var instance, uint32 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetElementI_UInt32, Op_SetElementI);
#if FLOATVAR
        return OP_SetElementI_JIT(instance, Js::JavascriptNumber::ToVar(index, scriptContext), value, scriptContext, flags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetElementI_JIT(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), value, scriptContext, flags);
#endif
        JIT_HELPER_END(Op_SetElementI_UInt32);
    }

    BOOL JavascriptOperators::OP_SetElementI_Int32(Var instance, int32 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetElementI_Int32, Op_SetElementI);
#if FLOATVAR
        return OP_SetElementI_JIT(instance, Js::JavascriptNumber::ToVar(index, scriptContext), value, scriptContext, flags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetElementI_JIT(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), value, scriptContext, flags);
#endif
        JIT_HELPER_END(Op_SetElementI_Int32);
    }

    BOOL JavascriptOperators::OP_SetElementI_JIT(Var instance, Var index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetElementI);
        if (TaggedNumber::Is(instance))
        {
            return OP_SetElementI(instance, index, value, scriptContext, flags);
        }

        INT_PTR vt = VirtualTableInfoBase::GetVirtualTable(instance);
        OP_SetElementI(instance, index, value, scriptContext, flags);
        return vt != VirtualTableInfoBase::GetVirtualTable(instance);
        JIT_HELPER_END(Op_SetElementI);
    }

    BOOL JavascriptOperators::OP_SetElementI(Var instance, Var index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif

        TypeId instanceType = JavascriptOperators::GetTypeId(instance);

        bool isTypedArray = (instanceType >= TypeIds_Int8Array && instanceType <= TypeIds_Float64Array);

        if (isTypedArray)
        {
            if (TaggedInt::Is(index) || JavascriptNumber::Is_NoTaggedIntCheck(index) || VarIs<JavascriptString>(index))
            {
                BOOL returnValue = FALSE;
                bool isNumericIndex = false;

                // CrossSite types will go down the slow path.

                switch (instanceType)
                {
                case TypeIds_Int8Array:
                {
                    // The typed array will deal with all possible values for the index

                    if (VirtualTableInfo<Int8VirtualArray>::HasVirtualTable(instance))
                    {
                        Int8VirtualArray* int8Array = UnsafeVarTo<Int8VirtualArray>(instance);
                        returnValue = int8Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if( VirtualTableInfo<Int8Array>::HasVirtualTable(instance))
                    {
                        Int8Array* int8Array = UnsafeVarTo<Int8Array>(instance);
                        returnValue = int8Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }

                case TypeIds_Uint8Array:
                {
                    // The typed array will deal with all possible values for the index
                    if (VirtualTableInfo<Uint8VirtualArray>::HasVirtualTable(instance))
                    {
                        Uint8VirtualArray* uint8Array = UnsafeVarTo<Uint8VirtualArray>(instance);
                        returnValue = uint8Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Uint8Array>::HasVirtualTable(instance))
                    {
                        Uint8Array* uint8Array = UnsafeVarTo<Uint8Array>(instance);
                        returnValue = uint8Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }

                case TypeIds_Uint8ClampedArray:
                {
                    // The typed array will deal with all possible values for the index
                    if (VirtualTableInfo<Uint8ClampedVirtualArray>::HasVirtualTable(instance))
                    {
                        Uint8ClampedVirtualArray* uint8ClampedArray = UnsafeVarTo<Uint8ClampedVirtualArray>(instance);
                        returnValue = uint8ClampedArray->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if(VirtualTableInfo<Uint8ClampedArray>::HasVirtualTable(instance))
                    {
                        Uint8ClampedArray* uint8ClampedArray = UnsafeVarTo<Uint8ClampedArray>(instance);
                        returnValue = uint8ClampedArray->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }

                case TypeIds_Int16Array:
                {
                    // The type array will deal with all possible values for the index
                    if (VirtualTableInfo<Int16VirtualArray>::HasVirtualTable(instance))
                    {
                        Int16VirtualArray* int16Array = UnsafeVarTo<Int16VirtualArray>(instance);
                        returnValue = int16Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Int16Array>::HasVirtualTable(instance))
                    {
                        Int16Array* int16Array = UnsafeVarTo<Int16Array>(instance);
                        returnValue = int16Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }

                case TypeIds_Uint16Array:
                {
                    // The type array will deal with all possible values for the index

                    if (VirtualTableInfo<Uint16VirtualArray>::HasVirtualTable(instance))
                    {
                        Uint16VirtualArray* uint16Array = UnsafeVarTo<Uint16VirtualArray>(instance);
                        returnValue = uint16Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Uint16Array>::HasVirtualTable(instance))
                    {
                        Uint16Array* uint16Array = UnsafeVarTo<Uint16Array>(instance);
                        returnValue = uint16Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }
                case TypeIds_Int32Array:
                {
                    // The type array will deal with all possible values for the index
                    if (VirtualTableInfo<Int32VirtualArray>::HasVirtualTable(instance))
                    {
                        Int32VirtualArray* int32Array = UnsafeVarTo<Int32VirtualArray>(instance);
                        returnValue = int32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if(VirtualTableInfo<Int32Array>::HasVirtualTable(instance))
                    {
                        Int32Array* int32Array = UnsafeVarTo<Int32Array>(instance);
                        returnValue = int32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }
                case TypeIds_Uint32Array:
                {
                    // The type array will deal with all possible values for the index

                    if (VirtualTableInfo<Uint32VirtualArray>::HasVirtualTable(instance))
                    {
                        Uint32VirtualArray* uint32Array = UnsafeVarTo<Uint32VirtualArray>(instance);
                        returnValue = uint32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Uint32Array>::HasVirtualTable(instance))
                    {
                        Uint32Array* uint32Array = UnsafeVarTo<Uint32Array>(instance);
                        returnValue = uint32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }
                case TypeIds_Float32Array:
                {
                    // The type array will deal with all possible values for the index
                    if (VirtualTableInfo<Float32VirtualArray>::HasVirtualTable(instance))
                    {
                        Float32VirtualArray* float32Array = UnsafeVarTo<Float32VirtualArray>(instance);
                        returnValue = float32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Float32Array>::HasVirtualTable(instance))
                    {
                        Float32Array* float32Array = UnsafeVarTo<Float32Array>(instance);
                        returnValue = float32Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }
                case TypeIds_Float64Array:
                {
                    // The type array will deal with all possible values for the index
                    if (VirtualTableInfo<Float64VirtualArray>::HasVirtualTable(instance))
                    {
                        Float64VirtualArray* float64Array = UnsafeVarTo<Float64VirtualArray>(instance);
                        returnValue = float64Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    else if (VirtualTableInfo<Float64Array>::HasVirtualTable(instance))
                    {
                        Float64Array* float64Array = UnsafeVarTo<Float64Array>(instance);
                        returnValue = float64Array->ValidateIndexAndDirectSetItem(index, value, &isNumericIndex);
                    }
                    break;
                }
                }

                // if this was numeric index, return operation status else
                // Return the result of calling the default ordinary object [[Set]] internal method (9.1.8) on O passing P, V, and Receiver as arguments.
                if (isNumericIndex)
                    return returnValue;
            }
        }
        else
        {
            if (TaggedInt::Is(index))
            {
            TaggedIntIndex:
                switch (instanceType)
                {
                case TypeIds_NativeIntArray:
                case TypeIds_NativeFloatArray:
                case TypeIds_Array: // fast path for array
                {
                    int indexInt = TaggedInt::ToInt32(index);
                    if (indexInt >= 0 && scriptContext->optimizationOverrides.IsEnabledArraySetElementFastPath())
                    {
                        UnsafeVarTo<JavascriptArray>(instance)->SetItem((uint32)indexInt, value, flags);
                        return TRUE;
                    }
                    break;
                }
                }
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(index))
            {
                double dIndexValue = JavascriptNumber::GetValue(index);
                uint32 uint32Index = JavascriptConversion::ToUInt32(index, scriptContext);

                if ((double)uint32Index == dIndexValue && !TaggedInt::IsOverflow(uint32Index))
                {
                    index = TaggedInt::ToVarUnchecked(uint32Index);
                    goto TaggedIntIndex;
                }
            }
        }

        RecyclableObject* object = nullptr;
        if (!GetPropertyObjectForSetElementI(instance, index, scriptContext, &object))
        {
            return FALSE;
        }

        return JavascriptOperators::SetElementIHelper(instance, object, index, value, scriptContext, flags);
    }

    BOOL JavascriptOperators::SetElementIHelper(Var receiver, RecyclableObject* object, Var index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        IndexType indexType;
        uint32 indexVal = 0;
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptString * propertyNameString = nullptr;
        PropertyValueInfo propertyValueInfo;

        RecyclableObject* cacheOwner;
        PropertyRecordUsageCache* propertyRecordUsageCache;
        if (JavascriptOperators::GetPropertyRecordUsageCache(index, scriptContext, &propertyRecordUsageCache, &cacheOwner))
        {
            return JavascriptOperators::SetElementIWithCache<false>(receiver, object, cacheOwner, value, propertyRecordUsageCache, scriptContext, flags, nullptr);
        }

        if (TaggedNumber::Is(receiver))
        {
            return JavascriptOperators::SetElementIOnTaggedNumber(receiver, object, index, value, scriptContext, flags);
        }

#if DBG_DUMP
        scriptContext->forinNoCache += (!TaggedInt::Is(index) && VarIs<JavascriptString>(index));
#endif
        indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, &propertyNameString, false, true);
        if (scriptContext->GetThreadContext()->IsDisableImplicitCall() &&
            scriptContext->GetThreadContext()->GetImplicitCallFlags() != ImplicitCall_None)
        {
            // We hit an implicit call trying to convert the index, and implicit calls are disabled, so
            // quit before we try to store the element.
            return FALSE;
        }

        if (indexType == IndexType_Number)
        {
SetElementIHelper_INDEX_TYPE_IS_NUMBER:
            return JavascriptOperators::SetItem(receiver, object, indexVal, value, scriptContext, flags);
        }
        else if (indexType == IndexType_JavascriptString)
        {
            Assert(propertyNameString);

            // At this point, we know that the propertyNameString is neither PropertyString
            // or LiteralStringWithPropertyStringPtr.. Get PropertyRecord!
            // we will get it anyways otherwise. (Also, 1:1 string comparison for Builtin types will be expensive.)

            if (propertyRecord == nullptr)
            {
                scriptContext->GetOrAddPropertyRecord(propertyNameString, &propertyRecord);
                if (propertyRecord->IsNumeric())
                {
                    indexVal = propertyRecord->GetNumericValue();
                    goto SetElementIHelper_INDEX_TYPE_IS_NUMBER;
                }
            }
        }

        Assert(indexType == IndexType_PropertyId || indexType == IndexType_JavascriptString);
        Assert(propertyRecord);
        return JavascriptOperators::SetProperty(receiver, object, propertyRecord->GetPropertyId(), value, scriptContext, flags);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI_NoConvert(
        Var instance,
        Var aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeIntElementI_NoConvert, Op_SetNativeIntElementI);
        BOOL converted = OP_SetNativeIntElementI(instance, aElementIndex, iValue, scriptContext, flags);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeIntElementI_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI_UInt32_NoConvert(
        Var instance,
        uint32 aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI_UInt32_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeIntElementI_UInt32_NoConvert, Op_SetNativeIntElementI_UInt32);
        BOOL converted = OP_SetNativeIntElementI_UInt32(instance, aElementIndex, iValue, scriptContext, flags);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeIntElementI_UInt32_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI_Int32_NoConvert(
        Var instance,
        int32 aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI_Int32_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeIntElementI_Int32_NoConvert, Op_SetNativeIntElementI_Int32);
        BOOL converted = OP_SetNativeIntElementI_Int32(instance, aElementIndex, iValue, scriptContext, flags);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeIntElementI_Int32_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI_NoConvert(
        Var instance,
        Var aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeFloatElementI_NoConvert, Op_SetNativeFloatElementI);
        BOOL converted = OP_SetNativeFloatElementI(instance, aElementIndex, scriptContext, flags, dValue);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeFloatElementI_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI_UInt32_NoConvert(
        Var instance,
        uint32 aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI_UInt32_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeFloatElementI_NoConvert, Op_SetNativeFloatElementI_UInt32);
        BOOL converted = OP_SetNativeFloatElementI_UInt32(instance, aElementIndex, scriptContext, flags, dValue);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeFloatElementI_UInt32_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI_Int32_NoConvert(
        Var instance,
        int32 aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI_Int32_NoConvert);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeFloatElementI_NoConvert, Op_SetNativeFloatElementI_Int32);
        BOOL converted = OP_SetNativeFloatElementI_Int32(instance, aElementIndex, scriptContext, flags, dValue);
        if (converted)
        {
            AssertMsg(false, "Unexpected native array conversion");
            Js::Throw::FatalInternalError();
        }
        return FALSE;
        JIT_HELPER_END(Op_SetNativeFloatElementI_Int32_NoConvert);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI(
        Var instance,
        Var aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI);

        INT_PTR vt = (INT_PTR)nullptr;
        vt = VirtualTableInfoBase::GetVirtualTable(instance);

        if (TaggedInt::Is(aElementIndex))
        {
            int32 indexInt = TaggedInt::ToInt32(aElementIndex);
            if (indexInt >= 0 && scriptContext->optimizationOverrides.IsEnabledArraySetElementFastPath())
            {
                JavascriptNativeIntArray *arr = VarTo<JavascriptNativeIntArray>(instance);
                if (!(arr->TryGrowHeadSegmentAndSetItem<int32, JavascriptNativeIntArray>((uint32)indexInt, iValue)))
                {
                    arr->SetItem(indexInt, iValue);
                }
                return vt != VirtualTableInfoBase::GetVirtualTable(instance);
            }
        }

        JavascriptOperators::OP_SetElementI(instance, aElementIndex, JavascriptNumber::ToVar(iValue, scriptContext), scriptContext, flags);
        return vt != VirtualTableInfoBase::GetVirtualTable(instance);
        JIT_HELPER_END(Op_SetNativeIntElementI);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI_UInt32(
        Var instance,
        uint32 aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeIntElementI_UInt32, Op_SetNativeIntElementI);
#if FLOATVAR
        return OP_SetNativeIntElementI(instance, Js::JavascriptNumber::ToVar(aElementIndex, scriptContext), iValue, scriptContext, flags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetNativeIntElementI(instance, Js::JavascriptNumber::ToVarInPlace(aElementIndex, scriptContext,
            (Js::JavascriptNumber *)buffer), iValue, scriptContext, flags);
#endif
        JIT_HELPER_END(Op_SetNativeIntElementI_UInt32);
    }

    BOOL JavascriptOperators::OP_SetNativeIntElementI_Int32(
        Var instance,
        int aElementIndex,
        int32 iValue,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeIntElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeIntElementI_Int32, Op_SetNativeIntElementI);
#if FLOATVAR
        return OP_SetNativeIntElementI(instance, Js::JavascriptNumber::ToVar(aElementIndex, scriptContext), iValue, scriptContext, flags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetNativeIntElementI(instance, Js::JavascriptNumber::ToVarInPlace(aElementIndex, scriptContext,
            (Js::JavascriptNumber *)buffer), iValue, scriptContext, flags);
#endif
        JIT_HELPER_END(Op_SetNativeIntElementI_Int32);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI(
        Var instance,
        Var aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI);

        INT_PTR vt = (INT_PTR)nullptr;
        vt = VirtualTableInfoBase::GetVirtualTable(instance);

        if (TaggedInt::Is(aElementIndex))
        {
            int32 indexInt = TaggedInt::ToInt32(aElementIndex);
            if (indexInt >= 0 && scriptContext->optimizationOverrides.IsEnabledArraySetElementFastPath())
            {
                JavascriptNativeFloatArray *arr = VarTo<JavascriptNativeFloatArray>(instance);
                if (!(arr->TryGrowHeadSegmentAndSetItem<double, JavascriptNativeFloatArray>((uint32)indexInt, dValue)))
                {
                    arr->SetItem(indexInt, dValue);
                }
                return vt != VirtualTableInfoBase::GetVirtualTable(instance);
            }
        }

        JavascriptOperators::OP_SetElementI(instance, aElementIndex, JavascriptNumber::ToVarWithCheck(dValue, scriptContext), scriptContext, flags);
        return vt != VirtualTableInfoBase::GetVirtualTable(instance);
        JIT_HELPER_END(Op_SetNativeFloatElementI);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI_UInt32(
        Var instance,
        uint32 aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeFloatElementI_UInt32, Op_SetNativeFloatElementI);
#if FLOATVAR
        return OP_SetNativeFloatElementI(instance, JavascriptNumber::ToVar(aElementIndex, scriptContext), scriptContext, flags, dValue);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetNativeFloatElementI(instance, JavascriptNumber::ToVarInPlace(aElementIndex, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext, flags, dValue);
#endif
        JIT_HELPER_END(Op_SetNativeFloatElementI_UInt32);
    }

    BOOL JavascriptOperators::OP_SetNativeFloatElementI_Int32(
        Var instance,
        int aElementIndex,
        ScriptContext* scriptContext,
        PropertyOperationFlags flags,
        double dValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_SetNativeFloatElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_SetNativeFloatElementI_Int32, Op_SetNativeFloatElementI);
#if FLOATVAR
        return OP_SetNativeFloatElementI(instance, JavascriptNumber::ToVar(aElementIndex, scriptContext), scriptContext, flags, dValue);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_SetNativeFloatElementI(instance, JavascriptNumber::ToVarInPlace(aElementIndex, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext, flags, dValue);
#endif
        JIT_HELPER_END(Op_SetNativeFloatElementI_Int32);
    }
    BOOL JavascriptOperators::OP_Memcopy(Var dstInstance, int32 dstStart, Var srcInstance, int32 srcStart, int32 length, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_Memcopy, reentrancylock, scriptContext->GetThreadContext());
        if (length <= 0)
        {
            return false;
        }

        TypeId instanceType = JavascriptOperators::GetTypeId(srcInstance);

        if (instanceType != JavascriptOperators::GetTypeId(dstInstance))
        {
            return false;
        }

        if (srcStart != dstStart)
        {
            return false;
        }

        BOOL  returnValue = false;
#define MEMCOPY_TYPED_ARRAY(type, conversion) VarTo< type ## >(dstInstance)->DirectSetItemAtRange( VarTo< type ## >(srcInstance), srcStart, dstStart, length, JavascriptConversion:: ## conversion)
        switch (instanceType)
        {
        case TypeIds_Int8Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Int8Array, ToInt8);
            break;
        }
        case TypeIds_Uint8Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Uint8Array, ToUInt8);
            break;
        }
        case TypeIds_Uint8ClampedArray:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Uint8ClampedArray, ToUInt8Clamped);
            break;
        }
        case TypeIds_Int16Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Int16Array, ToInt16);
            break;
        }
        case TypeIds_Uint16Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Uint16Array, ToUInt16);
            break;
        }
        case TypeIds_Int32Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Int32Array, ToInt32);
            break;
        }
        case TypeIds_Uint32Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Uint32Array, ToUInt32);
            break;
        }
        case TypeIds_Float32Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Float32Array, ToFloat);
            break;
        }
        case TypeIds_Float64Array:
        {
            returnValue = MEMCOPY_TYPED_ARRAY(Float64Array, ToNumber);
            break;
        }
        case TypeIds_Array:
        case TypeIds_NativeFloatArray:
        case TypeIds_NativeIntArray:
        {
            if (dstStart < 0 || srcStart < 0)
            {
                // This is not supported, Bailout
                break;
            }
            // Upper bounds check for source array
            JavascriptArray* srcArray = UnsafeVarTo<JavascriptArray>(srcInstance);
            JavascriptArray* dstArray = VarTo<JavascriptArray>(dstInstance);
            if (scriptContext->optimizationOverrides.IsEnabledArraySetElementFastPath())
            {
                INT_PTR vt = VirtualTableInfoBase::GetVirtualTable(dstInstance);
                if (instanceType == TypeIds_Array)
                {
                    returnValue = dstArray->DirectSetItemAtRangeFromArray<Var>(dstStart, length, srcArray, srcStart);
                }
                else if (instanceType == TypeIds_NativeIntArray)
                {
                    returnValue = dstArray->DirectSetItemAtRangeFromArray<int32>(dstStart, length, srcArray, srcStart);
                }
                else
                {
                    returnValue = dstArray->DirectSetItemAtRangeFromArray<double>(dstStart, length, srcArray, srcStart);
                }
                returnValue &= vt == VirtualTableInfoBase::GetVirtualTable(dstInstance);
            }
            break;
        }
        default:
            AssertMsg(false, "We don't support this type for memcopy yet.");
            break;
        }
#undef MEMCOPY_TYPED_ARRAY
        return returnValue;
        JIT_HELPER_END(Op_Memcopy);
    }

    template<typename T, T(*func)(Var, ScriptContext*)> bool MemsetConversion(Var value, ScriptContext* scriptContext, T* result)
    {
        ImplicitCallFlags flags = scriptContext->GetThreadContext()->TryWithDisabledImplicitCall([&]
        {
            *result = func(value, scriptContext);
        });
        return (flags & (~ImplicitCall_None)) == 0;
    }

    BOOL JavascriptOperators::OP_Memset(Var instance, int32 start, Var value, int32 length, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_Memset, reentrancylock, scriptContext->GetThreadContext());
        if (length <= 0)
        {
            return false;
        }

        TypeId instanceType = JavascriptOperators::GetTypeId(instance);
        BOOL  returnValue = false;

        // The typed array will deal with all possible values for the index
#define MEMSET_TYPED_ARRAY_CASE(type, conversion) \
        case TypeIds_##type: \
        { \
            type## ::TypedArrayType typedValue = 0; \
            if (!MemsetConversion<type## ::TypedArrayType, JavascriptConversion:: ##conversion>(value, scriptContext, &typedValue)) return false; \
            returnValue = VarTo< type## >(instance)->DirectSetItemAtRange(start, length, typedValue); \
            break; \
        }
        switch (instanceType)
        {
        MEMSET_TYPED_ARRAY_CASE(Int8Array, ToInt8)
        MEMSET_TYPED_ARRAY_CASE(Uint8Array, ToUInt8)
        MEMSET_TYPED_ARRAY_CASE(Uint8ClampedArray, ToUInt8Clamped)
        MEMSET_TYPED_ARRAY_CASE(Int16Array, ToInt16)
        MEMSET_TYPED_ARRAY_CASE(Uint16Array, ToUInt16)
        MEMSET_TYPED_ARRAY_CASE(Int32Array, ToInt32)
        MEMSET_TYPED_ARRAY_CASE(Uint32Array, ToUInt32)
        MEMSET_TYPED_ARRAY_CASE(Float32Array, ToFloat)
        MEMSET_TYPED_ARRAY_CASE(Float64Array, ToNumber)
        case TypeIds_NativeFloatArray:
        case TypeIds_NativeIntArray:
        case TypeIds_Array:
        {
            if (start < 0)
            {
                for (start; start < 0 && length > 0; ++start, --length)
                {
                    if (!OP_SetElementI(instance, JavascriptNumber::ToVar(start, scriptContext), value, scriptContext))
                    {
                        return false;
                    }
                }
            }
            if (scriptContext->optimizationOverrides.IsEnabledArraySetElementFastPath())
            {
                INT_PTR vt = VirtualTableInfoBase::GetVirtualTable(instance);
                if (instanceType == TypeIds_Array)
                {
                    returnValue = UnsafeVarTo<JavascriptArray>(instance)->DirectSetItemAtRange<Var>(start, length, value);
                }
                else if (instanceType == TypeIds_NativeIntArray)
                {
                    // Only accept tagged int.
                    if (!TaggedInt::Is(value))
                    {
                        return false;
                    }
                    int32 intValue = 0;
                    if (!MemsetConversion<int32, JavascriptConversion::ToInt32>(value, scriptContext, &intValue))
                    {
                        return false;
                    }
                     // Special case for missing item
                    if (SparseArraySegment<int32>::IsMissingItem(&intValue))
                    {
                        return false;
                    }
                    returnValue = UnsafeVarTo<JavascriptArray>(instance)->DirectSetItemAtRange<int32>(start, length, intValue);
                }
                else
                {
                    // For native float arrays, the jit doesn't check the type of the source so we have to do it here
                    if (!JavascriptNumber::Is(value) && !TaggedNumber::Is(value))
                    {
                        return false;
                    }

                    double doubleValue = 0;
                    if (!MemsetConversion<double, JavascriptConversion::ToNumber>(value, scriptContext, &doubleValue))
                    {
                        return false;
                    }
                    // Special case for missing item
                    if (SparseArraySegment<double>::IsMissingItem(&doubleValue))
                    {
                        return false;
                    }
                    returnValue = UnsafeVarTo<JavascriptArray>(instance)->DirectSetItemAtRange<double>(start, length, doubleValue);
                }
                returnValue &= vt == VirtualTableInfoBase::GetVirtualTable(instance);
            }
            break;
        }
        default:
            AssertMsg(false, "We don't support this type for memset yet.");
            break;
        }

#undef MEMSET_TYPED_ARRAY
        return returnValue;
        JIT_HELPER_END(Op_Memset);
    }

    Var JavascriptOperators::OP_DeleteElementI_UInt32(Var instance, uint32 index, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_DeleteElementI_UInt32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_DeleteElementI_UInt32, Op_DeleteElementI);
#if FLOATVAR
        return OP_DeleteElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext, propertyOperationFlags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_DeleteElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext, propertyOperationFlags);
#endif
        JIT_HELPER_END(Op_DeleteElementI_UInt32);
    }

    Var JavascriptOperators::OP_DeleteElementI_Int32(Var instance, int32 index, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_DeleteElementI_Int32);
        JIT_HELPER_SAME_ATTRIBUTES(Op_DeleteElementI_Int32, Op_DeleteElementI);
#if FLOATVAR
        return OP_DeleteElementI(instance, Js::JavascriptNumber::ToVar(index, scriptContext), scriptContext, propertyOperationFlags);
#else
        char buffer[sizeof(Js::JavascriptNumber)];
        return OP_DeleteElementI(instance, Js::JavascriptNumber::ToVarInPlace(index, scriptContext,
            (Js::JavascriptNumber *)buffer), scriptContext, propertyOperationFlags);
#endif
        JIT_HELPER_END(Op_DeleteElementI_Int32);
    }

    Var JavascriptOperators::OP_DeleteElementI(Var instance, Var index, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_DeleteElementI);
        if(TaggedNumber::Is(instance))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        if (JavascriptOperators::IsUndefinedOrNull(object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotDelete_NullOrUndefined, GetPropertyDisplayNameForError(index, scriptContext));
        }

        uint32 indexVal;
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptString * propertyNameString = nullptr;
        BOOL result = TRUE;
        IndexType indexType = GetIndexType(index, scriptContext, &indexVal, &propertyRecord, &propertyNameString, false, true);

        if (indexType == IndexType_Number)
        {
            result = JavascriptOperators::DeleteItem(object, indexVal, propertyOperationFlags);
        }
        else if (indexType == IndexType_JavascriptString)
        {
            result = JavascriptOperators::DeleteProperty(object, propertyNameString, propertyOperationFlags);
        }
        else
        {
            Assert(indexType == IndexType_PropertyId);

            if (propertyRecord == nullptr && !JavascriptOperators::CanShortcutOnUnknownPropertyName(object))
            {
                indexType = GetIndexTypeFromPrimitive(index, scriptContext, &indexVal, &propertyRecord, true);
                Assert(indexType == IndexType_PropertyId);
                Assert(propertyRecord != nullptr);
            }

            if (propertyRecord != nullptr)
            {
                result = JavascriptOperators::DeleteProperty(object, propertyRecord->GetPropertyId(), propertyOperationFlags);
            }
#if DBG
            else
            {
                JavascriptString* indexStr = JavascriptConversion::ToString(index, scriptContext);
                PropertyRecord const * debugPropertyRecord;
                scriptContext->GetOrAddPropertyRecord(indexStr, &debugPropertyRecord);
                AssertMsg(JavascriptOperators::DeleteProperty(object, debugPropertyRecord->GetPropertyId(), propertyOperationFlags), "delete should have been true. See OS Bug 2727708 if you see this come from the web");
            }
#endif
        }

        Assert(result || !(propertyOperationFlags & (PropertyOperation_StrictMode | PropertyOperation_ThrowOnDeleteIfNotConfig)));
        return scriptContext->GetLibrary()->CreateBoolean(result);
        JIT_HELPER_END(Op_DeleteElementI);
    }

    Var JavascriptOperators::OP_ToPropertyKey(Js::Var argument, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_ConvPropertyKey);
        PropertyRecord const* unused = nullptr;
        return JavascriptConversion::ToPropertyKey(argument, scriptContext, &unused, nullptr);
        JIT_HELPER_END(Op_ConvPropertyKey);
    }

    Var JavascriptOperators::OP_GetLength(Var instance, ScriptContext* scriptContext)
    {
        return JavascriptOperators::OP_GetProperty(instance, PropertyIds::length, scriptContext);
    }

    Var JavascriptOperators::GetThisFromModuleRoot(Var thisVar)
    {
        RootObjectBase * rootObject = static_cast<RootObjectBase*>(thisVar);
        RecyclableObject* hostObject = rootObject->GetHostObject();

        //
        // if the module root has the host object, use that as "this"
        //
        if (hostObject)
        {
            thisVar = hostObject->GetHostDispatchVar();
        }
        return thisVar;
    }

    inline void JavascriptOperators::TryLoadRoot(Var& thisVar, TypeId typeId, int moduleID, ScriptContextInfo* scriptContext)
    {
        bool loadRoot = false;
        if (JavascriptOperators::IsUndefinedOrNullType(typeId) || typeId == TypeIds_ActivationObject)
        {
            loadRoot = true;
        }
        else if (typeId == TypeIds_HostDispatch)
        {
            TypeId remoteTypeId = TypeIds_Limit;
            if (VarTo<RecyclableObject>(thisVar)->GetRemoteTypeId(&remoteTypeId))
            {
                if (remoteTypeId == TypeIds_Null || remoteTypeId == TypeIds_Undefined || remoteTypeId == TypeIds_ActivationObject)
                {
                    loadRoot = true;
                }
            }
        }

        if (loadRoot)
        {
            if (moduleID == 0)
            {
                thisVar = (Js::Var)scriptContext->GetGlobalObjectThisAddr();
            }
            else
            {
                // TODO: OOP JIT, create a copy of module roots in server side
                Js::ModuleRoot * moduleRoot = JavascriptOperators::GetModuleRoot(moduleID, (ScriptContext*)scriptContext);
                if (moduleRoot == nullptr)
                {
                    Assert(false);
                    thisVar = (Js::Var)scriptContext->GetUndefinedAddr();
                }
                else
                {
                    thisVar = GetThisFromModuleRoot(moduleRoot);
                }
            }
        }
    }

    Var JavascriptOperators::OP_GetThis(Var thisVar, int moduleID, ScriptContextInfo* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(LdThis);
        //
        // if "this" is null or undefined
        //   Pass the global object
        // Else
        //   Pass ToObject(this)
        //
        TypeId typeId = JavascriptOperators::GetTypeId(thisVar);

        Assert(!JavascriptOperators::IsThisSelf(typeId));

        return JavascriptOperators::GetThisHelper(thisVar, typeId, moduleID, scriptContext);
        JIT_HELPER_END(LdThis);
    }

    Var JavascriptOperators::OP_GetThisNoFastPath(Var thisVar, int moduleID, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(LdThisNoFastPath);
        TypeId typeId = JavascriptOperators::GetTypeId(thisVar);

        if (JavascriptOperators::IsThisSelf(typeId))
        {
            Assert(typeId != TypeIds_GlobalObject || ((Js::GlobalObject*)thisVar)->ToThis() == thisVar);
            Assert(typeId != TypeIds_ModuleRoot || JavascriptOperators::GetThisFromModuleRoot(thisVar) == thisVar);

            return thisVar;
        }

        return JavascriptOperators::GetThisHelper(thisVar, typeId, moduleID, scriptContext);
        JIT_HELPER_END(LdThisNoFastPath);
    }

    bool JavascriptOperators::IsThisSelf(TypeId typeId)
    {
        return (JavascriptOperators::IsObjectType(typeId) && ! JavascriptOperators::IsSpecialObjectType(typeId));
    }

    Var JavascriptOperators::GetThisHelper(Var thisVar, TypeId typeId, int moduleID, ScriptContextInfo *scriptContext)
    {
        if (! JavascriptOperators::IsObjectType(typeId) && ! JavascriptOperators::IsUndefinedOrNullType(typeId))
        {
#if ENABLE_NATIVE_CODEGEN
            Assert(!JITManager::GetJITManager()->IsJITServer());
#endif
#if !FLOATVAR
            // We allowed stack number to be used as the "this" for getter and setter activation of
            // n.x and n[prop], where n is the Javascript Number
            return JavascriptOperators::ToObject(
                JavascriptNumber::BoxStackNumber(thisVar, (ScriptContext*)scriptContext), (ScriptContext*)scriptContext);
#else
            return JavascriptOperators::ToObject(thisVar, (ScriptContext*)scriptContext);
#endif

        }
        else
        {
            TryLoadRoot(thisVar, typeId, moduleID, scriptContext);
            return thisVar;
        }
    }

    BOOL JavascriptOperators::GetRemoteTypeId(Var aValue, __out TypeId* typeId)
    {
        *typeId = TypeIds_Limit;
        if (GetTypeId(aValue) != TypeIds_HostDispatch)
        {
            return FALSE;
        }
        return VarTo<RecyclableObject>(aValue)->GetRemoteTypeId(typeId);
    }

    BOOL JavascriptOperators::IsJsNativeType(TypeId type)
    {
        switch(type)
        {
            case TypeIds_Object:
            case TypeIds_Function:
            case TypeIds_Array:
            case TypeIds_NativeIntArray:
#if ENABLE_COPYONACCESS_ARRAY
            case TypeIds_CopyOnAccessNativeIntArray:
#endif
            case TypeIds_NativeFloatArray:
            case TypeIds_ES5Array:
            case TypeIds_Date:
            case TypeIds_RegEx:
            case TypeIds_Error:
            case TypeIds_BooleanObject:
            case TypeIds_NumberObject:
            case TypeIds_StringObject:
            case TypeIds_Symbol:
            case TypeIds_SymbolObject:
            //case TypeIds_GlobalObject:
            //case TypeIds_ModuleRoot:
            //case TypeIds_HostObject:
            case TypeIds_Arguments:
            case TypeIds_ActivationObject:
            case TypeIds_Map:
            case TypeIds_Set:
            case TypeIds_WeakMap:
            case TypeIds_WeakSet:
            case TypeIds_ArrayIterator:
            case TypeIds_MapIterator:
            case TypeIds_SetIterator:
            case TypeIds_StringIterator:
            case TypeIds_Generator:
            case TypeIds_AsyncGenerator:
            case TypeIds_AsyncFromSyncIterator:
            case TypeIds_Promise:
            case TypeIds_Proxy:
                return true;
            default:
                return false;
        }
    }

    BOOL JavascriptOperators::IsJsNativeObject(Var instance)
    {
        return IsJsNativeType(GetTypeId(instance));
    }

    BOOL JavascriptOperators::IsJsNativeObject(_In_ RecyclableObject* instance)
    {
        return IsJsNativeType(GetTypeId(instance));
    }

    bool JavascriptOperators::CanShortcutOnUnknownPropertyName(RecyclableObject *instance)
    {
        if (!CanShortcutInstanceOnUnknownPropertyName(instance))
        {
            return false;
        }
        return CanShortcutPrototypeChainOnUnknownPropertyName(instance->GetPrototype());
    }

    bool JavascriptOperators::CanShortcutInstanceOnUnknownPropertyName(RecyclableObject *instance)
    {
        if (PHASE_OFF1(Js::OptUnknownElementNamePhase))
        {
            return false;
        }

        TypeId typeId = instance->GetTypeId();
        if (typeId == TypeIds_Proxy || typeId == TypeIds_HostDispatch)
        {
            return false;
        }
        if (DynamicType::Is(typeId) &&
            static_cast<DynamicObject*>(instance)->GetTypeHandler()->IsStringTypeHandler())
        {
            return false;
        }
        if (instance->IsExternal())
        {
            return false;
        }

        if (!(instance->HasDeferredTypeHandler()))
        {
            JavascriptFunction * function = JavascriptOperators::TryFromVar<JavascriptFunction>(instance);
            return function && function->IsExternalFunction();
        }
        return false;
    }

    bool JavascriptOperators::CanShortcutPrototypeChainOnUnknownPropertyName(RecyclableObject *prototype)
    {
        Assert(prototype);
        for (; !JavascriptOperators::IsNull(prototype); prototype = prototype->GetPrototype())
        {
            if (!CanShortcutInstanceOnUnknownPropertyName(prototype))
            {
                return false;
            }
        }
        return true;
    }

    RecyclableObject* JavascriptOperators::GetPrototype(RecyclableObject* instance)
    {
        if (JavascriptOperators::GetTypeId(instance) == TypeIds_Null)
        {
            return instance;
        }
        return instance->GetPrototype();
    }

    RecyclableObject* JavascriptOperators::OP_GetPrototype(Var instance, ScriptContext* scriptContext)
    {
        if (TaggedNumber::Is(instance))
        {
            return scriptContext->GetLibrary()->GetNumberPrototype();
        }
        else
        {
            RecyclableObject* object = VarTo<RecyclableObject>(instance);
            if (JavascriptOperators::IsNull(object))
            {
                return object;
            }

            return JavascriptOperators::GetPrototype(object);
        }
    }

     BOOL JavascriptOperators::OP_BrFncEqApply(Var instance, ScriptContext *scriptContext)
     {
         JIT_HELPER_NOT_REENTRANT_HEADER(Op_OP_BrFncEqApply, reentrancylock, scriptContext->GetThreadContext());
         // JavascriptFunction && !HostDispatch
         if (JavascriptOperators::GetTypeId(instance) == TypeIds_Function)
         {
             FunctionProxy *bod= ((JavascriptFunction*)instance)->GetFunctionProxy();
             if (bod != nullptr)
             {
                 return bod->GetDirectEntryPoint(bod->GetDefaultEntryPointInfo()) == &Js::JavascriptFunction::EntryApply;
             }
             else
             {
                 FunctionInfo* info = ((JavascriptFunction *)instance)->GetFunctionInfo();
                 if (info != nullptr)
                 {
                     return &Js::JavascriptFunction::EntryApply == info->GetOriginalEntryPoint();
                 }
                 else
                 {
                     return false;
                 }
             }
         }

         return false;
         JIT_HELPER_END(Op_OP_BrFncEqApply);
     }

     BOOL JavascriptOperators::OP_BrFncNeqApply(Var instance, ScriptContext *scriptContext)
     {
         JIT_HELPER_NOT_REENTRANT_HEADER(Op_OP_BrFncNeqApply, reentrancylock, scriptContext->GetThreadContext());
         // JavascriptFunction and !HostDispatch
         if (JavascriptOperators::GetTypeId(instance) == TypeIds_Function)
         {
             FunctionProxy *bod = ((JavascriptFunction *)instance)->GetFunctionProxy();
             if (bod != nullptr)
             {
                 return bod->GetDirectEntryPoint(bod->GetDefaultEntryPointInfo()) != &Js::JavascriptFunction::EntryApply;
             }
             else
             {
                 FunctionInfo* info = ((JavascriptFunction *)instance)->GetFunctionInfo();
                 if (info != nullptr)
                 {
                     return &Js::JavascriptFunction::EntryApply != info->GetOriginalEntryPoint();
                 }
                 else
                 {
                     return true;
                 }
             }
         }

         return true;
         JIT_HELPER_END(Op_OP_BrFncNeqApply);
     }

    BOOL JavascriptOperators::OP_BrHasSideEffects(int se, ScriptContext* scriptContext)
    {
        return (scriptContext->optimizationOverrides.GetSideEffects() & se) != SideEffects_None;
    }

    BOOL JavascriptOperators::OP_BrNotHasSideEffects(int se, ScriptContext* scriptContext)
    {
        return (scriptContext->optimizationOverrides.GetSideEffects() & se) == SideEffects_None;
    }

    // returns NULL if there is no more elements to enumerate.
    Var JavascriptOperators::OP_BrOnEmpty(ForInObjectEnumerator * aEnumerator)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_OP_BrOnEmpty);
        PropertyId id;
        return aEnumerator->MoveAndGetNext(id);
        JIT_HELPER_END(Op_OP_BrOnEmpty);
    }

    void JavascriptOperators::OP_InitForInEnumerator(Var enumerable, ForInObjectEnumerator * enumerator, ScriptContext* scriptContext, EnumeratorCache * forInCache)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_OP_InitForInEnumerator);
        RecyclableObject* enumerableObject;
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(enumerable);
#endif
        if (!GetPropertyObject(enumerable, scriptContext, &enumerableObject))
        {
            enumerableObject = nullptr;
        }

        enumerator->Initialize(enumerableObject, scriptContext, false, forInCache);
        JIT_HELPER_END(Op_OP_InitForInEnumerator);
    }

    Js::Var JavascriptOperators::OP_CmEq_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmEq_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::Equal(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmEq_A);
    }

    Var JavascriptOperators::OP_CmNeq_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmNeq_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::NotEqual(a,b,scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmNeq_A);
    }

    Var JavascriptOperators::OP_CmSrEq_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmSrEq_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::StrictEqual(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmSrEq_A);
    }

    Var JavascriptOperators::OP_CmSrEq_String(Var a, JavascriptString* b, ScriptContext *scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmSrEq_String);
        return JavascriptBoolean::ToVar(JavascriptOperators::StrictEqualString(a, b), scriptContext);
        JIT_HELPER_END(OP_CmSrEq_String);
    }

    Var JavascriptOperators::OP_CmSrEq_EmptyString(Var a, ScriptContext *scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmSrEq_EmptyString);
        return JavascriptBoolean::ToVar(JavascriptOperators::StrictEqualEmptyString(a), scriptContext);
        JIT_HELPER_END(OP_CmSrEq_EmptyString);
    }

    Var JavascriptOperators::OP_CmSrNeq_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmSrNeq_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::NotStrictEqual(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmSrNeq_A);
    }

    Var JavascriptOperators::OP_CmLt_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmLt_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::Less(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmLt_A);
    }

    Var JavascriptOperators::OP_CmLe_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmLe_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::LessEqual(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmLe_A);
    }

    Var JavascriptOperators::OP_CmGt_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmGt_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::Greater(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmGt_A);
    }

    Var JavascriptOperators::OP_CmGe_A(Var a, Var b, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_CmGe_A);
        return JavascriptBoolean::ToVar(JavascriptOperators::GreaterEqual(a, b, scriptContext), scriptContext);
        JIT_HELPER_END(OP_CmGe_A);
    }

    DetachedStateBase* JavascriptOperators::DetachVarAndGetState(Var var, bool queueForDelayFree/* = true*/)
    {
        switch (GetTypeId(var))
        {
        case TypeIds_ArrayBuffer:
            return Js::VarTo<Js::ArrayBuffer>(var)->DetachAndGetState(queueForDelayFree);
        default:
            if (!Js::VarTo<Js::RecyclableObject>(var)->IsExternal())
            {
                AssertMsg(false, "We should explicitly have a case statement for each non-external object that can be detached.");
            }
            return nullptr;
        }
    }

    bool JavascriptOperators::IsObjectDetached(Var var)
    {
        switch (GetTypeId(var))
        {
        case TypeIds_ArrayBuffer:
            return Js::VarTo<Js::ArrayBuffer>(var)->IsDetached();
        default:
            return false;
        }
    }

    Var JavascriptOperators::NewVarFromDetachedState(DetachedStateBase* state, JavascriptLibrary *library)
    {
        AssertOrFailFastMsg(state->GetTypeId() == TypeIds_ArrayBuffer, "We should only re-activate detached ArrayBuffer");
        return Js::ArrayBuffer::NewFromDetachedState(state, library);
    }

    DynamicType *
    JavascriptOperators::EnsureObjectLiteralType(ScriptContext* scriptContext, const Js::PropertyIdArray *propIds, Field(DynamicType*)* literalType)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(EnsureObjectLiteralType, reentrancylock, scriptContext->GetThreadContext());
        DynamicType * newType = *literalType;
        if (newType != nullptr)
        {
            if (!newType->GetIsShared())
            {
                newType->ShareType();
            }
        }
        else
        {
            DynamicType* objectType =
                FunctionBody::DoObjectHeaderInliningForObjectLiteral(propIds)
                    ?   scriptContext->GetLibrary()->GetObjectHeaderInlinedLiteralType((uint16)propIds->count)
                    :   scriptContext->GetLibrary()->GetObjectLiteralType(
                            static_cast<PropertyIndex>(
                                min(propIds->count, static_cast<uint32>(MaxPreInitializedObjectTypeInlineSlotCount))));
            newType = PathTypeHandlerBase::CreateTypeForNewScObject(scriptContext, objectType, propIds, false);
            *literalType = newType;
        }

        Assert(scriptContext);
        Assert(GetLiteralInlineSlotCapacity(propIds) == newType->GetTypeHandler()->GetInlineSlotCapacity());
        Assert(newType->GetTypeHandler()->GetSlotCapacity() >= 0);
        Assert(GetLiteralSlotCapacity(propIds) == (uint)newType->GetTypeHandler()->GetSlotCapacity());
        return newType;
        JIT_HELPER_END(EnsureObjectLiteralType);
    }

    Var JavascriptOperators::NewScObjectLiteral(ScriptContext* scriptContext, const Js::PropertyIdArray *propIds, Field(DynamicType*)* literalType)
    {
        Assert(propIds->count != 0);
        Assert(!propIds->hadDuplicates);        // duplicates are removed by parser

#ifdef PROFILE_OBJECT_LITERALS
        // Empty objects not counted in the object literal counts
        scriptContext->objectLiteralInstanceCount++;
        if (propIds->count > scriptContext->objectLiteralMaxLength)
        {
            scriptContext->objectLiteralMaxLength = propIds->count;
        }
#endif

        DynamicType* newType = EnsureObjectLiteralType(scriptContext, propIds, literalType);
        DynamicObject* instance = DynamicObject::New(scriptContext->GetRecycler(), newType);

        if (!newType->GetIsShared())
        {
#if ENABLE_FIXED_FIELDS
            newType->GetTypeHandler()->SetSingletonInstanceIfNeeded(instance);
#endif
        }
#ifdef PROFILE_OBJECT_LITERALS
        else
        {
            scriptContext->objectLiteralCacheCount++;
        }
#endif
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(instance));
        // can't auto-proxy here as object literal is not exactly "new" object and cannot be intercepted as proxy.
        return instance;
    }

    uint JavascriptOperators::GetLiteralSlotCapacity(Js::PropertyIdArray const * propIds)
    {
        const uint inlineSlotCapacity = GetLiteralInlineSlotCapacity(propIds);
        return DynamicTypeHandler::RoundUpSlotCapacity(propIds->count, static_cast<PropertyIndex>(inlineSlotCapacity));
    }

    uint JavascriptOperators::GetLiteralInlineSlotCapacity(
        Js::PropertyIdArray const * propIds)
    {
        if (propIds->hadDuplicates)
        {
            return 0;
        }

        return
            FunctionBody::DoObjectHeaderInliningForObjectLiteral(propIds)
                ?   DynamicTypeHandler::RoundUpObjectHeaderInlinedInlineSlotCapacity(static_cast<PropertyIndex>(propIds->count))
                :   DynamicTypeHandler::RoundUpInlineSlotCapacity(
                        static_cast<PropertyIndex>(
                            min(propIds->count, static_cast<uint32>(MaxPreInitializedObjectTypeInlineSlotCount))));
    }

    Var JavascriptOperators::OP_InitCachedScope(Var varFunc, const Js::PropertyIdArray *propIds, Field(DynamicType*)* literalType, bool formalsAreLetDecls, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_InitCachedScope, reentrancylock, scriptContext->GetThreadContext());
        bool isGAFunction = VarIs<JavascriptFunction>(varFunc);
        Assert(isGAFunction);
        if (isGAFunction)
        {
            JavascriptFunction *function = VarTo<JavascriptFunction>(varFunc);
            isGAFunction = JavascriptGeneratorFunction::Test(function) || JavascriptAsyncFunction::Test(function);
        }

        ScriptFunction *func = isGAFunction ?
            VarTo<JavascriptGeneratorFunction>(varFunc)->GetGeneratorVirtualScriptFunction() :
            VarTo<ScriptFunction>(varFunc);

#ifdef PROFILE_OBJECT_LITERALS
        // Empty objects not counted in the object literal counts
        scriptContext->objectLiteralInstanceCount++;
        if (propIds->count > scriptContext->objectLiteralMaxLength)
        {
            scriptContext->objectLiteralMaxLength = propIds->count;
        }
#endif

        PropertyId cachedFuncCount = ActivationObjectEx::GetCachedFuncCount(propIds);
        PropertyId firstFuncSlot = ActivationObjectEx::GetFirstFuncSlot(propIds);
        PropertyId firstVarSlot = ActivationObjectEx::GetFirstVarSlot(propIds);
        PropertyId lastFuncSlot = Constants::NoProperty;

        if (firstFuncSlot != Constants::NoProperty)
        {
            if (firstVarSlot == Constants::NoProperty || firstVarSlot < firstFuncSlot)
            {
                lastFuncSlot = propIds->count - 1;
            }
            else
            {
                lastFuncSlot = firstVarSlot - 1;
            }
        }

        DynamicType *type = *literalType;
        if (type != nullptr)
        {
#ifdef PROFILE_OBJECT_LITERALS
            scriptContext->objectLiteralCacheCount++;
#endif
        }
        else
        {
            type = scriptContext->GetLibrary()->GetActivationObjectType();
            if (formalsAreLetDecls)
            {
                uint formalsSlotLimit = (firstFuncSlot != Constants::NoProperty) ? (uint)firstFuncSlot :
                                        (firstVarSlot != Constants::NoProperty) ? (uint)firstVarSlot :
                                        propIds->count;
                if (func->GetFunctionBody()->HasReferenceableBuiltInArguments())
                {
                    type = PathTypeHandlerBase::CreateNewScopeObject<true>(scriptContext, type, propIds, PropertyLet, formalsSlotLimit);
                }
                else
                {
                    type = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, type, propIds, PropertyLet, formalsSlotLimit);
                }
            }
            else
            {
                type = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, type, propIds);
            }
            *literalType = type;
        }
        Var undef = scriptContext->GetLibrary()->GetUndefined();

        ActivationObjectEx *scopeObjEx = func->GetCachedScope();
        if (scopeObjEx && scopeObjEx->IsCommitted())
        {
            scopeObjEx->ReplaceType(type);
            scopeObjEx->SetCommit(false);
#if DBG
            for (uint i = firstVarSlot; i < propIds->count; i++)
            {
                AssertMsg(scopeObjEx->GetSlot(i) == undef, "Var attached to cached scope");
            }
#endif
        }
        else
        {
            ActivationObjectEx *tmp = RecyclerNewPlus(scriptContext->GetRecycler(), (cachedFuncCount == 0 ? 0 : cachedFuncCount - 1) * sizeof(FuncCacheEntry), ActivationObjectEx, type, func, cachedFuncCount, firstFuncSlot, lastFuncSlot);
            if (!scopeObjEx)
            {
                func->SetCachedScope(tmp);
            }
            scopeObjEx = tmp;

            for (uint i = firstVarSlot; i < propIds->count; i++)
            {
                scopeObjEx->SetSlot(SetSlotArguments(propIds->elements[i], i, undef));
            }
        }

        return scopeObjEx;
        JIT_HELPER_END(OP_InitCachedScope);
    }

    void JavascriptOperators::OP_InvalidateCachedScope(void* varEnv, int32 envIndex)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_InvalidateCachedScope);
        FrameDisplay *disp = (FrameDisplay*)varEnv;
        Var item = disp->GetItem(envIndex);
        if (item != nullptr)
        {
            Assert(VarIs<ActivationObjectEx>(item));
            RecyclableObject *objScope = VarTo<RecyclableObject>(item);
            objScope->InvalidateCachedScope();
        }
        JIT_HELPER_END(OP_InvalidateCachedScope);
    }

    void JavascriptOperators::OP_InitCachedFuncs(Var varScope, FrameDisplay *pDisplay, const FuncInfoArray *info, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_InitCachedFuncs, reentrancylock, scriptContext->GetThreadContext());
        ActivationObjectEx *scopeObj = VarTo<ActivationObjectEx>(varScope);
        Assert(scopeObj->GetTypeHandler()->GetInlineSlotCapacity() == 0);

        uint funcCount = info->count;

        if (funcCount == 0)
        {
            // Degenerate case: no nested funcs at all
            return;
        }

        if (scopeObj->HasCachedFuncs())
        {
            for (uint i = 0; i < funcCount; i++)
            {
                const FuncCacheEntry *entry = scopeObj->GetFuncCacheEntry(i);
                ScriptFunction *func = entry->func;

                FunctionProxy * proxy = func->GetFunctionProxy();

                // Reset the function's type to the default type with no properties
                // Use the cached type on the function proxy rather than the type in the func cache entry
                // CONSIDER: Stop caching the function types in the scope object
                func->ReplaceType(proxy->EnsureDeferredPrototypeType());
                func->ResetConstructorCacheToDefault();

                uint scopeSlot = info->elements[i].scopeSlot;
                if (scopeSlot != Constants::NoProperty)
                {
                    // CONSIDER: Store property IDs in FuncInfoArray in debug builds so we can properly assert in SetAuxSlot
                    scopeObj->SetAuxSlot(SetSlotArguments(Constants::NoProperty, scopeSlot, entry->func));
                }
            }
            return;
        }

        // No cached functions, so create them and cache them.
        JavascriptFunction *funcParent = scopeObj->GetParentFunc();
        for (uint i = 0; i < funcCount; i++)
        {
            const FuncInfoEntry *entry = &info->elements[i];
            uint nestedIndex = entry->nestedIndex;
            uint scopeSlot = entry->scopeSlot;

            FunctionProxy * proxy = funcParent->GetFunctionBody()->GetNestedFunctionProxy(nestedIndex);

            ScriptFunction *func = scriptContext->GetLibrary()->CreateScriptFunction(proxy);

            func->SetEnvironment(pDisplay);
            JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_FUNCTION(func, EtwTrace::GetFunctionId(proxy)));

            scopeObj->SetCachedFunc(i, func);
            if (scopeSlot != Constants::NoProperty)
            {
                // CONSIDER: Store property IDs in FuncInfoArray in debug builds so we can properly assert in SetAuxSlot
                scopeObj->SetAuxSlot(SetSlotArguments(Constants::NoProperty, scopeSlot, func));
            }
        }
        JIT_HELPER_END(OP_InitCachedFuncs);
    }

    Var JavascriptOperators::AddVarsToArraySegment(SparseArraySegment<Var> * segment, const Js::VarArray *vars)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ArraySegmentVars);
        uint32 count = vars->count;

        Assert(segment->left == 0);
        Assert(count <= segment->size);

        if(count > segment->length)
        {
            segment->length = count;
            segment->CheckLengthvsSize();
        }
        CopyArray(segment->elements, segment->length, vars->elements, count);

        return segment;
        JIT_HELPER_END(ArraySegmentVars);
    }

    void JavascriptOperators::AddIntsToArraySegment(SparseArraySegment<int32> * segment, const Js::AuxArray<int32> *ints)
    {
        uint32 count = ints->count;

        Assert(segment->left == 0);
        Assert(count <= segment->size);

        if(count > segment->length)
        {
            segment->length = count;
            segment->CheckLengthvsSize();
        }
        js_memcpy_s(segment->elements, sizeof(int32) * segment->length, ints->elements, sizeof(int32) * count);
    }

    void JavascriptOperators::AddFloatsToArraySegment(SparseArraySegment<double> * segment, const Js::AuxArray<double> *doubles)
    {
        uint32 count = doubles->count;

        Assert(segment->left == 0);
        Assert(count <= segment->size);

        if(count > segment->length)
        {
            segment->length = count;
            segment->CheckLengthvsSize();
        }
        js_memcpy_s(segment->elements, sizeof(double) * segment->length, doubles->elements, sizeof(double) * count);
    }

    RecyclableObject * JavascriptOperators::GetPrototypeObject(RecyclableObject * constructorFunction, ScriptContext * scriptContext)
    {
        Var prototypeProperty = JavascriptOperators::GetProperty(constructorFunction, PropertyIds::prototype, scriptContext);
        RecyclableObject* prototypeObject;
        PrototypeObject(prototypeProperty, constructorFunction, scriptContext, &prototypeObject);
        return prototypeObject;
    }

    RecyclableObject * JavascriptOperators::GetPrototypeObjectForConstructorCache(RecyclableObject * constructor, ScriptContext* requestContext, bool& canBeCached)
    {
        PropertyValueInfo info;
        Var prototypeValue;
        RecyclableObject* prototypeObject;

        canBeCached = false;

        // Do a local property lookup.  Since a function's prototype property is a non-configurable data property, we don't need to worry
        // about the prototype being an accessor property, whose getter returns different values every time it's called.
        if (constructor->GetProperty(constructor, PropertyIds::prototype, &prototypeValue, &info, requestContext))
        {
            if (!JavascriptOperators::PrototypeObject(prototypeValue, constructor, requestContext, &prototypeObject))
            {
                // The value returned by the property lookup is not a valid prototype object, default to object prototype.
                Assert(prototypeObject == constructor->GetLibrary()->GetObjectPrototype());
            }

            // For these scenarios, we do not want to populate the cache.
            if (constructor->GetScriptContext() != requestContext || info.GetInstance() != constructor)
            {
                return prototypeObject;
            }
        }
        else
        {
            // It's ok to cache Object.prototype, because Object.prototype cannot be overwritten.
            prototypeObject = constructor->GetLibrary()->GetObjectPrototype();
        }

        canBeCached = true;
        return prototypeObject;
    }

    bool JavascriptOperators::PrototypeObject(Var prototypeProperty, RecyclableObject * constructorFunction, ScriptContext * scriptContext, RecyclableObject** prototypeObject)
    {
        TypeId prototypeType = JavascriptOperators::GetTypeId(prototypeProperty);

        if (JavascriptOperators::IsObjectType(prototypeType))
        {
            *prototypeObject = VarTo<RecyclableObject>(prototypeProperty);
            return true;
        }
        *prototypeObject = constructorFunction->GetLibrary()->GetObjectPrototype();
        return false;
    }

    FunctionInfo* JavascriptOperators::GetConstructorFunctionInfo(Var instance, ScriptContext * scriptContext)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(instance);
        if (typeId == TypeIds_Function)
        {
            JavascriptFunction * function =  UnsafeVarTo<JavascriptFunction>(instance);
            return function->GetFunctionInfo();
        }
        if (typeId != TypeIds_HostDispatch && typeId != TypeIds_Proxy)
        {
            if (typeId == TypeIds_Null)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
            }

            JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
        }
        return nullptr;
    }

    Var JavascriptOperators::NewJavascriptObjectNoArg(ScriptContext* requestContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(NewJavascriptObjectNoArg, reentrancylock, requestContext->GetThreadContext());
        DynamicObject * newObject = requestContext->GetLibrary()->CreateObject(true);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(newObject));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            newObject = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(newObject));
        }
#endif
        return newObject;
        JIT_HELPER_END(NewJavascriptObjectNoArg);
    }

    Var JavascriptOperators::NewJavascriptArrayNoArg(ScriptContext* requestContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(NewJavascriptArrayNoArg, reentrancylock, requestContext->GetThreadContext());
        JavascriptArray * newArray = requestContext->GetLibrary()->CreateArray();
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(newArray));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            newArray = static_cast<JavascriptArray*>(JavascriptProxy::AutoProxyWrapper(newArray));
        }
#endif
        return newArray;
        JIT_HELPER_END(NewJavascriptArrayNoArg);
    }

    Var JavascriptOperators::NewScObjectNoArgNoCtorFull(Var instance, ScriptContext* requestContext)
    {
        // This helper can be reentrant because although we don't call the Constructor, we might have to parse it if bytecode is missing
        // In which case, we would leave script. When we leave script we DisposeObjects which can dispose of Edge objects that could
        // have a javascript onDispose handler and call that handler.
        JIT_HELPER_REENTRANT_HEADER(NewScObjectNoArgNoCtorFull);
        return NewScObjectNoArgNoCtorCommon(instance, requestContext, true);
        JIT_HELPER_END(NewScObjectNoArgNoCtorFull);
    }

    Var JavascriptOperators::NewScObjectNoArgNoCtor(Var instance, ScriptContext* requestContext)
    {
        // This helper can be reentrant because although we don't call the Constructor, we might have to parse it if bytecode is missing
        // In which case, we would leave script. When we leave script we DisposeObjects which can dispose of Edge objects that could
        // have a javascript onDispose handler and call that handler.
        JIT_HELPER_REENTRANT_HEADER(NewScObjectNoArgNoCtor);
        return NewScObjectNoArgNoCtorCommon(instance, requestContext, false);
        JIT_HELPER_END(NewScObjectNoArgNoCtor);
    }

    Var JavascriptOperators::NewScObjectNoArgNoCtorCommon(Var instance, ScriptContext* requestContext, bool isBaseClassConstructorNewScObject)
    {
        RecyclableObject * object = VarTo<RecyclableObject>(instance);
        FunctionInfo* functionInfo = JavascriptOperators::GetConstructorFunctionInfo(instance, requestContext);
        Assert(functionInfo != &JavascriptObject::EntryInfo::NewInstance); // built-ins are not inlined
        Assert(functionInfo != &JavascriptArray::EntryInfo::NewInstance); // built-ins are not inlined

        return functionInfo != nullptr ?
            JavascriptOperators::NewScObjectCommon(object, functionInfo, requestContext, isBaseClassConstructorNewScObject) :
            JavascriptOperators::NewScObjectHostDispatchOrProxy(object, requestContext);
    }

    Var JavascriptOperators::NewScObjectNoArg(Var instance, ScriptContext * requestContext)
    {
        JIT_HELPER_REENTRANT_HEADER(NewScObjectNoArg);
        JavascriptProxy * proxy = JavascriptOperators::TryFromVar<JavascriptProxy>(instance);
        if (proxy)
        {
            Var dummy = nullptr;
            Arguments args(CallInfo(CallFlags_New, 1), &dummy);
            return requestContext->GetThreadContext()->ExecuteImplicitCall(proxy, Js::ImplicitCall_Accessor, [=]()->Js::Var
            {
                return proxy->ConstructorTrap(args, requestContext, 0);
            });
        }

        FunctionInfo* functionInfo = JavascriptOperators::GetConstructorFunctionInfo(instance, requestContext);
        RecyclableObject * object = VarTo<RecyclableObject>(instance);

        if (functionInfo == &JavascriptObject::EntryInfo::NewInstance)
        {
            // Fast path for new Object()
            Assert((functionInfo->GetAttributes() & FunctionInfo::ErrorOnNew) == 0);
            JavascriptLibrary* library = object->GetLibrary();

            DynamicObject * newObject = library->CreateObject(true);
            JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(newObject));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
            {
                newObject = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(newObject));
            }
#endif

#if DBG
            DynamicType* newObjectType = newObject->GetDynamicType();
            Assert(newObjectType->GetIsShared());

            JavascriptFunction* constructor = VarTo<JavascriptFunction>(instance);
            Assert(!constructor->GetConstructorCache()->NeedsUpdateAfterCtor());
#endif

            ScriptContext * scriptContext = library->GetScriptContext();
            if (scriptContext != requestContext)
            {
                CrossSite::MarshalDynamicObjectAndPrototype(requestContext, newObject);
            }

            return newObject;
        }
        else if (functionInfo == &JavascriptArray::EntryInfo::NewInstance)
        {
            Assert((functionInfo->GetAttributes() & FunctionInfo::ErrorOnNew) == 0);
            JavascriptLibrary* library = object->GetLibrary();

            JavascriptArray * newArray = library->CreateArray();
            JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(newArray));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
            {
                newArray = static_cast<JavascriptArray*>(JavascriptProxy::AutoProxyWrapper(newArray));
            }
#endif

#if DBG
            DynamicType* newArrayType = newArray->GetDynamicType();
            Assert(newArrayType->GetIsShared());

            JavascriptFunction* constructor = VarTo<JavascriptFunction>(instance);
            Assert(!constructor->GetConstructorCache()->NeedsUpdateAfterCtor());
#endif

            ScriptContext * scriptContext = library->GetScriptContext();
            if (scriptContext != requestContext)
            {
                CrossSite::MarshalDynamicObjectAndPrototype(requestContext, newArray);
            }
            return newArray;
        }

        Var newObject = functionInfo != nullptr ?
            JavascriptOperators::NewScObjectCommon(object, functionInfo, requestContext) :
            JavascriptOperators::NewScObjectHostDispatchOrProxy(object, requestContext);

        ThreadContext * threadContext = object->GetScriptContext()->GetThreadContext();
        Var returnVar = threadContext->ExecuteImplicitCall(object, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(threadContext, object, CallInfo(CallFlags_New, 1), newObject);
        });
        if (JavascriptOperators::IsObject(returnVar))
        {
            newObject = returnVar;
        }

        ConstructorCache * constructorCache = nullptr;
        JavascriptFunction *function = JavascriptOperators::TryFromVar<JavascriptFunction>(instance);
        if (function)
        {
            constructorCache = function->GetConstructorCache();
        }

        if (constructorCache != nullptr && constructorCache->NeedsUpdateAfterCtor())
        {
            JavascriptOperators::UpdateNewScObjectCache(object, newObject, requestContext);
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            DynamicObject* newDynamicObject = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(newObject));
            // this might come from a different scriptcontext.
            newObject = CrossSite::MarshalVar(requestContext, newDynamicObject, newDynamicObject->GetScriptContext());
        }
#endif

        return newObject;
        JIT_HELPER_END(NewScObjectNoArg);
    }

    Var JavascriptOperators::NewScObjectNoCtorFull(Var instance, ScriptContext* requestContext)
    {
        // This helper can be reentrant because although we don't call the Constructor, we might have to parse it if bytecode is missing
        // In which case, we would leave script. When we leave script we DisposeObjects which can dispose of Edge objects that could
        // have a javascript onDispose handler and call that handler.
        JIT_HELPER_REENTRANT_HEADER(NewScObjectNoCtorFull);
        return NewScObjectNoCtorCommon(instance, requestContext, true);
        JIT_HELPER_END(NewScObjectNoCtorFull);
    }

    Var JavascriptOperators::NewScObjectNoCtor(Var instance, ScriptContext * requestContext)
    {
        // This helper can be reentrant because although we don't call the Constructor, we might have to parse it if bytecode is missing
        // In which case, we would leave script. When we leave script we DisposeObjects which can dispose of Edge objects that could
        // have a javascript onDispose handler and call that handler.
        JIT_HELPER_REENTRANT_HEADER(NewScObjectNoCtor);
        // We can still call into NewScObjectNoCtor variations in JIT code for performance; however for proxy we don't
        // really need the new object as the trap will handle the "this" pointer separately. pass back nullptr to ensure
        // failure in invalid case.
        return (VarIs<JavascriptProxy>(instance)) ? nullptr : NewScObjectNoCtorCommon(instance, requestContext, false);
        JIT_HELPER_END(NewScObjectNoCtor);
    }

    Var JavascriptOperators::NewScObjectNoCtorCommon(Var instance, ScriptContext* requestContext, bool isBaseClassConstructorNewScObject)
    {
        FunctionInfo* functionInfo = JavascriptOperators::GetConstructorFunctionInfo(instance, requestContext);

        if (functionInfo)
        {
            return JavascriptOperators::NewScObjectCommon(UnsafeVarTo<RecyclableObject>(instance), functionInfo, requestContext, isBaseClassConstructorNewScObject);
        }
        else
        {
            return JavascriptOperators::NewScObjectHostDispatchOrProxy(VarTo<RecyclableObject>(instance), requestContext);
        }
    }

    Var JavascriptOperators::NewScObjectHostDispatchOrProxy(RecyclableObject * function, ScriptContext * requestContext)
    {
        ScriptContext* functionScriptContext = function->GetScriptContext();

        RecyclableObject * prototype = JavascriptOperators::GetPrototypeObject(function, functionScriptContext);
        prototype = VarTo<RecyclableObject>(CrossSite::MarshalVar(requestContext, prototype, functionScriptContext));

        Var object = requestContext->GetLibrary()->CreateObject(prototype);
        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(object));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            object = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(object));
        }
#endif
        return object;
    }

    Var JavascriptOperators::NewScObjectCommon(RecyclableObject * function, FunctionInfo* functionInfo, ScriptContext * requestContext, bool isBaseClassConstructorNewScObject)
    {
        // CONSIDER: Allow for the cache to be repopulated if the type got collected, and a new one got populated with
        // the same number of inlined slots. This requires that the JIT-ed code actually load the type from the cache
        // (instead of hard-coding it), but it can (and must) keep the hard-coded number of inline slots.
        // CONSIDER: Consider also not pinning the type in the cache.  This can be done by using a registration based
        // weak reference (we need to control the memory address), which we don't yet have, or by allocating the cache from
        // the inline cache arena to allow it to be zeroed, but retain a recycler-allocated portion to hold on to the size of
        // inlined slots.

        JavascriptFunction* constructor = UnsafeVarTo<JavascriptFunction>(function);
        if (functionInfo->IsClassConstructor() && !isBaseClassConstructorNewScObject)
        {
            // If we are calling new on a class constructor, the contract is that we pass new.target as the 'this' argument.
            // function is the constructor on which we called new - which is new.target.
            // If we are trying to construct the object for a base class constructor as part of a super call, we should not
            // store new.target in the 'this' argument.
            return function;
        }
        ConstructorCache* constructorCache = constructor->GetConstructorCache();
        AssertMsg(constructorCache->GetScriptContext() == nullptr || constructorCache->GetScriptContext() == constructor->GetScriptContext(),
            "Why did we populate a constructor cache with a mismatched script context?");

        Assert(constructorCache != nullptr);
        DynamicType* type = constructorCache->GetGuardValueAsType();
        if (type != nullptr && constructorCache->GetScriptContext() == requestContext)
        {
#if DBG
            bool cachedProtoCanBeCached;
            Assert(type->GetPrototype() == JavascriptOperators::GetPrototypeObjectForConstructorCache(constructor, requestContext, cachedProtoCanBeCached));
            Assert(cachedProtoCanBeCached);
            Assert(type->GetIsShared());
#endif

#if DBG_DUMP
            TraceUseConstructorCache(constructorCache, constructor, true);
#endif
            Var object = DynamicObject::New(requestContext->GetRecycler(), type);
            JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(object));
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
            {
                object = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(object));
            }
#endif
            return object;
        }

        if (constructorCache->SkipDefaultNewObject())
        {
            Assert(!constructorCache->NeedsUpdateAfterCtor());

#if DBG_DUMP
            TraceUseConstructorCache(constructorCache, constructor, true);
#endif
            if (isBaseClassConstructorNewScObject)
            {
                return JavascriptOperators::CreateFromConstructor(function, requestContext);
            }

            return nullptr;
        }

#if DBG_DUMP
        TraceUseConstructorCache(constructorCache, constructor, false);
#endif

        ScriptContext* constructorScriptContext = function->GetScriptContext();
        Assert(!constructorScriptContext->GetThreadContext()->IsDisableImplicitException());
        // we shouldn't try to call the constructor if it's closed already.
        constructorScriptContext->VerifyAlive(TRUE, requestContext);

        FunctionInfo::Attributes attributes = functionInfo->GetAttributes();
        if (attributes & FunctionInfo::ErrorOnNew)
        {
            JavascriptError::ThrowTypeError(requestContext, JSERR_ErrorOnNew);
        }

        // Slow path
        FunctionProxy * ctorProxy = constructor->GetFunctionProxy();
        FunctionBody * functionBody = ctorProxy != nullptr ? ctorProxy->EnsureDeserialized()->Parse() : nullptr;

        if (attributes & FunctionInfo::SkipDefaultNewObject)
        {
            // The constructor doesn't use the default new object.
#pragma prefast(suppress:6236, "DevDiv bug 830883. False positive when PHASE_OFF is #defined as '(false)'.")
            if (!PHASE_OFF1(ConstructorCachePhase) && (functionBody == nullptr || !PHASE_OFF(ConstructorCachePhase, functionBody)))
            {
                constructorCache = constructor->EnsureValidConstructorCache();
                constructorCache->PopulateForSkipDefaultNewObject(constructorScriptContext);

#if DBG_DUMP
                if ((functionBody != nullptr && PHASE_TRACE(Js::ConstructorCachePhase, functionBody)) || (functionBody == nullptr && PHASE_TRACE1(Js::ConstructorCachePhase)))
                {
                    const char16* ctorName = functionBody != nullptr ? functionBody->GetDisplayName() : _u("<unknown>");
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                    Output::Print(_u("CtorCache: populated cache (0x%p) for ctor %s (%s): "), constructorCache, ctorName,
                        functionBody ? functionBody->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
                    constructorCache->Dump();
                    Output::Print(_u("\n"));
                    Output::Flush();
                }
#endif

            }

            Assert(!constructorCache->NeedsUpdateAfterCtor());
            return nullptr;
        }

        // CONSIDER: Create some form of PatchGetProtoObjForCtorCache, which actually caches the prototype object in the constructor cache.
        // Make sure that it does NOT populate the guard field.  On the slow path (the only path for cross-context calls) we can do a faster lookup
        // after we fail the guard check.  When invalidating the cache for proto change, make sure we zap the prototype field of the cache in
        // addition to the guard value.
        bool prototypeCanBeCached;
        RecyclableObject* prototype = JavascriptOperators::GetPrototypeObjectForConstructorCache(
          function, constructorScriptContext, prototypeCanBeCached);
        prototype = VarTo<RecyclableObject>(CrossSite::MarshalVar(requestContext,
          prototype, constructorScriptContext));

        DynamicObject* newObject = requestContext->GetLibrary()->CreateObject(prototype, 8);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(newObject));
#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.IsEnabled(Js::autoProxyFlag))
        {
            newObject = VarTo<DynamicObject>(JavascriptProxy::AutoProxyWrapper(newObject));
        }
#endif

        Assert(newObject->GetTypeHandler()->GetPropertyCount() == 0);

        if (prototypeCanBeCached && functionBody != nullptr && requestContext == constructorScriptContext &&
            !Js::VarIs<Js::JavascriptProxy>(newObject) &&
            !PHASE_OFF1(ConstructorCachePhase) && !PHASE_OFF(ConstructorCachePhase, functionBody))
        {
            DynamicType* newObjectType = newObject->GetDynamicType();
            // Initial type (without any properties) should always be shared up-front.  This allows us to populate the cache right away.
            Assert(newObjectType->GetIsShared());

            // Populate the cache here and set the updateAfterCtor flag.  This way, if the ctor is called recursively the
            // recursive calls will hit the cache and use the initial type.  On the unwind path, we will update the cache
            // after the innermost ctor and clear the flag.  After subsequent ctors we won't attempt an update anymore.
            // As long as the updateAfterCtor flag is set it is safe to update the cache, because it would not have been
            // hard-coded in the JIT-ed code.
            constructorCache = constructor->EnsureValidConstructorCache();
            constructorCache->Populate(newObjectType, constructorScriptContext, functionBody->GetHasNoExplicitReturnValue(), true);
            Assert(constructorCache->IsConsistent());

#if DBG_DUMP
            if ((functionBody != nullptr && PHASE_TRACE(Js::ConstructorCachePhase, functionBody)) || (functionBody == nullptr && PHASE_TRACE1(Js::ConstructorCachePhase)))
            {
                const char16* ctorName = functionBody != nullptr ? functionBody->GetDisplayName() : _u("<unknown>");
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                Output::Print(_u("CtorCache: populated cache (0x%p) for ctor %s (%s): "), constructorCache, ctorName,
                    functionBody ? functionBody->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
                constructorCache->Dump();
                Output::Print(_u("\n"));
                Output::Flush();
            }
#endif
        }
        else
        {
#if DBG_DUMP
            if ((functionBody != nullptr && PHASE_TRACE(Js::ConstructorCachePhase, functionBody)) || (functionBody == nullptr && PHASE_TRACE1(Js::ConstructorCachePhase)))
            {
                const char16* ctorName = functionBody != nullptr ? functionBody->GetDisplayName() : _u("<unknown>");
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                Output::Print(_u("CtorCache: did not populate cache (0x%p) for ctor %s (%s), because %s: prototype = 0x%p, functionBody = 0x%p, ctor context = 0x%p, request context = 0x%p"),
                    constructorCache, ctorName, functionBody ? functionBody->GetDebugNumberSet(debugStringBuffer) : _u("(null)"),
                    !prototypeCanBeCached ? _u("prototype cannot be cached") :
                    functionBody == nullptr ? _u("function has no body") :
                    requestContext != constructorScriptContext ? _u("of cross-context call") : _u("constructor cache phase is off"),
                    prototype, functionBody, constructorScriptContext, requestContext);
                Output::Print(_u("\n"));
                Output::Flush();
            }
#endif
        }

        return newObject;
    }

    void JavascriptOperators::UpdateNewScObjectCache(Var function, Var instance, ScriptContext* requestContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(UpdateNewScObjectCache, reentrancylock, requestContext->GetThreadContext());
        JavascriptFunction* constructor = VarTo<JavascriptFunction>(function);
        if(constructor->GetScriptContext() != requestContext)
        {
            // The cache is populated only when the constructor function's context is the same as the calling context. However,
            // the cached type is not finalized yet and may not be until multiple calls to the constructor have been made (see
            // flag ConstructorCallsRequiredToFinalizeCachedType). A subsequent call to the constructor may be made from a
            // different context, so ignore those cross-context calls and wait for the constructor to be called from its own
            // context again to finalize the cached type.
            return;
        }

        // Review : What happens if the cache got invalidated between NewScObject and here?
        // Should we allocate new?  Should we mark it as polymorphic?
        ConstructorCache* constructorCache = constructor->GetConstructorCache();
        Assert(constructorCache->IsConsistent());
        Assert(!ConstructorCache::IsDefault(constructorCache));
        AssertMsg(constructorCache->GetScriptContext() == constructor->GetScriptContext(), "Why did we populate a constructor cache with a mismatched script context?");
        AssertMsg(constructorCache->IsPopulated(), "Why are we updating a constructor cache that hasn't been populated?");

        // The presence of the updateAfterCtor flag guarantees that this cache hasn't been used in JIT-ed fast path.  Even, if the
        // cache is invalidated, this flag is not changed.
        AssertMsg(constructorCache->NeedsUpdateAfterCtor(), "Why are we updating a constructor cache that doesn't need to be updated?");

        const bool finalizeCachedType =
            constructorCache->CallCount() >= CONFIG_FLAG(ConstructorCallsRequiredToFinalizeCachedType);
        if(!finalizeCachedType)
        {
            constructorCache->IncCallCount();
        }
        else
        {
            constructorCache->ClearUpdateAfterCtor();
        }

        FunctionBody* constructorBody = constructor->GetFunctionBody();
        AssertMsg(constructorBody != nullptr, "Constructor function doesn't have a function body.");
        Assert(VarIs<RecyclableObject>(instance));

        // The cache might have been invalidated between NewScObjectCommon and UpdateNewScObjectCache.  This could occur, for example, if
        // the constructor updates its own prototype property.  If that happens we don't want to re-populate it here.  A new cache will
        // be created when the constructor is called again.
        if (constructorCache->IsInvalidated())
        {
#if DBG_DUMP
            TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because cache is invalidated"));
#endif
            return;
        }

        Assert(constructorCache->GetGuardValueAsType() != nullptr);

        if (DynamicType::Is(VarTo<RecyclableObject>(instance)->GetTypeId()))
        {
            DynamicObject *object = UnsafeVarTo<DynamicObject>(instance);
            DynamicType* type = object->GetDynamicType();
            DynamicTypeHandler* typeHandler = type->GetTypeHandler();

            if (constructorBody->GetHasOnlyThisStmts())
            {
                if (!typeHandler->IsSharable())
                {
                    // Dynamic type created is not sharable.
                    // So in future don't try to check for "this assignment optimization".
                    constructorBody->SetHasOnlyThisStmts(false);
#if DBG_DUMP
                    TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because final type is not shareable"));
#endif
                }
                else if (typeHandler->GetPropertyCount() >= Js::PropertyIndexRanges<PropertyIndex>::MaxValue)
                {
                    // Dynamic type created has too many properties.
                    // So in future don't try to check for "this assignment optimization".
                    constructorBody->SetHasOnlyThisStmts(false);
#if DBG_DUMP
                    TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because final type has too many properties"));
#endif
                }
                else
                {
#if DBG
                    bool cachedProtoCanBeCached = false;
                    Assert(type->GetPrototype() == JavascriptOperators::GetPrototypeObjectForConstructorCache(constructor, requestContext, cachedProtoCanBeCached));
                    Assert(cachedProtoCanBeCached);
                    Assert(type->GetScriptContext() == constructorCache->GetScriptContext());
                    Assert(type->GetPrototype() == constructorCache->GetType()->GetPrototype());
#endif

                    typeHandler->SetMayBecomeShared();
                    // CONSIDER: Remove only this for delayed type sharing.
                    type->ShareType();

#if ENABLE_PROFILE_INFO
                    DynamicProfileInfo* profileInfo = constructorBody->HasDynamicProfileInfo() ? constructorBody->GetAnyDynamicProfileInfo() : nullptr;
                    if ((profileInfo != nullptr && profileInfo->GetImplicitCallFlags() <= ImplicitCall_None) ||
                        CheckIfPrototypeChainHasOnlyWritableDataProperties(type->GetPrototype()))
                    {

                        for (PropertyIndex pi = 0; pi < typeHandler->GetPropertyCount(); pi++)
                        {
                            requestContext->RegisterConstructorCache(typeHandler->GetPropertyId(requestContext, pi), constructorCache);
                        }

                        Assert(constructorBody->GetUtf8SourceInfo()->GetIsLibraryCode() || !constructor->GetScriptContext()->IsScriptContextInDebugMode());

                        if (constructorCache->TryUpdateAfterConstructor(type, constructor->GetScriptContext()))
                        {
#if DBG_DUMP
                            TraceUpdateConstructorCache(constructorCache, constructorBody, true, _u(""));
#endif
                        }
                        else
                        {
#if DBG_DUMP
                            TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because number of slots > MaxCachedSlotCount"));
#endif
                        }
                    }
#if DBG_DUMP
                    else
                    {
                        if (profileInfo &&
                            ((profileInfo->GetImplicitCallFlags() & ~(Js::ImplicitCall_External | Js::ImplicitCall_Accessor)) == 0) &&
                            profileInfo != nullptr && CheckIfPrototypeChainHasOnlyWritableDataProperties(type->GetPrototype()) &&
                            Js::Configuration::Global.flags.Trace.IsEnabled(Js::HostOptPhase))
                        {
                            const char16* ctorName = constructorBody->GetDisplayName();
                            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                            Output::Print(_u("CtorCache: %s cache (0x%p) for ctor %s (#%u) did not update because external call"),
                                constructorCache, constructorBody, ctorName, constructorBody ? constructorBody->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
                            Output::Print(_u("\n"));
                            Output::Flush();
                        }
                    }
#endif
#endif
                }
            }
            else
            {
#if DBG_DUMP
                TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because ctor has not only this statements"));
#endif
            }
        }
        else
        {
            // Even though this constructor apparently returned something other than the default object we created,
            // it still makes sense to cache the parameters of the default object, since we must create it every time, anyway.
#if DBG_DUMP
            TraceUpdateConstructorCache(constructorCache, constructorBody, false, _u("because ctor return a non-object value"));
#endif
            return;
        }

        // Whatever the constructor returned, if we're caching a type we want to be sure we shrink its inline slot capacity.
        if (finalizeCachedType && constructorCache->IsEnabled())
        {
            DynamicType* cachedType = constructorCache->NeedsTypeUpdate() ? constructorCache->GetPendingType() : constructorCache->GetType();
            DynamicTypeHandler* cachedTypeHandler = cachedType->GetTypeHandler();

            // Consider: We could delay inline slot capacity shrinking until the second time this constructor is invoked.  In some cases
            // this might permit more properties to remain inlined if the objects grow after constructor.  This would require flagging
            // the cache as special (already possible) and forcing the shrinking during work item creation if we happen to JIT this
            // constructor while the cache is in this special state.
            if (cachedTypeHandler->GetInlineSlotCapacity())
            {
#if DBG_DUMP
                int inlineSlotCapacityBeforeShrink = cachedTypeHandler->GetInlineSlotCapacity();
#endif

                // Note that after the cache has been updated and might have been used in the JIT-ed code, it is no longer legal to
                // shrink the inline slot capacity of the type.  That's because we allocate memory for a fixed number of inlined properties
                // and if that number changed on the type, this update wouldn't get reflected in JIT-ed code and we would allocate objects
                // of a wrong size.  This could conceivably happen if the original object got collected, and with it some of the successor
                // types also.  If then another constructor has the same prototype and needs to populate its own cache, it would attempt to
                // shrink inlined slots again.  If all surviving type handlers have smaller inline slot capacity, we would shrink it further.
                // To address this problem the type handler has a bit indicating its inline slots have been shrunk already.  If that bit is
                // set ShrinkSlotAndInlineSlotCapacity does nothing.
                cachedTypeHandler->ShrinkSlotAndInlineSlotCapacity();
                constructorCache->UpdateInlineSlotCount();

#if DBG_DUMP
                Assert(inlineSlotCapacityBeforeShrink >= cachedTypeHandler->GetInlineSlotCapacity());
                if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::InlineSlotsPhase))
                {
                    if (inlineSlotCapacityBeforeShrink != cachedTypeHandler->GetInlineSlotCapacity())
                    {
                        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                        Output::Print(_u("Inline slot capacity shrunk: Function:%04s Before:%d After:%d\n"),
                            constructorBody->GetDebugNumberSet(debugStringBuffer), inlineSlotCapacityBeforeShrink, cachedTypeHandler->GetInlineSlotCapacity());
                    }
                }
#endif
            }
        }
        JIT_HELPER_END(UpdateNewScObjectCache);
    }

    void JavascriptOperators::TraceUseConstructorCache(const ConstructorCache* ctorCache, const JavascriptFunction* ctor, bool isHit)
    {
#if DBG_DUMP
        // We are under debug, so we can incur the extra check here.
        FunctionProxy* ctorBody = ctor->GetFunctionProxy();
        if (ctorBody != nullptr && !ctorBody->GetScriptContext()->IsClosed())
        {
            ctorBody = ctorBody->EnsureDeserialized();
        }
        if ((ctorBody != nullptr && PHASE_TRACE(Js::ConstructorCachePhase, ctorBody)) || (ctorBody == nullptr && PHASE_TRACE1(Js::ConstructorCachePhase)))
        {
            const char16* ctorName = ctorBody != nullptr ? ctorBody->GetDisplayName() : _u("<unknown>");
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

            Output::Print(_u("CtorCache: %s cache (0x%p) for ctor %s (%s): "), isHit ? _u("hit") : _u("missed"), ctorCache, ctorName,
                ctorBody ? ctorBody->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
            ctorCache->Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
    }

    void JavascriptOperators::TraceUpdateConstructorCache(const ConstructorCache* ctorCache, const FunctionBody* ctorBody, bool updated, const char16* reason)
    {
#if DBG_DUMP
        if (PHASE_TRACE(Js::ConstructorCachePhase, ctorBody))
        {
            const char16* ctorName = ctorBody->GetDisplayName();
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

            Output::Print(_u("CtorCache: %s cache (0x%p) for ctor %s (%s)%s %s: "),
                updated ? _u("updated") : _u("did not update"), ctorBody, ctorName,
                ctorBody ? const_cast<Js::FunctionBody *>(ctorBody)->GetDebugNumberSet(debugStringBuffer) : _u("(null)"),
                updated ? _u("") : _u(", because") , reason);
            ctorCache->Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif
    }

    Var JavascriptOperators::NewScObject(const Var callee, const Arguments args, ScriptContext *const scriptContext, const Js::AuxArray<uint32> *spreadIndices)
    {
        Assert(callee);
        Assert(args.Info.Count != 0);
        Assert(scriptContext);

        // Always save and restore implicit call flags when calling out
        // REVIEW: Can we avoid it if we don't collect dynamic profile info?
        ThreadContext *const threadContext = scriptContext->GetThreadContext();
        const ImplicitCallFlags savedImplicitCallFlags = threadContext->GetImplicitCallFlags();

        const Var newVarInstance = JavascriptFunction::CallAsConstructor(callee, /* overridingNewTarget = */nullptr, args, scriptContext, spreadIndices);

        threadContext->SetImplicitCallFlags(savedImplicitCallFlags);
        return newVarInstance;
    }

    Js::GlobalObject * JavascriptOperators::OP_LdRoot(ScriptContext* scriptContext)
    {
        return scriptContext->GetGlobalObject();
    }

    Js::ModuleRoot * JavascriptOperators::GetModuleRoot(int moduleID, ScriptContext* scriptContext)
    {
        Assert(moduleID != kmodGlobal);
        JavascriptLibrary* library = scriptContext->GetLibrary();
        HostObjectBase *hostObject = library->GetGlobalObject()->GetHostObject();
        if (hostObject)
        {
            Js::ModuleRoot * moduleRoot = hostObject->GetModuleRoot(moduleID);
            Assert(!CrossSite::NeedMarshalVar(moduleRoot, scriptContext));
            return moduleRoot;
        }
        HostScriptContext *hostScriptContext = scriptContext->GetHostScriptContext();
        if (hostScriptContext)
        {
            Js::ModuleRoot * moduleRoot = hostScriptContext->GetModuleRoot(moduleID);
            Assert(!CrossSite::NeedMarshalVar(moduleRoot, scriptContext));
            return moduleRoot;
        }
        Assert(FALSE);
        return nullptr;
    }

    Var JavascriptOperators::OP_LoadModuleRoot(int moduleID, ScriptContext* scriptContext)
    {
        Js::ModuleRoot * moduleRoot = GetModuleRoot(moduleID, scriptContext);
        if (moduleRoot)
        {
            return moduleRoot;
        }
        Assert(false);
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptOperators::OP_LdNull(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetNull();
    }

    Var JavascriptOperators::OP_LdUndef(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptOperators::OP_LdNaN(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptOperators::OP_LdChakraLib(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetChakraLib();
    }

    Var JavascriptOperators::OP_LdInfinity(ScriptContext* scriptContext)
    {
        return scriptContext->GetLibrary()->GetPositiveInfinite();
    }

    void JavascriptOperators::BuildHandlerScope(Var argThis, RecyclableObject * hostObject, FrameDisplay * pDisplay, ScriptContext * scriptContext)
    {
        // Event handlers need implicit lookups of @@unscopables on parent scopes.
        // We can intercept the property accesses by wrapping the object with the unscopables handler.
        // WebIDL: https://heycam.github.io/webidl/#ref-for-Unscopable

        Assert(argThis != nullptr);
        pDisplay->SetItem(0, TaggedNumber::Is(argThis) ? scriptContext->GetLibrary()->CreateNumberObject(argThis) : ToUnscopablesWrapperObject(argThis, scriptContext));
        uint16 i = 1;

        Var aChild = argThis;
        uint16 length = pDisplay->GetLength();

        // Now add any parent scopes
        // We need to support the namespace parent lookup in both fastDOM on and off scenario.
        while (aChild != NULL)
        {
            Var aParent = hostObject->GetNamespaceParent(aChild);

            if (aParent == nullptr)
            {
                break;
            }
            aParent = CrossSite::MarshalVar(scriptContext, aParent);
            if (i == length)
            {
                length = UInt16Math::Add(length, 8);
                FrameDisplay * tmp = RecyclerNewPlus(scriptContext->GetRecycler(), length * sizeof(void*), FrameDisplay, length);
                js_memcpy_s((char*)tmp + tmp->GetOffsetOfScopes(), tmp->GetLength() * sizeof(void *), (char*)pDisplay + pDisplay->GetOffsetOfScopes(), pDisplay->GetLength() * sizeof(void*));
                pDisplay = tmp;
            }

            Var aParentWrapped = ToUnscopablesWrapperObject(aParent, scriptContext);
            pDisplay->SetItem(i, aParentWrapped);

            aChild = aParent;
            i++;
        }

        Assert(i <= pDisplay->GetLength());
        pDisplay->SetLength(i);
    }

    FrameDisplay * JavascriptOperators::OP_LdHandlerScope(Var argThis, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(ScrObj_LdHandlerScope);
        // The idea here is to build a stack of nested scopes in the form of a JS array.
        //
        // The scope stack for an event handler looks like this:
        //
        // implicit "this"
        // implicit namespace parent scopes

        // Put the implicit "this"
        if (argThis != NULL)
        {
            RecyclableObject* hostObject = scriptContext->GetGlobalObject()->GetHostObject();
            if (hostObject == nullptr)
            {
                hostObject = scriptContext->GetGlobalObject()->GetDirectHostObject();
            }
            if (hostObject != nullptr)
            {
                uint16 length = 7;
                FrameDisplay *pDisplay =
                    RecyclerNewPlus(scriptContext->GetRecycler(), length * sizeof(void*), FrameDisplay, length);
                BuildHandlerScope(argThis, hostObject, pDisplay, scriptContext);
                return pDisplay;
            }
        }

        return const_cast<FrameDisplay *>(&Js::NullFrameDisplay);
        JIT_HELPER_END(ScrObj_LdHandlerScope);
    }

    FrameDisplay* JavascriptOperators::OP_LdFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(ScrObj_LdFrameDisplay, reentrancylock, scriptContext->GetThreadContext());
        // Build a display of nested frame objects.
        // argHead is the current scope; argEnv is either the lone trailing scope or an array of scopes
        // which we append to the new display.

        // Note that there are cases in which a function with no local frame must construct a display to pass
        // to the function(s) nested within it. In such a case, argHead will be a null object, and it's not
        // strictly necessary to include it. But such cases are rare and not perf critical, so it's not
        // worth the extra complexity to notify the nested functions that they can "skip" this slot in the
        // frame display when they're loading scopes nested outside it.

        FrameDisplay *pDisplay = nullptr;
        FrameDisplay *envDisplay = (FrameDisplay*)argEnv;
        uint16 length = UInt16Math::Add(envDisplay->GetLength(), 1);

        pDisplay = RecyclerNewPlus(scriptContext->GetRecycler(), length * sizeof(void*), FrameDisplay, length);
        for (uint16 j = 0; j < length - 1; j++)
        {
            pDisplay->SetItem(j + 1, envDisplay->GetItem(j));
        }

        pDisplay->SetItem(0, argHead);

        return pDisplay;
        JIT_HELPER_END(ScrObj_LdFrameDisplay);
    }

    FrameDisplay* JavascriptOperators::OP_LdFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdFrameDisplayNoParent);
        return OP_LdFrameDisplay(argHead, (void*)&NullFrameDisplay, scriptContext);
        JIT_HELPER_END(ScrObj_LdFrameDisplayNoParent);
    }

    FrameDisplay* JavascriptOperators::OP_LdStrictFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdStrictFrameDisplay);
        FrameDisplay * pDisplay = OP_LdFrameDisplay(argHead, argEnv, scriptContext);
        pDisplay->SetStrictMode(true);

        return pDisplay;
        JIT_HELPER_END(ScrObj_LdStrictFrameDisplay);
    }

    FrameDisplay* JavascriptOperators::OP_LdStrictFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdStrictFrameDisplayNoParent);
        return OP_LdStrictFrameDisplay(argHead, (void*)&StrictNullFrameDisplay, scriptContext);
        JIT_HELPER_END(ScrObj_LdStrictFrameDisplayNoParent);
    }

    FrameDisplay* JavascriptOperators::OP_LdInnerFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdInnerFrameDisplay);
        CheckInnerFrameDisplayArgument(argHead);
        return OP_LdFrameDisplay(argHead, argEnv, scriptContext);
        JIT_HELPER_END(ScrObj_LdInnerFrameDisplay);
    }

    FrameDisplay* JavascriptOperators::OP_LdInnerFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdInnerFrameDisplayNoParent);
        CheckInnerFrameDisplayArgument(argHead);
        return OP_LdFrameDisplayNoParent(argHead, scriptContext);
        JIT_HELPER_END(ScrObj_LdInnerFrameDisplayNoParent);
    }

    FrameDisplay* JavascriptOperators::OP_LdStrictInnerFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdStrictInnerFrameDisplay);
        CheckInnerFrameDisplayArgument(argHead);
        return OP_LdStrictFrameDisplay(argHead, argEnv, scriptContext);
        JIT_HELPER_END(ScrObj_LdStrictInnerFrameDisplay);
    }

    FrameDisplay* JavascriptOperators::OP_LdStrictInnerFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrObj_LdStrictInnerFrameDisplayNoParent);
        CheckInnerFrameDisplayArgument(argHead);
        return OP_LdStrictFrameDisplayNoParent(argHead, scriptContext);
        JIT_HELPER_END(ScrObj_LdStrictInnerFrameDisplayNoParent);
    }

    void JavascriptOperators::CheckInnerFrameDisplayArgument(void *argHead)
    {
        if (ThreadContext::IsOnStack(argHead))
        {
            AssertMsg(false, "Illegal byte code: stack object as with scope");
            Js::Throw::FatalInternalError();
        }
        if (!VarIs<RecyclableObject>(argHead))
        {
            AssertMsg(false, "Illegal byte code: non-object as with scope");
            Js::Throw::FatalInternalError();
        }
    }

    Js::PropertyId JavascriptOperators::GetPropertyId(Var propertyName, ScriptContext* scriptContext)
    {
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptSymbol * symbol = JavascriptOperators::TryFromVar<Js::JavascriptSymbol>(propertyName);
        if (symbol)
        {
            propertyRecord = symbol->GetValue();
        }
        else
        {
            JavascriptSymbolObject * symbolObject = JavascriptOperators::TryFromVar<JavascriptSymbolObject>(propertyName);
            if (symbolObject)
            {
                propertyRecord = symbolObject->GetValue();
            }
            else
            {
                JavascriptString * indexStr = JavascriptConversion::ToString(propertyName, scriptContext);
                scriptContext->GetOrAddPropertyRecord(indexStr, &propertyRecord);
            }
        }

        return propertyRecord->GetPropertyId();
    }

    void JavascriptOperators::OP_InitSetter(Var object, PropertyId propertyId, Var setter)
    {
        AssertMsg(!TaggedNumber::Is(object), "SetMember on a non-object?");
        RecyclableObject* recylableObject = VarTo<RecyclableObject>(object);
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_InitSetter, reentrancylock, recylableObject->GetScriptContext()->GetThreadContext());
        recylableObject->SetAccessors(propertyId, nullptr, setter);
        JIT_HELPER_END(OP_InitSetter);
    }

    void JavascriptOperators::OP_InitClassMemberSet(Var object, PropertyId propertyId, Var setter)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_InitClassMemberSet);
        JIT_HELPER_SAME_ATTRIBUTES(Op_InitClassMemberSet, OP_InitSetter);
        JavascriptOperators::OP_InitSetter(object, propertyId, setter);

        VarTo<RecyclableObject>(object)->SetAttributes(propertyId, PropertyClassMemberDefaults);
        JIT_HELPER_END(Op_InitClassMemberSet);
    }

    Js::PropertyId JavascriptOperators::OP_InitElemSetter(Var object, Var elementName, Var setter, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_InitElemSetter);
        AssertMsg(!TaggedNumber::Is(object), "SetMember on a non-object?");

        PropertyId propertyId = JavascriptOperators::GetPropertyId(elementName, scriptContext);

        VarTo<RecyclableObject>(object)->SetAccessors(propertyId, nullptr, setter);

        return propertyId;
        JIT_HELPER_END(OP_InitElemSetter);
    }

    Field(Var)* JavascriptOperators::OP_GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex, ScriptContextInfo* scriptContext)
    {
        return scriptContext->GetModuleExportSlotArrayAddress(moduleIndex, slotIndex);
    }

    Field(Var)* JavascriptOperators::OP_GetModuleExportSlotAddress(uint moduleIndex, uint slotIndex, ScriptContext* scriptContext)
    {
        Field(Var)* moduleRecordSlots = OP_GetModuleExportSlotArrayAddress(moduleIndex, slotIndex, scriptContext);
        Assert(moduleRecordSlots != nullptr);

        return &moduleRecordSlots[slotIndex];
    }

    Var JavascriptOperators::OP_LdModuleSlot(uint moduleIndex, uint slotIndex, ScriptContext* scriptContext)
    {
        Field(Var)* addr = OP_GetModuleExportSlotAddress(moduleIndex, slotIndex, scriptContext);

        Assert(addr != nullptr);

        return *addr;
    }

    void JavascriptOperators::OP_StModuleSlot(uint moduleIndex, uint slotIndex, Var value, ScriptContext* scriptContext)
    {
        Assert(value != nullptr);

        Field(Var)* addr = OP_GetModuleExportSlotAddress(moduleIndex, slotIndex, scriptContext);

        Assert(addr != nullptr);

        *addr = value;
    }

    Var JavascriptOperators::OP_LdImportMeta(uint moduleIndex, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(LdImportMeta);
        return scriptContext->GetLibrary()->GetModuleRecord(moduleIndex)->GetImportMetaObject();
        JIT_HELPER_END(LdImportMeta);
    }

    void JavascriptOperators::OP_InitClassMemberSetComputedName(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_InitClassMemberSetComputedName);
        Js::PropertyId propertyId = JavascriptOperators::OP_InitElemSetter(object, elementName, value, scriptContext);
        RecyclableObject* instance = VarTo<RecyclableObject>(object);

        // instance will be a function if it is the class constructor (otherwise it would be an object)
        if (VarIs<JavascriptFunction>(instance) && Js::PropertyIds::prototype == propertyId)
        {
            // It is a TypeError to have a static member with a computed name that evaluates to 'prototype'
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassStaticMethodCannotBePrototype);
        }

        instance->SetAttributes(propertyId, PropertyClassMemberDefaults);
        JIT_HELPER_END(Op_InitClassMemberSetComputedName);
    }

    BOOL JavascriptOperators::IsClassConstructor(Var instance)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_IsClassConstructor);
        JavascriptFunction * function = JavascriptOperators::TryFromVar<JavascriptFunction>(instance);
        return function && function->GetFunctionInfo()->IsClassConstructor();
        JIT_HELPER_END(Op_IsClassConstructor);
    }

    BOOL JavascriptOperators::IsClassMethod(Var instance)
    {
        JavascriptFunction * function = JavascriptOperators::TryFromVar<JavascriptFunction>(instance);
        return function && function->GetFunctionInfo()->IsClassMethod();
    }

    BOOL JavascriptOperators::IsBaseConstructorKind(Var instance)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_IsBaseConstructorKind);
        JavascriptFunction * function = JavascriptOperators::TryFromVar<JavascriptFunction>(instance);
        return function && (function->GetFunctionInfo()->GetBaseConstructorKind());
        JIT_HELPER_END(Op_IsBaseConstructorKind);
    }

    void JavascriptOperators::OP_InitGetter(Var object, PropertyId propertyId, Var getter)
    {
        AssertMsg(!TaggedNumber::Is(object), "GetMember on a non-object?");
        RecyclableObject* recylableObject = VarTo<RecyclableObject>(object);
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_InitGetter, reentrancylock, recylableObject->GetScriptContext()->GetThreadContext());
        recylableObject->SetAccessors(propertyId, getter, nullptr);
        JIT_HELPER_END(OP_InitGetter);
    }

    void JavascriptOperators::OP_InitClassMemberGet(Var object, PropertyId propertyId, Var getter)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_InitClassMemberGet);
        JIT_HELPER_SAME_ATTRIBUTES(Op_InitClassMemberGet, OP_InitGetter);
        JavascriptOperators::OP_InitGetter(object, propertyId, getter);

        VarTo<RecyclableObject>(object)->SetAttributes(propertyId, PropertyClassMemberDefaults);
        JIT_HELPER_END(Op_InitClassMemberGet);
    }

    Js::PropertyId JavascriptOperators::OP_InitElemGetter(Var object, Var elementName, Var getter, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_InitElemGetter);
        AssertMsg(!TaggedNumber::Is(object), "GetMember on a non-object?");

        PropertyId propertyId = JavascriptOperators::GetPropertyId(elementName, scriptContext);

        VarTo<RecyclableObject>(object)->SetAccessors(propertyId, getter, nullptr);

        return propertyId;
        JIT_HELPER_END(OP_InitElemGetter);
    }

    void JavascriptOperators::OP_InitClassMemberGetComputedName(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_InitClassMemberGetComputedName);
        Js::PropertyId propertyId = JavascriptOperators::OP_InitElemGetter(object, elementName, value, scriptContext);
        RecyclableObject* instance = VarTo<RecyclableObject>(object);

        // instance will be a function if it is the class constructor (otherwise it would be an object)
        if (VarIs<JavascriptFunction>(instance) && Js::PropertyIds::prototype == propertyId)
        {
            // It is a TypeError to have a static member with a computed name that evaluates to 'prototype'
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassStaticMethodCannotBePrototype);
        }

        instance->SetAttributes(propertyId, PropertyClassMemberDefaults);
        JIT_HELPER_END(Op_InitClassMemberGetComputedName);
    }

    void JavascriptOperators::OP_InitComputedProperty(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(OP_InitComputedProperty);
        PropertyId propertyId = JavascriptOperators::GetPropertyId(elementName, scriptContext);

        VarTo<RecyclableObject>(object)->InitProperty(propertyId, value, flags);
        JIT_HELPER_END(OP_InitComputedProperty);
    }

    void JavascriptOperators::OP_InitClassMemberComputedName(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_InitClassMemberComputedName);
        PropertyId propertyId = JavascriptOperators::GetPropertyId(elementName, scriptContext);
        RecyclableObject* instance = VarTo<RecyclableObject>(object);

        // instance will be a function if it is the class constructor (otherwise it would be an object)
        if (VarIs<JavascriptFunction>(instance) && Js::PropertyIds::prototype == propertyId)
        {
            // It is a TypeError to have a static member with a computed name that evaluates to 'prototype'
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassStaticMethodCannotBePrototype);
        }

        instance->SetPropertyWithAttributes(propertyId, value, PropertyClassMemberDefaults, NULL, flags);
        JIT_HELPER_END(Op_InitClassMemberComputedName);
    }

    //
    // Used by object literal {..., __proto__: ..., }.
    //
    void JavascriptOperators::OP_InitProto(Var instance, PropertyId propertyId, Var value)
    {
        AssertMsg(VarIs<RecyclableObject>(instance), "__proto__ member on a non-object?");
        Assert(propertyId == PropertyIds::__proto__);

        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        ScriptContext* scriptContext = object->GetScriptContext();
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_InitProto, reentrancylock, scriptContext->GetThreadContext());

        // B.3.1    __proto___ Property Names in Object Initializers
        //6.If propKey is the string value "__proto__" and if isComputedPropertyName(propKey) is false, then
        //    a.If Type(v) is either Object or Null, then
        //        i.Return the result of calling the [[SetInheritance]] internal method of object with argument propValue.
        //    b.Return NormalCompletion(empty).
        if (JavascriptOperators::IsObjectOrNull(value))
        {
            JavascriptObject::ChangePrototype(object, VarTo<RecyclableObject>(value), /*validate*/false, scriptContext);
        }
        JIT_HELPER_END(OP_InitProto);
    }

    Var JavascriptOperators::ConvertToUnmappedArguments(HeapArgumentsObject *argumentsObject,
        uint32 paramCount,
        Var *paramAddr,
        DynamicObject* frameObject,
        Js::PropertyIdArray *propIds,
        uint32 formalsCount,
        ScriptContext* scriptContext)
    {
        Var *paramIter = paramAddr;
        uint32 i = 0;

        for (paramIter = paramAddr + i; i < paramCount; i++, paramIter++)
        {
            JavascriptOperators::SetItem(argumentsObject, argumentsObject, i, *paramIter, scriptContext, PropertyOperation_None, /* skipPrototypeCheck = */ TRUE);
        }

        argumentsObject = argumentsObject->ConvertToUnmappedArgumentsObject();

        // Now as the unmapping is done we need to fill those frame object with Undecl
        for (i = 0; i < formalsCount; i++)
        {
            frameObject->SetSlot(SetSlotArguments(propIds != nullptr ? propIds->elements[i] : Js::Constants::NoProperty, i, scriptContext->GetLibrary()->GetUndeclBlockVar()));
        }

        return argumentsObject;
    }

    Var JavascriptOperators::LoadHeapArguments(JavascriptFunction *funcCallee, uint32 actualsCount, Var *paramAddr, Var frameObj, Var vArray, ScriptContext* scriptContext, bool nonSimpleParamList)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_LoadHeapArguments, reentrancylock, scriptContext->GetThreadContext());
        AssertMsg(actualsCount != (unsigned int)-1, "Loading the arguments object in the global function?");

        // Create and initialize the Arguments object.

        uint32 formalsCount = 0;
        Js::PropertyIdArray *propIds = nullptr;
        if (vArray != scriptContext->GetLibrary()->GetNull())
        {
            propIds = (Js::PropertyIdArray *)vArray;
            formalsCount = propIds->count;
            Assert(formalsCount != 0 && propIds != nullptr);
        }

        HeapArgumentsObject *argsObj = JavascriptOperators::CreateHeapArguments(funcCallee, actualsCount, formalsCount, frameObj, scriptContext);
        return FillScopeObject(funcCallee, actualsCount, formalsCount, frameObj, paramAddr, propIds, argsObj, scriptContext, nonSimpleParamList, false);
        JIT_HELPER_END(Op_LoadHeapArguments);
    }

    Var JavascriptOperators::LoadHeapArgsCached(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var *paramAddr, Var frameObj, ScriptContext* scriptContext, bool nonSimpleParamList)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_LoadHeapArgsCached, reentrancylock, scriptContext->GetThreadContext());
        // Disregard the "this" param.
        AssertMsg(actualsCount != (uint32)-1 && formalsCount != (uint32)-1,
                  "Loading the arguments object in the global function?");

        HeapArgumentsObject *argsObj = JavascriptOperators::CreateHeapArguments(funcCallee, actualsCount, formalsCount, frameObj, scriptContext);

        return FillScopeObject(funcCallee, actualsCount, formalsCount, frameObj, paramAddr, nullptr, argsObj, scriptContext, nonSimpleParamList, true);
        JIT_HELPER_END(Op_LoadHeapArgsCached);
    }

    Var JavascriptOperators::FillScopeObject(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var frameObj, Var * paramAddr,
        Js::PropertyIdArray *propIds, HeapArgumentsObject * argsObj, ScriptContext * scriptContext, bool nonSimpleParamList, bool useCachedScope)
    {
        Assert(formalsCount == 0 || frameObj != nullptr);

        // Transfer formal arguments (that were actually passed) from their ArgIn slots to the local frame object.
        uint32 i;

        Var *tmpAddr = paramAddr;

        if (formalsCount != 0)
        {
            DynamicObject* frameObject = nullptr;
            if (useCachedScope)
            {
                frameObject = VarTo<DynamicObject>(frameObj);
                __analysis_assume((uint32)frameObject->GetDynamicType()->GetTypeHandler()->GetSlotCapacity() >= formalsCount);
            }
            else
            {
                frameObject = (DynamicObject*)frameObj;
                // No fixed fields for formal parameters of the arguments object.  Also, mark all fields as initialized up-front, because
                // we will set them directly using SetSlot below, so the type handler will not have a chance to mark them as initialized later.
                // CONSIDER : When we delay type sharing until the second instance is created, pass an argument indicating we want the types
                // and handlers created here to be marked as shared up-front. This is to ensure we don't get any fixed fields and that the handler
                // is ready for storing values directly to slots.
                DynamicType* newType = nullptr;
                if (nonSimpleParamList)
                {
                    bool skipLetAttrForArguments = ( VarIs<JavascriptGeneratorFunction>(funcCallee) ?
                        UnsafeVarTo<JavascriptGeneratorFunction>(funcCallee)->GetGeneratorVirtualScriptFunction()->GetFunctionBody()->HasReferenceableBuiltInArguments()
                        : funcCallee->GetFunctionBody()->HasReferenceableBuiltInArguments());

                    if (skipLetAttrForArguments)
                    {
                        newType = PathTypeHandlerBase::CreateNewScopeObject<true>(scriptContext, frameObject->GetDynamicType(), propIds, PropertyLetDefaults);
                    }
                    else
                    {
                        newType = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, frameObject->GetDynamicType(), propIds, PropertyLetDefaults);
                    }
                }
                else
                {
                    newType = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, frameObject->GetDynamicType(), propIds);
                }
                int oldSlotCapacity = frameObject->GetDynamicType()->GetTypeHandler()->GetSlotCapacity();
                int newSlotCapacity = newType->GetTypeHandler()->GetSlotCapacity();
                __analysis_assume((uint32)newSlotCapacity >= formalsCount);

                frameObject->EnsureSlots(oldSlotCapacity, newSlotCapacity, scriptContext, newType->GetTypeHandler());
                frameObject->ReplaceType(newType);
            }

            if (argsObj && nonSimpleParamList)
            {
                return ConvertToUnmappedArguments(argsObj, actualsCount, paramAddr, frameObject, propIds, formalsCount, scriptContext);
            }

            for (i = 0; i < formalsCount && i < actualsCount; i++, tmpAddr++)
            {
                frameObject->SetSlot(SetSlotArguments(propIds != nullptr? propIds->elements[i] : Constants::NoProperty, i, *tmpAddr));
            }

            if (i < formalsCount)
            {
                // The formals that weren't passed still need to be put in the frame object so that
                // their names will be found. Initialize them to "undefined".
                for (; i < formalsCount; i++)
                {
                    frameObject->SetSlot(SetSlotArguments(propIds != nullptr? propIds->elements[i] : Constants::NoProperty, i, scriptContext->GetLibrary()->GetUndefined()));
                }
            }
        }

        if (argsObj != nullptr)
        {
            // Transfer the unnamed actual arguments, if any, to the Arguments object itself.
            for (i = formalsCount, tmpAddr = paramAddr + i; i < actualsCount; i++, tmpAddr++)
            {
                // ES5 10.6.11: use [[DefineOwnProperty]] semantics (instead of [[Put]]):
                // do not check whether property is non-writable/etc in the prototype.
                // ES3 semantics is same.
                JavascriptOperators::SetItem(argsObj, argsObj, i, *tmpAddr, scriptContext, PropertyOperation_None, /* skipPrototypeCheck = */ TRUE);
            }

            if (funcCallee->IsStrictMode())
            {
                // If the formals are let decls, then we just overwrote the frame object slots with
                // Undecl sentinels, and we can use the original arguments that were passed to the HeapArgumentsObject.
                return argsObj->ConvertToUnmappedArgumentsObject(!nonSimpleParamList);
            }
        }
        return argsObj;
    }

    HeapArgumentsObject *JavascriptOperators::CreateHeapArguments(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var frameObj, ScriptContext* scriptContext)
    {
        JavascriptLibrary *library = scriptContext->GetLibrary();
        HeapArgumentsObject *argsObj = library->CreateHeapArguments(frameObj, formalsCount, !!funcCallee->IsStrictMode());

#if DBG
        DynamicTypeHandler* typeHandler = argsObj->GetTypeHandler();
#endif

        //
        // Set the number of arguments of Arguments Object
        //
        argsObj->SetNumberOfArguments(actualsCount);

        JavascriptOperators::SetProperty(argsObj, argsObj, PropertyIds::length, JavascriptNumber::ToVar(actualsCount, scriptContext), scriptContext);
        JavascriptOperators::SetProperty(argsObj, argsObj, PropertyIds::_symbolIterator, library->EnsureArrayPrototypeValuesFunction(), scriptContext);
        if (funcCallee->IsStrictMode())
        {
            JavascriptFunction* restrictedPropertyAccessor = library->GetThrowTypeErrorRestrictedPropertyAccessorFunction();
            argsObj->SetAccessors(PropertyIds::callee, restrictedPropertyAccessor, restrictedPropertyAccessor, PropertyOperation_NonFixedValue);

        }
        else
        {
            JavascriptOperators::SetProperty(argsObj, argsObj, PropertyIds::callee,
                StackScriptFunction::EnsureBoxed(BOX_PARAM(funcCallee, nullptr, _u("callee"))), scriptContext);
        }

        AssertMsg(argsObj->GetTypeHandler() == typeHandler || scriptContext->IsScriptContextInDebugMode(), "type handler should not transition because we initialized it correctly");

        return argsObj;
    }

    Var JavascriptOperators::OP_NewScopeObject(ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_NewScopeObject);
        return scriptContext->GetLibrary()->CreateActivationObject();
        JIT_HELPER_END(OP_NewScopeObject);
    }

    Var JavascriptOperators::OP_NewScopeObjectWithFormals(ScriptContext* scriptContext, FunctionBody * calleeBody, bool nonSimpleParamList)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_NewScopeObjectWithFormals, reentrancylock, scriptContext->GetThreadContext());
        Js::ActivationObject * frameObject = (ActivationObject*)OP_NewScopeObject(scriptContext);
        // No fixed fields for formal parameters of the arguments object.  Also, mark all fields as initialized up-front, because
        // we will set them directly using SetSlot below, so the type handler will not have a chance to mark them as initialized later.
        // CONSIDER : When we delay type sharing until the second instance is created, pass an argument indicating we want the types
        // and handlers created here to be marked as shared up-front. This is to ensure we don't get any fixed fields and that the handler
        // is ready for storing values directly to slots.
        DynamicType* newType = nullptr;
        if (nonSimpleParamList)
        {
            if (calleeBody->HasReferenceableBuiltInArguments())
            {
                newType = PathTypeHandlerBase::CreateNewScopeObject<true>(scriptContext, frameObject->GetDynamicType(), calleeBody->GetFormalsPropIdArray(), PropertyLetDefaults);
            }
            else
            {
                newType = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, frameObject->GetDynamicType(), calleeBody->GetFormalsPropIdArray(), PropertyLetDefaults);
            }
        }
        else
        {
            newType = PathTypeHandlerBase::CreateNewScopeObject<false>(scriptContext, frameObject->GetDynamicType(), calleeBody->GetFormalsPropIdArray());
        }

        int oldSlotCapacity = frameObject->GetDynamicType()->GetTypeHandler()->GetSlotCapacity();
        int newSlotCapacity = newType->GetTypeHandler()->GetSlotCapacity();

        frameObject->EnsureSlots(oldSlotCapacity, newSlotCapacity, scriptContext, newType->GetTypeHandler());
        frameObject->ReplaceType(newType);

        return frameObject;
        JIT_HELPER_END(OP_NewScopeObjectWithFormals);
    }

    Field(Var)* JavascriptOperators::OP_NewScopeSlots(unsigned int size, ScriptContext *scriptContext, Var scope)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_NewScopeSlots, reentrancylock, scriptContext->GetThreadContext());
        Assert(size > ScopeSlots::FirstSlotIndex); // Should never see empty slot array
        Field(Var)* slotArray = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), size); // last initialized slot contains reference to array of propertyIds, correspondent to objects in previous slots
        uint count = size - ScopeSlots::FirstSlotIndex;
        ScopeSlots slots(slotArray);
        slots.SetCount(count);
        AssertMsg(!FunctionBody::Is(scope), "Scope should only be FunctionInfo or DebuggerScope, not FunctionBody");
        slots.SetScopeMetadata(scope);

        Var undef = scriptContext->GetLibrary()->GetUndefined();
        for (unsigned int i = 0; i < count; i++)
        {
            slots.Set(i, undef);
        }

        return slotArray;
        JIT_HELPER_END(OP_NewScopeSlots);
    }

    Field(Var)* JavascriptOperators::OP_NewScopeSlotsWithoutPropIds(unsigned int count, int scopeIndex, ScriptContext *scriptContext, FunctionBody *functionBody)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_NewScopeSlotsWithoutPropIds);
        DebuggerScope* scope = reinterpret_cast<DebuggerScope*>(Constants::FunctionBodyUnavailable);
        if (scopeIndex != DebuggerScope::InvalidScopeIndex)
        {
            AssertMsg(functionBody->GetScopeObjectChain(), "A scope chain should always be created when there are new scope slots for blocks.");
            scope = functionBody->GetScopeObjectChain()->pScopeChain->Item(scopeIndex);
        }
        return OP_NewScopeSlots(count, scriptContext, scope);
        JIT_HELPER_END(OP_NewScopeSlotsWithoutPropIds);
    }

    Field(Var)* JavascriptOperators::OP_CloneScopeSlots(Field(Var) *slotArray, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_CloneInnerScopeSlots, reentrancylock, scriptContext->GetThreadContext());
        ScopeSlots slots(slotArray);
        uint size = ScopeSlots::FirstSlotIndex + static_cast<uint>(slots.GetCount());

        Field(Var)* slotArrayClone = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), size);
        CopyArray(slotArrayClone, size, slotArray, size);

        return slotArrayClone;
        JIT_HELPER_END(OP_CloneInnerScopeSlots);
    }

    Var JavascriptOperators::OP_NewPseudoScope(ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_NewPseudoScope);
        return scriptContext->GetLibrary()->CreatePseudoActivationObject();
        JIT_HELPER_END(OP_NewPseudoScope);
    }

    Var JavascriptOperators::OP_NewBlockScope(ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_NewBlockScope);
        return scriptContext->GetLibrary()->CreateBlockActivationObject();
        JIT_HELPER_END(OP_NewBlockScope);
    }

    Var JavascriptOperators::OP_CloneBlockScope(BlockActivationObject *blockScope, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(OP_CloneBlockScope, reentrancylock, scriptContext->GetThreadContext());
        return blockScope->Clone(scriptContext);
        JIT_HELPER_END(OP_CloneBlockScope);
    }

    Var JavascriptOperators::OP_IsInst(Var instance, Var aClass, ScriptContext* scriptContext, IsInstInlineCache* inlineCache)
    {
        JIT_HELPER_REENTRANT_HEADER(ScrObj_OP_IsInst);
        if (!VarIs<RecyclableObject>(aClass))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedFunction, _u("instanceof"));
        }

        RecyclableObject* constructor = VarTo<RecyclableObject>(aClass);
        if (scriptContext->GetConfig()->IsES6HasInstanceEnabled())
        {
            if (VarIs<JavascriptFunction>(constructor))
            {
                JavascriptFunction* func = VarTo<JavascriptFunction>(constructor);
                if (func->IsBoundFunction())
                {
                    BoundFunction* boundFunc = (BoundFunction*)func;
                    constructor = boundFunc->GetTargetFunction();
                }
            }

            Var instOfHandler = JavascriptOperators::GetPropertyNoCache(constructor,
              PropertyIds::_symbolHasInstance, scriptContext);
            if (JavascriptOperators::IsUndefinedObject(instOfHandler)
                || instOfHandler == scriptContext->GetBuiltInLibraryFunction(JavascriptFunction::EntryInfo::SymbolHasInstance.GetOriginalEntryPoint()))
            {
                return JavascriptBoolean::ToVar(constructor->HasInstance(instance, scriptContext, inlineCache), scriptContext);
            }
            else
            {
                if (!JavascriptConversion::IsCallable(instOfHandler))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, _u("Symbol[Symbol.hasInstance]"));
                }

                ThreadContext * threadContext = scriptContext->GetThreadContext();
                RecyclableObject *instFunc = VarTo<RecyclableObject>(instOfHandler);
                Var result = threadContext->ExecuteImplicitCall(instFunc, ImplicitCall_Accessor, [=]()->Js::Var
                {
                    return CALL_FUNCTION(scriptContext->GetThreadContext(), instFunc, CallInfo(CallFlags_Value, 2), constructor, instance);
                });

                return  JavascriptBoolean::ToVar(JavascriptConversion::ToBoolean(result, scriptContext) ? TRUE : FALSE, scriptContext);
            }
        }
        else
        {
            return JavascriptBoolean::ToVar(constructor->HasInstance(instance, scriptContext, inlineCache), scriptContext);
        }
        JIT_HELPER_END(ScrObj_OP_IsInst);
    }

    Var JavascriptOperators::OP_NewClassProto(Var protoParent, ScriptContext * scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_NewClassProto, reentrancylock, scriptContext->GetThreadContext());
        return scriptContext->GetLibrary()->CreateClassPrototypeObject(VarTo<RecyclableObject>(protoParent));
        JIT_HELPER_END(Op_NewClassProto);
    }

    void JavascriptOperators::OP_LoadUndefinedToElement(Var instance, PropertyId propertyId)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_LdElemUndef, reentrancylock, VarTo<RecyclableObject>(instance)->GetScriptContext()->GetThreadContext());
        AssertMsg(!TaggedNumber::Is(instance), "Invalid scope/root object");
        JavascriptOperators::EnsureProperty(instance, propertyId);
        JIT_HELPER_END(Op_LdElemUndef);
    }

    void JavascriptOperators::OP_LoadUndefinedToElementScoped(FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_LdElemUndefScoped, reentrancylock, scriptContext->GetThreadContext());
        int i;
        int length = pScope->GetLength();
        Var argInstance;
        for (i = 0; i < length; i++)
        {
            argInstance = pScope->GetItem(i);
            if (JavascriptOperators::EnsureProperty(argInstance, propertyId))
            {
                return;
            }
        }

        if (!JavascriptOperators::HasOwnPropertyNoHostObject(defaultInstance, propertyId))
        {
            // CONSIDER : Consider adding pre-initialization support to activation objects.
            JavascriptOperators::OP_InitPropertyScoped(pScope, propertyId, scriptContext->GetLibrary()->GetUndefined(), defaultInstance, scriptContext);
        }
        JIT_HELPER_END(Op_LdElemUndefScoped);
    }

    void JavascriptOperators::OP_LoadUndefinedToElementDynamic(Var instance, PropertyId propertyId, ScriptContext *scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_LdElemUndefDynamic, reentrancylock, scriptContext->GetThreadContext());
        if (!JavascriptOperators::HasOwnPropertyNoHostObject(instance, propertyId))
        {
            VarTo<RecyclableObject>(instance)->InitPropertyScoped(propertyId, scriptContext->GetLibrary()->GetUndefined());
        }
        JIT_HELPER_END(Op_LdElemUndefDynamic);
    }

    BOOL JavascriptOperators::EnsureProperty(Var instance, PropertyId propertyId)
    {
        RecyclableObject *obj = VarTo<RecyclableObject>(instance);
        return (obj && obj->EnsureProperty(propertyId));
    }

    void JavascriptOperators::OP_EnsureNoRootProperty(Var instance, PropertyId propertyId)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_EnsureNoRootProperty);
        Assert(VarIs<RootObjectBase>(instance));
        RootObjectBase *obj = VarTo<RootObjectBase>(instance);
        obj->EnsureNoProperty(propertyId);
        JIT_HELPER_END(Op_EnsureNoRootProperty);
    }

    void JavascriptOperators::OP_EnsureNoRootRedeclProperty(Var instance, PropertyId propertyId)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_EnsureNoRootRedeclProperty);
        Assert(VarIs<RootObjectBase>(instance));
        RecyclableObject *obj = VarTo<RecyclableObject>(instance);
        obj->EnsureNoRedeclProperty(propertyId);
        JIT_HELPER_END(Op_EnsureNoRootRedeclProperty);
    }

    void JavascriptOperators::OP_EnsureCanDeclGloFunc(Var instance, PropertyId propertyId)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_EnsureCanDeclGloFunc);
        Assert(VarIs<RootObjectBase>(instance));
        RootObjectBase *obj = VarTo<RootObjectBase>(instance);
        obj->EnsureCanDeclGloFunc(propertyId);
        JIT_HELPER_END(Op_EnsureCanDeclGloFunc);
    }

    void JavascriptOperators::OP_ScopedEnsureNoRedeclProperty(FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_EnsureNoRedeclPropertyScoped);
        int i;
        int length = pDisplay->GetLength();
        RecyclableObject *object;

        for (i = 0; i < length; i++)
        {
            object = VarTo<RecyclableObject>(pDisplay->GetItem(i));
            if (object->EnsureNoRedeclProperty(propertyId))
            {
                return;
            }
        }

        object = VarTo<RecyclableObject>(defaultInstance);
        object->EnsureNoRedeclProperty(propertyId);
        JIT_HELPER_END(Op_EnsureNoRedeclPropertyScoped);
    }

    Var JavascriptOperators::IsIn(Var argProperty, Var instance, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_IsIn);
        // Note that the fact that we haven't seen a given name before doesn't mean that the instance doesn't

        if (!IsObject(instance))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Operand_Invalid_NeedObject, _u("in"));
        }

        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        BOOL result;
        PropertyRecord const * propertyRecord = nullptr;
        uint32 index;
        IndexType indexType;

        // Fast path for JavascriptSymbols and PropertyStrings
        RecyclableObject* cacheOwner;
        PropertyRecordUsageCache* propertyRecordUsageCache;
        if (GetPropertyRecordUsageCache(argProperty, scriptContext, &propertyRecordUsageCache, &cacheOwner))
        {
            Var value;
            propertyRecord = propertyRecordUsageCache->GetPropertyRecord();
            if (!propertyRecord->IsNumeric())
            {
                PropertyValueInfo info;
                if (propertyRecordUsageCache->TryGetPropertyFromCache<false /* OwnPropertyOnly */, true /* OutputExistence */, false /* ReturnOperationInfo */>(instance, object, &value, scriptContext, &info, cacheOwner, nullptr))
                {
                    Assert(VarIs<JavascriptBoolean>(value));
                    return value;
                }
                result = JavascriptOperators::GetPropertyWPCache<true /* OutputExistence */>(instance, object, propertyRecordUsageCache->GetPropertyRecord()->GetPropertyId(), &value, scriptContext, &info);
                Assert(value == JavascriptBoolean::ToVar(result, scriptContext));
                return value;
            }

            // We don't cache numeric property lookups, so fall through to the IndexType_Number case
            index = propertyRecord->GetNumericValue();
            indexType = IndexType_Number;
        }
        else
        {
            indexType = GetIndexType(argProperty, scriptContext, &index, &propertyRecord, true);
        }

        if (indexType == IndexType_Number)
        {
            result = JavascriptOperators::HasItem(object, index);
        }
        else
        {
            result = JavascriptOperators::HasProperty(object, propertyRecord->GetPropertyId());

#ifdef TELEMETRY_JSO
            {
                Assert(indexType != Js::IndexType_JavascriptString);
                if (indexType == Js::IndexType_PropertyId)
                {
                    scriptContext->GetTelemetry().GetOpcodeTelemetry().IsIn(instance, propertyId, result != 0);
                }
            }
#endif
        }
        return JavascriptBoolean::ToVar(result, scriptContext);
        JIT_HELPER_END(Op_IsIn);
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchGetValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetValue);
        return PatchGetValueWithThisPtr<IsFromFullJit, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, instance);
        JIT_HELPER_END(Op_PatchGetValue);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetValue, Op_PatchGetValuePolymorphic)

    template <bool IsFromFullJit, class TInlineCache>
    __forceinline Var JavascriptOperators::PatchGetValueWithThisPtr(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetValueWithThisPtr);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        Assert(Js::JavascriptStackWalker::ValidateTopJitFrame(scriptContext));

        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, true, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                instance, false, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetValue"), propertyId, scriptContext, object);
        }
#endif

        return JavascriptOperators::GetProperty(thisInstance, object, propertyId, scriptContext, &info);
        JIT_HELPER_END(Op_PatchGetValueWithThisPtr);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetValueWithThisPtr, Op_PatchGetValuePolymorphicWithThisPtr)

    template Var JavascriptOperators::PatchGetValue<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValue<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValue<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValue<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);

    template Var JavascriptOperators::PatchGetValueWithThisPtr<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);
    template Var JavascriptOperators::PatchGetValueWithThisPtr<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);
    template Var JavascriptOperators::PatchGetValueWithThisPtr<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);
    template Var JavascriptOperators::PatchGetValueWithThisPtr<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);

    template <bool IsFromFullJit, class TInlineCache>
    Var JavascriptOperators::PatchGetValueForTypeOf(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetValueForTypeOf);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        Assert(Js::JavascriptStackWalker::ValidateTopJitFrame(scriptContext));

        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, true, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
            instance, false, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetValueForTypeOf"), propertyId, scriptContext, object);
        }
#endif
        Var prop = nullptr;

        BEGIN_TYPEOF_ERROR_HANDLER(scriptContext);
        prop = JavascriptOperators::GetProperty(instance, object, propertyId, scriptContext, &info);
        END_TYPEOF_ERROR_HANDLER(scriptContext, prop);

        return prop;
        JIT_HELPER_END(Op_PatchGetValueForTypeOf);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetValueForTypeOf, Op_PatchGetValuePolymorphicForTypeOf)
    template Var JavascriptOperators::PatchGetValueForTypeOf<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValueForTypeOf<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValueForTypeOf<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetValueForTypeOf<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);

    Var JavascriptOperators::PatchGetValueUsingSpecifiedInlineCache(InlineCache * inlineCache, Var instance, RecyclableObject * object, PropertyId propertyId, ScriptContext* scriptContext)
    {
        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, inlineCache);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, true, false, true, !InlineCache::IsPolymorphic, InlineCache::IsPolymorphic, false, false>(
                instance, false, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetValue"), propertyId, scriptContext, object);
        }
#endif

        return JavascriptOperators::GetProperty(instance, object, propertyId, scriptContext, &info);
    }

    Var JavascriptOperators::PatchGetValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        return PatchGetValueWithThisPtrNoFastPath(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, instance);
    }

    Var JavascriptOperators::PatchGetValueWithThisPtrNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance)
    {
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        return JavascriptOperators::GetProperty(thisInstance, object, propertyId, scriptContext, &info);
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchGetRootValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetRootValue);
        AssertMsg(VarIs<RootObjectBase>(object), "Root must be a global object!");

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, false, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                object, true, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetRootValue"), propertyId, scriptContext, object);
        }
#endif

        return JavascriptOperators::OP_GetRootProperty(object, propertyId, &info, scriptContext);
        JIT_HELPER_END(Op_PatchGetRootValue);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetRootValue, Op_PatchGetRootValuePolymorphic)
    template Var JavascriptOperators::PatchGetRootValue<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValue<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValue<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValue<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);

    template <bool IsFromFullJit, class TInlineCache>
    Var JavascriptOperators::PatchGetRootValueForTypeOf(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetRootValueForTypeOf);
        AssertMsg(VarIs<RootObjectBase>(object), "Root must be a global object!");

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value = nullptr;
        if (CacheOperators::TryGetProperty<true, true, true, false, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
            object, true, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetRootValueForTypeOf"), propertyId, scriptContext, object);
        }
#endif
        value = nullptr;
        BEGIN_TYPEOF_ERROR_HANDLER(scriptContext);
        AssertOrFailFast(VarIsCorrectType(static_cast<RecyclableObject*>(object)));
        if (JavascriptOperators::GetRootProperty(object, propertyId, &value, scriptContext, &info))
        {
            if (scriptContext->IsUndeclBlockVar(value))
            {
                JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
            }
            return value;
        }
        END_TYPEOF_ERROR_HANDLER(scriptContext, value);

        value = scriptContext->GetLibrary()->GetUndefined();
        return value;
        JIT_HELPER_END(Op_PatchGetRootValueForTypeOf);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetRootValueForTypeOf, Op_PatchGetRootValuePolymorphicForTypeOf)
    template Var JavascriptOperators::PatchGetRootValueForTypeOf<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValueForTypeOf<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValueForTypeOf<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootValueForTypeOf<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject * object, PropertyId propertyId);


    Var JavascriptOperators::PatchGetRootValueNoFastPath_Var(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        return
            PatchGetRootValueNoFastPath(
                functionBody,
                inlineCache,
                inlineCacheIndex,
                VarTo<DynamicObject>(instance),
                propertyId);
    }

    Var JavascriptOperators::PatchGetRootValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId)
    {
        AssertMsg(VarIs<RootObjectBase>(object), "Root must be a global object!");

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        return JavascriptOperators::OP_GetRootProperty(object, propertyId, &info, scriptContext);
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchGetPropertyScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetPropertyScoped);
        // Get the property, using a scope stack rather than an individual instance.
        // Walk the stack until we find an instance that has the property.

        ScriptContext *const scriptContext = functionBody->GetScriptContext();
        uint16 length = pDisplay->GetLength();

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        for (uint16 i = 0; i < length; i++)
        {
            RecyclableObject* object = UnsafeVarTo<RecyclableObject>(pDisplay->GetItem(i));

            Var value;
            if (CacheOperators::TryGetProperty<true, true, true, false, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                    object, false, object, propertyId, &value, scriptContext, nullptr, &info))
            {
                return value;
            }

#if DBG_DUMP
            if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
            {
                CacheOperators::TraceCache(inlineCache, _u("PatchGetPropertyScoped"), propertyId, scriptContext, object);
            }
#endif
            if (JavascriptOperators::GetProperty(object, propertyId, &value, scriptContext, &info))
            {
                if (scriptContext->IsUndeclBlockVar(value) && propertyId != PropertyIds::_this)
                {
                    JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
                }
                return value;
            }
        }

        // There is no root decl for 'this', we should instead load the global 'this' value.
        if (propertyId == PropertyIds::_this)
        {
            Var varNull = OP_LdNull(scriptContext);
            return JavascriptOperators::OP_GetThis(varNull, functionBody->GetModuleID(), scriptContext);
        }
        else if (propertyId == PropertyIds::_super)
        {
            JavascriptError::ThrowReferenceError(scriptContext, JSERR_BadSuperReference);
        }

        // No one in the scope stack has the property, so get it from the default instance provided by the caller.
        Var value = JavascriptOperators::PatchGetRootValue<IsFromFullJit>(functionBody, inlineCache, inlineCacheIndex, VarTo<DynamicObject>(defaultInstance), propertyId);
        if (scriptContext->IsUndeclBlockVar(value))
        {
            JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration);
        }
        return value;
        JIT_HELPER_END(Op_PatchGetPropertyScoped);
    }
    template Var JavascriptOperators::PatchGetPropertyScoped<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyScoped<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyScoped<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyScoped<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);

    template <bool IsFromFullJit, class TInlineCache>
    Var JavascriptOperators::PatchGetPropertyForTypeOfScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetPropertyForTypeOfScoped);
        Var value = nullptr;
        ScriptContext *scriptContext = functionBody->GetScriptContext();

        BEGIN_TYPEOF_ERROR_HANDLER(scriptContext);
        value = JavascriptOperators::PatchGetPropertyScoped<IsFromFullJit, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, pDisplay, propertyId, defaultInstance);
        END_TYPEOF_ERROR_HANDLER(scriptContext, value)

        return value;
        JIT_HELPER_END(Op_PatchGetPropertyForTypeOfScoped);
    }
    template Var JavascriptOperators::PatchGetPropertyForTypeOfScoped<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyForTypeOfScoped<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyForTypeOfScoped<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);
    template Var JavascriptOperators::PatchGetPropertyForTypeOfScoped<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pDisplay, PropertyId propertyId, Var defaultInstance);


    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchGetMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetMethod);
        Assert(inlineCache != nullptr);

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = nullptr;
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            // Don't error if we disabled implicit calls
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
#ifdef TELEMETRY_JSO
                if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
                {
                    // `successful` will be true as PatchGetMethod throws an exception if not found.
                    scriptContext->GetTelemetry().GetOpcodeTelemetry().GetMethodProperty(object, propertyId, value, /*successful:*/false);
                }
#endif
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, false, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                instance, false, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetMethod"), propertyId, scriptContext, object);
        }
#endif

        value = Js::JavascriptOperators::PatchGetMethodFromObject(instance, object, propertyId, &info, scriptContext, false);
#ifdef TELEMETRY_JSO
        if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
        {
            // `successful` will be true as PatchGetMethod throws an exception if not found.
            scriptContext->GetTelemetry().GetOpcodeTelemetry().GetMethodProperty(object, propertyId, value, /*successful:*/true);
        }
#endif
        return value;
        JIT_HELPER_END(Op_PatchGetMethod);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetMethod, Op_PatchGetMethodPolymorphic)
    template Var JavascriptOperators::PatchGetMethod<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetMethod<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetMethod<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetMethod<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);

    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchGetRootMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchGetRootMethod);
        Assert(inlineCache != nullptr);

        AssertMsg(VarIs<RootObjectBase>(object), "Root must be a global object!");

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, false, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                object, true, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetRootMethod"), propertyId, scriptContext, object);
        }
#endif

        value = Js::JavascriptOperators::PatchGetMethodFromObject(object, object, propertyId, &info, scriptContext, true);
#ifdef TELEMETRY_JSO
        if (TELEMETRY_PROPERTY_OPCODE_FILTER(propertyId))
        {
            // `successful` will be true as PatchGetMethod throws an exception if not found.
            scriptContext->GetTelemetry().GetOpcodeTelemetry().GetMethodProperty(object, propertyId, value, /*successful:*/ true);
        }
#endif
        return value;
        JIT_HELPER_END(Op_PatchGetRootMethod);
    }
    JIT_HELPER_TEMPLATE(Op_PatchGetRootMethod, Op_PatchGetRootMethodPolymorphic)
    template Var JavascriptOperators::PatchGetRootMethod<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootMethod<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootMethod<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);
    template Var JavascriptOperators::PatchGetRootMethod<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);

    template <bool IsFromFullJit, class TInlineCache>
    inline Var JavascriptOperators::PatchScopedGetMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_ScopedGetMethod);
        Assert(inlineCache != nullptr);

        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            // Don't error if we disabled implicit calls
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        const bool isRoot = VarIs<RootObjectBase>(object);
        Var value;
        if (CacheOperators::TryGetProperty<true, true, true, false, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false, false>(
                instance, isRoot, object, propertyId, &value, scriptContext, nullptr, &info))
        {
            return value;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchGetMethod"), propertyId, scriptContext, object);
        }
#endif

        return Js::JavascriptOperators::PatchGetMethodFromObject(instance, object, propertyId, &info, scriptContext, isRoot);
        JIT_HELPER_END(Op_ScopedGetMethod);
    }
    JIT_HELPER_TEMPLATE(Op_ScopedGetMethod, Op_ScopedGetMethodPolymorphic)
    template Var JavascriptOperators::PatchScopedGetMethod<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchScopedGetMethod<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchScopedGetMethod<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
    template Var JavascriptOperators::PatchScopedGetMethod<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);

    Var JavascriptOperators::PatchGetMethodNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptOperators::GetPropertyObject(instance, scriptContext, &object))
        {
            // Don't error if we disabled implicit calls
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotGet_NullOrUndefined,
                    scriptContext->GetPropertyName(propertyId)->GetBuffer());
            }
            else
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
        }

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        return Js::JavascriptOperators::PatchGetMethodFromObject(instance, object, propertyId, &info, scriptContext, false);
    }

    Var JavascriptOperators::PatchGetRootMethodNoFastPath_Var(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId)
    {
        return
            PatchGetRootMethodNoFastPath(
                functionBody,
                inlineCache,
                inlineCacheIndex,
                VarTo<DynamicObject>(instance),
                propertyId);
    }

    Var JavascriptOperators::PatchGetRootMethodNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId)
    {
        AssertMsg(VarIs<RootObjectBase>(object), "Root must be a global object!");

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        return Js::JavascriptOperators::PatchGetMethodFromObject(object, object, propertyId, &info, functionBody->GetScriptContext(), true);
    }

    Var JavascriptOperators::PatchGetMethodFromObject(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, PropertyValueInfo * info, ScriptContext* scriptContext, bool isRootLd)
    {
        Assert(IsPropertyObject(propertyObject));

        Var value = nullptr;
        BOOL foundValue = FALSE;

        if (isRootLd)
        {
            RootObjectBase* rootObject = VarTo<RootObjectBase>(instance);
            foundValue = JavascriptOperators::GetRootPropertyReference(rootObject, propertyId, &value, scriptContext, info);
        }
        else
        {
            foundValue = JavascriptOperators::GetPropertyReference(instance, propertyObject, propertyId, &value, scriptContext, info);
        }

        if (!foundValue)
        {
            // Don't error if we disabled implicit calls
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                const char16* propertyName = scriptContext->GetPropertyName(propertyId)->GetBuffer();

                value = scriptContext->GetLibrary()->GetUndefined();
                JavascriptFunction * caller = NULL;
                if (JavascriptStackWalker::GetCaller(&caller, scriptContext))
                {
                    FunctionBody * callerBody = caller->GetFunctionBody();
                    if (callerBody && callerBody->GetUtf8SourceInfo()->GetIsXDomain())
                    {
                        propertyName = NULL;
                    }
                }

                // Prior to version 12 we had mistakenly immediately thrown an error for property reference method calls
                // (i.e. <expr>.foo() form) when the target object is the global object.  The spec says that a GetValue
                // on a reference should throw if the reference is unresolved, of which a property reference can never be,
                // however it can be unresolved in the case of an identifier expression, e.g. foo() with no qualification.
                // Such a case would come down to the global object if foo was undefined, hence the check for root object,
                // except that it should have been a check for isRootLd to be correct.
                //
                //   // (at global scope)
                //   foo(x());
                //
                // should throw an error before evaluating x() if foo is not defined, but
                //
                //   // (at global scope)
                //   this.foo(x());
                //
                // should evaluate x() before throwing an error if foo is not a property on the global object.
                // Maintain old behavior prior to version 12.
                bool isPropertyReference = !isRootLd;
                if (!isPropertyReference)
                {
                    JavascriptError::ThrowReferenceError(scriptContext, JSERR_UndefVariable, propertyName);
                }
                else
                {
                    // ES5 11.2.3 #2: We evaluate the call target but don't throw yet if target member is missing. We need to evaluate argList
                    // first (#3). Postpone throwing error to invoke time.
                    value = ThrowErrorObject::CreateThrowTypeErrorObject(scriptContext, VBSERR_OLENoPropOrMethod, propertyName);
                }
            }
        }
        return value;
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValue);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValue, Op_PatchPutValueWithThisPtr);
        PatchPutValueWithThisPtr<IsFromFullJit, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        JIT_HELPER_END(Op_PatchPutValue);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValue, Op_PatchPutValuePolymorphic)

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutValueWithThisPtr(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtr);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        if (TaggedNumber::Is(instance))
        {
            JavascriptOperators::SetPropertyOnTaggedNumber(instance, nullptr, propertyId, newValue, scriptContext, flags);
            return;
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        if (CacheOperators::TrySetProperty<true, true, true, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
                object, false, propertyId, newValue, scriptContext, flags, nullptr, &info))
        {
            return;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchPutValue"), propertyId, scriptContext, object);
        }
#endif

        ImplicitCallFlags prevImplicitCallFlags = ImplicitCall_None;
        ImplicitCallFlags currImplicitCallFlags = ImplicitCall_None;
        bool hasThisOnlyStatements = functionBody->GetHasOnlyThisStmts();
        if (hasThisOnlyStatements)
        {
            prevImplicitCallFlags = CacheAndClearImplicitBit(scriptContext);
        }
        if (!JavascriptOperators::OP_SetProperty(object, propertyId, newValue, scriptContext, &info, flags, thisInstance))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
        if (hasThisOnlyStatements)
        {
            currImplicitCallFlags = CheckAndUpdateFunctionBodyWithImplicitFlag(functionBody);
            RestoreImplicitFlag(scriptContext, prevImplicitCallFlags, currImplicitCallFlags);
        }
        JIT_HELPER_END(Op_PatchPutValueWithThisPtr);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtr, Op_PatchPutValueWithThisPtrPolymorphic)
    template void JavascriptOperators::PatchPutValue<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValue<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValue<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValue<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutRootValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutRootValue);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = VarTo<RecyclableObject>(instance);
        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        if (CacheOperators::TrySetProperty<true, true, true, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
                object, true, propertyId, newValue, scriptContext, flags, nullptr, &info))
        {
            return;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchPutRootValue"), propertyId, scriptContext, object);
        }
#endif

        ImplicitCallFlags prevImplicitCallFlags = ImplicitCall_None;
        ImplicitCallFlags currImplicitCallFlags = ImplicitCall_None;
        bool hasThisOnlyStatements = functionBody->GetHasOnlyThisStmts();
        if (hasThisOnlyStatements)
        {
            prevImplicitCallFlags = CacheAndClearImplicitBit(scriptContext);
        }
        if (!JavascriptOperators::SetRootProperty(object, propertyId, newValue, &info, scriptContext, flags))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
        if (hasThisOnlyStatements)
        {
            currImplicitCallFlags = CheckAndUpdateFunctionBodyWithImplicitFlag(functionBody);
            RestoreImplicitFlag(scriptContext, prevImplicitCallFlags, currImplicitCallFlags);
        }
        JIT_HELPER_END(Op_PatchPutRootValue);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutRootValue, Op_PatchPutRootValuePolymorphic)
    template void JavascriptOperators::PatchPutRootValue<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValue<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValue<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValue<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutValueNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueNoLocalFastPath);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueNoLocalFastPath, Op_PatchPutValueWithThisPtrNoLocalFastPath);
        PatchPutValueWithThisPtrNoLocalFastPath<IsFromFullJit, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        JIT_HELPER_END(Op_PatchPutValueNoLocalFastPath);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueNoLocalFastPath, Op_PatchPutValueNoLocalFastPathPolymorphic)
    template void JavascriptOperators::PatchPutValueNoLocalFastPath<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueNoLocalFastPath<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueNoLocalFastPath<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueNoLocalFastPath<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtrNoLocalFastPath);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        if (TaggedNumber::Is(instance))
        {
            JavascriptOperators::SetPropertyOnTaggedNumber(instance,
                nullptr,
                propertyId,
                newValue,
                scriptContext,
                flags);
            return;
        }
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(instance);
#endif
        RecyclableObject *object = UnsafeVarTo<RecyclableObject>(instance);

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        if (CacheOperators::TrySetProperty<!TInlineCache::IsPolymorphic, true, true, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
            object, false, propertyId, newValue, scriptContext, flags, nullptr, &info))
        {
            return;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchPutValueNoLocalFastPath"), propertyId, scriptContext, object);
        }
#endif

        ImplicitCallFlags prevImplicitCallFlags = ImplicitCall_None;
        ImplicitCallFlags currImplicitCallFlags = ImplicitCall_None;
        bool hasThisOnlyStatements = functionBody->GetHasOnlyThisStmts();
        if (hasThisOnlyStatements)
        {
            prevImplicitCallFlags = CacheAndClearImplicitBit(scriptContext);
        }
        if (!JavascriptOperators::OP_SetProperty(instance, propertyId, newValue, scriptContext, &info, flags, thisInstance))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
        if (hasThisOnlyStatements)
        {
            currImplicitCallFlags = CheckAndUpdateFunctionBodyWithImplicitFlag(functionBody);
            RestoreImplicitFlag(scriptContext, prevImplicitCallFlags, currImplicitCallFlags);
        }
        JIT_HELPER_END(Op_PatchPutValueWithThisPtrNoLocalFastPath);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtrNoLocalFastPath, Op_PatchPutValueWithThisPtrNoLocalFastPathPolymorphic)
    template void JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchPutRootValueNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutRootValueNoLocalFastPath);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject *object = VarTo<RecyclableObject>(instance);

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        if (CacheOperators::TrySetProperty<!TInlineCache::IsPolymorphic, true, true, true, false, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
                object, true, propertyId, newValue, scriptContext, flags, nullptr, &info))
        {
            return;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchPutRootValueNoLocalFastPath"), propertyId, scriptContext, object);
        }
#endif

        ImplicitCallFlags prevImplicitCallFlags = ImplicitCall_None;
        ImplicitCallFlags currImplicitCallFlags = ImplicitCall_None;
        bool hasThisOnlyStatements = functionBody->GetHasOnlyThisStmts();
        if (hasThisOnlyStatements)
        {
            prevImplicitCallFlags = CacheAndClearImplicitBit(scriptContext);
        }
        if (!JavascriptOperators::SetRootProperty(object, propertyId, newValue, &info, scriptContext, flags))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
        if (hasThisOnlyStatements)
        {
            currImplicitCallFlags = CheckAndUpdateFunctionBodyWithImplicitFlag(functionBody);
            RestoreImplicitFlag(scriptContext, prevImplicitCallFlags, currImplicitCallFlags);
        }
        JIT_HELPER_END(Op_PatchPutRootValueNoLocalFastPath);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutRootValueNoLocalFastPath, Op_PatchPutRootValueNoLocalFastPathPolymorphic)
    template void JavascriptOperators::PatchPutRootValueNoLocalFastPath<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValueNoLocalFastPath<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValueNoLocalFastPath<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template void JavascriptOperators::PatchPutRootValueNoLocalFastPath<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    void JavascriptOperators::PatchPutValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        PatchPutValueWithThisPtrNoFastPath(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
    }

    void JavascriptOperators::PatchPutValueWithThisPtrNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        if (TaggedNumber::Is(instance))
        {
            JavascriptOperators::SetPropertyOnTaggedNumber(instance, nullptr, propertyId, newValue, scriptContext, flags);
            return;
        }
        RecyclableObject* object = VarTo<RecyclableObject>(instance);

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        if (!JavascriptOperators::OP_SetProperty(object, propertyId, newValue, scriptContext, &info, flags, thisInstance))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
    }

    void JavascriptOperators::PatchPutRootValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        RecyclableObject* object = VarTo<RecyclableObject>(instance);

        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        if (!JavascriptOperators::SetRootProperty(object, propertyId, newValue, &info, scriptContext, flags))
        {
            // Add implicit call flags, to bail out if field copy prop may propagate the wrong value.
            scriptContext->GetThreadContext()->AddImplicitCallFlags(ImplicitCall_NoOpSet);
        }
    }

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueCantChangeType(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueCantChangeType);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueCantChangeType, Op_PatchPutValue);

        Type * oldType = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetType() : nullptr;
        PatchPutValueWithThisPtr<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        return (oldType != nullptr && oldType != UnsafeVarTo<DynamicObject>(instance)->GetType());

        JIT_HELPER_END(Op_PatchPutValueCantChangeType);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueCantChangeType, Op_PatchPutValuePolymorphicCantChangeType);
    template bool JavascriptOperators::PatchPutValueCantChangeType<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueCantChangeType<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueWithThisPtrCantChangeType(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtrCantChangeType);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueWithThisPtrCantChangeType, Op_PatchPutValueWithThisPtr);

        Type * oldType = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetType() : nullptr;
        PatchPutValueWithThisPtr<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, thisInstance, flags);
        return (oldType != nullptr && oldType != UnsafeVarTo<DynamicObject>(instance)->GetType());

        JIT_HELPER_END(Op_PatchPutValueWithThisPtrCantChangeType);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtrCantChangeType, Op_PatchPutValueWithThisPtrPolymorphicCantChangeType);
    template bool JavascriptOperators::PatchPutValueWithThisPtrCantChangeType<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueWithThisPtrCantChangeType<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueNoLocalFastPathCantChangeType(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueNoLocalFastPathCantChangeType);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueNoLocalFastPathCantChangeType, Op_PatchPutValueNoLocalFastPath);

        Type * oldType = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetType() : nullptr;
        PatchPutValueWithThisPtrNoLocalFastPath<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        return (oldType != nullptr && oldType != UnsafeVarTo<DynamicObject>(instance)->GetType());

        JIT_HELPER_END(Op_PatchPutValueNoLocalFastPathCantChangeType);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueNoLocalFastPathCantChangeType, Op_PatchPutValueNoLocalFastPathPolymorphicCantChangeType);
    template bool JavascriptOperators::PatchPutValueNoLocalFastPathCantChangeType<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueNoLocalFastPathCantChangeType<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCantChangeType(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtrNoLocalFastPathCantChangeType);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueWithThisPtrNoLocalFastPathCantChangeType, Op_PatchPutValueWithThisPtrNoLocalFastPath);

        Type * oldType = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetType() : nullptr;
        PatchPutValueWithThisPtrNoLocalFastPath<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, thisInstance, flags);
        return (oldType != nullptr && oldType != UnsafeVarTo<DynamicObject>(instance)->GetType());

        JIT_HELPER_END(Op_PatchPutValueWithThisPtrNoLocalFastPathCantChangeType);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtrNoLocalFastPathCantChangeType, Op_PatchPutValueWithThisPtrNoLocalFastPathPolymorphicCantChangeType);
    template bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCantChangeType<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCantChangeType<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchInitValueCantChangeType(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchInitValueCantChangeType);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchInitValueCantChangeType, Op_PatchInitValue);

        Type * oldType = VarIs<DynamicObject>(object) ? UnsafeVarTo<DynamicObject>(object)->GetType() : nullptr;
        PatchInitValue<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, object, propertyId, newValue);
        return (oldType != nullptr && oldType != UnsafeVarTo<DynamicObject>(object)->GetType());

        JIT_HELPER_END(Op_PatchInitValueCantChangeType);
    }
    JIT_HELPER_TEMPLATE(Op_PatchInitValueCantChangeType, Op_PatchInitValuePolymorphicCantChangeType);
    template bool JavascriptOperators::PatchInitValueCantChangeType<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
    template bool JavascriptOperators::PatchInitValueCantChangeType<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueCheckLayout(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueCheckLayout);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueCheckLayout, Op_PatchPutValue);

        DynamicTypeHandler * oldTypeHandler = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetTypeHandler() : nullptr;
        PatchPutValueWithThisPtr<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        return (oldTypeHandler != nullptr && LayoutChanged(UnsafeVarTo<DynamicObject>(instance), oldTypeHandler));

        JIT_HELPER_END(Op_PatchPutValueCheckLayout);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueCheckLayout, Op_PatchPutValuePolymorphicCheckLayout);
    template bool JavascriptOperators::PatchPutValueCheckLayout<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueCheckLayout<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueWithThisPtrCheckLayout(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtrCheckLayout);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueWithThisPtrCheckLayout, Op_PatchPutValueWithThisPtr);

        DynamicTypeHandler * oldTypeHandler = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetTypeHandler() : nullptr;
        PatchPutValueWithThisPtr<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, thisInstance, flags);
        return (oldTypeHandler != nullptr && LayoutChanged(UnsafeVarTo<DynamicObject>(instance), oldTypeHandler));

        JIT_HELPER_END(Op_PatchPutValueWithThisPtrCheckLayout);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtrCheckLayout, Op_PatchPutValueWithThisPtrPolymorphicCheckLayout);
    template bool JavascriptOperators::PatchPutValueWithThisPtrCheckLayout<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueWithThisPtrCheckLayout<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueNoLocalFastPathCheckLayout(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueNoLocalFastPathCheckLayout);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueNoLocalFastPathCheckLayout, Op_PatchPutValueNoLocalFastPath);

        DynamicTypeHandler * oldTypeHandler = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetTypeHandler() : nullptr;
        PatchPutValueWithThisPtrNoLocalFastPath<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, instance, flags);
        return (oldTypeHandler != nullptr && LayoutChanged(UnsafeVarTo<DynamicObject>(instance), oldTypeHandler));

        JIT_HELPER_END(Op_PatchPutValueNoLocalFastPathCheckLayout);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueNoLocalFastPathCheckLayout, Op_PatchPutValueNoLocalFastPathPolymorphicCheckLayout);
    template bool JavascriptOperators::PatchPutValueNoLocalFastPathCheckLayout<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueNoLocalFastPathCheckLayout<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCheckLayout(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchPutValueWithThisPtrNoLocalFastPathCheckLayout);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchPutValueWithThisPtrNoLocalFastPathCheckLayout, Op_PatchPutValueWithThisPtrNoLocalFastPath);

        DynamicTypeHandler * oldTypeHandler = VarIs<DynamicObject>(instance) ? UnsafeVarTo<DynamicObject>(instance)->GetTypeHandler() : nullptr;
        PatchPutValueWithThisPtrNoLocalFastPath<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, instance, propertyId, newValue, thisInstance, flags);
        return (oldTypeHandler != nullptr && LayoutChanged(UnsafeVarTo<DynamicObject>(instance), oldTypeHandler));

        JIT_HELPER_END(Op_PatchPutValueWithThisPtrNoLocalFastPathCheckLayout);
    }
    JIT_HELPER_TEMPLATE(Op_PatchPutValueWithThisPtrNoLocalFastPathCheckLayout, Op_PatchPutValueWithThisPtrNoLocalFastPathPolymorphicCheckLayout);
    template bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCheckLayout<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);
    template bool JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPathCheckLayout<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags);

    template <class TInlineCache>
    inline bool JavascriptOperators::PatchInitValueCheckLayout(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchInitValueCheckLayout);
        JIT_HELPER_SAME_ATTRIBUTES(Op_PatchInitValueCheckLayout, Op_PatchInitValue);

        DynamicTypeHandler * oldTypeHandler = VarIs<DynamicObject>(object) ? UnsafeVarTo<DynamicObject>(object)->GetTypeHandler() : nullptr;
        PatchInitValue<true, TInlineCache>(functionBody, inlineCache, inlineCacheIndex, object, propertyId, newValue);
        return (oldTypeHandler != nullptr && LayoutChanged(UnsafeVarTo<DynamicObject>(object), oldTypeHandler));

        JIT_HELPER_END(Op_PatchInitValueCheckLayout);
    }
    JIT_HELPER_TEMPLATE(Op_PatchInitValueCheckLayout, Op_PatchInitValuePolymorphicCheckLayout);
    template bool JavascriptOperators::PatchInitValueCheckLayout<InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
    template bool JavascriptOperators::PatchInitValueCheckLayout<PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);

    bool JavascriptOperators::LayoutChanged(DynamicObject *const instance, DynamicTypeHandler *const oldTypeHandler)
    {
        DynamicTypeHandler * newTypeHandler = instance->GetTypeHandler();
        return (oldTypeHandler != newTypeHandler &&
                (oldTypeHandler->GetInlineSlotCapacity() != newTypeHandler->GetInlineSlotCapacity() ||
                 oldTypeHandler->GetOffsetOfInlineSlots() != newTypeHandler->GetOffsetOfInlineSlots()));
    }

    template <bool IsFromFullJit, class TInlineCache>
    inline void JavascriptOperators::PatchInitValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_PatchInitValue);
        ScriptContext *const scriptContext = functionBody->GetScriptContext();

        const PropertyOperationFlags flags = newValue == NULL ? PropertyOperation_SpecialValue : PropertyOperation_None;
        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, !IsFromFullJit);
        if (CacheOperators::TrySetProperty<true, true, false, true, true, !TInlineCache::IsPolymorphic, TInlineCache::IsPolymorphic, false>(
                object, false, propertyId, newValue, scriptContext, flags, nullptr, &info))
        {
            return;
        }

#if DBG_DUMP
        if (PHASE_VERBOSE_TRACE1(Js::InlineCachePhase))
        {
            CacheOperators::TraceCache(inlineCache, _u("PatchInitValue"), propertyId, scriptContext, object);
        }
#endif

        Type *typeWithoutProperty = object->GetType();

        if (functionBody->IsEval())
        {
            if (object->InitPropertyInEval(propertyId, newValue, flags, &info))
            {
                CacheOperators::CachePropertyWrite(object, false, typeWithoutProperty, propertyId, &info, scriptContext);
                return;
            }
        }

        // Ideally the lowerer would emit a call to the right flavor of PatchInitValue, so that we can ensure that we only
        // ever initialize to NULL in the right cases.  But the backend uses the StFld opcode for initialization, and it
        // would be cumbersome to thread the different helper calls all the way down
        if (object->InitProperty(propertyId, newValue, flags, &info))
        {
            CacheOperators::CachePropertyWrite(object, false, typeWithoutProperty, propertyId, &info, scriptContext);
        }
        JIT_HELPER_END(Op_PatchInitValue);
    }
    JIT_HELPER_TEMPLATE(Op_PatchInitValue, Op_PatchInitValuePolymorphic)
    template void JavascriptOperators::PatchInitValue<false, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
    template void JavascriptOperators::PatchInitValue<true, InlineCache>(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
    template void JavascriptOperators::PatchInitValue<false, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
    template void JavascriptOperators::PatchInitValue<true, PolymorphicInlineCache>(FunctionBody *const functionBody, PolymorphicInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);

    void JavascriptOperators::PatchInitValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue)
    {
        PropertyValueInfo info;
        PropertyValueInfo::SetCacheInfo(&info, functionBody, inlineCache, inlineCacheIndex, true);
        Type *typeWithoutProperty = object->GetType();

        if (functionBody->IsEval())
        {
            if (object->InitPropertyInEval(propertyId, newValue, PropertyOperation_None, &info))
            {
                CacheOperators::CachePropertyWrite(object, false, typeWithoutProperty, propertyId, &info, functionBody->GetScriptContext());
                return;
            }
        }

        if (object->InitProperty(propertyId, newValue, PropertyOperation_None, &info))
        {
            CacheOperators::CachePropertyWrite(object, false, typeWithoutProperty, propertyId, &info, functionBody->GetScriptContext());
        }
    }

    void JavascriptOperators::GetPropertyIdForInt(uint64 value, ScriptContext* scriptContext, PropertyRecord const ** propertyRecord)
    {
        char16 buffer[20];
        ::_ui64tow_s(value, buffer, sizeof(buffer)/sizeof(char16), 10);
        scriptContext->GetOrAddPropertyRecord(buffer, JavascriptString::GetBufferLength(buffer), propertyRecord);
    }

    void JavascriptOperators::GetPropertyIdForInt(uint32 value, ScriptContext* scriptContext, PropertyRecord const ** propertyRecord)
    {
        GetPropertyIdForInt(static_cast<uint64>(value), scriptContext, propertyRecord);
    }

    Var JavascriptOperators::FromPropertyDescriptor(const PropertyDescriptor& descriptor, ScriptContext* scriptContext)
    {
        DynamicObject* object = scriptContext->GetLibrary()->CreateObject();

        // ES5 Section 8.10.4 specifies the order for adding these properties.
        if (descriptor.IsDataDescriptor())
        {
            if (descriptor.ValueSpecified())
            {
                JavascriptOperators::InitProperty(object, PropertyIds::value, descriptor.GetValue());
            }
            if (descriptor.WritableSpecified())
            {
                JavascriptOperators::InitProperty(object, PropertyIds::writable, JavascriptBoolean::ToVar(descriptor.IsWritable(), scriptContext));
            }
        }
        else if (descriptor.IsAccessorDescriptor())
        {
            JavascriptOperators::InitProperty(object, PropertyIds::get, JavascriptOperators::CanonicalizeAccessor(descriptor.GetGetter(), scriptContext));
            JavascriptOperators::InitProperty(object, PropertyIds::set, JavascriptOperators::CanonicalizeAccessor(descriptor.GetSetter(), scriptContext));
        }

        if (descriptor.EnumerableSpecified())
        {
            JavascriptOperators::InitProperty(object, PropertyIds::enumerable, JavascriptBoolean::ToVar(descriptor.IsEnumerable(), scriptContext));
        }
        if (descriptor.ConfigurableSpecified())
        {
            JavascriptOperators::InitProperty(object, PropertyIds::configurable, JavascriptBoolean::ToVar(descriptor.IsConfigurable(), scriptContext));
        }
        return object;
    }

    // ES5 8.12.9 [[DefineOwnProperty]].
    // Return value:
    // - TRUE = success.
    // - FALSE (can throw depending on throwOnError parameter) = unsuccessful.
    BOOL JavascriptOperators::DefineOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* scriptContext)
    {
        Assert(obj);
        Assert(scriptContext);

        if (VarIs<JavascriptProxy>(obj))
        {
            return JavascriptProxy::DefineOwnPropertyDescriptor(obj, propId, descriptor, throwOnError, scriptContext);
        }
#ifdef _CHAKRACOREBUILD
        else if (VarIs<CustomExternalWrapperObject>(obj))
        {
            // See if there is a trap for defineProperty.
            BOOL wrapperResult = CustomExternalWrapperObject::DefineOwnPropertyDescriptor(obj, propId, descriptor, throwOnError, scriptContext);
            if (wrapperResult)
            {
                return TRUE;
            }
        }
#endif

        PropertyDescriptor currentDescriptor;
        BOOL isCurrentDescriptorDefined = JavascriptOperators::GetOwnPropertyDescriptor(obj, propId, scriptContext, &currentDescriptor);

        bool isExtensible = !!obj->IsExtensible();
        return ValidateAndApplyPropertyDescriptor<true>(obj, propId, descriptor, isCurrentDescriptorDefined ? &currentDescriptor : nullptr, isExtensible, throwOnError, scriptContext);
    }

    BOOL JavascriptOperators::IsCompatiblePropertyDescriptor(const PropertyDescriptor& descriptor, PropertyDescriptor* currentDescriptor, bool isExtensible, bool throwOnError, ScriptContext* scriptContext)
    {
        return ValidateAndApplyPropertyDescriptor<false>(nullptr, Constants::NoProperty, descriptor, currentDescriptor, isExtensible, throwOnError, scriptContext);
    }

    template<bool needToSetProperty>
    BOOL JavascriptOperators::ValidateAndApplyPropertyDescriptor(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor,
        PropertyDescriptor* currentDescriptor, bool isExtensible, bool throwOnError, ScriptContext* scriptContext)
    {
        Var defaultDataValue = scriptContext->GetLibrary()->GetUndefined();
        Var defaultAccessorValue = scriptContext->GetLibrary()->GetDefaultAccessorFunction();

        if (currentDescriptor == nullptr)
        {
            if (!isExtensible) // ES5 8.12.9.3.
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotExtensible, propId);
            }
            else // ES5 8.12.9.4.
            {
                if (needToSetProperty)
                {
                    if (descriptor.IsGenericDescriptor() || descriptor.IsDataDescriptor())
                    {
                        // ES5 8.12.9.4a: Create an own data property named P of object O whose [[Value]], [[Writable]],
                        // [[Enumerable]] and [[Configurable]]  attribute values are described by Desc.
                        // If the value of an attribute field of Desc is absent, the attribute of the newly created property
                        // is set to its default value.
                        PropertyDescriptor filledDescriptor = FillMissingPropertyDescriptorFields<false>(descriptor, scriptContext);

                        BOOL tempResult = obj->SetPropertyWithAttributes(propId, filledDescriptor.GetValue(), filledDescriptor.GetAttributes(), nullptr);
                        if (!obj->IsExternal() && !tempResult)
                        {
                            Assert(
                                // Arrays return false when length property is non-writable and property is numeric
                                // and greater than or equal to length
                                DynamicObject::IsAnyArray(obj) ||
                                // Typed arrays return false when canonical numeric index is not integer or out of range
                                DynamicObject::IsAnyTypedArray(obj)
                            );
                            return FALSE;
                        }
                    }
                    else
                    {
                        // ES5 8.12.9.4b: Create an own accessor property named P of object O whose [[Get]], [[Set]], [[Enumerable]]
                        // and [[Configurable]] attribute values are described by Desc. If the value of an attribute field of Desc is absent,
                        // the attribute of the newly created property is set to its default value.
                        Assert(descriptor.IsAccessorDescriptor());
                        PropertyDescriptor filledDescriptor = FillMissingPropertyDescriptorFields<true>(descriptor, scriptContext);

                        BOOL isSetAccessorsSuccess = obj->SetAccessors(propId, filledDescriptor.GetGetter(), filledDescriptor.GetSetter());

                        // It is valid for some objects to not-support getters and setters, specifically, for projection of an ABI method
                        // (CustomExternalObject => MapWithStringKey) which SetAccessors returns VBSErr_ActionNotSupported.
                        // But for non-external objects SetAccessors should succeed.
                        Assert(isSetAccessorsSuccess || obj->IsExternal());

                        // If SetAccessors failed, the property wasn't created, so no need to change the attributes.
                        if (isSetAccessorsSuccess)
                        {
                            JavascriptOperators::SetAttributes(obj, propId, filledDescriptor, true);   // use 'force' as default attributes in type system are different from ES5.
                        }
                    }
                }
                return TRUE;
            }
        }

        // ES5 8.12.9.5: Return true, if every field in Desc is absent.
        if (!descriptor.ConfigurableSpecified() && !descriptor.EnumerableSpecified() && !descriptor.WritableSpecified() &&
            !descriptor.ValueSpecified() && !descriptor.GetterSpecified() && !descriptor.SetterSpecified())
        {
            return TRUE;
        }

        // ES5 8.12.9.6: Return true, if every field in Desc also occurs in current and the value of every field in Desc is the same value
        // as the corresponding field in current when compared using the SameValue algorithm (9.12).
        PropertyDescriptor filledDescriptor = descriptor.IsAccessorDescriptor() ? FillMissingPropertyDescriptorFields<true>(descriptor, scriptContext)
            : FillMissingPropertyDescriptorFields<false>(descriptor, scriptContext);
        if (JavascriptOperators::AreSamePropertyDescriptors(&filledDescriptor, currentDescriptor, scriptContext))
        {
            return TRUE;
        }

        if (!currentDescriptor->IsConfigurable()) // ES5 8.12.9.7.
        {
            if (descriptor.ConfigurableSpecified() && descriptor.IsConfigurable())
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
            }
            if (descriptor.EnumerableSpecified() && descriptor.IsEnumerable() != currentDescriptor->IsEnumerable())
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
            }
        }

        // Whether to merge attributes from tempDescriptor into descriptor to keep original values
        // of some attributes from the object/use tempDescriptor for SetAttributes, or just use descriptor.
        // This is optimization to avoid 2 calls to SetAttributes.
        bool mergeDescriptors = false;

        // Whether to call SetAttributes with 'force' flag which forces setting all attributes
        // rather than only specified or which have true values.
        // This is to make sure that the object has correct attributes, as default values in the object are not for ES5.
        bool forceSetAttributes = false;

        PropertyDescriptor tempDescriptor;

        // ES5 8.12.9.8: If IsGenericDescriptor(Desc) is true, then no further validation is required.
        if (!descriptor.IsGenericDescriptor())
        {
            if (currentDescriptor->IsDataDescriptor() != descriptor.IsDataDescriptor())
            {
                // ES5 8.12.9.9: Else, if IsDataDescriptor(current) and IsDataDescriptor(Desc) have different results...
                if (!currentDescriptor->IsConfigurable())
                {
                    return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
                }

                if (needToSetProperty)
                {
                    if (currentDescriptor->IsDataDescriptor())
                    {
                        // ES5 8.12.9.9.b: Convert the property named P of object O from a data property to an accessor property.
                        // Preserve the existing values of the converted property's [[Configurable]] and [[Enumerable]] attributes
                        // and set the rest of the property's attributes to their default values.
                        PropertyAttributes preserveFromObject = currentDescriptor->GetAttributes() & (PropertyConfigurable | PropertyEnumerable);

                        BOOL isSetAccessorsSuccess = obj->SetAccessors(propId, defaultAccessorValue, defaultAccessorValue);

                        // It is valid for some objects to not-support getters and setters, specifically, for projection of an ABI method
                        // (CustomExternalObject => MapWithStringKey) which SetAccessors returns VBSErr_ActionNotSupported.
                        // But for non-external objects SetAccessors should succeed.
                        Assert(isSetAccessorsSuccess || obj->IsExternal());

                        if (isSetAccessorsSuccess)
                        {
                            tempDescriptor.SetAttributes(preserveFromObject, PropertyConfigurable | PropertyEnumerable);
                            forceSetAttributes = true;  // use SetAttributes with 'force' as default attributes in type system are different from ES5.
                            mergeDescriptors = true;
                        }
                    }
                    else
                    {
                        // ES5 8.12.9.9.c: Convert the property named P of object O from an accessor property to a data property.
                        // Preserve the existing values of the converted property's [[Configurable]] and [[Enumerable]] attributes
                        // and set the rest of the property's attributes to their default values.
                        // Note: avoid using SetProperty/SetPropertyWithAttributes here because they has undesired side-effects:
                        //       it calls previous setter and in some cases of attribute values throws.
                        //       To walk around, call DeleteProperty and then AddProperty.
                        PropertyAttributes preserveFromObject = currentDescriptor->GetAttributes() & (PropertyConfigurable | PropertyEnumerable);

                        tempDescriptor.SetAttributes(preserveFromObject, PropertyConfigurable | PropertyEnumerable);
                        tempDescriptor.MergeFrom(descriptor);   // Update only fields specified in 'descriptor'.
                        Var descriptorValue = descriptor.ValueSpecified() ? descriptor.GetValue() : defaultDataValue;

                        // Note: HostDispath'es implementation of DeleteProperty currently throws E_NOTIMPL.
                        obj->DeleteProperty(propId, PropertyOperation_None);
                        BOOL tempResult = obj->SetPropertyWithAttributes(propId, descriptorValue, tempDescriptor.GetAttributes(), NULL, PropertyOperation_Force);
                        Assert(tempResult);

                        // At this time we already set value and attributes to desired values,
                        // thus we can skip step ES5 8.12.9.12 and simply return true.
                        return TRUE;
                    }
                }
            }
            else if (currentDescriptor->IsDataDescriptor() && descriptor.IsDataDescriptor())
            {
                // ES5 8.12.9.10: Else, if IsDataDescriptor(current) and IsDataDescriptor(Desc) are both true...
                if (!currentDescriptor->IsConfigurable())
                {
                    if (!currentDescriptor->IsWritable())
                    {
                        if (descriptor.WritableSpecified() && descriptor.IsWritable())  // ES5 8.12.9.10.a.i
                        {
                            return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
                        }
                        else if (descriptor.ValueSpecified() &&
                                !JavascriptConversion::SameValue(descriptor.GetValue(), currentDescriptor->GetValue())) // ES5 8.12.9.10.a.ii
                        {
                            return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotWritable, propId);
                        }
                    }
                }
                // ES5 8.12.9.10.b: else, the [[Configurable]] field of current is true, so any change is acceptable.
            }
            else
            {
                // ES5 8.12.9.11: Else, IsAccessorDescriptor(current) and IsAccessorDescriptor(Desc) are both true, so...
                Assert(currentDescriptor->IsAccessorDescriptor() && descriptor.IsAccessorDescriptor());
                if (!currentDescriptor->IsConfigurable())
                {
                    if ((descriptor.SetterSpecified() &&
                            !JavascriptConversion::SameValue(
                            JavascriptOperators::CanonicalizeAccessor(descriptor.GetSetter(), scriptContext),
                                JavascriptOperators::CanonicalizeAccessor(currentDescriptor->GetSetter(), scriptContext))) ||
                        (descriptor.GetterSpecified() &&
                            !JavascriptConversion::SameValue(
                            JavascriptOperators::CanonicalizeAccessor(descriptor.GetGetter(), scriptContext),
                                JavascriptOperators::CanonicalizeAccessor(currentDescriptor->GetGetter(), scriptContext))))
                    {
                        return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
                    }
                }
            }

            // This part is only for non-generic descriptors:
            //   ES5 8.12.9.12: For each attribute field of Desc that is present,
            //   set the correspondingly named attribute of the property named P of object O to the value of the field.
            if (descriptor.IsDataDescriptor())
            {
                if (descriptor.ValueSpecified() && needToSetProperty)
                {
                    // Set just the value by passing the current attributes of the property.
                    // If the property's attributes are also changing (perhaps becoming non-writable),
                    // this will be taken care of in the call to JavascriptOperators::SetAttributes below.
                    // Built-in Function.prototype properties 'length', 'arguments', and 'caller' are special cases.
                    BOOL tempResult = obj->SetPropertyWithAttributes(propId, descriptor.GetValue(), currentDescriptor->GetAttributes(), nullptr);
                    AssertMsg(tempResult || JavascriptFunction::IsBuiltinProperty(obj, propId), "If you hit this assert, most likely there is something wrong with the object/type.");
                }
            }
            else if (descriptor.IsAccessorDescriptor() && needToSetProperty)
            {
                Assert(descriptor.GetterSpecified() || descriptor.SetterSpecified());
                Var oldGetter = defaultAccessorValue, oldSetter = defaultAccessorValue;
                if (!descriptor.GetterSpecified() || !descriptor.SetterSpecified())
                {
                    // Unless both getter and setter are specified, make sure we don't overwrite old accessor.
#pragma prefast(suppress:6031, "We defaulted oldGetter and oldSetter already, so ignoring the return value here is safe")
                    obj->GetAccessors(propId, &oldGetter, &oldSetter, scriptContext);
                }

                Var getter = descriptor.GetterSpecified() ? descriptor.GetGetter() : oldGetter;
                Var setter = descriptor.SetterSpecified() ? descriptor.GetSetter() : oldSetter;

                obj->SetAccessors(propId, getter, setter);
            }
        } // if (!descriptor.IsGenericDescriptor())

        // Continue for all descriptors including generic:
        //   ES5 8.12.9.12: For each attribute field of Desc that is present,
        //   set the correspondingly named attribute of the property named P of object O to the value of the field.
        if (needToSetProperty)
        {
            if (mergeDescriptors)
            {
                tempDescriptor.MergeFrom(descriptor);
                JavascriptOperators::SetAttributes(obj, propId, tempDescriptor, forceSetAttributes);
            }
            else
            {
                JavascriptOperators::SetAttributes(obj, propId, descriptor, forceSetAttributes);
            }
        }
        return TRUE;
    }

    template <bool isAccessor>
    PropertyDescriptor JavascriptOperators::FillMissingPropertyDescriptorFields(PropertyDescriptor descriptor, ScriptContext* scriptContext)
    {
        PropertyDescriptor newDescriptor;
        const PropertyDescriptor* defaultDescriptor = scriptContext->GetLibrary()->GetDefaultPropertyDescriptor();
        if (isAccessor)
        {
            newDescriptor.SetGetter(descriptor.GetterSpecified() ? descriptor.GetGetter() : defaultDescriptor->GetGetter());
            newDescriptor.SetSetter(descriptor.SetterSpecified() ? descriptor.GetSetter() : defaultDescriptor->GetSetter());
        }
        else
        {
            newDescriptor.SetValue(descriptor.ValueSpecified() ? descriptor.GetValue() : defaultDescriptor->GetValue());
            newDescriptor.SetWritable(descriptor.WritableSpecified() ? descriptor.IsWritable() : defaultDescriptor->IsWritable());
        }
        newDescriptor.SetConfigurable(descriptor.ConfigurableSpecified() ? descriptor.IsConfigurable() : defaultDescriptor->IsConfigurable());
        newDescriptor.SetEnumerable(descriptor.EnumerableSpecified() ? descriptor.IsEnumerable() : defaultDescriptor->IsEnumerable());
        return newDescriptor;
    }
    // ES5: 15.4.5.1
    BOOL JavascriptOperators::DefineOwnPropertyForArray(JavascriptArray* arr, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* scriptContext)
    {
        if (propId == PropertyIds::length)
        {
            if (!descriptor.ValueSpecified())
            {
                return DefineOwnPropertyDescriptor(arr, PropertyIds::length, descriptor, throwOnError, scriptContext);
            }

            PropertyDescriptor newLenDesc = descriptor;
            uint32 newLen = ES5Array::ToLengthValue(descriptor.GetValue(), scriptContext);
            newLenDesc.SetValue(JavascriptNumber::ToVar(newLen, scriptContext));

            uint32 oldLen = arr->GetLength();
            if (newLen >= oldLen)
            {
                return DefineOwnPropertyDescriptor(arr, PropertyIds::length, newLenDesc, throwOnError, scriptContext);
            }

            BOOL oldLenWritable = arr->IsWritable(PropertyIds::length);
            if (!oldLenWritable)
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotWritable, propId);
            }

            bool newWritable = (!newLenDesc.WritableSpecified() || newLenDesc.IsWritable());
            if (!newWritable)
            {
                // Need to defer setting writable to false in case any elements cannot be deleted
                newLenDesc.SetWritable(true);
            }

            BOOL succeeded = DefineOwnPropertyDescriptor(arr, PropertyIds::length, newLenDesc, throwOnError, scriptContext);
            //
            // Our SetProperty(length) is also responsible to trim elements. When succeeded is
            //
            //  false:
            //      * length attributes rejected
            //      * elements not touched
            //  true:
            //      * length attributes are set successfully
            //      * elements trimming may be either completed or incompleted, length value is correct
            //
            //      * Strict mode TODO: Currently SetProperty(length) does not throw. If that throws, we need
            //        to update here to set correct newWritable even on exception.
            //
            if (!succeeded)
            {
                return false;
            }

            if (!newWritable) // Now set requested newWritable.
            {
                PropertyDescriptor newWritableDesc;
                newWritableDesc.SetWritable(false);
                DefineOwnPropertyDescriptor(arr, PropertyIds::length, newWritableDesc, false, scriptContext);
            }

            if (arr->GetLength() > newLen) // Delete incompleted
            {
                // Since SetProperty(length) not throwing, we'll reject here
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_Default, propId);
            }

            return true;
        }

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propId, &index))
        {
            if (index >= arr->GetLength() && !arr->IsWritable(PropertyIds::length))
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_LengthNotWritable, propId);
            }

            BOOL succeeded = DefineOwnPropertyDescriptor(arr, propId, descriptor, false, scriptContext);
            if (!succeeded)
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_Default, propId);
            }

            // Out SetItem takes care of growing "length". we are done.
            return true;
        }

        return DefineOwnPropertyDescriptor(arr, propId, descriptor, throwOnError, scriptContext);
    }

    // ES2017: 9.4.5.3 https://tc39.github.io/ecma262/#sec-integer-indexed-exotic-objects-defineownproperty-p-desc
    BOOL JavascriptOperators::DefineOwnPropertyForTypedArray(TypedArrayBase* typedArray, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* scriptContext)
    {
        // 1. Assert: IsPropertyKey(P) is true.
        // 2. Assert: Assert: O is an Object that has a [[ViewedArrayBuffer]] internal slot.

        const PropertyRecord* propertyRecord = scriptContext->GetPropertyName(propId);
        // 3. If Type(P) is String, then
        // a. Let numericIndex be ! CanonicalNumericIndexString(P).
        // b. If numericIndex is not undefined, then
        // i. if IsInteger(numbericIndex), return false
        // ii. if numbericIndex = -0, return false
        // iii. If numericIndex < 0, return false.

        if (propertyRecord->IsNumeric()) {
            uint32 uint32Index = propertyRecord->GetNumericValue();
            // iv. Let length be O.[[ArrayLength]].
            uint32 length = typedArray->GetLength();
            // v. If numericIndex >= length, return false.
            if (uint32Index >= length)
            {
                return Reject(throwOnError, scriptContext, JSERR_InvalidTypedArrayIndex, propId);
            }
            // vi. If IsAccessorDescriptor(Desc) is true, return false.
            // vii. If Desc has a[[Configurable]] field and if Desc.[[Configurable]] is true, return false.
            // viii. If Desc has an[[Enumerable]] field and if Desc.[[Enumerable]] is false, return false.
            // ix. If Desc has a[[Writable]] field and if Desc.[[Writable]] is false, return false.
            if (descriptor.IsAccessorDescriptor()
                || (descriptor.ConfigurableSpecified() && descriptor.IsConfigurable())
                || (descriptor.EnumerableSpecified() && !descriptor.IsEnumerable())
                || (descriptor.WritableSpecified() && !descriptor.IsWritable()))
            {
                return Reject(throwOnError, scriptContext, JSERR_DefineProperty_NotConfigurable, propId);
            }            // x. If Desc has a[[Value]] field, then
            // 1. Let value be Desc.[[Value]].
            // 2. Return ? IntegerIndexedElementSet(O, numericIndex, value).
            if (descriptor.ValueSpecified())
            {
                Js::Var value = descriptor.GetValue();
                return typedArray->DirectSetItem(uint32Index, value);
            }
            // xi. Return true.
            return true;
        }
        if (!propertyRecord->IsSymbol())
        {
            PropertyString *propertyString = scriptContext->GetPropertyString(propId);
            double result;
            if (JavascriptConversion::CanonicalNumericIndexString(propertyString, &result, scriptContext))
            {
                return Reject(throwOnError, scriptContext, JSERR_InvalidTypedArrayIndex, propId);
            }
        }
        // 4. Return ! OrdinaryDefineOwnProperty(O, P, Desc).
        return DefineOwnPropertyDescriptor(typedArray, propId, descriptor, throwOnError, scriptContext);
    }


    BOOL JavascriptOperators::SetPropertyDescriptor(RecyclableObject* object, PropertyId propId, const PropertyDescriptor& descriptor)
    {
        if (descriptor.ValueSpecified())
        {
            ScriptContext* requestContext = object->GetScriptContext(); // Real requestContext?
            JavascriptOperators::SetProperty(object, object, propId, descriptor.GetValue(), requestContext);
        }
        else if (descriptor.GetterSpecified() || descriptor.SetterSpecified())
        {
            JavascriptOperators::SetAccessors(object, propId, descriptor.GetGetter(), descriptor.GetSetter());
        }

        if (descriptor.EnumerableSpecified())
        {
            object->SetEnumerable(propId, descriptor.IsEnumerable());
        }
        if (descriptor.ConfigurableSpecified())
        {
            object->SetConfigurable(propId, descriptor.IsConfigurable());
        }
        if (descriptor.WritableSpecified())
        {
            object->SetWritable(propId, descriptor.IsWritable());
        }

        return true;
    }

    BOOL JavascriptOperators::ToPropertyDescriptorForProxyObjects(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext)
    {
        if (!JavascriptOperators::IsObject(propertySpec))
        {
            return FALSE;
        }

        Var value;
        RecyclableObject* propertySpecObj = VarTo<RecyclableObject>(propertySpec);

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::enumerable) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::enumerable, &value, scriptContext))
            {
                descriptor->SetEnumerable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetEnumerable(false);
            }
        }

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::configurable) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::configurable, &value, scriptContext))
            {
                descriptor->SetConfigurable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetConfigurable(false);
            }
        }

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::value) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::value, &value, scriptContext))
            {
                descriptor->SetValue(value);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetValue(scriptContext->GetLibrary()->GetUndefined());
            }
        }

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::writable) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::writable, &value, scriptContext))
            {
                descriptor->SetWritable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetWritable(false);
            }
        }

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::get) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::get, &value, scriptContext))
            {
                if (JavascriptOperators::GetTypeId(value) != TypeIds_Undefined && (false == JavascriptConversion::IsCallable(value)))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, scriptContext->GetPropertyName(PropertyIds::get)->GetBuffer());
                }
                descriptor->SetGetter(value);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetGetter(scriptContext->GetLibrary()->GetUndefined());
            }
        }

        if (JavascriptOperators::HasProperty(propertySpecObj, PropertyIds::set) == TRUE)
        {
            if (JavascriptOperators::GetProperty(propertySpecObj, PropertyIds::set, &value, scriptContext))
            {
                if (JavascriptOperators::GetTypeId(value) != TypeIds_Undefined && (false == JavascriptConversion::IsCallable(value)))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, scriptContext->GetPropertyName(PropertyIds::set)->GetBuffer());
                }
                descriptor->SetSetter(value);
            }
            else
            {
                // The proxy said we have the property, so we try to read the property and get the default value.
                descriptor->SetSetter(scriptContext->GetLibrary()->GetUndefined());
            }
        }

        return TRUE;
    }

    BOOL JavascriptOperators::ToPropertyDescriptorForGenericObjects(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext)
    {
        if (!JavascriptOperators::IsObject(propertySpec))
        {
            return FALSE;
        }

        Var value;
        RecyclableObject* propertySpecObj = VarTo<RecyclableObject>(propertySpec);

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::enumerable, &value, scriptContext))
        {
            descriptor->SetEnumerable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
        }

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::configurable, &value, scriptContext))
        {
            descriptor->SetConfigurable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
        }

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::value, &value, scriptContext))
        {
            descriptor->SetValue(value);
        }

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::writable, &value, scriptContext))
        {
            descriptor->SetWritable(JavascriptConversion::ToBoolean(value, scriptContext) ? true : false);
        }

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::get, &value, scriptContext))
        {
            if (JavascriptOperators::GetTypeId(value) != TypeIds_Undefined && (false == JavascriptConversion::IsCallable(value)))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, scriptContext->GetPropertyName(PropertyIds::get)->GetBuffer());
            }
            descriptor->SetGetter(value);
        }

        if (JavascriptOperators::GetPropertyNoCache(propertySpecObj, PropertyIds::set, &value, scriptContext))
        {
            if (JavascriptOperators::GetTypeId(value) != TypeIds_Undefined && (false == JavascriptConversion::IsCallable(value)))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, scriptContext->GetPropertyName(PropertyIds::set)->GetBuffer());
            }
            descriptor->SetSetter(value);
        }

        return TRUE;
    }

    BOOL JavascriptOperators::ToPropertyDescriptor(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext)
    {
        if (VarIs<JavascriptProxy>(propertySpec) || (
            VarIs<RecyclableObject>(propertySpec) &&
            JavascriptOperators::CheckIfPrototypeChainContainsProxyObject(VarTo<RecyclableObject>(propertySpec)->GetPrototype())))
        {
            if (ToPropertyDescriptorForProxyObjects(propertySpec, descriptor, scriptContext) == FALSE)
            {
                return FALSE;
            }
        }
        else
        {
            if (ToPropertyDescriptorForGenericObjects(propertySpec, descriptor, scriptContext) == FALSE)
            {
                return FALSE;
            }
        }

        if (descriptor->GetterSpecified() || descriptor->SetterSpecified())
        {
            if (descriptor->ValueSpecified())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_CannotHaveAccessorsAndValue);
            }
            if (descriptor->WritableSpecified())
            {
                int32 hCode = descriptor->IsWritable() ? JSERR_InvalidAttributeTrue : JSERR_InvalidAttributeFalse;
                JavascriptError::ThrowTypeError(scriptContext, hCode, _u("writable"));
            }
        }

        descriptor->SetOriginal(propertySpec);

        return TRUE;
    }

    void JavascriptOperators::CompletePropertyDescriptor(PropertyDescriptor* resultDescriptor, PropertyDescriptor* likeDescriptor, ScriptContext* requestContext)
    {
        const PropertyDescriptor* likePropertyDescriptor = likeDescriptor;
        //    1. Assert: LikeDesc is either a Property Descriptor or undefined.
        //    2. ReturnIfAbrupt(Desc).
        //    3. Assert : Desc is a Property Descriptor
        //    4. If LikeDesc is undefined, then set LikeDesc to Record{ [[Value]]: undefined, [[Writable]] : false, [[Get]] : undefined, [[Set]] : undefined, [[Enumerable]] : false, [[Configurable]] : false }.
        if (likePropertyDescriptor == nullptr)
        {
            likePropertyDescriptor = requestContext->GetLibrary()->GetDefaultPropertyDescriptor();
        }
        //    5. If either IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then
        if (resultDescriptor->IsDataDescriptor() || resultDescriptor->IsGenericDescriptor())
        {
            //    a.If Desc does not have a[[Value]] field, then set Desc.[[Value]] to LikeDesc.[[Value]].
            //    b.If Desc does not have a[[Writable]] field, then set Desc.[[Writable]] to LikeDesc.[[Writable]].
            if (!resultDescriptor->ValueSpecified())
            {
                resultDescriptor->SetValue(likePropertyDescriptor->GetValue());
            }
            if (!resultDescriptor->WritableSpecified())
            {
                resultDescriptor->SetWritable(likePropertyDescriptor->IsWritable());
            }
        }
        else
        {
            //    6. Else,
            //    a.If Desc does not have a[[Get]] field, then set Desc.[[Get]] to LikeDesc.[[Get]].
            //    b.If Desc does not have a[[Set]] field, then set Desc.[[Set]] to LikeDesc.[[Set]].
            if (!resultDescriptor->GetterSpecified())
            {
                resultDescriptor->SetGetter(likePropertyDescriptor->GetGetter());
            }
            if (!resultDescriptor->SetterSpecified())
            {
                resultDescriptor->SetSetter(likePropertyDescriptor->GetSetter());
            }
        }
        //    7. If Desc does not have an[[Enumerable]] field, then set Desc.[[Enumerable]] to LikeDesc.[[Enumerable]].
        //    8. If Desc does not have a[[Configurable]] field, then set Desc.[[Configurable]] to LikeDesc.[[Configurable]].
        //    9. Return Desc.
        if (!resultDescriptor->EnumerableSpecified())
        {
            resultDescriptor->SetEnumerable(likePropertyDescriptor->IsEnumerable());
        }
        if (!resultDescriptor->ConfigurableSpecified())
        {
            resultDescriptor->SetConfigurable(likePropertyDescriptor->IsConfigurable());
        }
    }

    // Conformance to: ES5 8.6.1.
    // Set attributes on the object as provided by property descriptor.
    // If force parameter is true, we force SetAttributes call even if none of the attributes are defined by the descriptor.
    // NOTE: does not set [[Get]], [Set]], [[Value]]
    void JavascriptOperators::SetAttributes(RecyclableObject* object, PropertyId propId, const PropertyDescriptor& descriptor, bool force)
    {
        Assert(object);

        BOOL isWritable = FALSE;
        if (descriptor.IsDataDescriptor())
        {
            isWritable = descriptor.WritableSpecified() ? descriptor.IsWritable() : FALSE;
        }
        else if (descriptor.IsAccessorDescriptor())
        {
            // The reason is that JavascriptOperators::OP_SetProperty checks for VarTo<RecyclableObject>(instance)->IsWritableOrAccessor(propertyId),
            // which should in fact check for 'is writable or accessor' but since there is no GetAttributes, we can't do that efficiently.
            isWritable = TRUE;
        }

        // CONSIDER: call object->SetAttributes which is much more efficient as that's 1 call instead of 3.
        //       Can't do that now as object->SetAttributes doesn't provide a way which attributes to modify and which not.
        if (force || descriptor.ConfigurableSpecified())
        {
            object->SetConfigurable(propId, descriptor.ConfigurableSpecified() ? descriptor.IsConfigurable() : FALSE);
        }
        if (force || descriptor.EnumerableSpecified())
        {
            object->SetEnumerable(propId, descriptor.EnumerableSpecified() ? descriptor.IsEnumerable() : FALSE);
        }
        if (force || descriptor.WritableSpecified() || isWritable)
        {
            object->SetWritable(propId, isWritable);
        }
    }

    void JavascriptOperators::OP_ClearAttributes(Var instance, PropertyId propertyId)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(OP_ClearAttributes);
        Assert(instance);

        if (VarIs<RecyclableObject>(instance))
        {
            RecyclableObject* obj = VarTo<RecyclableObject>(instance);
            obj->SetAttributes(propertyId, PropertyNone);
        }
        JIT_HELPER_END(OP_ClearAttributes);
    }

    BOOL JavascriptOperators::Reject(bool throwOnError, ScriptContext* scriptContext, int32 errorCode, PropertyId propertyId)
    {
        Assert(scriptContext);

        if (throwOnError)
        {
            JavascriptError::ThrowTypeError(scriptContext, errorCode, scriptContext->GetThreadContext()->GetPropertyName(propertyId)->GetBuffer());
        }
        return FALSE;
    }

    bool JavascriptOperators::AreSamePropertyDescriptors(const PropertyDescriptor* x, const PropertyDescriptor* y, ScriptContext* scriptContext)
    {
        Assert(scriptContext);

        if (x->ConfigurableSpecified() != y->ConfigurableSpecified() || x->IsConfigurable() != y->IsConfigurable() ||
            x->EnumerableSpecified() != y->EnumerableSpecified() || x->IsEnumerable() != y->IsEnumerable())
        {
            return false;
        }

        if (x->IsDataDescriptor())
        {
            if (!y->IsDataDescriptor() || x->WritableSpecified() != y->WritableSpecified() || x->IsWritable() != y->IsWritable())
            {
                return false;
            }

            if (x->ValueSpecified())
            {
                if (!y->ValueSpecified() || !JavascriptConversion::SameValue(x->GetValue(), y->GetValue()))
                {
                    return false;
                }
            }
        }
        else if (x->IsAccessorDescriptor())
        {
            if (!y->IsAccessorDescriptor())
            {
                return false;
            }

            if (x->GetterSpecified())
            {
                if (!y->GetterSpecified() || !JavascriptConversion::SameValue(
                    JavascriptOperators::CanonicalizeAccessor(x->GetGetter(), scriptContext),
                    JavascriptOperators::CanonicalizeAccessor(y->GetGetter(), scriptContext)))
                {
                    return false;
                }
            }

            if (x->SetterSpecified())
            {
                if (!y->SetterSpecified() || !JavascriptConversion::SameValue(
                    JavascriptOperators::CanonicalizeAccessor(x->GetSetter(), scriptContext),
                    JavascriptOperators::CanonicalizeAccessor(y->GetSetter(), scriptContext)))
                {
                    return false;
                }
            }
        }

        return true;
    }

    // Check if an accessor is undefined (null or defaultAccessor)
    bool JavascriptOperators::IsUndefinedAccessor(Var accessor, ScriptContext* scriptContext)
    {
        return nullptr == accessor || scriptContext->GetLibrary()->GetDefaultAccessorFunction() == accessor;
    }

    // Converts default accessor to undefined.
    // Can be used when comparing accessors.
    Var JavascriptOperators::CanonicalizeAccessor(Var accessor, ScriptContext* scriptContext)
    {
        Assert(scriptContext);

        if (IsUndefinedAccessor(accessor, scriptContext))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        return accessor;
    }

    Var JavascriptOperators::DefaultAccessor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        return function->GetLibrary()->GetUndefined();
    }

    void FrameDisplay::SetItem(uint index, void* item)
    {
        AssertMsg(index < this->length, "Invalid frame display access");

        scopes[index] = item;
    }

    void *FrameDisplay::GetItem(uint index)
    {
        AssertMsg(index < this->length, "Invalid frame display access");

        return scopes[index];
    }

    // Grab the "this" pointer, mapping a root object to its associated host object.
    Var JavascriptOperators::RootToThisObject(const Var object, ScriptContext* scriptContext)
    {
        Js::Var thisVar = object;
        TypeId typeId = Js::JavascriptOperators::GetTypeId(thisVar);

        switch (typeId)
        {
        case Js::TypeIds_GlobalObject:
            return ((Js::GlobalObject*)thisVar)->ToThis();

        case Js::TypeIds_ModuleRoot:
            return Js::JavascriptOperators::GetThisFromModuleRoot(thisVar);

        default:
            if (typeId == scriptContext->GetDirectHostTypeId())
            {
                return ((RecyclableObject*)thisVar)->GetLibrary()->GetGlobalObject()->ToThis();
            }

        }

        return thisVar;
    }

    Var JavascriptOperators::CallGetter(RecyclableObject * const function, Var const object, ScriptContext * requestContext)
    {
#if ENABLE_TTD
        if(function->GetScriptContext()->ShouldSuppressGetterInvocationForDebuggerEvaluation())
        {
            return requestContext->GetLibrary()->GetUndefined();
        }
#endif

        ScriptContext * scriptContext = function->GetScriptContext();
        ThreadContext * threadContext = scriptContext->GetThreadContext();
        return threadContext->ExecuteImplicitCall(function, ImplicitCall_Accessor, [=]() -> Js::Var
        {
            // Stack object should have a pre-op bail on implicit call.  We shouldn't see them here.
            // Stack numbers are ok, as we will call ToObject to wrap it in a number object anyway
            // See JavascriptOperators::GetThisHelper
            Assert(JavascriptOperators::GetTypeId(object) == TypeIds_Integer ||
                JavascriptOperators::GetTypeId(object) == TypeIds_Number ||
                threadContext->HasNoSideEffect(function) ||
                !ThreadContext::IsOnStack(object));

            // Verify that the scriptcontext is alive before firing getter/setter
            if (!scriptContext->VerifyAlive(!function->IsExternal(), requestContext))
            {
                return nullptr;
            }
            CallFlags flags = CallFlags_Value;

            Var thisVar = RootToThisObject(object, scriptContext);

            RecyclableObject* marshalledFunction = UnsafeVarTo<RecyclableObject>(
              CrossSite::MarshalVar(requestContext, function, scriptContext));

            Var result = CALL_ENTRYPOINT(threadContext, marshalledFunction->GetEntryPoint(), function, CallInfo(flags, 1), thisVar);
            result = CrossSite::MarshalVar(requestContext, result);

            // Set implicit call flags so we bail out if we're trying to propagate the value forward, e.g., from a compare. Subsequent calls
            // to the getter may produce different results.
            threadContext->AddImplicitCallFlags(ImplicitCall_Accessor);

            return result;
        });
    }

    void JavascriptOperators::CallSetter(RecyclableObject * const function, Var const  object, Var const value, ScriptContext * requestContext)
    {
        ScriptContext * scriptContext = function->GetScriptContext();
        ThreadContext * threadContext = scriptContext->GetThreadContext();
        threadContext->ExecuteImplicitCall(function, ImplicitCall_Accessor, [=]() -> Js::Var
        {
            // Stack object should have a pre-op bail on implicit call.  We shouldn't see them here.
            // Stack numbers are ok, as we will call ToObject to wrap it in a number object anyway
            // See JavascriptOperators::GetThisHelper
            Assert(JavascriptOperators::GetTypeId(object) == TypeIds_Integer ||
                JavascriptOperators::GetTypeId(object) == TypeIds_Number || !ThreadContext::IsOnStack(object));

            // Verify that the scriptcontext is alive before firing getter/setter
            if (!scriptContext->VerifyAlive(!function->IsExternal(), requestContext))
            {
                return nullptr;
            }

            CallFlags flags = CallFlags_Value;
            Var putValue = value;

            // CONSIDER: Have requestContext everywhere, even in the setProperty related codepath.
            if (requestContext)
            {
                putValue = CrossSite::MarshalVar(requestContext, value);
            }

            Var thisVar = RootToThisObject(object, scriptContext);

            RecyclableObject* marshalledFunction = function;
            if (requestContext)
            {
                marshalledFunction = UnsafeVarTo<RecyclableObject>(CrossSite::MarshalVar(requestContext, function, function->GetScriptContext()));
            }

            Var result = CALL_ENTRYPOINT(threadContext, marshalledFunction->GetEntryPoint(), function, CallInfo(flags, 2), thisVar, putValue);
            Assert(result);

            // Set implicit call flags so we bail out if we're trying to propagate the stored value forward. We can't count on the getter/setter
            // to produce the stored value on a LdFld.
            threadContext->AddImplicitCallFlags(ImplicitCall_Accessor);

            return nullptr;
        });
    }

    void * JavascriptOperators::AllocMemForVarArray(size_t size, Recycler* recycler)
    {
        TRACK_ALLOC_INFO(recycler, Js::Var, Recycler, 0, (size_t)(size / sizeof(Js::Var)));
        return recycler->AllocZero(size);
    }

#if !FLOATVAR
    void * JavascriptOperators::AllocUninitializedNumber(Js::RecyclerJavascriptNumberAllocator * allocator)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(AllocUninitializedNumber);
        TRACK_ALLOC_INFO(allocator->GetRecycler(), Js::JavascriptNumber, Recycler, 0, (size_t)-1);
        return allocator->Alloc(sizeof(Js::JavascriptNumber));
        JIT_HELPER_END(AllocUninitializedNumber);
    }
#endif

    void JavascriptOperators::ScriptAbort()
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScriptAbort);
        throw ScriptAbortException();
        JIT_HELPER_END(ScriptAbort);
    }

    JavascriptString * JavascriptOperators::Concat3(Var aLeft, Var aCenter, Var aRight, ScriptContext * scriptContext)
    {
        // Make sure we do the conversion in order from left to right
        JavascriptString * strLeft = JavascriptConversion::ToPrimitiveString(aLeft, scriptContext);
        JavascriptString * strCenter = JavascriptConversion::ToPrimitiveString(aCenter, scriptContext);
        JavascriptString * strRight = JavascriptConversion::ToPrimitiveString(aRight, scriptContext);
        return JavascriptString::Concat3(strLeft, strCenter, strRight);
    }

    JavascriptString *
    JavascriptOperators::NewConcatStrMulti(Var a1, Var a2, uint count, ScriptContext * scriptContext)
    {
        // Make sure we do the conversion in order
        JavascriptString * str1 = JavascriptConversion::ToPrimitiveString(a1, scriptContext);
        JavascriptString * str2 = JavascriptConversion::ToPrimitiveString(a2, scriptContext);
        return ConcatStringMulti::New(count, str1, str2, scriptContext);
    }

    void
    JavascriptOperators::SetConcatStrMultiItem(Var concatStr, Var str, uint index, ScriptContext * scriptContext)
    {
        VarTo<ConcatStringMulti>(concatStr)->SetItem(index,
            JavascriptConversion::ToPrimitiveString(str, scriptContext));
    }

    void
    JavascriptOperators::SetConcatStrMultiItem2(Var concatStr, Var str1, Var str2, uint index, ScriptContext * scriptContext)
    {
        ConcatStringMulti * cs = VarTo<ConcatStringMulti>(concatStr);
        cs->SetItem(index, JavascriptConversion::ToPrimitiveString(str1, scriptContext));
        cs->SetItem(index + 1, JavascriptConversion::ToPrimitiveString(str2, scriptContext));
    }

    void JavascriptOperators::OP_SetComputedNameVar(Var method, Var computedNameVar)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(SetComputedNameVar);
        ScriptFunctionBase *scriptFunction = VarTo<ScriptFunctionBase>(method);
        scriptFunction->SetComputedNameVar(computedNameVar);
        JIT_HELPER_END(SetComputedNameVar);
    }

    void JavascriptOperators::OP_SetHomeObj(Var method, Var homeObj)
    {
        ScriptFunctionBase *scriptFunction = VarTo<ScriptFunctionBase>(method);
        JIT_HELPER_NOT_REENTRANT_HEADER(SetHomeObj, reentrancylock, scriptFunction->GetScriptContext()->GetThreadContext());
        scriptFunction->SetHomeObj(homeObj);
        JIT_HELPER_END(SetHomeObj);
    }

    Var JavascriptOperators::OP_LdHomeObj(Var scriptFunction, ScriptContext * scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(LdHomeObj, reentrancylock, scriptContext->GetThreadContext());
        // Ensure this is not a stack ScriptFunction
        if (!VarIs<ScriptFunction>(scriptFunction) || ThreadContext::IsOnStack(scriptFunction))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        ScriptFunction *instance = UnsafeVarTo<ScriptFunction>(scriptFunction);

        // We keep a reference to the current class rather than its super prototype
        // since the prototype could change.
        Var homeObj = instance->GetHomeObj();

        return (homeObj != nullptr) ? homeObj : scriptContext->GetLibrary()->GetUndefined();
        JIT_HELPER_END(LdHomeObj);
    }

    Var JavascriptOperators::OP_LdHomeObjProto(Var homeObj, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(LdHomeObjProto, reentrancylock, scriptContext->GetThreadContext());
        if (homeObj == nullptr || !VarIs<RecyclableObject>(homeObj))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        RecyclableObject *thisObjPrototype = VarTo<RecyclableObject>(homeObj);

        TypeId typeId = thisObjPrototype->GetTypeId();

        if (typeId == TypeIds_Null || typeId == TypeIds_Undefined)
        {
            JavascriptError::ThrowReferenceError(scriptContext, JSERR_BadSuperReference);
        }

        Assert(thisObjPrototype != nullptr);

        RecyclableObject *superBase = thisObjPrototype->GetPrototype();

        if (superBase == nullptr || !VarIsCorrectType(superBase))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return superBase;
        JIT_HELPER_END(LdHomeObjProto);
    }

    Var JavascriptOperators::OP_LdFuncObj(Var scriptFunction, ScriptContext * scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(LdFuncObj);
        // use self as value of [[FunctionObject]] - this is true only for constructors

        Assert(VarIs<RecyclableObject>(scriptFunction));

        return scriptFunction;
        JIT_HELPER_END(LdFuncObj);
    }

    Var JavascriptOperators::OP_LdFuncObjProto(Var funcObj, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(LdFuncObjProto, reentrancylock, scriptContext->GetThreadContext());
        RecyclableObject *superCtor = VarTo<RecyclableObject>(funcObj)->GetPrototype();

        if (superCtor == nullptr || !IsConstructor(superCtor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NotAConstructor);
        }

        return superCtor;
        JIT_HELPER_END(LdFuncObjProto);
    }

    Var JavascriptOperators::OP_ImportCall(__in JavascriptFunction *function, __in Var specifier, __in ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(ImportCall);
        ModuleRecordBase *moduleRecordBase = nullptr;
        SourceTextModuleRecord *moduleRecord = nullptr;

        FunctionBody* parentFuncBody = function->GetFunctionBody();
        JavascriptString *specifierString = nullptr;

        try
        {
            specifierString = JavascriptConversion::ToString(specifier, scriptContext);
        }
        catch (const JavascriptException &err)
        {
            Var errorObject = err.GetAndClear()->GetThrownObject(scriptContext);
            AssertMsg(errorObject != nullptr, "OP_ImportCall: null error object thrown by ToString(specifier)");
            if (errorObject != nullptr)
            {
                return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, errorObject, scriptContext);
            }

            Throw::InternalError();
        }

        DWORD_PTR dwReferencingSourceContext = parentFuncBody->GetHostSourceContext();
        if (!parentFuncBody->IsES6ModuleCode() && dwReferencingSourceContext == Js::Constants::NoHostSourceContext)
        {
            // import() called from eval
            if (parentFuncBody->GetUtf8SourceInfo()->GetCallerUtf8SourceInfo() == nullptr)
            {
                JavascriptError *error = scriptContext->GetLibrary()->CreateError();
                JavascriptError::SetErrorMessageProperties(error, E_FAIL, _u("Unable to locate active script or module that calls import()"), scriptContext);
                return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, error, scriptContext);
            }

            dwReferencingSourceContext = parentFuncBody->GetUtf8SourceInfo()->GetCallerUtf8SourceInfo()->GetSourceContextInfo()->dwHostSourceContext;

            if (dwReferencingSourceContext == Js::Constants::NoHostSourceContext)
            {
                // Walk the call stack if caller function is neither module code nor having host source context

                JavascriptFunction* caller = nullptr;
                Js::JavascriptStackWalker walker(scriptContext);
                walker.GetCaller(&caller);

                do
                {
                    if (walker.GetCaller(&caller) && caller != nullptr && caller->IsScriptFunction())
                    {
                        parentFuncBody = caller->GetFunctionBody();
                        dwReferencingSourceContext = parentFuncBody->GetHostSourceContext();
                    }
                    else
                    {
                        JavascriptError *error = scriptContext->GetLibrary()->CreateError();
                        JavascriptError::SetErrorMessageProperties(error, E_FAIL, _u("Unable to locate active script or module that calls import()"), scriptContext);
                        return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, error, scriptContext);
                    }

                } while (!parentFuncBody->IsES6ModuleCode() && dwReferencingSourceContext == Js::Constants::NoHostSourceContext);
            }
        }

        LPCOLESTR moduleName = specifierString->GetSz();
        HRESULT hr = 0;

        if (parentFuncBody->IsES6ModuleCode())
        {
            SourceTextModuleRecord *referenceModuleRecord = parentFuncBody->GetScriptContext()->GetLibrary()->GetModuleRecord(parentFuncBody->GetModuleID());
            BEGIN_LEAVE_SCRIPT(scriptContext);
            BEGIN_TRANSLATE_TO_HRESULT(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            hr = scriptContext->GetHostScriptContext()->FetchImportedModule(referenceModuleRecord, moduleName, &moduleRecordBase);
            END_TRANSLATE_EXCEPTION_TO_HRESULT(hr);
            END_LEAVE_SCRIPT(scriptContext);
        }
        else
        {
            Assert(dwReferencingSourceContext != Js::Constants::NoHostSourceContext);
            BEGIN_LEAVE_SCRIPT(scriptContext);
            BEGIN_TRANSLATE_TO_HRESULT(static_cast<ExceptionType>(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            hr = scriptContext->GetHostScriptContext()->FetchImportedModuleFromScript(dwReferencingSourceContext, moduleName, &moduleRecordBase);
            END_TRANSLATE_EXCEPTION_TO_HRESULT(hr);
            END_LEAVE_SCRIPT(scriptContext);
        }

        if (FAILED(hr))
        {
            // We cannot just use the buffer in the specifier string - need to make a copy here.
            size_t length = wcslen(moduleName);
            char16* allocatedString = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, length + 1);
            wmemcpy_s(allocatedString, length + 1, moduleName, length);
            allocatedString[length] = _u('\0');

            Js::JavascriptError *error = scriptContext->GetLibrary()->CreateURIError();
            JavascriptError::SetErrorMessageProperties(error, hr, allocatedString, scriptContext);
            return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, error, scriptContext);
        }

        moduleRecord = SourceTextModuleRecord::FromHost(moduleRecordBase);

        if (moduleRecord->GetErrorObject() != nullptr)
        {
            return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(false, moduleRecord->GetErrorObject(), scriptContext, moduleRecord);
        }
        else if (moduleRecord->WasEvaluated())
        {
            return SourceTextModuleRecord::ResolveOrRejectDynamicImportPromise(true, moduleRecord->GetNamespace(), scriptContext, moduleRecord);
        }

        return moduleRecord->PostProcessDynamicModuleImport();
        JIT_HELPER_END(ImportCall);
    }

    Var JavascriptOperators::OP_NewAwaitObject(ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(NewAwaitObject);
        auto* awaitObject = DynamicObject::New(
            scriptContext->GetRecycler(),
            scriptContext->GetLibrary()->GetAwaitObjectType());
        return awaitObject;
        JIT_HELPER_END(NewAwaitObject);
    }

    Var JavascriptOperators::OP_NewAsyncFromSyncIterator(Var syncIterator, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(NewAsyncFromSyncIterator);
        return scriptContext->GetLibrary()->CreateAsyncFromSyncIterator(VarTo<RecyclableObject>(syncIterator));
        JIT_HELPER_END(NewAsyncFromSyncIterator);
    }

    Js::Var
    JavascriptOperators::BoxStackInstance(Js::Var instance, ScriptContext * scriptContext, bool allowStackFunction, bool deepCopy)
    {
        if (!ThreadContext::IsOnStack(instance) || (allowStackFunction && !TaggedNumber::Is(instance) && (*(int*)instance & 1)))
        {
            return instance;
        }

        TypeId typeId = JavascriptOperators::GetTypeId(instance);
        switch (typeId)
        {
        case Js::TypeIds_Number:
#if !FLOATVAR
            return JavascriptNumber::BoxStackInstance(instance, scriptContext);
#endif
            // fall-through
        case Js::TypeIds_Integer:
            return instance;
        case Js::TypeIds_RegEx:
            return JavascriptRegExp::BoxStackInstance(VarTo<JavascriptRegExp>(instance), deepCopy);
        case Js::TypeIds_Object:
            return DynamicObject::BoxStackInstance(VarTo<DynamicObject>(instance), deepCopy);
        case Js::TypeIds_Array:
            return JavascriptArray::BoxStackInstance(UnsafeVarTo<JavascriptArray>(instance), deepCopy);
        case Js::TypeIds_NativeIntArray:
            return JavascriptNativeIntArray::BoxStackInstance(UnsafeVarTo<JavascriptNativeIntArray>(instance), deepCopy);
        case Js::TypeIds_NativeFloatArray:
            return JavascriptNativeFloatArray::BoxStackInstance(UnsafeVarTo<JavascriptNativeFloatArray>(instance), deepCopy);
        case Js::TypeIds_Function:
            Assert(allowStackFunction);
            // Stack functions are deal with not mar mark them, but by nested function escape analysis
            // in the front end.  No need to box here.
            return instance;
#if ENABLE_COPYONACCESS_ARRAY
        case Js::TypeIds_CopyOnAccessNativeIntArray:
            Assert(false);
            // fall-through
#endif
        default:
            Assert(false);
            return instance;
        };
    }
    ImplicitCallFlags
    JavascriptOperators::CacheAndClearImplicitBit(ScriptContext* scriptContext)
    {
        ImplicitCallFlags prevImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
        scriptContext->GetThreadContext()->ClearImplicitCallFlags();
        return prevImplicitCallFlags;
    }
    ImplicitCallFlags
    JavascriptOperators::CheckAndUpdateFunctionBodyWithImplicitFlag(FunctionBody* functionBody)
    {
        ScriptContext* scriptContext = functionBody->GetScriptContext();
        ImplicitCallFlags currImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
        if ((currImplicitCallFlags > ImplicitCall_None))
        {
            functionBody->SetHasOnlyThisStmts(false);
        }
        return currImplicitCallFlags;
    }
    void
    JavascriptOperators::RestoreImplicitFlag(ScriptContext* scriptContext, ImplicitCallFlags prevImplicitCallFlags, ImplicitCallFlags currImplicitCallFlags)
    {
        scriptContext->GetThreadContext()->SetImplicitCallFlags((ImplicitCallFlags)(prevImplicitCallFlags | currImplicitCallFlags));
    }

    FunctionProxy*
    JavascriptOperators::GetDeferredDeserializedFunctionProxy(JavascriptFunction* func)
    {
        FunctionProxy* proxy = func->GetFunctionProxy();
        Assert(proxy->GetFunctionInfo()->GetFunctionProxy() != proxy);
        return proxy;
    }

    template <>
    Js::Var JavascriptOperators::GetElementAtIndex(Js::JavascriptArray* arrayObject, UINT index, Js::ScriptContext* scriptContext)
    {
        Js::Var result;
        if (Js::JavascriptOperators::OP_GetElementI_ArrayFastPath(arrayObject, index, &result, scriptContext))
        {
            return result;
        }
        return scriptContext->GetMissingItemResult();
    }

    template<>
    Js::Var JavascriptOperators::GetElementAtIndex(Js::JavascriptNativeIntArray* arrayObject, UINT index, Js::ScriptContext* scriptContext)
    {
        Js::Var result;
        if (Js::JavascriptOperators::OP_GetElementI_ArrayFastPath(arrayObject, index, &result, scriptContext))
        {
            return result;
        }
        return scriptContext->GetMissingItemResult();
    }

    template<>
    Js::Var JavascriptOperators::GetElementAtIndex(Js::JavascriptNativeFloatArray* arrayObject, UINT index, Js::ScriptContext* scriptContext)
    {
        Js::Var result;
        if (Js::JavascriptOperators::OP_GetElementI_ArrayFastPath(arrayObject, index, &result, scriptContext))
        {
            return result;
        }
        return scriptContext->GetMissingItemResult();
    }

    template<>
    Js::Var JavascriptOperators::GetElementAtIndex(Js::Var* arrayObject, UINT index, Js::ScriptContext* scriptContext)
    {
        return Js::JavascriptOperators::OP_GetElementI_Int32(*arrayObject, index, scriptContext);
    }

    template<typename T>
    void JavascriptOperators::ObjectToNativeArray(T* arrayObject,
        JsNativeValueType valueType,
        __in UINT length,
        __in UINT elementSize,
        __out_bcount(length*elementSize) byte* buffer,
        Js::ScriptContext* scriptContext)
    {
        Var element;
        uint64 allocSize = UInt32Math::Mul(length, elementSize);

        // TODO:further fast path the call for things like IntArray convert to int, floatarray convert to float etc.
        // such that we don't need boxing.
        switch (valueType)
        {
        case JsInt8Type:
            AnalysisAssert(elementSize == sizeof(int8));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(int8) <= allocSize);
#pragma prefast(suppress:22102)
                ((int8*)buffer)[i] = Js::JavascriptConversion::ToInt8(element, scriptContext);
            }
            break;
        case JsUint8Type:
            AnalysisAssert(elementSize == sizeof(uint8));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(uint8) <= allocSize);
                ((uint8*)buffer)[i] = Js::JavascriptConversion::ToUInt8(element, scriptContext);
            }
            break;
        case JsInt16Type:
            AnalysisAssert(elementSize == sizeof(int16));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(int16) <= allocSize);
                ((int16*)buffer)[i] = Js::JavascriptConversion::ToInt16(element, scriptContext);
            }
            break;
        case JsUint16Type:
            AnalysisAssert(elementSize == sizeof(uint16));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(uint16) <= allocSize);
                ((uint16*)buffer)[i] = Js::JavascriptConversion::ToUInt16(element, scriptContext);
            }
            break;
        case JsInt32Type:
            AnalysisAssert(elementSize == sizeof(int32));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(int32) <= allocSize);
                ((int32*)buffer)[i] = Js::JavascriptConversion::ToInt32(element, scriptContext);
            }
            break;
        case JsUint32Type:
            AnalysisAssert(elementSize == sizeof(uint32));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(uint32) <= allocSize);
                ((uint32*)buffer)[i] = Js::JavascriptConversion::ToUInt32(element, scriptContext);
            }
            break;
        case JsInt64Type:
            AnalysisAssert(elementSize == sizeof(int64));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(int64) <= allocSize);
                ((int64*)buffer)[i] = Js::JavascriptConversion::ToInt64(element, scriptContext);
            }
            break;
        case JsUint64Type:
            AnalysisAssert(elementSize == sizeof(uint64));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(uint64) <= allocSize);
                ((uint64*)buffer)[i] = Js::JavascriptConversion::ToUInt64(element, scriptContext);
            }
            break;
        case JsFloatType:
            AnalysisAssert(elementSize == sizeof(float));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(float) <= allocSize);
                ((float*)buffer)[i] = Js::JavascriptConversion::ToFloat(element, scriptContext);
            }
            break;
        case JsDoubleType:
            AnalysisAssert(elementSize == sizeof(double));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(double) <= allocSize);
                ((double*)buffer)[i] = Js::JavascriptConversion::ToNumber(element, scriptContext);
            }
            break;
        case JsNativeStringType:
            AnalysisAssert(elementSize == sizeof(JsNativeString));
            for (UINT i = 0; i < length; i++)
            {
                element = GetElementAtIndex(arrayObject, i, scriptContext);
                AnalysisAssert((i + 1) * sizeof(JsNativeString) <= allocSize);
                Js::JavascriptString* string = Js::JavascriptConversion::ToString(element, scriptContext);
                (((JsNativeString*)buffer)[i]).str = string->GetSz();
                (((JsNativeString*)buffer)[i]).length = string->GetLength();
            }
            break;
        default:
            Assert(FALSE);
        }
    }

    void JavascriptOperators::VarToNativeArray(Var arrayObject,
        JsNativeValueType valueType,
        __in UINT length,
        __in UINT elementSize,
        __out_bcount(length*elementSize) byte* buffer,
        Js::ScriptContext* scriptContext)
    {
        Js::DynamicObject* dynamicObject = VarTo<DynamicObject>(arrayObject);
        if (dynamicObject->IsCrossSiteObject() || Js::TaggedInt::IsOverflow(length))
        {
            Js::JavascriptOperators::ObjectToNativeArray(&arrayObject, valueType, length, elementSize, buffer, scriptContext);
        }
        else
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arrayObject);
#endif
            switch (Js::JavascriptOperators::GetTypeId(arrayObject))
            {
            case TypeIds_Array:
                Js::JavascriptOperators::ObjectToNativeArray(Js::UnsafeVarTo<Js::JavascriptArray>(arrayObject), valueType, length, elementSize, buffer, scriptContext);
                break;
            case TypeIds_NativeFloatArray:
                Js::JavascriptOperators::ObjectToNativeArray(Js::UnsafeVarTo<Js::JavascriptNativeFloatArray>(arrayObject), valueType, length, elementSize, buffer, scriptContext);
                break;
            case TypeIds_NativeIntArray:
                Js::JavascriptOperators::ObjectToNativeArray(Js::UnsafeVarTo<Js::JavascriptNativeIntArray>(arrayObject), valueType, length, elementSize, buffer, scriptContext);
                break;
                // We can have more specialized template if needed.
            default:
                Js::JavascriptOperators::ObjectToNativeArray(&arrayObject, valueType, length, elementSize, buffer, scriptContext);
            }
        }
    }

    // SpeciesConstructor abstract operation as described in ES6.0 Section 7.3.20
    RecyclableObject* JavascriptOperators::SpeciesConstructor(_In_ RecyclableObject* object, _In_ JavascriptFunction* defaultConstructor, _In_ ScriptContext* scriptContext)
    {
        //1.Assert: Type(O) is Object.
        Assert(JavascriptOperators::IsObject(object));

        //2.Let C be Get(O, "constructor").
        //3.ReturnIfAbrupt(C).
        Var constructor = JavascriptOperators::GetProperty(object, PropertyIds::constructor, scriptContext);

        //4.If C is undefined, return defaultConstructor.
        if (JavascriptOperators::IsUndefinedObject(constructor))
        {
            return defaultConstructor;
        }
        //5.If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject, _u("[constructor]"));
        }
        //6.Let S be Get(C, @@species).
        //7.ReturnIfAbrupt(S).
        Var species = nullptr;
        if (!JavascriptOperators::GetProperty(VarTo<RecyclableObject>(constructor),
            PropertyIds::_symbolSpecies, &species, scriptContext)
            || JavascriptOperators::IsUndefinedOrNull(species))
        {
            //8.If S is either undefined or null, return defaultConstructor.
            return defaultConstructor;
        }
        constructor = species;

        //9.If IsConstructor(S) is true, return S.
        RecyclableObject* constructorObj = JavascriptOperators::TryFromVar<RecyclableObject>(constructor);
        if (constructorObj && JavascriptOperators::IsConstructor(constructorObj))
        {
            return constructorObj;
        }
        //10.Throw a TypeError exception.
        JavascriptError::ThrowTypeError(scriptContext, JSERR_NotAConstructor, _u("constructor[Symbol.species]"));
    }

    BOOL JavascriptOperators::GreaterEqual(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_GreaterEqual);
        if (TaggedInt::Is(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                // Works whether it is TaggedInt31 or TaggedInt32
                return ::Math::PointerCastToIntegralTruncate<int>(aLeft) >= ::Math::PointerCastToIntegralTruncate<int>(aRight);
            }
            if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return TaggedInt::ToDouble(aLeft) >= JavascriptNumber::GetValue(aRight);
            }
        }
        else if (TaggedInt::Is(aRight))
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
            {
                return JavascriptNumber::GetValue(aLeft) >= TaggedInt::ToDouble(aRight);
            }
        }
        else
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::GetValue(aLeft) >= JavascriptNumber::GetValue(aRight);
            }
        }

        return !RelationalComparisonHelper(aLeft, aRight, scriptContext, true, true);
        JIT_HELPER_END(Op_GreaterEqual);
    }

    BOOL JavascriptOperators::LessEqual(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_LessEqual);
        if (TaggedInt::Is(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                // Works whether it is TaggedInt31 or TaggedInt32
                return ::Math::PointerCastToIntegralTruncate<int>(aLeft) <= ::Math::PointerCastToIntegralTruncate<int>(aRight);
            }

            if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return TaggedInt::ToDouble(aLeft) <= JavascriptNumber::GetValue(aRight);
            }
        }
        else if (TaggedInt::Is(aRight))
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
            {
                return JavascriptNumber::GetValue(aLeft) <= TaggedInt::ToDouble(aRight);
            }
        }
        else
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::GetValue(aLeft) <= JavascriptNumber::GetValue(aRight);
            }
        }

        return !RelationalComparisonHelper(aRight, aLeft, scriptContext, false, true);
        JIT_HELPER_END(Op_LessEqual);
    }

    BOOL JavascriptOperators::NotEqual(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_NotEqual);
        JIT_HELPER_SAME_ATTRIBUTES(Op_NotEqual, Op_Equal);
        //
        // TODO: Change to use Abstract Equality Comparison Algorithm (ES3.0: S11.9.3):
        // - Evaluate left, then right, operands to preserve correct evaluation order.
        // - Call algorithm, potentially reversing arguments.
        //

        return !Equal(aLeft, aRight, scriptContext);
        JIT_HELPER_END(Op_NotEqual);
    }

    // NotStrictEqual() returns whether the two vars have strict equality, as
    // described in (ES3.0: S11.9.5, S11.9.6).
    BOOL JavascriptOperators::NotStrictEqual(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_NotStrictEqual);
        JIT_HELPER_SAME_ATTRIBUTES(Op_NotStrictEqual, Op_StrictEqual);
        return !StrictEqual(aLeft, aRight, scriptContext);
        JIT_HELPER_END(Op_NotStrictEqual);
    }

    bool JavascriptOperators::CheckIfObjectAndPrototypeChainHasOnlyWritableDataProperties(_In_ RecyclableObject* object)
    {
        return object->GetLibrary()->GetTypesWithOnlyWritablePropertyProtoChainCache()->Check(object);
    }

    bool JavascriptOperators::CheckIfPrototypeChainHasOnlyWritableDataProperties(_In_ RecyclableObject* prototype)
    {
        return prototype->GetLibrary()->GetTypesWithOnlyWritablePropertyProtoChainCache()->CheckProtoChain(prototype);
    }

    bool JavascriptOperators::CheckIfObjectAndProtoChainHasNoSpecialProperties(_In_ RecyclableObject* object)
    {
        return object->GetLibrary()->GetTypesWithNoSpecialPropertyProtoChainCache()->Check(object);
    }

    // Checks to see if the specified object (which should be a prototype object)
    // contains a proxy anywhere in the prototype chain.
    bool JavascriptOperators::CheckIfPrototypeChainContainsProxyObject(RecyclableObject* prototype)
    {
        if (prototype == nullptr)
        {
            return false;
        }

        Assert(JavascriptOperators::IsObjectOrNull(prototype));

        while (prototype->GetTypeId() != TypeIds_Null)
        {
            if (prototype->GetTypeId() == TypeIds_Proxy)
            {
                return true;
            }

            prototype = prototype->GetPrototype();
        }

        return false;
    }

    BOOL JavascriptOperators::Equal(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Equal);
        JIT_HELPER_SAME_ATTRIBUTES(Op_Equal, Op_Equal_Full);
        if (aLeft == aRight)
        {
            if (TaggedInt::Is(aLeft) || DynamicObject::IsBaseDynamicObject(aLeft))
            {
                return true;
            }
            else
            {
                return Equal_Full(aLeft, aRight, scriptContext);
            }
        }

        if (VarIs<JavascriptString>(aLeft) && VarIs<JavascriptString>(aRight))
        {
            JavascriptString* left = (JavascriptString*)aLeft;
            JavascriptString* right = (JavascriptString*)aRight;

            if (left->GetLength() == right->GetLength())
            {
                if (left->UnsafeGetBuffer() != NULL && right->UnsafeGetBuffer() != NULL)
                {
                    if (left->GetLength() == 1)
                    {
                        return left->UnsafeGetBuffer()[0] == right->UnsafeGetBuffer()[0];
                    }
                    return memcmp(left->UnsafeGetBuffer(), right->UnsafeGetBuffer(), left->GetLength() * sizeof(left->UnsafeGetBuffer()[0])) == 0;
                }
                // fall through to Equal_Full
            }
            else
            {
                return false;
            }
        }

        return Equal_Full(aLeft, aRight, scriptContext);
        JIT_HELPER_END(Op_Equal);
    }

    BOOL JavascriptOperators::Greater(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Greater);
        if (TaggedInt::Is(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                // Works whether it is TaggedInt31 or TaggedInt32
                return ::Math::PointerCastToIntegralTruncate<int>(aLeft) > ::Math::PointerCastToIntegralTruncate<int>(aRight);
            }
            if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return TaggedInt::ToDouble(aLeft) > JavascriptNumber::GetValue(aRight);
            }
        }
        else if (TaggedInt::Is(aRight))
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
            {
                return JavascriptNumber::GetValue(aLeft) > TaggedInt::ToDouble(aRight);
            }
        }
        else
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::GetValue(aLeft) > JavascriptNumber::GetValue(aRight);
            }
        }

        return Greater_Full(aLeft, aRight, scriptContext);
        JIT_HELPER_END(Op_Greater);
    }

    BOOL JavascriptOperators::Less(Var aLeft, Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_Less);
        if (TaggedInt::Is(aLeft))
        {
            if (TaggedInt::Is(aRight))
            {
                // Works whether it is TaggedInt31 or TaggedInt32
                return ::Math::PointerCastToIntegralTruncate<int>(aLeft) < ::Math::PointerCastToIntegralTruncate<int>(aRight);
            }
            if (JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return TaggedInt::ToDouble(aLeft) < JavascriptNumber::GetValue(aRight);
            }
        }
        else if (TaggedInt::Is(aRight))
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft))
            {
                return JavascriptNumber::GetValue(aLeft) < TaggedInt::ToDouble(aRight);
            }
        }
        else
        {
            if (JavascriptNumber::Is_NoTaggedIntCheck(aLeft) && JavascriptNumber::Is_NoTaggedIntCheck(aRight))
            {
                return JavascriptNumber::GetValue(aLeft) < JavascriptNumber::GetValue(aRight);
            }
        }

        return Less_Full(aLeft, aRight, scriptContext);
        JIT_HELPER_END(Op_Less);
    }

    RecyclableObject* JavascriptOperators::ToObject(Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_ConvObject, reentrancylock, scriptContext->GetThreadContext());
        RecyclableObject* object = nullptr;
        if (FALSE == JavascriptConversion::ToObject(aRight, scriptContext, &object))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject /* TODO-ERROR: get arg name - aValue */);
        }

        return object;
        JIT_HELPER_END(Op_ConvObject);
    }

    Var JavascriptOperators::ToUnscopablesWrapperObject(Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Op_NewUnscopablesWrapperObject, reentrancylock, scriptContext->GetThreadContext());
        RecyclableObject* object = VarTo<RecyclableObject>(aRight);

        UnscopablesWrapperObject* withWrapper = RecyclerNew(scriptContext->GetRecycler(), UnscopablesWrapperObject, object, scriptContext->GetLibrary()->GetWithType());
        return withWrapper;
        JIT_HELPER_END(Op_NewUnscopablesWrapperObject);
    }

    Var JavascriptOperators::ToNumber(Var aRight, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Op_ConvNumber_Full);
        if (TaggedInt::Is(aRight) || (JavascriptNumber::Is_NoTaggedIntCheck(aRight)))
        {
            return aRight;
        }

        return JavascriptNumber::ToVarIntCheck(JavascriptConversion::ToNumber_Full(aRight, scriptContext), scriptContext);
        JIT_HELPER_END(Op_ConvNumber_Full);
    }

    Var JavascriptOperators::ToNumeric(Var aRight, ScriptContext* scriptContext)
    {
        if (JavascriptOperators::GetTypeId(aRight) == TypeIds_BigInt)
        {
            return aRight;
        }
        return JavascriptOperators::ToNumber(aRight, scriptContext);
    }

    BOOL JavascriptOperators::IsObject(_In_ RecyclableObject* instance)
    {
        return GetTypeId(instance) > TypeIds_LastJavascriptPrimitiveType;
    }

    BOOL JavascriptOperators::IsObject(_In_ Var instance)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_IsObject);
        return GetTypeId(instance) > TypeIds_LastJavascriptPrimitiveType;
        JIT_HELPER_END(Op_IsObject);
    }

    BOOL JavascriptOperators::IsObjectType(TypeId typeId)
    {
        return typeId > TypeIds_LastJavascriptPrimitiveType;
    }

    BOOL JavascriptOperators::IsExposedType(TypeId typeId)
    {
        return typeId <= TypeIds_LastTrueJavascriptObjectType && typeId != TypeIds_HostDispatch;
    }

    BOOL JavascriptOperators::IsObjectOrNull(Var instance)
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Op_IsObjectOrNull);
        TypeId typeId = GetTypeId(instance);
        return IsObjectType(typeId) || typeId == TypeIds_Null;
        JIT_HELPER_END(Op_IsObjectOrNull);
    }

    BOOL JavascriptOperators::IsUndefined(_In_ RecyclableObject* instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_Undefined;
    }

    BOOL JavascriptOperators::IsUndefined(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_Undefined;
    }

    BOOL JavascriptOperators::IsUndefinedOrNullType(TypeId typeId)
    {
        return typeId <= TypeIds_UndefinedOrNull;
    }

    BOOL JavascriptOperators::IsUndefinedOrNull(Var instance)
    {
        return IsUndefinedOrNullType(JavascriptOperators::GetTypeId(instance));
    }

    BOOL JavascriptOperators::IsUndefinedOrNull(RecyclableObject* instance)
    {
        return JavascriptOperators::IsUndefinedOrNullType(instance->GetTypeId());
    }

    BOOL JavascriptOperators::IsUndefinedOrNull(Var instance, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        return IsUndefinedObject(instance, library) || IsNull(instance, library);
    }

    BOOL JavascriptOperators::IsUndefinedOrNull(Var instance, JavascriptLibrary* library)
    {
        return IsUndefinedObject(instance, library) || IsNull(instance, library);
    }

    BOOL JavascriptOperators::IsNull(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_Null;
    }

    BOOL JavascriptOperators::IsNull(Var instance, ScriptContext* scriptContext)
    {
        return JavascriptOperators::IsNull(instance, scriptContext->GetLibrary());
    }

    BOOL JavascriptOperators::IsNull(Var instance, JavascriptLibrary* library)
    {
        Assert(!VarIs<RecyclableObject>(instance) ? TRUE : ((RecyclableObject*)instance)->GetScriptContext()->GetLibrary() == library );
        return library->GetNull() == instance;
    }

    BOOL JavascriptOperators::IsNull(RecyclableObject* instance)
    {
        return instance->GetType()->GetTypeId() == TypeIds_Null;
    }

    BOOL JavascriptOperators::IsSpecialObjectType(TypeId typeId)
    {
        return typeId > TypeIds_LastTrueJavascriptObjectType;
    }

    BOOL JavascriptOperators::IsUndefinedObject(Var instance)
    {
        return JavascriptOperators::GetTypeId(instance) == TypeIds_Undefined;
    }

    BOOL JavascriptOperators::IsUndefinedObject(RecyclableObject* instance)
    {
        return instance->GetType()->GetTypeId() == TypeIds_Undefined;
    }

    BOOL JavascriptOperators::IsUndefinedObject(Var instance, RecyclableObject *libraryUndefined)
    {
        Assert(JavascriptOperators::IsUndefinedObject(libraryUndefined));
        AssertMsg((instance == libraryUndefined)
          == JavascriptOperators::IsUndefinedObject(instance), "Wrong ScriptContext?");
        return instance == libraryUndefined;
    }

    BOOL JavascriptOperators::IsUndefinedObject(Var instance, ScriptContext *scriptContext)
    {
        return JavascriptOperators::IsUndefinedObject(instance, scriptContext->GetLibrary());
    }

    BOOL JavascriptOperators::IsUndefinedObject(Var instance, JavascriptLibrary* library)
    {
        Assert(!VarIs<RecyclableObject>(instance) ? TRUE : ((RecyclableObject*)instance)->GetScriptContext()->GetLibrary() == library );
        return JavascriptOperators::IsUndefinedObject(instance, library->GetUndefined());
    }

    BOOL JavascriptOperators::IsAnyNumberValue(Var instance)
    {
        TypeId typeId = GetTypeId(instance);
        return TypeIds_FirstNumberType <= typeId && typeId <= TypeIds_LastNumberType;
    }

    // GetIterator as described in ES6.0 (draft 22) Section 7.4.1
    RecyclableObject* JavascriptOperators::GetIterator(Var iterable, ScriptContext* scriptContext, bool optional)
    {
        RecyclableObject* iterableObj = JavascriptOperators::ToObject(iterable, scriptContext);
        return JavascriptOperators::GetIterator(iterableObj, scriptContext, optional);
    }

    RecyclableObject* JavascriptOperators::GetIteratorFunction(Var iterable, ScriptContext* scriptContext, bool optional)
    {
        RecyclableObject* iterableObj = JavascriptOperators::ToObject(iterable, scriptContext);
        return JavascriptOperators::GetIteratorFunction(iterableObj, scriptContext, optional);
    }

    RecyclableObject* JavascriptOperators::GetIteratorFunction(RecyclableObject* instance, ScriptContext * scriptContext, bool optional)
    {
        Var func = JavascriptOperators::GetPropertyNoCache(instance, PropertyIds::_symbolIterator, scriptContext);

        if (optional && JavascriptOperators::IsUndefinedOrNull(func))
        {
            return nullptr;
        }

        if (!JavascriptConversion::IsCallable(func))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction);
        }

        RecyclableObject* function = VarTo<RecyclableObject>(func);
        return function;
    }

    RecyclableObject* JavascriptOperators::GetIterator(RecyclableObject* instance, ScriptContext * scriptContext, bool optional)
    {
        RecyclableObject* function = GetIteratorFunction(instance, scriptContext, optional);

        if (function == nullptr)
        {
            Assert(optional);
            return nullptr;
        }

        Var iterator = scriptContext->GetThreadContext()->ExecuteImplicitCall(function, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(), function, CallInfo(Js::CallFlags_Value, 1), instance);
        });

        if (!JavascriptOperators::IsObject(iterator))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
        }

        return VarTo<RecyclableObject>(iterator);
    }

    void JavascriptOperators::IteratorClose(RecyclableObject* iterator, ScriptContext* scriptContext)
    {
        try
        {
            Var func = JavascriptOperators::GetProperty(iterator, PropertyIds::return_, scriptContext);

            if (JavascriptConversion::IsCallable(func))
            {
                RecyclableObject* callable = VarTo<RecyclableObject>(func);
                scriptContext->GetThreadContext()->ExecuteImplicitCall(callable, ImplicitCall_Accessor, [=]()->Var
                {
                    Js::Var args[] = { iterator };
                    Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args));
                    return JavascriptFunction::CallFunction<true>(callable, callable->GetEntryPoint(), Js::Arguments(callInfo, args));
                });
            }
        }
        catch (const JavascriptException& err)
        {
            err.GetAndClear();  // discard exception object
            // We have arrived in this function due to AbruptCompletion (which is an exception), so we don't need to
            // propagate the exception of calling return function
        }
    }

    RecyclableObject* JavascriptOperators::CacheIteratorNext(RecyclableObject* iterator, ScriptContext* scriptContext)
    {
        Var nextFunc = JavascriptOperators::GetPropertyNoCache(iterator, PropertyIds::next, scriptContext);

        ThreadContext *threadContext = scriptContext->GetThreadContext();
        if (!JavascriptConversion::IsCallable(nextFunc))
        {
            if (!threadContext->RecordImplicitException())
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }
        return VarTo<RecyclableObject>(nextFunc);
    }

    // IteratorNext as described in ES6.0 (draft 22) Section 7.4.2
    RecyclableObject* JavascriptOperators::IteratorNext(RecyclableObject* iterator, ScriptContext* scriptContext, RecyclableObject* nextFunc, Var value)
    {
        ThreadContext *threadContext = scriptContext->GetThreadContext();

        Var result = threadContext->ExecuteImplicitCall(nextFunc, ImplicitCall_Accessor, [=]() -> Var
            {
                Js::Var args[] = { iterator, value };
                Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args) + (value == nullptr ? -1 : 0));
                return JavascriptFunction::CallFunction<true>(nextFunc, nextFunc->GetEntryPoint(), Arguments(callInfo, args));
            });

        if (!JavascriptOperators::IsObject(result))
        {
            if (!threadContext->RecordImplicitException())
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
        }

        return VarTo<RecyclableObject>(result);
    }

    // IteratorComplete as described in ES6.0 (draft 22) Section 7.4.3
    bool JavascriptOperators::IteratorComplete(RecyclableObject* iterResult, ScriptContext* scriptContext)
    {
        Var done = JavascriptOperators::GetPropertyNoCache(iterResult, Js::PropertyIds::done, scriptContext);

        return JavascriptConversion::ToBool(done, scriptContext);
    }

    // IteratorValue as described in ES6.0 (draft 22) Section 7.4.4
    Var JavascriptOperators::IteratorValue(RecyclableObject* iterResult, ScriptContext* scriptContext)
    {
        return JavascriptOperators::GetPropertyNoCache(iterResult, Js::PropertyIds::value, scriptContext);
    }

    // IteratorStep as described in ES6.0 (draft 22) Section 7.4.5
    bool JavascriptOperators::IteratorStep(RecyclableObject* iterator, ScriptContext* scriptContext, RecyclableObject* nextFunc, RecyclableObject** result)
    {
        Assert(result);

        *result = JavascriptOperators::IteratorNext(iterator, scriptContext, nextFunc);
        return !JavascriptOperators::IteratorComplete(*result, scriptContext);
    }

    bool JavascriptOperators::IteratorStepAndValue(RecyclableObject* iterator, ScriptContext* scriptContext, RecyclableObject* nextFunc, Var* resultValue)
    {
        // CONSIDER: Fast-pathing for iterators that are built-ins?
        RecyclableObject* result = JavascriptOperators::IteratorNext(iterator, scriptContext, nextFunc);

        if (!JavascriptOperators::IteratorComplete(result, scriptContext))
        {
            *resultValue = JavascriptOperators::IteratorValue(result, scriptContext);
            return true;
        }

        return false;
    }

    RecyclableObject* JavascriptOperators::CreateFromConstructor(RecyclableObject* constructor, ScriptContext* scriptContext)
    {
        // Create a regular object and set the internal proto from the constructor
        return JavascriptOperators::OrdinaryCreateFromConstructor(constructor, scriptContext->GetLibrary()->CreateObject(), nullptr, scriptContext);
    }

    RecyclableObject* JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject* constructor, RecyclableObject* obj, DynamicObject* intrinsicProto, ScriptContext* scriptContext)
    {
        // There isn't a good way for us to add internal properties to objects in Chakra.
        // Thus, caller should take care to create obj with the correct internal properties.

        Var proto = JavascriptOperators::GetPropertyNoCache(constructor, Js::PropertyIds::prototype, scriptContext);

        // If constructor.prototype is an object, we should use that as the [[Prototype]] for our obj.
        // Else, we set the [[Prototype]] internal slot of obj to %intrinsicProto% - which should be the default.
        if (JavascriptOperators::IsObjectType(JavascriptOperators::GetTypeId(proto)) &&
            VarTo<DynamicObject>(proto) != intrinsicProto)
        {
            JavascriptObject::ChangePrototype(obj, VarTo<RecyclableObject>(proto), /*validate*/true, scriptContext);
        }

        return obj;
    }

    Var JavascriptOperators::GetProperty(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return JavascriptOperators::GetProperty(instance, instance, propertyId, requestContext, info);
    }

    BOOL JavascriptOperators::GetProperty(RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return JavascriptOperators::GetProperty(instance, instance, propertyId, value, requestContext, info);
    }

    Var JavascriptOperators::GetProperty(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        Var value;
        if (JavascriptOperators::GetProperty(instance, propertyObject, propertyId, &value, requestContext, info))
        {
            return value;
        }
        return requestContext->GetMissingPropertyResult();
    }

    Var JavascriptOperators::GetPropertyNoCache(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetPropertyNoCache(instance, instance, propertyId, requestContext);
    }

    Var JavascriptOperators::GetPropertyNoCache(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, ScriptContext* requestContext)
    {
        Var value;
        JavascriptOperators::GetProperty_InternalSimple(instance, propertyObject, propertyId, &value, requestContext);
        return value;
    }

    BOOL JavascriptOperators::GetPropertyNoCache(RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetPropertyNoCache(instance, instance, propertyId, value, requestContext);
    }

    BOOL JavascriptOperators::GetPropertyNoCache(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetProperty_InternalSimple(instance, propertyObject, propertyId, value, requestContext);
    }

    Var JavascriptOperators::GetRootProperty(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        Var value;
        if (JavascriptOperators::GetRootProperty(instance, propertyId, &value, requestContext, info))
        {
            return value;
        }
        return requestContext->GetMissingPropertyResult();
    }

    BOOL JavascriptOperators::GetPropertyReference(RecyclableObject *instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info)
    {
        return JavascriptOperators::GetPropertyReference(instance, instance, propertyId, value, requestContext, info);
    }

    Var JavascriptOperators::GetItem(RecyclableObject* instance, uint32 index, ScriptContext* requestContext)
    {
        Var value;
        if (GetItem(instance, index, &value, requestContext))
        {
            return value;
        }

        return requestContext->GetMissingItemResult();
    }

    Var JavascriptOperators::GetItem(RecyclableObject* instance, uint64 index, ScriptContext* requestContext)
    {
        Var value;
        if (GetItem(instance, index, &value, requestContext))
        {
            return value;
        }

        return requestContext->GetMissingItemResult();
    }

    BOOL JavascriptOperators::GetItem(RecyclableObject* instance, uint64 index, Var* value, ScriptContext* requestContext)
    {
        if (index < JavascriptArray::InvalidIndex)
        {
            // In case index fits in uint32, we can avoid the (slower) big-index path
            return GetItem(instance, static_cast<uint32>(index), value, requestContext);
        }
        PropertyRecord const * propertyRecord = nullptr;
        JavascriptOperators::GetPropertyIdForInt(index, requestContext, &propertyRecord);
        return JavascriptOperators::GetProperty(instance, propertyRecord->GetPropertyId(), value, requestContext);
    }

    BOOL JavascriptOperators::GetItem(RecyclableObject* instance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptOperators::GetItem(instance, instance, index, value, requestContext);
    }

    BOOL JavascriptOperators::GetItemReference(RecyclableObject* instance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return GetItemReference(instance, instance, index, value, requestContext);
    }

    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritableProperty(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        if (propertyId == Js::PropertyIds::__proto__)
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyId, false, false>(instance, propertyId, setterValue, flags, info, scriptContext);
        }
        else
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyId, true, false>(instance, propertyId, setterValue, flags, info, scriptContext);
        }
    }

    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritableRootProperty(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        if (propertyId == Js::PropertyIds::__proto__)
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyId, false, true>(instance, propertyId, setterValue, flags, info, scriptContext);
        }
        else
        {
            return CheckPrototypesForAccessorOrNonWritablePropertyCore<PropertyId, true, true>(instance, propertyId, setterValue, flags, info, scriptContext);
        }
    }

    BOOL JavascriptOperators::CheckPrototypesForAccessorOrNonWritableProperty(RecyclableObject* instance, JavascriptString* propertyNameString, Var* setterValue, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext)
    {
        Js::PropertyRecord const * localPropertyRecord;
        propertyNameString->GetPropertyRecord(&localPropertyRecord);
        PropertyId propertyId = localPropertyRecord->GetPropertyId();
        return CheckPrototypesForAccessorOrNonWritableProperty(instance, propertyId, setterValue, flags, info, scriptContext);
    }

    BOOL JavascriptOperators::SetProperty(Var instance, RecyclableObject* object, PropertyId propertyId, Var newValue, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags)
    {
        PropertyValueInfo info;
        return JavascriptOperators::SetProperty(instance, object, propertyId, newValue, &info, requestContext, propertyOperationFlags);
    }

    BOOL JavascriptOperators::TryConvertToUInt32(const char16* str, int length, uint32* intVal)
    {
        return NumberUtilities::TryConvertToUInt32(str, length, intVal);
    }

    template <typename TPropertyKey>
    DescriptorFlags JavascriptOperators::GetRootSetter(RecyclableObject* instance, TPropertyKey propertyKey, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        // This is provided only so that CheckPrototypesForAccessorOrNonWritablePropertyCore will compile.
        // It will never be called.
        Throw::FatalInternalError();
    }

    template <>
    inline DescriptorFlags JavascriptOperators::GetRootSetter(RecyclableObject* instance, PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(JavascriptOperators::GetTypeId(instance) == TypeIds_GlobalObject
            || JavascriptOperators::GetTypeId(instance) == TypeIds_ModuleRoot,
            "Root must be a global object!");

        RootObjectBase* rootObject = static_cast<RootObjectBase*>(instance);
        return rootObject->GetRootSetter(propertyId, setterValue, info, requestContext);
    }

    // Helper to fetch @@species from a constructor object
    Var JavascriptOperators::GetSpecies(RecyclableObject* constructor, ScriptContext* scriptContext)
    {
        Var species = nullptr;

        // Let S be Get(C, @@species)
        if (JavascriptOperators::GetProperty(constructor, PropertyIds::_symbolSpecies, &species, scriptContext)
            && !JavascriptOperators::IsUndefinedOrNull(species))
        {
            // If S is neither undefined nor null, let C be S
            return species;
        }

        return constructor;
    }
