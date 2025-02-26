//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "Library/StackScriptFunction.h"
#include "Types/SpreadArgument.h"

#include "Language/AsmJsTypes.h"
#ifdef _M_X64
#include "ByteCode/PropertyIdArray.h"
#include "Language/AsmJsModule.h"
#endif

#ifdef _M_IX86
#ifdef _CONTROL_FLOW_GUARD
extern "C" PVOID __guard_check_icall_fptr;
#endif
extern "C" void __cdecl _alloca_probe_16();
#endif

using namespace Js;

    // The VS2013 linker treats this as a redefinition of an already
    // defined constant and complains. So skip the declaration if we're compiling
    // with VS2013 or below.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    const charcount_t JavascriptFunction::DIAG_MAX_FUNCTION_STRING;
#endif

    DEFINE_RECYCLER_TRACKER_PERF_COUNTER(JavascriptFunction);
    JavascriptFunction::JavascriptFunction(DynamicType * type)
        : DynamicObject(type), functionInfo(nullptr), constructorCache(&ConstructorCache::DefaultInstance)
    {
        Assert(this->constructorCache != nullptr);
    }


    JavascriptFunction::JavascriptFunction(DynamicType * type, FunctionInfo * functionInfo)
        : DynamicObject(type), functionInfo(functionInfo), constructorCache(&ConstructorCache::DefaultInstance)
    {
        Assert(this->constructorCache != nullptr);
        this->GetTypeHandler()->ClearHasOnlyWritableDataProperties(); // length is non-writable
        if (GetTypeHandler()->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {
            // No need to invalidate store field caches for non-writable properties here. Since this type is just being created, it cannot represent
            // an object that is already a prototype. If it becomes a prototype and then we attempt to add a property to an object derived from this
            // object, then we will check if this property is writable, and only if it is will we do the fast path for add property.
            // GetScriptContext()->InvalidateStoreFieldCaches(PropertyIds::length);
            GetLibrary()->GetTypesWithOnlyWritablePropertyProtoChainCache()->Clear();
        }
    }

    JavascriptFunction::JavascriptFunction(DynamicType * type, FunctionInfo * functionInfo, ConstructorCache* cache)
        : DynamicObject(type), functionInfo(functionInfo), constructorCache(cache)
    {
        Assert(this->constructorCache != nullptr);
        this->GetTypeHandler()->ClearHasOnlyWritableDataProperties(); // length is non-writable
        if (GetTypeHandler()->GetFlags() & DynamicTypeHandler::IsPrototypeFlag)
        {
            // No need to invalidate store field caches for non-writable properties here. Since this type is just being created, it cannot represent
            // an object that is already a prototype. If it becomes a prototype and then we attempt to add a property to an object derived from this
            // object, then we will check if this property is writable, and only if it is will we do the fast path for add property.
            // GetScriptContext()->InvalidateStoreFieldCaches(PropertyIds::length);
            GetLibrary()->GetTypesWithOnlyWritablePropertyProtoChainCache()->Clear();
        }
    }

    FunctionProxy *JavascriptFunction::GetFunctionProxy() const
    {
        Assert(functionInfo != nullptr);
        return functionInfo->GetFunctionProxy();
    }

    ParseableFunctionInfo *JavascriptFunction::GetParseableFunctionInfo() const
    {
        Assert(functionInfo != nullptr);
        return functionInfo->GetParseableFunctionInfo();
    }

    DeferDeserializeFunctionInfo *JavascriptFunction::GetDeferDeserializeFunctionInfo() const
    {
        Assert(functionInfo != nullptr);
        return functionInfo->GetDeferDeserializeFunctionInfo();
    }

    FunctionBody *JavascriptFunction::GetFunctionBody() const
    {
        Assert(functionInfo != nullptr);
        return functionInfo->GetFunctionBody();
    }

    BOOL JavascriptFunction::IsScriptFunction() const
    {
        Assert(functionInfo != nullptr);
        return functionInfo->HasBody();
    }

    template <> bool Js::VarIsImpl<JavascriptFunction>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_Function;
    }

    BOOL JavascriptFunction::IsStrictMode() const
    {
        FunctionProxy * proxy = this->GetFunctionProxy();
        return proxy && proxy->EnsureDeserialized()->GetIsStrictMode();
    }

    BOOL JavascriptFunction::IsLambda() const
    {
        return this->GetFunctionInfo()->IsLambda();
    }

    BOOL JavascriptFunction::IsConstructor() const
    {
        return this->GetFunctionInfo()->IsConstructor();
    }

#if DBG
    /* static */
    bool JavascriptFunction::IsBuiltinProperty(Var objectWithProperty, PropertyIds propertyId)
    {
        return VarIs<ScriptFunctionBase>(objectWithProperty)
            && (propertyId == PropertyIds::length || (VarTo<JavascriptFunction>(objectWithProperty)->HasRestrictedProperties() && (propertyId == PropertyIds::arguments || propertyId == PropertyIds::caller)));
    }
