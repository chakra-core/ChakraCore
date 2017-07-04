//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{
    Var SIMDInt16x8Lib::EntryInt16x8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        const uint LANES = 8;
        int16 values[LANES];

        for (uint i = 0; i < LANES; i++)
        {
            values[i] = JavascriptConversion::ToInt16(args.Info.Count >= (i + 2) ? args[i + 1] : undefinedVar, scriptContext);
        }

        SIMDValue lanes = SIMDInt16x8Operation::OpInt16x8(values);
        return JavascriptSIMDInt16x8::New(&lanes, scriptContext);
    }

    Var SIMDInt16x8Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        short value = JavascriptConversion::ToInt16(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDInt16x8Operation::OpSplat(value);

        return JavascriptSIMDInt16x8::New(&lanes, scriptContext);
    }

    Var SIMDInt16x8Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        if (args.Info.Count >= 2 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("Int16x8"));
    }

    Var SIMDInt16x8Lib::EntryFromFloat32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDFloat32x4, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromFloat32x4Bits"));
    }

    Var SIMDInt16x8Lib::EntryFromInt32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt32x4, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromInt32x4Bits"));
    }

    Var SIMDInt16x8Lib::EntryFromInt8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt8x16, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromInt8x16Bits"));
    }
    Var SIMDInt16x8Lib::EntryFromUint32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint32x4, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromUint32x4Bits"));
    }

    Var SIMDInt16x8Lib::EntryFromUint16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint16x8, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromUint16x8Bits"));
    }

    Var SIMDInt16x8Lib::EntryFromUint8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint8x16, JavascriptSIMDInt16x8>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("fromUint8x16Bits"));
    }

    Var SIMDInt16x8Lib::EntryNeg(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt16x8Operation::OpNeg(value);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("neg"));
    }

    Var SIMDInt16x8Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt16x8Operation::OpNot(value);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("not"));
    }

    Var SIMDInt16x8Lib::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpAdd(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("add"));
    }

    Var SIMDInt16x8Lib::EntrySub(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpSub(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("sub"));
    }

    Var SIMDInt16x8Lib::EntryMul(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpMul(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("mul"));
    }

    Var SIMDInt16x8Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("and"));
    }

    Var SIMDInt16x8Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpOr(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("or"));
    }

    Var SIMDInt16x8Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpXor(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("xor"));
    }

    Var SIMDInt16x8Lib::EntryLessThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpLessThan(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("lessThan"));
    }

    Var SIMDInt16x8Lib::EntryLessThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpLessThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("lessThanOrEqual"));
    }

    Var SIMDInt16x8Lib::EntryEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("equal"));
    }

    Var SIMDInt16x8Lib::EntryNotEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpNotEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("notEqual"));
    }

    Var SIMDInt16x8Lib::EntryGreaterThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpGreaterThan(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("greaterThan"));
    }

    Var SIMDInt16x8Lib::EntryGreaterThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt16x8Operation::OpGreaterThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("greaterThanOrEqual"));
    }

    Var SIMDInt16x8Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Int16x8, so cannot be missing.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            int16 result = SIMDUtils::SIMD128ExtractLane<JavascriptSIMDInt16x8, 8, int16>(args[1], laneVar, scriptContext);

            return JavascriptNumber::ToVarNoCheck(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDInt16x8Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type Int16x8, so cannot be missing.
        if (args.Info.Count >= 4 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            int16 value = JavascriptConversion::ToInt16(argVal, scriptContext);

            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDInt16x8, 8, int16>(args[1], laneVar, value, scriptContext);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("ReplaceLane"));
    }


    Var SIMDInt16x8Lib::EntryShiftLeftByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int32 count = JavascriptConversion::ToInt32(countVar, scriptContext);

            result = SIMDInt16x8Operation::OpShiftLeftByScalar(aValue, count);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("shiftLeftByScalar"));
    }

    Var SIMDInt16x8Lib::EntryShiftRightByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int32 count = JavascriptConversion::ToInt32(countVar, scriptContext);

            result = SIMDInt16x8Operation::OpShiftRightByScalar(aValue, count);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("shiftRightByScalar"));
    }

    Var SIMDInt16x8Lib::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDInt16x8>(tarray, index, 8 * INT16_SIZE, scriptContext);
    }

    Var SIMDInt16x8Lib::EntryStore(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDInt16x8::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDInt16x8>(args[1], args[2], args[3], 8 * INT16_SIZE, scriptContext);
            return JavascriptSIMDInt16x8::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.Int16x8.store"));
    }

    Var SIMDInt16x8Lib::EntrySwizzle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDInt16x8::Is(args[1]))
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
            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDInt16x8>(args[1], args[1], lanes, 8, 8, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("swizzle"));
    }

    Var SIMDInt16x8Lib::EntryShuffle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
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

            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDInt16x8>(args[1], args[2], lanes, 8, 16, scriptContext);

        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("shuffle"));
    }

    Var SIMDInt16x8Lib::EntryAddSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpAddSaturate(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("addSaturate"));
    }

    Var SIMDInt16x8Lib::EntrySubSaturate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpSubSaturate(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("subSaturate"));
    }

    Var SIMDInt16x8Lib::EntryMin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8*b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpMin(aValue, bValue);
            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("min"));
    }

    Var SIMDInt16x8Lib::EntryMax(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDInt16x8::Is(args[1]) && JavascriptSIMDInt16x8::Is(args[2]))
        {
            JavascriptSIMDInt16x8 *a = JavascriptSIMDInt16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *b = JavascriptSIMDInt16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt16x8Operation::OpMax(aValue, bValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("max"));
    }

    Var SIMDInt16x8Lib::EntrySelect(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDBool16x8::Is(args[1]) &&
            JavascriptSIMDInt16x8::Is(args[2]) && JavascriptSIMDInt16x8::Is(args[3]))
        {
            JavascriptSIMDBool16x8 *m = JavascriptSIMDBool16x8::FromVar(args[1]);
            JavascriptSIMDInt16x8 *t  = JavascriptSIMDInt16x8::FromVar(args[2]);
            JavascriptSIMDInt16x8 *f  = JavascriptSIMDInt16x8::FromVar(args[3]);
            Assert(m && t && f);

            SIMDValue result, maskValue, trueValue, falseValue;
            maskValue = m->GetValue();
            trueValue = t->GetValue();
            falseValue = f->GetValue();
            result = SIMDInt32x4Operation::OpSelect(maskValue, trueValue, falseValue);

            return JavascriptSIMDInt16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt16x8TypeMismatch, _u("select"));
    }
}
#endif
