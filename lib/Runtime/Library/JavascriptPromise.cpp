//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptPromise::JavascriptPromise(DynamicType * type)
        : DynamicObject(type),
        isHandled(false),
        status(PromiseStatus::PromiseStatusCode_Undefined),
        result(nullptr),
        reactions(nullptr)

    {
        Assert(type->GetTypeId() == TypeIds_Promise);
    }

    // Promise() as defined by ES 2016 Sections 25.4.3.1
    Var JavascriptPromise::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Promise, scriptContext);

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch
        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise"));

        // 1. If NewTarget is undefined, throw a TypeError exception.
        if ((callInfo.Flags & CallFlags_New) != CallFlags_New || (newTarget != nullptr && JavascriptOperators::IsUndefined(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("Promise"));
        }

        // 2. If IsCallable(executor) is false, throw a TypeError exception.
        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Promise"));
        }
        RecyclableObject* executor = VarTo<RecyclableObject>(args[1]);

        // 3. Let promise be ? OrdinaryCreateFromConstructor(NewTarget, "%PromisePrototype%", <<[[PromiseState]], [[PromiseResult]], [[PromiseFulfillReactions]], [[PromiseRejectReactions]], [[PromiseIsHandled]] >>).
        JavascriptPromise* promise = library->CreatePromise();
        if (isCtorSuperCall)
        {
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), promise, library->GetPromisePrototype(), scriptContext);
        }

        JavascriptPromiseResolveOrRejectFunction* resolve;
        JavascriptPromiseResolveOrRejectFunction* reject;

        // 4. Set promise's [[PromiseState]] internal slot to "pending".
        // 5. Set promise's [[PromiseFulfillReactions]] internal slot to a new empty List.
        // 6. Set promise's [[PromiseRejectReactions]] internal slot to a new empty List.
        // 7. Set promise's [[PromiseIsHandled]] internal slot to false.
        // 8. Let resolvingFunctions be CreateResolvingFunctions(promise).
        InitializePromise(promise, &resolve, &reject, scriptContext);

        JavascriptExceptionObject* exception = nullptr;

        // 9. Let completion be Call(executor, undefined, << resolvingFunctions.[[Resolve]], resolvingFunctions.[[Reject]] >>).
        try
        {
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                CALL_FUNCTION(scriptContext->GetThreadContext(),
                        executor, CallInfo(CallFlags_Value, 3),
                        library->GetUndefined(),
                        resolve,
                        reject);
            }
            END_SAFE_REENTRANT_CALL
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            // 10. If completion is an abrupt completion, then
            //    a. Perform ? Call(resolvingFunctions.[[Reject]], undefined, << completion.[[Value]] >>).
            TryRejectWithExceptionObject(exception, reject, scriptContext);
        }

        // 11. Return promise.
        return promise;
    }

    void JavascriptPromise::InitializePromise(JavascriptPromise* promise, JavascriptPromiseResolveOrRejectFunction** resolve, JavascriptPromiseResolveOrRejectFunction** reject, ScriptContext* scriptContext)
    {
        Assert(promise->GetStatus() == PromiseStatusCode_Undefined);
        Assert(resolve);
        Assert(reject);

        Recycler* recycler = scriptContext->GetRecycler();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        promise->SetStatus(PromiseStatusCode_Unresolved);

        promise->reactions = RecyclerNew(recycler, JavascriptPromiseReactionList, recycler);

        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
        alreadyResolvedRecord->alreadyResolved = false;

        *resolve = library->CreatePromiseResolveOrRejectFunction(EntryResolveOrRejectFunction, promise, false, alreadyResolvedRecord);
        *reject = library->CreatePromiseResolveOrRejectFunction(EntryResolveOrRejectFunction, promise, true, alreadyResolvedRecord);
    }

    BOOL JavascriptPromise::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("[...]"));

        return TRUE;
    }

    BOOL JavascriptPromise::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Promise"));
        return TRUE;
    }

    JavascriptPromiseReactionList* JavascriptPromise::GetReactions()
    {
        return this->reactions;
    }

    // Promise.all as described in ES 2015 Section 25.4.4.1
    Var JavascriptPromise::EntryAll(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.all"));

        // 1. Let C be the this value.
        Var constructor = args[0];

        // 2. If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.all"));
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var iterable;

        if (args.Info.Count > 1)
        {
            iterable = args[1];
        }
        else
        {
            iterable = library->GetUndefined();
        }

        // 3. Let promiseCapability be NewPromiseCapability(C).
        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(constructor, scriptContext);

        // We know that constructor is an object at this point - further, we even know that it is a constructor - because NewPromiseCapability
        // would throw otherwise. That means we can safely cast constructor into a RecyclableObject* now and avoid having to perform ToObject
        // as part of the Invoke operation performed inside the loop below.
        RecyclableObject* constructorObject = VarTo<RecyclableObject>(constructor);

        uint32 index = 0;
        JavascriptArray* values = nullptr;

        // We can't use a simple counter for the remaining element count since each Promise.all Resolve Element Function needs to know how many
        // elements are remaining when it runs and needs to update that counter for all other functions created by this call to Promise.all.
        // We can't just use a static variable, either, since this element count is only used for the Promise.all Resolve Element Functions created
        // by this call to Promise.all.
        JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper);
        remainingElementsWrapper->remainingElements = 1;

        JavascriptExceptionObject* exception = nullptr;
        try
        {
            // 4. Let iterator be GetIterator(iterable).
            RecyclableObject* iterator = JavascriptOperators::GetIterator(iterable, scriptContext);

            Var resolveVar = JavascriptOperators::GetProperty(constructorObject, Js::PropertyIds::resolve, scriptContext);
            if (!JavascriptConversion::IsCallable(resolveVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }

            RecyclableObject* resolveFunc = VarTo<RecyclableObject>(resolveVar);
            values = library->CreateArray(0);

            JavascriptOperators::DoIteratorStepAndValue(iterator, scriptContext, [&](Var next)
            {
                ThreadContext * threadContext = scriptContext->GetThreadContext();
                Var nextPromise = nullptr;
                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    nextPromise = CALL_FUNCTION(threadContext,
                                        resolveFunc, Js::CallInfo(CallFlags_Value, 2),
                                        constructorObject,
                                        next);
                }
                END_SAFE_REENTRANT_CALL

                JavascriptPromiseAllResolveElementFunction* resolveElement = library->CreatePromiseAllResolveElementFunction(EntryAllResolveElementFunction, index, values, promiseCapability, remainingElementsWrapper);

                remainingElementsWrapper->remainingElements++;

                RecyclableObject* nextPromiseObject;

                if (!JavascriptConversion::ToObject(nextPromise, scriptContext, &nextPromiseObject))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                }

                Var thenVar = JavascriptOperators::GetProperty(nextPromiseObject, Js::PropertyIds::then, scriptContext);

                if (!JavascriptConversion::IsCallable(thenVar))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
                }

                RecyclableObject* thenFunc = VarTo<RecyclableObject>(thenVar);

                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    CALL_FUNCTION(scriptContext->GetThreadContext(),
                        thenFunc, Js::CallInfo(CallFlags_Value, 3),
                        nextPromiseObject,
                        resolveElement,
                        promiseCapability->GetReject());
                }
                END_SAFE_REENTRANT_CALL

                index++;
            });

            remainingElementsWrapper->remainingElements--;
            if (remainingElementsWrapper->remainingElements == 0)
            {
                Assert(values != nullptr);

                TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), values, scriptContext);
            }
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        return promiseCapability->GetPromise();
    }

    Var JavascriptPromise::EntryAny(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.any"));

        // 1. Let C be the this value.
        Var C = args[0];

        // 2. Let promiseCapability be ? NewPromiseCapability(C).
        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(C, scriptContext);
        RecyclableObject* constructor = UnsafeVarTo<RecyclableObject>(C);

        JavascriptLibrary* library = scriptContext->GetLibrary();
        RecyclableObject* promiseResolve = nullptr;
        RecyclableObject* iteratorRecord = nullptr;
        try {
            // 3. Let promiseResolve be GetPromiseResolve(C).
            // 4. IfAbruptRejectPromise(promiseResolve, promiseCapability).
            Var resolveVar = JavascriptOperators::GetProperty(constructor, Js::PropertyIds::resolve, scriptContext);
            if (!JavascriptConversion::IsCallable(resolveVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }
            promiseResolve = UnsafeVarTo<RecyclableObject>(resolveVar);

            // 5. Let iteratorRecord be GetIterator(iterable).
            // 6. IfAbruptRejectPromise(iteratorRecord, promiseCapability).
            Var iterable = args.Info.Count > 1 ? args[1] : library->GetUndefined();
            iteratorRecord = JavascriptOperators::GetIterator(iterable, scriptContext); 
        }
        catch (const JavascriptException& err)
        {
            JavascriptExceptionObject* exception = err.GetAndClear();
            return JavascriptPromise::CreateRejectedPromise(exception->GetThrownObject(scriptContext), scriptContext);
        }

        // Abstract operation PerformPromiseAny
        // 7. Let result be PerformPromiseAny(iteratorRecord, C, promiseCapability, promiseResolve).
        try {
            // 1. Assert: ! IsConstructor(constructor) is true. 
            // 2. Assert: resultCapability is a PromiseCapability Record.
            // 3. Assert: ! IsCallable(promiseResolve) is true.
            // 4. Let errors be a new empty List.
            JavascriptArray* errors = library->CreateArray();

            // 5. Let remainingElementsCount be a new Record { [[Value]]: 1 }.
            JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper);
            remainingElementsWrapper->remainingElements = 1;

            // 6. Let index be 0.
            uint32 index = 0;

            // 7. Repeat,
            JavascriptOperators::DoIteratorStepAndValue(iteratorRecord, scriptContext, [&](Var nextValue) {
                // a. Let next be IteratorStep(iteratorRecord).
                // e. Let nextValue be IteratorValue(next).
                // h. Append undefined to errors.
                errors->DirectAppendItem(library->GetUndefined());

                // i. Let nextPromise be ? Call(promiseResolve, constructor, << nextValue >> ).
                ThreadContext* threadContext = scriptContext->GetThreadContext();
                Var nextPromise = nullptr;
                BEGIN_SAFE_REENTRANT_CALL(threadContext);
                {
                    nextPromise = CALL_FUNCTION(threadContext,
                        promiseResolve, Js::CallInfo(CallFlags_Value, 2),
                        constructor,
                        nextValue);
                }
                END_SAFE_REENTRANT_CALL;


                JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
                alreadyCalledWrapper->alreadyResolved = false;

                // j. Let steps be the algorithm steps defined in Promise.any Reject Element Functions.
                // k. Let rejectElement be ! CreateBuiltinFunction(steps, << [[AlreadyCalled]], [[Index]], [[Errors]], [[Capability]], [[RemainingElements]] >> ).
                // p. Set rejectElement.[[RemainingElements]] to remainingElementsCount.
                Var rejectElement = library->CreatePromiseAnyRejectElementFunction(EntryAnyRejectElementFunction, index, errors, promiseCapability, remainingElementsWrapper, alreadyCalledWrapper);

                // q. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] + 1.
                remainingElementsWrapper->remainingElements++;

                // r. Perform ? Invoke(nextPromise, "then", << resultCapability.[[Resolve]], rejectElement >> ).
                RecyclableObject* nextPromiseObject;
                if (!JavascriptConversion::ToObject(nextPromise, scriptContext, &nextPromiseObject))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                }
                Var thenVar = JavascriptOperators::GetProperty(nextPromiseObject, Js::PropertyIds::then, scriptContext);
                if (!JavascriptConversion::IsCallable(thenVar))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
                }

                RecyclableObject* thenFunc = UnsafeVarTo<RecyclableObject>(thenVar);

                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    CALL_FUNCTION(scriptContext->GetThreadContext(),
                        thenFunc, Js::CallInfo(CallFlags_Value, 3),
                        nextPromiseObject,
                        promiseCapability->GetResolve(),
                        rejectElement);
                }
                END_SAFE_REENTRANT_CALL;

                // s.Increase index by 1.
                index++;
            });

            // 7.d. If next is false, then
            // 7.d.i. Set iteratorRecord.[[Done]] to true.
            // 7.d.ii. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
            remainingElementsWrapper->remainingElements--;
            // 7.d.iii. If remainingElementsCount.[[Value]] is 0, then
            if (remainingElementsWrapper->remainingElements == 0)
            {
                // 7.d.iii.1 Let error be a newly created AggregateError object.
                JavascriptError* pError = library->CreateAggregateError();
                JavascriptError::SetErrorsList(pError, errors, scriptContext);
                JavascriptError::SetErrorMessage(pError, JSERR_PromiseAllRejected, _u(""), scriptContext);
                JavascriptExceptionOperators::Throw(pError, scriptContext);
            }
        }
        catch (const JavascriptException& err)
        {
            // 8. If result is an abrupt completion, then
            //   a. If iteratorRecord.[[Done]] is false, set result to IteratorClose(iteratorRecord, result).
            //   b. IfAbruptRejectPromise(result, promiseCapability).
            JavascriptExceptionObject* exception = err.GetAndClear();
            TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        return promiseCapability->GetPromise();
    }

    Var JavascriptPromise::EntryAnyRejectElementFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();
        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var undefinedVar = library ->GetUndefined();
        Var x = args.Info.Count > 1 ? args[1] : undefinedVar;


        // 1. Let F be the active function object.
        JavascriptPromiseAnyRejectElementFunction* anyRejectElementFunction = VarTo<JavascriptPromiseAnyRejectElementFunction>(function);

        // 2. Let alreadyCalled be F. [[AlreadyCalled]].
        // 3. If alreadyCalled. [[Value]] is true, return undefined.
        if (anyRejectElementFunction->IsAlreadyCalled())
        {
            return undefinedVar;
        }

        // 4. Set alreadyCalled.[[Value]] to true.
        anyRejectElementFunction->SetAlreadyCalled(true);

        // 5. Let index be F.[[Index]].
        uint32 index = anyRejectElementFunction->GetIndex();

        // 6. Let errors be F.[[Errors]].
        JavascriptArray* errors = anyRejectElementFunction->GetValues();


        // 7. Let promiseCapability be F.[[Capability]].
        JavascriptPromiseCapability* promiseCapability = anyRejectElementFunction->GetCapabilities();

        // 9. Set errors[index] to x.
        errors->DirectSetItemAt(index, x);

        // 8. Let remainingElementsCount be F.[[RemainingElements]].
        // 10. Set remainingElementsCount.[[Value]] to remainingElementsCount.[[Value]] - 1.
        // 11. If remainingElementsCount.[[Value]] is 0, then
        if (anyRejectElementFunction->DecrementRemainingElements() == 0)
        {
            // a. Let error be a newly created AggregateError object.
            JavascriptError* pError = library->CreateAggregateError();
            // b. Perform ! DefinePropertyOrThrow(error, "errors", Property Descriptor { [[Configurable]]: true, [[Enumerable]]: false, [[Writable]]: true, [[Value]]: ! CreateArrayFromList(errors) }).
            JavascriptError::SetErrorsList(pError, errors, scriptContext);
            JavascriptError::SetErrorMessage(pError, JSERR_PromiseAllRejected, _u(""), scriptContext);

            // c. Return ? Call(promiseCapability.[[Reject]], undefined, << error >> ).
            return TryCallResolveOrRejectHandler(promiseCapability->GetReject(), pError, scriptContext);
        }

        return undefinedVar;
    }

    Var JavascriptPromise::EntryAllSettled(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.allSettled"));

        // 1. Let C be the this value.
        Var constructor = args[0];

        // 2. If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.allSettled"));
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var iterable;

        if (args.Info.Count > 1)
        {
            iterable = args[1];
        }
        else
        {
            iterable = library->GetUndefined();
        }

        // 3. Let promiseCapability be NewPromiseCapability(C).
        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(constructor, scriptContext);

        // We know that constructor is an object at this point - further, we even know that it is a constructor - because NewPromiseCapability
        // would throw otherwise. That means we can safely cast constructor into a RecyclableObject* now and avoid having to perform ToObject
        // as part of the Invoke operation performed inside the loop below.
        RecyclableObject* constructorObject = VarTo<RecyclableObject>(constructor);

        uint32 index = 0;
        JavascriptArray* values = nullptr;

        // We can't use a simple counter for the remaining element count since each Promise.all Resolve Element Function needs to know how many
        // elements are remaining when it runs and needs to update that counter for all other functions created by this call to Promise.all.
        // We can't just use a static variable, either, since this element count is only used for the Promise.all Resolve Element Functions created
        // by this call to Promise.all.
        JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper);
        remainingElementsWrapper->remainingElements = 1;

        JavascriptExceptionObject* exception = nullptr;
        try
        {
            // 4. Let iterator be GetIterator(iterable).
            RecyclableObject* iterator = JavascriptOperators::GetIterator(iterable, scriptContext);

            // Abstract operation PerformPromiseAllSettled
            Var resolveVar = JavascriptOperators::GetProperty(constructorObject, Js::PropertyIds::resolve, scriptContext);
            if (!JavascriptConversion::IsCallable(resolveVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }

            RecyclableObject* resolveFunc = VarTo<RecyclableObject>(resolveVar);
            values = library->CreateArray(0);

            JavascriptOperators::DoIteratorStepAndValue(iterator, scriptContext, [&](Var next)
            {
                ThreadContext* threadContext = scriptContext->GetThreadContext();
                Var nextPromise = nullptr;
                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    nextPromise = CALL_FUNCTION(threadContext,
                        resolveFunc, Js::CallInfo(CallFlags_Value, 2),
                        constructorObject,
                        next);
                }
                END_SAFE_REENTRANT_CALL

                JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
                alreadyCalledWrapper->alreadyResolved = false;
                Var resolveElement = library->CreatePromiseAllSettledResolveOrRejectElementFunction(EntryAllSettledResolveOrRejectElementFunction, index, values, promiseCapability, remainingElementsWrapper, alreadyCalledWrapper, false);
                Var rejectElement = library->CreatePromiseAllSettledResolveOrRejectElementFunction(EntryAllSettledResolveOrRejectElementFunction, index, values, promiseCapability, remainingElementsWrapper, alreadyCalledWrapper, true);

                remainingElementsWrapper->remainingElements++;

                RecyclableObject* nextPromiseObject;

                if (!JavascriptConversion::ToObject(nextPromise, scriptContext, &nextPromiseObject))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                }

                Var thenVar = JavascriptOperators::GetProperty(nextPromiseObject, Js::PropertyIds::then, scriptContext);

                if (!JavascriptConversion::IsCallable(thenVar))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
                }

                RecyclableObject* thenFunc = VarTo<RecyclableObject>(thenVar);

                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    CALL_FUNCTION(scriptContext->GetThreadContext(),
                        thenFunc, Js::CallInfo(CallFlags_Value, 3),
                        nextPromiseObject,
                        resolveElement,
                        rejectElement);
                }
                END_SAFE_REENTRANT_CALL

                index++;
            });

            remainingElementsWrapper->remainingElements--;
            if (remainingElementsWrapper->remainingElements == 0)
            {
                Assert(values != nullptr);

                TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), values, scriptContext);
            }
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        return promiseCapability->GetPromise();
    }

    // Promise.prototype.catch as defined in ES 2015 Section 25.4.5.1
    Var JavascriptPromise::EntryCatch(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.prototype.catch"));

        RecyclableObject* promise;

        if (!JavascriptConversion::ToObject(args[0], scriptContext, &promise))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.prototype.catch"));
        }

        Var funcVar = JavascriptOperators::GetProperty(promise, Js::PropertyIds::then, scriptContext);

        if (!JavascriptConversion::IsCallable(funcVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Promise.prototype.catch"));
        }

        Var onRejected;
        RecyclableObject* undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        if (args.Info.Count > 1)
        {
            onRejected = args[1];
        }
        else
        {
            onRejected = undefinedVar;
        }

        RecyclableObject* func = VarTo<RecyclableObject>(funcVar);

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(),
                    func, Js::CallInfo(CallFlags_Value, 3),
                    promise,
                    undefinedVar,
                    onRejected);
        }
        END_SAFE_REENTRANT_CALL
    }

    // Promise.race as described in ES 2015 Section 25.4.4.3
    Var JavascriptPromise::EntryRace(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.race"));

        // 1. Let C be the this value.
        Var constructor = args[0];

        // 2. If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.race"));
        }

        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        Var iterable;

        if (args.Info.Count > 1)
        {
            iterable = args[1];
        }
        else
        {
            iterable = undefinedVar;
        }

        // 3. Let promiseCapability be NewPromiseCapability(C).
        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(constructor, scriptContext);

        // We know that constructor is an object at this point - further, we even know that it is a constructor - because NewPromiseCapability
        // would throw otherwise. That means we can safely cast constructor into a RecyclableObject* now and avoid having to perform ToObject
        // as part of the Invoke operation performed inside the loop below.
        RecyclableObject* constructorObject = VarTo<RecyclableObject>(constructor);
        JavascriptExceptionObject* exception = nullptr;

        try
        {
            // 4. Let iterator be GetIterator(iterable).
            RecyclableObject* iterator = JavascriptOperators::GetIterator(iterable, scriptContext);

            Var resolveVar = JavascriptOperators::GetProperty(constructorObject, Js::PropertyIds::resolve, scriptContext);
            if (!JavascriptConversion::IsCallable(resolveVar))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
            }

            RecyclableObject* resolveFunc = VarTo<RecyclableObject>(resolveVar);

            JavascriptOperators::DoIteratorStepAndValue(iterator, scriptContext, [&](Var next)
            {
                ThreadContext * threadContext = scriptContext->GetThreadContext();
                Var nextPromise = nullptr;
                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    nextPromise = CALL_FUNCTION(threadContext,
                                    resolveFunc, Js::CallInfo(CallFlags_Value, 2),
                                    constructorObject,
                                    next);
                }
                END_SAFE_REENTRANT_CALL

                RecyclableObject* nextPromiseObject;

                if (!JavascriptConversion::ToObject(nextPromise, scriptContext, &nextPromiseObject))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
                }

                Var thenVar = JavascriptOperators::GetProperty(nextPromiseObject, Js::PropertyIds::then, scriptContext);

                if (!JavascriptConversion::IsCallable(thenVar))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
                }

                RecyclableObject* thenFunc = VarTo<RecyclableObject>(thenVar);

                BEGIN_SAFE_REENTRANT_CALL(threadContext)
                {
                    CALL_FUNCTION(threadContext,
                        thenFunc, Js::CallInfo(CallFlags_Value, 3),
                        nextPromiseObject,
                        promiseCapability->GetResolve(),
                        promiseCapability->GetReject());
                }
                END_SAFE_REENTRANT_CALL

            });
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        return promiseCapability->GetPromise();
    }

    // Promise.reject as described in ES 2015 Section 25.4.4.4
    Var JavascriptPromise::EntryReject(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.reject"));

        // 1. Let C be the this value.
        Var constructor = args[0];

        // 2. If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.reject"));
        }

        Var r;

        if (args.Info.Count > 1)
        {
            r = args[1];
        }
        else
        {
            r = scriptContext->GetLibrary()->GetUndefined();
        }

        // 3. Let promiseCapability be NewPromiseCapability(C).
        // 4. Perform ? Call(promiseCapability.[[Reject]], undefined, << r >>).
        // 5. Return promiseCapability.[[Promise]].
        return CreateRejectedPromise(r, scriptContext, constructor);
    }

    // Promise.resolve as described in ES 2015 Section 25.4.4.5
    Var JavascriptPromise::EntryResolve(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.resolve"));

        Var x;

        // 1. Let C be the this value.
        Var constructor = args[0];

        // 2. If Type(C) is not Object, throw a TypeError exception.
        if (!JavascriptOperators::IsObject(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.resolve"));
        }

        if (args.Info.Count > 1)
        {
            x = args[1];
        }
        else
        {
            x = scriptContext->GetLibrary()->GetUndefined();
        }

        return PromiseResolve(constructor, x, scriptContext);
    }

    JavascriptPromise* JavascriptPromise::InternalPromiseResolve(Var value, ScriptContext* scriptContext)
    {
        Var constructor = scriptContext->GetLibrary()->GetPromiseConstructor();
        Var promise = PromiseResolve(constructor, value, scriptContext);
        return UnsafeVarTo<JavascriptPromise>(promise);
    }

    Var JavascriptPromise::PromiseResolve(Var constructor, Var value, ScriptContext* scriptContext)
    {
        if (VarIs<JavascriptPromise>(value))
        {
            Var valueConstructor = JavascriptOperators::GetProperty(
                (RecyclableObject*)value,
                PropertyIds::constructor,
                scriptContext);

            // If `value` is a Promise or Promise subclass instance and its "constructor"
            // property is `constructor`, then return the value unchanged
            if (JavascriptConversion::SameValue(valueConstructor, constructor))
            {
                return value;
            }
        }

        return CreateResolvedPromise(value, scriptContext, constructor);
    }

    // Promise.prototype.then as described in ES 2015 Section 25.4.5.3
    Var JavascriptPromise::EntryThen(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.prototype.then"));

        if (args.Info.Count < 1 || !VarIs<JavascriptPromise>(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedPromise, _u("Promise.prototype.then"));
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptPromise* promise = VarTo<JavascriptPromise>(args[0]);
        RecyclableObject* rejectionHandler;
        RecyclableObject* fulfillmentHandler;

        if (args.Info.Count > 1 && JavascriptConversion::IsCallable(args[1]))
        {
            fulfillmentHandler = VarTo<RecyclableObject>(args[1]);
        }
        else
        {
            fulfillmentHandler = library->GetIdentityFunction();
        }

        if (args.Info.Count > 2 && JavascriptConversion::IsCallable(args[2]))
        {
            rejectionHandler = VarTo<RecyclableObject>(args[2]);
        }
        else
        {
            rejectionHandler = library->GetThrowerFunction();
        }

        return CreateThenPromise(promise, fulfillmentHandler, rejectionHandler, scriptContext);
    }

    // Promise.prototype.finally as described in the draft ES 2018 #sec-promise.prototype.finally
    Var JavascriptPromise::EntryFinally(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Promise.prototype.finally"));
        // 1. Let promise be the this value
        // 2. If Type(promise) is not Object, throw a TypeError exception
        if (args.Info.Count < 1 || !JavascriptOperators::IsObject(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Promise.prototype.finally"));
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        RecyclableObject* promise = UnsafeVarTo<RecyclableObject>(args[0]);
        // 3. Let C be ? SpeciesConstructor(promise, %Promise%).
        RecyclableObject* constructor = JavascriptOperators::SpeciesConstructor(promise, scriptContext->GetLibrary()->GetPromiseConstructor(), scriptContext);
        // 4. Assert IsConstructor(C)
        Assert(JavascriptOperators::IsConstructor(constructor));

        // 5. If IsCallable(onFinally) is false
        //   a. Let thenFinally be onFinally
        //   b. Let catchFinally be onFinally
        // 6. Else,
        //   a. Let thenFinally be a new built-in function object as defined in ThenFinally Function.
        //   b. Let catchFinally be a new built-in function object as defined in CatchFinally Function.
        //   c. Set thenFinally and catchFinally's [[Constructor]] internal slots to C.
        //   d. Set thenFinally and catchFinally's [[OnFinally]] internal slots to onFinally.

        Var thenFinally = nullptr;
        Var catchFinally = nullptr;

        if (args.Info.Count > 1)
        {
            if (JavascriptConversion::IsCallable(args[1]))
            {
                //note to avoid duplicating code the ThenFinallyFunction works as both thenFinally and catchFinally using a flag
                thenFinally = library->CreatePromiseThenFinallyFunction(EntryThenFinallyFunction, VarTo<RecyclableObject>(args[1]), constructor, false);
                catchFinally = library->CreatePromiseThenFinallyFunction(EntryThenFinallyFunction, VarTo<RecyclableObject>(args[1]), constructor, true);
            }
            else
            {
                thenFinally = args[1];
                catchFinally = args[1];
            }
        }
        else
        {
            thenFinally = library->GetUndefined();
            catchFinally = library->GetUndefined();
        }

        Assert(thenFinally != nullptr && catchFinally != nullptr);

        // 7. Return ? Invoke(promise, "then", << thenFinally, catchFinally >>).
        Var funcVar = JavascriptOperators::GetProperty(promise, Js::PropertyIds::then, scriptContext);
        if (!JavascriptConversion::IsCallable(funcVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Promise.prototype.finally"));
        }
        RecyclableObject* func = UnsafeVarTo<RecyclableObject>(funcVar);

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(),
                func, Js::CallInfo(CallFlags_Value, 3),
                promise,
                thenFinally,
                catchFinally);
        }
        END_SAFE_REENTRANT_CALL
    }

    // ThenFinallyFunction as described in draft ES2018 #sec-thenfinallyfunctions
    // AND CatchFinallyFunction as described in draft ES2018 #sec-catchfinallyfunctions
    Var JavascriptPromise::EntryThenFinallyFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));
        ScriptContext* scriptContext = function->GetScriptContext();

        JavascriptLibrary* library = scriptContext->GetLibrary();

        JavascriptPromiseThenFinallyFunction* thenFinallyFunction = VarTo<JavascriptPromiseThenFinallyFunction>(function);

        // 1. Let onFinally be F.[[OnFinally]]
        // 2. Assert: IsCallable(onFinally)=true
        Assert(JavascriptConversion::IsCallable(thenFinallyFunction->GetOnFinally()));

        // 3. Let result be ? Call(onFinally, undefined)
        Var result = nullptr;

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            result = CALL_FUNCTION(scriptContext->GetThreadContext(), thenFinallyFunction->GetOnFinally(), CallInfo(CallFlags_Value, 1), library->GetUndefined());
        }
        END_SAFE_REENTRANT_CALL

        Assert(result);

        // 4. Let C be F.[[Constructor]]
        // 5. Assert IsConstructor(C)
        Assert(JavascriptOperators::IsConstructor(thenFinallyFunction->GetConstructor()));

        // 6. Let promise be ? PromiseResolve(c, result)
        Var promiseVar = CreateResolvedPromise(result, scriptContext, thenFinallyFunction->GetConstructor());

        // 7. Let valueThunk be equivalent to a function that returns value
        // OR 7. Let thrower be equivalent to a function that throws reason

        Var valueOrReason = nullptr;

        if (args.Info.Count > 1)
        {
            valueOrReason = args[1];
        }
        else
        {
            valueOrReason = scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptPromiseThunkFinallyFunction* thunkFinallyFunction = library->CreatePromiseThunkFinallyFunction(EntryThunkFinallyFunction, valueOrReason, thenFinallyFunction->GetShouldThrow());

        // 8. Return ? Invoke(promise, "then", <<valueThunk>>)
        RecyclableObject* promise = JavascriptOperators::ToObject(promiseVar, scriptContext);
        Var funcVar = JavascriptOperators::GetProperty(promise, Js::PropertyIds::then, scriptContext);

        if (!JavascriptConversion::IsCallable(funcVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Promise.prototype.finally"));
        }

        RecyclableObject* func = VarTo<RecyclableObject>(funcVar);

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(),
                func, Js::CallInfo(CallFlags_Value, 2),
                promiseVar,
                thunkFinallyFunction);
        }
        END_SAFE_REENTRANT_CALL
    }

    // valueThunk Function as referenced within draft ES2018 #sec-thenfinallyfunctions
    // and thrower as referenced within draft ES2018 #sec-catchfinallyfunctions
    Var JavascriptPromise::EntryThunkFinallyFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        JavascriptPromiseThunkFinallyFunction* thunkFinallyFunction = VarTo<JavascriptPromiseThunkFinallyFunction>(function);

        if (!thunkFinallyFunction->GetShouldThrow())
        {
            return thunkFinallyFunction->GetValue();
        }
        else
        {
            JavascriptExceptionOperators::Throw(thunkFinallyFunction->GetValue(), function->GetScriptContext());
        }
    }

    // Promise Reject and Resolve Functions as described in ES 2015 Section 25.4.1.4.1 and 25.4.1.4.2
    Var JavascriptPromise::EntryResolveOrRejectFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var undefinedVar = library->GetUndefined();
        Var resolution;

        if (args.Info.Count > 1)
        {
            resolution = args[1];
        }
        else
        {
            resolution = undefinedVar;
        }

        JavascriptPromiseResolveOrRejectFunction* resolveOrRejectFunction = VarTo<JavascriptPromiseResolveOrRejectFunction>(function);

        if (resolveOrRejectFunction->IsAlreadyResolved())
        {
            return undefinedVar;
        }

        resolveOrRejectFunction->SetAlreadyResolved(true);

        bool rejecting = resolveOrRejectFunction->IsRejectFunction();

        JavascriptPromise* promise = resolveOrRejectFunction->GetPromise();

        return promise->ResolveHelper(resolution, rejecting, scriptContext);
    }

    Var JavascriptPromise::Resolve(Var resolution, ScriptContext* scriptContext)
    {
        return this->ResolveHelper(resolution, false, scriptContext);
    }

    Var JavascriptPromise::Reject(Var resolution, ScriptContext* scriptContext)
    {
        return this->ResolveHelper(resolution, true, scriptContext);
    }

    Var JavascriptPromise::ResolveHelper(Var resolution, bool isRejecting, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var undefinedVar = library->GetUndefined();

        // We only need to check SameValue and check for thenable resolution in the Resolve function case (not Reject)
        if (!isRejecting)
        {
            if (JavascriptConversion::SameValue(resolution, this))
            {
                JavascriptError* selfResolutionError = scriptContext->GetLibrary()->CreateTypeError();
                JavascriptError::SetErrorMessage(selfResolutionError, JSERR_PromiseSelfResolution, _u(""), scriptContext);

                resolution = selfResolutionError;
                isRejecting = true;
            }
            else if (VarIs<RecyclableObject>(resolution))
            {
                try
                {
                    RecyclableObject* thenable = VarTo<RecyclableObject>(resolution);
                    Var then = JavascriptOperators::GetPropertyNoCache(thenable, Js::PropertyIds::then, scriptContext);

                    if (JavascriptConversion::IsCallable(then))
                    {
                        JavascriptPromiseResolveThenableTaskFunction* resolveThenableTaskFunction = library->CreatePromiseResolveThenableTaskFunction(EntryResolveThenableTaskFunction, this, thenable, VarTo<RecyclableObject>(then));

                        library->EnqueueTask(resolveThenableTaskFunction);

                        return undefinedVar;
                    }
                }
                catch (const JavascriptException& err)
                {
                    resolution = err.GetAndClear()->GetThrownObject(scriptContext);

                    if (resolution == nullptr)
                    {
                        resolution = undefinedVar;
                    }

                    isRejecting = true;
                }
            }
        }


        PromiseStatus newStatus;

        // Need to check rejecting state again as it might have changed due to failures
        if (isRejecting)
        {
            newStatus = PromiseStatusCode_HasRejection;
            if (!GetIsHandled())
            {
                scriptContext->GetLibrary()->CallNativeHostPromiseRejectionTracker(this, resolution, false);
            }
        }
        else
        {
            newStatus = PromiseStatusCode_HasResolution;
        }

        Assert(resolution != nullptr);

        // SList only supports "prepend" operation, so we need to reverse the list
        // before triggering reactions
        JavascriptPromiseReactionList* reactions = this->GetReactions();
        if (reactions != nullptr)
        {
            reactions->Reverse();
        }

        this->result = resolution;
        this->reactions = nullptr;
        this->SetStatus(newStatus);

        return TriggerPromiseReactions(reactions, isRejecting, resolution, scriptContext);
    }

    // Promise Capabilities Executor Function as described in ES 2015 Section 25.4.1.6.2
    Var JavascriptPromise::EntryCapabilitiesExecutorFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        Var resolve = undefinedVar;
        Var reject = undefinedVar;

        if (args.Info.Count > 1)
        {
            resolve = args[1];

            if (args.Info.Count > 2)
            {
                reject = args[2];
            }
        }

        JavascriptPromiseCapabilitiesExecutorFunction* capabilitiesExecutorFunction = VarTo<JavascriptPromiseCapabilitiesExecutorFunction>(function);
        JavascriptPromiseCapability* promiseCapability = capabilitiesExecutorFunction->GetCapability();

        if (!JavascriptOperators::IsUndefined(promiseCapability->GetResolve()) || !JavascriptOperators::IsUndefined(promiseCapability->GetReject()))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_UnexpectedMetadataFailure, _u("Promise"));
        }

        promiseCapability->SetResolve(resolve);
        promiseCapability->SetReject(reject);

        return undefinedVar;
    }

    // Promise Reaction Task Function as described in ES 2015 Section 25.4.2.1
    Var JavascriptPromise::EntryReactionTaskFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        JavascriptPromiseReactionTaskFunction* reactionTaskFunction = VarTo<JavascriptPromiseReactionTaskFunction>(function);
        JavascriptPromiseReaction* reaction = reactionTaskFunction->GetReaction();
        Var argument = reactionTaskFunction->GetArgument();
        JavascriptPromiseCapability* promiseCapability = reaction->GetCapabilities();
        RecyclableObject* handler = reaction->GetHandler();
        Var handlerResult = nullptr;
        JavascriptExceptionObject* exception = nullptr;

        {

            bool isPromiseRejectionHandled = true;
            if (scriptContext->IsScriptContextInDebugMode())
            {
                // only necessary to determine if false if debugger is attached.  This way we'll
                // correctly break on exceptions raised in promises that result in uhandled rejection
                // notifications
                Var promiseVar = promiseCapability->GetPromise();
                if (VarIs<JavascriptPromise>(promiseVar))
                {
                    JavascriptPromise* promise = VarTo<JavascriptPromise>(promiseVar);
                    isPromiseRejectionHandled = !promise->WillRejectionBeUnhandled();
                }
            }

            Js::JavascriptExceptionOperators::AutoCatchHandlerExists autoCatchHandlerExists(scriptContext, isPromiseRejectionHandled);
            try
            {
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    handlerResult = CALL_FUNCTION(scriptContext->GetThreadContext(),
                                        handler, Js::CallInfo(Js::CallFlags::CallFlags_Value, 2),
                                        undefinedVar,
                                        argument);
                }
                END_SAFE_REENTRANT_CALL
            }
            catch (const JavascriptException& err)
            {
                exception = err.GetAndClear();
            }
        }

        if (exception != nullptr)
        {
            return TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        Assert(handlerResult != nullptr);

        return TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), handlerResult, scriptContext);
    }


    /**
     * Determine if at the current point in time, the given promise has a path of reactions that result
     * in an unhandled rejection.  This doesn't account for potential of a rejection handler added later
     * in time.
     */
    bool JavascriptPromise::WillRejectionBeUnhandled()
    {
        bool willBeUnhandled = !this->GetIsHandled();
        if (!willBeUnhandled)
        {
            // if this promise is handled, then we need to do a depth-first search over this promise's reject
            // reactions. If we find a reaction that
            //    - associated promise is "unhandled" (ie, it's never been "then'd")
            //    - AND its rejection handler is our default "thrower function"
            // then this promise results in an unhandled rejection path.

            JsUtil::Stack<JavascriptPromise*, HeapAllocator> stack(&HeapAllocator::Instance);
            SimpleHashTable<JavascriptPromise*, int, HeapAllocator> visited(&HeapAllocator::Instance);
            stack.Push(this);
            visited.Add(this, 1);

            while (!willBeUnhandled && !stack.Empty())
            {
                JavascriptPromise * curr = stack.Pop();
                {
                    JavascriptPromiseReactionList* reactions = curr->GetReactions();
                    JavascriptPromiseReactionList::Iterator it = reactions->GetIterator();
                    while (it.Next())
                    {
                        JavascriptPromiseReactionPair pair = it.Data();
                        JavascriptPromiseReaction* reaction = pair.rejectReaction;
                        Var promiseVar = reaction->GetCapabilities()->GetPromise();

                        if (VarIs<JavascriptPromise>(promiseVar))
                        {
                            JavascriptPromise* p = VarTo<JavascriptPromise>(promiseVar);
                            if (!p->GetIsHandled())
                            {
                                RecyclableObject* handler = reaction->GetHandler();
                                if (VarIs<JavascriptFunction>(handler))
                                {
                                    JavascriptFunction* func = VarTo<JavascriptFunction>(handler);
                                    FunctionInfo* functionInfo = func->GetFunctionInfo();

#ifdef DEBUG
                                    if (!func->IsCrossSiteObject())
                                    {
                                        // assert that Thrower function's FunctionInfo hasn't changed
                                        AssertMsg(func->GetScriptContext()->GetLibrary()->GetThrowerFunction()->GetFunctionInfo() == &JavascriptPromise::EntryInfo::Thrower, "unexpected FunctionInfo for thrower function!");
                                    }
#endif

                                    // If the function info is the default thrower function's function info, then assume that this is unhandled
                                    // this will work across script contexts
                                    if (functionInfo == &JavascriptPromise::EntryInfo::Thrower)
                                    {
                                        willBeUnhandled = true;
                                        break;
                                    }
                                }
                            }
                            AssertMsg(visited.HasEntry(p) == false, "Unexpected cycle in promise reaction tree!");
                            if (!visited.HasEntry(p))
                            {
                                stack.Push(p);
                                visited.Add(p, 1);
                            }
                        }
                    }
                }
            }
        }
        return willBeUnhandled;
    }

    Var JavascriptPromise::TryCallResolveOrRejectHandler(Var handler, Var value, ScriptContext* scriptContext)
    {
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();

        if (!JavascriptConversion::IsCallable(handler))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }

        RecyclableObject* handlerFunc = VarTo<RecyclableObject>(handler);

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CALL_FUNCTION(scriptContext->GetThreadContext(),
                    handlerFunc, CallInfo(CallFlags_Value, 2),
                    undefinedVar,
                    value);
        }
        END_SAFE_REENTRANT_CALL
    }

    Var JavascriptPromise::TryRejectWithExceptionObject(JavascriptExceptionObject* exceptionObject, Var handler, ScriptContext* scriptContext)
    {
        Var thrownObject = exceptionObject->GetThrownObject(scriptContext);

        if (thrownObject == nullptr)
        {
            thrownObject = scriptContext->GetLibrary()->GetUndefined();
        }

        return TryCallResolveOrRejectHandler(handler, thrownObject, scriptContext);
    }

    Var JavascriptPromise::CreateRejectedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor)
    {
        if (promiseConstructor == nullptr)
        {
            promiseConstructor = scriptContext->GetLibrary()->GetPromiseConstructor();
        }

        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(promiseConstructor, scriptContext);

        TryCallResolveOrRejectHandler(promiseCapability->GetReject(), resolution, scriptContext);

        return promiseCapability->GetPromise();
    }

    Var JavascriptPromise::CreateResolvedPromise(Var resolution, ScriptContext* scriptContext, Var promiseConstructor)
    {
        if (promiseConstructor == nullptr)
        {
            promiseConstructor = scriptContext->GetLibrary()->GetPromiseConstructor();
        }

        JavascriptPromiseCapability* promiseCapability = NewPromiseCapability(promiseConstructor, scriptContext);

        TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), resolution, scriptContext);

        return promiseCapability->GetPromise();
    }

    Var JavascriptPromise::CreatePassThroughPromise(JavascriptPromise* sourcePromise, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();

        return CreateThenPromise(sourcePromise, library->GetIdentityFunction(), library->GetThrowerFunction(), scriptContext);
    }

    Var JavascriptPromise::CreateThenPromise(JavascriptPromise* sourcePromise, RecyclableObject* fulfillmentHandler, RecyclableObject* rejectionHandler, ScriptContext* scriptContext)
    {
        JavascriptFunction* defaultConstructor = scriptContext->GetLibrary()->GetPromiseConstructor();
        RecyclableObject* constructor = JavascriptOperators::SpeciesConstructor(sourcePromise, defaultConstructor, scriptContext);
        AssertOrFailFast(JavascriptOperators::IsConstructor(constructor));

        bool isDefaultConstructor = constructor == defaultConstructor;
        JavascriptPromiseCapability* promiseCapability = (JavascriptPromiseCapability*)JavascriptOperators::NewObjectCreationHelper_ReentrancySafe(constructor, isDefaultConstructor, scriptContext->GetThreadContext(), [=]()->JavascriptPromiseCapability*
        {
            return NewPromiseCapability(constructor, scriptContext);
        });

        PerformPromiseThen(sourcePromise, promiseCapability, fulfillmentHandler, rejectionHandler, scriptContext);

        return promiseCapability->GetPromise();
    }

    void JavascriptPromise::PerformPromiseThen(
        JavascriptPromise* sourcePromise,
        JavascriptPromiseCapability* capability,
        RecyclableObject* fulfillmentHandler,
        RecyclableObject* rejectionHandler,
        ScriptContext* scriptContext)
    {
        auto* resolveReaction = JavascriptPromiseReaction::New(capability, fulfillmentHandler, scriptContext);
        auto* rejectReaction = JavascriptPromiseReaction::New(capability, rejectionHandler, scriptContext);

        switch (sourcePromise->GetStatus())
        {
        case PromiseStatusCode_Unresolved:
            JavascriptPromiseReactionPair pair;
            pair.resolveReaction = resolveReaction;
            pair.rejectReaction = rejectReaction;
            sourcePromise->reactions->Prepend(pair);
            break;

        case PromiseStatusCode_HasResolution:
            EnqueuePromiseReactionTask(
                resolveReaction, 
                CrossSite::MarshalVar(scriptContext, sourcePromise->result),
                scriptContext);
            break;

        case PromiseStatusCode_HasRejection:
        {
            if (!sourcePromise->GetIsHandled())
            {
                scriptContext->GetLibrary()->CallNativeHostPromiseRejectionTracker(
                    sourcePromise,
                    CrossSite::MarshalVar(scriptContext, sourcePromise->result),
                    true);
            }
            EnqueuePromiseReactionTask(
                rejectReaction,
                CrossSite::MarshalVar(scriptContext, sourcePromise->result),
                scriptContext);
            break;
        }

        default:
            AssertMsg(false, "Promise status is in an invalid state");
            break;
        }

        sourcePromise->SetIsHandled();
    }

    // Promise Resolve Thenable Job as described in ES 2015 Section 25.4.2.2
    Var JavascriptPromise::EntryResolveThenableTaskFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        JavascriptPromiseResolveThenableTaskFunction* resolveThenableTaskFunction = VarTo<JavascriptPromiseResolveThenableTaskFunction>(function);
        JavascriptPromise* promise = resolveThenableTaskFunction->GetPromise();
        RecyclableObject* thenable = resolveThenableTaskFunction->GetThenable();
        RecyclableObject* thenFunction = resolveThenableTaskFunction->GetThenFunction();

        JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord = RecyclerNewStructZ(scriptContext->GetRecycler(), JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper);
        alreadyResolvedRecord->alreadyResolved = false;

        JavascriptPromiseResolveOrRejectFunction* resolve = library->CreatePromiseResolveOrRejectFunction(EntryResolveOrRejectFunction, promise, false, alreadyResolvedRecord);
        JavascriptPromiseResolveOrRejectFunction* reject = library->CreatePromiseResolveOrRejectFunction(EntryResolveOrRejectFunction, promise, true, alreadyResolvedRecord);
        JavascriptExceptionObject* exception = nullptr;

        {
            bool isPromiseRejectionHandled = true;
            if (scriptContext->IsScriptContextInDebugMode())
            {
                // only necessary to determine if false if debugger is attached.  This way we'll
                // correctly break on exceptions raised in promises that result in uhandled rejections
                isPromiseRejectionHandled = !promise->WillRejectionBeUnhandled();
            }

            Js::JavascriptExceptionOperators::AutoCatchHandlerExists autoCatchHandlerExists(scriptContext, isPromiseRejectionHandled);
            try
            {
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    return CALL_FUNCTION(scriptContext->GetThreadContext(),
                            thenFunction, Js::CallInfo(Js::CallFlags::CallFlags_Value, 3),
                            thenable,
                            resolve,
                            reject);
                }
                END_SAFE_REENTRANT_CALL
            }
            catch (const JavascriptException& err)
            {
                exception = err.GetAndClear();
            }
        }

        Assert(exception != nullptr);

        return TryRejectWithExceptionObject(exception, reject, scriptContext);
    }

    // Promise Identity Function as described in ES 2015Section 25.4.5.3.1
    Var JavascriptPromise::EntryIdentityFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count > 1)
        {
            Assert(args[1] != nullptr);

            return args[1];
        }
        else
        {
            return function->GetScriptContext()->GetLibrary()->GetUndefined();
        }
    }

    // Promise Thrower Function as described in ES 2015Section 25.4.5.3.3
    Var JavascriptPromise::EntryThrowerFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var arg;

        if (args.Info.Count > 1)
        {
            Assert(args[1] != nullptr);

            arg = args[1];
        }
        else
        {
            arg = scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptExceptionOperators::Throw(arg, scriptContext);
    }

    // Promise.all Resolve Element Function as described in ES6.0 (Release Candidate 3) Section 25.4.4.1.2
    Var JavascriptPromise::EntryAllResolveElementFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        Var x;

        if (args.Info.Count > 1)
        {
            x = args[1];
        }
        else
        {
            x = undefinedVar;
        }

        JavascriptPromiseAllResolveElementFunction* allResolveElementFunction = VarTo<JavascriptPromiseAllResolveElementFunction>(function);

        if (allResolveElementFunction->IsAlreadyCalled())
        {
            return undefinedVar;
        }

        allResolveElementFunction->SetAlreadyCalled(true);

        uint32 index = allResolveElementFunction->GetIndex();
        JavascriptArray* values = allResolveElementFunction->GetValues();
        JavascriptPromiseCapability* promiseCapability = allResolveElementFunction->GetCapabilities();
        JavascriptExceptionObject* exception = nullptr;

        try
        {
            values->SetItem(index, x, PropertyOperation_None);
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            return TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        if (allResolveElementFunction->DecrementRemainingElements() == 0)
        {
            return TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), values, scriptContext);
        }

        return undefinedVar;
    }

    Var JavascriptPromise::EntryAllSettledResolveOrRejectElementFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var undefinedVar = scriptContext->GetLibrary()->GetUndefined();
        Var x;

        if (args.Info.Count > 1)
        {
            x = args[1];
        }
        else
        {
            x = undefinedVar;
        }

        JavascriptPromiseAllSettledResolveOrRejectElementFunction* allSettledResolveElementFunction = VarTo<JavascriptPromiseAllSettledResolveOrRejectElementFunction>(function);

        if (allSettledResolveElementFunction->IsAlreadyCalled())
        {
            return undefinedVar;
        }

        allSettledResolveElementFunction->SetAlreadyCalled(true);

        bool isRejecting = allSettledResolveElementFunction->IsRejectFunction();
        uint32 index = allSettledResolveElementFunction->GetIndex();
        JavascriptArray* values = allSettledResolveElementFunction->GetValues();
        JavascriptPromiseCapability* promiseCapability = allSettledResolveElementFunction->GetCapabilities();
        JavascriptExceptionObject* exception = nullptr;

        try
        {
            RecyclableObject* obj = scriptContext->GetLibrary()->CreateObject();
            Var statusString = isRejecting ?
                scriptContext->GetPropertyString(PropertyIds::rejected) :
                scriptContext->GetPropertyString(PropertyIds::fulfilled);
            JavascriptOperators::SetProperty(obj, obj, PropertyIds::status, statusString, scriptContext);
            PropertyIds valuePropId = isRejecting ? PropertyIds::reason : PropertyIds::value;
            JavascriptOperators::SetProperty(obj, obj, valuePropId, x, scriptContext);

            values->SetItem(index, obj, PropertyOperation_None);
        }
        catch (const JavascriptException& err)
        {
            exception = err.GetAndClear();
        }

        if (exception != nullptr)
        {
            return TryRejectWithExceptionObject(exception, promiseCapability->GetReject(), scriptContext);
        }

        if (allSettledResolveElementFunction->DecrementRemainingElements() == 0)
        {
            return TryCallResolveOrRejectHandler(promiseCapability->GetResolve(), values, scriptContext);
        }

        return undefinedVar;
    }

