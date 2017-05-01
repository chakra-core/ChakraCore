//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "EngineInterfaceObject.h"
#include "BuiltInEngineInterfaceExtensionObject.h"
#include "Types/DeferredTypeHandler.h"

#ifdef ENABLE_BUILTIN_OBJECT
#include "ByteCode/ByteCodeSerializer.h"
#include "errstr.h"
#include "ByteCode/ByteCodeDumper.h"

#pragma warning(push)
#pragma warning(disable:4309) // truncation of constant value
#pragma warning(disable:4838) // conversion from 'int' to 'const char' requires a narrowing conversion

#if DISABLE_JIT
#if TARGET_64
#include "BuiltIn/BuiltIn.js.nojit.bc.64b.h"
#else
#include "BuiltIn/BuiltIn.js.nojit.bc.32b.h"
#endif // TARGET_64
#else
#if TARGET_64
#include "BuiltIn/BuiltIn.js.bc.64b.h"
#else
#include "BuiltIn/BuiltIn.js.bc.32b.h"
#endif // TARGET_64
#endif // DISABLE_JIT

#pragma warning(pop)

#define IfFailAssertMsgAndThrowHr(op, msg) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, msg); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#endif // ENABLE_BUILTIN_OBJECT

namespace Js
{
#ifdef ENABLE_BUILTIN_OBJECT

    BuiltInEngineInterfaceExtensionObject::BuiltInEngineInterfaceExtensionObject(ScriptContext * scriptContext) :
        EngineExtensionObjectBase(EngineInterfaceExtensionKind_BuiltIn, scriptContext),
        builtInByteCode(nullptr),
        wasInitialized(false)
    {
    }

    NoProfileFunctionInfo BuiltInEngineInterfaceExtensionObject::EntryInfo::BuiltIn_RegisterFunction(FORCE_NO_WRITE_BARRIER_TAG(BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_RegisterFunction));

    NoProfileFunctionInfo BuiltInEngineInterfaceExtensionObject::EntryInfo::BuiltIn_Internal_ToLengthFunction(FORCE_NO_WRITE_BARRIER_TAG(BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_Internal_ToLengthFunction));
    NoProfileFunctionInfo BuiltInEngineInterfaceExtensionObject::EntryInfo::BuiltIn_Internal_ToIntegerFunction(FORCE_NO_WRITE_BARRIER_TAG(BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_Internal_ToIntegerFunction));

    void BuiltInEngineInterfaceExtensionObject::Initialize()
    {
        if (wasInitialized)
        {
            return;
        }

        JavascriptLibrary* library = scriptContext->GetLibrary();
        DynamicObject* commonObject = library->GetEngineInterfaceObject()->GetCommonNativeInterfaces();
        if (scriptContext->IsBuiltInEnabled())
        {
            Assert(library->GetEngineInterfaceObject() != nullptr);
            this->builtInNativeInterfaces = DynamicObject::New(library->GetRecycler(),
                DynamicType::New(scriptContext, TypeIds_Object, commonObject, nullptr,
                    DeferredTypeHandler<InitializeBuiltInNativeInterfaces>::GetDefaultInstance()));
            library->AddMember(library->GetEngineInterfaceObject(), Js::PropertyIds::BuiltIn, this->builtInNativeInterfaces);
        }

        wasInitialized = true;
    }

    void BuiltInEngineInterfaceExtensionObject::InjectBuiltInLibraryCode(ScriptContext * scriptContext)
    {
        if (this->builtInByteCode != nullptr)
        {
            return;
        }

        try {
            this->EnsureBuiltInByteCode(scriptContext);
            Assert(builtInByteCode != nullptr);

            Js::ScriptFunction *function = scriptContext->GetLibrary()->CreateScriptFunction(builtInByteCode->GetNestedFunctionForExecution(0));

            // If we are profiling, we need to register the script to the profiler callback, so the script compiled event will be sent.
            if (scriptContext->IsProfiling())
            {
                scriptContext->RegisterScript(function->GetFunctionProxy());
            }
            // Mark we are profiling library code already, so that any initialization library code called here won't be reported to profiler
            AutoProfilingUserCode autoProfilingUserCode(scriptContext->GetThreadContext(), /*isProfilingUserCode*/false);

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
            auto ex = err.GetAndClear();
            ex->GetScriptContext();
            JavascriptError::ThrowTypeError(scriptContext, JSERR_BuiltInNotAvailable);
        }
    }

    void BuiltInEngineInterfaceExtensionObject::InitializeBuiltInNativeInterfaces(DynamicObject * builtInNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(builtInNativeInterfaces, mode, 16);

        ScriptContext* scriptContext = builtInNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerFunction, &BuiltInEngineInterfaceExtensionObject::EntryInfo::BuiltIn_RegisterFunction, 2);

