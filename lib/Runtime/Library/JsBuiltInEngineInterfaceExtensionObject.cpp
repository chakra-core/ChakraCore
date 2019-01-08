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

    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_GetLength(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_GetLength));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_GetIteratorPrototype(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_GetIteratorPrototype));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_InitInternalProperties(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_InitInternalProperties));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ToLengthFunction(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ToLengthFunction));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ToIntegerFunction(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ToIntegerFunction));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ArraySpeciesCreate(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ArraySpeciesCreate));
    NoProfileFunctionInfo JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow(FORCE_NO_WRITE_BARRIER_TAG(JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow));

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

    void JsBuiltInEngineInterfaceExtensionObject::InitializePrototypes(ScriptContext * scriptContext)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();
        JavascriptString * methodName = JavascriptString::NewWithSz(_u("ArrayIterator"), scriptContext);
        auto arrayIterator = JavascriptOperators::GetProperty(library->GetChakraLib(), JavascriptOperators::GetPropertyId(methodName, scriptContext), scriptContext);
        library->arrayIteratorPrototype = DynamicObject::FromVar(JavascriptOperators::GetProperty(DynamicObject::FromVar(arrayIterator), PropertyIds::prototype, scriptContext));
        library->arrayIteratorPrototypeBuiltinNextFunction = JavascriptFunction::FromVar(JavascriptOperators::GetProperty(library->arrayIteratorPrototype, PropertyIds::next, scriptContext));
    }

    void JsBuiltInEngineInterfaceExtensionObject::InjectJsBuiltInLibraryCode(ScriptContext * scriptContext)
    {
        JavascriptExceptionObject *pExceptionObject = nullptr;
        if (jsBuiltInByteCode != nullptr)
        {
            return;
        }
        
        struct AutoRestoreFlags
        {
            ThreadContext * ctx;
            ImplicitCallFlags savedImplicitCallFlags;
            DisableImplicitFlags savedDisableImplicitFlags;
            AutoRestoreFlags(ThreadContext *ctx, Js::ImplicitCallFlags implFlags, DisableImplicitFlags disableImplFlags) :
                ctx(ctx),
                savedImplicitCallFlags(implFlags),
                savedDisableImplicitFlags(disableImplFlags)
            {
                ctx->ClearDisableImplicitFlags();
            }

            ~AutoRestoreFlags()
            {
                ctx->SetImplicitCallFlags((Js::ImplicitCallFlags)(savedImplicitCallFlags));
                ctx->SetDisableImplicitFlags((DisableImplicitFlags)savedDisableImplicitFlags);
            }
        };

        try {
            EnsureJsBuiltInByteCode(scriptContext);
            Assert(jsBuiltInByteCode != nullptr);

            Js::ScriptFunction *functionGlobal = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode);

            // If we are in the debugger and doing a GetProperty of arguments, then Js Builtins code will be injected on-demand
            // Since it is a cross site call, we marshall not only functionGlobal but also its entire prototype chain
            // The prototype of functionGlobal will be shared by a normal Js function,
            // so marshalling will inadvertantly transition the entrypoint of the prototype to a crosssite entrypoint
            // So we set the prototype to null here
            functionGlobal->SetPrototype(scriptContext->GetLibrary()->nullValue);

#ifdef ENABLE_SCRIPT_PROFILING
            // If we are profiling, we need to register the script to the profiler callback, so the script compiled event will be sent.
            if (scriptContext->IsProfiling())
            {
                scriptContext->RegisterScript(functionGlobal->GetFunctionProxy());
            }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
            // Mark we are profiling library code already, so that any initialization library code called here won't be reported to profiler.
            // Also tell the debugger not to record events during intialization so that we don't leak information about initialization.
            AutoInitLibraryCodeScope autoInitLibraryCodeScope(scriptContext);
#endif

            Js::Var args[] = { scriptContext->GetLibrary()->GetUndefined(), scriptContext->GetLibrary()->GetEngineInterfaceObject() };
            Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args));