#if ENABLE_TTD
    void JavascriptPromise::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        if(this->result != nullptr)
        {
            extractor->MarkVisitVar(this->result);
        }

        if(this->reactions != nullptr)
        {
            this->reactions->Map([&](JavascriptPromiseReactionPair pair) {
                pair.rejectReaction->MarkVisitPtrs(extractor);
                pair.resolveReaction->MarkVisitPtrs(extractor);
            });
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromise::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseObject;
    }

    void JavascriptPromise::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        JsUtil::List<TTD_PTR_ID, HeapAllocator> depOnList(&HeapAllocator::Instance);

        TTD::NSSnapObjects::SnapPromiseInfo* spi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseInfo>();

        spi->Result = this->result;

        //Primitive kinds always inflated first so we only need to deal with complex kinds as depends on
        if(this->result != nullptr && TTD::JsSupport::IsVarComplexKind(this->result))
        {
            depOnList.Add(TTD_CONVERT_VAR_TO_PTR_ID(this->result));
        }

        spi->Status = this->GetStatus();
        spi->isHandled = this->GetIsHandled();

        // get count of # of reactions
        spi->ResolveReactionCount = 0;
        if (this->reactions != nullptr)
        {
            this->reactions->Map([&spi](JavascriptPromiseReactionPair pair) {
                spi->ResolveReactionCount++;
            });
        }
        spi->RejectReactionCount = spi->ResolveReactionCount;

        // move resolve & reject reactions into slab
        spi->ResolveReactions = nullptr;
        spi->RejectReactions = nullptr;
        if(spi->ResolveReactionCount != 0)
        {
            spi->ResolveReactions = alloc.SlabAllocateArray<TTD::NSSnapValues::SnapPromiseReactionInfo>(spi->ResolveReactionCount);
            spi->RejectReactions = alloc.SlabAllocateArray<TTD::NSSnapValues::SnapPromiseReactionInfo>(spi->RejectReactionCount);

            JavascriptPromiseReactionList::Iterator it = this->reactions->GetIterator();
            uint32 i = 0;
            while (it.Next())
            {
                it.Data().resolveReaction->ExtractSnapPromiseReactionInto(spi->ResolveReactions + i, depOnList, alloc);
                it.Data().rejectReaction->ExtractSnapPromiseReactionInto(spi->RejectReactions + i, depOnList, alloc);
                ++i;
            }
        }

        //see what we need to do wrt dependencies
        if(depOnList.Count() == 0)
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseObject>(objData, spi);
        }
        else
        {
            uint32 depOnCount = depOnList.Count();
            TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

            for(uint32 i = 0; i < depOnCount; ++i)
            {
                depOnArray[i] = depOnList.Item(i);
            }

            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseObject>(objData, spi, alloc, depOnCount, depOnArray);
        }
    }

    JavascriptPromise* JavascriptPromise::InitializePromise_TTD(ScriptContext* scriptContext, uint32 status, bool isHandled, Var result, SList<Js::JavascriptPromiseReaction*, HeapAllocator>& resolveReactions,SList<Js::JavascriptPromiseReaction*, HeapAllocator>& rejectReactions)
    {
        Recycler* recycler = scriptContext->GetRecycler();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        JavascriptPromise* promise = library->CreatePromise();

        promise->SetStatus((PromiseStatus)status);
        if (isHandled)
        {
            promise->SetIsHandled();
        }
        promise->result = result;

        promise->reactions = RecyclerNew(recycler, JavascriptPromiseReactionList, recycler);
        SList<Js::JavascriptPromiseReaction*, HeapAllocator>::Iterator resolveIterator = resolveReactions.GetIterator();
        SList<Js::JavascriptPromiseReaction*, HeapAllocator>::Iterator rejectIterator = rejectReactions.GetIterator();

        bool hasResolve = resolveIterator.Next();
        bool hasReject = rejectIterator.Next();
        while (hasResolve && hasReject)
        {
            JavascriptPromiseReactionPair pair;
            pair.resolveReaction = resolveIterator.Data();
            pair.rejectReaction = rejectIterator.Data();
            promise->reactions->Prepend(pair);
            hasResolve = resolveIterator.Next();
            hasReject = rejectIterator.Next();
        }
        AssertMsg(hasResolve == false && hasReject == false, "mismatched resolve/reject reaction counts");
        promise->reactions->Reverse();

        return promise;
    }