#endif

    Var JavascriptFunction::NewInstanceHelper(ScriptContext *scriptContext, RecyclableObject* function, CallInfo callInfo, Js::ArgumentReader& args, FunctionKind functionKind /* = FunctionKind::Normal */)
    {
        JavascriptLibrary* library = function->GetLibrary();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        bool isAsync = functionKind == FunctionKind::Async || functionKind == FunctionKind::AsyncGenerator;
        bool isGenerator = functionKind == FunctionKind::Generator || functionKind == FunctionKind::AsyncGenerator;

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = args.GetNewTarget();
        bool isCtorSuperCall = JavascriptOperators::GetAndAssertIsConstructorSuperCall(args);

        JavascriptString* separator = library->GetCommaDisplayString();

        // Gather all the formals into a string like (fml1, fml2, fml3)
        JavascriptString *formals = library->GetOpenRBracketString();

        for (uint i = 1; i < args.Info.Count - 1; ++i)
        {
            if (i != 1)
            {
                formals = JavascriptString::Concat(formals, separator);
            }
            formals = JavascriptString::Concat(formals, JavascriptConversion::ToString(args.Values[i], scriptContext));
        }
        formals = JavascriptString::Concat(formals, library->GetNewLineCloseRBracketString());
        // Function body, last argument to Function(...)
        JavascriptString *fnBody = NULL;
        if (args.Info.Count > 1)
        {
            fnBody = JavascriptConversion::ToString(args.Values[args.Info.Count - 1], scriptContext);
        }

        // Create a string representing the anonymous function
        Assert(
            0 + // "function anonymous" GetFunctionAnonymousString
            0 + // "("   GetOpenRBracketString
            1 + // "\n)" GetNewLineCloseRBracketString
            0 // " {"  GetSpaceOpenBracketString
            == numberLinesPrependedToAnonymousFunction); // Be sure to add exactly one line to anonymous function

        JavascriptString *bs = functionKind == FunctionKind::Async ?
            library->GetAsyncFunctionAnonymousString() :
            functionKind == FunctionKind::Generator ?
            library->GetFunctionPTRAnonymousString() :
            functionKind == FunctionKind::AsyncGenerator ?
            library->GetAsyncGeneratorAnonymousString() :
            library->GetFunctionAnonymousString();

        bs = JavascriptString::Concat(bs, formals);
        bs = JavascriptString::Concat(bs, library->GetSpaceOpenBracketString());
        if (fnBody != NULL)
        {
            bs = JavascriptString::Concat(bs, fnBody);
        }

        bs = JavascriptString::Concat(bs, library->GetNewLineCloseBracketString());
        // Bug 1105479. Get the module id from the caller
        ModuleID moduleID = kmodGlobal;

        BOOL strictMode = FALSE;

        JavascriptFunction* pfuncScript;
        FunctionInfo *pfuncInfoCache = NULL;
        char16 const * sourceString = bs->GetSz();
        charcount_t sourceLen = bs->GetLength();
        EvalMapString key(bs, sourceString, sourceLen, moduleID, strictMode, /* isLibraryCode = */ false);
        if (!scriptContext->IsInNewFunctionMap(key, &pfuncInfoCache))
        {
            // Validate formals here
            scriptContext->GetGlobalObject()->ValidateSyntax(
                scriptContext, formals->GetSz(), formals->GetLength(),
                isGenerator, isAsync,
                &Parser::ValidateFormals);
            if (fnBody != NULL)
            {
                // Validate function body
                scriptContext->GetGlobalObject()->ValidateSyntax(
                    scriptContext, fnBody->GetSz(), fnBody->GetLength(),
                    isGenerator, isAsync,
                    &Parser::ValidateSourceElementList);
            }

            pfuncScript = scriptContext->GetGlobalObject()->EvalHelper(scriptContext, sourceString, sourceLen, moduleID, fscrCanDeferFncParse, Constants::FunctionCode, TRUE, TRUE, strictMode);

            // Indicate that this is a top-level function. We don't pass the fscrGlobalCode flag to the eval helper,
            // or it will return the global function that wraps the declared function body, as though it were an eval.
            // But we want, for instance, to be able to verify that we did the right amount of deferred parsing.
            ParseableFunctionInfo *functionInfo = pfuncScript->GetParseableFunctionInfo();
            Assert(functionInfo);
            functionInfo->SetGrfscr(functionInfo->GetGrfscr() | fscrGlobalCode);

#if ENABLE_TTD
            if(!scriptContext->IsTTDRecordOrReplayModeEnabled())
            {
                scriptContext->AddToNewFunctionMap(key, functionInfo->GetFunctionInfo());
            }
#else
            scriptContext->AddToNewFunctionMap(key, functionInfo->GetFunctionInfo());
#endif
        }
        else if (pfuncInfoCache->IsCoroutine())
        {
            pfuncScript = scriptContext->GetLibrary()->CreateGeneratorVirtualScriptFunction(pfuncInfoCache->GetFunctionProxy());
        }
        else
        {
            pfuncScript = scriptContext->GetLibrary()->CreateScriptFunction(pfuncInfoCache->GetFunctionProxy());
        }

#if ENABLE_TTD
        //
        //TODO: We may (probably?) want to use the debugger source rundown functionality here instead
        //
        if(pfuncScript != nullptr && (scriptContext->IsTTDRecordModeEnabled() || scriptContext->ShouldPerformReplayAction()))
        {
            //Make sure we have the body and text information available
            FunctionBody* globalBody = TTD::JsSupport::ForceAndGetFunctionBody(pfuncScript->GetParseableFunctionInfo());
            if(!scriptContext->TTDContextInfo->IsBodyAlreadyLoadedAtTopLevel(globalBody))
            {
                uint32 bodyIdCtr = 0;

                if(scriptContext->IsTTDRecordModeEnabled())
                {
                    const TTD::NSSnapValues::TopLevelNewFunctionBodyResolveInfo* tbfi = scriptContext->GetThreadContext()->TTDLog->AddNewFunction(globalBody, moduleID, sourceString, sourceLen);

                    //We always want to register the top-level load but we don't always need to log the event
                    if(scriptContext->ShouldPerformRecordAction())
                    {
                        scriptContext->GetThreadContext()->TTDLog->RecordTopLevelCodeAction(tbfi->TopLevelBase.TopLevelBodyCtr);
                    }

                    bodyIdCtr = tbfi->TopLevelBase.TopLevelBodyCtr;
                }

                if(scriptContext->ShouldPerformReplayAction())
                {
                    bodyIdCtr = scriptContext->GetThreadContext()->TTDLog->ReplayTopLevelCodeAction();
                }

                //walk global body to (1) add functions to pin set (2) build parent map
                scriptContext->TTDContextInfo->ProcessFunctionBodyOnLoad(globalBody, nullptr);
                scriptContext->TTDContextInfo->RegisterNewScript(globalBody, bodyIdCtr);

                if(scriptContext->ShouldPerformRecordOrReplayAction())
                {
                    globalBody->GetUtf8SourceInfo()->SetSourceInfoForDebugReplay_TTD(bodyIdCtr);
                }

                if(scriptContext->ShouldPerformReplayDebuggerAction())
                {
                    scriptContext->GetThreadContext()->TTDExecutionInfo->ProcessScriptLoad(scriptContext, bodyIdCtr, globalBody, globalBody->GetUtf8SourceInfo(), nullptr);
                }
            }
        }
#endif

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_FUNCTION(pfuncScript, EtwTrace::GetFunctionId(pfuncScript->GetFunctionProxy())));

        if (isGenerator || isAsync)
        {
            Assert(pfuncScript->GetFunctionInfo()->IsCoroutine());
            auto pfuncVirt = static_cast<GeneratorVirtualScriptFunction*>(pfuncScript);
            auto pfuncGen = functionKind == FunctionKind::Async ?
                scriptContext->GetLibrary()->CreateAsyncFunction(JavascriptAsyncFunction::EntryAsyncFunctionImplementation, pfuncVirt) :
                functionKind == FunctionKind::AsyncGenerator ?
                scriptContext->GetLibrary()->CreateAsyncGeneratorFunction(JavascriptAsyncGeneratorFunction::EntryAsyncGeneratorFunctionImplementation, pfuncVirt) :
                scriptContext->GetLibrary()->CreateGeneratorFunction(JavascriptGeneratorFunction::EntryGeneratorFunctionImplementation, pfuncVirt);
            pfuncVirt->SetRealGeneratorFunction(pfuncGen);
            pfuncScript = pfuncGen;
        }

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(VarTo<RecyclableObject>(newTarget), pfuncScript, nullptr, scriptContext) :
            pfuncScript;
    }

    Var JavascriptFunction::NewInstanceRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        scriptContext->CheckEvalRestriction();

        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return NewInstanceHelper(scriptContext, function, callInfo, args);
    }

    Var JavascriptFunction::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        return NewInstanceHelper(scriptContext, function, callInfo, args);
    }

    Var JavascriptFunction::NewAsyncGeneratorFunctionInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Get called when creating a new async generator function through the constructor (e.g. agf.__proto__.constructor)
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(function->GetScriptContext(), function, callInfo, args, JavascriptFunction::FunctionKind::AsyncGenerator);
    }

    Var JavascriptFunction::NewAsyncGeneratorFunctionInstanceRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        scriptContext->CheckEvalRestriction();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(scriptContext, function, callInfo, args, JavascriptFunction::FunctionKind::AsyncGenerator);
    }

    Var JavascriptFunction::NewAsyncFunctionInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Get called when creating a new async function through the constructor (e.g. af.__proto__.constructor)
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(function->GetScriptContext(), function, callInfo, args, JavascriptFunction::FunctionKind::Async);
    }

    Var JavascriptFunction::NewAsyncFunctionInstanceRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        scriptContext->CheckEvalRestriction();

        PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        return JavascriptFunction::NewInstanceHelper(scriptContext, function, callInfo, args, JavascriptFunction::FunctionKind::Async);
    }

    //
    // Dummy EntryPoint for Function.prototype
    //
    Var JavascriptFunction::PrototypeEntryPoint(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptLibrary* library = function->GetLibrary();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        if (callInfo.Flags & CallFlags_New)
        {
            JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
        }

        return library->GetUndefined();
    }

    enum : unsigned { STACK_ARGS_ALLOCA_THRESHOLD = 8 }; // Number of stack args we allow before using _alloca

    // ES5 15.3.4.3
    //When the apply method is called on an object func with arguments thisArg and argArray the following steps are taken:
    //    1.    If IsCallable(func) is false, then throw a TypeError exception.
    //    2.    If argArray is null or undefined, then
    //          a.      Return the result of calling the [[Call]] internal method of func, providing thisArg as the this value and an empty list of arguments.
    //    3.    If Type(argArray) is not Object, then throw a TypeError exception.
    //    4.    Let len be the result of calling the [[Get]] internal method of argArray with argument "length".
    //
    //    Steps 5 and 7 deleted from July 19 Errata of ES5 spec
    //
    //    5.    If len is null or undefined, then throw a TypeError exception.
    //    6.    Len n be ToUint32(len).
    //    7.    If n is not equal to ToNumber(len), then throw a TypeError exception.
    //    8.    Let argList  be an empty List.
    //    9.    Let index be 0.
    //    10.   Repeat while index < n
    //          a.      Let indexName be ToString(index).
    //          b.      Let nextArg be the result of calling the [[Get]] internal method of argArray with indexName as the argument.
    //          c.      Append nextArg as the last element of argList.
    //          d.      Set index to index + 1.
    //    11.   Return the result of calling the [[Call]] internal method of func, providing thisArg as the this value and argList as the list of arguments.
    //    The length property of the apply method is 2.

    Var JavascriptFunction::EntryApply(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        // Ideally, we want to maintain CallFlags_Eval behavior and pass along the extra FrameDisplay parameter
        // but that we would be a bigger change than what we want to do in this ship cycle. See WIN8: 915315.
        // If eval is executed using apply it will not get the frame display and always execute in global scope.
        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        ///
        /// Check Argument[0] has internal [[Call]] property
        /// If not, throw TypeError
        ///
        if (args.Info.Count == 0 || !JavascriptConversion::IsCallable(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("Function.prototype.apply"));
        }

        Var thisVar = NULL;
        Var argArray = NULL;
        RecyclableObject* pFunc = VarTo<RecyclableObject>(args[0]);

        if (args.Info.Count == 1)
        {
            thisVar = scriptContext->GetLibrary()->GetUndefined();
        }
        else if (args.Info.Count == 2)
        {
            thisVar = args.Values[1];
        }
        else if (args.Info.Count > 2)
        {
            thisVar = args.Values[1];
            argArray = args.Values[2];
        }

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CalloutHelper<false>(pFunc, thisVar, /* overridingNewTarget = */nullptr, argArray, scriptContext);
        }
        END_SAFE_REENTRANT_CALL
    }

    template <bool isConstruct>
    Var JavascriptFunction::CalloutHelper(RecyclableObject* pFunc, Var thisVar, Var overridingNewTarget, Var argArray, ScriptContext* scriptContext)
    {
        CallFlags callFlag;
        if (isConstruct)
        {
            callFlag = CallFlags_New;
        }
        else
        {
            callFlag = CallFlags_Value;
        }
        Arguments outArgs(CallInfo(callFlag, 0), nullptr);

        Var stackArgs[STACK_ARGS_ALLOCA_THRESHOLD];

        if (nullptr == argArray)
        {
            outArgs.Info.Count = 1;
            outArgs.Values = &thisVar;
        }
        else
        {
            bool isArray = JavascriptArray::IsNonES5Array(argArray);
            TypeId typeId = JavascriptOperators::GetTypeId(argArray);
            bool isNullOrUndefined = typeId <= TypeIds_UndefinedOrNull;

            if (!isNullOrUndefined && !JavascriptOperators::IsObject(argArray)) // ES5: throw if Type(argArray) is not Object
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Function.prototype.apply"));
            }

            int64 len;
            JavascriptArray* arr = NULL;
            RecyclableObject* dynamicObject = VarTo<RecyclableObject>(argArray);

            if (isNullOrUndefined)
            {
                len = 0;
            }
            else if (isArray)
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(argArray);
#endif
                arr = VarTo<JavascriptArray>(argArray);
                len = arr->GetLength();
            }
            else
            {
                Var lenProp = JavascriptOperators::OP_GetLength(dynamicObject, scriptContext);
                len = JavascriptConversion::ToLength(lenProp, scriptContext);
            }

            if (len >= CallInfo::kMaxCountArgs)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
            }

            outArgs.Info.Count = (uint)len + 1;
            if (len == 0)
            {
                outArgs.Values = &thisVar;
            }
            else
            {
                if (outArgs.Info.Count > STACK_ARGS_ALLOCA_THRESHOLD)
                {
                    PROBE_STACK(scriptContext, outArgs.Info.Count * sizeof(Var)+Js::Constants::MinStackDefault); // args + function call
                    outArgs.Values = (Var*)_alloca(outArgs.Info.Count * sizeof(Var));
                }
                else
                {
                    outArgs.Values = stackArgs;
                }
                outArgs.Values[0] = thisVar;


                Var undefined = pFunc->GetLibrary()->GetUndefined();
                if (isArray && arr->GetScriptContext() == scriptContext)
                {
                    arr->ForEachItemInRange<false>(0, (uint)len, undefined, scriptContext,
                        [&outArgs](uint index, Var element)
                    {
                        outArgs.Values[index + 1] = element;
                    });
                }
                else
                {
                    for (uint i = 0; i < len; i++)
                    {
                        Var element = nullptr;
                        if (!JavascriptOperators::GetItem(dynamicObject, i, &element, scriptContext))
                        {
                            element = undefined;
                        }
                        outArgs.Values[i + 1] = element;
                    }
                }
            }
        }

        if (isConstruct)
        {
            return JavascriptFunction::CallAsConstructor(pFunc, overridingNewTarget, outArgs, scriptContext);
        }
        else
        {
            // Apply scenarios can have more than Constants::MaxAllowedArgs number of args. Need to use the large argCount logic here.
            return JavascriptFunction::CallFunction<true>(pFunc, pFunc->GetEntryPoint(), outArgs, /* useLargeArgCount */true);
        }
    }

    Var JavascriptFunction::ApplyHelper(RecyclableObject* function, Var thisArg, Var argArray, ScriptContext* scriptContext)
    {
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CalloutHelper<false>(function, thisArg, /* overridingNewTarget = */nullptr, argArray, scriptContext);
        }
        END_SAFE_REENTRANT_CALL
    }

    Var JavascriptFunction::ConstructHelper(RecyclableObject* function, Var thisArg, Var overridingNewTarget, Var argArray, ScriptContext* scriptContext)
    {
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return CalloutHelper<true>(function, thisArg, overridingNewTarget, argArray, scriptContext);
        }
        END_SAFE_REENTRANT_CALL
    }

    Var JavascriptFunction::EntryBind(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Function_Prototype_bind);

        Assert(!(callInfo.Flags & CallFlags_New));

        ///
        /// Check Argument[0] has internal [[Call]] property
        /// If not, throw TypeError
        ///
        if (args.Info.Count == 0 || !JavascriptConversion::IsCallable(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("Function.prototype.bind"));
        }

        BoundFunction* boundFunc = BoundFunction::New(scriptContext, args);

        return boundFunc;
    }

    // ES5 15.3.4.4
    // Function.prototype.call (thisArg [ , arg1 [ , arg2, ... ] ] )
    //    When the call method is called on an object func with argument thisArg and optional arguments arg1, arg2 etc, the following steps are taken:
    //    1.    If IsCallable(func) is false, then throw a TypeError exception.
    //    2.    Let argList be an empty List.
    //    3.    If this method was called with more than one argument then in left to right order starting with arg1 append each argument as the last element of argList
    //    4.    Return the result of calling the [[Call]] internal method of func, providing thisArg as the this value and argList as the list of arguments.
    //    The length property of the call method is 1.

    Var JavascriptFunction::EntryCall(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        RUNTIME_ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        ///
        /// Check Argument[0] has internal [[Call]] property
        /// If not, throw TypeError
        ///
        uint argCount = args.Info.Count;
        if (argCount == 0 || !JavascriptConversion::IsCallable(args[0]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("Function.prototype.call"));
        }

        RecyclableObject *pFunc = VarTo<RecyclableObject>(args[0]);
        if (argCount == 1)
        {
            args.Values[0] = scriptContext->GetLibrary()->GetUndefined();
        }
        else
        {
            ///
            /// Remove function object from the arguments and pass the rest
            ///
            for (uint i = 0; i < args.Info.Count - 1; ++i)
            {
                args.Values[i] = args.Values[i + 1];
            }
            args.Info.Count = args.Info.Count - 1;
        }

        ///
        /// Call the [[Call]] method on the function object
        ///
        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return JavascriptFunction::CallFunction<true>(pFunc, pFunc->GetEntryPoint(), args, true /*useLargeArgCount*/);
        }
        END_SAFE_REENTRANT_CALL
    }

    Var JavascriptFunction::CallRootFunctionInScript(JavascriptFunction* func, Arguments args)
    {
        ScriptContext* scriptContext = func->GetScriptContext();
        if (scriptContext->GetThreadContext()->HasPreviousHostScriptContext())
        {
            ScriptContext* requestContext = scriptContext->GetThreadContext()->
              GetPreviousHostScriptContext()->GetScriptContext();
            func = VarTo<JavascriptFunction>(CrossSite::MarshalVar(requestContext,
              func, scriptContext));
        }
        return func->CallRootFunction(args, scriptContext, true);
    }

    Var JavascriptFunction::CallRootFunction(RecyclableObject* obj, Arguments args, ScriptContext * scriptContext, bool inScript)
    {
        Var ret = nullptr;

#ifdef FAULT_INJECTION
        if (Js::Configuration::Global.flags.FaultInjection >= 0)
        {
            Js::FaultInjection::pfnHandleAV = JavascriptFunction::CallRootEventFilter;
            __try
            {
                ret = JavascriptFunction::CallRootFunctionInternal(obj, args, scriptContext, inScript);
            }
            __finally
            {
                Js::FaultInjection::pfnHandleAV = nullptr;
            }
            //ret should never be null here
            Assert(ret);
            return ret;
        }
#endif

#ifdef DISABLE_SEH
        // xplat: JavascriptArrayBuffer::AllocWrapper is disabled on cross-platform
        // (IsValidVirtualBufferLength always returns false).
        // SEH and ResumeForOutOfBoundsArrayRefs are not needed.
        ret = JavascriptFunction::CallRootFunctionInternal(obj, args, scriptContext, inScript);
#else
        if (scriptContext->GetThreadContext()->GetAbnormalExceptionCode() != 0)
        {
            // ensure that hosts are not doing SEH across Chakra frames, as that can lead to bad state (e.g. destructors not being called)
            UnexpectedExceptionHandling_fatal_error();
        }

        // mark volatile, because otherwise VC will incorrectly optimize away load in the finally block
        volatile uint32 exceptionCode = 0;
        EXCEPTION_POINTERS exceptionInfo = { 0 };
        __try
        {
            __try
            {
                ret = JavascriptFunction::CallRootFunctionInternal(obj, args, scriptContext, inScript);
            }
            __except (
                exceptionInfo = *GetExceptionInformation(),
                exceptionCode = GetExceptionCode(),
                CallRootEventFilter(exceptionCode, GetExceptionInformation()))
            {
                Assert(UNREACHED);
            }
        }
        __finally
        {
            // 0xE06D7363 is C++ exception code
            if (exceptionCode != 0 && exceptionCode != 0xE06D7363 && AbnormalTermination() && !IsDebuggerPresent())
            {
                scriptContext->GetThreadContext()->SetAbnormalExceptionCode(exceptionCode);
                scriptContext->GetThreadContext()->SetAbnormalExceptionRecord(&exceptionInfo);
            }
        }
#endif
        //ret should never be null here
        Assert(ret);
        return ret;
    }

    Var JavascriptFunction::CallRootFunctionInternal(RecyclableObject* obj, Arguments args, ScriptContext * scriptContext, bool inScript)
    {
#if DBG
        if (IsInAssert != 0)
        {
            // Just don't execute anything if we are in an assert
            Js::Throw::FatalInternalError();
        }
#endif

        if (inScript)
        {
            Assert(!(args.Info.Flags & CallFlags_New));
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                return JavascriptFunction::CallFunction<true>(obj, obj->GetEntryPoint(), args);
            }
            END_SAFE_REENTRANT_CALL
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        Js::Var varThis;
        if (PHASE_FORCE1(Js::EvalCompilePhase) && args.Info.Count == 0)
        {
            varThis = JavascriptOperators::OP_GetThis(scriptContext->GetLibrary()->GetUndefined(), kmodGlobal, scriptContext);
            args.Info.Flags = (Js::CallFlags)(args.Info.Flags | CallFlags_Eval);
            args.Info.Count = 1;
            args.Values = &varThis;
        }
#endif

        Var varResult = nullptr;
        ThreadContext *threadContext;
        threadContext = scriptContext->GetThreadContext();

        JavascriptExceptionObject* pExceptionObject = NULL;
        bool hasCaller = scriptContext->GetHostScriptContext() ? !!scriptContext->GetHostScriptContext()->HasCaller() : false;
        Assert(scriptContext == obj->GetScriptContext());
        BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, hasCaller)
        {
            scriptContext->VerifyAlive(true);
            try
            {
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    varResult = args.Info.Flags & CallFlags_New ?
                            JavascriptFunction::CallAsConstructor(obj, /* overridingNewTarget = */nullptr, args, scriptContext) :
                            JavascriptFunction::CallFunction<true>(obj, obj->GetEntryPoint(), args);
                }
                END_SAFE_REENTRANT_CALL

                // A recent compiler bug 150148 can incorrectly eliminate catch block, temporary workaround
                if (threadContext == NULL)
                {
                    throw JavascriptException(nullptr);
                }
            }
            catch (const JavascriptException& err)
            {
                pExceptionObject = err.GetAndClear();
            }

            if (pExceptionObject)
            {
                JavascriptExceptionOperators::DoThrowCheckClone(pExceptionObject, scriptContext);
            }
        }
        END_JS_RUNTIME_CALL(scriptContext);

        Assert(varResult != nullptr);
        return varResult;
    }

    Var JavascriptFunction::CallRootFunction(Arguments args, ScriptContext * scriptContext, bool inScript)
    {
        return JavascriptFunction::CallRootFunction(this, args, scriptContext, inScript);
    }