#if ENABLE_JS_REENTRANCY_CHECK
            // Create a Reentrancy lock to make sure we correctly restore the lock at the end of BuiltIns initialization
            JsReentLock lock(scriptContext->GetThreadContext());
            // Clear ReentrancyLock bit as initialization code doesn't have any side effect
            scriptContext->GetThreadContext()->SetNoJsReentrancy(false);
#endif
            // Clear disable implicit call bit as initialization code doesn't have any side effect
            {
                AutoRestoreFlags autoRestoreFlags(scriptContext->GetThreadContext(), scriptContext->GetThreadContext()->GetImplicitCallFlags(), scriptContext->GetThreadContext()->GetDisableImplicitFlags());
                JavascriptFunction::CallRootFunctionInScript(functionGlobal, Js::Arguments(callInfo, args));
            }

            Js::ScriptFunction *functionBuiltins = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode->GetNestedFunctionForExecution(0));
            functionBuiltins->SetPrototype(scriptContext->GetLibrary()->nullValue);

            // Clear disable implicit call bit as initialization code doesn't have any side effect
            {
                AutoRestoreFlags autoRestoreFlags(scriptContext->GetThreadContext(), scriptContext->GetThreadContext()->GetImplicitCallFlags(), scriptContext->GetThreadContext()->GetDisableImplicitFlags());
                JavascriptFunction::CallRootFunctionInScript(functionBuiltins, Js::Arguments(callInfo, args));
            }

            InitializePrototypes(scriptContext);
#if DBG_DUMP
            if (PHASE_DUMP(Js::ByteCodePhase, functionGlobal->GetFunctionProxy()) && Js::Configuration::Global.flags.Verbose)
            {
                DumpByteCode();
            }
#endif
        }
        catch (const JavascriptException& err)
        {
            pExceptionObject = err.GetAndClear();
        }

        if (pExceptionObject)
        {
            jsBuiltInByteCode = nullptr;
            if (pExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingSOErrorObject())
            {
                JavascriptExceptionOperators::DoThrowCheckClone(pExceptionObject, scriptContext);
            }
            Js::Throw::FatalJsBuiltInError();
        }
    }

    bool JsBuiltInEngineInterfaceExtensionObject::InitializeJsBuiltInNativeInterfaces(DynamicObject * builtInNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(builtInNativeInterfaces, mode, 16);

        ScriptContext* scriptContext = builtInNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerChakraLibraryFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterChakraLibraryFunction, 2);
        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterFunction, 2);
        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::GetIteratorPrototype, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_GetIteratorPrototype, 1);

        builtInNativeInterfaces->SetHasNoEnumerableProperties(true);
        return true;
    }