#endif

    // NewPromiseCapability as described in ES6.0 (draft 29) Section 25.4.1.6
    JavascriptPromiseCapability* JavascriptPromise::NewPromiseCapability(Var constructor, ScriptContext* scriptContext)
    {
        if (!JavascriptOperators::IsConstructor(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);
        }

        RecyclableObject* constructorFunc = VarTo<RecyclableObject>(constructor);

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CreatePromiseCapabilityRecord(constructorFunc, scriptContext);
        }
        END_SAFE_REENTRANT_CALL
    }

    JavascriptPromiseCapability* JavascriptPromise::UnusedPromiseCapability(ScriptContext* scriptContext)
    {
        // OPTIMIZE: In async functions and async generators, await resolves the operand
        // and then executes PerformPromiseThen, attaching handlers that resume the coroutine.
        // The promise capability provided is unused. For these operations we should be able
        // to eliminate the extra promise allocation.
        return NewPromiseCapability(scriptContext->GetLibrary()->GetPromiseConstructor(), scriptContext);
    }

    // CreatePromiseCapabilityRecord as described in ES6.0 (draft 29) Section 25.4.1.6.1
    JavascriptPromiseCapability* JavascriptPromise::CreatePromiseCapabilityRecord(RecyclableObject* constructor, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var undefinedVar = library->GetUndefined();
        JavascriptPromiseCapability* promiseCapability = JavascriptPromiseCapability::New(undefinedVar, undefinedVar, undefinedVar, scriptContext);

        JavascriptPromiseCapabilitiesExecutorFunction* executor = library->CreatePromiseCapabilitiesExecutorFunction(EntryCapabilitiesExecutorFunction, promiseCapability);

        CallInfo callinfo = Js::CallInfo((Js::CallFlags)(Js::CallFlags::CallFlags_Value | Js::CallFlags::CallFlags_New), 2);
        Var argVars[] = { constructor, executor };
        Arguments args(callinfo, argVars);
        Var promise = JavascriptFunction::CallAsConstructor(constructor, nullptr, args, scriptContext);

        if (!JavascriptConversion::IsCallable(promiseCapability->GetResolve()) || !JavascriptConversion::IsCallable(promiseCapability->GetReject()))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction, _u("Promise"));
        }

        promiseCapability->SetPromise(promise);

        return promiseCapability;
    }

    // TriggerPromiseReactions as defined in ES 2015 Section 25.4.1.7
    Var JavascriptPromise::TriggerPromiseReactions(JavascriptPromiseReactionList* reactions, bool isRejecting, Var resolution, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (reactions != nullptr)
        {
            JavascriptPromiseReactionList::Iterator it = reactions->GetIterator();
            while (it.Next())
            {
                JavascriptPromiseReaction* reaction;
                if (isRejecting)
                {
                    reaction = it.Data().rejectReaction;
                }
                else
                {
                    reaction = it.Data().resolveReaction;
                }

                EnqueuePromiseReactionTask(reaction, resolution, scriptContext);
            }
        }

        return library->GetUndefined();
    }

    void JavascriptPromise::EnqueuePromiseReactionTask(JavascriptPromiseReaction* reaction, Var resolution, ScriptContext* scriptContext)
    {
        Assert(resolution != nullptr);

        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptPromiseReactionTaskFunction* reactionTaskFunction = library->CreatePromiseReactionTaskFunction(EntryReactionTaskFunction, reaction, resolution);

        library->EnqueueTask(reactionTaskFunction);
    }

    JavascriptPromiseResolveOrRejectFunction::JavascriptPromiseResolveOrRejectFunction(DynamicType* type)
        : RuntimeFunction(type, &Js::JavascriptPromise::EntryInfo::ResolveOrRejectFunction), promise(nullptr), isReject(false), alreadyResolvedWrapper(nullptr)
    { }

    JavascriptPromiseResolveOrRejectFunction::JavascriptPromiseResolveOrRejectFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromise* promise, bool isReject, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyResolvedRecord)
        : RuntimeFunction(type, functionInfo), promise(promise), isReject(isReject), alreadyResolvedWrapper(alreadyResolvedRecord)
    { }

    template <> bool VarIsImpl<JavascriptPromiseResolveOrRejectFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseResolveOrRejectFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseResolveOrRejectFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptPromise* JavascriptPromiseResolveOrRejectFunction::GetPromise()
    {
        return this->promise;
    }

    bool JavascriptPromiseResolveOrRejectFunction::IsRejectFunction()
    {
        return this->isReject;
    }

    bool JavascriptPromiseResolveOrRejectFunction::IsAlreadyResolved()
    {
        Assert(this->alreadyResolvedWrapper);

        return this->alreadyResolvedWrapper->alreadyResolved;
    }

    void JavascriptPromiseResolveOrRejectFunction::SetAlreadyResolved(bool is)
    {
        Assert(this->alreadyResolvedWrapper);

        this->alreadyResolvedWrapper->alreadyResolved = is;
    }

