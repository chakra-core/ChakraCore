//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Language/InterpreterStackFrame.h"

using namespace Js;

JavascriptAsyncGenerator* JavascriptAsyncGenerator::New(
    Recycler* recycler,
    DynamicType* generatorType,
    Arguments& args,
    ScriptFunction* scriptFunction)
{
    // InterpreterStackFrame takes a pointer to the args, so copy them to the recycler
    // heap and use that buffer for the asyncgenerator's InterpreterStackFrame
    Field(Var)* argValuesCopy = nullptr;

    if (args.Info.Count > 0)
    {
        argValuesCopy = RecyclerNewArray(recycler, Field(Var), args.Info.Count);
        CopyArray(argValuesCopy, args.Info.Count, args.Values, args.Info.Count);
    }

    Arguments heapArgs(args.Info, unsafe_write_barrier_cast<Var*>(argValuesCopy));

    auto* requestQueue = RecyclerNew(recycler, JavascriptAsyncGenerator::RequestQueue, recycler);

    JavascriptAsyncGenerator* generator = RecyclerNewFinalized(
        recycler,
        JavascriptAsyncGenerator,
        generatorType,
        heapArgs,
        scriptFunction,
        requestQueue);

    auto* library = scriptFunction->GetLibrary();
    generator->onFulfilled = library->CreateAsyncGeneratorCallbackFunction(
        EntryAwaitFulfilledCallback,
        generator);

    generator->onRejected = library->CreateAsyncGeneratorCallbackFunction(
        EntryAwaitRejectedCallback,
        generator);

    return generator;
}

Var JavascriptAsyncGenerator::EntryNext(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    auto* scriptContext = function->GetScriptContext();
    auto* library = scriptContext->GetLibrary();

    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("AsyncGenerator.prototype.next"));

    Var thisValue = args[0];
    Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

    return EnqueueRequest(
        thisValue,
        scriptContext,
        input,
        ResumeYieldKind::Normal,
        _u("AsyncGenerator.prototype.next"));
}

Var JavascriptAsyncGenerator::EntryReturn(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    auto* scriptContext = function->GetScriptContext();
    auto* library = scriptContext->GetLibrary();

    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("AsyncGenerator.prototype.return"));

    Var thisValue = args[0];
    Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

    return EnqueueRequest(
        thisValue,
        scriptContext,
        input,
        ResumeYieldKind::Return,
        _u("AsyncGenerator.prototype.return"));
}

Var JavascriptAsyncGenerator::EntryThrow(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    auto* scriptContext = function->GetScriptContext();
    auto* library = scriptContext->GetLibrary();

    AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("AsyncGenerator.prototype.throw"));

    Var thisValue = args[0];
    Var input = args.Info.Count > 1 ? args[1] : library->GetUndefined();

    return EnqueueRequest(
        thisValue,
        scriptContext,
        input,
        ResumeYieldKind::Throw,
        _u("AsyncGenerator.prototype.throw"));
}

