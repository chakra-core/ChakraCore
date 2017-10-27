//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "EngineInterfaceObject.h"
#include "JsBuiltInEngineInterfaceExtensionObject.h"
#include "Types/DeferredTypeHandler.h"

#ifdef ENABLE_JS_BUILTINS
#include "ByteCode/ByteCodeSerializer.h"
#include "errstr.h"
#include "ByteCode/ByteCodeDumper.h"

#pragma warning(push)
#pragma warning(disable:4309) // truncation of constant value
#pragma warning(disable:4838) // conversion from 'int' to 'const char' requires a narrowing conversion

#if DISABLE_JIT
#if TARGET_64
#include "JsBuiltIn/JsBuiltIn.js.nojit.bc.64b.h"
#else
#include "JsBuiltIn/JsBuiltIn.js.nojit.bc.32b.h"
#endif // TARGET_64
#else
#if TARGET_64
#include "JsBuiltIn/JsBuiltIn.js.bc.64b.h"
#else
#include "JsBuiltIn/JsBuiltIn.js.bc.32b.h"
#endif // TARGET_64
#endif // DISABLE_JIT

#pragma warning(pop)

#define IfFailAssertMsgAndThrowHr(op, msg) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, msg); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#endif // ENABLE_JS_BUILTINS

namespace Js
{
#ifdef ENABLE_JS_BUILTINS

    JsBuiltInEngineInterfaceExtensionObject::JsBuiltInEngineInterfaceExtensionObject(ScriptContext * scriptContext) :
        EngineExtensionObjectBase(EngineInterfaceExtensionKind_JsBuiltIn, scriptContext),
        jsBuiltInByteCode(nullptr),
        wasInitialized(false)
    {
    }

    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterFunction(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterFunction));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterChakraLibraryFunction(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterChakraLibraryFunction));

    void JsBuiltInEngineInterfaceExtensionObject::Initialize()
    {
        if (wasInitialized)
        {
            return;
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        DynamicObject* commonObject = library->GetEngineInterfaceObject()->GetCommonNativeInterfaces();
        if (scriptContext->IsJsBuiltInEnabled())
        {
            Assert(library->GetEngineInterfaceObject() != nullptr);
            builtInNativeInterfaces = DynamicObject::New(library->GetRecycler(),
                DynamicType::New(scriptContext, TypeIds_Object, commonObject, nullptr,
                    DeferredTypeHandler<InitializeJsBuiltInNativeInterfaces>::GetDefaultInstance()));
            library->AddMember(library->GetEngineInterfaceObject(), Js::PropertyIds::JsBuiltIn, builtInNativeInterfaces);
        }

        wasInitialized = true;
    }

    void JsBuiltInEngineInterfaceExtensionObject::InjectJsBuiltInLibraryCode(ScriptContext * scriptContext)
    {
        if (jsBuiltInByteCode != nullptr)
        {
            return;
        }

        try {
            EnsureJsBuiltInByteCode(scriptContext);
            Assert(jsBuiltInByteCode != nullptr);

            Js::ScriptFunction *function = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode->GetNestedFunctionForExecution(0));

#ifdef ENABLE_SCRIPT_PROFILING
            // If we are profiling, we need to register the script to the profiler callback, so the script compiled event will be sent.
            if (scriptContext->IsProfiling())
            {
                scriptContext->RegisterScript(function->GetFunctionProxy());
            }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
            // Mark we are profiling library code already, so that any initialization library code called here won't be reported to profiler.
            // Also tell the debugger not to record events during intialization so that we don't leak information about initialization.
            AutoInitLibraryCodeScope autoInitLibraryCodeScope(scriptContext);
#endif

            Js::Var args[] = { scriptContext->GetLibrary()->GetUndefined(), scriptContext->GetLibrary()->GetEngineInterfaceObject() };
            Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args));

            // Clear disable implicit call bit as initialization code doesn't have any side effect
            Js::ImplicitCallFlags saveImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
            scriptContext->GetThreadContext()->ClearDisableImplicitFlags();
            JavascriptFunction::CallRootFunctionInScript(function, Js::Arguments(callInfo, args));
            scriptContext->GetThreadContext()->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags));