#if DBG
    /*static*/
    void JavascriptFunction::CheckValidDebugThunk(ScriptContext* scriptContext, RecyclableObject *function)
    {
        Assert(scriptContext != nullptr);
        Assert(function != nullptr);

        if (scriptContext->IsScriptContextInDebugMode()
            && !scriptContext->IsInterpreted() && !CONFIG_FLAG(ForceDiagnosticsMode)    // Does not work nicely if we change the default settings.
            && function->GetEntryPoint() != scriptContext->CurrentThunk
            && !CrossSite::IsThunk(function->GetEntryPoint())
            && VarIs<JavascriptFunction>(function))
        {

            JavascriptFunction *jsFunction = VarTo<JavascriptFunction>(function);
            if (!jsFunction->IsBoundFunction()
                && !jsFunction->GetFunctionInfo()->IsDeferred()
                && (jsFunction->GetFunctionInfo()->GetAttributes() & FunctionInfo::DoNotProfile) != FunctionInfo::DoNotProfile
                && jsFunction->GetFunctionInfo() != &JavascriptExternalFunction::EntryInfo::WrappedFunctionThunk)
            {
                Js::FunctionProxy *proxy = jsFunction->GetFunctionProxy();
                if (proxy)
                {
                    AssertMsg(proxy->HasValidEntryPoint(), "Function does not have valid entrypoint");
                }
            }
        }
    }
#endif

    Var JavascriptFunction::CallAsConstructor(Var v, Var overridingNewTarget, Arguments args, ScriptContext* scriptContext, const Js::AuxArray<uint32> *spreadIndices)
    {
        Assert(v);
        Assert(args.Info.Flags & CallFlags_New);
        Assert(scriptContext);

        // newCount is ushort.
        if (args.Info.Count >= USHORT_MAX)
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
        }
        AnalysisAssert(args.Info.Count < USHORT_MAX);

        // Create the empty object if necessary:
        // - Built-in constructor functions will return a new object of a specific type, so a new empty object does not need to
        //   be created
        // - If the newTarget is specified and the function is base kind then the this object will be already created. So we can
        //   just use it instead of creating a new one.
        // - For user-defined constructor functions, an empty object is created with the function's prototype
        Var resultObject = nullptr;
        if (overridingNewTarget != nullptr && args.Info.Count > 0)
        {
            resultObject = args.Values[0];
        }
        else
        {
            BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
            {
                resultObject = JavascriptOperators::NewScObjectNoCtor(v, scriptContext);
            }
            END_SAFE_REENTRANT_CALL
        }

        // JavascriptOperators::NewScObjectNoCtor should have thrown if 'v' is not a constructor
        RecyclableObject* functionObj = UnsafeVarTo<RecyclableObject>(v);

        const unsigned STACK_ARGS_ALLOCA_THRESHOLD = 8; // Number of stack args we allow before using _alloca
        Var stackArgs[STACK_ARGS_ALLOCA_THRESHOLD];
        Var* newValues = args.Values;
        CallFlags newFlags = args.Info.Flags;

        bool thisAlreadySpecified = false;

        if (overridingNewTarget != nullptr)
        {
            ScriptFunction * scriptFunctionObj = JavascriptOperators::TryFromVar<ScriptFunction>(functionObj);
            uint newCount = args.Info.Count;
            if (scriptFunctionObj && scriptFunctionObj->GetFunctionInfo()->IsClassConstructor())
            {
                thisAlreadySpecified = true;
                args.Values[0] = overridingNewTarget;
            }
            else
            {
                newCount++;
                newFlags = (CallFlags)(newFlags | CallFlags_NewTarget | CallFlags_ExtraArg);
                if (newCount > STACK_ARGS_ALLOCA_THRESHOLD)
                {
                    PROBE_STACK(scriptContext, newCount * sizeof(Var) + Js::Constants::MinStackDefault); // args + function call
                    newValues = (Var*)_alloca(newCount * sizeof(Var));
                }
                else
                {
                    newValues = stackArgs;
                }

                for (unsigned int i = 0; i < args.Info.Count; i++)
                {
                    newValues[i] = args.Values[i];
                }
#pragma prefast(suppress:6386, "The index is within the bounds")
                newValues[args.Info.Count] = overridingNewTarget;
            }
        }

        // Call the constructor function:
        // - If this is not already specified as the overriding new target in Reflect.construct a class case, then
        // - Pass in the new empty object as the 'this' parameter. This can be null if an empty object was not created.

        if (!thisAlreadySpecified)
        {
            newValues[0] = resultObject;
        }

        CallInfo newCallInfo(newFlags, args.Info.Count);
        Arguments newArgs(newCallInfo, newValues);

        if (VarIs<JavascriptProxy>(v))
        {
            JavascriptProxy* proxy = VarTo<JavascriptProxy>(v);
            return proxy->ConstructorTrap(newArgs, scriptContext, spreadIndices);
        }

#if DBG
        if (scriptContext->IsScriptContextInDebugMode())
        {
            CheckValidDebugThunk(scriptContext, functionObj);
        }
#endif

        Var functionResult;
        if (spreadIndices != nullptr)
        {
            functionResult = CallSpreadFunction(functionObj, newArgs, spreadIndices);
        }
        else
        {
            functionResult = CallFunction<true>(functionObj, functionObj->GetEntryPoint(), newArgs, true /*useLargeArgCount*/);
        }

        return
            FinishConstructor(
                functionResult,
                resultObject,
                VarIs<JavascriptFunction>(functionObj) && functionObj->GetScriptContext() == scriptContext ?
                VarTo<JavascriptFunction>(functionObj) :
                nullptr,
                overridingNewTarget != nullptr);
    }

    Var JavascriptFunction::FinishConstructor(
        const Var constructorReturnValue,
        Var newObject,
        JavascriptFunction *const function,
        bool hasOverridingNewTarget)
    {
        Assert(constructorReturnValue);

        // CONSIDER: Using constructorCache->ctorHasNoExplicitReturnValue to speed up this interpreter code path.
        if (JavascriptOperators::IsObject(constructorReturnValue))
        {
            newObject = constructorReturnValue;
        }

        // #3217: Cases with overriding newTarget are not what constructor cache is intended for;
        //     Bypass constructor cache to avoid prototype mismatch/confusion.
        if (function && function->GetConstructorCache()->NeedsUpdateAfterCtor() && !hasOverridingNewTarget)
        {
            JavascriptOperators::UpdateNewScObjectCache(function, newObject, function->GetScriptContext());
        }

        return newObject;
    }

    Var JavascriptFunction::EntrySpreadCall(const Js::AuxArray<uint32> *spreadIndices, RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        RUNTIME_ARGUMENTS(args, spreadIndices, function, callInfo);

        BEGIN_SAFE_REENTRANT_CALL(function->GetScriptContext()->GetThreadContext())
        {
            return JavascriptFunction::CallSpreadFunction(function, args, spreadIndices);
        }
        END_SAFE_REENTRANT_CALL
    }

    uint JavascriptFunction::GetSpreadSize(const Arguments args, const Js::AuxArray<uint32> *spreadIndices, ScriptContext *scriptContext)
    {
        // Work out the expanded number of arguments.
        AssertOrFailFast(args.Info.Count < CallInfo::kMaxCountArgs && args.Info.Count >= spreadIndices->count);

        uint spreadArgsCount = spreadIndices->count;
        uint32 totalLength = args.Info.Count - spreadArgsCount;

        for (unsigned i = 0; i < spreadArgsCount; ++i)
        {
            uint32 elementLength = JavascriptArray::GetSpreadArgLen(args[spreadIndices->elements[i]], scriptContext);
            if (elementLength >= CallInfo::kMaxCountArgs)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
            }
            totalLength = UInt32Math::Add(totalLength, elementLength);
        }

        if (totalLength >= CallInfo::kMaxCountArgs)
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_ArgListTooLarge);
        }

        return totalLength;
    }

    void JavascriptFunction::SpreadArgs(const Arguments args, Arguments& destArgs, const Js::AuxArray<uint32> *spreadIndices, ScriptContext *scriptContext)
    {
        Assert(args.Values != nullptr);
        Assert(destArgs.Values != nullptr);

        CallInfo callInfo = args.Info;
        uint argCount = args.GetArgCountWithExtraArgs();
        unsigned destArgCount = destArgs.GetLargeArgCountWithExtraArgs(); // Result can be bigger than Constants::MaxAllowedArgs
        size_t destArgsByteSize = destArgCount * sizeof(Var);

        destArgs.Values[0] = args[0];

        // Iterate over the arguments, spreading inline. We skip 'this'.

        uint32 argsIndex = 1;
        for (unsigned i = 1, spreadArgIndex = 0; i < argCount; ++i)
        {
            uint32 spreadIndex = spreadIndices->elements[spreadArgIndex]; // Next index to be spread.
            if (i < spreadIndex)
            {
                // Copy everything until the next spread index.
                js_memcpy_s(destArgs.Values + argsIndex,
                            destArgsByteSize - (argsIndex * sizeof(Var)),
                            args.Values + i,
                            (spreadIndex - i) * sizeof(Var));
                argsIndex += spreadIndex - i;
                i = spreadIndex - 1;
                continue;
            }
            else if (i > spreadIndex)
            {
                // Copy everything after the last spread index.
                js_memcpy_s(destArgs.Values + argsIndex,
                            destArgsByteSize - (argsIndex * sizeof(Var)),
                            args.Values + i,
                            (argCount - i) * sizeof(Var));
                break;
            }
            else
            {
                // Expand the spread element.
                Var instance = args[spreadIndex];

                if (VarIs<SpreadArgument>(instance))
                {
                    SpreadArgument* spreadedArgs = VarTo<SpreadArgument>(instance);
                    uint size = spreadedArgs->GetArgumentSpreadCount();
                    if (size > 0)
                    {
                        const Var * spreadBuffer = spreadedArgs->GetArgumentSpread();
                        js_memcpy_s(destArgs.Values + argsIndex,
                            size * sizeof(Var),
                            spreadBuffer,
                            size * sizeof(Var));
                        argsIndex += size;
                    }
                }
                else
                {
                    Assert(JavascriptOperators::IsUndefinedObject(instance));
                    destArgs.Values[argsIndex++] = instance;
                }

                if (spreadArgIndex < spreadIndices->count - 1)
                {
                    spreadArgIndex++;
                }
            }
        }
        if (argsIndex > destArgCount)
        {
            AssertMsg(false, "The array length has changed since we allocated the destArgs buffer?");
            Throw::FatalInternalError();
        }

    }

    Var JavascriptFunction::CallSpreadFunction(RecyclableObject* function, Arguments args, const Js::AuxArray<uint32> *spreadIndices)
    {
        ScriptContext* scriptContext = function->GetScriptContext();

        // Work out the expanded number of arguments.
        uint spreadSize = GetSpreadSize(args, spreadIndices, scriptContext);
        uint32 actualLength = CallInfo::GetLargeArgCountWithExtraArgs(args.Info.Flags, spreadSize);

        // Allocate (if needed) space for the expanded arguments.
        Arguments outArgs(CallInfo(args.Info.Flags, spreadSize), nullptr);
        Var stackArgs[STACK_ARGS_ALLOCA_THRESHOLD];
        size_t outArgsSize = 0;
        if (actualLength > STACK_ARGS_ALLOCA_THRESHOLD)
        {
            PROBE_STACK(scriptContext, actualLength * sizeof(Var) + Js::Constants::MinStackDefault); // args + function call
            outArgsSize = actualLength * sizeof(Var);
            outArgs.Values = (Var*)_alloca(outArgsSize);
            ZeroMemory(outArgs.Values, outArgsSize);
        }
        else
        {
            outArgs.Values = stackArgs;
            outArgsSize = STACK_ARGS_ALLOCA_THRESHOLD * sizeof(Var);
            ZeroMemory(outArgs.Values, outArgsSize); // We may not use all of the elements
        }

        SpreadArgs(args, outArgs, spreadIndices, scriptContext);

        // Number of arguments are allowed to be more than Constants::MaxAllowedArgs in runtime. Need to use the large argcount logic in this case.
        return JavascriptFunction::CallFunction<true>(function, function->GetEntryPoint(), outArgs, true);
    }

    Var JavascriptFunction::CallFunction(Arguments args)
    {
        return JavascriptFunction::CallFunction<true>(this, this->GetEntryPoint(), args);
    }

    template Var JavascriptFunction::CallFunction<true>(RecyclableObject* function, JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount);
    template Var JavascriptFunction::CallFunction<false>(RecyclableObject* function, JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount);

#if _M_IX86
    extern "C" Var BreakSpeculation(Var passthrough)
    {
        Var result = nullptr;
        __asm
        {
            mov ecx, passthrough;
            cmp ecx, ecx;
            cmove eax, ecx;
            mov result, eax;
        }
        return result;
    }
#ifdef ASMJS_PLAT
    template <> int JavascriptFunction::CallAsmJsFunction<int>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
    {
        return CallAsmJsFunctionX86Thunk(function, entryPoint, argv, argsSize, reg).i32;
    }
    template <> int64 JavascriptFunction::CallAsmJsFunction<int64>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
    {
        return CallAsmJsFunctionX86Thunk(function, entryPoint, argv, argsSize, reg).i64;
    }
    template <> float JavascriptFunction::CallAsmJsFunction<float>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
    {
        return CallAsmJsFunctionX86Thunk(function, entryPoint, argv, argsSize, reg).f32;
    }
    template <> double JavascriptFunction::CallAsmJsFunction<double>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
    {
        return CallAsmJsFunctionX86Thunk(function, entryPoint, argv, argsSize, reg).f64;
    }
    template <> AsmJsSIMDValue JavascriptFunction::CallAsmJsFunction<AsmJsSIMDValue>(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte* reg)
    {
        return CallAsmJsFunctionX86Thunk(function, entryPoint, argv, argsSize, reg).simd;
    }

    PossibleAsmJsReturnValues JavascriptFunction::CallAsmJsFunctionX86Thunk(RecyclableObject * function, JavascriptMethod entryPoint, Var * argv, uint argsSize, byte*)
    {
        void* savedEsp;
        _declspec(align(16)) PossibleAsmJsReturnValues retVals;
        CompileAssert(sizeof(PossibleAsmJsReturnValues) == sizeof(int64) + sizeof(AsmJsSIMDValue));
        CompileAssert(offsetof(PossibleAsmJsReturnValues, low) == offsetof(PossibleAsmJsReturnValues, i32));
        CompileAssert(offsetof(PossibleAsmJsReturnValues, high) == offsetof(PossibleAsmJsReturnValues, i32) + sizeof(int32));

        // call variable argument function provided in entryPoint
        __asm
        {
            mov savedEsp, esp;
            mov ecx, argsSize;
            cmp ecx, 0x1000;
            jl allocate_stack;
            // Use _chkstk to probe each page when using more then a page size
            mov eax, ecx;
            call _chkstk; // _chkstk saves/restores ecx
        allocate_stack:
            sub esp, ecx;

            mov edi, esp;
            mov esi, argv;
            add esi, 4; // Skip function
            mov ecx, argsSize;
            rep movs byte ptr[edi], byte ptr[esi];

            mov  ecx, entryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            call[__guard_check_icall_fptr]
#endif
            push function;
            call ecx;
            mov retVals.low, eax;
            mov retVals.high, edx;
            movaps retVals.xmm, xmm0;
            // Restore ESP
            mov esp, savedEsp;
        }
        return retVals;
    }
