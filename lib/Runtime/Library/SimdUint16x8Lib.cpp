//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    Var SIMDUint16x8Lib::EntryUint16x8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        const uint LANES = 8;
        uint16 values[LANES];

        for (uint i = 0; i < LANES; i++)
        {
            values[i] = JavascriptConversion::ToUInt16(args.Info.Count >= (i + 2) ? args[i + 1] : undefinedVar, scriptContext);
        }

        SIMDValue lanes = SIMDUint16x8Operation::OpUint16x8(values);

        return JavascriptSIMDUint16x8::New(&lanes, scriptContext);
    }

    Var SIMDUint16x8Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        if (args.Info.Count >= 2 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("Uint16x8"));
    }

    Var SIMDUint16x8Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        uint16 value = JavascriptConversion::ToUInt16(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDInt16x8Operation::OpSplat(static_cast<int16>(value));;

        return JavascriptSIMDUint16x8::New(&lanes, scriptContext);
    }

   Var SIMDUint16x8Lib::EntryFromFloat32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDFloat32x4::Is(args[1]))
        {
            JavascriptSIMDFloat32x4 *instance = JavascriptSIMDFloat32x4::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDFloat32x4, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromFloat32x4Bits"));
    }

    Var SIMDUint16x8Lib::EntryFromInt32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt32x4::Is(args[1]))
        {
            JavascriptSIMDInt32x4 *instance = JavascriptSIMDInt32x4::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt32x4, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromInt32x4Bits"));
    }


    Var SIMDUint16x8Lib::EntryFromInt16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            JavascriptSIMDInt16x8 *instance = JavascriptSIMDInt16x8::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt16x8, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromInt16x8Bits"));
    }

    Var SIMDUint16x8Lib::EntryFromInt8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt8x16::Is(args[1]))
        {
            JavascriptSIMDInt8x16 *instance = JavascriptSIMDInt8x16::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt8x16, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromInt8x16Bits"));
    }

    Var SIMDUint16x8Lib::EntryFromUint32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            JavascriptSIMDUint32x4 *instance = JavascriptSIMDUint32x4::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint32x4, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromUint32x4Bits"));
    }

    Var SIMDUint16x8Lib::EntryFromUint8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            JavascriptSIMDUint8x16 *instance = JavascriptSIMDUint8x16::FromVar(args[1]);
            Assert(instance);
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint8x16, JavascriptSIMDUint16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("fromUint8x16Bits"));
    }

    Var SIMDUint16x8Lib::EntryMin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDUint16x8Operation::OpMin(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("min"));
    }

    Var SIMDUint16x8Lib::EntryMax(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDUint16x8Operation::OpMax(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("max"));
    }

    Var SIMDUint16x8Lib::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var tarray;
        Var index;
        if (args.Info.Count > 1)
        {
            tarray = args[1];
        }
        else
        {
            tarray = scriptContext->GetLibrary()->GetUndefined();
        }
        if (args.Info.Count > 2)
        {
            index = args[2];
        }
        else
        {
            index = scriptContext->GetLibrary()->GetUndefined();
        }

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint16x8>(tarray, index, 8 * INT16_SIZE, scriptContext);
    }

    Var SIMDUint16x8Lib::EntryStore(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint16x8::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint16x8>(args[1], args[2], args[3], 8 * INT16_SIZE, scriptContext);
            return JavascriptSIMDUint16x8::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.Uint16x8.store"));
    }

    Var SIMDUint16x8Lib::EntryNeg(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt16x8Operation::OpNeg(value);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("neg"));
    }

    Var SIMDUint16x8Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt32x4Operation::OpNot(value);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("not"));
    }

    Var SIMDUint16x8Lib::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpAdd(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("add"));
    }

    Var SIMDUint16x8Lib::EntrySub(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpSub(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("sub"));
    }

    Var SIMDUint16x8Lib::EntryMul(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpMul(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("mul"));
    }

    Var SIMDUint16x8Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("and"));
    }

    Var SIMDUint16x8Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("or"));
    }

    Var SIMDUint16x8Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("xor"));
    }

    Var SIMDUint16x8Lib::EntryLessThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpLessThan(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("lessThan"));
    }

    Var SIMDUint16x8Lib::EntryLessThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpLessThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("lessThanOrEqual"));
    }

    Var SIMDUint16x8Lib::EntryEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("equal"));
    }

    Var SIMDUint16x8Lib::EntryNotEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpNotEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("notEqual"));
    }

    Var SIMDUint16x8Lib::EntryGreaterThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpGreaterThan(aValue, bValue);


            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("greaterThan"));
    }

    Var SIMDUint16x8Lib::EntryGreaterThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpGreaterThanOrEqual(aValue, bValue);
            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("greaterThanOrEqual"));
    }

    Var SIMDUint16x8Lib::EntryShiftLeftByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int8 count = JavascriptConversion::ToInt8(countVar, scriptContext);

            result = SIMDInt16x8Operation::OpShiftLeftByScalar(aValue, count);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("shiftLeft"));
    }

    Var SIMDUint16x8Lib::EntryShiftRightByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int8 count = JavascriptConversion::ToInt8(countVar, scriptContext);

            result = SIMDUint16x8Operation::OpShiftRightByScalar(aValue, count);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("shiftRightByScalar"));
    }

    Var SIMDUint16x8Lib::EntrySwizzle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            // type check on lane indices
            if (args.Info.Count < 10)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[8];
            for (uint i = 0; i < 8; ++i)
            {
                lanes[i] = args[i + 2];
            }
            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint16x8>(args[1], args[1], lanes, 8, 8, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("swizzle"));
    }

    Var SIMDUint16x8Lib::EntryShuffle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            // type check on lane indices
            if (args.Info.Count < 11)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[8];
            for (uint i = 0; i < 8; ++i)
            {
                lanes[i] = args[i + 3];
            }
            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint16x8>(args[1], args[2], lanes, 8, 16, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("shuffle"));
    }

    //Lane Access
    Var SIMDUint16x8Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Uint16x8, so cannot be missing.
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            uint16 result = SIMDUtils::SIMD128ExtractLane<JavascriptSIMDUint16x8, 8, uint16>(args[1], laneVar, scriptContext);

            return JavascriptNumber::ToVarNoCheck(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDUint16x8Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Uint16x8, so cannot be missing.
        if (args.Info.Count >= 4 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            uint16 value = JavascriptConversion::ToInt16(argVal, scriptContext);

            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDUint16x8, 8, uint16>(args[1], laneVar, value, scriptContext);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("ReplaceLane"));
    }

    Var SIMDUint16x8Lib::EntryAddSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpAddSaturate(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("addSaturate"));
    }

    Var SIMDUint16x8Lib::EntrySubSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint16x8::Is(args[1]) && JavascriptSIMDUint16x8::Is(args[2]))
        {
            JavascriptSIMDUint16x8 *a = JavascriptSIMDUint16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *b = JavascriptSIMDUint16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint16x8Operation::OpSubSaturate(aValue, bValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("subSaturate"));
    }

    Var SIMDUint16x8Lib::EntrySelect(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDBool16x8::Is(args[1]) &&
            JavascriptSIMDUint16x8::Is(args[2]) && JavascriptSIMDUint16x8::Is(args[3]))
        {
            JavascriptSIMDBool16x8 *m = JavascriptSIMDBool16x8::FromVar(args[1]);
            JavascriptSIMDUint16x8 *t = JavascriptSIMDUint16x8::FromVar(args[2]);
            JavascriptSIMDUint16x8 *f = JavascriptSIMDUint16x8::FromVar(args[3]);
            Assert(m && t && f);

            SIMDValue result, maskValue, trueValue, falseValue;

            maskValue = m->GetValue();
            trueValue = t->GetValue();
            falseValue = f->GetValue();

            result = SIMDInt32x4Operation::OpSelect(maskValue, trueValue, falseValue);

            return JavascriptSIMDUint16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint16x8TypeMismatch, _u("select"));
    }
}
#endif
