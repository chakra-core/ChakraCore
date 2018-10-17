//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_JS_BUILTINS) || defined(ENABLE_PROJECTION)

#include "errstr.h"
#include "Library/EngineInterfaceObject.h"
#include "Types/DeferredTypeHandler.h"

#define IfFailThrowHr(op) \
    if (FAILED(hr=(op))) \
    { \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#define IfFailAssertAndThrowHr(op) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, "HRESULT was a failure."); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \


#define SetPropertyOn(obj, propID, value) \
    obj->SetProperty(propID, value, Js::PropertyOperationFlags::PropertyOperation_None, nullptr) \

#define SetStringPropertyOn(obj, propID, propValue) \
    SetPropertyOn(obj, propID, Js::JavascriptString::NewCopySz(propValue, scriptContext)) \

#define SetHSTRINGPropertyOn(obj, propID, hstringValue) \
    SetStringPropertyOn(obj, propID, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#define SetPropertyLOn(obj, literalProperty, value) \
    obj->SetProperty(Js::JavascriptString::NewCopySz(literalProperty, scriptContext), value, Js::PropertyOperationFlags::PropertyOperation_None, nullptr) \

#define SetStringPropertyLOn(obj, literalProperty, propValue) \
    SetPropertyLOn(obj, literalProperty, Js::JavascriptString::NewCopySz(propValue, scriptContext)) \

#define SetHSTRINGPropertyLOn(obj, literalProperty, hstringValue) \
    SetStringPropertyLOn(obj, literalProperty, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#define SetPropertyBuiltInOn(obj, builtInPropID, value) \
    SetPropertyOn(obj, Js::PropertyIds::builtInPropID, value) \

#define SetStringPropertyBuiltInOn(obj, builtInPropID, propValue) \
    SetPropertyBuiltInOn(obj, builtInPropID, Js::JavascriptString::NewCopySz(propValue, scriptContext))

#define SetHSTRINGPropertyBuiltInOn(obj, builtInPropID, hstringValue) \
    SetStringPropertyBuiltInOn(obj, builtInPropID, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#define GetPropertyFrom(obj, propertyID) \
    Js::JavascriptOperators::GetProperty(obj, propertyID, &propertyValue, scriptContext) \

#define GetPropertyLFrom(obj, propertyName) \
    GetPropertyFrom(obj, scriptContext->GetOrAddPropertyIdTracked(propertyName, wcslen(propertyName)))

#define GetPropertyBuiltInFrom(obj, builtInPropID) \
    GetPropertyFrom(obj, Js::PropertyIds::builtInPropID) \

#define GetTypedPropertyBuiltInFrom(obj, builtInPropID, Type) \
    (GetPropertyFrom(obj, Js::PropertyIds::builtInPropID) && VarIs<Type>(propertyValue)) \

#define HasPropertyOn(obj, propID) \
    Js::JavascriptOperators::HasProperty(obj, propID) \

#define HasPropertyBuiltInOn(obj, builtInPropID) \
    HasPropertyOn(obj, Js::PropertyIds::builtInPropID) \

#define HasPropertyLOn(obj, propertyName) \
    HasPropertyOn(obj, scriptContext->GetOrAddPropertyIdTracked(propertyName, wcslen(propertyName)))

namespace Js
{
    EngineExtensionObjectBase* EngineInterfaceObject::GetEngineExtension(EngineInterfaceExtensionKind extensionKind) const
    {
        AnalysisAssert(extensionKind >= 0 && extensionKind <= MaxEngineInterfaceExtensionKind);
        if (extensionKind <= MaxEngineInterfaceExtensionKind)
        {
            Assert(engineExtensions[extensionKind] == nullptr || engineExtensions[extensionKind]->GetExtensionKind() == extensionKind);
            return engineExtensions[extensionKind];
        }
        return nullptr;
    }

    void EngineInterfaceObject::SetEngineExtension(EngineInterfaceExtensionKind extensionKind, EngineExtensionObjectBase* extensionObject)
    {
        AnalysisAssert(extensionKind >= 0 && extensionKind <= MaxEngineInterfaceExtensionKind);
        if (extensionKind <= MaxEngineInterfaceExtensionKind)
        {
            Assert(engineExtensions[extensionKind] == nullptr);
            engineExtensions[extensionKind] = extensionObject;

            // Init the extensionObject if this was already initialized
            if (this->IsInitialized())
            {
                extensionObject->Initialize();
            }
        }
    }

// initialize EngineInterfaceObject::EntryInfo
#define EngineInterfaceBuiltIn2(propId, nativeMethod) NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::nativeMethod(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_##nativeMethod));
#define BuiltInRaiseException(exceptionType, exceptionID) NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::BuiltIn_raise##exceptionID(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID));
#include "EngineInterfaceObjectBuiltIns.h"

    EngineInterfaceObject * EngineInterfaceObject::New(Recycler * recycler, DynamicType * type)
    {
        EngineInterfaceObject* newObject = NewObject<EngineInterfaceObject>(recycler, type);
        for (uint i = 0; i <= MaxEngineInterfaceExtensionKind; i++)
        {
            newObject->engineExtensions[i] = nullptr;
        }
        return newObject;
    }

    void EngineInterfaceObject::Initialize()
    {
        Recycler* recycler = this->GetRecycler();
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        // CommonNativeInterfaces is used as a prototype for the other native interface objects
        // to share the common APIs without requiring everyone to access EngineInterfaceObject.Common.
        this->commonNativeInterfaces = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, library->GetObjectPrototype(), nullptr,
            DeferredTypeHandler<InitializeCommonNativeInterfaces>::GetDefaultInstance()));
        library->AddMember(this, Js::PropertyIds::Common, this->commonNativeInterfaces);

        for (uint i = 0; i <= MaxEngineInterfaceExtensionKind; i++)
        {
            if (engineExtensions[i] != nullptr)
            {
                engineExtensions[i]->Initialize();
            }
        }
    }

#if ENABLE_TTD
    void EngineInterfaceObject::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        extractor->MarkVisitVar(this->commonNativeInterfaces);
    }

    void EngineInterfaceObject::ProcessCorePaths()
    {
        this->GetScriptContext()->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, this->commonNativeInterfaces, _u("!commonNativeInterfaces"));
    }

    TTD::NSSnapObjects::SnapObjectType EngineInterfaceObject::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapWellKnownObject;
    }

    void EngineInterfaceObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<void*, TTD::NSSnapObjects::SnapObjectType::SnapWellKnownObject>(objData, nullptr);
    }