#endif

#ifdef __clang__
void __cdecl _alloca_probe_16()
{
    // todo: fix this!!!
    abort();
    __asm
    {
        push    ecx
        lea     ecx, [esp + 8]
        sub     ecx, eax
        and     ecx, (16 - 1)
        add     eax, ecx
        ret
    }
}
#endif

    static Var LocalCallFunction(RecyclableObject* function,
        JavascriptMethod entryPoint, Arguments args, bool doStackProbe, bool useLargeArgCount = false)
    {
        Js::Var varResult;

#if DBG && ENABLE_NATIVE_CODEGEN
        CheckIsExecutable(function, entryPoint);
#endif
        // compute size of stack to reserve
        CallInfo callInfo = args.Info;
        uint argCount = useLargeArgCount ? args.GetLargeArgCountWithExtraArgs() : args.GetArgCountWithExtraArgs();
        uint argsSize = argCount * sizeof(Var);

        ScriptContext * scriptContext = function->GetScriptContext();

        if (doStackProbe)
        {
            PROBE_STACK_CALL(scriptContext, function, argsSize);
        }

        JS_REENTRANCY_CHECK(scriptContext->GetThreadContext());

        void *data;
        void *savedEsp;
        __asm {
            // Save ESP
            mov savedEsp, esp
            mov eax, argsSize
            // Make sure we don't go beyond guard page
            cmp eax, 0x1000
            jge alloca_probe
            sub esp, eax
            jmp dbl_align
alloca_probe:
            // Use alloca to allocate more then a page size
            // Alloca assumes eax, contains size, and adjust ESP while
            // probing each page.
            call _alloca_probe_16
dbl_align:
            // 8-byte align frame to improve floating point perf of our JIT'd code.
            and esp, -8

            mov data, esp
        }

        {

            Var* dest = (Var*)data;
            Var* src = args.Values;
            for(unsigned int i =0; i < argCount; i++)
            {
                dest[i] = src[i];
            }
        }

        // call variable argument function provided in entryPoint
        __asm
        {
            mov  ecx, entryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            call [__guard_check_icall_fptr]
#endif

            push callInfo
            push function
            call ecx

            // Restore ESP
            mov esp, savedEsp

            // save the return value from realsum.
            mov varResult, eax;
        }

        return varResult;
    }

    // clang fails to create the labels,
    // when __asm op is under a template function
    template <bool doStackProbe>
    Var JavascriptFunction::CallFunction(RecyclableObject* function,
        JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount)
    {
        return LocalCallFunction(function, entryPoint, args, doStackProbe, useLargeArgCount);
    }

#elif _M_X64
    template <bool doStackProbe>
    Var JavascriptFunction::CallFunction(RecyclableObject *function, JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount)
    {
        // compute size of stack to reserve and make sure we have enough stack.
        uint argCount = useLargeArgCount ? args.GetLargeArgCountWithExtraArgs() : args.GetArgCountWithExtraArgs();
        uint argsSize = argCount * sizeof(Var);

        if (doStackProbe == true)
        {
            PROBE_STACK_CALL(function->GetScriptContext(), function, argsSize);
        }
#if DBG && ENABLE_NATIVE_CODEGEN
        CheckIsExecutable(function, entryPoint);
#endif

        return JS_REENTRANCY_CHECK(function->GetScriptContext()->GetThreadContext(),
            amd64_CallFunction(function, entryPoint, args.Info, argCount, &args.Values[0]));
    }
#elif defined(_M_ARM)
    extern "C"
    {
#ifdef _WIN32
        extern Var arm_CallFunction(JavascriptFunction* function, CallInfo info, uint argCount, Var* values, JavascriptMethod entryPoint);
#else
        extern Var arm_CallFunction(JavascriptFunction* function, CallInfo info, Var* values, JavascriptMethod entryPoint);
#endif
    }

    extern "C" Var BreakSpeculation(Var passthrough)
    {
        return passthrough;
    }

    template <bool doStackProbe>
    Var JavascriptFunction::CallFunction(RecyclableObject* function, JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount)
    {
        // compute size of stack to reserve and make sure we have enough stack.
        uint argCount = useLargeArgCount ? args.GetLargeArgCountWithExtraArgs() : args.GetArgCountWithExtraArgs();
        uint argsSize = argCount * sizeof(Var);
        if (doStackProbe)
        {
            PROBE_STACK_CALL(function->GetScriptContext(), function, argsSize);
        }

#if DBG && ENABLE_NATIVE_CODEGEN
        CheckIsExecutable(function, entryPoint);
#endif
        Js::Var varResult;

#ifdef _WIN32
        //The ARM can pass 4 arguments via registers so handle the cases for 0 or 1 values without resorting to asm code
        //(so that the asm code can assume 0 or more values will go on the stack: putting -1 values on the stack is unhealthy).
        if (argCount == 0)
        {
            varResult = CALL_ENTRYPOINT(function->GetScriptContext()->GetThreadContext(), entryPoint, (JavascriptFunction*)function, args.Info);
        }
        else if (argCount == 1)
        {
            varResult = CALL_ENTRYPOINT(function->GetScriptContext()->GetThreadContext(), entryPoint, (JavascriptFunction*)function, args.Info, args.Values[0]);
        }
        else
        {
            varResult = JS_REENTRANCY_CHECK(function->GetScriptContext()->GetThreadContext(),
                arm_CallFunction((JavascriptFunction*)function, args.Info, argCount, args.Values, entryPoint));
        }
#else
        // ARM passes 4 arguments via registers (not on the stack), which is exactly
        // as many as we need. The arm_CallFunction takes care of handling 0 and 1 args.
        varResult = JS_REENTRANCY_CHECK(function->GetScriptContext()->GetThreadContext(),
                arm_CallFunction((JavascriptFunction*)function, args.Info, args.Values, entryPoint));
#endif

        return varResult;
    }
#elif defined(_M_ARM64)
    extern "C"
    {
        extern Var arm64_CallFunction(JavascriptFunction* function, CallInfo info, uint argCount, Var* values, JavascriptMethod entryPoint);
    }

    template <bool doStackProbe>
    Var JavascriptFunction::CallFunction(RecyclableObject* function, JavascriptMethod entryPoint, Arguments args, bool useLargeArgCount)
    {
        // compute size of stack to reserve and make sure we have enough stack.
        uint argCount = useLargeArgCount ? args.GetLargeArgCountWithExtraArgs() : args.GetArgCountWithExtraArgs();
        uint argsSize = argCount * sizeof(Var);
        if (doStackProbe)
        {
            PROBE_STACK_CALL(function->GetScriptContext(), function, argsSize);
        }

#if DBG && ENABLE_NATIVE_CODEGEN
        CheckIsExecutable(function, entryPoint);
#endif
        Js::Var varResult;

        varResult = JS_REENTRANCY_CHECK(function->GetScriptContext()->GetThreadContext(),
            arm64_CallFunction((JavascriptFunction*)function, args.Info, argCount, args.Values, entryPoint));

        return varResult;
    }
#else
    Var JavascriptFunction::CallFunction(RecyclableObject *function, JavascriptMethod entryPoint, Arguments args)
    {
#if DBG && ENABLE_NATIVE_CODEGEN
        CheckIsExecutable(function, entryPoint);
#endif
#if 1
        Js::Throw::NotImplemented();
        return nullptr;
#else
        Var varResult;
        switch (info.Count)
        {
        case 0:
            {
                varResult=entryPoint((JavascriptFunction*)function, args.Info);
                break;
            }
        case 1: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0]);
            break;
                }
        case 2: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1]);
            break;
                }
        case 3: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2]);
            break;
                }
        case 4: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3]);
            break;
                }
        case 5: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3],
                args.Values[4]);
            break;
                }
        case 6: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3],
                args.Values[4],
                args.Values[5]);
            break;
                }
        case 7: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3],
                args.Values[4],
                args.Values[5],
                args.Values[6]);
            break;
                }
        case 8: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3],
                args.Values[4],
                args.Values[5],
                args.Values[6],
                args.Values[7]);
            break;
                }
        case 9: {
            varResult=entryPoint(
                (JavascriptFunction*)function,
                args.Info,
                args.Values[0],
                args.Values[1],
                args.Values[2],
                args.Values[3],
                args.Values[4],
                args.Values[5],
                args.Values[6],
                args.Values[7],
                args.Values[8]);
            break;
                }
        default:
            ScriptContext* scriptContext = function->type->GetScriptContext();
            varResult = scriptContext->GetLibrary()->GetUndefined();
            AssertMsg(false, "CallFunction call with unsupported number of arguments");
            break;
        }

#endif
        return varResult;
    }
#endif

    Var JavascriptFunction::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        Var arg0 = args[0];

        // callable proxy is considered as having [[Call]] internal method
        // and should behave here like a function.
        // we will defer to the underlying target.
        while (VarIs<JavascriptProxy>(arg0) && JavascriptConversion::IsCallable(arg0))
        {
            arg0 = VarTo<JavascriptProxy>(arg0)->GetTarget();
        }

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        if (args.Info.Count == 0 || !VarIs<JavascriptFunction>(arg0))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("Function.prototype.toString"));
        }
        JavascriptFunction *pFunc = VarTo<JavascriptFunction>(arg0);

        // pFunc can be from a different script context if Function.prototype.toString is invoked via .call/.apply.
        // Marshal the resulting string to the current script context (that of the toString)
        return CrossSite::MarshalVar(scriptContext, pFunc->EnsureSourceString());
    }

    JavascriptString* JavascriptFunction::GetNativeFunctionDisplayString(ScriptContext *scriptContext, JavascriptString *name)
    {
        return GetNativeFunctionDisplayStringCommon<JavascriptString>(scriptContext, name);
    }

    JavascriptString* JavascriptFunction::GetLibraryCodeDisplayString(ScriptContext *scriptContext, PCWSTR displayName)
    {
        return GetLibraryCodeDisplayStringCommon<JavascriptString, JavascriptString*>(scriptContext, displayName);
    }

#ifdef _M_IX86
    // This code is enabled by the -checkAlignment switch.
    // It verifies that all of our JS frames are 8 byte aligned.
    // Our alignments is based on aligning the return address of the function.
    // Note that this test can fail when Javascript functions are called directly
    // from helper functions.  This could be fixed by making these calls through
    // CallFunction(), or by having the helper 8 byte align the frame itself before
    // the call.  A lot of these though are not dealing with floats, so the cost
    // of doing the 8 byte alignment would outweigh the benefit...
    __declspec (naked)
    void JavascriptFunction::CheckAlignment()
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrFunc_CheckAlignment);
        _asm
        {
            test esp, 0x4
            je   LABEL1
            ret
LABEL1:
            call Throw::InternalError
        }
        JIT_HELPER_END(ScrFunc_CheckAlignment);
    }
#else
    void JavascriptFunction::CheckAlignment()
    {
        JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(ScrFunc_CheckAlignment);
        // Note: in order to enable this on ARM, uncomment/fix code in LowerMD.cpp (LowerEntryInstr).
        JIT_HELPER_END(ScrFunc_CheckAlignment);
    }
#endif

    BOOL JavascriptFunction::IsNativeAddress(ScriptContext * scriptContext, void * codeAddr)
    {
#if ENABLE_NATIVE_CODEGEN
        return scriptContext->IsNativeAddress(codeAddr);
#else
        return false;
#endif
    }

    Js::JavascriptMethod JavascriptFunction::DeferredParse(ScriptFunction** functionRef)
    {
        BOOL fParsed;
        return Js::ScriptFunction::DeferredParseCore(functionRef, fParsed);
    }

    Js::JavascriptMethod JavascriptFunction::DeferredParseCore(ScriptFunction** functionRef, BOOL &fParsed)
    {
        // Do the actual deferred parsing and byte code generation, passing the new entry point to the caller.

        ParseableFunctionInfo* functionInfo = (*functionRef)->GetParseableFunctionInfo();
        FunctionBody* funcBody = nullptr;

        Assert(functionInfo);

        if (functionInfo->IsDeferredParseFunction())
        {
            funcBody = functionInfo->Parse(functionRef);
            fParsed = funcBody->IsFunctionParsed() ? TRUE : FALSE;

#if ENABLE_PROFILE_INFO
            // This is the first call to the function, ensure dynamic profile info
            funcBody->EnsureDynamicProfileInfo();
#endif
        }
        else
        {
            funcBody = functionInfo->GetFunctionBody();
            Assert(funcBody != nullptr);
            Assert(!funcBody->IsDeferredParseFunction());
        }

        DebugOnly(JavascriptMethod directEntryPoint = funcBody->GetDirectEntryPoint(funcBody->GetDefaultEntryPointInfo()));
#if defined(ENABLE_SCRIPT_PROFILING) || defined(ENABLE_SCRIPT_DEBUGGING)
        Assert(directEntryPoint != DefaultDeferredParsingThunk
            && directEntryPoint != ProfileDeferredParsingThunk);
#else // !ENABLE_SCRIPT_PROFILING && !ENABLE_SCRIPT_DEBUGGING
        Assert(directEntryPoint != DefaultDeferredParsingThunk);
#endif

        JavascriptMethod thunkEntryPoint = (*functionRef)->UpdateUndeferredBody(funcBody);

        return thunkEntryPoint;
    }

    void JavascriptFunction::ReparseAsmJsModule(ScriptFunction** functionRef)
    {
        ParseableFunctionInfo* functionInfo = (*functionRef)->GetParseableFunctionInfo();
        Assert(functionInfo);
        try
        {
            functionInfo->GetFunctionBody()->AddDeferParseAttribute();
            functionInfo->GetFunctionBody()->ResetEntryPoint();
            functionInfo->GetFunctionBody()->ResetInParams();

            FunctionBody * funcBody = functionInfo->Parse(functionRef);

    #if ENABLE_PROFILE_INFO
            // This is the first call to the function, ensure dynamic profile info
            funcBody->EnsureDynamicProfileInfo();
    #endif

            (*functionRef)->UpdateUndeferredBody(funcBody);
        }
        catch (JavascriptException&)
        {
            Js::Throw::FatalInternalError();
        }
    }

    // Thunk for handling calls to functions that have not had byte code generated for them.

