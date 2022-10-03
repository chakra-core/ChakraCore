//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2022 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Library/EngineInterfaceObject.h"
#include "Library/IntlEngineInterfaceExtensionObject.h"

namespace Js
{
    JavascriptDate::JavascriptDate(double value, DynamicType * type)
        : DynamicObject(type), m_date(value)
    {
        Assert(type->GetTypeId() == TypeIds_Date);
    }

    JavascriptDate::JavascriptDate(DynamicType * type)
        : DynamicObject(type), m_date(0)
    {
        Assert(type->GetTypeId() == TypeIds_Date);
    }

    Var JavascriptDate::GetDateData(JavascriptDate* date, DateImplementation::DateData dd, ScriptContext* scriptContext)
    {
        return JavascriptNumber::ToVarIntCheck(date->m_date.GetDateData(dd, false, scriptContext), scriptContext);
    }

    Var JavascriptDate::GetUTCDateData(JavascriptDate* date, DateImplementation::DateData dd, ScriptContext* scriptContext)
    {
        return JavascriptNumber::ToVarIntCheck(date->m_date.GetDateData(dd, true, scriptContext), scriptContext);
    }

    Var JavascriptDate::SetDateData(JavascriptDate* date, Arguments args, DateImplementation::DateData dd, ScriptContext* scriptContext)
    {
        return JavascriptNumber::ToVarNoCheck(date->m_date.SetDateData(args, dd, false, scriptContext), scriptContext);
    }

    Var JavascriptDate::SetUTCDateData(JavascriptDate* date, Arguments args, DateImplementation::DateData dd, ScriptContext* scriptContext)
    {
        return JavascriptNumber::ToVarNoCheck(date->m_date.SetDateData(args, dd, true, scriptContext), scriptContext);
    }