#if ENABLE_TTD
    void JavascriptPromiseResolveOrRejectFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->promise != nullptr, "Was not expecting that!!!");

        extractor->MarkVisitVar(this->promise);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseResolveOrRejectFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject;
    }

    void JavascriptPromiseResolveOrRejectFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo* sprri = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo>();

        uint32 depOnCount = 1;
        TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        sprri->PromiseId = TTD_CONVERT_VAR_TO_PTR_ID(this->promise);
        depOnArray[0] = sprri->PromiseId;

        sprri->IsReject = this->isReject;

        sprri->AlreadyResolvedWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->alreadyResolvedWrapper);
        sprri->AlreadyResolvedValue = this->alreadyResolvedWrapper->alreadyResolved;

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(objData, sprri, alloc, depOnCount, depOnArray);
    }
#endif

    JavascriptPromiseCapabilitiesExecutorFunction::JavascriptPromiseCapabilitiesExecutorFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptPromiseCapability* capability)
        : RuntimeFunction(type, functionInfo), capability(capability)
    { }

    template <> bool VarIsImpl<JavascriptPromiseCapabilitiesExecutorFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseCapabilitiesExecutorFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseCapabilitiesExecutorFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptPromiseCapability* JavascriptPromiseCapabilitiesExecutorFunction::GetCapability()
    {
        return this->capability;
    }

