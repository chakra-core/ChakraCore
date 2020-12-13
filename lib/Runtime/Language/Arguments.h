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
// We use a custom calling convention to invoke JavascriptMethod based on
// System ABI. At entry of JavascriptMethod the stack layout is:
//      [Return Address] [function] [callInfo] [arg0] [arg1] ...
//
#define DECLARE_ARGS_VARARRAY_N(va, n)                              \
    Js::Var* va = _get_va(_AddressOfReturnAddress(), n);            \
    Assert(*reinterpret_cast<Js::CallInfo*>(va - 1) == callInfo)

#define DECLARE_ARGS_VARARRAY(va, ...)                              \
    DECLARE_ARGS_VARARRAY_N(va, _count_args(__VA_ARGS__))

inline Js::Var* _get_va(void* addrOfReturnAddress, int n)
{
    // All args are right after ReturnAddress by custom calling convention
    Js::Var* pArgs = reinterpret_cast<Js::Var*>(addrOfReturnAddress) + 1;
#ifdef _ARM_
    n += 2; // ip + fp
#endif
    return pArgs + n;
}

inline int _count_args(Js::CallInfo callInfo)
{
    // This is to support typical runtime "ARGUMENTS(args, callInfo)" usage.
    // Only "callInfo" listed, but we have 2 known args "function, callInfo".
    return 2;
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
template <class T1, class T2, class T3, class T4, class T5>
inline int _count_args(const T1&, const T2&, const T3&, const T4&, const T5&, Js::CallInfo callInfo)
{
    return 6;
}
#endif


#ifdef _WIN32
#define CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, ##__VA_ARGS__)
#elif defined(_M_X64) || defined(_M_IX86)
// Call an entryPoint (JavascriptMethod) with custom calling convention.
//  RDI == function, RSI == callInfo, (RDX/RCX/R8/R9==null/unused),
//  all parameters on stack.
#define CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, nullptr, nullptr, nullptr, nullptr, \
               function, callInfo, ##__VA_ARGS__)
#elif defined(_ARM_)
// xplat-todo: fix me ARM
#define CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ...) \
    entryPoint(function, callInfo, ##__VA_ARGS__)
#else
#error CALL_ENTRYPOINT_NOASSERT not yet implemented
#endif

#define CALL_FUNCTION_NOASSERT(function, callInfo, ...) \
    CALL_ENTRYPOINT_NOASSERT(function->GetEntryPoint(), \
                    function, callInfo, ##__VA_ARGS__)

#if DBG && ENABLE_JS_REENTRANCY_CHECK
#define SETOBJECT_FOR_MUTATION(reentrancyLock, arrayObject) reentrancyLock.setObjectForMutation(arrayObject)
#define SET_SECOND_OBJECT_FOR_MUTATION(reentrancyLock, arrayObject) reentrancyLock.setSecondObjectForMutation(arrayObject)
#define MUTATE_ARRAY_OBJECT(reentrancyLock) reentrancyLock.MutateArrayObject()
#else
#define SETOBJECT_FOR_MUTATION(reentrancyLock, arrayObject) 
#define SET_SECOND_OBJECT_FOR_MUTATION(reentrancyLock, arrayObject) 
#define MUTATE_ARRAY_OBJECT(reentrancyLock) 
#endif

#if ENABLE_JS_REENTRANCY_CHECK
#define JS_REENTRANCY_CHECK(threadContext, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(), \
    threadContext->AssertJsReentrancy(), \
    __VA_ARGS__);
#if DBG && ENABLE_NATIVE_CODEGEN
#define CALL_FUNCTION(threadContext, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(), \
    threadContext->AssertJsReentrancy(), \
    CheckIsExecutable(function, function->GetEntryPoint()), \
    CALL_FUNCTION_NOASSERT(function, callInfo, ##__VA_ARGS__));
#else
#define CALL_FUNCTION(threadContext, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(), \
    threadContext->AssertJsReentrancy(), \
    CALL_FUNCTION_NOASSERT(function, callInfo, ##__VA_ARGS__));
#endif
#define CALL_ENTRYPOINT(threadContext, entryPoint, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(), \
    threadContext->AssertJsReentrancy(), \
    CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ##__VA_ARGS__));
#define JS_REENTRANT(reentrancyLock, ...) \
    reentrancyLock.unlock(); \
    __VA_ARGS__; \
    MUTATE_ARRAY_OBJECT(reentrancyLock); \
    reentrancyLock.relock();
#define JS_REENTRANT_NO_MUTATE(reentrancyLock, ...) \
    reentrancyLock.unlock(); \
    __VA_ARGS__; \
    reentrancyLock.relock();
#define JS_REENTRANT_UNLOCK(reentrancyLock, ...) \
    reentrancyLock.unlock(); \
    __VA_ARGS__;
#define JS_REENTRANCY_LOCK(reentrancyLock, threadContext) \
    JsReentLock reentrancyLock(threadContext);
#else
#if DBG && ENABLE_NATIVE_CODEGEN
#define CALL_FUNCTION(threadContext, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(),\
    CheckIsExecutable(function, function->GetEntryPoint()), \
    CALL_FUNCTION_NOASSERT(function, callInfo, ##__VA_ARGS__));
#else
#define CALL_FUNCTION(threadContext, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(),\
    CALL_FUNCTION_NOASSERT(function, callInfo, ##__VA_ARGS__));
#endif
#define JS_REENTRANCY_CHECK(threadContext, ...) \
    __VA_ARGS__;
#define CALL_ENTRYPOINT(threadContext, entryPoint, function, callInfo, ...) \
    (threadContext->CheckAndResetReentrancySafeOrHandled(), \
    CALL_ENTRYPOINT_NOASSERT(entryPoint, function, callInfo, ##__VA_ARGS__));
#define JS_REENTRANT(reentrancyLock, ...) \
    __VA_ARGS__;
#define JS_REENTRANT_NO_MUTATE(reentrancyLock, ...) \
    __VA_ARGS__;
#define JS_REENTRANT_UNLOCK(reentrancyLock, ...) \
    __VA_ARGS__;
#define JS_REENTRANCY_LOCK(reentrancyLock, threadContext)
#endif // ENABLE_JS_REENTRANCY_CHECK

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

        Arguments(const Arguments& other) : Info(other.Info), Values(other.Values) {}

        Var operator [](int idxArg) { return const_cast<Var>(static_cast<const Arguments&>(*this)[idxArg]); }
        Var operator [](int idxArg) const
        {
            AssertMsg((idxArg < (int)Info.Count) && (idxArg >= 0), "Ensure a valid argument index");
            return Values[idxArg];
        }

        bool IsDirectEvalCall() const
        {
            // This was recognized as an eval call at compile time. The last one or two args are internal to us.
            // Argcount will be one of the following when called from global code
            //  - eval("...")     : argcount 3 : this, evalString, frameDisplay
            //  - eval.call("..."): argcount 2 : this(which is string) , frameDisplay

            return (Info.Flags & (CallFlags_ExtraArg | CallFlags_NewTarget)) == CallFlags_ExtraArg;  // ExtraArg == 1 && NewTarget == 0
        }

        bool HasExtraArg() const
        {
            return Info.HasExtraArg();
        }

        bool HasArg() const
        {
            return Info.Count > 0;
        }

        ushort GetArgCountWithExtraArgs() const
        {
            return Info.GetArgCountWithExtraArgs();
        }

        uint GetLargeArgCountWithExtraArgs() const
        {
            return Info.GetLargeArgCountWithExtraArgs();
        }

        FrameDisplay* GetFrameDisplay() const
        {
            AssertOrFailFast((Info.Flags & CallFlags_ExtraArg) && (!this->HasNewTarget()));

            // There is an extra arg, so values should have Count + 1 members
            return (FrameDisplay*)(this->Values[Info.Count]);
        }

        bool IsNewCall() const
        {
            return (Info.Flags & CallFlags_New) == CallFlags_New;
        }

        bool HasNewTarget() const
        {
            return Info.HasNewTarget();
        }

        // New target value is passed as an extra argument which is nto included in the Info.Count
        Var GetNewTarget() const
        {
            return CallInfo::GetNewTarget(Info.Flags, this->Values, Info.Count);
        }

        // swb: Arguments is mostly used on stack and does not need write barrier.
        // It is recycler allocated with ES6 generators. We handle that specially.
        FieldNoBarrier(CallInfo) Info;
        FieldNoBarrier(Var*) Values;

        static uint32 GetCallInfoOffset() { return offsetof(Arguments, Info); }
        static uint32 GetValuesOffset() { return offsetof(Arguments, Values); }

        // Prevent heap/recycler allocation, so we don't need write barrier for this
        static void* operator new   (size_t)    = delete;
        static void* operator new[] (size_t)    = delete;
        static void  operator delete   (void*)  = delete;
        static void  operator delete[] (void*)  = delete;
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
            AssertMsg(!this->HasNewTarget() || (Info.Flags & Js::CallFlags_ExtraArg), "NewTarget flag must be used together with ExtraArg.");
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
                callInfo->Flags = (CallFlags)(callInfo->Flags & ~Js::CallFlags_ExtraArg);
            }
        }
    };
}