#if _M_IX86
    __declspec(naked)
    Var JavascriptFunction::DeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        __asm
        {
            push ebp
            mov ebp, esp
            lea eax, [esp+8]                // load the address of the function os that if we need to box, we can patch it up
            push eax
            call JavascriptFunction::DeferredParse
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            jmp eax
        }
    }
#elif defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
    //Do nothing: the implementation of JavascriptFunction::DeferredParsingThunk is declared (appropriately decorated) in
    // Library\amd64\javascriptfunctiona.asm
    // Library\arm\arm_DeferredParsingThunk.asm
    // Library\arm64\arm64_DeferredParsingThunk.asm
#else
    Var JavascriptFunction::DeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        Js::Throw::NotImplemented();
        return nullptr;
    }
#endif

    ConstructorCache* JavascriptFunction::EnsureValidConstructorCache()
    {
        Assert(this->constructorCache != nullptr);
        this->constructorCache = ConstructorCache::EnsureValidInstance(this->constructorCache, this->GetScriptContext());
        return this->constructorCache;
    }

    void JavascriptFunction::ResetConstructorCacheToDefault()
    {
        Assert(this->constructorCache != nullptr);

        if (!this->constructorCache->IsDefault())
        {
            this->constructorCache = &ConstructorCache::DefaultInstance;
        }
    }

    // Thunk for handling calls to functions that have not had byte code generated for them.

#if _M_IX86
    __declspec(naked)
    Var JavascriptFunction::DeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        __asm
        {
            push ebp
            mov ebp, esp
            push [esp+8]
            call JavascriptFunction::DeferredDeserialize
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            jmp eax
        }
    }
#elif (defined(_M_X64) || defined(_M_ARM32_OR_ARM64)) && defined(_MSC_VER)
    //Do nothing: the implementation of JavascriptFunction::DeferredParsingThunk is declared (appropriately decorated) in
    // Library\amd64\javascriptfunctiona.asm
    // Library\arm\arm_DeferredParsingThunk.asm
    // Library\arm64\arm64_DeferredParsingThunk.asm
#else
    // xplat implement in
    // Library/amd64/JavascriptFunctionA.S
#endif

    Js::JavascriptMethod JavascriptFunction::DeferredDeserialize(ScriptFunction* function)
    {
        FunctionInfo* funcInfo = function->GetFunctionInfo();
        Assert(funcInfo);
        FunctionBody* funcBody = nullptr;

        // If we haven't already deserialized this function, do so now
        // FunctionProxies could have gotten deserialized during the interpreter when
        // we tried to record the callsite info for the function which meant that it was a
        // target of a call. Or we could have deserialized the function info in another JavascriptFunctionInstance
        // In any case, fix up the function info if it's already been deserialized so that
        // we don't hold on to the proxy for too long, and rethunk it so that it directly
        // calls the default entry point the next time around
        if (funcInfo->IsDeferredDeserializeFunction())
        {
            DeferDeserializeFunctionInfo* deferDeserializeFunction = funcInfo->GetDeferDeserializeFunctionInfo();

            // This is the first call to the function, ensure dynamic profile info
            // Deserialize is a no-op if the function has already been deserialized
            funcBody = deferDeserializeFunction->Deserialize();
#if ENABLE_PROFILE_INFO
            funcBody->EnsureDynamicProfileInfo();
#endif
        }
        else
        {
            funcBody = funcInfo->GetFunctionBody();
            Assert(funcBody != nullptr);
            Assert(!funcBody->IsDeferredDeserializeFunction());
        }

        return function->UpdateUndeferredBody(funcBody);
    }
    void JavascriptFunction::SetEntryPoint(JavascriptMethod method)
    {
        this->GetDynamicType()->SetEntryPoint(method);
    }

    JavascriptString * JavascriptFunction::EnsureSourceString()
    {
        return this->GetLibrary()->GetFunctionDisplayString();
    }

    /*
    *****************************************************************************************************************
                                Conditions checked by instruction decoder (In sequential order)
    ******************************************************************************************************************
    1)  Exception Code is AV i.e STATUS_ACCESS_VIOLATION
    2)  Check if Rip is Native address
    3)  Get the function object from RBP (a fixed offset from RBP) and check for the following
        a.  Not Null
        b.  Ensure that the function object is heap allocated
        c.  Ensure that the entrypointInfo is heap allocated
        d.  Ensure that the functionbody is heap allocated
        e.  Is a function
        f.  Is AsmJs Function object for asmjs
    4)  Check if Array BufferLength > 0x10000 (64K), power of 2 if length is less than 2^24 or multiple of 2^24  and multiple of 0x1000(4K) for asmjs
    5)  Check If the instruction is valid
        a.  Is one of the move instructions , i.e. mov, movsx, movzx, movsxd, movss or movsd
        b.  Get the array buffer register and its value for asmjs
        c.  Get the dst register(in case of load)
        d.  Calculate the number of bytes read in order to get the length of the instruction , ensure that the length should never be greater than 15 bytes
    6)  Check that the Array buffer value is same as the one we passed in EntryPointInfo in asmjs
    7)  Set the dst reg if the instr type is load
    8)  Add the bytes read to Rip and set it as new Rip
    9)  Return EXCEPTION_CONTINUE_EXECUTION

    */
#if ENABLE_NATIVE_CODEGEN
#if defined(_M_IX86) || defined(_M_X64)
    class ExceptionFilterHelper
    {
        Js::ScriptFunction* m_func = nullptr;
        bool m_checkedForFunc = false;
        PEXCEPTION_POINTERS exceptionInfo;
    public:
        ExceptionFilterHelper(PEXCEPTION_POINTERS exceptionInfo) : exceptionInfo(exceptionInfo) {}
        PEXCEPTION_POINTERS GetExceptionInfo() const
        {
            return exceptionInfo;
        }

        uintptr_t GetFaultingAddress() const
        {
            // For AVs, the second element of ExceptionInformation array is address of inaccessible data
            // https://msdn.microsoft.com/en-us/library/windows/desktop/aa363082.aspx
            Assert(this->exceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION);
            Assert(this->exceptionInfo->ExceptionRecord->NumberParameters >= 2);
            return exceptionInfo->ExceptionRecord->ExceptionInformation[1];
        }
        Var GetIPAddress() const
        {
#if _M_IX86
            return (Var)exceptionInfo->ContextRecord->Eip;
#elif _M_X64
            return (Var)exceptionInfo->ContextRecord->Rip;
#else
#error Not yet Implemented
#endif
        }
        Var* GetAddressOfFuncObj() const
        {
#if _M_IX86
            return (Var*)(exceptionInfo->ContextRecord->Ebp + 2 * sizeof(Var));
#elif _M_X64
            return (Var*)(exceptionInfo->ContextRecord->Rbp + 2 * sizeof(Var));
#else
#error Not yet Implemented
#endif
        }
        Js::ScriptFunction* GetScriptFunction()
        {
            if (m_checkedForFunc)
            {
                return m_func;
            }
            m_checkedForFunc = true;
            ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();

            // AV should come from JITed code, since we don't eliminate bound checks in interpreter
            if (!threadContext->IsNativeAddress(GetIPAddress()))
            {
                return nullptr;
            }

            Var* addressOfFuncObj = GetAddressOfFuncObj();
            if (!addressOfFuncObj || *addressOfFuncObj == nullptr || !VarIs<ScriptFunction>(*addressOfFuncObj))
            {
                return nullptr;
            }

            Js::ScriptFunction* func = (Js::ScriptFunction*)(*addressOfFuncObj);

            RecyclerHeapObjectInfo heapObject;
            Recycler* recycler = threadContext->GetRecycler();

            bool isEntryPointHeapAllocated = recycler->FindHeapObject(func->GetEntryPointInfo(), FindHeapObjectFlags_NoFlags, heapObject);
            bool isFunctionBodyHeapAllocated = recycler->FindHeapObject(func->GetFunctionBody(), FindHeapObjectFlags_NoFlags, heapObject);

            // ensure that all our objects are heap allocated
            if (!(isEntryPointHeapAllocated && isFunctionBodyHeapAllocated))
            {
                return nullptr;
            }
            m_func = func;
            return m_func;
        }
    };

    void CheckWasmMathException(int exceptionCode, ExceptionFilterHelper& helper)
    {
        if (CONFIG_FLAG(WasmMathExFilter) && (exceptionCode == STATUS_INTEGER_DIVIDE_BY_ZERO || exceptionCode == STATUS_INTEGER_OVERFLOW))
        {
            Js::ScriptFunction* func = helper.GetScriptFunction();
            if (func)
            {
                Js::FunctionBody* funcBody = func->GetFunctionBody();
                if (funcBody && funcBody->IsWasmFunction())
                {
                    int32 code = exceptionCode == STATUS_INTEGER_DIVIDE_BY_ZERO ? WASMERR_DivideByZero : VBSERR_Overflow;
                    JavascriptError::ThrowWebAssemblyRuntimeError(func->GetScriptContext(), code);
                }
            }
        }
    }

    // x64 specific exception filters
#ifdef _M_X64
    ArrayAccessDecoder::InstructionData ArrayAccessDecoder::CheckValidInstr(BYTE* &pc, PEXCEPTION_POINTERS exceptionInfo) // get the reg operand and isLoad and
    {
        InstructionData instrData;
        uint prefixValue = 0;
        ArrayAccessDecoder::RexByteValue rexByteValue;
        bool isFloat = false;
        uint  immBytes = 0;
        uint dispBytes = 0;
        bool isImmediate = false;
        bool isSIB = false;
        // Read first  byte - check for prefix
        BYTE* beginPc = pc;


        if (((*pc) == 0x0F2) || ((*pc) == 0x0F3))
        {
            //MOVSD or MOVSS
            prefixValue = *pc;
            isFloat = true;
            pc++;
        }
        else if (*pc == 0x66)
        {
            prefixValue = *pc;
            pc++;
        }

        // Check for Rex Byte - After prefix we should have a rexByte if there is one
        if (*pc >= 0x40 && *pc <= 0x4F)
        {
            rexByteValue.rexValue = *pc;
            uint rexByte = *pc - 0x40;
            if (rexByte & 0x8)
            {
                rexByteValue.isW = true;
            }
            if (rexByte & 0x4)
            {
                rexByteValue.isR = true;
            }
            if (rexByte & 0x2)
            {
                rexByteValue.isX = true;
            }
            if (rexByte & 0x1)
            {
                rexByteValue.isB = true;
            }
            pc++;
        }

        // read opcode
        // Is one of the move instructions , i.e. mov, movsx, movzx, movsxd, movss or movsd
        switch (*pc)
        {
        //MOV - Store
        case 0x89:
        case 0x88:
        {
            pc++;
            instrData.isLoad = false;
            break;
        }
        //MOVSXD
        case 0x63:
        //MOV - Load
        case 0x8A:
        case 0x8B:
        {
            pc++;
            instrData.isLoad = true;
            break;
        }
        case 0x0F:
        {
            // more than one byte opcode and hence we will read pc multiple times
            pc++;
            //MOVSX
            if (*pc == 0xBE || *pc == 0xBF)
            {
                instrData.isLoad = true;
            }
            //MOVZX
            else if (*pc == 0xB6 || *pc == 0xB7)
            {
                instrData.isLoad = true;
            }
            //MOVSS - Load
            else if (*pc == 0x10 && prefixValue == 0xF3)
            {
                Assert(isFloat);
                instrData.isLoad = true;
                instrData.isFloat32 = true;
            }
            //MOVSS - Store
            else if (*pc == 0x11 && prefixValue == 0xF3)
            {
                Assert(isFloat);
                instrData.isLoad = false;
                instrData.isFloat32 = true;
            }
            //MOVSD - Load
            else if (*pc == 0x10 && prefixValue == 0xF2)
            {
                Assert(isFloat);
                instrData.isLoad = true;
                instrData.isFloat64 = true;
            }
            //MOVSD - Store
            else if (*pc == 0x11 && prefixValue == 0xF2)
            {
                Assert(isFloat);
                instrData.isLoad = false;
                instrData.isFloat64 = true;
            }
            //MOVUPS - Load
            else if (*pc == 0x10 && prefixValue == 0)
            {
                instrData.isLoad = true;
                instrData.isSimd = true;
            }
            //MOVUPS - Store
            else if (*pc == 0x11 && prefixValue == 0)
            {
                instrData.isLoad = false;
                instrData.isSimd = true;
            }
            else
            {
                instrData.isInvalidInstr = true;
            }
            pc++;
            break;
        }
        // Support Mov Immediates
        // MOV
        case 0xC6:
        case 0xC7:
        {
            instrData.isLoad = false;
            instrData.isFloat64 = false;
            isImmediate = true;
            if (*pc == 0xC6)
            {
                immBytes = 1;
            }
            else if (rexByteValue.isW) // For MOV, REX.W set means we have a 32 bit immediate value, which gets extended to 64 bit.
            {
                immBytes = 4;
            }
            else
            {
                if (prefixValue == 0x66)
                {
                    immBytes = 2;
                }
                else
                {
                    immBytes = 4;
                }
            }
            pc++;
            break;
        }

        default:
            instrData.isInvalidInstr = true;
            break;
        }
        // if the opcode is not a move return
        if (instrData.isInvalidInstr)
        {
            return instrData;
        }

        //Read ModR/M
        // Read the Src Reg and also check for SIB
        // Add the isR bit to SrcReg and get the actual SRCReg
        // Get the number of bytes for displacement

        //get mod bits
        BYTE modVal = *pc & 0xC0; // first two bits(7th and 6th bits)
        modVal >>= 6;

        //get the R/M bits
        BYTE rmVal = (*pc) & 0x07; // last 3 bits ( 0,1 and 2nd bits)

        //get the reg value
        BYTE dstReg = (*pc) & 0x38; // mask reg bits (3rd 4th and 5th bits)
        dstReg >>= 3;

        Assert(dstReg <= 0x07);
        Assert(modVal <= 0x03);
        Assert(rmVal <= 0x07);

        switch (modVal)
        {
        case 0x00:
            dispBytes = 0;
            break;
        case 0x01:
            dispBytes = 1;
            break;
        case 0x02:
            dispBytes = 4;
            break;
        default:
            instrData.isInvalidInstr = true;
            break;
        }

        if (instrData.isInvalidInstr)
        {
            return instrData;
        }

        // Get the R/M value and see if SIB is present , else get the buffer reg
        if (rmVal == 0x04)
        {
            isSIB = true;
        }
        else
        {
            instrData.bufferReg = rmVal;
        }
        // Get the RegByes from ModRM

        instrData.dstReg = dstReg;

        // increment the modrm byte
        pc++;
        // Check if we have SIB and in that case bufferReg should not be set
        if (isSIB)
        {
            Assert(!instrData.bufferReg);
            // Get the Base and Index Reg from SIB and ensure that Scale is zero
            // We don't care about the Index reg
            // Add the isB value from Rex and get the actual Base Reg
            // Get the base register

            // 6f. Get the array buffer register and its value
            instrData.bufferReg = (*pc % 8);
            pc++;
        }
        // check for the Rex.B value and append it to the base register
        if (rexByteValue.isB)
        {
            instrData.bufferReg |= 1 << 3;
        }
        // check for the Rex.R value and append it to the dst register
        if (rexByteValue.isR)
        {
            instrData.dstReg |= 1 << 3;
        }

        // Get the buffer address - this is always 64 bit GPR
        switch (instrData.bufferReg)
        {
        case 0x0:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rax;
            break;
        case 0x1:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rcx;
            break;
        case 0x2:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rdx;
            break;
        case 0x3:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rbx;
            break;
        case 0x4:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rsp;
            break;
        case 0x5:
            // RBP wouldn't point to an array buffer
            instrData.bufferValue = NULL;
            break;
        case 0x6:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rsi;
            break;
        case 0x7:
            instrData.bufferValue = exceptionInfo->ContextRecord->Rdi;
            break;
        case 0x8:
            instrData.bufferValue = exceptionInfo->ContextRecord->R8;
            break;
        case 0x9:
            instrData.bufferValue = exceptionInfo->ContextRecord->R9;
            break;
        case 0xA:
            instrData.bufferValue = exceptionInfo->ContextRecord->R10;
            break;
        case 0xB:
            instrData.bufferValue = exceptionInfo->ContextRecord->R11;
            break;
        case 0xC:
            instrData.bufferValue = exceptionInfo->ContextRecord->R12;
            break;
        case 0xD:
            instrData.bufferValue = exceptionInfo->ContextRecord->R13;
            break;
        case 0xE:
            instrData.bufferValue = exceptionInfo->ContextRecord->R14;
            break;
        case 0xF:
            instrData.bufferValue = exceptionInfo->ContextRecord->R15;
            break;
        default:
            instrData.isInvalidInstr = true;
            Assert(false);// should never reach here as validation is done before itself
            return instrData;
        }
        // add the pc for displacement , we don't need the displacement Byte value
        if (dispBytes > 0)
        {
            pc = pc + dispBytes;
        }
        instrData.instrSizeInByte = (uint)(pc - beginPc);
        if (isImmediate)
        {
            Assert(immBytes > 0);
            instrData.instrSizeInByte += immBytes;
        }
        // Calculate the number of bytes read in order to get the length of the instruction , ensure that the length should never be greater than 15 bytes
        if (instrData.instrSizeInByte > 15)
        {
            // no instr size can be greater than 15
            instrData.isInvalidInstr = true;
        }
        return instrData;
    }