#if ENABLE_TTD
    void JavascriptPromiseCapabilitiesExecutorFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(false, "Not Implemented Yet");
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseCapabilitiesExecutorFunction::GetSnapTag_TTD() const
    {
        TTDAssert(false, "Not Implemented Yet");
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void JavascriptPromiseCapabilitiesExecutorFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Not Implemented Yet");
    }
#endif

    JavascriptPromiseCapability* JavascriptPromiseCapability::New(Var promise, Var resolve, Var reject, ScriptContext* scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), JavascriptPromiseCapability, promise, resolve, reject);
    }

    Var JavascriptPromiseCapability::GetResolve()
    {
        return this->resolve;
    }

    Var JavascriptPromiseCapability::GetReject()
    {
        return this->reject;
    }

    Var JavascriptPromiseCapability::GetPromise()
    {
        return this->promise;
    }

    void JavascriptPromiseCapability::SetPromise(Var promise)
    {
        this->promise = promise;
    }

    void JavascriptPromiseCapability::SetResolve(Var resolve)
    {
        this->resolve = resolve;
    }

    void JavascriptPromiseCapability::SetReject(Var reject)
    {
        this->reject = reject;
    }

#if ENABLE_TTD
    void JavascriptPromiseCapability::MarkVisitPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->promise != nullptr && this->resolve != nullptr && this->reject != nullptr, "Seems odd, I was not expecting this!!!");

        extractor->MarkVisitVar(this->promise);

        extractor->MarkVisitVar(this->resolve);
        extractor->MarkVisitVar(this->reject);
    }

    void JavascriptPromiseCapability::ExtractSnapPromiseCapabilityInto(TTD::NSSnapValues::SnapPromiseCapabilityInfo* snapPromiseCapability, JsUtil::List<TTD_PTR_ID, HeapAllocator>& depOnList, TTD::SlabAllocator& alloc)
    {
        snapPromiseCapability->CapabilityId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this);

        snapPromiseCapability->PromiseVar = this->promise;
        if(TTD::JsSupport::IsVarComplexKind(this->promise))
        {
            depOnList.Add(TTD_CONVERT_VAR_TO_PTR_ID(this->resolve));
        }

        snapPromiseCapability->ResolveVar = this->resolve;
        if(TTD::JsSupport::IsVarComplexKind(this->resolve))
        {
            depOnList.Add(TTD_CONVERT_VAR_TO_PTR_ID(this->resolve));
        }

        snapPromiseCapability->RejectVar = this->reject;
        if(TTD::JsSupport::IsVarComplexKind(this->reject))
        {
            depOnList.Add(TTD_CONVERT_VAR_TO_PTR_ID(this->reject));
        }
    }
