//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

using namespace Js;

FunctionInfo JavascriptAsyncGeneratorFunction::functionInfo(
    FORCE_NO_WRITE_BARRIER_TAG(
        JavascriptAsyncGeneratorFunction::EntryAsyncGeneratorFunctionImplementation),
    (FunctionInfo::Attributes)(FunctionInfo::DoNotProfile | FunctionInfo::ErrorOnNew));

JavascriptAsyncGeneratorFunction::JavascriptAsyncGeneratorFunction(
    DynamicType* type,
    GeneratorVirtualScriptFunction* scriptFunction) :
        JavascriptGeneratorFunction(type, &functionInfo, scriptFunction)
{
    DebugOnly(VerifyEntryPoint());
}

JavascriptAsyncGeneratorFunction* JavascriptAsyncGeneratorFunction::New(
    ScriptContext* scriptContext,
    GeneratorVirtualScriptFunction* scriptFunction)
{
    return scriptContext->GetLibrary()->CreateAsyncGeneratorFunction(
        functionInfo.GetOriginalEntryPoint(),
        scriptFunction);
}

template<>
bool Js::VarIsImpl<JavascriptAsyncGeneratorFunction>(RecyclableObject* obj)
{
    return VarIs<JavascriptFunction>(obj) && (
        VirtualTableInfo<JavascriptAsyncGeneratorFunction>::HasVirtualTable(obj) ||
        VirtualTableInfo<CrossSiteObject<JavascriptAsyncGeneratorFunction>>::HasVirtualTable(obj)
    );
}

Var JavascriptAsyncGeneratorFunction::EntryAsyncGeneratorFunctionImplementation(
    RecyclableObject* function,
    CallInfo callInfo, ...)
{
    auto* scriptContext = function->GetScriptContext();
    PROBE_STACK(scriptContext, Js::Constants::MinStackDefault);
    ARGUMENTS(args, callInfo);

    Assert(!(callInfo.Flags & CallFlags_New));

    auto* asyncGeneratorFn = VarTo<JavascriptAsyncGeneratorFunction>(function);
    auto* library = scriptContext->GetLibrary();
    Var prototype = JavascriptOperators::GetPropertyNoCache(function, Js::PropertyIds::prototype, scriptContext);

    // fall back to the original prototype if we have an invalid prototype object
    DynamicObject* protoObject = VarIs<DynamicObject>(prototype) ?
        UnsafeVarTo<DynamicObject>(prototype) : library->GetAsyncGeneratorPrototype();

    auto* scriptFn = asyncGeneratorFn->GetGeneratorVirtualScriptFunction();
    auto* generator = library->CreateAsyncGenerator(args, scriptFn, protoObject);

    // Run the generator to execute until the beginning of the body
    if (scriptFn->GetFunctionInfo()->GetGeneratorWithComplexParams())
    BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
    {
        generator->CallGenerator(library->GetUndefined(), ResumeYieldKind::Normal);
    }
    END_SAFE_REENTRANT_CALL

    generator->SetSuspendedStart();

    return generator;
}
