//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
namespace Js
{

    // Q: Are we allowed to call this as a constructor ?
    Var SIMDUint32x4Lib::EntryUint32x4(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        unsigned int intSIMDX = JavascriptConversion::ToUInt32(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);
        unsigned int intSIMDY = JavascriptConversion::ToUInt32(args.Info.Count >= 3 ? args[2] : undefinedVar, scriptContext);
        unsigned int intSIMDZ = JavascriptConversion::ToUInt32(args.Info.Count >= 4 ? args[3] : undefinedVar, scriptContext);
        unsigned int intSIMDW = JavascriptConversion::ToUInt32(args.Info.Count >= 5 ? args[4] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDUint32x4Operation::OpUint32x4(intSIMDX, intSIMDY, intSIMDZ, intSIMDW);

        return JavascriptSIMDUint32x4::New(&lanes, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("UInt32x4"));
    }

    Var SIMDUint32x4Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        int value = JavascriptConversion::ToUInt32(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDUint32x4Operation::OpSplat(value);

        return JavascriptSIMDUint32x4::New(&lanes, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryFromFloat32x4(RecyclableObject* function, CallInfo callInfo, ...)
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

            SIMDValue input = instance->GetValue();
            bool throws = false;
            SIMDValue result = SIMDUint32x4Operation::OpFromFloat32x4(input, throws);

            // out of range
            if (throws)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("SIMD.UInt32x4.FromFloat32x4"));
            }
            return JavascriptSIMDUint32x4::New(&result, scriptContext);

        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("fromFloat32x4"));
    }

    Var SIMDUint32x4Lib::EntryFromFloat32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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
            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDFloat32x4, JavascriptSIMDUint32x4>(*instance, *scriptContext);

        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdFloat32x4TypeMismatch, _u("fromFloat32x4Bits"));
    }

    Var SIMDUint32x4Lib::EntryFromInt32x4Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt32x4, JavascriptSIMDUint32x4>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("fromInt32x4Bits"));
    }

    Var SIMDUint32x4Lib::EntryFromInt16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt16x8, JavascriptSIMDUint32x4>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("fromInt16x8Bits"));
    }

    Var SIMDUint32x4Lib::EntryFromInt8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDInt8x16, JavascriptSIMDUint32x4>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("fromInt8x16Bits"));
    }

    Var SIMDUint32x4Lib::EntryFromUint16x8Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint16x8, JavascriptSIMDUint32x4>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("fromUint16x8Bits"));
    }

    Var SIMDUint32x4Lib::EntryFromUint8x16Bits(RecyclableObject* function, CallInfo callInfo, ...)
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

            return SIMDUtils::SIMDConvertTypeFromBits<JavascriptSIMDUint8x16, JavascriptSIMDUint32x4>(*instance, *scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInt32x4TypeMismatch, _u("fromUint8x16Bits"));
    }

    //Lane Access
    Var SIMDUint32x4Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type UInt32x4, so cannot be missing.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            uint result = SIMDUtils::SIMD128ExtractLane<JavascriptSIMDUint32x4, 4, uint>(args[1], laneVar, scriptContext);

            return JavascriptNumber::ToVarNoCheck((double)result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDUint32x4Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // first arg has to be of type UInt32x4, so cannot be missing.
        if (args.Info.Count >= 4 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            uint value = JavascriptConversion::ToUInt32(argVal, scriptContext);

            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDUint32x4, 4, uint32>(args[1], laneVar, value, scriptContext);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("ReplaceLane"));
    }

    Var SIMDUint32x4Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt32x4Operation::OpNot(value);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("not"));
    }

    Var SIMDUint32x4Lib::EntryNeg(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            Assert(a);

            SIMDValue value, result;

            value = a->GetValue();
            result = SIMDInt32x4Operation::OpNeg(value);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("neg"));
    }

    Var SIMDUint32x4Lib::EntryAdd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpAdd(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("add"));
    }

    Var SIMDUint32x4Lib::EntrySub(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpSub(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("sub"));
    }

    Var SIMDUint32x4Lib::EntryMul(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpMul(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("mul"));
    }

    Var SIMDUint32x4Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("and"));
    }

    Var SIMDUint32x4Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("or"));
    }

    Var SIMDUint32x4Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("xor"));
    }

    Var SIMDUint32x4Lib::EntryMin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint32x4Operation::OpMin(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("lessThan"));
    }

    Var SIMDUint32x4Lib::EntryMax(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint32x4Operation::OpMax(aValue, bValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("lessThan"));
    }

    Var SIMDUint32x4Lib::EntryLessThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint32x4Operation::OpLessThan(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("lessThan"));
    }

    Var SIMDUint32x4Lib::EntryLessThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint32x4Operation::OpLessThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("lessThanOrEqual"));
    }

    Var SIMDUint32x4Lib::EntryEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpEqual(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("equal"));
    }

    Var SIMDUint32x4Lib::EntryNotEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpEqual(aValue, bValue);
            result = SIMDInt32x4Operation::OpNot(result);
            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("notEqual"));
    }

    Var SIMDUint32x4Lib::EntryGreaterThan(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDUint32x4Operation::OpGreaterThan(aValue, bValue);
            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("greaterThan"));
    }

    Var SIMDUint32x4Lib::EntryGreaterThanOrEqual(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *b = JavascriptSIMDUint32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            result = SIMDUint32x4Operation::OpGreaterThanOrEqual(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("greaterThanOrEqual"));
    }

    Var SIMDUint32x4Lib::EntrySwizzle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 2 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            // type check on lane indices
            if (args.Info.Count < 6)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[4];
            lanes[0] = args[2];
            lanes[1] = args[3];
            lanes[2] = args[4];
            lanes[3] = args[5];

            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint32x4>(args[1], args[1], lanes, 4, 4, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("swizzle"));
    }

    Var SIMDUint32x4Lib::EntryShuffle(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]) && JavascriptSIMDUint32x4::Is(args[2]))
        {
            // type check on lane indices
            if (args.Info.Count < 7)
            {
                // missing lane args
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedNumber, _u("Lane index"));
            }

            Var lanes[4];
            lanes[0] = args[3];
            lanes[1] = args[4];
            lanes[2] = args[5];
            lanes[3] = args[6];

            return SIMDUtils::SIMD128SlowShuffle<JavascriptSIMDUint32x4>(args[1], args[2], lanes, 4, 8, scriptContext);

        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("shuffle"));
    }

    Var SIMDUint32x4Lib::EntryShiftLeftByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int32 count = JavascriptConversion::ToInt32(countVar, scriptContext);

            result = SIMDInt32x4Operation::OpShiftLeftByScalar(aValue, count);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("shiftLeftByScalar"));
    }

    Var SIMDUint32x4Lib::EntryShiftRightByScalar(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDUint32x4::Is(args[1]))
        {
            JavascriptSIMDUint32x4 *a = JavascriptSIMDUint32x4::FromVar(args[1]);
            Assert(a);

            SIMDValue result, aValue;

            aValue = a->GetValue();
            Var countVar = args[2]; // {int} bits Bit count
            int32 count = JavascriptConversion::ToInt32(countVar, scriptContext);

            result = SIMDUint32x4Operation::OpShiftRightByScalar(aValue, count);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("shiftRightByScalar"));
    }

    Var SIMDUint32x4Lib::EntrySelect(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDBool32x4::Is(args[1]) &&
            JavascriptSIMDUint32x4::Is(args[2]) && JavascriptSIMDUint32x4::Is(args[3]))
        {
            JavascriptSIMDBool32x4 *m = JavascriptSIMDBool32x4::FromVar(args[1]);
            JavascriptSIMDUint32x4 *t = JavascriptSIMDUint32x4::FromVar(args[2]);
            JavascriptSIMDUint32x4 *f = JavascriptSIMDUint32x4::FromVar(args[3]);
            Assert(m && t && f);

            SIMDValue result, maskValue, trueValue, falseValue;

            maskValue = m->GetValue();
            trueValue = t->GetValue();
            falseValue = f->GetValue();

            result = SIMDInt32x4Operation::OpSelect(maskValue, trueValue, falseValue);

            return JavascriptSIMDUint32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdUint32x4TypeMismatch, _u("select"));
    }

    Var SIMDUint32x4Lib::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint32x4>(tarray, index, 4 * INT32_SIZE, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryLoad1(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint32x4>(tarray, index, 1 * INT32_SIZE, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryLoad2(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint32x4>(tarray, index, 2 * INT32_SIZE, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryLoad3(RecyclableObject* function, CallInfo callInfo, ...)
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

        return SIMDUtils::SIMD128TypedArrayLoad<JavascriptSIMDUint32x4>(tarray, index, 3 * INT32_SIZE, scriptContext);
    }

    Var SIMDUint32x4Lib::EntryStore(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint32x4::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint32x4>(args[1], args[2], args[3], 4 * INT32_SIZE, scriptContext);
            return JavascriptSIMDUint32x4::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.UInt32x4.store"));
    }

    Var SIMDUint32x4Lib::EntryStore1(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint32x4::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint32x4>(args[1], args[2], args[3], 1 * INT32_SIZE, scriptContext);
            return JavascriptSIMDUint32x4::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.UInt32x4.store"));
    }

    Var SIMDUint32x4Lib::EntryStore2(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint32x4::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint32x4>(args[1], args[2], args[3], 2 * INT32_SIZE, scriptContext);
            return JavascriptSIMDUint32x4::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.UInt32x4.store"));
    }

    Var SIMDUint32x4Lib::EntryStore3(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDUint32x4::Is(args[3]))
        {
            SIMDUtils::SIMD128TypedArrayStore<JavascriptSIMDUint32x4>(args[1], args[2], args[3], 3 * INT32_SIZE, scriptContext);
            return JavascriptSIMDUint32x4::FromVar(args[3]);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdInvalidArgType, _u("SIMD.UInt32x4.store"));
    }

}
#endif