#if ENABLE_FAST_ARRAYBUFFER
    bool ResumeForOutOfBoundsArrayRefs(int exceptionCode, ExceptionFilterHelper& helper)
    {
        if (exceptionCode != STATUS_ACCESS_VIOLATION)
        {
            return false;
        }
        Js::ScriptFunction* func = helper.GetScriptFunction();
        if (!func)
        {
            return false;
        }

        bool isAsmJs = VarIs<AsmJsScriptFunction>(func);
        bool isWasmOnly = VarIs<WasmScriptFunction>(func);
        uintptr_t faultingAddr = helper.GetFaultingAddress();
        if (isAsmJs)
        {
            AsmJsScriptFunction* asmFunc = VarTo<AsmJsScriptFunction>(func);
            // some extra checks for asm.js because we have slightly more information that we can validate
            if (!asmFunc->GetModuleEnvironment())
            {
                return false;
            }

            ArrayBufferBase* arrayBuffer = nullptr;
            size_t reservationSize = 0;
#ifdef ENABLE_WASM
            if (isWasmOnly)
            {
                WebAssemblyMemory* mem = VarTo<WasmScriptFunction>(func)->GetWebAssemblyMemory();
                arrayBuffer = mem->GetBuffer();
                reservationSize = MAX_WASM__ARRAYBUFFER_LENGTH;
            }
            else
#endif
            {
                arrayBuffer = asmFunc->GetAsmJsArrayBuffer();
                reservationSize = MAX_ASMJS_ARRAYBUFFER_LENGTH;
            }

            if (!arrayBuffer || !arrayBuffer->GetBuffer())
            {
                // don't have a heap buffer for asm.js... so this shouldn't be an asm.js heap access
                return false;
            }
            uintptr_t bufferAddr = (uintptr_t)arrayBuffer->GetBuffer();

            uint bufferLength = arrayBuffer->GetByteLength();

            if (!isWasmOnly && !((ArrayBuffer*)arrayBuffer)->IsValidAsmJsBufferLength(bufferLength))
            {
                return false;
            }
            if (faultingAddr < bufferAddr)
            {
                return false;
            }
            if (faultingAddr >= bufferAddr + reservationSize)
            {
                return false;
            }

            if (isWasmOnly)
            {
                // It is possible to have an A/V on other instructions then load/store (ie: xchg for atomics)
                // Which we don't decode at this time
                // We've confirmed the A/V occurred in the Virtual Memory, so just throw now
                JavascriptError::ThrowWebAssemblyRuntimeError(func->GetScriptContext(), WASMERR_ArrayIndexOutOfRange);
            }
        }
        else
        {
            MEMORY_BASIC_INFORMATION info = { 0 };
            size_t size = VirtualQuery((LPCVOID)faultingAddr, &info, sizeof(info));
            if (size == 0)
            {
                return false;
            }
            size_t allocationSize = info.RegionSize + ((uintptr_t)info.BaseAddress - (uintptr_t)info.AllocationBase);
            if (allocationSize != MAX_WASM__ARRAYBUFFER_LENGTH && allocationSize != MAX_ASMJS_ARRAYBUFFER_LENGTH)
            {
                return false;
            }
            if (info.State != MEM_RESERVE)
            {
                return false;
            }
            if (info.Type != MEM_PRIVATE)
            {
                return false;
            }
        }

        PEXCEPTION_POINTERS exceptionInfo = helper.GetExceptionInfo();
        BYTE* pc = (BYTE*)exceptionInfo->ExceptionRecord->ExceptionAddress;
        ArrayAccessDecoder::InstructionData instrData = ArrayAccessDecoder::CheckValidInstr(pc, exceptionInfo);
        // Check If the instruction is valid
        if (instrData.isInvalidInstr)
        {
            return false;
        }

        // If we didn't find the array buffer, ignore
        if (!instrData.bufferValue)
        {
            return false;
        }

        // SIMD loads/stores do bounds checks.
        if (instrData.isSimd)
        {
            return false;
        }

        // Set the dst reg if the instr type is load
        if (instrData.isLoad)
        {
            Var exceptionInfoReg = exceptionInfo->ContextRecord;
            Var* exceptionInfoIntReg = (Var*)((uint64)exceptionInfoReg + offsetof(CONTEXT, Rax)); // offset in the contextRecord for RAX , the assert below checks for any change in the exceptionInfo struct
            Var* exceptionInfoFloatReg = (Var*)((uint64)exceptionInfoReg + offsetof(CONTEXT, Xmm0));// offset in the contextRecord for XMM0 , the assert below checks for any change in the exceptionInfo struct
            Assert((DWORD64)*exceptionInfoIntReg == exceptionInfo->ContextRecord->Rax);
            Assert((uint64)*exceptionInfoFloatReg == exceptionInfo->ContextRecord->Xmm0.Low);

            if (instrData.isLoad)
            {
                double nanVal = JavascriptNumber::NaN;
                if (instrData.isFloat64)
                {
                    double* destRegLocation = (double*)((uint64)exceptionInfoFloatReg + 16 * (instrData.dstReg));
                    *destRegLocation = nanVal;
                }
                else if (instrData.isFloat32)
                {
                    float* destRegLocation = (float*)((uint64)exceptionInfoFloatReg + 16 * (instrData.dstReg));
                    *destRegLocation = (float)nanVal;
                }
                else
                {
                    uint64* destRegLocation = (uint64*)((uint64)exceptionInfoIntReg + 8 * (instrData.dstReg));
                    *destRegLocation = 0;
                }
            }
        }
        // Add the bytes read to Rip and set it as new Rip
        exceptionInfo->ContextRecord->Rip = exceptionInfo->ContextRecord->Rip + instrData.instrSizeInByte;

        return true;
    }
#endif
#endif
#endif
#endif

    int JavascriptFunction::CallRootEventFilter(int exceptionCode, PEXCEPTION_POINTERS exceptionInfo)
    {
#if ENABLE_NATIVE_CODEGEN
#if defined(_M_IX86) || defined(_M_X64)
        ExceptionFilterHelper helper(exceptionInfo);
        CheckWasmMathException(exceptionCode, helper);
#if ENABLE_FAST_ARRAYBUFFER
        if (ResumeForOutOfBoundsArrayRefs(exceptionCode, helper))
        {
            return EXCEPTION_CONTINUE_EXECUTION;
        }
#endif
#endif
#endif
        return EXCEPTION_CONTINUE_SEARCH;
    }

#if DBG
    void JavascriptFunction::VerifyEntryPoint()
    {
        JavascriptMethod callEntryPoint = this->GetType()->GetEntryPoint();
        if (this->IsCrossSiteObject())
        {
            Assert(CrossSite::IsThunk(callEntryPoint));
        }
        else if (VarIs<ScriptFunction>(this))
        {
        }
        else
        {
            JavascriptMethod originalEntryPoint = this->GetFunctionInfo()->GetOriginalEntryPoint();
            Assert(callEntryPoint == originalEntryPoint || callEntryPoint == ProfileEntryThunk
                || (this->GetScriptContext()->GetHostScriptContext()
                    && this->GetScriptContext()->GetHostScriptContext()->IsHostCrossSiteThunk(callEntryPoint))
                );
        }
    }