#if DBG_DUMP
            if (PHASE_DUMP(Js::ByteCodePhase, function->GetFunctionProxy()) && Js::Configuration::Global.flags.Verbose)
            {
                DumpByteCode();
            }
#endif
        }
        catch (const JavascriptException& err)
        {
            Js::JavascriptExceptionObject* ex = err.GetAndClear();
            ex->GetScriptContext();
            JavascriptError::ThrowTypeError(ex->GetScriptContext(), JSERR_BuiltInNotAvailable);
        }
    }

    bool JsBuiltInEngineInterfaceExtensionObject::InitializeJsBuiltInNativeInterfaces(DynamicObject * builtInNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(builtInNativeInterfaces, mode, 16);

        ScriptContext* scriptContext = builtInNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerChakraLibraryFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterChakraLibraryFunction, 2);
        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterFunction, 2);

        builtInNativeInterfaces->SetHasNoEnumerableProperties(true);
        return true;
    }

#if DBG
    void JsBuiltInEngineInterfaceExtensionObject::DumpByteCode()
    {
        Output::Print(_u("Dumping JS Built Ins Byte Code:\r"));
        EnsureJsBuiltInByteCode(scriptContext);
        Js::ByteCodeDumper::DumpRecursively(jsBuiltInByteCode);
    }