    Var JavascriptDate::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        //
        // Determine if called as a constructor or a function.
        //

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        if (!(callInfo.Flags & CallFlags_New))
        {
            //
            // Called as a function.
            //

            //
            // Be sure the latest time zone info is loaded
            //

            // ES5 15.9.2.1: Date() should returns a string exactly the same as (new Date().toString()).
            JavascriptDate* pDate = NewInstanceAsConstructor(args, scriptContext, /* forceCurrentDate */ true);
            JavascriptString* res = JavascriptDate::ToString(pDate, scriptContext);

#if ENABLE_TTD
            if(scriptContext->ShouldPerformReplayAction())
            {
                scriptContext->GetThreadContext()->TTDLog->ReplayDateStringEvent(scriptContext, &res);
            }
            else if(scriptContext->ShouldPerformRecordAction())
            {
                scriptContext->GetThreadContext()->TTDLog->RecordDateStringEvent(res);
            }
            else
            {
                ;
            }
#endif
            return res;
        }
        else
        {
            //
            // Called as a constructor.
            //
            RecyclableObject* pNew = NewInstanceAsConstructor(args, scriptContext);
            return isCtorSuperCall ?
                JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), pNew, nullptr, scriptContext) :
                pNew;
        }
    }

    JavascriptDate* JavascriptDate::NewInstanceAsConstructor(Js::Arguments args, ScriptContext* scriptContext, bool forceCurrentDate)
    {
        Assert(scriptContext);

        double timeValue = 0;
        JavascriptDate* pDate;

        pDate = scriptContext->GetLibrary()->CreateDate();


        //
        // [15.9.3.3]
        // No arguments passed. Return the current time
        //
        if (forceCurrentDate || args.Info.Count == 1)
        {
            double resTime = DateImplementation::NowFromHiResTimer(scriptContext);

#if ENABLE_TTD
            if(scriptContext->ShouldPerformReplayAction())
            {
                scriptContext->GetThreadContext()->TTDLog->ReplayDateTimeEvent(&resTime);
            }
            else if(scriptContext->ShouldPerformRecordAction())
            {
                scriptContext->GetThreadContext()->TTDLog->RecordDateTimeEvent(resTime);
            }
            else
            {
                ;
            }
#endif

            pDate->m_date.SetTvUtc(resTime);
            return pDate;
        }

        //
        // [15.9.3.2]
        // One argument given
        // If string parse it and use that timeValue
        // Else convert to Number and use that as timeValue
        //
        if (args.Info.Count == 2)
        {
            if (VarIs<JavascriptDate>(args[1]))
            {
                JavascriptDate* dateObject = VarTo<JavascriptDate>(args[1]);
                timeValue = ((dateObject)->m_date).m_tvUtc;
            }
            else
            {
                Var value = JavascriptConversion::ToPrimitive<Js::JavascriptHint::None>(args[1], scriptContext);
                if (VarIs<JavascriptString>(value))
                {
                    timeValue = ParseHelper(scriptContext, VarTo<JavascriptString>(value));
                }
                else
                {
                    timeValue = JavascriptConversion::ToNumber(value, scriptContext);
                }
            }
            
            timeValue = TimeClip(JavascriptNumber::New(timeValue, scriptContext));

            pDate->m_date.SetTvUtc(timeValue);
            return pDate;
        }

        //
        // [15.9.3.1]
        // Date called with two to seven arguments
        //

        const int parameterCount = 7;
        double values[parameterCount];

        for (uint i=1; i < args.Info.Count && i < parameterCount+1; i++)
        {
            double curr = JavascriptConversion::ToNumber(args[i], scriptContext);
            values[i-1] = curr;
            if (JavascriptNumber::IsNan(curr) || !NumberUtilities::IsFinite(curr))
            {
                pDate->m_date.SetTvUtc(curr);
                return pDate;
            }
        }

        for (uint i=0; i < parameterCount; i++)
        {
            if ( i >= args.Info.Count-1 )
            {
                values[i] = ( i == 2 );
                continue;
            }
            // MakeTime (ES5 15.9.1.11) && MakeDay (ES5 15.9.1.12) always
            // call ToInteger (which is same as JavascriptConversion::ToInteger) on arguments.
            // All are finite (not Inf or Nan) as we check them explicitly in the previous loop.
            // +-0 & +0 are same in this context.
#pragma prefast(suppress:6001, "value index i < args.Info.Count - 1 are initialized")
            values[i] = JavascriptConversion::ToInteger(values[i]);
        }

        // adjust the year
        if (values[0] < 100 && values[0] >= 0)
            values[0] += 1900;

        // Get the local time value.
        timeValue = DateImplementation::TvFromDate(values[0], values[1], values[2] - 1,
            values[3] * 3600000 + values[4] * 60000 + values[5] * 1000 + values[6]);

        // Set the time.
        pDate->m_date.SetTvLcl(timeValue, scriptContext);

        return pDate;
    }

    // Implementation of ECMA 262 #sec-timeclip
    double JavascriptDate::TimeClip(Var x)
    {
        double time = 0.0;
        if (TaggedInt::Is(x))
        {
            time = TaggedInt::ToDouble(x);
        }
        else
        {
            AssertOrFailFast(JavascriptNumber::Is(x));
            time = JavascriptNumber::GetValue(x);

            // Only perform steps 1, 3, and 4 if the input was not a TaggedInt, since TaggedInts cant be infinite or -0
            if (!NumberUtilities::IsFinite(time))
            {
                return NumberConstants::NaN;
            }

            // This performs both steps 3 and 4
            time = JavascriptConversion::ToInteger(time);
        }

        // Step 2: If abs(time) > 8.64e15, return NaN.
        if (Math::Abs(time) > 8.64e15)
        {
            return NumberConstants::NaN;
        }

        return time;
    }

    // Date.prototype[@@toPrimitive] as described in ES6 spec (Draft May 22, 2014) 20.3.4.45
    Var JavascriptDate::EntrySymbolToPrimitive(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // One argument given will be hint
        //The allowed values for hint are "default", "number", and "string"
        if (args.Info.Count == 2)
        {
            if (!JavascriptOperators::IsObjectType(JavascriptOperators::GetTypeId(args[0])))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Date[Symbol.toPrimitive]"));
            }

            if (VarIs<JavascriptString>(args[1]))
            {
                JavascriptString* StringObject = VarTo<JavascriptString>(args[1]);
                const char16 * str = StringObject->GetString();

                if (wcscmp(str, _u("default")) == 0 || wcscmp(str, _u("string")) == 0)
                {
                    // Date objects, are unique among built-in ECMAScript object in that they treat "default" as being equivalent to "string"
                    // If hint is the string value "string" or the string value "default", then
                    // Let tryFirst be "string".
                    return JavascriptConversion::OrdinaryToPrimitive<JavascriptHint::HintString>(UnsafeVarTo<RecyclableObject>(args[0]), scriptContext);
                }
                // Else if hint is the string value "number", then
                // Let tryFirst be "number".
                else if(wcscmp(str, _u("number")) == 0)
                {
                    return JavascriptConversion::OrdinaryToPrimitive<JavascriptHint::HintNumber>(UnsafeVarTo<RecyclableObject>(args[0]), scriptContext);
                }
                //anything else should throw a type error
            }
        }

        JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidHint, _u("Date[Symbol.toPrimitive]"));
    }

    Var JavascriptDate::EntryGetDate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetDate, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getDate"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetDate(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetDay(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetDay, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getDay"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetDay(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetFullYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetFullYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getFullYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetFullYear(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetYear(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetHours(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetHours, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getHours"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetHours(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetMilliseconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetMilliseconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getMilliseconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetDateMilliSeconds(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetMinutes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetMinutes, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getMinutes"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetMinutes(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetMonth(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetMonth, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getMonth"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetMonth(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetSeconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetSeconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getSeconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        if (!date->m_date.IsNaN())
        {
            return date->m_date.GetSeconds(scriptContext);
        }
        return scriptContext->GetLibrary()->GetNaN();
    }

    Var JavascriptDate::EntryGetTime(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetTime, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getTime"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptNumber::ToVarNoCheck(date->GetTime(), scriptContext);
    }

    Var JavascriptDate::EntryGetTimezoneOffset(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetTimezoneOffset, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getTimezoneOffset"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetDateData(date, DateImplementation::DateData::TimezoneOffset, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCDate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCDate, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCDate"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Date, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCDay(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCDay, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCDay"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Day, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCFullYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCFullYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCFullYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::FullYear, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCHours(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCHours, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCHours"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Hours, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCMilliseconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCMilliseconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCMilliseconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Milliseconds, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCMinutes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCMinutes, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCMinutes"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Minutes, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCMonth(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCMonth, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCMonth"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Month, scriptContext);
    }

    Var JavascriptDate::EntryGetUTCSeconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryGetUTCSeconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.getUTCSeconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::GetUTCDateData(date, DateImplementation::DateData::Seconds, scriptContext);
    }

    Var JavascriptDate::EntryParse(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        double dblRetVal = JavascriptNumber::NaN;

        if (args.Info.Count > 1)
        {
            // We convert to primitive value based on hint == String, which JavascriptConversion::ToString does.
            JavascriptString *pParseString = JavascriptConversion::ToString(args[1], scriptContext);
            dblRetVal = ParseHelper(scriptContext, pParseString);
        }

        return JavascriptNumber::ToVarNoCheck(dblRetVal,scriptContext);
    }

    Var JavascriptDate::EntryNow(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        double dblRetVal = DateImplementation::NowInMilliSeconds(scriptContext);

#if ENABLE_TTD
        if(scriptContext->ShouldPerformReplayAction())
        {
            scriptContext->GetThreadContext()->TTDLog->ReplayDateTimeEvent(&dblRetVal);
        }
        else if(scriptContext->ShouldPerformRecordAction())
        {
            scriptContext->GetThreadContext()->TTDLog->RecordDateTimeEvent(dblRetVal);
        }
        else
        {
            ;
        }
#endif

        return JavascriptNumber::ToVarNoCheck(dblRetVal,scriptContext);
    }

    Var JavascriptDate::EntryUTC(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        Assert(!(callInfo.Flags & CallFlags_New));

        double dblRetVal = DateImplementation::DateFncUTC(scriptContext, args);
        return JavascriptNumber::ToVarNoCheck(dblRetVal, scriptContext);
    }

    double JavascriptDate::ParseHelper(ScriptContext *scriptContext, JavascriptString *str)
    {
        return DateImplementation::UtcTimeFromStr(scriptContext, str);
    }

    Var JavascriptDate::EntrySetDate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetDate, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setDate"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Date, scriptContext);
    }

    Var JavascriptDate::EntrySetFullYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetFullYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setFullYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::FullYear, scriptContext);
    }

    Var JavascriptDate::EntrySetYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Year, scriptContext);
    }

    Var JavascriptDate::EntrySetHours(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetHours, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setHours"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Hours, scriptContext);
    }

    Var JavascriptDate::EntrySetMilliseconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetMilliseconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setMilliseconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Milliseconds, scriptContext);
    }

    Var JavascriptDate::EntrySetMinutes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetMinutes, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setMinutes"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Minutes, scriptContext);
    }

    Var JavascriptDate::EntrySetMonth(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetMonth, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setMonth"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Month, scriptContext);
    }

    Var JavascriptDate::EntrySetSeconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetSeconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setSeconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetDateData(date, args, DateImplementation::DateData::Seconds, scriptContext);
    }

    Var JavascriptDate::EntrySetTime(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetTime, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setTime"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        double value;
        if (args.Info.Count > 1)
        {
            value = JavascriptConversion::ToNumber(args[1], scriptContext);
            if (Js::NumberUtilities::IsFinite(value))
            {
                value = JavascriptConversion::ToInteger(value);
            }
        }
        else
        {
            value = JavascriptNumber::NaN;
        }

        date->m_date.SetTvUtc(value);

        return JavascriptNumber::ToVarNoCheck(value, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCDate(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCDate, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCDate"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Date, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCFullYear(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCFullYear, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCFullYear"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::FullYear, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCHours(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCHours, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCHours"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Hours, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCMilliseconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCMilliseconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCMilliseconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Milliseconds, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCMinutes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCMinutes, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCMinutes"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Minutes, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCMonth(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCMonth, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCMonth"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Month, scriptContext);
    }

    Var JavascriptDate::EntrySetUTCSeconds(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntrySetUTCSeconds, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.setUTCSeconds"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        return JavascriptDate::SetUTCDateData(date, args, DateImplementation::DateData::Seconds, scriptContext);
    }

    Var JavascriptDate::EntryToDateString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToDateString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toDateString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetString(
            DateImplementation::DateStringFormat::Default, scriptContext,
            DateImplementation::DateTimeFlag::NoTime);
    }

    Var JavascriptDate::EntryToISOString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));
        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Date_Prototype_toISOString);

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToISOString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toISOString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetISOString(scriptContext);
    }

    Var JavascriptDate::EntryToJSON(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Data.prototype.toJSON"));
        }
        RecyclableObject* thisObj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Date.prototype.toJSON"));
        }

        Var result = nullptr;
        if (TryInvokeRemotely(EntryToJSON, scriptContext, args, &result))
        {
            return result;
        }

        Var num = JavascriptConversion::ToPrimitive<JavascriptHint::HintNumber>(thisObj, scriptContext);
        if (JavascriptNumber::Is(num)
            && !NumberUtilities::IsFinite(JavascriptNumber::GetValue(num)))
        {
            return scriptContext->GetLibrary()->GetNull();
        }

        Var toISO = JavascriptOperators::GetProperty(thisObj, PropertyIds::toISOString, scriptContext, NULL);
        if (!JavascriptConversion::IsCallable(toISO))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_Property_NeedFunction, scriptContext->GetPropertyName(PropertyIds::toISOString)->GetBuffer());
        }
        RecyclableObject* toISOFunc = VarTo<RecyclableObject>(toISO);
        return scriptContext->GetThreadContext()->ExecuteImplicitCall(toISOFunc, Js::ImplicitCall_Accessor, [=]()->Js::Var
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(), toISOFunc, CallInfo(1), thisObj);
        });
    }

    Var JavascriptDate::EntryToLocaleDateString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToLocaleDateString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toLocaleDateString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

