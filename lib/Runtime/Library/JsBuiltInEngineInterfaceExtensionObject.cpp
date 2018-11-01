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

#define FUNCTIONKIND_VALUES(VALUE) \
VALUE(Array, values, Prototype) \
VALUE(Array, keys, Prototype) \
VALUE(Array, entries, Prototype) \
VALUE(Array, indexOf, Prototype) \
VALUE(Array, filter, Prototype) \
VALUE(Array, flat, Prototype) \
VALUE(Array, flatMap, Prototype) \
VALUE(Array, forEach, Prototype) \
VALUE(Array, some, Prototype) \
VALUE(Array, sort, Prototype) \
VALUE(Array, every, Prototype) \
VALUE(Array, includes, Prototype) \
VALUE(Array, reduce, Prototype) \
VALUE(Object, fromEntries, Constructor) \
VALUE(Math, max, Object) \
VALUE(Math, min, Object)

enum class FunctionKind
{
#define VALUE(ClassName, methodName, propertyType) ClassName##_##methodName,
    FUNCTIONKIND_VALUES(VALUE) 
#undef VALUE
    Max
};

enum class IsPropertyTypeStatic : bool
{
    Prototype = false,
    Constructor = true,
    Object = true
};

namespace Js
{

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
        library->arrayIteratorPrototype = VarTo<DynamicObject>(JavascriptOperators::GetProperty(VarTo<DynamicObject>(arrayIterator), PropertyIds::prototype, scriptContext));
        library->arrayIteratorPrototypeBuiltinNextFunction = VarTo<JavascriptFunction>(JavascriptOperators::GetProperty(library->arrayIteratorPrototype, PropertyIds::next, scriptContext));
    }

    void JsBuiltInEngineInterfaceExtensionObject::InjectJsBuiltInLibraryCode(ScriptContext * scriptContext)
    {
        JavascriptExceptionObject *pExceptionObject = nullptr;
        if (jsBuiltInByteCode != nullptr)
        {
            return;
        }

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
            Js::ImplicitCallFlags saveImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
            scriptContext->GetThreadContext()->ClearDisableImplicitFlags();
            JavascriptFunction::CallRootFunctionInScript(functionGlobal, Js::Arguments(callInfo, args));
            scriptContext->GetThreadContext()->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags));

            Js::ScriptFunction *functionBuiltins = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode->GetNestedFunctionForExecution(0));
            functionBuiltins->SetPrototype(scriptContext->GetLibrary()->nullValue);

            // Clear disable implicit call bit as initialization code doesn't have any side effect
            saveImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
            scriptContext->GetThreadContext()->ClearDisableImplicitFlags();
            JavascriptFunction::CallRootFunctionInScript(functionBuiltins, Js::Arguments(callInfo, args));
            scriptContext->GetThreadContext()->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags));

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
        int initSlotCapacity = 6; // for register{ChakraLibrary}Function, FunctionKind, POSITIVE_INFINITY, NEGATIVE_INFINITY, and GetIteratorPrototype

        typeHandler->Convert(builtInNativeInterfaces, mode, initSlotCapacity);

        ScriptContext* scriptContext = builtInNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        DynamicObject * functionKindObj = library->CreateObject();

#define VALUE(ClassName, methodName, propertyType) library->AddMember(functionKindObj, PropertyIds::ClassName##_##methodName, JavascriptNumber::ToVar((int)FunctionKind::ClassName##_##methodName, scriptContext));
        FUNCTIONKIND_VALUES(VALUE)