#endif

    bool EngineInterfaceObject::InitializeCommonNativeInterfaces(DynamicObject* commonNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        // start with 1 for CallInstanceFunction
        int initSlotCapacity = 1;

#define GlobalMathBuiltIn(mathMethod) initSlotCapacity++;
#define GlobalBuiltIn(global, method) initSlotCapacity++;
#define GlobalBuiltInConstructor(global) initSlotCapacity++;
#define BuiltInRaiseException(exceptionType, exceptionID) initSlotCapacity++;
#define EngineInterfaceBuiltIn2(propId, nativeMethod) initSlotCapacity++;
#include "EngineInterfaceObjectBuiltIns.h"

        typeHandler->Convert(commonNativeInterfaces, mode, initSlotCapacity);

        JavascriptLibrary* library = commonNativeInterfaces->GetScriptContext()->GetLibrary();

#define GlobalMathBuiltIn(mathMethod) library->AddFunctionToLibraryObject(commonNativeInterfaces, PropertyIds::builtInMath##mathMethod, &Math::EntryInfo::mathMethod, 1);
#define GlobalBuiltIn(global, method) library->AddFunctionToLibraryObject(commonNativeInterfaces, PropertyIds::builtIn##global##Entry##method, &global::EntryInfo::method, 1);
#define GlobalBuiltInConstructor(global) SetPropertyOn(commonNativeInterfaces, PropertyIds::##global##, library->Get##global##Constructor());
#define BuiltInRaiseException(exceptionType, exceptionID) library->AddFunctionToLibraryObject(commonNativeInterfaces, PropertyIds::raise##exceptionID, &EngineInterfaceObject::EntryInfo::BuiltIn_raise##exceptionID, 1);
#define EngineInterfaceBuiltIn2(propId, nativeMethod) library->AddFunctionToLibraryObject(commonNativeInterfaces, PropertyIds::propId, &EngineInterfaceObject::EntryInfo::nativeMethod, 1);
#include "EngineInterfaceObjectBuiltIns.h"

        library->AddFunctionToLibraryObject(commonNativeInterfaces, PropertyIds::builtInCallInstanceFunction, &EngineInterfaceObject::EntryInfo::CallInstanceFunction, 1);

        commonNativeInterfaces->SetHasNoEnumerableProperties(true);

        return true;
    }

    Var EngineInterfaceObject::Entry_GetErrorMessage(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 2)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        int hr = Js::JavascriptConversion::ToInt32(args[1], scriptContext);
        int resourceId;

        switch(hr)
        {
        case ASYNCERR_NoErrorInErrorState:
            resourceId = 5200;
            break;
        case ASYNCERR_InvalidStatusArg:
            resourceId = 5201;
            break;
        case ASYNCERR_InvalidSenderArg:
            resourceId = 5202;
            break;
        default:
            AssertMsg(false, "Invalid HRESULT passed to GetErrorMessage. This shouldn't come from Promise.js - who called us?");
            return scriptContext->GetLibrary()->GetUndefined();
        }

        const int strLength = 1024;
        OLECHAR errorString[strLength];

        if(FGetResourceString(resourceId, errorString, strLength))
        {
            return Js::JavascriptString::NewCopySz(errorString, scriptContext);
        }

        AssertMsg(false, "FGetResourceString returned false.");
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var EngineInterfaceObject::Entry_LogDebugMessage(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#if DBG
        if (callInfo.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString* message = VarTo<JavascriptString>(args.Values[1]);

        Output::Print(message->GetString());
        Output::Flush();
#endif

        return scriptContext->GetLibrary()->GetUndefined();
    }

    /* static */
    ScriptFunction *EngineInterfaceObject::CreateLibraryCodeScriptFunction(ScriptFunction *scriptFunction, JavascriptString *displayName, bool isConstructor, bool isJsBuiltIn, bool isPublic)
    {
        if (scriptFunction->GetFunctionProxy()->IsPublicLibraryCode())
        {
            // this can happen when we re-initialize Intl for a different mode -- for instance, if we have the following JS:
            // print((1).toLocaleString())
            // print(new Intl.NumberFormat().format(1))
            // Intl will first get initialized for Number, and then will get re-initialized for all of Intl. This will cause
            // Number.prototype.toLocaleString to be registered twice, which breaks some of our assertions below.
            return scriptFunction;
        }

        ScriptContext *scriptContext = scriptFunction->GetScriptContext();

        if (!isConstructor)
        {
            // set the ErrorOnNew attribute to disallow construction. JsBuiltIn/Intl functions are usually regular ScriptFunctions
            // (not lambdas or class methods), so they are usually constructable by default.
            FunctionInfo *info = scriptFunction->GetFunctionInfo();
            AssertMsg((info->GetAttributes() & FunctionInfo::Attributes::ErrorOnNew) == 0, "Why are we trying to disable construction of a function that already isn't constructable?");
            info->SetAttributes((FunctionInfo::Attributes) (info->GetAttributes() | FunctionInfo::Attributes::ErrorOnNew));

            // Assert that the type handler is deferred to ensure that we aren't overwriting previous modifications.
            // Script functions start with deferred type handlers, which undefer as soon as any property is modified.
            // Since the function that is passed in should be an inline function expression, its type should still be deferred by the time it gets here.
            AssertOrFailFast(scriptFunction->GetDynamicType()->GetTypeHandler()->IsDeferredTypeHandler());

            // give the function a type handler with name and length but without prototype
            DynamicTypeHandler::SetInstanceTypeHandler(scriptFunction, scriptContext->GetLibrary()->GetDeferredFunctionWithLengthTypeHandler());
        }
        else
        {
            AssertMsg((scriptFunction->GetFunctionInfo()->GetAttributes() & FunctionInfo::Attributes::ErrorOnNew) == 0, "Why is the function not constructable by default?");
        }

        if (isPublic)
        {
            // Use GetSz rather than GetString because we use wcsrchr below, which expects a null-terminated string
            // Callers can pass in a string like "get compare" or "Intl.Collator.prototype.resolvedOptions" -- only for the
            // latter do we extract a shortName.
            const char16 *methodNameBuf = displayName->GetSz();
            charcount_t methodNameLength = displayName->GetLength();
            const char16 *shortName = wcsrchr(methodNameBuf, _u('.'));
            charcount_t shortNameOffset = 0;
            if (shortName != nullptr)
            {
                shortName++;
                shortNameOffset = static_cast<charcount_t>(shortName - methodNameBuf);
            }

            scriptFunction->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(methodNameBuf, methodNameLength, shortNameOffset);

            // handle the name property AFTER handling isConstructor, because this can initialize the function's deferred type
            Var existingName = nullptr;
            if (JavascriptOperators::GetOwnProperty(scriptFunction, PropertyIds::name, &existingName, scriptContext, nullptr))
            {
                JavascriptString *existingNameString = VarTo<JavascriptString>(existingName);
                if (existingNameString->GetLength() == 0)
                {
                    // Only overwrite the name of the function object if it was anonymous coming in
                    // If the input function was named, it is likely intentional
                    existingName = nullptr;
                }
            }

            if (existingName == nullptr || JavascriptOperators::IsUndefined(existingName))
            {
                // It is convenient to set the name here rather than in script, since it is often duplicated.
                JavascriptString *funcName = displayName;
                if (shortName)
                {
                    funcName = JavascriptString::NewCopyBuffer(shortName, methodNameLength - shortNameOffset, scriptContext);
                }

                scriptFunction->SetPropertyWithAttributes(PropertyIds::name, funcName, PropertyConfigurable, nullptr);
            }

            scriptFunction->GetFunctionProxy()->SetIsPublicLibraryCode();
        }

        if (isJsBuiltIn)
        {
            scriptFunction->GetFunctionProxy()->SetIsJsBuiltInCode();

            // This makes it so that the given scriptFunction can't reference/close over any outside variables,
            // which is desirable for JsBuiltIns (though currently not for Intl)
            scriptFunction->SetEnvironment(const_cast<FrameDisplay *>(&StrictNullFrameDisplay));

            // TODO(jahorto): investigate force-inlining Intl code
            scriptFunction->GetFunctionProxy()->EnsureDeserialized();
            AssertOrFailFast(scriptFunction->HasFunctionBody());
            scriptFunction->GetFunctionBody()->SetJsBuiltInForceInline();
        }

        return scriptFunction;
    }

    Var EngineInterfaceObject::Entry_TagPublicLibraryCode(RecyclableObject *function, CallInfo callInfo, ...)
    {
#pragma warning(push)
#pragma warning(disable: 4189) // 'scriptContext': local variable is initialized but not referenced
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
#pragma warning(pop)

        AssertOrFailFast((args.Info.Count == 3 || args.Info.Count == 4) && VarIs<ScriptFunction>(args.Values[1]) && VarIs<JavascriptString>(args.Values[2]));

        ScriptFunction *func = UnsafeVarTo<ScriptFunction>(args[1]);
        JavascriptString *methodName = UnsafeVarTo<JavascriptString>(args[2]);

        bool isConstructor = true;
        if (args.Info.Count == 4)
        {
            AssertOrFailFast(VarIs<JavascriptBoolean>(args.Values[3]));
            isConstructor = UnsafeVarTo<JavascriptBoolean>(args.Values[3])->GetValue();
        }

        // isConstructor = true is the default (when no 3rd arg is provided)
        return CreateLibraryCodeScriptFunction(func, methodName, isConstructor, false /* isJsBuiltIn */, true /* isPublic */);
    }

    /*
    * First parameter is the object onto which prototype should be set; second is the value
    */
    Var EngineInterfaceObject::Entry_SetPrototype(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 3 || !DynamicObject::IsBaseDynamicObject(args.Values[1]) || !VarIs<RecyclableObject>(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = VarTo<DynamicObject>(args.Values[1]);
        RecyclableObject* value = VarTo<RecyclableObject>(args.Values[2]);

        obj->SetPrototype(value);

        return obj;
    }

    /*
    * First parameter is the array object.
    */
    Var EngineInterfaceObject::Entry_GetArrayLength(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 2)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        if (DynamicObject::IsAnyArray(args.Values[1]))
        {
            JavascriptArray* arr = JavascriptArray::FromAnyArray(args.Values[1]);
            return TaggedInt::ToVarUnchecked(arr->GetLength());
        }
        else
        {
            AssertMsg(false, "Object passed in with unknown type ID, verify Intl.js is correct.");
            return TaggedInt::ToVarUnchecked(0);
        }
    }

    /*
    * First parameter is the string on which to match.
    * Second parameter is the regex object
    */
    Var EngineInterfaceObject::Entry_RegexMatch(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 2 || !VarIs<JavascriptString>(args.Values[1]) || !VarIs<JavascriptRegExp>(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *stringToUse = VarTo<JavascriptString>(args.Values[1]);
        JavascriptRegExp *regexpToUse = VarTo<JavascriptRegExp>(args.Values[2]);

        return RegexHelper::RegexMatchNoHistory(scriptContext, regexpToUse, stringToUse, false);
    }

    /*
    * First parameter is the function, then its the this arg; so at least 2 are needed.
    */
    Var EngineInterfaceObject::Entry_CallInstanceFunction(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 3 || !JavascriptConversion::IsCallable(args.Values[1]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        RecyclableObject *func = VarTo<RecyclableObject>(args.Values[1]);

        AssertOrFailFastMsg(func != scriptContext->GetLibrary()->GetUndefined(), "Trying to callInstanceFunction(undefined, ...)");

        //Shift the arguments by 2 so argument at index 2 becomes the 'this' argument at index 0
        for (uint i = 0; i < args.Info.Count - 2; ++i)
        {
            args.Values[i] = args.Values[i + 2];
        }

        args.Info.Count -= 2;

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return JavascriptFunction::CallFunction<true>(func, func->GetEntryPoint(), args);
        }
        END_SAFE_REENTRANT_CALL
    }

#define BuiltInRaiseException(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
    }

#define BuiltInRaiseException1(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        if(args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, VarTo<JavascriptString>(args.Values[1])->GetSz()); \
    }

#define BuiltInRaiseException2(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        if(args.Info.Count < 3 || !VarIs<JavascriptString>(args.Values[1]) || !VarIs<JavascriptString>(args.Values[2])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, VarTo<JavascriptString>(args.Values[1])->GetSz(), VarTo<JavascriptString>(args.Values[2])->GetSz()); \
    }

#define BuiltInRaiseException3(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID##_3(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        if(args.Info.Count < 4 || !VarIs<JavascriptString>(args.Values[1]) || !VarIs<JavascriptString>(args.Values[2]) || !VarIs<JavascriptString>(args.Values[3])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, VarTo<JavascriptString>(args.Values[1])->GetSz(), VarTo<JavascriptString>(args.Values[2])->GetSz(), VarTo<JavascriptString>(args.Values[3])->GetSz()); \
    }

#include "EngineInterfaceObjectBuiltIns.h"

}
#endif // ENABLE_INTL_OBJECT || ENABLE_JS_BUILTINS || ENABLE_PROJECTION
