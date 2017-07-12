//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#ifdef ENABLE_SIMDJS
// SIMD_JS

namespace Js
{
    Var SIMDBool32x4Lib::EntryBool32x4(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        bool bSIMDX = JavascriptConversion::ToBool(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);
        bool bSIMDY = JavascriptConversion::ToBool(args.Info.Count >= 3 ? args[2] : undefinedVar, scriptContext);
        bool bSIMDZ = JavascriptConversion::ToBool(args.Info.Count >= 4 ? args[3] : undefinedVar, scriptContext);
        bool bSIMDW = JavascriptConversion::ToBool(args.Info.Count >= 5 ? args[4] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDBool32x4Operation::OpBool32x4(bSIMDX, bSIMDY, bSIMDZ, bSIMDW);

        return JavascriptSIMDBool32x4::New(&lanes, scriptContext);

    }

    Var SIMDBool32x4Lib::EntryCheck(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            return args[1];
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("bool32x4"));
    }

    Var SIMDBool32x4Lib::EntrySplat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        bool value = JavascriptConversion::ToBool(args.Info.Count >= 2 ? args[1] : undefinedVar, scriptContext);

        SIMDValue lanes = SIMDInt32x4Operation::OpSplat(value ? -1 : 0);

        return JavascriptSIMDBool32x4::New(&lanes, scriptContext);
    }

    //Lane Access
    Var SIMDBool32x4Lib::EntryExtractLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 3 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 3 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            bool result = (SIMDUtils::SIMD128ExtractLane<JavascriptSIMDBool32x4, 4, int32>(args[1], laneVar, scriptContext)) ? true : false;
            return JavascriptBoolean::ToVar(result, scriptContext);
        }
        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("ExtractLane"));
    }

    Var SIMDBool32x4Lib::EntryReplaceLane(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 4 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            // if value arg is missing, then it is undefined.
            Var laneVar = args.Info.Count >= 4 ? args[2] : scriptContext->GetLibrary()->GetUndefined();
            Var argVal = args.Info.Count >= 4 ? args[3] : scriptContext->GetLibrary()->GetUndefined();
            bool value = JavascriptConversion::ToBool(argVal, scriptContext);
            int32 intValue = (value) ? -1 : 0;
            SIMDValue result = SIMDUtils::SIMD128ReplaceLane<JavascriptSIMDBool32x4, 4, int32>(args[1], laneVar, intValue, scriptContext);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("ReplaceLane"));
    }

    // UnaryOps
    Var SIMDBool32x4Lib::EntryAllTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAllTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("AllTrue"));
    }

    Var SIMDBool32x4Lib::EntryAnyTrue(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            Assert(a);

            bool result = SIMDBool32x4Operation::OpAnyTrue(a->GetValue());

            return JavascriptBoolean::ToVar(result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("AnyTrue"));
    }

    Var SIMDBool32x4Lib::EntryNot(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count >= 2 && JavascriptSIMDBool32x4::Is(args[1]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            Assert(a);

            SIMDValue result = SIMDInt32x4Operation::OpNot(a->GetValue());

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("not"));
    }

    Var SIMDBool32x4Lib::EntryAnd(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool32x4::Is(args[1]) && JavascriptSIMDBool32x4::Is(args[2]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            JavascriptSIMDBool32x4 *b = JavascriptSIMDBool32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpAnd(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("and"));
    }

    Var SIMDBool32x4Lib::EntryOr(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool32x4::Is(args[1]) && JavascriptSIMDBool32x4::Is(args[2]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            JavascriptSIMDBool32x4 *b = JavascriptSIMDBool32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpOr(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("or"));
    }

    Var SIMDBool32x4Lib::EntryXor(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        Assert(!(callInfo.Flags & CallFlags_New));

        // If any of the args are missing, then it is Undefined type which causes TypeError exception.
        // strict type on both operands
        if (args.Info.Count >= 3 && JavascriptSIMDBool32x4::Is(args[1]) && JavascriptSIMDBool32x4::Is(args[2]))
        {
            JavascriptSIMDBool32x4 *a = JavascriptSIMDBool32x4::FromVar(args[1]);
            JavascriptSIMDBool32x4 *b = JavascriptSIMDBool32x4::FromVar(args[2]);
            Assert(a && b);

            SIMDValue result, aValue, bValue;

            aValue = a->GetValue();
            bValue = b->GetValue();

            result = SIMDInt32x4Operation::OpXor(aValue, bValue);

            return JavascriptSIMDBool32x4::New(&result, scriptContext);
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_SimdBool32x4TypeMismatch, _u("xor"));
    }

}
#endif
