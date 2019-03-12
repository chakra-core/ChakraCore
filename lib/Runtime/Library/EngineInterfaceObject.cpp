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
    (GetPropertyFrom(obj, Js::PropertyIds::builtInPropID) && Type::Is(propertyValue)) \

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

    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::GetErrorMessage(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_GetErrorMessage));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::LogDebugMessage(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_LogDebugMessage));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::TagPublicLibraryCode(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_TagPublicLibraryCode));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::SetPrototype(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_SetPrototype));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::GetArrayLength(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_GetArrayLength));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::RegexMatch(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_RegexMatch));
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::CallInstanceFunction(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_CallInstanceFunction));

#ifndef GlobalBuiltIn
#define GlobalBuiltIn(global, method) \
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::BuiltIn_##global##_##method##(FORCE_NO_WRITE_BARRIER_TAG(global##::##method##)); \

#define GlobalBuiltInConstructor(global)

#define BuiltInRaiseException(exceptionType, exceptionID) \
    NoProfileFunctionInfo EngineInterfaceObject::EntryInfo::BuiltIn_raise##exceptionID(FORCE_NO_WRITE_BARRIER_TAG(EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID)); \

#define BuiltInRaiseException1(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID)
#define BuiltInRaiseException2(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID)
#define BuiltInRaiseException3(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID##_3)

#include "EngineInterfaceObjectBuiltIns.h"