#endif

    JavascriptPromiseReaction* JavascriptPromiseReaction::New(JavascriptPromiseCapability* capabilities, RecyclableObject* handler, ScriptContext* scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), JavascriptPromiseReaction, capabilities, handler);
    }

    JavascriptPromiseCapability* JavascriptPromiseReaction::GetCapabilities()
    {
        return this->capabilities;
    }

    RecyclableObject* JavascriptPromiseReaction::GetHandler()
    {
        return this->handler;
    }

#if ENABLE_TTD
    void JavascriptPromiseReaction::MarkVisitPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->handler != nullptr && this->capabilities != nullptr, "Seems odd, I was not expecting this!!!");

        extractor->MarkVisitVar(this->handler);

        this->capabilities->MarkVisitPtrs(extractor);
    }

    void JavascriptPromiseReaction::ExtractSnapPromiseReactionInto(TTD::NSSnapValues::SnapPromiseReactionInfo* snapPromiseReaction, JsUtil::List<TTD_PTR_ID, HeapAllocator>& depOnList, TTD::SlabAllocator& alloc)
    {
        TTDAssert(this->handler != nullptr && this->capabilities != nullptr, "Seems odd, I was not expecting this!!!");

        snapPromiseReaction->PromiseReactionId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this);

        snapPromiseReaction->HandlerObjId = TTD_CONVERT_VAR_TO_PTR_ID(this->handler);
        depOnList.Add(snapPromiseReaction->HandlerObjId);

        this->capabilities->ExtractSnapPromiseCapabilityInto(&snapPromiseReaction->Capabilities, depOnList, alloc);
    }
