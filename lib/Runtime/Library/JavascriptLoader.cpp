//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    JavascriptLoader::JavascriptLoader(DynamicType* type)
        : DynamicObject(type), registry(nullptr)
    {
    }

    JavascriptLoader* JavascriptLoader::New(ScriptContext* scriptContext)
    {
        JavascriptLoader* loader = scriptContext->GetLibrary()->CreateLoader();

        loader->registry = JavascriptRegistry::New(scriptContext);

        return loader;
    }

    bool JavascriptLoader::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_Loader;
    }

    JavascriptLoader* JavascriptLoader::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptLoader'");

        return static_cast<JavascriptLoader*>(RecyclableObject::FromVar(aValue));
    }

    JavascriptRegistry* JavascriptLoader::GetRegistry()
    {
        return this->registry;
    }

    JavascriptPromise* JavascriptLoader::Resolve(JavascriptLoader* loader, Var name, Var referrer, ScriptContext* scriptContext)
    {
        Var fetchVar = JavascriptOperators::GetProperty(loader, Js::PropertyIds::_symbolResolve, scriptContext);
        if (!JavascriptConversion::IsCallable(fetchVar))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction);
        }

        RecyclableObject* hook = RecyclableObject::FromVar(fetchVar);

        Var hookResult = CALL_FUNCTION(hook, Js::CallInfo(CallFlags_Value, 3),
            hook,
            name,
            referrer);

        // TODO: Confirm we should throw here? What does "promise-call the hook function" mean?
        if (!JavascriptPromise::Is(hookResult))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }
        return JavascriptPromise::FromVar(hookResult);
    }

    void JavascriptLoader::ExtractDependencies(JavascriptModuleStatus* entry, Var instance, ScriptContext* scriptContext)
    {
        // TODO
        return;
    }

    Var JavascriptLoader::Instantiation(JavascriptLoader* loader, Var optionalInstance, Var source, ScriptContext* scriptContext)
    {
        // TODO
        return nullptr;
    }

    Var JavascriptLoader::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Loader"));

        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr
            || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);

        if ((callInfo.Flags & CallFlags_New) != CallFlags_New || (newTarget != nullptr && JavascriptOperators::IsUndefined(newTarget)))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("Loader"));
        }

        JavascriptLoader* loader = JavascriptLoader::New(scriptContext);

        if (isCtorSuperCall)
        {
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), loader, scriptContext->GetLibrary()->GetLoaderPrototype(), scriptContext);
        }

        return loader;
    }

    Var JavascriptLoader::EntryImport(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (!JavascriptLoader::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Reflect.Loader.prototype.import"), _u("Loader"));
        }

        JavascriptLoader* loader = JavascriptLoader::FromVar(args[0]);
        Var name;
        Var referrer;

        if (args.Info.Count > 1)
        {
            name = args[1];
        }
        else
        {
            name = library->GetUndefined();
        }
        if (args.Info.Count > 2)
        {
            referrer = args[2];
        }
        else
        {
            referrer = library->GetUndefined();
        }

        JavascriptPromise* result = Resolve(loader, name, referrer, scriptContext);
        JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleLoaderFulfillmentHandlerFunction(
            EntryEnsureRegisteredAndEvaluatedFulfillmentHandler,
            &JavascriptLoader::EntryInfo::EnsureRegisteredAndEvaluatedFulfillmentHandler,
            loader,
            JavascriptModuleStatusStage_Instantiate);

        return JavascriptPromise::CreateThenPromise(result, fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
    }
    
    Var JavascriptLoader::EntryResolve(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (!JavascriptLoader::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Reflect.Loader.prototype.resolve"), _u("Loader"));
        }

        JavascriptLoader* loader = JavascriptLoader::FromVar(args[0]);
        Var name;
        Var referrer;

        if (args.Info.Count > 1)
        {
            name = args[1];
        }
        else
        {
            name = library->GetUndefined();
        }
        if (args.Info.Count > 2)
        {
            referrer = args[2];
        }
        else
        {
            referrer = library->GetUndefined();
        }

        JavascriptPromise* result = Resolve(loader, name, referrer, scriptContext);

        return JavascriptPromise::CreatePassThroughPromise(result, scriptContext);
    }

    Var JavascriptLoader::EntryLoad(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptLoader::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("Reflect.Loader.prototype.load"), _u("Loader"));
        }

        JavascriptExceptionObject* exception = nullptr;

        try
        {
            JavascriptLibrary* library = scriptContext->GetLibrary();
            JavascriptLoader* loader = JavascriptLoader::FromVar(args[0]);
            JavascriptModuleStatusStage stage = JavascriptModuleStatusStage_Invalid;
            Var name;
            Var referrer;

            if (args.Info.Count > 1)
            {
                name = args[1];
            }
            else
            {
                name = library->GetUndefined();
            }
            if (args.Info.Count > 2)
            {
                referrer = args[2];
            }
            else
            {
                referrer = library->GetUndefined();
            }
            if (args.Info.Count < 4 || JavascriptOperators::IsUndefined(args[3]))
            {
                stage = JavascriptModuleStatusStage_Instantiate;
            }
            if (!JavascriptModuleStatus::IsValidStageValue(args[1], scriptContext, &stage))
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgumentOutOfRange, _u("stage"));
            }

            Assert(stage != JavascriptModuleStatusStage_Invalid);

            JavascriptPromise* result = Resolve(loader, name, referrer, scriptContext);

            JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleLoaderFulfillmentHandlerFunction(
                EntryEnsureRegisteredFulfillmentHandler,
                &JavascriptLoader::EntryInfo::EnsureRegisteredFulfillmentHandler,
                loader,
                stage);

            return JavascriptPromise::CreateThenPromise(result, fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
        }
        catch (JavascriptExceptionObject* e)
        {
            exception = e;
        }

        Assert(exception != nullptr);

        return JavascriptPromise::CreateRejectedPromiseFromExceptionObject(exception, scriptContext);
    }

    Var JavascriptLoader::EntryGetterRegistry(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        if (!JavascriptLoader::Is(args[0]))
        {
            JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedObjectOfType, _u("get Reflect.Loader.prototype.registry"), _u("Loader"));
        }

        JavascriptLoader* loader = JavascriptLoader::FromVar(args[0]);

        return loader->registry;
    }

    Var EnsureRegisteredFulfillmentHandlerHelper(ArgumentReader& args, JavascriptLoader* loader, JavascriptModuleStatusStage stage, bool ensureEvaluated, ScriptContext* scriptContext)
    {
        Var key;

        if (args.Info.Count > 1)
        {
            key = args[1];
        }
        else
        {
            key = scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptModuleStatus* entry = JavascriptModuleStatus::EnsureRegistered(loader, key, scriptContext);
        Var result = JavascriptModuleStatus::LoadModule(entry, stage, scriptContext);

        if (!ensureEvaluated)
        {
            return result;
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();

        // TODO: Is it required this should be a promise?
        if (!JavascriptPromise::Is(result))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedPromise);
        }

        JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = library->CreateModuleLoaderFulfillmentHandlerFunction(
            JavascriptLoader::EntryEnsureEvaluatedFulfillmentHandler,
            &JavascriptLoader::EntryInfo::EnsureEvaluatedFulfillmentHandler,
            entry,
            JavascriptModuleStatusStage_Invalid);

        return JavascriptPromise::CreateThenPromise(JavascriptPromise::FromVar(result), fulfillmentHandler, library->GetThrowerFunction(), scriptContext);
    }

    Var JavascriptLoader::EntryEnsureRegisteredFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = JavascriptLoaderFulfillmentHandlerFunction::FromVar(function);
        JavascriptLoader* loader = fulfillmentHandler->GetLoader();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();

        return EnsureRegisteredFulfillmentHandlerHelper(args, loader, stage, false, scriptContext);
    }

    Var JavascriptLoader::EntryEnsureRegisteredAndEvaluatedFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = JavascriptLoaderFulfillmentHandlerFunction::FromVar(function);
        JavascriptLoader* loader = fulfillmentHandler->GetLoader();
        JavascriptModuleStatusStage stage = fulfillmentHandler->GetStage();

        return EnsureRegisteredFulfillmentHandlerHelper(args, loader, stage, true, scriptContext);
    }

    Var JavascriptLoader::EntryEnsureEvaluatedFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);
        ARGUMENTS(args, callInfo);
        Assert(!(callInfo.Flags & CallFlags_New));

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLoaderFulfillmentHandlerFunction* fulfillmentHandler = JavascriptLoaderFulfillmentHandlerFunction::FromVar(function);
        JavascriptModuleStatus* entry = fulfillmentHandler->GetModuleStatus();

        return JavascriptModuleStatus::EnsureEvaluated(entry, scriptContext);
    }

    BOOL JavascriptLoader::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Loader"));
        return TRUE;
    }

    JavascriptLoaderFulfillmentHandlerFunction::JavascriptLoaderFulfillmentHandlerFunction(DynamicType* type)
        : RuntimeFunction(type, &Js::JavascriptLoader::EntryInfo::EnsureRegisteredAndEvaluatedFulfillmentHandler), loaderOrEntry(nullptr), stage(JavascriptModuleStatusStage_Invalid)
    { }

    JavascriptLoaderFulfillmentHandlerFunction::JavascriptLoaderFulfillmentHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, Var loaderOrEntry, JavascriptModuleStatusStage stage)
        : RuntimeFunction(type, functionInfo), loaderOrEntry(loaderOrEntry), stage(stage)
    { }

    bool JavascriptLoaderFulfillmentHandlerFunction::Is(Var var)
    {
        if (JavascriptFunction::Is(var))
        {
            JavascriptFunction* obj = JavascriptFunction::FromVar(var);

            return VirtualTableInfo<JavascriptLoaderFulfillmentHandlerFunction>::HasVirtualTable(obj)
                || VirtualTableInfo<CrossSiteObject<JavascriptLoaderFulfillmentHandlerFunction>>::HasVirtualTable(obj);
        }

        return false;
    }

    JavascriptLoaderFulfillmentHandlerFunction* JavascriptLoaderFulfillmentHandlerFunction::FromVar(Var var)
    {
        Assert(JavascriptLoaderFulfillmentHandlerFunction::Is(var));

        return static_cast<JavascriptLoaderFulfillmentHandlerFunction*>(var);
    }

    JavascriptLoader* JavascriptLoaderFulfillmentHandlerFunction::GetLoader()
    {
        return JavascriptLoader::FromVar(this->loaderOrEntry);
    }

    JavascriptModuleStatusStage JavascriptLoaderFulfillmentHandlerFunction::GetStage()
    {
        return this->stage;
    }

    JavascriptModuleStatus* JavascriptLoaderFulfillmentHandlerFunction::GetModuleStatus()
    {
        return JavascriptModuleStatus::FromVar(this->loaderOrEntry);
    }

#if ENABLE_TTD
    void JavascriptLoaderFulfillmentHandlerFunction::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        AssertMsg(this->loaderOrEntry != nullptr, "Was not expecting that!!!");

        extractor->MarkVisitVar(this->loaderOrEntry);
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptLoaderFulfillmentHandlerFunction::GetSnapTag_TTD() const
    {
        // TODO
        return TTD::NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject;
    }

    void JavascriptLoaderFulfillmentHandlerFunction::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
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
