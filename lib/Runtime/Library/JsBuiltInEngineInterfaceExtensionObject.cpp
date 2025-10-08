//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
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

#define IfFailAssertMsgAndThrowHr(op, msg) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, msg); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

enum class IsTypeStatic : bool
{
    prototype = false,
    constructor = true,
    object = true
};

namespace Js
{

    JsBuiltInEngineInterfaceExtensionObject::JsBuiltInEngineInterfaceExtensionObject(ScriptContext * scriptContext) :
        EngineExtensionObjectBase(EngineInterfaceExtensionKind_JsBuiltIn, scriptContext)
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

    void JsBuiltInEngineInterfaceExtensionObject::InjectJsBuiltInLibraryCode(ScriptContext * scriptContext, JsBuiltInFile file)
    {
        JavascriptExceptionObject *pExceptionObject = nullptr;
        FunctionBody* jsBuiltInByteCode = nullptr;

        switch (file)
        {
            #define jsBuiltInByteCodeCase(class, type, obj) \
            case (JsBuiltInFile::class##_##type): \
            { \
                if (jsBuiltIn##class##_##type##Bytecode != nullptr) \
                    return; \
                EnsureSourceInfo(); \
                uint32 flags = fscrJsBuiltIn | (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling() ? fscrAllowFunctionProxy : 0); \
                SRCINFO* hsi = sourceInfo; \
                Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, (LPCUTF8)nullptr, hsi, (byte*)js::Library_Bytecode_##class##_##type, nullptr, &jsBuiltIn##class##_##type##Bytecode); \
                jsBuiltInByteCode = jsBuiltIn##class##_##type##Bytecode; \
                break; \
            }
            JsBuiltIns(jsBuiltInByteCodeCase)
            #undef jsBuiltInByteCodeCase
        }
        this->SetHasBytecode();

        try {
            AssertOrFailFast(jsBuiltInByteCode != nullptr);

            Js::ScriptFunction *functionGlobal = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode);

            // If we are in the debugger and doing a GetProperty of arguments, then Js Builtins code will be injected on-demand
            // Since it is a cross site call, we marshall not only functionGlobal but also its entire prototype chain
            // The prototype of functionGlobal will be shared by a normal Js function,
            // so marshalling will inadvertently transition the entrypoint of the prototype to a crosssite entrypoint
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
            // specify which set of BuiltIns are currently being loaded
            current = file;

            // Clear disable implicit call bit as initialization code doesn't have any side effect
            {
                ThreadContext::AutoRestoreImplicitFlags autoRestoreImplicitFlags(scriptContext->GetThreadContext(), scriptContext->GetThreadContext()->GetImplicitCallFlags(), scriptContext->GetThreadContext()->GetDisableImplicitFlags());
                scriptContext->GetThreadContext()->ClearDisableImplicitFlags();
                JavascriptFunction::CallRootFunctionInScript(functionGlobal, Js::Arguments(callInfo, args));

                Js::ScriptFunction *functionBuiltins = scriptContext->GetLibrary()->CreateScriptFunction(jsBuiltInByteCode->GetNestedFunctionForExecution(0));
                functionBuiltins->SetPrototype(scriptContext->GetLibrary()->nullValue);
                JavascriptFunction::CallRootFunctionInScript(functionBuiltins, Js::Arguments(callInfo, args));
            }

            if (file == JsBuiltInFile::Array_prototype)
            {
                InitializePrototypes(scriptContext);
            }
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
            switch (file)
            {
                #define clearJsBuiltInByteCodeCase(class, type, obj) \
            case (JsBuiltInFile::class##_##type): \
                jsBuiltIn##class##_##type##Bytecode = nullptr;
                    break;
            JsBuiltIns(clearJsBuiltInByteCodeCase)
            #undef clearJsBuiltInByteCodeCase
            }
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
        int initSlotCapacity = 5; // for register{ChakraLibrary}Function, POSITIVE_INFINITY, NEGATIVE_INFINITY, and GetIteratorPrototype

        typeHandler->Convert(builtInNativeInterfaces, mode, initSlotCapacity);

        ScriptContext* scriptContext = builtInNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        library->AddMember(builtInNativeInterfaces, PropertyIds::POSITIVE_INFINITY, library->GetPositiveInfinite());
        library->AddMember(builtInNativeInterfaces, PropertyIds::NEGATIVE_INFINITY, library->GetNegativeInfinite());

        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerChakraLibraryFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterChakraLibraryFunction, 2);
        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::registerFunction, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_RegisterFunction, 2);
        library->AddFunctionToLibraryObject(builtInNativeInterfaces, Js::PropertyIds::GetIteratorPrototype, &JsBuiltInEngineInterfaceExtensionObject::EntryInfo::JsBuiltIn_Internal_GetIteratorPrototype, 1);

        builtInNativeInterfaces->SetHasNoEnumerableProperties(true);
        return true;
    }

#if DBG
    void JsBuiltInEngineInterfaceExtensionObject::DumpByteCode(JsBuiltInFile file)
    {
        Output::Print(_u("Dumping JS Built Ins Byte Code:\n"));
        switch (file)
        {
            #define fileCase(class, type, obj) \
            case class##_##type: \
                Assert(this->jsBuiltIn##class##_##type##Bytecode != nullptr); \
                Js::ByteCodeDumper::DumpRecursively(this->jsBuiltIn##class##_##type##Bytecode); \
                break;
            JsBuiltIns(fileCase)
            #undef fileCase
        }
    }

    void JsBuiltInEngineInterfaceExtensionObject::DumpByteCode()
    {
        Output::Print(_u("Dumping JS Built Ins Byte Code:\n"));
        #define dumpOne(class, type, obj) \
        if (this->jsBuiltIn##class##_##type##Bytecode != nullptr) \
        { \
            DumpByteCode(JsBuiltInFile::class##_##type); \
        }
        JsBuiltIns(dumpOne)
        #undef dumpOne
    }
#endif // DBG

    void JsBuiltInEngineInterfaceExtensionObject::EnsureSourceInfo()
    {
        if (sourceInfo == nullptr)
        {
            SourceContextInfo* sourceContextInfo = RecyclerNewStructZ(scriptContext->GetRecycler(), SourceContextInfo);
            sourceContextInfo->dwHostSourceContext = Js::Constants::JsBuiltInSourceContext;
            sourceContextInfo->isHostDynamicDocument = true;
            sourceContextInfo->sourceContextId = Js::Constants::JsBuiltInSourceContextId;

            Assert(sourceContextInfo != nullptr);

            SRCINFO si;
            memset((void*)&si, 0, sizeof(si));
            si.sourceContextInfo = sourceContextInfo;
            sourceInfo = scriptContext->AddHostSrcInfo(&si);
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

        AssertOrFailFast(args.Info.Count == 3 && VarIs<ScriptFunction>(args.Values[2]));

        JavascriptLibrary * library = scriptContext->GetLibrary();

        // process function name and get property ID
        JavascriptString* methodString = VarTo<JavascriptString>(args.Values[1]);
        PropertyRecord* methodPropertyRecord = nullptr;
        methodString->GetPropertyRecord((PropertyRecord const **) &methodPropertyRecord, false);
        PropertyId methodPropID = methodPropertyRecord->GetPropertyId();

        DynamicObject *installTarget = nullptr;
        bool isStatic = false;
        PropertyString *classPropString = nullptr;
        JavascriptString *fullName = nullptr;
        JavascriptString *dot = library->GetDotString();
        JsBuiltInFile current = (static_cast<JsBuiltInEngineInterfaceExtensionObject*>(scriptContext->GetLibrary()
            ->GetEngineInterfaceObject()->GetEngineExtension(EngineInterfaceExtensionKind_JsBuiltIn)))->current;

        // determine what object this function is being added too
        switch (current)
        {
            #define file(class, type, obj) \
            case class##_##type: \
                isStatic = static_cast<bool>(IsTypeStatic::##type); \
                installTarget = library->Get##obj##(); \
                classPropString = scriptContext->GetPropertyString(PropertyIds::class); \
                break;
            JsBuiltIns(file)
        }

        Assert(methodString && classPropString && installTarget && methodPropID != PropertyIds::_none);

        if (isStatic)
        {
            fullName = JavascriptString::Concat3(classPropString, dot, methodString);
        }
        else
        {
            JavascriptString *dotPrototypeDot = JavascriptString::Concat3(dot, scriptContext->GetPropertyString(PropertyIds::prototype), dot);
            fullName = JavascriptString::Concat3(classPropString, dotPrototypeDot, methodString);
        }

        ScriptFunction *func = EngineInterfaceObject::CreateLibraryCodeScriptFunction(
            UnsafeVarTo<ScriptFunction>(args.Values[2]),
            fullName,
            false /* isConstructor */,
            true /* isJsBuiltIn */,
            true /* isPublic */
        );

        library->AddMember(installTarget, methodPropID, func);

        // Make function available to other internal facilities that need it
        // applicable for specific functions only - this may need review upon moving other functions into JsBuiltins
        if (current == JsBuiltInFile::Array_prototype)
        {
            switch (methodPropID)
            {
                case PropertyIds::entries:
                    library->arrayPrototypeEntriesFunction = func;
                    break;
                case PropertyIds::values:
                    library->arrayPrototypeValuesFunction = func;
                    library->AddMember(installTarget, PropertyIds::_symbolIterator, func);
                    break;
                case PropertyIds::keys:
                    library->arrayPrototypeKeysFunction = func;
                    break;
                case PropertyIds::forEach:
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInArray_prototype_forEach, func);
                    break;
                case PropertyIds::filter:
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInArray_prototype_filter, func);
                    break;
                case PropertyIds::indexOf:
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInArray_prototype_indexOf, func);
                    break;
                case PropertyIds::reduce:
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInArray_prototype_reduce, func);
                    break;
            }
        }
        else if (current == JsBuiltInFile::Math_object)
        {
            switch (methodPropID)
            {
                case PropertyIds::max:
                    library->mathMax = func;
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInMath_object_max, func);
                    break;
                case PropertyIds::min:
                    library->mathMin = func;
                    library->AddMember(scriptContext->GetLibrary()->GetEngineInterfaceObject()->GetCommonNativeInterfaces(), PropertyIds::builtInMath_object_min, func);
                    break;
            }
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