#ifdef ENABLE_INTL_OBJECT
        if (CONFIG_FLAG(IntlBuiltIns) && scriptContext->IsIntlEnabled()){

            EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
            if (nativeEngineInterfaceObj)
            {
                IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));
                JavascriptFunction* func = extensionObject->GetDateToLocaleDateString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }

                // Initialize Date.prototype.toLocaleDateString
                scriptContext->GetLibrary()->InitializeIntlForDatePrototype();
                func = extensionObject->GetDateToLocaleDateString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }
            }
        }
#endif

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetString(
            DateImplementation::DateStringFormat::Locale, scriptContext,
            DateImplementation::DateTimeFlag::NoTime);
    }

    Var JavascriptDate::EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);

        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToLocaleString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toLocaleString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

#ifdef ENABLE_INTL_OBJECT
        if (CONFIG_FLAG(IntlBuiltIns) && scriptContext->IsIntlEnabled()){

            EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
            if (nativeEngineInterfaceObj)
            {
                IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));
                JavascriptFunction* func = extensionObject->GetDateToLocaleString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }
                // Initialize Date.prototype.toLocaleString
                scriptContext->GetLibrary()->InitializeIntlForDatePrototype();
                func = extensionObject->GetDateToLocaleString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }
            }
        }
#endif

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return JavascriptDate::ToLocaleString(date, scriptContext);
    }

    JavascriptString* JavascriptDate::ToLocaleString(JavascriptDate* date,
        ScriptContext* requestContext)
    {
        return date->m_date.GetString(DateImplementation::DateStringFormat::Locale, requestContext);
    }

    JavascriptString* JavascriptDate::ToString(JavascriptDate* date,
        ScriptContext* requestContext)
    {
        Assert(date);
        return date->m_date.GetString(DateImplementation::DateStringFormat::Default, requestContext);
    }

    Var JavascriptDate::EntryToLocaleTimeString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToLocaleTimeString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toLocaleTimeString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

