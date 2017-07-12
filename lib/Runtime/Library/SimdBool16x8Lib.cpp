//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
// SIMD_JS

namespace Js
{
    Var SIMDBool16x8Lib::EntryBool16x8(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        bool b[8]; // values

        uint argCount = args.Info.Count;
        for (uint i = 0; i < argCount - 1 && i < 8; i++)
        {
            b[i] = JavascriptConversion::ToBool(args[i + 1], scriptContext);
        }
        for (uint i = argCount - 1; i < 8; i++)
        {
            b[i] = JavascriptConversion::ToBool(undefinedVar, scriptContext);
        }

        SIMDValue lanes = SIMDBool16x8Operation::OpBool16x8(b);

        return JavascriptSIMDBool16x8::New(&lanes, scriptContext);

    }

    Var SIMDBool16x8Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("bool16x8"));
    }

    Var SIMDBool16x8Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        bool value = JavascriptConversion::ToBool(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);


        SIMDValue lanes = SIMDInt16x8Operation::OpSplat(value ? -1 : 0);

        return JavascriptSIMDBool16x8::New(&lanes, scriptContext);
    }

    //Lane Access
    Var SIMDBool16x8Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            bool result = (SIMDUtils::SIMD128ExtractLane<JavascriptSIMDBool16x8, 8, int16>(args[1], laneVar, scriptContext)) ? true : false;
            return JavascriptBoolean::ToVar(result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool8x16TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDBool16x8Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));


        if (args.Info.Count >= 4 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            bool value = JavascriptConversion::ToBool(argVal, scriptContext);
            int16 intValue = (value) ? -1 : 0;
            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDBool16x8, 8, int16>(args[1], laneVar, intValue, scriptContext);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("ReplaceLane"));
    }

    // UnaryOps
    Var SIMDBool16x8Lib::EntryAllTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAllTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("AllTrue"));
    }

    Var SIMDBool16x8Lib::EntryAnyTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAnyTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("AnyTrue"));
    }

    // TODO: Dependent on Int16x8
    Var SIMDBool16x8Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool16x8::Is(args[1]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            Assert(a);

            // TODO: Change to Int16x8
            SIMDValue result = SIMDInt32x4Operation::OpNot(a->GetValue());

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("not"));
    }

    Var SIMDBool16x8Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool16x8::Is(args[1]) && JavascriptSIMDBool16x8::Is(args[2]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            JavascriptSIMDBool16x8 *b = JavascriptSIMDBool16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            // TODO: Change to Int16x8
            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("and"));
    }

    Var SIMDBool16x8Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool16x8::Is(args[1]) && JavascriptSIMDBool16x8::Is(args[2]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            JavascriptSIMDBool16x8 *b = JavascriptSIMDBool16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            // TODO: Change to Int16x8
            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("or"));
    }

    Var SIMDBool16x8Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool16x8::Is(args[1]) && JavascriptSIMDBool16x8::Is(args[2]))
        {
            JavascriptSIMDBool16x8 *a = JavascriptSIMDBool16x8::FromVar(args[1]);
            JavascriptSIMDBool16x8 *b = JavascriptSIMDBool16x8::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();
            // TODO: Change to Int16x8
            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDBool16x8::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool16x8TypeMismatch, _u("xor"));
    }
}
#endif