#if DBG
    void JsBuiltInEngineInterfaceExtensionObject::DumpByteCode()
    {
        Output::Print(_u("Dumping JS Built Ins Byte Code:\n"));
        Assert(this->jsBuiltInByteCode);
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
            this->SetHasBytecode();
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

    void JsBuiltInEngineInterfaceExtensionObject::RecordCommonNativeInterfaceBuiltIns(Js::PropertyIds propertyId, ScriptContext * scriptContext, JavascriptFunction * scriptFunction)
    {
        PropertyId commonNativeInterfaceId;
        switch (propertyId)
        {
            case PropertyIds::indexOf:
                commonNativeInterfaceId = Js::PropertyIds::builtInJavascriptArrayEntryIndexOf;
                break;

            case PropertyIds::filter:
                commonNativeInterfaceId = Js::PropertyIds::builtInJavascriptArrayEntryFilter;
                break;

            default:
                return;
        }

        scriptContext->GetLibrary()->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), commonNativeInterfaceId, scriptFunction);
    }

    void JsBuiltInEngineInterfaceExtensionObject::RecordDefaultIteratorFunctions(Js::PropertyIds propertyId, ScriptContext * scriptContext, JavascriptFunction * iteratorFunc)
    {
        JavascriptLibrary* library = scriptContext->GetLibrary();

        switch (propertyId) {
        case PropertyIds::entries:
            library->arrayPrototypeEntriesFunction = iteratorFunc;
            break;
        case PropertyIds::values:
            library->arrayPrototypeValuesFunction = iteratorFunc;
            break;
        case PropertyIds::keys:
            library->arrayPrototypeKeysFunction = iteratorFunc;
            break;
        }
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterChakraLibraryFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count >= 3 && JavascriptString::Is(args.Values[1]) && JavascriptFunction::Is(args.Values[2]));

        JavascriptLibrary * library = scriptContext->GetLibrary();

        // retrieves arguments
        JavascriptString* methodName = JavascriptString::FromVar(args.Values[1]);
        JavascriptFunction* func = JavascriptFunction::FromVar(args.Values[2]);

        // Set the function's display name, as the function we pass in argument are anonym.
        func->GetFunctionProxy()->SetIsPublicLibraryCode();
        func->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(methodName->GetString(), methodName->GetLength(), 0);

        DynamicObject* chakraLibraryObject = GetPrototypeFromName(PropertyIds::__chakraLibrary, scriptContext);
        PropertyIds functionIdentifier = JavascriptOperators::GetPropertyId(methodName, scriptContext);

        // Link the function to __chakraLibrary.
        ScriptFunction* scriptFunction = library->CreateScriptFunction(func->GetFunctionProxy());
        scriptFunction->GetFunctionProxy()->SetIsJsBuiltInCode();

        Assert(scriptFunction->HasFunctionBody());
        scriptFunction->GetFunctionBody()->SetJsBuiltInForceInline();

        scriptFunction->SetPropertyWithAttributes(PropertyIds::name, methodName, PropertyConfigurable, nullptr);

        library->AddMember(chakraLibraryObject, functionIdentifier, scriptFunction);

        //Don't need to return anything
        return library->GetUndefined();
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count >= 3 && JavascriptObject::Is(args.Values[1]) && JavascriptFunction::Is(args.Values[2]));

        JavascriptLibrary * library = scriptContext->GetLibrary();

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
        Var aliasProperty = JavascriptOperators::OP_GetProperty(funcInfo, Js::PropertyIds::alias, scriptContext);

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
        PropertyIds functionIdentifier = methodName->BufferEquals(_u("Symbol.iterator"), 15)? PropertyIds::_symbolIterator :
            JavascriptOperators::GetPropertyId(methodName, scriptContext);

        // Link the function to the prototype.
        ScriptFunction* scriptFunction = library->CreateScriptFunction(func->GetFunctionProxy());
        scriptFunction->GetFunctionProxy()->SetIsJsBuiltInCode();

        if (forceInline)
        {
            Assert(scriptFunction->HasFunctionBody());
            scriptFunction->GetFunctionBody()->SetJsBuiltInForceInline();
        }
        scriptFunction->SetPropertyWithAttributes(PropertyIds::length, TaggedInt::ToVarUnchecked(argumentsCount), PropertyConfigurable, nullptr);

        scriptFunction->SetConfigurable(PropertyIds::prototype, true);
        scriptFunction->DeleteProperty(PropertyIds::prototype, Js::PropertyOperationFlags::PropertyOperation_None);

        scriptFunction->SetPropertyWithAttributes(PropertyIds::name, methodName, PropertyConfigurable, nullptr);

        library->AddMember(prototype, functionIdentifier, scriptFunction);

        RecordCommonNativeInterfaceBuiltIns(functionIdentifier, scriptContext, scriptFunction);

        if (!JavascriptOperators::IsUndefinedOrNull(aliasProperty))
        {
            JavascriptString * alias = JavascriptConversion::ToString(aliasProperty, scriptContext);
            // Cannot do a string to property id search here, Symbol.* have different hashing mechanism, so resort to this str compare
            PropertyIds aliasFunctionIdentifier = alias->BufferEquals(_u("Symbol.iterator"), 15) ? PropertyIds::_symbolIterator :
                JavascriptOperators::GetPropertyId(alias, scriptContext);
            library->AddMember(prototype, aliasFunctionIdentifier, scriptFunction);
        }

        if (prototype == library->arrayPrototype)
        {
            RecordDefaultIteratorFunctions(functionIdentifier, scriptContext, scriptFunction);
        }

        //Don't need to return anything
        return library->GetUndefined();
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ToLengthFunction(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        Assert(args.Info.Count == 2);
        return JavascriptNumber::ToVar(JavascriptConversion::ToLength(args.Values[1], scriptContext), scriptContext);
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ToIntegerFunction(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        Assert(args.Info.Count == 2);

        Var value = args.Values[1];
        if (JavascriptOperators::IsUndefinedOrNull(value))
        {
            return TaggedInt::ToVarUnchecked(0);
        }
        else if (TaggedInt::Is(value))
        {
            return value;
        }

        return JavascriptNumber::ToVarIntCheck(JavascriptConversion::ToInteger(value, scriptContext), scriptContext);
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_GetLength(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        Assert(callInfo.Count == 2);

        int64 length;
        Var iterable = args.Values[1];

        TypedArrayBase *typedArrayBase = nullptr;
        Assert(!JavascriptArray::Is(iterable));

        if (TypedArrayBase::Is(iterable))
        {
            typedArrayBase = TypedArrayBase::FromVar(iterable);
            if (typedArrayBase->IsDetachedBuffer())
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
            }

            length = typedArrayBase->GetLength();
        }
        else
        {
            length = JavascriptConversion::ToLength(JavascriptOperators::OP_GetLength(iterable, scriptContext), scriptContext);
        }

        return JavascriptNumber::ToVar(length, scriptContext);
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_GetIteratorPrototype(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        Assert(callInfo.Count == 1);

        return scriptContext->GetLibrary()->iteratorPrototype;
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_InitInternalProperties(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        DynamicObject* obj = DynamicObject::FromVar(args.Values[1]);
        unsigned propCount = TaggedInt::ToUInt32(args.Values[2]);

        Assert(callInfo.Count == 3 + propCount);

        for (unsigned i = 0; i < propCount; i++)
        {
            JavascriptString *propName = JavascriptString::FromVar(args.Values[i + 3]);
            obj->SetPropertyWithAttributes(JavascriptOperators::GetPropertyId(propName, scriptContext), scriptContext->GetLibrary()->GetNull(), PropertyWritable, nullptr);
        }

        return obj;
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ArraySpeciesCreate(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        AssertOrFailFast(args.Info.Count == 3);

        int64 length64 = JavascriptConversion::ToLength(args.Values[2], scriptContext);
        if (length64 > UINT_MAX)
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
        }

        uint32 length = static_cast<uint32>(length64);

        bool isBuiltinArrayCtor = true;
        RecyclableObject * newObj = JavascriptArray::ArraySpeciesCreate(args.Values[1], length, scriptContext, nullptr, nullptr, &isBuiltinArrayCtor);
        nullptr;

        if (newObj == nullptr)
        {
            newObj = scriptContext->GetLibrary()->CreateArray(length);
        }
        else
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
        }

        return newObj;
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_Internal_ArrayCreateDataPropertyOrThrow(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        AssertOrFailFast(args.Info.Count == 4);

        RecyclableObject * obj = RecyclableObject::FromVar(args.Values[1]);
        double index = JavascriptConversion::ToInteger(args.Values[2], scriptContext);
        AssertOrFailFast(index >= 0);
        JavascriptArray::BigIndex bigIndex(static_cast<uint64>(index));
        Var item = args.Values[3];

        JavascriptArray::CreateDataPropertyOrThrow(obj, bigIndex, item, scriptContext);
        return scriptContext->GetLibrary()->GetTrue();
    }
#endif // ENABLE_JS_BUILTINS
}