#ifdef ENABLE_INTL_OBJECT
        if (CONFIG_FLAG(IntlBuiltIns) && scriptContext->IsIntlEnabled()){

            EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
            if (nativeEngineInterfaceObj)
            {
                IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));
                JavascriptFunction* func = extensionObject->GetDateToLocaleTimeString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }
                // Initialize Date.prototype.toLocaleTimeString
                scriptContext->GetLibrary()->InitializeIntlForDatePrototype();
                func = extensionObject->GetDateToLocaleTimeString();
                if (func)
                {
                    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                    {
                        return func->CallFunction(args);
                    }
                    END_SAFE_REENTRANT_CALL
                }
            }
        }
#endif

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetString(
            DateImplementation::DateStringFormat::Locale, scriptContext,
            DateImplementation::DateTimeFlag::NoDate);
    }

    Var JavascriptDate::EntryToTimeString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToTimeString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toTimeString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);


        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetString(
            DateImplementation::DateStringFormat::Default, scriptContext,
            DateImplementation::DateTimeFlag::NoDate);
    }

    Var JavascriptDate::EntryToUTCString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToUTCString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toUTCString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return date->m_date.GetString(
            DateImplementation::DateStringFormat::GMT, scriptContext,
            DateImplementation::DateTimeFlag::None);
    }

    Var JavascriptDate::EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryValueOf, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.valueOf"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        double value = date->m_date.GetMilliSeconds();
        return JavascriptNumber::ToVarNoCheck(value, scriptContext);
    }

    Var JavascriptDate::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0 || !VarIs<JavascriptDate>(args[0]))
        {
            Var result = nullptr;
            if (TryInvokeRemotely(EntryToString, scriptContext, args, &result))
            {
                return result;
            }
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedDate, _u("Date.prototype.toString"));
        }
        JavascriptDate* date = VarTo<JavascriptDate>(args[0]);

        AssertMsg(args.Info.Count > 0, "Negative argument count");
        return JavascriptDate::ToString(date, scriptContext);
    }

    BOOL JavascriptDate::TryInvokeRemotely(JavascriptMethod entryPoint, ScriptContext * scriptContext, Arguments & args, Var * result)
    {
        if (JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch)
        {
            if (VarTo<RecyclableObject>(args[0])->InvokeBuiltInOperationRemotely(entryPoint, args, result))
            {
                return TRUE;
            }
        }
        return FALSE;
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptDate::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapDateObject;
    }

    void JavascriptDate::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(this->GetTypeId() == TypeIds_Date, "We don't handle WinRT or other types of dates yet!");

        double* millis = alloc.SlabAllocateStruct<double>();
        *millis = m_date.GetMilliSeconds();

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<double*, TTD::NSSnapObjects::SnapObjectType::SnapDateObject>(objData, millis);
    }
#endif

    BOOL JavascriptDate::ToPrimitive(JavascriptHint hint, Var* result, ScriptContext * requestContext)
    {
        if (hint == JavascriptHint::None)
        {
            hint = JavascriptHint::HintString;
        }

        return DynamicObject::ToPrimitive(hint, result, requestContext);
    }

    BOOL JavascriptDate::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        ENTER_PINNED_SCOPE(JavascriptString, valueStr);
        valueStr = this->m_date.GetString(
            DateImplementation::DateStringFormat::Default, requestContext);
        stringBuilder->Append(valueStr->GetString(), valueStr->GetLength());
        LEAVE_PINNED_SCOPE();
        return TRUE;
    }

    BOOL JavascriptDate::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (Date)"));
        return TRUE;
    }
} // namespace Js
