//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// To extract variadic args array after known args list:
//      argx, callInfo, ...
// NOTE: The last known arg name is hard-coded to "callInfo".
#ifdef _WIN32
#define DECLARE_ARGS_VARARRAY(va, ...)                              \
    va_list _vl;                                                    \
    va_start(_vl, callInfo);                                        \
    Js::Var* va = (Js::Var*)_vl
#else
#if defined(_M_X64) || defined(_M_IX86)
// We use a custom calling convention to invoke JavascriptMethod based on
// System V AMD64 ABI. At entry of JavascriptMethod the stack layout is:
//      [Return Address] [function] [callInfo] [arg0] [arg1] ...
//
#define DECLARE_ARGS_VARARRAY_N(va, n)                              \
    Js::Var* va = _get_va(_AddressOfReturnAddress(), n);            \
    Assert(*reinterpret_cast<Js::CallInfo*>(va - 1) == callInfo)

#define DECLARE_ARGS_VARARRAY(va, ...)                              \
    DECLARE_ARGS_VARARRAY_N(va, _count_args(__VA_ARGS__))

inline Js::Var* _get_va(void* addrOfReturnAddress, int n)
{
    Js::Var* pArgs = reinterpret_cast<Js::Var*>(addrOfReturnAddress) + 1;
    return pArgs + n;
}

inline int _count_args(Js::CallInfo callInfo)
{
    return 2;  // for typical JsMethod with 2 known args "function, callInfo"
}
template <class T1>
inline int _count_args(const T1&, Js::CallInfo callInfo)
{
    return 2;
}
template <class T1, class T2>
inline int _count_args(const T1&, const T2&, Js::CallInfo callInfo)
{
    return 3;
}
template <class T1, class T2, class T3>
inline int _count_args(const T1&, const T2&, const T3&, Js::CallInfo callInfo)
{
    return 4;
}
template <class T1, class T2, class T3, class T4>
inline int _count_args(const T1&, const T2&, const T3&, const T4&, Js::CallInfo callInfo)
{
    return 5;
}

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
 *
 * The ... list must be
 *  * "callInfo", typically for JsMethod that has only 2 known args
 *    "function, callInfo";
 *  * or the full known args list ending with "callInfo" (for some runtime
 *    helpers).
 */
#define RUNTIME_ARGUMENTS(n, ...)                       \
    DECLARE_ARGS_VARARRAY(_argsVarArray, __VA_ARGS__);  \
    Js::Arguments n(callInfo, _argsVarArray);

#define ARGUMENTS(n, ...)                               \
    DECLARE_ARGS_VARARRAY(_argsVarArray, __VA_ARGS__);  \
    Js::ArgumentReader n(&callInfo, _argsVarArray);

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
