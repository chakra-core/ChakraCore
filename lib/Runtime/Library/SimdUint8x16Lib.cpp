//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    Var SIMDUint8x16Lib::EntryUint8x16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        const uint LANES = 16;
        uint8 values[LANES];

        for (uint i = 0; i < LANES; i++)
        {
            values[i] = JavascriptConversion::ToUInt8(args.Info.Count >= (i + 2) ? args[i + 1] : undefinedVar, scriptContext);
        }

        SIMDValue lanes = SIMDUint8x16Operation::OpUint8x16(values);

        return JavascriptSIMDUint8x16::New(&lanes, scriptContext);
    }

    Var SIMDUint8x16Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        if (args.Info.Count >= 2 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("Uint8x16"));
    }

    Var SIMDUint8x16Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        uint8 value = JavascriptConversion::ToUInt8(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDInt8x16Operation::OpSplat(value);

        return JavascriptSIMDUint8x16::New(&lanes, scriptContext);
    }

   Var SIMDUint8x16Lib::EntryFromFloat32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDFloat32x4, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromFloat32x4Bits"));
    }

    Var SIMDUint8x16Lib::EntryFromInt32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return  SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt32x4, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromInt32x4Bits"));
    }

    Var SIMDUint8x16Lib::EntryFromInt16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt16x8, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromInt16x8Bits"));
    }

    Var SIMDUint8x16Lib::EntryFromInt8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return  SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt8x16, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromInt8x16Bits"));
    }

    Var SIMDUint8x16Lib::EntryFromUint32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return  SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint32x4, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromUint32x4Bits"));
    }

    Var SIMDUint8x16Lib::EntryFromUint16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint16x8::Is(args[1]))
        {
            JavascriptSIMDUint16x8 *instance = JavascriptSIMDUint16x8::FromVar(args[1]);
            Assert(instance);
            return  SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint16x8, JavascriptSIMDUint8x16>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("fromUint16x8Bits"));
    }

    Var SIMDUint8x16Lib::EntryMin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDUint8x16Operation::OpMin(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("min"));
    }

    Var SIMDUint8x16Lib::EntryMax(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDUint8x16Operation::OpMax(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("max"));
    }

    Var SIMDUint8x16Lib::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint8x16>(tarray, index, 16 * INT8_SIZE, scriptContext);
    }

    Var SIMDUint8x16Lib::EntryStore(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint8x16::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint8x16>(args[1], args[2], args[3], 16 * INT8_SIZE, scriptContext);
            return JavascriptSIMDUint8x16::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.Uint8x16.store"));
    }

    Var SIMDUint8x16Lib::EntryNeg(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt8x16Operation::OpNeg(value);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt8x16TypeMismatch, _u("neg"));
    }

    Var SIMDUint8x16Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt32x4Operation::OpNot(value);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("not"));
    }

    Var SIMDUint8x16Lib::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt8x16Operation::OpAdd(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("add"));
    }

    Var SIMDUint8x16Lib::EntrySub(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt8x16Operation::OpSub(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("sub"));
    }

    Var SIMDUint8x16Lib::EntryMul(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt8x16Operation::OpMul(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("mul"));
    }

    Var SIMDUint8x16Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("and"));
    }

    Var SIMDUint8x16Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("or"));
    }

    Var SIMDUint8x16Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("xor"));
    }

    Var SIMDUint8x16Lib::EntryLessThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpLessThan(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("lessThan"));
    }

    Var SIMDUint8x16Lib::EntryLessThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpLessThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("lessThanOrEqual"));
    }

    Var SIMDUint8x16Lib::EntryEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt8x16Operation::OpEqual(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("equal"));
    }

    Var SIMDUint8x16Lib::EntryNotEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt8x16Operation::OpNotEqual(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("notEqual"));
    }

    Var SIMDUint8x16Lib::EntryGreaterThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpGreaterThan(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("greaterThan"));
    }

    Var SIMDUint8x16Lib::EntryGreaterThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpGreaterThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("greaterThanOrEqual"));
    }

    Var SIMDUint8x16Lib::EntryShiftLeftByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int8 count = JavascriptConversion::ToInt8(countVar, scriptContext);

            result = SIMDInt8x16Operation::OpShiftLeftByScalar(aValue, count);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("shiftLeft"));
    }

    Var SIMDUint8x16Lib::EntryShiftRightByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int8 count = JavascriptConversion::ToInt8(countVar, scriptContext);

            result = SIMDUint8x16Operation::OpShiftRightByScalar(aValue, count);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("shiftRightByScalar"));
    }

    Var SIMDUint8x16Lib::EntrySwizzle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            // type check on lane indices
            if (args.Info.Count < 18)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[16];
            for (uint i = 0; i < 16; ++i)
            {
                lanes[i] = args[i + 2];
            }
            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint8x16>(args[1], args[1], lanes, 16, 16, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("swizzle"));
    }

    Var SIMDUint8x16Lib::EntryShuffle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            // type check on lane indices
            if (args.Info.Count < 19)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[16];
            for (uint i = 0; i < 16; ++i)
            {
                lanes[i] = args[i + 3];
            }
            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint8x16>(args[1], args[2], lanes, 16, 32, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("shuffle"));
    }

    //Lane Access
    Var SIMDUint8x16Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Uint8x16, so cannot be missing.
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            uint8 result = SIMDUtils::SIMD128ExtractLane<JavascriptSIMDUint8x16, 16, uint8>(args[1], laneVar, scriptContext);

            return JavascriptNumber::ToVarNoCheck(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDUint8x16Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Uint8x16, so cannot be missing.
        if (args.Info.Count >= 4 && JavascriptSIMDUint8x16::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            uint8 value = JavascriptConversion::ToInt8(argVal, scriptContext);

            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDUint8x16, 16, uint8>(args[1], laneVar, value, scriptContext);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("ReplaceLane"));
    }

    Var SIMDUint8x16Lib::EntryAddSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpAddSaturate(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("addSaturate"));
    }

    Var SIMDUint8x16Lib::EntrySubSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint8x16::Is(args[1]) && JavascriptSIMDUint8x16::Is(args[2]))
        {
            JavascriptSIMDUint8x16 *a = JavascriptSIMDUint8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *b = JavascriptSIMDUint8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint8x16Operation::OpSubSaturate(aValue, bValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("subSaturate"));
    }

    Var SIMDUint8x16Lib::EntrySelect(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDBool8x16::Is(args[1]) &&
            JavascriptSIMDUint8x16::Is(args[2]) && JavascriptSIMDUint8x16::Is(args[3]))
        {
            JavascriptSIMDBool8x16 *m = JavascriptSIMDBool8x16::FromVar(args[1]);
            JavascriptSIMDUint8x16 *t = JavascriptSIMDUint8x16::FromVar(args[2]);
            JavascriptSIMDUint8x16 *f = JavascriptSIMDUint8x16::FromVar(args[3]);
            Assert(m && t && f);

            SIMDValue result, maskValue, trueValue, falseValue;

            maskValue = m->GetValue();
            trueValue = t->GetValue();
            falseValue = f->GetValue();

            result = SIMDInt32x4Operation::OpSelect(maskValue, trueValue, falseValue);

            return JavascriptSIMDUint8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint8x16TypeMismatch, _u("select"));
    }
}
#endif