        builtInNativeInterfaces->SetHasNoEnumerableProperties(true);
    }

#if DBG
    void BuiltInEngineInterfaceExtensionObject::DumpByteCode()
    {
        Output::Print(_u("Dumping Built In Byte Code:"));
        this->EnsureBuiltInByteCode(scriptContext);
        Js::ByteCodeDumper::DumpRecursively(builtInByteCode);
    }
#endif // DBG

    void BuiltInEngineInterfaceExtensionObject::EnsureBuiltInByteCode(ScriptContext * scriptContext)
    {
        if (this->builtInByteCode == nullptr)
        {
            SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(Js::Constants::NoHostSourceContext, NULL);

            Assert(sourceContextInfo != nullptr);

            SRCINFO si;
            memset(&si, 0, sizeof(si));
            si.sourceContextInfo = sourceContextInfo;
            SRCINFO *hsi = scriptContext->AddHostSrcInfo(&si);
            uint32 flags = fscrIsLibraryCode | (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling() ? fscrAllowFunctionProxy : 0);

            HRESULT hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, (LPCUTF8)nullptr, hsi, (byte*)Library_Bytecode_builtin, nullptr, &this->builtInByteCode);

            IfFailAssertMsgAndThrowHr(hr, "Failed to deserialize BuiltIn.js bytecode - very probably the bytecode needs to be rebuilt.");
        }
    }

    DynamicObject* BuiltInEngineInterfaceExtensionObject::GetPrototypeFromName(Js::PropertyIds propertyId, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();

        switch (propertyId) {
        case PropertyIds::Array:
            return library->arrayPrototype;

        case PropertyIds::String:
            return library->stringPrototype;

        default:
            AssertMsg(false, "Unable to find a prototype that match with this className.");
            return nullptr;
        }
    }

    Var BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        Assert(args.Info.Count >= 3 && JavascriptObject::Is(args.Values[1]) && JavascriptFunction::Is(args.Values[2]));

        // retrieves arguments
        RecyclableObject* funcInfo = nullptr;
        if (!JavascriptConversion::ToObject(args.Values[1], scriptContext, &funcInfo))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Object.assign"));
        }

        Var classNameProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::className, scriptContext);
        Var methodNameProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::methodName, scriptContext);
        Var argumentsCountProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::argumentsCount, scriptContext);

        Assert(JavascriptString::Is(classNameProperty));
        Assert(JavascriptString::Is(methodNameProperty));
        Assert(TaggedInt::Is(argumentsCountProperty));

        JavascriptString* className = JavascriptString::FromVar(classNameProperty);
        JavascriptString* methodName = JavascriptString::FromVar(methodNameProperty);
        int argumentsCount = TaggedInt::ToInt32(argumentsCountProperty);
        JavascriptFunction* func = JavascriptFunction::FromVar(args.Values[2]);

        // Set the function's display name, as the function we pass in argument are anonym.
        func->GetFunctionProxy()->SetIsPublicLibraryCode();
        func->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(methodName->GetString(), methodName->GetLength(), 0);

        DynamicObject* prototype = GetPrototypeFromName(JavascriptOperators::GetPropertyId(className, scriptContext), scriptContext);
        PropertyIds functionIdentifier = JavascriptOperators::GetPropertyId(methodName, scriptContext);

        // Link the function to the prototype.
        ScriptFunction* scriptFunction = scriptContext->GetLibrary()->CreateScriptFunction(func->GetFunctionProxy());
        scriptFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(argumentsCount), PropertyConfigurable, nullptr);
        scriptContext->GetLibrary()->AddMember(prototype, functionIdentifier, scriptFunction);

        if (functionIdentifier == PropertyIds::indexOf) {
            // Special case for Intl who requires to call the non-overriden (by the user) IndexOf function.
            scriptContext->GetLibrary()->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), Js::PropertyIds::builtInJavascriptArrayEntryIndexOf, scriptFunction);
        }

        //Don't need to return anything
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_Internal_ToLengthFunction(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        Assert(args.Info.Count >= 2);
        return JavascriptNumber::ToVar(JavascriptConversion::ToLength(args.Values[1], scriptContext), scriptContext);
    }

    Var BuiltInEngineInterfaceExtensionObject::EntryBuiltIn_Internal_ToIntegerFunction(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        Assert(args.Info.Count >= 2);
        return JavascriptNumber::ToVarNoCheck(JavascriptConversion::ToInteger(args.Values[1], scriptContext), scriptContext);
    }

#endif // ENABLE_BUILTIN_OBJECT
}