#undef BuiltInRaiseException
#undef BuiltInRaiseException1
#undef BuiltInRaiseException2
#undef BuiltInRaiseException3
#undef GlobalBuiltInConstructor
#undef GlobalBuiltIn
#endif

    EngineInterfaceObject * EngineInterfaceObject::New(Recycler * recycler, DynamicType * type)
    {
        EngineInterfaceObject* newObject = NewObject<EngineInterfaceObject>(recycler, type);
        for (uint i = 0; i <= MaxEngineInterfaceExtensionKind; i++)
        {
            newObject->engineExtensions[i] = nullptr;
        }
        return newObject;
    }

    bool EngineInterfaceObject::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_EngineInterfaceObject;
    }

    EngineInterfaceObject* EngineInterfaceObject::FromVar(Var aValue)
    {
        AssertOrFailFastMsg(Is(aValue), "aValue is actually an EngineInterfaceObject");

        return static_cast<EngineInterfaceObject *>(aValue);
    }

    EngineInterfaceObject* EngineInterfaceObject::UnsafeFromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "aValue is actually an EngineInterfaceObject");

        return static_cast<EngineInterfaceObject *>(aValue);
    }
    void EngineInterfaceObject::Initialize()
    {
        Recycler* recycler = this->GetRecycler();
        ScriptContext* scriptContext = this->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        // CommonNativeInterfaces is used as a prototype for the other native interface objects
        // to share the common APIs without requiring everyone to access EngineInterfaceObject.Common.
        this->commonNativeInterfaces = DynamicObject::New(recycler,
            DynamicType::New(scriptContext, TypeIds_Object, library->GetNull(), nullptr,
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
        typeHandler->Convert(commonNativeInterfaces, mode, 38);

        JavascriptLibrary* library = commonNativeInterfaces->GetScriptContext()->GetLibrary();

#ifndef GlobalBuiltIn
#define GlobalBuiltIn(global, method) \
    library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtIn##global##method, &EngineInterfaceObject::EntryInfo::BuiltIn_##global##_##method##, 1); \

#define GlobalBuiltInConstructor(global) SetPropertyOn(commonNativeInterfaces, Js::PropertyIds::##global##, library->Get##global##Constructor());

#define BuiltInRaiseException(exceptionType, exceptionID) \
    library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::raise##exceptionID, &EngineInterfaceObject::EntryInfo::BuiltIn_raise##exceptionID, 1); \

#define BuiltInRaiseException1(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID)
#define BuiltInRaiseException2(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID)
#define BuiltInRaiseException3(exceptionType, exceptionID) BuiltInRaiseException(exceptionType, exceptionID##_3)

#include "EngineInterfaceObjectBuiltIns.h"

#undef BuiltInRaiseException
#undef BuiltInRaiseException1
#undef BuiltInRaiseException2
#undef BuiltInRaiseException3
#undef GlobalBuiltIn
#undef GlobalBuiltInConstructor
#endif
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInJavascriptObjectCreate, &JavascriptObject::EntryInfo::Create, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInJavascriptObjectPreventExtensions, &JavascriptObject::EntryInfo::PreventExtensions, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInJavascriptObjectGetOwnPropertyDescriptor, &JavascriptObject::EntryInfo::GetOwnPropertyDescriptor, 1);

        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInGlobalObjectEval, &GlobalObject::EntryInfo::Eval, 2);

        library->AddMember(commonNativeInterfaces, PropertyIds::Object_prototype, library->GetObjectPrototype());

        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::getErrorMessage, &EngineInterfaceObject::EntryInfo::GetErrorMessage, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::logDebugMessage, &EngineInterfaceObject::EntryInfo::LogDebugMessage, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::tagPublicLibraryCode, &EngineInterfaceObject::EntryInfo::TagPublicLibraryCode, 1);

        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInSetPrototype, &EngineInterfaceObject::EntryInfo::SetPrototype, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInGetArrayLength, &EngineInterfaceObject::EntryInfo::GetArrayLength, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInRegexMatch, &EngineInterfaceObject::EntryInfo::RegexMatch, 1);
        library->AddFunctionToLibraryObject(commonNativeInterfaces, Js::PropertyIds::builtInCallInstanceFunction, &EngineInterfaceObject::EntryInfo::CallInstanceFunction, 1);

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
        if (callInfo.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString* message = JavascriptString::FromVar(args.Values[1]);

        Output::Print(message->GetString());
        Output::Flush();
#endif

        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var EngineInterfaceObject::Entry_TagPublicLibraryCode(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count >= 2 && JavascriptFunction::Is(args.Values[1]))
        {
            JavascriptFunction* func = JavascriptFunction::FromVar(args.Values[1]);
            func->GetFunctionProxy()->SetIsPublicLibraryCode();

            if (callInfo.Count >= 3 && JavascriptString::Is(args.Values[2]))
            {
                JavascriptString* customFunctionName = JavascriptString::FromVar(args.Values[2]);
                // tagPublicFunction("Intl.Collator", Collator); in Intl.js calls TagPublicLibraryCode the expected name is Collator so we need to calculate the offset
                const char16 * shortName = wcsrchr(customFunctionName->GetString(), _u('.'));
                uint shortNameOffset = 0;
                if (shortName != nullptr)
                {
                    // JavascriptString length is bounded by uint max
                    shortName++;
                    shortNameOffset = static_cast<uint>(shortName - customFunctionName->GetString());
                }
                func->GetFunctionProxy()->EnsureDeserialized()->SetDisplayName(customFunctionName->GetString(), customFunctionName->GetLength(), shortNameOffset);
            }

            return func;
        }

        return scriptContext->GetLibrary()->GetUndefined();
    }

    /*
    * First parameter is the object onto which prototype should be set; second is the value
    */
    Var EngineInterfaceObject::Entry_SetPrototype(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 3 || !DynamicObject::Is(args.Values[1]) || !RecyclableObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = DynamicObject::FromVar(args.Values[1]);
        RecyclableObject* value = RecyclableObject::FromVar(args.Values[2]);

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

        if (callInfo.Count < 2 || !JavascriptString::Is(args.Values[1]) || !JavascriptRegExp::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *stringToUse = JavascriptString::FromVar(args.Values[1]);
        JavascriptRegExp *regexpToUse = JavascriptRegExp::FromVar(args.Values[2]);

        return RegexHelper::RegexMatchNoHistory(scriptContext, regexpToUse, stringToUse, false);
    }

    /*
    * First parameter is the function, then its the this arg; so at least 2 are needed.
    */
    Var EngineInterfaceObject::Entry_CallInstanceFunction(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        Assert(args.Info.Count <= 5);
        if (callInfo.Count < 3 || args.Info.Count > 5 || !JavascriptConversion::IsCallable(args.Values[1]) || !RecyclableObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        RecyclableObject *func = RecyclableObject::FromVar(args.Values[1]);

        AssertOrFailFastMsg(func != scriptContext->GetLibrary()->GetUndefined(), "Trying to callInstanceFunction(undefined, ...)");

        //Shift the arguments by 2 so argument at index 2 becomes the 'this' argument at index 0
        Var newVars[3];
        Js::Arguments newArgs(callInfo, newVars);

        for (uint i = 0; i<args.Info.Count - 2; ++i)
        {
            newArgs.Values[i] = args.Values[i + 2];
        }

        newArgs.Info.Count = args.Info.Count - 2;

        BEGIN_SAFE_REENTRANT_CALL(scriptContext->GetThreadContext())
        {
            return JavascriptFunction::CallFunction<true>(func, func->GetEntryPoint(), newArgs);
        }
        END_SAFE_REENTRANT_CALL
    }

#ifndef GlobalBuiltIn
#define GlobalBuiltIn(global, method)
#define GlobalBuiltInConstructor(global)

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
        if(args.Info.Count < 2 || !JavascriptString::Is(args.Values[1])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, JavascriptString::FromVar(args.Values[1])->GetSz()); \
    }

#define BuiltInRaiseException2(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        if(args.Info.Count < 3 || !JavascriptString::Is(args.Values[1]) || !JavascriptString::Is(args.Values[2])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, JavascriptString::FromVar(args.Values[1])->GetSz(), JavascriptString::FromVar(args.Values[2])->GetSz()); \
    }

#define BuiltInRaiseException3(exceptionType, exceptionID) \
    Var EngineInterfaceObject::Entry_BuiltIn_raise##exceptionID##_3(RecyclableObject *function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        \
        if(args.Info.Count < 4 || !JavascriptString::Is(args.Values[1]) || !JavascriptString::Is(args.Values[2]) || !JavascriptString::Is(args.Values[3])) \
        { \
            Assert(false); \
            JavascriptError::Throw##exceptionType(scriptContext, JSERR_##exceptionID); \
        } \
        JavascriptError::Throw##exceptionType##Var(scriptContext, JSERR_##exceptionID, JavascriptString::FromVar(args.Values[1])->GetSz(), JavascriptString::FromVar(args.Values[2])->GetSz(), JavascriptString::FromVar(args.Values[3])->GetSz()); \
    }

#include "EngineInterfaceObjectBuiltIns.h"

#undef BuiltInRaiseException
#undef BuiltInRaiseException1
#undef BuiltInRaiseException2
#undef BuiltInRaiseException3
#undef GlobalBuiltIn
#undef GlobalBuiltInConstructor
#endif

}
#endif // ENABLE_INTL_OBJECT || ENABLE_JS_BUILTINS || ENABLE_PROJECTION