#endif

    template <> bool VarIsImpl<JavascriptPromiseReactionTaskFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseReactionTaskFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseReactionTaskFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptPromiseReaction* JavascriptPromiseReactionTaskFunction::GetReaction()
    {
        return this->reaction;
    }

    Var JavascriptPromiseReactionTaskFunction::GetArgument()
    {
        return this->argument;
    }

#if ENABLE_TTD
    void JavascriptPromiseReactionTaskFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->argument != nullptr && this->reaction != nullptr, "Was not expecting this!!!");

        extractor->MarkVisitVar(this->argument);

        this->reaction->MarkVisitPtrs(extractor);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseReactionTaskFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseReactionTaskFunctionObject;
    }

    void JavascriptPromiseReactionTaskFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapPromiseReactionTaskFunctionInfo* sprtfi = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseReactionTaskFunctionInfo>();

        JsUtil::List<TTD_PTR_ID, HeapAllocator> depOnList(&HeapAllocator::Instance);

        sprtfi->Argument = this->argument;

        if(this->argument != nullptr && TTD::JsSupport::IsVarComplexKind(this->argument))
        {
            depOnList.Add(TTD_CONVERT_VAR_TO_PTR_ID(this->argument));
        }

        this->reaction->ExtractSnapPromiseReactionInto(&sprtfi->Reaction, depOnList, alloc);

        //see what we need to do wrt dependencies
        if(depOnList.Count() == 0)
        {
            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseReactionTaskFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseReactionTaskFunctionObject>(objData, sprtfi);
        }
        else
        {
            uint32 depOnCount = depOnList.Count();
            TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

            for(uint32 i = 0; i < depOnCount; ++i)
            {
                depOnArray[i] = depOnList.Item(i);
            }

            TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseReactionTaskFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseReactionTaskFunctionObject>(objData, sprtfi, alloc, depOnCount, depOnArray);
        }
    }
#endif

    template <> bool VarIsImpl<JavascriptPromiseThenFinallyFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseThenFinallyFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseThenFinallyFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    template <> bool VarIsImpl<JavascriptPromiseThunkFinallyFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseThunkFinallyFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseThunkFinallyFunction>>::HasVirtualTable(obj);
        }
        return false;
    }

    template <> bool VarIsImpl<JavascriptPromiseResolveThenableTaskFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseResolveThenableTaskFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseResolveThenableTaskFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptPromise* JavascriptPromiseResolveThenableTaskFunction::GetPromise()
    {
        return this->promise;
    }

    RecyclableObject* JavascriptPromiseResolveThenableTaskFunction::GetThenable()
    {
        return this->thenable;
    }

    RecyclableObject* JavascriptPromiseResolveThenableTaskFunction::GetThenFunction()
    {
        return this->thenFunction;
    }

