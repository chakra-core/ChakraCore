//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

using namespace Js;

FunctionInfo JavascriptAsyncFunction::functionInfo(
    FORCE_NO_WRITE_BARRIER_TAG(JavascriptAsyncFunction::EntryAsyncFunctionImplementation),
    (FunctionInfo::Attributes)(FunctionInfo::DoNotProfile | FunctionInfo::ErrorOnNew));

JavascriptAsyncFunction::JavascriptAsyncFunction(
    DynamicType* type,
    GeneratorVirtualScriptFunction* scriptFunction) :
        JavascriptGeneratorFunction(type, &functionInfo, scriptFunction)
{
    DebugOnly(VerifyEntryPoint());
}

JavascriptAsyncFunction* JavascriptAsyncFunction::New(
    ScriptContext* scriptContext,
    GeneratorVirtualScriptFunction* scriptFunction)
{
    return scriptContext->GetLibrary()->CreateAsyncFunction(
        functionInfo.GetOriginalEntryPoint(),
        scriptFunction);
}

template<>
bool Js::VarIsImpl<JavascriptAsyncFunction>(RecyclableObject* obj)
{
    return VarIs<JavascriptFunction>(obj) && (
        VirtualTableInfo<JavascriptAsyncFunction>::HasVirtualTable(obj) ||
        VirtualTableInfo<CrossSiteObject<JavascriptAsyncFunction>>::HasVirtualTable(obj)
    );
}

Var JavascriptAsyncFunction::EntryAsyncFunctionImplementation(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();
    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    ARGUMENTS(args, callInfo);

    auto* library = scriptContext->GetLibrary();
    auto* asyncFn = VarTo<JavascriptAsyncFunction>(function);
    auto* scriptFn = asyncFn->GetGeneratorVirtualScriptFunction();
    auto* generator = library->CreateGenerator(args, scriptFn, library->GetNull());

    return BeginAsyncFunctionExecution(generator);
}

JavascriptPromise* JavascriptAsyncFunction::BeginAsyncFunctionExecution(JavascriptGenerator* generator)
{
    auto* library = generator->GetLibrary();
    auto* scriptContext = generator->GetScriptContext();
    auto* promise = library->CreatePromise();

    auto* stepFn = library->CreateAsyncSpawnStepFunction(
        EntryAsyncSpawnStepNextFunction,
        generator,
        library->GetUndefined());

    JavascriptExceptionObject* exception = nullptr;
    JavascriptPromiseResolveOrRejectFunction* resolve;
    JavascriptPromiseResolveOrRejectFunction* reject;
    JavascriptPromise::InitializePromise(promise, &resolve, &reject, scriptContext);

    try
    {
        AsyncSpawnStep(stepFn, generator, resolve, reject);
    }
    catch (const JavascriptException& err)
    {
        exception = err.GetAndClear();
    }

    if (exception != nullptr)
        JavascriptPromise::TryRejectWithExceptionObject(exception, reject, scriptContext);

    return promise;
}

Var JavascriptAsyncFunction::EntryAsyncSpawnStepNextFunction(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();
    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    auto* stepFn = VarTo<JavascriptAsyncSpawnStepFunction>(function);
    return stepFn->generator->CallGenerator(stepFn->argument, ResumeYieldKind::Normal);
}

Var JavascriptAsyncFunction::EntryAsyncSpawnStepThrowFunction(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();
    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

    auto* stepFn = VarTo<JavascriptAsyncSpawnStepFunction>(function);
    return stepFn->generator->CallGenerator(stepFn->argument, ResumeYieldKind::Throw);
}

