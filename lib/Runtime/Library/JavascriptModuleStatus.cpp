//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptModuleStatus::JavascriptModuleStatus(DynamicType* type)
        : DynamicObject(type), error(false), loader(nullptr), key(nullptr), pipeline(nullptr), module(nullptr)
    {
        Recycler* recycler = type->GetRecycler();
        this->pipeline = RecyclerNew(recycler, JavascriptModuleStatusStageRecordList, recycler);
    }

    JavascriptModuleStatus* JavascriptModuleStatus::New(JavascriptLoader* loader, JavascriptString* key, Var ns, ScriptContext* scriptContext)
    {
        JavascriptModuleStatus* moduleStatus = scriptContext->GetLibrary()->CreateModuleStatus();

        moduleStatus->Initialize(loader, key, ns, scriptContext);

        return moduleStatus;
    }

    bool JavascriptModuleStatus::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_ModuleStatus;
    }

    JavascriptModuleStatus* JavascriptModuleStatus::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptModuleStatus'");

        return static_cast<JavascriptModuleStatus*>(RecyclableObject::FromVar(aValue));
    }

    bool JavascriptModuleStatus::IsValidStageValue(Var stage, ScriptContext* scriptContext, JavascriptModuleStatusStage* result)
    {
        Assert(result != nullptr);

        JavascriptString* stringStage = JavascriptConversion::ToString(stage, scriptContext);

        if (stringStage->BufferEquals(_u("fetch"), 5))
        {
            *result = JavascriptModuleStatusStage::Fetch;
            return true;
        }
        else if (stringStage->BufferEquals(_u("instantiate"), 11))
        {
            *result = JavascriptModuleStatusStage::Instantiate;
            return true;
        }
        else if (stringStage->BufferEquals(_u("translate"), 9))
        {
            *result = JavascriptModuleStatusStage::Translate;
            return true;
        }

        *result = JavascriptModuleStatusStage::Invalid;
        return false;
    }

    bool JavascriptModuleStatus::GetStage(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext, JavascriptModuleStatusStageRecord** result)
    {
        Assert(entry != nullptr && entry->pipeline != nullptr);
        Assert(result != nullptr);

        *result = nullptr;

        entry->pipeline->MapUntil([=](JavascriptModuleStatusStageRecord stageEntry) {
            if (stageEntry.stage == stage)
            {
                *result = &stageEntry;
                return true;
            }
            return false;
        });

        return *result != nullptr;
    }

    JavascriptModuleStatusStageRecord* JavascriptModuleStatus::GetCurrentStage(JavascriptModuleStatus* entry, ScriptContext* scriptContext)
    {
        Assert(entry != nullptr && entry->pipeline != nullptr);

        if (!entry->pipeline->Empty())
        {
            return &entry->pipeline->Head();
        }

        // TODO: Assert here?
        return nullptr;
    }

    void JavascriptModuleStatus::UpgradeToStage(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext)
    {
        JavascriptModuleStatusStageRecord* stageEntry = nullptr;

        if (GetStage(entry, stage, scriptContext, &stageEntry))
        {
            Assert(stageEntry != nullptr);

            while (!entry->pipeline->Empty() && &entry->pipeline->Head() != stageEntry)
            {
                entry->pipeline->RemoveHead();
            }
        }
    }

    Var JavascriptModuleStatus::LoadModule(JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, ScriptContext* scriptContext)
    {
        Assert(entry != nullptr);

        JavascriptPromise* requestResult = nullptr;

        switch (stage)
        {
        case JavascriptModuleStatusStage::Fetch:
            requestResult = RequestFetch(entry, scriptContext);
            break;
        case JavascriptModuleStatusStage::Translate:
            requestResult = RequestTranslate(entry, scriptContext);
            break;
        case JavascriptModuleStatusStage::Instantiate:
            requestResult = RequestInstantiate(entry, nullptr, scriptContext);
            break;
        default:
            {
            JavascriptError* rangeError = scriptContext->GetLibrary()->CreateRangeError();
            JavascriptError::SetErrorMessage(rangeError, JSERR_InvalidModuleStatusStage, _u(""), scriptContext);

            return JavascriptPromise::CreateRejectedPromise(rangeError, scriptContext);
            }
        }

        Assert(requestResult != nullptr);

        return JavascriptPromise::CreatePassThroughPromise(requestResult, scriptContext);
    }

    JavascriptPromise* JavascriptModuleStatus::RequestFetch(JavascriptModuleStatus* entry, ScriptContext* scriptContext)
    {
        JavascriptModuleStatusStageRecord* fetchStageEntry = nullptr;
        if (!GetStage(entry, JavascriptModuleStatusStage::Fetch, scriptContext, &fetchStageEntry))
        {
            return JavascriptPromise::CreateResolvedPromise(scriptContext->GetLibrary()->GetUndefined(), scriptContext);
        }

        Assert(fetchStageEntry != nullptr);

        if (fetchStageEntry->result != nullptr)
        {
            return fetchStageEntry->result;
        }

        Var fetchVar = JavascriptOperators::GetProperty(entry->loader, Js::PropertyIds::_symbolFetch, scriptContext);
        if (!JavascriptConversion::IsCallable(fetchVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction);
        }

        RecyclableObject* hook = RecyclableObject::FromVar(fetchVar);

        Var hookResult = CALL_FUNCTION(hook, Js::CallInfo(CallFlags_Value, 3),
            hook,
            entry,
            entry->key);

        // TODO: Confirm we should throw here? What does "promise-call the hook function" mean?
        if (!JavascriptPromise::Is(hookResult))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptFunction* throwerFunction = library->GetThrowerFunction();
        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
            EntryUpgradeToStageFulfillmentHandler, 
            &JavascriptModuleStatus::EntryInfo::UpgradeToStageFulfillmentHandler, 
            entry, 
            JavascriptModuleStatusStage::Translate, 
            nullptr);
        
        Var p = JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(hookResult), fulfillmentHandler, throwerFunction, scriptContext);

        // TODO: Confirm we should throw here? What does "promise-call the hook function" mean?
        if (!JavascriptPromise::Is(p))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        JavascriptPromise* promise = JavascriptPromise::FromVar(p);
        JavascriptFunction* identityFunction = library->GetIdentityFunction();
        JavascriptModuleStatusErrorHandlerFunction* errorHandler = library->CreateModuleStatusErrorHandlerFunction(EntryErrorHandler, entry);
        JavascriptPromise::CreateThenPromise(promise, identityFunction, errorHandler, scriptContext);

        fetchStageEntry->result = promise;

        return promise;
    }

    JavascriptPromise* JavascriptModuleStatus::RequestTranslate(JavascriptModuleStatus* entry, ScriptContext* scriptContext)
    {
        JavascriptModuleStatusStageRecord* translateStageEntry = nullptr;
        if (!GetStage(entry, JavascriptModuleStatusStage::Fetch, scriptContext, &translateStageEntry))
        {
            return JavascriptPromise::CreateResolvedPromise(scriptContext->GetLibrary()->GetUndefined(), scriptContext);
        }

        Assert(translateStageEntry != nullptr);

        if (translateStageEntry->result != nullptr)
        {
            return translateStageEntry->result;
        }

        JavascriptPromise* requestFetchResult = RequestFetch(entry, scriptContext);
        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
            EntryRequestTranslateOrInstantiateFulfillmentHandler,
            &JavascriptModuleStatus::EntryInfo::RequestTranslateOrInstantiateFulfillmentHandler,
            entry, 
            JavascriptModuleStatusStage::Translate,
            nullptr);

        Var p = JavascriptPromise::CreateThenPromise(requestFetchResult, fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
        
        // TODO: Confirm we should throw here?
        if (!JavascriptPromise::Is(p))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        JavascriptPromise* promise = JavascriptPromise::FromVar(p);
        JavascriptModuleStatusErrorHandlerFunction* errorHandler = library->CreateModuleStatusErrorHandlerFunction(EntryErrorHandler, entry);
        JavascriptPromise::CreateThenPromise(promise, library->GetIdentityFunction(), errorHandler, scriptContext);

        translateStageEntry->result = promise;

        return promise;
    }

    JavascriptPromise* JavascriptModuleStatus::RequestInstantiate(JavascriptModuleStatus* entry, Var instantiateSet, ScriptContext* scriptContext)
    {
        JavascriptModuleStatusStageRecord* instantiateStageEntry = nullptr;
        if (!GetStage(entry, JavascriptModuleStatusStage::Instantiate, scriptContext, &instantiateStageEntry))
        {
            return JavascriptPromise::CreateResolvedPromise(scriptContext->GetLibrary()->GetUndefined(), scriptContext);
        }

        Assert(instantiateStageEntry != nullptr);

        if (instantiateStageEntry->result != nullptr)
        {
            return instantiateStageEntry->result;
        }

        JavascriptPromise* requestTranslateResult = RequestTranslate(entry, scriptContext);
        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
            EntryRequestTranslateOrInstantiateFulfillmentHandler,
            &JavascriptModuleStatus::EntryInfo::RequestTranslateOrInstantiateFulfillmentHandler,
            entry,
            JavascriptModuleStatusStage::Instantiate,
            instantiateSet);

        Var p = JavascriptPromise::CreateThenPromise(requestTranslateResult, fulfillmentHandler, library->GetThrowerFunction(), scriptContext);

        // TODO: Confirm we should throw here?
        if (!JavascriptPromise::Is(p))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        JavascriptPromise* promise = JavascriptPromise::FromVar(p);
        JavascriptModuleStatusErrorHandlerFunction* errorHandler = library->CreateModuleStatusErrorHandlerFunction(EntryErrorHandler, entry);
        JavascriptPromise::CreateThenPromise(promise, library->GetIdentityFunction(), errorHandler, scriptContext);

        instantiateStageEntry->result = promise;

        return promise;
    }

    JavascriptPromise* JavascriptModuleStatus::SatisfyInstance(JavascriptModuleStatus* entry, Var optionalInstance, Var source, Var instantiateSet, ScriptContext* scriptContext)
    {
        // TODO
        return nullptr;
    }

    JavascriptModuleStatus* JavascriptModuleStatus::EnsureRegistered(JavascriptLoader* loader, Var key, ScriptContext* scriptContext)
    {
        JavascriptRegistry* registry = loader->GetRegistry();
        Var value = nullptr;
        JavascriptModuleStatus* entry = nullptr;

        if (registry->Get(key, &value))
        {
            entry = JavascriptModuleStatus::FromVar(value);
        }
        else
        {
            entry = JavascriptModuleStatus::New(loader, JavascriptConversion::ToString(key, scriptContext), scriptContext->GetLibrary()->GetUndefined(), scriptContext);
        }

        return entry;
    }

    Var JavascriptModuleStatus::EnsureEvaluated(JavascriptModuleStatus* entry, ScriptContext* scriptContext)
    {
        JavascriptModuleStatusStageRecord* stageEntry = GetCurrentStage(entry, scriptContext);

        Assert(stageEntry != nullptr);

        // TODO:
        //Let module be entry.[[Module]].
        //    If module.[[Evaluated]] is false, then:
        //Perform ? EnsureLinked(entry).
        //    Perform ? module.ModuleEvaluation().
        //    Return ? GetModuleNamespace(module).

        return stageEntry->result;
    }
    
    JavascriptModuleStatusStageRecord* JavascriptModuleStatusStageRecord::New(JavascriptModuleStatusStage stage, JavascriptPromise* result, ScriptContext* scriptContext)
    {
        return RecyclerNew(scriptContext->GetRecycler(), JavascriptModuleStatusStageRecord, stage, result);
    }

    Var JavascriptModuleStatus::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("ModuleStatus"));

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr
            || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);

        JavascriptLibrary* library = scriptContext->GetLibrary();

        if ((callInfo.Flags & CallFlags_New) != CallFlags_New || (newTarget != nullptr && JavascriptOperators::IsUndefined(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("ModuleStatus"));
        }
        if (args.Info.Count < 2 || !JavascriptLoader::Is(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_Invalid, _u("loader"));
        }

        JavascriptLoader* loader = JavascriptLoader::FromVar(args[1]);
        JavascriptString* key = nullptr;
        Var ns = nullptr;

        if (args.Info.Count < 3)
        {
            key = library->GetUndefinedDisplayString();
        }
        else
        {
            key = JavascriptConversion::ToString(args[2], scriptContext);
        }

        if (args.Info.Count < 4)
        {
            ns = library->GetUndefined();
        }
        else
        {
            ns = args[3];
        }

        JavascriptModuleStatus* moduleStatus = library->CreateModuleStatus();
        if (isCtorSuperCall)
        {
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), moduleStatus, library->GetModuleStatusPrototype(), scriptContext);
        }
        moduleStatus->Initialize(loader, key, ns, scriptContext);

        return moduleStatus;
    }

    void JavascriptModuleStatus::Initialize(JavascriptLoader* loader, JavascriptString* key, Var ns, ScriptContext* scriptContext)
    {
        if (JavascriptOperators::IsUndefined(ns))
        {
            this->module = nullptr;

            // TODO:
            //Let deps be undefined.

            this->pipeline->Prepend(*JavascriptModuleStatusStageRecord::New(JavascriptModuleStatusStage::Instantiate, nullptr, scriptContext));
            this->pipeline->Prepend(*JavascriptModuleStatusStageRecord::New(JavascriptModuleStatusStage::Translate, nullptr, scriptContext));
            this->pipeline->Prepend(*JavascriptModuleStatusStageRecord::New(JavascriptModuleStatusStage::Fetch, nullptr, scriptContext));
        }
        else
        {
            if (!ModuleNamespace::Is(ns))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_Invalid, _u("ns"));
            }

            //Let module be ns.[[Module]].
            ModuleNamespace* namespaceObject = ModuleNamespace::FromVar(ns);
            this->module = namespaceObject;

            // TODO:
            //Let deps be a new empty List.

            JavascriptPromise* result = JavascriptPromise::CreateResolvedPromise(ns, scriptContext);
            this->pipeline->Prepend(*JavascriptModuleStatusStageRecord::New(JavascriptModuleStatusStage::Instantiate, result, scriptContext));
        }

        this->loader = loader;
        this->key = key;
        this->error = false;
    }

    Var JavascriptModuleStatus::EntryGetterStage(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (!JavascriptModuleStatus::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get ModuleStatus.prototype.stage"), _u("ModuleStatus"));
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
        JavascriptModuleStatusStageRecord* stageEntry = GetCurrentStage(entry, scriptContext);

        Assert(stageEntry != nullptr);

        switch (stageEntry->stage)
        {
        case JavascriptModuleStatusStage::Instantiate:
            return library->CreateStringFromCppLiteral(_u("instantiate"));
        case JavascriptModuleStatusStage::Fetch:
            return library->CreateStringFromCppLiteral(_u("fetch"));
        case JavascriptModuleStatusStage::Translate:
            return library->CreateStringFromCppLiteral(_u("translate"));
        default:
            Assert(false);
            return library->CreateStringFromCppLiteral(_u("invalid"));
        }
    }

    Var JavascriptModuleStatus::EntryGetterOriginalKey(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptModuleStatus::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get ModuleStatus.prototype.originalKey"), _u("ModuleStatus"));
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);

        return entry->key;
    }

    Var JavascriptModuleStatus::EntryGetterModule(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptModuleStatus::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get ModuleStatus.prototype.module"), _u("ModuleStatus"));
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);

        if (entry->module != nullptr)
        {
            return entry->module;
        }

        //TODO:
        //Let module be entry.[[Module]].
        //If module is a Module Record, return GetModuleNamespace(module).

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptModuleStatus::EntryGetterError(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptModuleStatus::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get ModuleStatus.prototype.error"), _u("ModuleStatus"));
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);

        return scriptContext->GetLibrary()->CreateBoolean(entry->error);
    }

    Var JavascriptModuleStatus::EntryGetterDependencies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (!JavascriptModuleStatus::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get ModuleStatus.prototype.dependencies"), _u("ModuleStatus"));
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
        JavascriptArray* array = library->CreateArray(0);


        // TODO
        array->SetItem(0, entry, PropertyOperation_None);

        //Let n be 0.
        //    For each pair in entry.[[Dependencies]], do:
        //Let O be ObjectCreate(%ObjectPrototype%).
        //    Let requestNameDesc be the PropertyDescriptor{ [[Value]]: pair.[[RequestName]],[[Writable]] : false,[[Enumerable]] : true,[[Configurable]] : false }.
        //    Perform ? DefinePropertyOrThrow(O, "requestName", requestNameDesc).
        //    Let moduleStatusDesc be the PropertyDescriptor{ [[Value]]: pair.[[ModuleStatus]],[[Writable]] : false,[[Enumerable]] : true,[[Configurable]] : false }.
        //    Perform ? DefinePropertyOrThrow(O, "entry", moduleStatusDesc).
        //    Perform ? CreateDataProperty(array, ? ToString(n), O).
        //    Increment n by 1.

        return array;
    }

    Var JavascriptModuleStatus::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptExceptionObject* exception;

        try
        {
            if (!JavascriptModuleStatus::Is(args[0]))
            {
                JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("ModuleStatus.prototype.load"), _u("ModuleStatus"));
            }

            JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
            JavascriptModuleStatusStage stage = JavascriptModuleStatusStage::Invalid;

            if (args.Info.Count < 2 || JavascriptOperators::IsUndefined(args[1]))
            {
                stage = JavascriptModuleStatusStage::Fetch;
            }
            else if (!IsValidStageValue(args[1], scriptContext, &stage))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("stage"));
            }

            Assert(stage != JavascriptModuleStatusStage::Invalid);

            return LoadModule(entry, stage, scriptContext);
        }
        catch (JavascriptExceptionObject* e)
        {
            exception = e;
        }

        // We can't fall out of the try unless we caught an exception
        Assert(exception != nullptr);

        return JavascriptPromise::CreateRejectedPromiseFromExceptionObject(exception, scriptContext);
    }

    Var JavascriptModuleStatus::EntryResult(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptExceptionObject* exception;

        try
        {
            if (!JavascriptModuleStatus::Is(args[0]))
            {
                JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("ModuleStatus.prototype.result"), _u("ModuleStatus"));
            }

            JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
            JavascriptModuleStatusStage stage = JavascriptModuleStatusStage::Invalid;

            if (args.Info.Count < 2 || JavascriptOperators::IsUndefined(args[1]) || !IsValidStageValue(args[1], scriptContext, &stage))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("stage"));
            }

            Assert(stage != JavascriptModuleStatusStage::Invalid);

            JavascriptModuleStatusStageRecord* stageEntry = nullptr;
            if (!GetStage(entry, stage, scriptContext, &stageEntry)
                 || stageEntry == nullptr
                 || stageEntry->result == nullptr
                )
            {
                return JavascriptPromise::CreateResolvedPromise(scriptContext->GetLibrary()->GetUndefined(), scriptContext);
            }

            return JavascriptPromise::CreatePassThroughPromise(stageEntry->result, scriptContext);
        }
        catch (JavascriptExceptionObject* e)
        {
            exception = e;
        }

        // We can't fall out of the try unless we caught an exception
        Assert(exception != nullptr);

        return JavascriptPromise::CreateRejectedPromiseFromExceptionObject(exception, scriptContext);
    }

    Var JavascriptModuleStatus::EntryResolve(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptExceptionObject* exception;

        try
        {
            if (!JavascriptModuleStatus::Is(args[0]))
            {
                JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("ModuleStatus.prototype.resolve"), _u("ModuleStatus"));
            }

            JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
            JavascriptModuleStatusStage stage = JavascriptModuleStatusStage::Invalid;
            JavascriptPromise* result;

            if (args.Info.Count < 2 || JavascriptOperators::IsUndefined(args[1]) || !IsValidStageValue(args[1], scriptContext, &stage))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("stage"));
            }
            // TODO: Confirm that we should throw TypeError if result argument is not a promise object ?
            //       Other option might be to create a promise to wrap result via Promise.resolve(result)
            if (args.Info.Count < 3 || !JavascriptPromise::Is(args[2]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise, _u("result"));
            }

            result = JavascriptPromise::FromVar(args[2]);

            Assert(stage != JavascriptModuleStatusStage::Invalid);

            JavascriptModuleStatusStageRecord* stageEntry = nullptr;

            if (!GetStage(entry, stage, scriptContext, &stageEntry))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_ModuleNotFound);
            }

            Assert(stageEntry != nullptr);

            UpgradeToStage(entry, stage, scriptContext);

            JavascriptLibrary* library = scriptContext->GetLibrary();
            Var p0 = JavascriptPromise::CreatePassThroughPromise(result, scriptContext);

            // TODO: Confirm we want to throw if creating a pass-through promise returns a non-promise object.
            //       Should we even allow this? We could make pass-through promise objects only construct via %Promise% 
            //       instead of using @@species.
            if (!JavascriptPromise::Is(p0))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise, _u("p0"));
            }

            JavascriptFunction* rejectionHandler = library->GetThrowerFunction();
            JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
                EntryResolveFulfillmentHandler,
                &JavascriptModuleStatus::EntryInfo::ResolveFulfillmentHandler,
                entry, 
                stage,
                nullptr);
            
            Var p1 = JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(p0), fulfillmentHandler, rejectionHandler, scriptContext);

            // TODO: Same note about p1 being a promise object.
            if (!JavascriptPromise::Is(p1))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise, _u("p1"));
            }

            JavascriptPromise* p1Promise = JavascriptPromise::FromVar(p1);
            JavascriptModuleStatusErrorHandlerFunction* errorHandler = library->CreateModuleStatusErrorHandlerFunction(EntryErrorHandler, entry);
            JavascriptFunction* identityFunction = library->GetIdentityFunction();

            JavascriptPromise::CreateThenPromise(p1Promise, identityFunction, errorHandler, scriptContext);

            if (stageEntry->result == nullptr)
            {
                stageEntry->result = p1Promise;
            }

            return p1Promise;
        }
        catch (JavascriptExceptionObject* e)
        {
            exception = e;
        }

        // We can't fall out of the try unless we caught an exception
        Assert(exception != nullptr);

        return JavascriptPromise::CreateRejectedPromiseFromExceptionObject(exception, scriptContext);
    }

    Var JavascriptModuleStatus::EntryReject(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptExceptionObject* exception;

        try
        {
            if (!JavascriptModuleStatus::Is(args[0]))
            {
                JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("ModuleStatus.prototype.reject"), _u("ModuleStatus"));
            }

            JavascriptModuleStatus* entry = JavascriptModuleStatus::FromVar(args[0]);
            JavascriptModuleStatusStage stage = JavascriptModuleStatusStage::Invalid;
            JavascriptPromise* error;

            if (args.Info.Count < 2 || JavascriptOperators::IsUndefined(args[1]) || !IsValidStageValue(args[1], scriptContext, &stage))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("stage"));
            }
            // TODO: Confirm that we should throw TypeError if error argument is not a promise object ?
            //       Other option might be to create a promise to wrap error via Promise.reject(error)
            if (args.Info.Count < 3 || !JavascriptPromise::Is(args[2]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise, _u("error"));
            }

            error = JavascriptPromise::FromVar(args[2]);

            Assert(stage != JavascriptModuleStatusStage::Invalid);

            JavascriptModuleStatusStageRecord* stageEntry = nullptr;

            if (!GetStage(entry, stage, scriptContext, &stageEntry))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_ModuleNotFound);
            }

            Assert(stageEntry != nullptr);

            UpgradeToStage(entry, stage, scriptContext);

            Var p0 = JavascriptPromise::CreatePassThroughPromise(error, scriptContext);

            // TODO: Should we throw here?
            if (!JavascriptPromise::Is(p0))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
            }

            JavascriptLibrary* library = scriptContext->GetLibrary();
            JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
                EntryResolveFulfillmentHandler,
                &JavascriptModuleStatus::EntryInfo::ResolveFulfillmentHandler,
                entry,
                stage,
                nullptr);
            
            Var p1 = JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(p0), fulfillmentHandler, library->GetThrowerFunction(), scriptContext);

            // TODO: Should we throw here?
            if (!JavascriptPromise::Is(p1))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
            }

            JavascriptModuleStatusErrorHandlerFunction* errorHandler = library->CreateModuleStatusErrorHandlerFunction(EntryErrorHandler, entry);
            JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(p1), library->GetIdentityFunction(), errorHandler, scriptContext);

            if (stageEntry->result == nullptr)
            {
                stageEntry->result = JavascriptPromise::FromVar(p1);
            }

            return p1;
        }
        catch (JavascriptExceptionObject* e)
        {
            exception = e;
        }

        // We can't fall out of the try unless we caught an exception
        Assert(exception != nullptr);

        return JavascriptPromise::CreateRejectedPromiseFromExceptionObject(exception, scriptContext);
    }

    BOOL JavascriptModuleStatus::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("ModuleStatus"));
        return TRUE;
    }

    Var JavascriptModuleStatus::EntryResolveFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var value;

        if (args.Info.Count > 1)
        {
            value = args[1];
        }
        else
        {
            value = library->GetUndefined();
        }

        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();

        if (stage == JavascriptModuleStatusStage::Instantiate)
        {
            JavascriptPromise* pSatisfyInstance = SatisfyInstance(entry, value, nullptr, nullptr, scriptContext);
            JavascriptFunction* throwerFunction = library->GetThrowerFunction();
            JavascriptModuleStatusFulfillmentHandlerFunction* postSatisfyInstanceFulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
                EntryPostSatisfyInstanceFulfillmentHandler, 
                &JavascriptModuleStatus::EntryInfo::PostSatisfyInstanceFulfillmentHandler,
                entry, 
                stage, 
                value);

            return JavascriptPromise::CreateThenPromise(pSatisfyInstance, postSatisfyInstanceFulfillmentHandler, throwerFunction, scriptContext);
        }
        else 
        {
            JavascriptModuleStatusStageRecord* stageEntry = nullptr;
            if (!GetStage(entry, stage, scriptContext, &stageEntry))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_CannotResolveModule);
            }

            Assert(stageEntry != nullptr);
            Assert(stageEntry->result != nullptr);

            stageEntry->result->Resolve(value, scriptContext);
        }

        return library->GetUndefined();
    }

    Var JavascriptModuleStatus::EntryRejectFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var value;

        if (args.Info.Count > 1)
        {
            value = args[1];
        }
        else
        {
            value = library->GetUndefined();
        }

        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();
        JavascriptModuleStatusStageRecord* stageEntry = nullptr;

        if (!GetStage(entry, stage, scriptContext, &stageEntry))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_CannotResolveModule);
        }

        Assert(stageEntry != nullptr);
        Assert(stageEntry->result != nullptr);

        stageEntry->result->Reject(value, scriptContext);

        return library->GetUndefined();
    }

    Var JavascriptModuleStatus::EntryErrorHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptModuleStatusErrorHandlerFunction* errorHandler = JavascriptModuleStatusErrorHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = errorHandler->GetModuleStatus();

        Assert(entry != nullptr);

        entry->error = true;

        return library->GetUndefined();
    }

    Var JavascriptModuleStatus::EntrySatisfyInstanceWrapperFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        Var instantiateSet = fulfillmentHandler->GetInstantiateSet();
        Var source = fulfillmentHandler->GetValue();
        Var optionalInstance;

        if (args.Info.Count > 1)
        {
            optionalInstance = args[1];
        }
        else
        {
            optionalInstance = library->GetUndefined();
        }

        JavascriptPromise* result = SatisfyInstance(entry, optionalInstance, source, instantiateSet, scriptContext);

        fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
            EntryPostSatisfyInstanceSimpleFulfillmentHandler,
            &JavascriptModuleStatus::EntryInfo::PostSatisfyInstanceSimpleFulfillmentHandler,
            entry,
            JavascriptModuleStatusStage::Invalid,
            optionalInstance);

        return JavascriptPromise::CreateThenPromise(result, fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
    }

    Var JavascriptModuleStatus::EntryPostSatisfyInstanceSimpleFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var instance;

        if (args.Info.Count > 1)
        {
            instance = args[1];
        }
        else
        {
            instance = library->GetUndefined();
        }

        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        Var optionalInstance = fulfillmentHandler->GetValue();

        entry->module = instance;

        return optionalInstance;
    }

    Var JavascriptModuleStatus::EntryPostSatisfyInstanceFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();
        Var instance;

        if (args.Info.Count > 1)
        {
            instance = args[1];
        }
        else
        {
            instance = library->GetUndefined();
        }

        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();
        Var value = fulfillmentHandler->GetValue();

        entry->module = instance;
        JavascriptModuleStatusStageRecord* stageEntry = nullptr;
        GetStage(entry, stage, scriptContext, &stageEntry);

        Assert(stageEntry != nullptr);

        stageEntry->result = JavascriptPromise::FromVar(value);

        return library->GetUndefined();
    }

    Var JavascriptModuleStatus::EntryUpgradeToStageFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var payload;

        if (args.Info.Count > 1)
        {
            payload = args[1];
        }
        else
        {
            payload = scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();

        UpgradeToStage(entry, stage, scriptContext);

        return payload;
    }

    Var JavascriptModuleStatus::EntryRequestTranslateOrInstantiateFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        Var payload;

        if (args.Info.Count > 1)
        {
            payload = args[1];
        }
        else
        {
            payload = scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptModuleStatusFulfillmentHandlerFunction* fulfillmentHandler = JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();
        Var instantiateSet = fulfillmentHandler->GetValue();
        Js::PropertyId propertyId;

        if (stage == JavascriptModuleStatusStage::Translate)
        {
            propertyId = Js::PropertyIds::_symbolTranslate;
            fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
                EntryUpgradeToStageFulfillmentHandler, 
                &JavascriptModuleStatus::EntryInfo::UpgradeToStageFulfillmentHandler, 
                entry, 
                JavascriptModuleStatusStage::Instantiate, 
                nullptr);
        }
        else
        {
            Assert(stage == JavascriptModuleStatusStage::Instantiate);

            propertyId = Js::PropertyIds::_symbolInstantiate;
            fulfillmentHandler = library->CreateModuleStatusFulfillmentHandlerFunction(
                EntrySatisfyInstanceWrapperFulfillmentHandler, 
                &JavascriptModuleStatus::EntryInfo::SatisfyInstanceWrapperFulfillmentHandler, 
                entry, 
                JavascriptModuleStatusStage::Invalid, 
                payload);
            fulfillmentHandler->SetInstantiateSet(instantiateSet);
        }

        Var hookVar = JavascriptOperators::GetProperty(entry->loader, propertyId, scriptContext);
        if (!JavascriptConversion::IsCallable(hookVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction);
        }

        RecyclableObject* hook = RecyclableObject::FromVar(hookVar);

        Var hookResult = CALL_FUNCTION(hook, Js::CallInfo(CallFlags_Value, 3),
            hook,
            entry,
            payload);

        // TODO: Confirm we should throw here? What does "promise-call the hook function" mean?
        if (!JavascriptPromise::Is(hookResult))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        return JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(hookResult), fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
    }
    
    JavascriptModuleStatusErrorHandlerFunction::JavascriptModuleStatusErrorHandlerFunction(DynamicType* type)
        : RuntimeFunction(type, &Js::JavascriptModuleStatus::EntryInfo::ErrorHandler), entry(nullptr)
    { }

    JavascriptModuleStatusErrorHandlerFunction::JavascriptModuleStatusErrorHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptModuleStatus* entry)
        : RuntimeFunction(type, functionInfo), entry(entry)
    { }

    bool JavascriptModuleStatusErrorHandlerFunction::Is(Var var)
    {
        if (JavascriptFunction::Is(var))
        {
            JavascriptFunction* obj = JavascriptFunction::FromVar(var);

            return VirtualTableInfo<JavascriptModuleStatusErrorHandlerFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptModuleStatusErrorHandlerFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptModuleStatusErrorHandlerFunction* JavascriptModuleStatusErrorHandlerFunction::FromVar(Var var)
    {
        Assert(JavascriptModuleStatusErrorHandlerFunction::Is(var));

        return static_cast<JavascriptModuleStatusErrorHandlerFunction*>(var);
    }

    JavascriptModuleStatus* JavascriptModuleStatusErrorHandlerFunction::GetModuleStatus()
    {
        return this->entry;
    }

#if ENABLE_TTD
    void JavascriptModuleStatusErrorHandlerFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        AssertMsg(this->entry != nullptr, "Was not expecting that!!!");

        extractor->MarkVisitVar(this->entry);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptModuleStatusErrorHandlerFunction::GetSnapTag_TTD() const
    {
        // TODO
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject;
    }

    void JavascriptModuleStatusErrorHandlerFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        //TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo* sprri = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo>();

        //uint32 depOnCount = 1;
        //TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        //sprri->PromiseId = TTD_CONVERT_VAR_TO_PTR_ID(this->promise);
        //depOnArray[0] = sprri->PromiseId;

        //sprri->IsReject = this->isReject;

        //sprri->AlreadyResolvedWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->alreadyResolvedWrapper);
        //sprri->AlreadyResolvedValue = this->alreadyResolvedWrapper->alreadyResolved;

        //TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(objData, sprri, alloc, depOnCount, depOnArray);
    }
#endif

    JavascriptModuleStatusFulfillmentHandlerFunction::JavascriptModuleStatusFulfillmentHandlerFunction(DynamicType* type)
        : RuntimeFunction(type, &Js::JavascriptModuleStatus::EntryInfo::ResolveFulfillmentHandler), entry(nullptr), stage(JavascriptModuleStatusStage::Invalid), value(nullptr)
    { }

    JavascriptModuleStatusFulfillmentHandlerFunction::JavascriptModuleStatusFulfillmentHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptModuleStatus* entry, JavascriptModuleStatusStage stage, Var value)
        : RuntimeFunction(type, functionInfo), entry(entry), stage(stage), value(value)
    { }

    bool JavascriptModuleStatusFulfillmentHandlerFunction::Is(Var var)
    {
        if (JavascriptFunction::Is(var))
        {
            JavascriptFunction* obj = JavascriptFunction::FromVar(var);

            return VirtualTableInfo<JavascriptModuleStatusFulfillmentHandlerFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptModuleStatusFulfillmentHandlerFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptModuleStatusFulfillmentHandlerFunction* JavascriptModuleStatusFulfillmentHandlerFunction::FromVar(Var var)
    {
        Assert(JavascriptModuleStatusFulfillmentHandlerFunction::Is(var));

        return static_cast<JavascriptModuleStatusFulfillmentHandlerFunction*>(var);
    }

    JavascriptModuleStatus* JavascriptModuleStatusFulfillmentHandlerFunction::GetModuleStatus()
    {
        return this->entry;
    }

    JavascriptModuleStatusStage JavascriptModuleStatusFulfillmentHandlerFunction::GetStage()
    {
        return this->stage;
    }

    Var JavascriptModuleStatusFulfillmentHandlerFunction::GetValue()
    {
        return this->value;
    }

    Var JavascriptModuleStatusFulfillmentHandlerFunction::GetInstantiateSet()
    {
        return this->instantiateSet;
    }

    void JavascriptModuleStatusFulfillmentHandlerFunction::SetInstantiateSet(Var value)
    {
        this->instantiateSet = value;
    }
    
#if ENABLE_TTD
    void JavascriptModuleStatusFulfillmentHandlerFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        AssertMsg(this->entry != nullptr, "Was not expecting that!!!");

        extractor->MarkVisitVar(this->entry);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptModuleStatusFulfillmentHandlerFunction::GetSnapTag_TTD() const
    {
        // TODO
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject;
    }

    void JavascriptModuleStatusFulfillmentHandlerFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        //TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo* sprri = alloc.SlabAllocateStruct<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo>();

        //uint32 depOnCount = 1;
        //TTD_PTR_ID* depOnArray = alloc.SlabAllocateArray<TTD_PTR_ID>(depOnCount);

        //sprri->PromiseId = TTD_CONVERT_VAR_TO_PTR_ID(this->promise);
        //depOnArray[0] = sprri->PromiseId;

        //sprri->IsReject = this->isReject;

        //sprri->AlreadyResolvedWrapperId = TTD_CONVERT_PROMISE_INFO_TO_PTR_ID(this->alreadyResolvedWrapper);
        //sprri->AlreadyResolvedValue = this->alreadyResolvedWrapper->alreadyResolved;

        //TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapPromiseResolveOrRejectFunctionInfo*, TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject>(objData, sprri, alloc, depOnCount, depOnArray);
    }
#endif
}