#undef VALUE

        library->AddMember(builtInNativeInterfaces, PropertyIds::FunctionKind, functionKindObj);
        library->AddMember(builtInNativeInterfaces, PropertyIds::POSITIVE_INFINITY, library->GetPositiveInfinite());
        library->AddMember(builtInNativeInterfaces, PropertyIds::NEGATIVE_INFINITY, library->GetNegativeInfinite());

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

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterChakraLibraryFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count >= 3 && VarIs<JavascriptString>(args.Values[1]) && VarIs<ScriptFunction>(args.Values[2]));

        JavascriptLibrary * library = scriptContext->GetLibrary();

        JavascriptString* methodName = UnsafeVarTo<JavascriptString>(args.Values[1]);

        // chakra library functions, since they aren't public, can be constructors (__chakraLibrary.ArrayIterator is one)
        ScriptFunction* func = EngineInterfaceObject::CreateLibraryCodeScriptFunction(
            UnsafeVarTo<ScriptFunction>(args.Values[2]),
            methodName,
            true /* isConstructor */,
            true /* isJsBuiltIn */,
            false /* isPublic */
        );

        PropertyIds functionIdentifier = JavascriptOperators::GetPropertyId(methodName, scriptContext);

        library->AddMember(library->GetChakraLib(), functionIdentifier, func);

        //Don't need to return anything
        return library->GetUndefined();
    }

    Var JsBuiltInEngineInterfaceExtensionObject::EntryJsBuiltIn_RegisterFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        AssertOrFailFast(args.Info.Count == 3 && TaggedInt::Is(args.Values[1]) && VarIs<ScriptFunction>(args.Values[2]));

        JavascriptLibrary * library = scriptContext->GetLibrary();

        FunctionKind funcKind = static_cast<FunctionKind>(TaggedInt::ToInt32(args.Values[1]));
        AssertOrFailFast(funcKind >= (FunctionKind)0 && funcKind < FunctionKind::Max);

        DynamicObject *installTarget = nullptr;
        bool isStatic = false;
        PropertyId methodPropID = PropertyIds::_none;
        PropertyString *methodPropString = nullptr;
        PropertyString *classPropString = nullptr;
        JavascriptString *fullName = nullptr;
        JavascriptString *dot = library->GetDotString();
        switch (funcKind)
        {
#define VALUE(ClassName, methodName, propertyType) \
        case FunctionKind::ClassName##_##methodName: \
            isStatic = static_cast<bool>(IsPropertyTypeStatic::##propertyType); \
            installTarget = library->Get##ClassName##propertyType##(); \
            methodPropID = PropertyIds::methodName; \
            methodPropString = scriptContext->GetPropertyString(methodPropID); \
            classPropString = scriptContext->GetPropertyString(PropertyIds::ClassName); \
            break;
FUNCTIONKIND_VALUES(VALUE)
#undef VALUE
        default:
            AssertOrFailFastMsg(false, "funcKind should never be outside the range of projected values");
        }

        Assert(methodPropString && classPropString && installTarget && methodPropID != PropertyIds::_none);

        if (isStatic)
        {
            fullName = JavascriptString::Concat3(classPropString, dot, methodPropString);
        }
        else
        {
            JavascriptString *dotPrototypeDot = JavascriptString::Concat3(dot, scriptContext->GetPropertyString(PropertyIds::prototype), dot);
            fullName = JavascriptString::Concat3(classPropString, dotPrototypeDot, methodPropString);
        }

        ScriptFunction *func = EngineInterfaceObject::CreateLibraryCodeScriptFunction(
            UnsafeVarTo<ScriptFunction>(args.Values[2]),
            fullName,
            false /* isConstructor */,
            true /* isJsBuiltIn */,
            true /* isPublic */
        );

        library->AddMember(installTarget, methodPropID, func);

        // do extra logic here which didnt easily fit into the macro table
        switch (funcKind)
        {
        case FunctionKind::Array_entries:
            library->arrayPrototypeEntriesFunction = func;
            break;
        case FunctionKind::Array_values:
            library->arrayPrototypeValuesFunction = func;
            library->AddMember(installTarget, PropertyIds::_symbolIterator, func);
            break;
        case FunctionKind::Array_keys:
            library->arrayPrototypeKeysFunction = func;
            break;
        case FunctionKind::Array_forEach:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryForEach, func);
            break;
        case FunctionKind::Array_filter:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryFilter, func);
            break;
        case FunctionKind::Array_indexOf:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryIndexOf, func);
            break;
        case FunctionKind::Array_some:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntrySome, func);
            break;
        case FunctionKind::Array_every:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryEvery, func);
            break;
        case FunctionKind::Array_includes:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryIncludes, func);
            break;
        case FunctionKind::Array_reduce:
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInJavascriptArrayEntryReduce, func);
            break;
        case FunctionKind::Math_max:
            library->mathMax = func;
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInMathMax, func);
            break;
        case FunctionKind::Math_min:
            library->mathMin = func;
            library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInMathMin, func);
            break;
        // FunctionKinds with no entry functions
        case FunctionKind::Array_sort:
        case FunctionKind::Array_flat:
        case FunctionKind::Array_flatMap:
        case FunctionKind::Object_fromEntries:
            break;
        default:
            AssertOrFailFastMsg(false, "funcKind should never be outside the range of projected values");
            break;
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
        Assert(!JavascriptArray::IsNonES5Array(iterable));

        if (VarIs<TypedArrayBase>(iterable))
        {
            typedArrayBase = VarTo<TypedArrayBase>(iterable);
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

        DynamicObject* obj = VarTo<DynamicObject>(args.Values[1]);
        unsigned propCount = TaggedInt::ToUInt32(args.Values[2]);

        Assert(callInfo.Count == 3 + propCount);

        for (unsigned i = 0; i < propCount; i++)
        {
            JavascriptString *propName = VarTo<JavascriptString>(args.Values[i + 3]);
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

        RecyclableObject * obj = VarTo<RecyclableObject>(args.Values[1]);
        double index = JavascriptConversion::ToInteger(args.Values[2], scriptContext);
        AssertOrFailFast(index >= 0);
        JavascriptArray::BigIndex bigIndex(static_cast<uint64>(index));
        Var item = args.Values[3];

        JavascriptArray::CreateDataPropertyOrThrow(obj, bigIndex, item, scriptContext);
        return scriptContext->GetLibrary()->GetTrue();
    }
}
#endif // ENABLE_JS_BUILTINS