#endif

    /*static*/
    PropertyId const JavascriptFunction::specialPropertyIds[] =
    {
        PropertyIds::caller,
        PropertyIds::arguments
    };

    bool JavascriptFunction::HasRestrictedProperties() const
    {
        return !(
            this->functionInfo->IsClassMethod() ||
            this->functionInfo->IsClassConstructor() ||
            this->functionInfo->IsMethod() ||
            this->functionInfo->IsLambda() ||
            this->functionInfo->IsAsync() ||
            this->IsGeneratorFunction() ||
            this->IsStrictMode() ||
            !this->IsScriptFunction() || // -> (BoundFunction || RuntimeFunction) // (RuntimeFunction = Native-defined built-in library functions)
            this->IsLibraryCode() || // JS-defined built-in library functions
            this == this->GetLibrary()->GetFunctionPrototype() // the intrinsic %FunctionPrototype% (original value of Function.prototype)
            );
    }

    PropertyQueryFlags JavascriptFunction::HasPropertyQuery(PropertyId propertyId, _Inout_opt_ PropertyValueInfo* info)
    {
        switch (propertyId)
        {
        case PropertyIds::caller:
        case PropertyIds::arguments:
            if (this->HasRestrictedProperties())
            {
                return PropertyQueryFlags::Property_Found;
            }
            break;
        }
        return DynamicObject::HasPropertyQuery(propertyId, info);
    }

    _Check_return_ _Success_(return) BOOL JavascriptFunction::GetAccessors(PropertyId propertyId, _Outptr_result_maybenull_ Var* getter, _Outptr_result_maybenull_ Var* setter, ScriptContext* requestContext)
    {
        Assert(!this->IsBoundFunction());
        Assert(propertyId != Constants::NoProperty);
        Assert(getter);
        Assert(setter);
        Assert(requestContext);

        if (this->HasRestrictedProperties())
        {
            switch (propertyId)
            {
            case PropertyIds::caller:
            case PropertyIds::arguments:
                if (this->GetEntryPoint() == JavascriptFunction::PrototypeEntryPoint)
                {
                    *setter = *getter = requestContext->GetLibrary()->GetThrowTypeErrorRestrictedPropertyAccessorFunction();
                    return true;
                }
                break;
            }
        }

        return __super::GetAccessors(propertyId, getter, setter, requestContext);
    }

    DescriptorFlags JavascriptFunction::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        if (GetSetterBuiltIns(propertyId, setterValue, info, requestContext, &flags))
        {
            return flags;
        }

        return __super::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags JavascriptFunction::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetSetterBuiltIns(propertyRecord->GetPropertyId(), setterValue, info, requestContext, &flags))
        {
            return flags;
        }

        return __super::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    bool JavascriptFunction::GetSetterBuiltIns(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext, DescriptorFlags* descriptorFlags)
    {
        Assert(propertyId != Constants::NoProperty);
        Assert(setterValue);
        Assert(requestContext);

        switch (propertyId)
        {
        case PropertyIds::caller:
        case PropertyIds::arguments:
            if (this->HasRestrictedProperties()) {
                PropertyValueInfo::SetNoCache(info, this);
                if (this->GetEntryPoint() == JavascriptFunction::PrototypeEntryPoint)
                {
                    *setterValue = requestContext->GetLibrary()->GetThrowTypeErrorRestrictedPropertyAccessorFunction();
                    *descriptorFlags = Accessor;
                }
                else
                {
                    *descriptorFlags = Data;
                }
                return true;
            }
            break;
        }

        return false;
    }

    BOOL JavascriptFunction::IsConfigurable(PropertyId propertyId)
    {
        if (DynamicObject::GetPropertyIndex(propertyId) == Constants::NoSlot)
        {
            switch (propertyId)
            {
            case PropertyIds::caller:
            case PropertyIds::arguments:
                if (this->HasRestrictedProperties())
                {
                    return false;
                }
                break;
            }
        }
        return DynamicObject::IsConfigurable(propertyId);
    }

    BOOL JavascriptFunction::IsEnumerable(PropertyId propertyId)
    {
        if (DynamicObject::GetPropertyIndex(propertyId) == Constants::NoSlot)
        {
            switch (propertyId)
            {
            case PropertyIds::caller:
            case PropertyIds::arguments:
                if (this->HasRestrictedProperties())
                {
                    return false;
                }
                break;
            }
        }
        return DynamicObject::IsEnumerable(propertyId);
    }

    BOOL JavascriptFunction::IsWritable(PropertyId propertyId)
    {
        if (DynamicObject::GetPropertyIndex(propertyId) == Constants::NoSlot)
        {
            switch (propertyId)
            {
            case PropertyIds::caller:
            case PropertyIds::arguments:
                if (this->HasRestrictedProperties())
                {
                    return false;
                }
                break;
            }
        }
        return DynamicObject::IsWritable(propertyId);
    }

    BOOL JavascriptFunction::GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext)
    {
        uint length = GetSpecialPropertyCount();
        if (index < length)
        {
            Assert(DynamicObject::GetPropertyIndex(specialPropertyIds[index]) == Constants::NoSlot);
            *propertyName = requestContext->GetPropertyString(specialPropertyIds[index]);
            return true;
        }

        return false;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptFunction::GetSpecialPropertyCount() const
    {
        return this->HasRestrictedProperties() ? _countof(specialPropertyIds) : 0;
    }

    // Returns the list of special non-enumerable properties for the type.
    PropertyId const * JavascriptFunction::GetSpecialPropertyIds() const
    {
        return specialPropertyIds;
    }

    PropertyQueryFlags JavascriptFunction::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptFunction::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    JavascriptFunction* JavascriptFunction::FindCaller(BOOL* foundThis, JavascriptFunction* nullValue, ScriptContext* requestContext)
    {
        ScriptContext* scriptContext = this->GetScriptContext();

        JavascriptFunction* funcCaller = nullValue;
        JavascriptStackWalker walker(scriptContext);

        if (walker.WalkToTarget(this))
        {
            *foundThis = TRUE;
            while (walker.GetCaller(&funcCaller))
            {
                if (walker.IsCallerGlobalFunction())
                {
                    // Caller is global/eval. If it's eval, keep looking.
                    // Otherwise, return null.
                    if (walker.IsEvalCaller())
                    {
                        continue;
                    }
                    funcCaller = nullValue;
                }
                break;
            }

            if (funcCaller == nullptr)
            {
                // We no longer return Null objects as JavascriptFunctions, so we don't have to worry about
                // cross-context null objects. We do want to clean up null pointers though, since some call
                // later in this function may depend on non-nullptr calls.
                funcCaller = nullValue;
            }

            if (VarIs<ScriptFunction>(funcCaller))
            {
                // If this is the internal function of a generator function then return the original generator function
                funcCaller = VarTo<ScriptFunction>(funcCaller)->GetRealFunctionObject();

                // This function is escaping, so make sure there isn't some nested parent that has a cached scope.
                if (VarIs<ScriptFunction>(funcCaller))
                {
                    FrameDisplay * pFrameDisplay = Js::VarTo<Js::ScriptFunction>(funcCaller)->GetEnvironment();
                    uint length = (uint)pFrameDisplay->GetLength();
                    for (uint i = 0; i < length; i++)
                    {
                        Var scope = pFrameDisplay->GetItem(i);
                        if (scope && !Js::ScopeSlots::Is(scope) && Js::VarIs<Js::ActivationObjectEx>(scope))
                        {
                            Js::VarTo<Js::ActivationObjectEx>(scope)->InvalidateCachedScope();
                        }
                    }
                }
            }
        }

        return StackScriptFunction::EnsureBoxed(BOX_PARAM(funcCaller, nullptr, _u("caller")));
    }

    BOOL JavascriptFunction::GetCallerProperty(Var originalInstance, Var* value, ScriptContext* requestContext)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        *value = nullptr;

        if (this->IsStrictMode())
        {
            return false;
        }

        if (this->GetEntryPoint() == JavascriptFunction::PrototypeEntryPoint)
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptFunction* accessor = requestContext->GetLibrary()->GetThrowTypeErrorRestrictedPropertyAccessorFunction();
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    *value = CALL_FUNCTION(scriptContext->GetThreadContext(), accessor, CallInfo(1), originalInstance);
                }
                END_SAFE_REENTRANT_CALL
            }
            return true;
        }

        JavascriptFunction* nullValue = (JavascriptFunction*)requestContext->GetLibrary()->GetNull();
        if (this->IsLibraryCode()) // Hide .caller for builtins
        {
            *value = nullValue;
            return true;
        }

        // Use a stack walker to find this function's frame. If we find it, find its caller.
        BOOL foundThis = FALSE;
        JavascriptFunction* funcCaller = FindCaller(&foundThis, nullValue, requestContext);

        // WOOB #1142373. We are trying to get the caller in window.onerror = function(){alert(arguments.callee.caller);} case
        // window.onerror is called outside of JavascriptFunction::CallFunction loop, so the caller information is not available
        // in the stack to be found by the stack walker.
        // As we had already walked the stack at throw time retrieve the caller information stored in the exception object
        // The down side is that we can only find the top level caller at thrown time, and won't be able to find caller.caller etc.
        // We'll try to fetch the caller only if we can find the function on the stack, but we can't find the caller if and we are in
        // window.onerror scenario.
        *value = funcCaller;
        if (foundThis && funcCaller == nullValue && scriptContext->GetThreadContext()->HasUnhandledException())
        {
            Js::JavascriptExceptionObject* unhandledExceptionObject = scriptContext->GetThreadContext()->GetUnhandledExceptionObject();
            if (unhandledExceptionObject)
            {
                JavascriptFunction* exceptionFunction = unhandledExceptionObject->GetFunction();
                // This is for getcaller in window.onError. The behavior is different in different browsers
                if (exceptionFunction
                    && scriptContext == exceptionFunction->GetScriptContext()
                    && exceptionFunction->IsScriptFunction()
                    && !exceptionFunction->GetFunctionBody()->GetIsGlobalFunc())
                {
                    *value = exceptionFunction;
                }
            }
        }
        else if (foundThis && scriptContext != funcCaller->GetScriptContext())
        {
            HRESULT hr = scriptContext->GetHostScriptContext()->CheckCrossDomainScriptContext(funcCaller->GetScriptContext());
            if (S_OK != hr)
            {
                *value = nullValue;
            }
            else
            {
                *value = CrossSite::MarshalVar(requestContext, funcCaller, funcCaller->GetScriptContext());
            }
        }

        if (Js::VarIs<Js::JavascriptFunction>(*value) && Js::VarTo<Js::JavascriptFunction>(*value)->IsStrictMode())
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                // ES5.15.3.5.4 [[Get]] (P) -- access to the 'caller' property of strict mode function results in TypeError.
                // Note that for caller coming from remote context (see the check right above) we can't call IsStrictMode()
                // unless CheckCrossDomainScriptContext succeeds. If it fails we don't know whether caller is strict mode
                // function or not and throw if it's not, so just return Null.
                JavascriptError::ThrowTypeError(scriptContext, JSERR_AccessRestrictedProperty);
            }
        }

        return true;
    }

    BOOL JavascriptFunction::GetArgumentsProperty(Var originalInstance, Var* value, ScriptContext* requestContext)
    {
        ScriptContext* scriptContext = this->GetScriptContext();

        if (this->IsStrictMode())
        {
            return false;
        }

        if (this->GetEntryPoint() == JavascriptFunction::PrototypeEntryPoint)
        {
            if (scriptContext->GetThreadContext()->RecordImplicitException())
            {
                JavascriptFunction* accessor = requestContext->GetLibrary()->GetThrowTypeErrorRestrictedPropertyAccessorFunction();
                BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
                {
                    *value = CALL_FUNCTION(scriptContext->GetThreadContext(), accessor, CallInfo(1), originalInstance);
                }
                END_SAFE_REENTRANT_CALL
            }
            return true;
        }

        if (!this->IsScriptFunction())
        {
            // builtin function do not have an argument object - return null.
            *value = scriptContext->GetLibrary()->GetNull();
            return true;
        }

        // Use a stack walker to find this function's frame. If we find it, compute its arguments.
        // Note that we are currently unable to guarantee that the binding between formal arguments
        // and foo.arguments[n] will be maintained after this object is returned.

        JavascriptStackWalker walker(scriptContext);

        if (walker.WalkToTarget(this))
        {
            if (walker.IsCallerGlobalFunction())
            {
                *value = requestContext->GetLibrary()->GetNull();
            }
            else
            {
                Var args = nullptr;
                // Since the arguments will be returned back to script, box the arguments to ensure a copy of
                // them with their own lifetime (as well as move any from the stack to the heap).

                const CallInfo callInfo = walker.GetCallInfo();
                args = JavascriptOperators::LoadHeapArguments(
                    this, callInfo.Count - 1,
                    walker.GetJavascriptArgs(true /* boxArgsAndDeepCopy */),
                    scriptContext->GetLibrary()->GetNull(),
                    scriptContext->GetLibrary()->GetNull(),
                    scriptContext,
                    /* formalsAreLetDecls */ false);

                *value = args;
            }
        }
        else
        {
            *value = scriptContext->GetLibrary()->GetNull();
        }
        return true;
    }

    PropertyQueryFlags JavascriptFunction::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext)) ? TRUE : FALSE;

        if (result)
        {
            if (propertyId == PropertyIds::prototype)
            {
                PropertyValueInfo::DisableStoreFieldCache(info);
            }
        }
        else
        {
            GetPropertyBuiltIns(originalInstance, propertyId, value, requestContext, &result);
        }

        return JavascriptConversion::BooleanToPropertyQueryFlags(result);
    }

    PropertyQueryFlags JavascriptFunction::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        PropertyRecord const* propertyRecord = nullptr;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        result = JavascriptConversion::PropertyQueryFlagsToBoolean(DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext)) ? TRUE : FALSE;
        if (result)
        {
            if (propertyRecord != nullptr && propertyRecord->GetPropertyId() == PropertyIds::prototype)
            {
                PropertyValueInfo::DisableStoreFieldCache(info);
            }

            return JavascriptConversion::BooleanToPropertyQueryFlags(result);
        }

        if (propertyRecord != nullptr)
        {
            GetPropertyBuiltIns(originalInstance, propertyRecord->GetPropertyId(), value, requestContext, &result);
        }

        return JavascriptConversion::BooleanToPropertyQueryFlags(result);
    }

    bool JavascriptFunction::GetPropertyBuiltIns(Var originalInstance, PropertyId propertyId, Var* value, ScriptContext* requestContext, BOOL* result)
    {
        if (propertyId == PropertyIds::caller && this->HasRestrictedProperties())
        {
            *result = GetCallerProperty(originalInstance, value, requestContext);
            return true;
        }

        if (propertyId == PropertyIds::arguments && this->HasRestrictedProperties())
        {
            *result = GetArgumentsProperty(originalInstance, value, requestContext);
            return true;
        }

        *result = false;
        return false;
    }

    BOOL JavascriptFunction::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        bool isReadOnly = false;
        switch (propertyId)
        {
        case PropertyIds::caller:
            if (this->HasRestrictedProperties())
            {
                isReadOnly = true;
            }
            break;

        case PropertyIds::arguments:
            if (this->HasRestrictedProperties())
            {
                isReadOnly = true;
            }
            break;
        }

        if (isReadOnly)
        {
            JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());
            return false;
        }

        BOOL result = DynamicObject::SetProperty(propertyId, value, flags, info);

        if (propertyId == PropertyIds::prototype || propertyId == PropertyIds::_symbolHasInstance)
        {
            PropertyValueInfo::SetNoCache(info, this);
            InvalidateConstructorCacheOnPrototypeChange();
            this->GetScriptContext()->GetThreadContext()->InvalidateIsInstInlineCachesForFunction(this);
        }

        return result;
    }

    BOOL JavascriptFunction::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
        BOOL result = __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);

        if (propertyId == PropertyIds::prototype || propertyId == PropertyIds::_symbolHasInstance)
        {
            PropertyValueInfo::SetNoCache(info, this);
            InvalidateConstructorCacheOnPrototypeChange();
            this->GetScriptContext()->GetThreadContext()->InvalidateIsInstInlineCachesForFunction(this);
        }

        return result;
    }

    BOOL JavascriptFunction::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr)
        {
            return JavascriptFunction::SetProperty(propertyRecord->GetPropertyId(), value, flags, info);
        }
        else
        {
            return DynamicObject::SetProperty(propertyNameString, value, flags, info);
        }
    }

    BOOL JavascriptFunction::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        switch (propertyId)
        {
        case PropertyIds::caller:
        case PropertyIds::arguments:
            if (this->HasRestrictedProperties())
            {
                JavascriptError::ThrowCantDeleteIfStrictMode(flags, this->GetScriptContext(), this->GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());
                return false;
            }
            break;
        }

        BOOL result = DynamicObject::DeleteProperty(propertyId, flags);

        if (result && (propertyId == PropertyIds::prototype || propertyId == PropertyIds::_symbolHasInstance))
        {
            InvalidateConstructorCacheOnPrototypeChange();
            this->GetScriptContext()->GetThreadContext()->InvalidateIsInstInlineCachesForFunction(this);
            if (propertyId == PropertyIds::prototype)
            {
                this->GetTypeHandler()->ClearHasKnownSlot0();
            }
        }

        return result;
    }

    BOOL JavascriptFunction::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        if (BuiltInPropertyRecords::caller.Equals(propertyNameString) || BuiltInPropertyRecords::arguments.Equals(propertyNameString))
        {
            if (this->HasRestrictedProperties())
            {
                JavascriptError::ThrowCantDeleteIfStrictMode(flags, this->GetScriptContext(), propertyNameString->GetString());
                return false;
            }
        }

        BOOL result = DynamicObject::DeleteProperty(propertyNameString, flags);

        if (result && (BuiltInPropertyRecords::prototype.Equals(propertyNameString) || BuiltInPropertyRecords::_symbolHasInstance.Equals(propertyNameString)))
        {
            InvalidateConstructorCacheOnPrototypeChange();
            this->GetScriptContext()->GetThreadContext()->InvalidateIsInstInlineCachesForFunction(this);
            if (BuiltInPropertyRecords::prototype.Equals(propertyNameString))
            {
                this->GetTypeHandler()->ClearHasKnownSlot0();
            }
        }

        return result;
    }

    void JavascriptFunction::InvalidateConstructorCacheOnPrototypeChange()
    {
        Assert(this->constructorCache != nullptr);

#if DBG_DUMP
        if (PHASE_TRACE1(Js::ConstructorCachePhase))
        {
            // This is under DBG_DUMP so we can allow a check
            ParseableFunctionInfo* body = this->GetFunctionProxy() != nullptr ? this->GetFunctionProxy()->EnsureDeserialized() : nullptr;
            const char16* ctorName = body != nullptr ? body->GetDisplayName() : _u("<unknown>");

            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

            Output::Print(_u("CtorCache: before invalidating cache (0x%p) for ctor %s (%s): "), PointerValue(this->constructorCache), ctorName,
                body ? body->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
            this->constructorCache->Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif

        this->constructorCache->InvalidateOnPrototypeChange();

#if DBG_DUMP
        if (PHASE_TRACE1(Js::ConstructorCachePhase))
        {
            // This is under DBG_DUMP so we can allow a check
            ParseableFunctionInfo* body = this->GetFunctionProxy() != nullptr ? this->GetFunctionProxy()->EnsureDeserialized() : nullptr;
            const char16* ctorName = body != nullptr ? body->GetDisplayName() : _u("<unknown>");
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

            Output::Print(_u("CtorCache: after invalidating cache (0x%p) for ctor %s (%s): "), PointerValue(this->constructorCache), ctorName,
                body ? body->GetDebugNumberSet(debugStringBuffer) : _u("(null)"));
            this->constructorCache->Dump();
            Output::Print(_u("\n"));
            Output::Flush();
        }
#endif

    }

    BOOL JavascriptFunction::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        JavascriptString * pString = NULL;

        Var sourceString = this->GetSourceString();

        if (sourceString == nullptr)
        {
            FunctionProxy* proxy = this->GetFunctionProxy();
            if (proxy)
            {
                ParseableFunctionInfo * func = proxy->EnsureDeserialized();
                Utf8SourceInfo* sourceInfo = func->GetUtf8SourceInfo();
                if (sourceInfo->GetIsLibraryCode())
                {
                    charcount_t displayNameLength = 0;
                    pString = JavascriptFunction::GetLibraryCodeDisplayString(this->GetScriptContext(), func->GetShortDisplayName(&displayNameLength));
                }
                else
                {
                    utf8::DecodeOptions options = sourceInfo->IsCesu8() ? utf8::doAllowThreeByteSurrogates : utf8::doDefault;
                    charcount_t count = func->LengthInChars();
                    LPCUTF8 pbStart = func->GetToStringSource(_u("JavascriptFunction::GetDiagValueString"));
                    size_t cbLength = func->LengthInBytes();
                    PrintOffsets* printOffsets = func->GetPrintOffsets();
                    if (printOffsets != nullptr)
                    {
                        count += 3*(charcount_t)((printOffsets->cbEndPrintOffset - printOffsets->cbStartPrintOffset) - cbLength);
                        cbLength = printOffsets->cbEndPrintOffset - printOffsets->cbStartPrintOffset;
                    }

                    size_t decodedCount = utf8::DecodeUnitsInto(stringBuilder->AllocBufferSpace(count), pbStart, pbStart + cbLength, options);
                    Assert(decodedCount < MaxCharCount);
                    stringBuilder->IncreaseCount(min(DIAG_MAX_FUNCTION_STRING, (charcount_t)decodedCount));
                    return TRUE;
                }
            }
            else
            {
                pString = GetLibrary()->GetFunctionDisplayString();
            }
        }
        else
        {
            if (TaggedInt::Is(sourceString))
            {
                pString = GetNativeFunctionDisplayString(this->GetScriptContext(), this->GetScriptContext()->GetPropertyString(TaggedInt::ToInt32(sourceString)));
            }
            else
            {
                Assert(VarIs<JavascriptString>(sourceString));
                pString = VarTo<JavascriptString>(sourceString);
            }
        }

        Assert(pString);
        stringBuilder->Append(pString->GetString(), pString->GetLength());

        return TRUE;
    }

    BOOL JavascriptFunction::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (Function)"));
        return TRUE;
    }

    JavascriptString* JavascriptFunction::GetDisplayNameImpl() const
    {
        Assert(this->GetFunctionProxy() != nullptr); // The caller should guarantee a proxy exists
        ParseableFunctionInfo * func = this->GetFunctionProxy()->EnsureDeserialized();
        charcount_t length = 0;
        const char16* name = func->GetShortDisplayName(&length);

        return DisplayNameHelper(name, length);
    }

    JavascriptString* JavascriptFunction::DisplayNameHelper(const char16* name, charcount_t length) const
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        Assert(this->GetFunctionProxy() != nullptr); // The caller should guarantee a proxy exists
        ParseableFunctionInfo * func = this->GetFunctionProxy()->EnsureDeserialized();
        if (func->GetDisplayName() == Js::Constants::FunctionCode)
        {
            // TODO(jahorto): multiple places use pointer equality on the Constants:: string buffers. Consider moving these to StringCacheList.h and use backing buffer pointer equality if need be.
            return JavascriptString::NewWithBuffer(Constants::Anonymous, Constants::AnonymousLength, scriptContext);
        }
        else if (func->GetIsAccessor())
        {
            const char16* accessorName = func->GetDisplayName();
            if (accessorName[0] == _u('g'))
            {
                return JavascriptString::Concat(scriptContext->GetLibrary()->GetGetterFunctionPrefixString(), JavascriptString::NewCopyBuffer(name, length, scriptContext));
            }
            AssertMsg(accessorName[0] == _u('s'), "should be a set");
            return JavascriptString::Concat(scriptContext->GetLibrary()->GetSetterFunctionPrefixString(), JavascriptString::NewCopyBuffer(name, length, scriptContext));
        }
        return JavascriptString::NewCopyBuffer(name, length, scriptContext);
    }

    bool JavascriptFunction::GetFunctionName(JavascriptString** name) const
    {
        Assert(name != nullptr);
        FunctionProxy* proxy = this->GetFunctionProxy();
        JavascriptFunction* thisFunction = const_cast<JavascriptFunction*>(this);

        if (proxy || thisFunction->IsBoundFunction() || JavascriptGeneratorFunction::Test(thisFunction) || JavascriptAsyncFunction::Test(thisFunction))
        {
            *name = GetDisplayNameImpl();
            return true;
        }

        Assert(!VarIs<ScriptFunction>(thisFunction));
        return GetSourceStringName(name);
    }

    bool JavascriptFunction::GetSourceStringName(JavascriptString** name) const
    {
        Assert(name != nullptr);
        ScriptContext* scriptContext = this->GetScriptContext();
        Var sourceString = this->GetSourceString();

        if (sourceString)
        {
            if (TaggedInt::Is(sourceString))
            {
                int32 propertyIdOfSourceString = TaggedInt::ToInt32(sourceString);
                *name = scriptContext->GetPropertyString(propertyIdOfSourceString);
                return true;
            }
            Assert(VarIs<JavascriptString>(sourceString));
            *name = VarTo<JavascriptString>(sourceString);
            return true;
        }
        return false;
    }

    JavascriptString* JavascriptFunction::GetDisplayName() const
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        FunctionProxy* proxy = this->GetFunctionProxy();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        if (proxy)
        {
            ParseableFunctionInfo * func = proxy->EnsureDeserialized();
            if (func->GetDisplayNameIsRecyclerAllocated())
            {
                return JavascriptString::NewWithSz(func->GetDisplayName(), scriptContext);
            }
            else
            {
                return JavascriptString::NewCopySz(func->GetDisplayName(), scriptContext);
            }
        }
        JavascriptString* sourceStringName = nullptr;
        if (GetSourceStringName(&sourceStringName))
        {
            return sourceStringName;
        }

        return library->GetFunctionDisplayString();
    }

    Var JavascriptFunction::GetTypeOfString(ScriptContext * requestContext)
    {
        return requestContext->GetLibrary()->GetFunctionTypeDisplayString();
    }

    // Check if this function is native/script library code
    bool JavascriptFunction::IsLibraryCode() const
    {
        return !this->IsScriptFunction() || this->GetFunctionProxy()->GetUtf8SourceInfo()->GetIsLibraryCode();
    }

    // Implementation of Function.prototype[@@hasInstance](V) as specified in 19.2.3.6 of ES6 spec
    Var JavascriptFunction::EntrySymbolHasInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        if (!JavascriptConversion::IsCallable(args[0]) || args.Info.Count < 2)
        {
            return JavascriptBoolean::ToVar(FALSE, scriptContext);
        }

        RecyclableObject * constructor = VarTo<RecyclableObject>(args[0]);
        Var instance = args[1];

        Assert(VarIs<JavascriptProxy>(constructor) || VarIs<JavascriptFunction>(constructor));
        return JavascriptBoolean::ToVar(constructor->HasInstance(instance, scriptContext, NULL), scriptContext);
    }

    BOOL JavascriptFunction::HasInstance(Var instance, ScriptContext* scriptContext, IsInstInlineCache* inlineCache)
    {
        Var funcPrototype;

        if (this->GetTypeHandler()->GetHasKnownSlot0())
        {
            Assert(this->GetDynamicType()->GetTypeHandler()->GetPropertyId(scriptContext, (PropertyIndex)0) == PropertyIds::prototype);
            funcPrototype = this->GetSlot(0);
        }
        else
        {
            funcPrototype = JavascriptOperators::GetPropertyNoCache(this, PropertyIds::prototype, scriptContext);
        }
        funcPrototype = CrossSite::MarshalVar(scriptContext, funcPrototype);
        return JavascriptFunction::HasInstance(funcPrototype, instance, scriptContext, inlineCache, this);
    }

    BOOL JavascriptFunction::HasInstance(Var funcPrototype, Var instance, ScriptContext * scriptContext, IsInstInlineCache* inlineCache, JavascriptFunction *function)
    {
        BOOL result = FALSE;
        JavascriptBoolean * javascriptResult;

        //
        // if "instance" is not a JavascriptObject, return false
        //
        if (!JavascriptOperators::IsObject(instance))
        {
            // Only update the cache for primitive cache if it is empty already for the JIT fast path
            if (inlineCache && inlineCache->function == nullptr
                && scriptContext == function->GetScriptContext())// only register when function has same scriptContext
            {
                inlineCache->Cache(VarIs<RecyclableObject>(instance) ?
                    UnsafeVarTo<RecyclableObject>(instance)->GetType() : nullptr,
                    function, scriptContext->GetLibrary()->GetFalse(), scriptContext);
            }
            return result;
        }

        // If we have an instance of inline cache, let's try to use it to speed up the operation.
        // We would like to catch all cases when we already know (by having checked previously)
        // that an object on the left of instance of has been created by a function on the right,
        // as well as when we already know the object on the left has not been created by a function on the right.
        // In practice, we can do so only if the function matches the function in the cache, and the object's type matches the
        // type in the cache.  Notably, this typically means that if some of the objects evolved after construction,
        // while others did not, we will miss the cache for one of the two (sets of objects).
        // An important subtlety here arises when a function is called from different script contexts.
        // Suppose we called function foo from script context A, and we pass it an object o created in the same script context.
        // When function foo checks if object o is an instance of itself (function foo) for the first time (from context A) we will
        // populate the cache with function foo and object o's type (which is permanently bound to the script context A,
        // in which object o was created). If we later invoked function foo from script context B and perform the same instance-of check,
        // the function will still match the function in the cache (because objects' identities do not change during cross-context marshalling).
        // However, object o's type (even if it is of the same "shape" as before) will be different, because the object types are permanently
        // bound and unique to the script context from which they were created.  Hence, the cache may miss, even if the function matches.
        if (inlineCache != nullptr)
        {
            Assert(function != nullptr);
            if (inlineCache->TryGetResult(instance, function, &javascriptResult))
            {
                return javascriptResult == scriptContext->GetLibrary()->GetTrue();
            }
        }

        // If we are here, then me must have missed the cache.  This may be because:
        // a) the cache has never been populated in the first place,
        // b) the cache has been populated, but for an object of a different type (even if the object was created by the same constructor function),
        // c) the cache has been populated, but for a different function,
        // d) the cache has been populated, even for the same object type and function, but has since been invalidated, because the function's
        //    prototype property has been changed (see JavascriptFunction::SetProperty and ThreadContext::InvalidateIsInstInlineCachesForFunction).

        // We may even miss the cache if we ask again about the very same object the very same function the cache was populated with.
        // This subtlety arises when a function is called from two (or more) different script contexts.
        // Suppose we called function foo from script context A, and passed it an object o created in the same script context.
        // When function foo checks if object o is an instance of itself (function foo) for the first time (from context A) we will
        // populate the cache with function foo and object o's type (which is permanently bound to the script context A,
        // in which object o was created). If we later invoked function foo from script context B and perform the same instance of check,
        // the function will still match the function in the cache (because objects' identities do not change during cross-context marshalling).
        // However, object o's type (even if it is of the same "shape" as before, and even if o is the very same object) will be different,
        // because the object types are permanently bound and unique to the script context from which they were created.

        RecyclableObject* instanceObject = VarTo<RecyclableObject>(instance);
        Var prototype = JavascriptOperators::GetPrototype(instanceObject);

        if (!JavascriptOperators::IsObject(funcPrototype))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidPrototype);
        }

        // Since we missed the cache, we must now walk the prototype chain of the object to check if the given function's prototype is somewhere in
        // that chain. If it is, we return true. Otherwise (i.e., we hit the end of the chain before finding the function's prototype) we return false.
        while (!JavascriptOperators::IsNull(prototype))
        {
            if (prototype == funcPrototype)
            {
                result = TRUE;
                break;
            }

            prototype = JavascriptOperators::GetPrototype(VarTo<RecyclableObject>(prototype));
        }

        // Now that we know the answer, let's cache it for next time if we have a cache.
        if (inlineCache != NULL)
        {
            Assert(function != NULL);
            JavascriptBoolean * boolResult = result ? scriptContext->GetLibrary()->GetTrue() :
                scriptContext->GetLibrary()->GetFalse();
            Type * instanceType = VarTo<RecyclableObject>(instance)->GetType();

            if (!instanceType->HasSpecialPrototype()
                && scriptContext == function->GetScriptContext()) // only register when function has same scriptContext, otherwise when scriptContext close
                                                                  // and the isInst inline cache chain will be broken by clearing the arenaAllocator
            {
                inlineCache->Cache(instanceType, function, boolResult, scriptContext);
            }
        }

        return result;
    }

