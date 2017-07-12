//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
// SIMD_JS

namespace Js
{
    Var SIMDBool8x16Lib::EntryBool8x16(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        bool b[16]; // values

        uint argCount = args.Info.Count;
        for (uint i = 0; i < argCount - 1 && i < 16; i++)
        {
            b[i] = JavascriptConversion::ToBool(args[i + 1], scriptContext);
        }
        for (uint i = argCount - 1; i < 16; i++)
        {
            b[i] = JavascriptConversion::ToBool(undefinedVar, scriptContext);
        }

        SIMDValue lanes = SIMDBool8x16Operation::OpBool8x16(b);

        return JavascriptSIMDBool8x16::New(&lanes, scriptContext);

    }

    Var SIMDBool8x16Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("bool8x16"));
    }

    Var SIMDBool8x16Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        bool value = JavascriptConversion::ToBool(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDInt8x16Operation::OpSplat(value ? -1 : 0);

        return JavascriptSIMDBool8x16::New(&lanes, scriptContext);
    }

    //Lane Access
    Var SIMDBool8x16Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            bool result = (SIMDUtils::SIMD128ExtractLane<JavascriptSIMDBool8x16, 16, int8>(args[1], laneVar, scriptContext)) ? true : false;
            return JavascriptBoolean::ToVar(result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDBool8x16Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));


        if (args.Info.Count >= 4 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            bool value = JavascriptConversion::ToBool(argVal, scriptContext);
            int8 intValue = (value) ? -1 : 0;
            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDBool8x16, 16, int8>(args[1], laneVar, intValue, scriptContext);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("ReplaceLane"));
    }

    // UnaryOps
    Var SIMDBool8x16Lib::EntryAllTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAllTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("AllTrue"));
    }

    Var SIMDBool8x16Lib::EntryAnyTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAnyTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("AnyTrue"));
    }

    Var SIMDBool8x16Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool8x16::Is(args[1]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            Assert(a);

            SIMDValue result = SIMDInt32x4Operation::OpNot(a->GetValue());

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("not"));
    }

    Var SIMDBool8x16Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool8x16::Is(args[1]) && JavascriptSIMDBool8x16::Is(args[2]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            JavascriptSIMDBool8x16 *b = JavascriptSIMDBool8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("and"));
    }

    Var SIMDBool8x16Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool8x16::Is(args[1]) && JavascriptSIMDBool8x16::Is(args[2]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            JavascriptSIMDBool8x16 *b = JavascriptSIMDBool8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("or"));
    }

    Var SIMDBool8x16Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool8x16::Is(args[1]) && JavascriptSIMDBool8x16::Is(args[2]))
        {
            JavascriptSIMDBool8x16 *a = JavascriptSIMDBool8x16::FromVar(args[1]);
            JavascriptSIMDBool8x16 *b = JavascriptSIMDBool8x16::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDBool8x16::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("xor"));
    }

}
#endif