#if ENABLE_TTD
    void JavascriptPromiseResolveThenableTaskFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(false, "Not Implemented Yet");
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseResolveThenableTaskFunction::GetSnapTag_TTD() const
    {
        TTDAssert(false, "Not Implemented Yet");
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void JavascriptPromiseResolveThenableTaskFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Not Implemented Yet");
    }
#endif

    JavascriptPromiseAllSettledResolveOrRejectElementFunction::JavascriptPromiseAllSettledResolveOrRejectElementFunction(DynamicType* type)
        : JavascriptPromiseAllResolveElementFunction(type), alreadyCalledWrapper(nullptr), isRejecting(false)
    { }

    JavascriptPromiseAllSettledResolveOrRejectElementFunction::JavascriptPromiseAllSettledResolveOrRejectElementFunction(DynamicType* type, FunctionInfo* functionInfo, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper, bool isRejecting)
        : JavascriptPromiseAllResolveElementFunction(type, functionInfo, index, values, capabilities, remainingElementsWrapper), alreadyCalledWrapper(alreadyCalledWrapper), isRejecting(isRejecting)
    { }

    bool JavascriptPromiseAllSettledResolveOrRejectElementFunction::IsAlreadyCalled() const
    {
        Assert(this->alreadyCalledWrapper);

        return this->alreadyCalledWrapper->alreadyResolved;
    }

    void JavascriptPromiseAllSettledResolveOrRejectElementFunction::SetAlreadyCalled(const bool is)
    {
        Assert(this->alreadyCalledWrapper);

        this->alreadyCalledWrapper->alreadyResolved = is;
    }

    bool JavascriptPromiseAllSettledResolveOrRejectElementFunction::IsRejectFunction()
    {
        return this->isRejecting;
    }

    template <> bool VarIsImpl<JavascriptPromiseAllSettledResolveOrRejectElementFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseAllSettledResolveOrRejectElementFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseAllSettledResolveOrRejectElementFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

#if ENABLE_TTD
    void JavascriptPromiseAllSettledResolveOrRejectElementFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->capabilities != nullptr && this->remainingElementsWrapper != nullptr && this->alreadyCalledWrapper != nullptr && this->values != nullptr, "Don't think these can be null");

        this->capabilities->MarkVisitPtrs(extractor);
        extractor->MarkVisitVar(this->values);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseAllSettledResolveOrRejectElementFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseAllSettledResolveOrRejectElementFunctionObject;
    }

    void JavascriptPromiseAllSettledResolveOrRejectElementFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo* sprai = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo>();

        JsUtil::List<TTD_PTR_ID, HeapAllocator> depOnList(&HeapAllocator::Instance);
        this->capabilities->ExtractSnapPromiseCapabilityInto(&sprai->Capabilities, depOnList, alloc);

        sprai->Index = this->index;
        sprai->RemainingElementsWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->remainingElementsWrapper);
        sprai->RemainingElementsValue = this->remainingElementsWrapper->remainingElements;

        sprai->Values = TTD_CONVERT_VAR_TO_PTR_ID(this->values);
        depOnList.Add(sprai->Values);

        sprai->AlreadyCalled = this->alreadyCalled;

        uint32 depOnCount = depOnList.Count();
        TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        for (uint32 i = 0; i < depOnCount; ++i)
        {
            depOnArray[i] = depOnList.Item(i);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(objData, sprai, alloc, depOnCount, depOnArray);
    }
#endif

    JavascriptPromiseAnyRejectElementFunction::JavascriptPromiseAnyRejectElementFunction(DynamicType* type)
        : JavascriptPromiseAllResolveElementFunction(type), alreadyCalledWrapper(nullptr)
    { }

    JavascriptPromiseAnyRejectElementFunction::JavascriptPromiseAnyRejectElementFunction(DynamicType* type, FunctionInfo* functionInfo, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper, JavascriptPromiseResolveOrRejectFunctionAlreadyResolvedWrapper* alreadyCalledWrapper)
        : JavascriptPromiseAllResolveElementFunction(type, functionInfo, index, values, capabilities, remainingElementsWrapper), alreadyCalledWrapper(alreadyCalledWrapper)
    { }

#if ENABLE_TTD
    void JavascriptPromiseAnyRejectElementFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->capabilities != nullptr && this->remainingElementsWrapper != nullptr && this->alreadyCalledWrapper != nullptr && this->values != nullptr, "Don't think these can be null");

        this->capabilities->MarkVisitPtrs(extractor);
        extractor->MarkVisitVar(this->values);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseAnyRejectElementFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseAnyRejectElementFunctionObject;
    }

    void JavascriptPromiseAnyRejectElementFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo* sprai = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo>();

        JsUtil::List<TTD_PTR_ID, HeapAllocator> depOnList(&HeapAllocator::Instance);
        this->capabilities->ExtractSnapPromiseCapabilityInto(&sprai->Capabilities, depOnList, alloc);

        sprai->Index = this->index;
        sprai->RemainingElementsWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->remainingElementsWrapper);
        sprai->RemainingElementsValue = this->remainingElementsWrapper->remainingElements;

        sprai->Values = TTD_CONVERT_VAR_TO_PTR_ID(this->values);
        depOnList.Add(sprai->Values);

        sprai->AlreadyCalled = this->alreadyCalled;

        uint32 depOnCount = depOnList.Count();
        TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        for (uint32 i = 0; i < depOnCount; ++i)
        {
            depOnArray[i] = depOnList.Item(i);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(objData, sprai, alloc, depOnCount, depOnArray);
    }
#endif

    template <> bool VarIsImpl<JavascriptPromiseAnyRejectElementFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseAnyRejectElementFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseAnyRejectElementFunction>>::HasVirtualTable(obj);
        }

        return false;
    }


    JavascriptPromiseAllResolveElementFunction::JavascriptPromiseAllResolveElementFunction(DynamicType* type)
        : RuntimeFunction(type, &Js::JavascriptPromise::EntryInfo::AllResolveElementFunction), index(0), values(nullptr), capabilities(nullptr), remainingElementsWrapper(nullptr), alreadyCalled(false)
    { }

    JavascriptPromiseAllResolveElementFunction::JavascriptPromiseAllResolveElementFunction(DynamicType* type, FunctionInfo* functionInfo, uint32 index, JavascriptArray* values, JavascriptPromiseCapability* capabilities, JavascriptPromiseAllResolveElementFunctionRemainingElementsWrapper* remainingElementsWrapper)
        : RuntimeFunction(type, functionInfo), index(index), values(values), capabilities(capabilities), remainingElementsWrapper(remainingElementsWrapper), alreadyCalled(false)
    { }

    template <> bool VarIsImpl<JavascriptPromiseAllResolveElementFunction>(RecyclableObject* obj)
    {
        if (VarIs<JavascriptFunction>(obj))
        {
            return VirtualTableInfo<JavascriptPromiseAllResolveElementFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptPromiseAllResolveElementFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptPromiseCapability* JavascriptPromiseAllResolveElementFunction::GetCapabilities()
    {
        return this->capabilities;
    }

    uint32 JavascriptPromiseAllResolveElementFunction::GetIndex()
    {
        return this->index;
    }

    uint32 JavascriptPromiseAllResolveElementFunction::GetRemainingElements()
    {
        return this->remainingElementsWrapper->remainingElements;
    }

    JavascriptArray* JavascriptPromiseAllResolveElementFunction::GetValues()
    {
        return this->values;
    }

    uint32 JavascriptPromiseAllResolveElementFunction::DecrementRemainingElements()
    {
        return --(this->remainingElementsWrapper->remainingElements);
    }

    bool JavascriptPromiseAllResolveElementFunction::IsAlreadyCalled() const
    {
        return this->alreadyCalled;
    }

    void JavascriptPromiseAllResolveElementFunction::SetAlreadyCalled(const bool is)
    {
        this->alreadyCalled = is;
    }

#if ENABLE_TTD
    void JavascriptPromiseAllResolveElementFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->capabilities != nullptr && this->remainingElementsWrapper != nullptr && this->values != nullptr, "Don't think these can be null");

        this->capabilities->MarkVisitPtrs(extractor);
        extractor->MarkVisitVar(this->values);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptPromiseAllResolveElementFunction::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject;
    }

    void JavascriptPromiseAllResolveElementFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo* sprai = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo>();

        JsUtil::List<TTD_PTR_ID, HeapAllocator> depOnList(&HeapAllocator::Instance);
        this->capabilities->ExtractSnapPromiseCapabilityInto(&sprai->Capabilities, depOnList, alloc);

        sprai->Index = this->index;
        sprai->RemainingElementsWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->remainingElementsWrapper);
        sprai->RemainingElementsValue = this->remainingElementsWrapper->remainingElements;

        sprai->Values = TTD_CONVERT_VAR_TO_PTR_ID(this->values);
        depOnList.Add(sprai->Values);

        sprai->AlreadyCalled = this->alreadyCalled;

        uint32 depOnCount = depOnList.Count();
        TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        for(uint32 i = 0; i < depOnCount; ++i)
        {
            depOnArray[i] = depOnList.Item(i);
        }

        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseAllResolveElementFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject>(objData, sprai, alloc, depOnCount, depOnArray);
    }
#endif

    Var JavascriptPromise::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    //static
    JavascriptPromise* JavascriptPromise::CreateEnginePromise(ScriptContext *scriptContext)
    {
        JavascriptPromiseResolveOrRejectFunction *resolve = nullptr;
        JavascriptPromiseResolveOrRejectFunction *reject = nullptr;

        JavascriptPromise *promise = scriptContext->GetLibrary()->CreatePromise();
        JavascriptPromise::InitializePromise(promise, &resolve, &reject, scriptContext);

        return promise;
    }
} // namespace Js