Var JavascriptAsyncFunction::EntryAsyncSpawnCallStepFunction(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();
    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    ARGUMENTS(args, callInfo);

    auto* library = scriptContext->GetLibrary();
    Var undefinedVar = library->GetUndefined();
    Var resolvedValue = args.Info.Count > 1 ? args[1] : undefinedVar;

    auto* stepFn = VarTo<JavascriptAsyncSpawnStepFunction>(function);

    JavascriptMethod method = stepFn->isReject
        ? EntryAsyncSpawnStepThrowFunction
        : EntryAsyncSpawnStepNextFunction;
    
    auto* nextStepFn = library->CreateAsyncSpawnStepFunction(
        method,
        stepFn->generator,
        resolvedValue);

    AsyncSpawnStep(nextStepFn, stepFn->generator, stepFn->resolve, stepFn->reject);
    return undefinedVar;
}

void JavascriptAsyncFunction::AsyncSpawnStep(
    JavascriptAsyncSpawnStepFunction* stepFunction,
    JavascriptGenerator* generator,
    Var resolve,
    Var reject)
{
    ScriptContext* scriptContext = generator->GetScriptContext();
    BEGIN_SAFE_REENTRANT_REGION(scriptContext->GetThreadContext())

    JavascriptLibrary* library = scriptContext->GetLibrary();
    Var undefinedVar = library->GetUndefined();

    JavascriptExceptionObject* exception = nullptr;
    RecyclableObject* result = nullptr;

    try
    {
        Var resultVar = CALL_FUNCTION(
            scriptContext->GetThreadContext(),
            stepFunction,
            CallInfo(CallFlags_Value, 1),
            undefinedVar);

        result = VarTo<RecyclableObject>(resultVar);
    }
    catch (const JavascriptException& err)
    {
        exception = err.GetAndClear();
    }

    if (exception != nullptr)
    {
        // If the generator threw an exception, reject the promise
        JavascriptPromise::TryRejectWithExceptionObject(exception, reject, scriptContext);
        return;
    }

    Assert(result != nullptr);

    Var value = JavascriptOperators::GetProperty(result, PropertyIds::value, scriptContext);

    // If the generator is done, resolve the promise
    if (generator->IsCompleted())
    {
        if (!JavascriptConversion::IsCallable(resolve))
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedFunction);

        CALL_FUNCTION(
            scriptContext->GetThreadContext(),
            VarTo<RecyclableObject>(resolve),
            CallInfo(CallFlags_Value, 2),
            undefinedVar,
            value);

        return;
    }
    else
    {
        Assert(JavascriptOperators::GetTypeId(result) == TypeIds_AwaitObject);
    }


    // Chain off the yielded promise and step again
    auto* successFunction = library->CreateAsyncSpawnStepFunction(
        EntryAsyncSpawnCallStepFunction,
        generator,
        undefinedVar,
        resolve,
        reject);

    auto* failFunction = library->CreateAsyncSpawnStepFunction(
        EntryAsyncSpawnCallStepFunction,
        generator,
        undefinedVar,
        resolve,
        reject,
        true);

    auto* promise = JavascriptPromise::InternalPromiseResolve(value, scriptContext);
    auto* unused = JavascriptPromise::UnusedPromiseCapability(scriptContext);
    
    JavascriptPromise::PerformPromiseThen(
        promise,
        unused,
        successFunction,
        failFunction,
        scriptContext);

    END_SAFE_REENTRANT_REGION
}

template<>
bool Js::VarIsImpl<JavascriptAsyncSpawnStepFunction>(RecyclableObject* obj)
{
    return VarIs<JavascriptFunction>(obj) && (
        VirtualTableInfo<JavascriptAsyncSpawnStepFunction>::HasVirtualTable(obj) ||
        VirtualTableInfo<CrossSiteObject<JavascriptAsyncSpawnStepFunction>>::HasVirtualTable(obj)
    );
}

#if ENABLE_TTD

TTD::NSSnapObjects::SnapObjectType JavascriptAsyncFunction::GetSnapTag_TTD() const
{
    return TTD::NSSnapObjects::SnapObjectType::SnapAsyncFunction;
}

void JavascriptAsyncFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::SnapGeneratorFunctionInfo* fi = nullptr;
    uint32 depCount = 0;
    TTD_PTR_ID* depArray = nullptr;

    this->CreateSnapObjectInfo(alloc, &fi, &depArray, &depCount);

    if (depCount == 0)
    {
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapGeneratorFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapAsyncFunction>(objData, fi);
    }
    else
    {
        TTDAssert(depArray != nullptr, "depArray should be non-null if depCount is > 0");
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapGeneratorFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapAsyncFunction>(objData, fi, alloc, depCount, depArray);
    }
}

void JavascriptAsyncSpawnStepFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
{
    if (this->generator != nullptr)
    {
        extractor->MarkVisitVar(this->generator);
    }

    if (this->reject != nullptr)
    {
        extractor->MarkVisitVar(this->reject);
    }

    if (this->resolve != nullptr)
    {
        extractor->MarkVisitVar(this->resolve);
    }

    if (this->argument != nullptr)
    {
        extractor->MarkVisitVar(this->argument);
    }
}

TTD::NSSnapObjects::SnapObjectType JavascriptAsyncSpawnStepFunction::GetSnapTag_TTD() const
{
    return TTD::NSSnapObjects::SnapObjectType::JavascriptAsyncSpawnStepFunction;
}

void JavascriptAsyncSpawnStepFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::SnapJavascriptAsyncSpawnStepFunctionInfo* info = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapJavascriptAsyncSpawnStepFunctionInfo>();
    info->generator = TTD_CONVERT_VAR_TO_PTR_ID(this->generator);
    info->reject = this->reject;
    info->resolve = this->resolve;
    info->argument = this->argument;
    info->isReject = this->isReject;

    info->entryPoint = 0;
    JavascriptMethod entryPoint = this->GetFunctionInfo()->GetOriginalEntryPoint();
    if (entryPoint == JavascriptAsyncFunction::EntryAsyncSpawnStepNextFunction)
    {
        info->entryPoint = 1;
    }
    else if (entryPoint == JavascriptAsyncFunction::EntryAsyncSpawnStepThrowFunction)
    {
        info->entryPoint = 2;
    }
    else if (entryPoint == JavascriptAsyncFunction::EntryAsyncSpawnCallStepFunction)
    {
        info->entryPoint = 3;
    }
    else
    {
        TTDAssert(false, "Unexpected entrypoint found JavascriptAsyncSpawnStepArgumentExecutorFunction");
    }

    const uint32 maxDeps = 4;
    uint32 depCount = 0;
    TTD_PTR_ID* depArray = alloc.SlabReserveArraySpace<TTD_PTR_ID>(maxDeps);
    if (this->reject != nullptr &&  TTD::JsSupport::IsVarComplexKind(this->reject))
    {
        depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->reject);
        depCount++;
    }

    if (this->resolve != nullptr &&  TTD::JsSupport::IsVarComplexKind(this->resolve))
    {
        depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->resolve);
        depCount++;
    }

    if (this->argument != nullptr &&  TTD::JsSupport::IsVarComplexKind(this->argument))
    {
        depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->argument);
        depCount++;
    }

    if (this->generator != nullptr)
    {
        depArray[depCount] = TTD_CONVERT_VAR_TO_PTR_ID(this->generator);
        depCount++;
    }

    if (depCount > 0)
    {
        alloc.SlabCommitArraySpace<TTD_PTR_ID>(depCount, maxDeps);
    }
    else
    {
        alloc.SlabAbortArraySpace<TTD_PTR_ID>(maxDeps);
    }

    if (depCount == 0)
    {
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapJavascriptAsyncSpawnStepFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::JavascriptAsyncSpawnStepFunction>(objData, info);
    }
    else
    {
        TTDAssert(depArray != nullptr, "depArray should be non-null if depCount is > 0");
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapJavascriptAsyncSpawnStepFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::JavascriptAsyncSpawnStepFunction>(objData, info, alloc, depCount, depArray);
    }
}

#endif
