//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    Var JavascriptAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(RecyclableObject* result, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        // 1. Let done be IteratorComplete(result).
        // 2. IfAbruptRejectPromise(done, promiseCapability).
        bool done;
        try
        {
            done = JavascriptOperators::IteratorComplete(result, scriptContext);
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject* exception = err.GetAndClear();
            return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
        }

        // 3. Let value be IteratorValue(result).
        // 4. IfAbruptRejectPromise(value, promiseCapability).
        Var value = nullptr;
        try
        {
            value = JavascriptOperators::IteratorValue(result, scriptContext);
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject* exception = err.GetAndClear();
            return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
        }

        // 5. Let valueWrapper be ? PromiseResolve(%Promise%, <<value>>).
        JavascriptPromise* valueWrapper = JavascriptPromise::InternalPromiseResolve(value, scriptContext);

        // 6. Let steps be the algorithm steps defined in Async-from-Sync Iterator Value Unwrap Functions.
        // 7. Let onFulfilled be CreateBuiltinFunction(steps, <<[[Done]]>>).
        // 8. Set onFulfilled.[[Done]] to done.
        RecyclableObject* onFulfilled;
        if (done)
        {
            onFulfilled = library->EnsureAsyncFromSyncIteratorValueUnwrapTrueFunction();
        }
        else
        {
            onFulfilled = library->EnsureAsyncFromSyncIteratorValueUnwrapFalseFunction();
        }

        // 9. Perform ! PerformPromiseThen(valueWrapper, onFulfilled, undefined, promiseCapability).
        // 10. Return promiseCapability.[[Promise]].
        return JavascriptPromise::CreateThenPromise(valueWrapper, onFulfilled, library->GetThrowerFunction(), scriptContext);
    }

    Var JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorValueUnwrapTrueFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertOrFailFastMsg(args.Info.Count > 1, "AsyncFromSyncIteratorValueUnwrap should never be called without an argument");
        JavascriptLibrary* library = scriptContext->GetLibrary();

        return library->CreateIteratorResultObject(args[1], library->GetTrue());
    }

    Var JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorValueUnwrapFalseFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertOrFailFastMsg(args.Info.Count > 1, "AsyncFromSyncIteratorValueUnwrap should never be called without an argument");
        JavascriptLibrary* library = scriptContext->GetLibrary();

        return library->CreateIteratorResultObject(args[1], library->GetFalse());
    }

    Var JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorNext(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertMsg(args.Info.Count > 0, "AsyncFromSyncIteratorNext should have implicit this");
        // 1. Let O be the this value.
        // 2. Let promiseCapability be ! NewPromiseCapability(%Promise%).
        // promise creation deferred to the end to simplify logic

        // this step skipped as no codepath exists that will enable reaching here with wrong object type
        // 3. If Type(O) is not Object, or if O does not have a [[SyncIteratorRecord]] internal slot, then
        //    a. Let invalidIteratorError be a newly created TypeError object.
        //    b. Perform ! Call(promiseCapability.[[Reject]], undefined, << invalidIteratorError >>).
        //    c. Return promiseCapability.[[Promise]].

        // this will failfast if the logic that makes steps 3 irrelevant changes
        JavascriptAsyncFromSyncIterator* thisValue = VarTo<JavascriptAsyncFromSyncIterator>(args[0]);

        // 4. Let syncIteratorRecord be O.[[SyncIteratorRecord]].
        RecyclableObject* syncIteratorRecord = thisValue->GetSyncIterator();
        RecyclableObject* result = nullptr;
        // 5. Let result be IteratorNext(syncIteratorRecord, value).
        try
        {
            result = JavascriptOperators::IteratorNext(syncIteratorRecord, scriptContext, thisValue->EnsureSyncNextFunction(scriptContext), args.Info.Count > 1 ? args[1] : nullptr);
        }
        catch (const JavascriptException& err)
        {
            // 6. IfAbruptRejectPromise(result, promiseCapability).
            JavascriptExceptionObject* exception = err.GetAndClear();
            if (exception != nullptr)
            {
                return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
            }
        }
 
        // 7. Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
        return JavascriptAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(result, scriptContext);
    }

    Var JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorReturn(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertMsg(args.Info.Count > 0, "AsyncFromSyncIteratorReturn should have implicit this");
        JavascriptLibrary* library = scriptContext->GetLibrary();

        // 1. Let O be the this value.
        // 2. Let promiseCapability be ! NewPromiseCapability(%Promise%).
        // promise creation deferred to the end to simplify logic

        // this step skipped as no codepath exists that will enable reaching here with wrong object type
        // 3. If Type(O) is not Object, or if O does not have a [[SyncIteratorRecord]] internal slot, then
        //    a. Let invalidIteratorError be a newly created TypeError object.
        //    b. Perform ! Call(promiseCapability.[[Reject]], undefined, << invalidIteratorError >>).
        //    c. Return promiseCapability.[[Promise]].

        // this will failfast if the logic that makes steps 3-6 irrelevant changes
        JavascriptAsyncFromSyncIterator* thisValue = VarTo<JavascriptAsyncFromSyncIterator>(args[0]);

        // 4. Let syncIteratorRecord be O.[[SyncIteratorRecord]].
        RecyclableObject* syncIteratorRecord = thisValue->GetSyncIterator();
        Var returnMethod = nullptr;
        Var result = nullptr;

        // 5. Let return be GetMethod(syncIterator, "return").
        // 6. IfAbruptRejectPromise(return, promiseCapability).
        // 7. If return is undefined, then
        //    a. Perform ! Call(promiseCapability.[[Reject]], undefined, << value >>).
        //    b. Return promiseCapability.[[Promise]].
        try
        {
            returnMethod = JavascriptOperators::GetProperty(syncIteratorRecord, PropertyIds::return_, scriptContext);
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject* exception = err.GetAndClear();
            if (exception != nullptr)
            {
                return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
            }
        }

        if (returnMethod == library->GetUndefined())
        {
            result = library->CreateIteratorResultObject(args.Info.Count > 1 ? args[1] : library->GetUndefined(), library->GetTrue());
            return JavascriptPromise::CreateResolvedPromise(result, scriptContext);
        }

        if (!JavascriptConversion::IsCallable(returnMethod))
        {
            JavascriptError* typeError = library->CreateTypeError();
            JavascriptError::SetErrorMessage(typeError, JSERR_NeedFunction, _u("AsyncFromSyncIteratorThrow.prototype.return"), scriptContext);
            return JavascriptPromise::CreateRejectedPromise(typeError, scriptContext);
        }

        // 8. Let result be Call(return, syncIterator, << value >>).
        try
        {
            RecyclableObject* callable = VarTo<RecyclableObject>(returnMethod);
            Var value = args.Info.Count > 1 ? args[1] : nullptr;
            result = scriptContext->GetThreadContext()->ExecuteImplicitCall(callable, ImplicitCall_Accessor, [=]() -> Var
            {
                Js::Var args[] = { syncIteratorRecord, value };
                Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args) + (value == nullptr ? -1 : 0));
                return JavascriptFunction::CallFunction<true>(callable, callable->GetEntryPoint(), Arguments(callInfo, args));
            });
        }
        catch (const JavascriptException& err)
        {
            // 9. IfAbruptRejectPromise(result, promiseCapability).
            JavascriptExceptionObject* exception = err.GetAndClear();
            if (exception != nullptr)
            {
                return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
            }
        }

        // 10. If Type(result) is not Object, then
        //     a. Perform ! Call(promiseCapability.[[Reject]], undefined, << a newly created TypeError object >>).
        //     b. Return promiseCapability.[[Promise]].
        if (!JavascriptOperators::IsObject(result))
        {
            JavascriptError* typeError = library->CreateTypeError();
            JavascriptError::SetErrorMessage(typeError, JSERR_NonObjectFromIterable, _u("AsyncFromSyncIteratorThrow.prototype.return"), scriptContext);
            return JavascriptPromise::CreateRejectedPromise(typeError, scriptContext);   
        }

        // 11. Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
        return JavascriptAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(UnsafeVarTo<RecyclableObject>(result), scriptContext);
    }

    Var JavascriptAsyncFromSyncIterator::EntryAsyncFromSyncIteratorThrow(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        AssertMsg(args.Info.Count > 0, "AsyncFromSyncIteratorThrow should have implicit this");
        JavascriptLibrary* library = scriptContext->GetLibrary();

        // 1. Let O be the this value.
        // 2. Let promiseCapability be ! NewPromiseCapability(%Promise%).
        // promise creation deferred to the end to simplify logic

        // this step skipped as no code path exists that will enable reaching here with wrong object type
        // 3. If Type(O) is not Object, or if O does not have a [[SyncIteratorRecord]] internal slot, then
        //    a. Let invalidIteratorError be a newly created TypeError object.
        //    b. Perform ! Call(promiseCapability.[[Reject]], undefined, << invalidIteratorError >>).
        //    c. Return promiseCapability.[[Promise]].

        // this will failfast if the logic that makes step 3 irrelevant changes
        JavascriptAsyncFromSyncIterator* thisValue = VarTo<JavascriptAsyncFromSyncIterator>(args[0]);

        // 4. Let syncIteratorRecord be O.[[SyncIteratorRecord]].
        RecyclableObject* syncIteratorRecord = thisValue->GetSyncIterator();
        Var throwMethod = nullptr;

        // 5. Let throw be GetMethod(syncIterator, "throw").
        // 6. IfAbruptRejectPromise(throw, promiseCapability).
        // 7. If throw is undefined, then
        //    a. Perform ! Call(promiseCapability.[[Reject]], undefined, << value >>).
        //    b. Return promiseCapability.[[Promise]].
        try
        {
            throwMethod = JavascriptOperators::GetProperty(syncIteratorRecord, PropertyIds::throw_, scriptContext);
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject* exception = err.GetAndClear();
            if (exception != nullptr)
            {
                return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
            }
        }

        if (throwMethod == library->GetUndefined())
        {
            return JavascriptPromise::CreateRejectedPromise(library->GetUndefined(), scriptContext);
        }

        if (!JavascriptConversion::IsCallable(throwMethod))
        {

            JavascriptError* typeError = library->CreateTypeError();
            JavascriptError::SetErrorMessage(typeError, JSERR_NeedFunction, _u("AsyncFromSyncIteratorThrow.prototype.throw"), scriptContext);
            return JavascriptPromise::CreateRejectedPromise(typeError, scriptContext);
        }

        // 8. Let result be Call(throw, syncIterator, << value >>).
        Var result = nullptr;
        try
        {
            RecyclableObject* callable = VarTo<RecyclableObject>(throwMethod);
            Var value = args.Info.Count > 1 ? args[1] : nullptr;
            result = scriptContext->GetThreadContext()->ExecuteImplicitCall(callable, ImplicitCall_Accessor, [=]() -> Var
            {
                Js::Var args[] = { syncIteratorRecord, value };
                Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args) + (value == nullptr ? -1 : 0));
                return JavascriptFunction::CallFunction<true>(callable, callable->GetEntryPoint(), Arguments(callInfo, args));
            });
        }
        catch (const JavascriptException& err)
        {
            // 9. IfAbruptRejectPromise(result, promiseCapability).
            JavascriptExceptionObject* exception = err.GetAndClear();
            if (exception != nullptr)
            {
                return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
            }
        }

        // 10. If Type(result) is not Object, then
        //     a. Perform ! Call(promiseCapability.[[Reject]], undefined, << a newly created TypeError object >>).
        //     b. Return promiseCapability.[[Promise]].
        if (!JavascriptOperators::IsObject(result))
        {
            JavascriptError* typeError = library->CreateTypeError();
            JavascriptError::SetErrorMessage(typeError, JSERR_NonObjectFromIterable, _u("AsyncFromSyncIteratorThrow.prototype.throw"), scriptContext);
            return JavascriptPromise::CreateRejectedPromise(typeError, scriptContext);   
        }

        // 11. Return ! AsyncFromSyncIteratorContinuation(result, promiseCapability).
        return JavascriptAsyncFromSyncIterator::AsyncFromSyncIteratorContinuation(UnsafeVarTo<RecyclableObject>(result), scriptContext);
    }

    RecyclableObject* JavascriptAsyncFromSyncIterator::EnsureSyncNextFunction(ScriptContext* scriptContext)
    {
        if (syncNextFunction == nullptr)
        {
            syncNextFunction = JavascriptOperators::CacheIteratorNext(syncIterator, scriptContext);
        }
        return syncNextFunction;
    }

    template <> bool VarIsImpl<JavascriptAsyncFromSyncIterator>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_AsyncFromSyncIterator;
    }
}