#ifdef ALLOW_JIT_REPRO
    Var JavascriptFunction::EntryInvokeJit(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // todo:: make it work with inproc jit
        if (!JITManager::GetJITManager()->IsOOPJITEnabled())
        {
            Output::Print(_u("Out of proc jit is necessary to repro using an encoded buffer"));
            Js::Throw::FatalInternalError();
        }

        if (args.Info.Count < 2 || !VarIs<ArrayBufferBase>(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedArrayBufferObject);
        }

        ArrayBufferBase* arrayBuffer = VarTo<ArrayBufferBase>(args[1]);
        const byte* buffer = arrayBuffer->GetBuffer();
        uint32 size = arrayBuffer->GetByteLength();
        HRESULT hr = JitFromEncodedWorkItem(scriptContext->GetNativeCodeGenerator(), buffer, size);
        if (FAILED(hr))
        {
#ifdef _WIN32
            char16* lpMsgBuf = nullptr;
            DWORD bufLen = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);
            if (bufLen)
            {
                JavascriptString* string = JavascriptString::NewCopyBuffer(lpMsgBuf, bufLen, scriptContext);
                LocalFree(lpMsgBuf);
                JavascriptExceptionOperators::OP_Throw(string, scriptContext);
            }
#endif
            return JavascriptNumber::New(hr, scriptContext);
        }
        return scriptContext->GetLibrary()->GetUndefined();
    }
#endif