#endif // DBG

    void JsBuiltInEngineInterfaceExtensionObject::EnsureJsBuiltInByteCode(ScriptContext * scriptContext)
    {
        if (jsBuiltInByteCode == nullptr)
        {
            SourceContextInfo* sourceContextInfo = RecyclerNewStructZ(scriptContext->GetRecycler(), SourceContextInfo);
            sourceContextInfo->dwHostSourceContext = Js::Constants::JsBuiltInSourceContext;
            sourceContextInfo->isHostDynamicDocument = true;
            sourceContextInfo->sourceContextId = Js::Constants::JsBuiltInSourceContextId;

            Assert(sourceContextInfo != nullptr);

            SRCINFO si;
            memset(&si, 0, sizeof(si));
            si.sourceContextInfo = sourceContextInfo;
            SRCINFO *hsi = scriptContext->AddHostSrcInfo(&si);
            uint32 flags = fscrJsBuiltIn | (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling() ? fscrAllowFunctionProxy : 0);

            HRESULT hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, (LPCUTF8)nullptr, hsi, (byte*)Library_Bytecode_JsBuiltIn, nullptr, &jsBuiltInByteCode);

            IfFailAssertMsgAndThrowHr(hr, "Failed to deserialize JsBuiltIn.js bytecode - very probably the bytecode needs to be rebuilt.");
        }
    }

    DynamicObject* JsBuiltInEngineInterfaceExtensionObject::GetPrototypeFromName(Js::PropertyIds propertyId, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();

        switch (propertyId) {
        case PropertyIds::Array:
            return library->arrayPrototype;

        case PropertyIds::String:
            return library->stringPrototype;

        case PropertyIds::__chakraLibrary:
            return library->GetChakraLib();

        default:
            AssertMsg(false, "Unable to find a prototype that match with this className.");
            return nullptr;
        }
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterChakraLibraryFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count >= 3 && JavascriptString::Is(args.Values[1]) && JavascriptFunction::Is(args.Values[2]));

        // retrieves arguments
        JavascriptString* methodName = JavascriptString::FromVar(args.Values[1]);
        JavascriptFunction* func = JavascriptFunction::FromVar(args.Values[2]);

        // Set the function's display name, as the function we pass in argument are anonym.
        func->GetFunctionProxy()->SetIsPublicLibraryCode();
        func->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(methodName->GetString(), methodName->GetLength(), 0);

        DynamicObject* chakraLibraryObject = GetPrototypeFromName(PropertyIds::__chakraLibrary, scriptContext);
        PropertyIds functionIdentifier = JavascriptOperators::GetPropertyId(methodName, scriptContext);

        // Link the function to __chakraLibrary.
        ScriptFunction* scriptFunction = scriptContext->GetLibrary()->CreateScriptFunction(func->GetFunctionProxy());
        scriptFunction->SetIsJsBuiltInCode();
        scriptFunction->GetFunctionProxy()->SetIsJsBuiltInCode();

        if (scriptFunction->GetScriptContext()->GetConfig()->IsES6FunctionNameEnabled())
        {
            scriptFunction->SetPropertyWithAttributes(PropertyIds::name, methodName, PropertyConfigurable, nullptr);
        }

        scriptContext->GetLibrary()->AddMember(chakraLibraryObject, functionIdentifier, scriptFunction);

        //Don't need to return anything
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count >= 3 && JavascriptObject::Is(args.Values[1]) && JavascriptFunction::Is(args.Values[2]));

        // retrieves arguments
        RecyclableObject* funcInfo = nullptr;
        if (!JavascriptConversion::ToObject(args.Values[1], scriptContext, &funcInfo))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.assign"));
        }

        Var classNameProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::className, scriptContext);
        Var methodNameProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::methodName, scriptContext);
        Var argumentsCountProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::argumentsCount, scriptContext);
        Var forceInlineProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::forceInline, scriptContext);

        Assert(JavascriptString::Is(classNameProperty));
        Assert(JavascriptString::Is(methodNameProperty));
        Assert(TaggedInt::Is(argumentsCountProperty));

        JavascriptString* className = JavascriptString::FromVar(classNameProperty);
        JavascriptString* methodName = JavascriptString::FromVar(methodNameProperty);
        int argumentsCount = TaggedInt::ToInt32(argumentsCountProperty);

        BOOL forceInline = JavascriptConversion::ToBoolean(forceInlineProperty, scriptContext);

        JavascriptFunction* func = JavascriptFunction::FromVar(args.Values[2]);

        // Set the function's display name, as the function we pass in argument are anonym.
        func->GetFunctionProxy()->SetIsPublicLibraryCode();
        func->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(methodName->GetString(), methodName->GetLength(), 0);

        DynamicObject* prototype = GetPrototypeFromName(JavascriptOperators::GetPropertyId(className, scriptContext), scriptContext);
        PropertyIds functionIdentifier = JavascriptOperators::GetPropertyId(methodName, scriptContext);

        // Link the function to the prototype.
        ScriptFunction* scriptFunction = scriptContext->GetLibrary()->CreateScriptFunction(func->GetFunctionProxy());
        scriptFunction->SetIsJsBuiltInCode();
        scriptFunction->GetFunctionProxy()->SetIsJsBuiltInCode();
        if (forceInline)
        {
            Assert(scriptFunction->HasFunctionBody());
            scriptFunction->GetFunctionBody()->SetJsBuiltInForceInline();
        }
        scriptFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(argumentsCount), PropertyConfigurable, nullptr);

        scriptFunction->SetConfigurable(PropertyIds::prototype, true);
        scriptFunction->DeleteProperty(PropertyIds::prototype, Js::PropertyOperationFlags::PropertyOperation_None);

        if (scriptFunction->GetScriptContext()->GetConfig()->IsES6FunctionNameEnabled())
        {
            scriptFunction->SetPropertyWithAttributes(PropertyIds::name, methodName, PropertyConfigurable, nullptr);
        }

        scriptContext->GetLibrary()->AddMember(prototype, functionIdentifier, scriptFunction);

        //Don't need to return anything
        return scriptContext->GetLibrary()->GetUndefined();
    }

#endif // ENABLE_JS_BUILTINS
}
