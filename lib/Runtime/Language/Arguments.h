//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _WIN32
#define VA_LIST_TO_VARARRAY(vl, va, callInfo) Js::Var* va = (Js::Var*) vl;
#else
#if defined(_M_X64) || defined(_M_IX86)
// We use a custom calling convention to invoke JavascriptMethod based on
// System V AMD64 ABI. At entry of JavascriptMethod the stack layout is:
//      [Return Address] [function] [callInfo] [arg0] [arg1] ...
//
#define VA_LIST_TO_VARARRAY(vl, va, callInfo)                              \
    Js::Var* va = reinterpret_cast<Js::Var*>(_AddressOfReturnAddress()) + 3; \
    Assert(*reinterpret_cast<Js::CallInfo*>(va - 1) == callInfo);
#else
#error Not yet implemented
#endif
#endif


#ifdef _WIN32
#define CALL_ENTRYPOINT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, ##__VA_ARGS__)
#elif defined(_M_X64) || defined(_M_IX86)
// Call an entryPoint (JavascriptMethod) with custom calling convention.
//  RDI == function, RSI == callInfo, (RDX/RCX/R8/R9==null/unused),
//  all parameters on stack.
#define CALL_ENTRYPOINT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, nullptr, nullptr, nullptr, nullptr, \
               function, callInfo, ##__VA_ARGS__)
#else
#error CALL_ENTRYPOINT not yet implemented
#endif

#define CALL_FUNCTION(function, callInfo, ...) \
    CALL_ENTRYPOINT(function->GetEntryPoint(), \
                    function, callInfo, ##__VA_ARGS__)


/*
 * RUNTIME_ARGUMENTS is a simple wrapper around the variadic calling convention
 * used by JavaScript functions. It is a low level macro that does not try to
 * differentiate between script usable Vars and runtime data structures.
 * To be able to access only script usable args use the ARGUMENTS macro instead.
 */
#define RUNTIME_ARGUMENTS(n, s)                                           \
    va_list argptr;                                                       \
    va_start(argptr, s);                                                  \
    VA_LIST_TO_VARARRAY(argptr, _argsVarArray, s)                         \
    Js::Arguments n(s, _argsVarArray);

#define ARGUMENTS(n, s)                                                   \
    va_list argptr;                                                       \
    va_start(argptr, s);                                                  \
    VA_LIST_TO_VARARRAY(argptr, _argsVarArray, s)                         \
    Js::ArgumentReader n(&s, _argsVarArray);

namespace Js
{
    struct Arguments
    {
    public:
        Arguments(CallInfo callInfo, Var* values) :
            Info(callInfo), Values(values) {}

        Arguments(ushort count, Var* values) :
            Info(count), Values(values) {}

        Arguments(VirtualTableInfoCtorEnum v) : Info(v) {}

        Var operator [](int idxArg) { return const_cast<Var>(static_cast<const Arguments&>(*this)[idxArg]); }
        const Var operator [](int idxArg) const
        {
            AssertMsg((idxArg < (int)Info.Count) && (idxArg >= 0), "Ensure a valid argument index");
            return Values[idxArg];
        }
        CallInfo Info;
        Var* Values;

        static uint32 GetCallInfoOffset() { return offsetof(Arguments, Info); }
        static uint32 GetValuesOffset() { return offsetof(Arguments, Values); }
    };

    struct ArgumentReader : public Arguments
    {
        ArgumentReader(CallInfo *callInfo, Var* values)
            : Arguments(*callInfo, values)
        {
            AdjustArguments(callInfo);
        }

    private:
        void AdjustArguments(CallInfo *callInfo)
        {
            AssertMsg(!(Info.Flags & Js::CallFlags_NewTarget) || (Info.Flags & Js::CallFlags_ExtraArg), "NewTarget flag must be used together with ExtraArg.");
            if (Info.Flags & Js::CallFlags_ExtraArg)
            {
                // If "calling eval" is set, then the last param is the frame display, which only
                // the eval built-in should see.
                Assert(Info.Count > 0);
                // The local version should be consistent. On the other hand, lots of code throughout
                // jscript uses the callInfo from stack to get argument list etc. We'll need
                // to change all the caller to be aware of the id or somehow make sure they don't use
                // the stack version. Both seem risky. It would be safer and more robust to just
                // change the stack version.
                Info.Flags = (CallFlags)(Info.Flags & ~Js::CallFlags_ExtraArg);
                Info.Count--;
                callInfo->Flags = (CallFlags)(callInfo->Flags & ~Js::CallFlags_ExtraArg);
                callInfo->Count--;
            }
        }
    };
}