Var JavascriptAsyncGenerator::EntryAwaitFulfilledCallback(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();

    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    ARGUMENTS(args, callInfo);

    AssertOrFailFast(args.Info.Count > 1);

    Var value = args[1];
    auto* callbackFn = VarTo<AsyncGeneratorCallbackFunction>(function);
    JavascriptAsyncGenerator* generator = callbackFn->generator;

    PendingState state = generator->pendingState;
    generator->pendingState = PendingState::None;

    switch (state)
    {
        case PendingState::Await:
            generator->ResumeCoroutine(value, ResumeYieldKind::Normal);
            break;
        case PendingState::AwaitReturn:
            generator->ResumeCoroutine(value, ResumeYieldKind::Return);
            break;
        case PendingState::Yield:
            generator->ResolveNext(value);
            break;
        default:
            AssertMsg(false, "Expected an async generator pending state");
            break;
    }

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptAsyncGenerator::EntryAwaitRejectedCallback(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();

    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    ARGUMENTS(args, callInfo);

    AssertOrFailFast(args.Info.Count > 1);

    Var value = args[1];
    auto* callbackFn = VarTo<AsyncGeneratorCallbackFunction>(function);
    JavascriptAsyncGenerator* generator = callbackFn->generator;

    PendingState state = generator->pendingState;
    generator->pendingState = PendingState::None;

    switch (state)
    {
        case PendingState::Await:
        case PendingState::AwaitReturn:
            generator->ResumeCoroutine(value, ResumeYieldKind::Throw);
            break;
        case PendingState::Yield:
            generator->RejectNext(value);
            break;
        default:
            AssertMsg(false, "Expected an async generator pending state");
            break;
    }

    return scriptContext->GetLibrary()->GetUndefined();
}

Var JavascriptAsyncGenerator::EnqueueRequest(
    Var thisValue,
    ScriptContext* scriptContext,
    Var input,
    ResumeYieldKind resumeKind,
    const char16* apiNameForErrorMessage)
{
    auto* promise = JavascriptPromise::CreateEnginePromise(scriptContext);

    if (!VarIs<JavascriptAsyncGenerator>(thisValue))
    {
        auto* library = scriptContext->GetLibrary();
        auto* error = library->CreateTypeError();

        JavascriptError::SetErrorMessage(
            error,
            JSERR_NeedObjectOfType,
            scriptContext,
            apiNameForErrorMessage,
            _u("AsyncGenerator"));

        promise->Reject(error, scriptContext);
    }
    else
    {
        auto* request = RecyclerNew(
            scriptContext->GetRecycler(),
            AsyncGeneratorRequest,
            input,
            resumeKind,
            promise);

        auto* generator = UnsafeVarTo<JavascriptAsyncGenerator>(thisValue);
        generator->PushRequest(request);
        generator->ResumeNext();
    }

    return promise;
}

void JavascriptAsyncGenerator::ResumeNext()
{
    if (IsExecuting() || this->pendingState != PendingState::None || !HasRequest())
        return;

    auto* scriptContext = GetScriptContext();
    auto* library = scriptContext->GetLibrary();

    AsyncGeneratorRequest* next = PeekRequest();

    if (next->kind != ResumeYieldKind::Normal)
    {
        if (IsSuspendedStart())
            SetCompleted();

        if (next->kind == ResumeYieldKind::Return)
        {
            if (IsCompleted()) UnwrapValue(next->data, PendingState::Yield);
            else UnwrapValue(next->data, PendingState::AwaitReturn);
        }
        else
        {
            if (IsCompleted()) RejectNext(next->data);
            else ResumeCoroutine(next->data, next->kind);
        }
    }
    else
    {
        if (IsCompleted()) ResolveNext(library->GetUndefined());
        else ResumeCoroutine(next->data, next->kind);
    }
}

void JavascriptAsyncGenerator::ResumeCoroutine(Var value, ResumeYieldKind resumeKind)
{
    Assert(this->pendingState == PendingState::None);

    RecyclableObject* result = nullptr;

    try
    {
        // Call the internal (sync) generator entry point
        result = VarTo<RecyclableObject>(this->CallGenerator(value, resumeKind));
    }
    catch (const JavascriptException& err)
    {
        RejectNext(err.GetAndClear()->GetThrownObject(nullptr));
        return;
    }

    Var resultValue = JavascriptOperators::GetProperty(
        result,
        PropertyIds::value,
        GetScriptContext());

    if (JavascriptOperators::GetTypeId(result) == TypeIds_AwaitObject)
    {
        UnwrapValue(resultValue, PendingState::Await);
        return;
    }

    if (IsCompleted())
    {
        // If the generator is completed, then resolve immediately. Return
        // values are unwrapped explicitly by the code generated for the
        // return statement.
        ResolveNext(resultValue);
    }
    else
    {
        // Otherwise, await the yielded value
        UnwrapValue(resultValue, PendingState::Yield);
    }
}

void JavascriptAsyncGenerator::ResolveNext(Var value)
{
    auto* scriptContext = GetScriptContext();
    auto* library = scriptContext->GetLibrary();
    Var result = library->CreateIteratorResultObject(value, IsCompleted());
    ShiftRequest()->promise->Resolve(result, scriptContext);
    ResumeNext();
}

void JavascriptAsyncGenerator::RejectNext(Var reason)
{
    SetCompleted();
    ShiftRequest()->promise->Reject(reason, GetScriptContext());
    ResumeNext();
}

void JavascriptAsyncGenerator::UnwrapValue(Var value, PendingState pendingState)
{
    this->pendingState = pendingState;

    auto* scriptContext = GetScriptContext();
    auto* promise = JavascriptPromise::InternalPromiseResolve(value, scriptContext);
    auto* unused = JavascriptPromise::UnusedPromiseCapability(scriptContext);

    JavascriptPromise::PerformPromiseThen(promise, unused, onFulfilled, onRejected, scriptContext);
}

template<>
bool Js::VarIsImpl<JavascriptAsyncGenerator>(RecyclableObject* obj)
{
    return JavascriptOperators::GetTypeId(obj) == TypeIds_AsyncGenerator;
}

template<>
bool Js::VarIsImpl<AsyncGeneratorCallbackFunction>(RecyclableObject* obj)
{
    return VarIs<JavascriptFunction>(obj) && (
        VirtualTableInfo<AsyncGeneratorCallbackFunction>::HasVirtualTable(obj) ||
        VirtualTableInfo<CrossSiteObject<AsyncGeneratorCallbackFunction>>::HasVirtualTable(obj)
    );
}
