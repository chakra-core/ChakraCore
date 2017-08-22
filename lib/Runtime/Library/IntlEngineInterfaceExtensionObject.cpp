//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"
#include "EngineInterfaceObject.h"
#include "IntlEngineInterfaceExtensionObject.h"
#include "Types/DeferredTypeHandler.h"
#include "Base/WindowsGlobalizationAdapter.h"

#ifdef ENABLE_INTL_OBJECT
#include "ByteCode/ByteCodeSerializer.h"
#include "errstr.h"
#include "ByteCode/ByteCodeDumper.h"
#ifdef INTL_WINGLOB
using namespace Windows::Globalization;
#endif
#ifdef INTL_ICU
#include <CommonPal.h>
#endif

#pragma warning(push)
#pragma warning(disable:4309) // truncation of constant value
#pragma warning(disable:4838) // conversion from 'int' to 'const char' requires a narrowing conversion

#if DISABLE_JIT
#if _M_AMD64
#include "InJavascript/Intl.js.nojit.bc.64b.h"
#else
#include "InJavascript/Intl.js.nojit.bc.32b.h"
#endif
#else
#if _M_AMD64
#include "InJavascript/Intl.js.bc.64b.h"
#else
#include "InJavascript/Intl.js.bc.32b.h"
#endif
#endif

#pragma warning(pop)

#define TO_JSBOOL(sc, b) ((b) ? (sc)->GetLibrary()->GetTrue() : (sc)->GetLibrary()->GetFalse())

#define IfCOMFailIgnoreSilentlyAndReturn(op) \
    if(FAILED(hr=(op))) \
    { \
        return; \
    } \

#define IfFailAssertMsgAndThrowHr(op, msg) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, msg); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#define HandleOOMSOEHR(hr) \
    if (hr == E_OUTOFMEMORY) \
    { \
    JavascriptError::ThrowOutOfMemoryError(scriptContext); \
    } \
    else if(hr == VBSERR_OutOfStack) \
    { \
    JavascriptError::ThrowStackOverflowError(scriptContext); \
    } \

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

#define SetPropertyLOn(obj, literalProperty, value) \
    obj->SetProperty(Js::JavascriptString::NewCopySz(literalProperty, scriptContext), value, Js::PropertyOperationFlags::PropertyOperation_None, nullptr) \

#define SetStringPropertyLOn(obj, literalProperty, propValue) \
    SetPropertyLOn(obj, literalProperty, Js::JavascriptString::NewCopySz(propValue, scriptContext)) \

#define SetPropertyBuiltInOn(obj, builtInPropID, value) \
    SetPropertyOn(obj, Js::PropertyIds::builtInPropID, value) \

#define SetStringPropertyBuiltInOn(obj, builtInPropID, propValue) \
    SetPropertyBuiltInOn(obj, builtInPropID, Js::JavascriptString::NewCopySz(propValue, scriptContext))

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

#ifdef INTL_WINGLOB

#define SetHSTRINGPropertyOn(obj, propID, hstringValue) \
    SetStringPropertyOn(obj, propID, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#define SetHSTRINGPropertyLOn(obj, literalProperty, hstringValue) \
    SetStringPropertyLOn(obj, literalProperty, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#define SetHSTRINGPropertyBuiltInOn(obj, builtInPropID, hstringValue) \
    SetStringPropertyBuiltInOn(obj, builtInPropID, wgl->WindowsGetStringRawBuffer(hstringValue, &length)) \

#endif

namespace Js
{
#ifdef ENABLE_INTL_OBJECT
#ifdef INTL_WINGLOB
    class AutoHSTRING
    {
        PREVENT_COPY(AutoHSTRING)

    private:
        HSTRING value;
    public:
        HSTRING *operator&() { Assert(value == nullptr); return &value; }
        HSTRING operator*() const { Assert(value != nullptr); return value; }

        AutoHSTRING()
            : value(nullptr)
        { }

        ~AutoHSTRING()
        {
            Clear();
        }

        void Clear()
        {
            if (value != nullptr)
            {
                WindowsDeleteString(value);
                value = nullptr;
            }
        }
    };

    class AutoCOMJSObject : public FinalizableObject
    {
        IInspectable *instance;

    public:
        DEFINE_VTABLE_CTOR_NOBASE(AutoCOMJSObject);

        AutoCOMJSObject(IInspectable *object)
            : instance(object)
        { }

        static AutoCOMJSObject * New(Recycler * recycler, IInspectable *object)
        {
            return RecyclerNewFinalized(recycler, AutoCOMJSObject, object);
        }

        void Finalize(bool isShutdown) override
        {

        }

        void Dispose(bool isShutdown) override
        {
            if (!isShutdown)
            {
                instance->Release();
            }
        }
        void Mark(Recycler * recycler) override
        {

        }

        IInspectable *GetInstance()
        {
            return instance;
        }
    };
#endif

    IntlEngineInterfaceExtensionObject::IntlEngineInterfaceExtensionObject(Js::ScriptContext* scriptContext) :
        EngineExtensionObjectBase(EngineInterfaceExtensionKind_Intl, scriptContext),
        dateToLocaleString(nullptr),
        dateToLocaleTimeString(nullptr),
        dateToLocaleDateString(nullptr),
        numberToLocaleString(nullptr),
        stringLocaleCompare(nullptr),
        intlNativeInterfaces(nullptr),
        intlByteCode(nullptr),
        wasInitialized(false)
    {
    }

    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_RaiseAssert(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_RaiseAssert));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_IsWellFormedLanguageTag(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_IsWellFormedLanguageTag));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_NormalizeLanguageTag(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_NormalizeLanguageTag));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ResolveLocaleLookup(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_ResolveLocaleLookup));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ResolveLocaleBestFit(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_ResolveLocaleBestFit));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetDefaultLocale(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_GetDefaultLocale));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetExtensions(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_GetExtensions));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CompareString(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_CompareString));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CurrencyDigits(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_CurrencyDigits));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_FormatNumber(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_FormatNumber));

    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CacheNumberFormat(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_CacheNumberFormat));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CreateDateTimeFormat(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_CreateDateTimeFormat));

    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_FormatDateTime(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_FormatDateTime));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ValidateAndCanonicalizeTimeZone(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_ValidateAndCanonicalizeTimeZone));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetDefaultTimeZone(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_GetDefaultTimeZone));

    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_RegisterBuiltInFunction(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_RegisterBuiltInFunction));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetHiddenObject(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_GetHiddenObject));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_SetHiddenObject(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_SetHiddenObject));

    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_GetArrayLength(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_GetArrayLength));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_SetPrototype(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_SetPrototype));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_RegexMatch(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_RegexMatch));
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_CallInstanceFunction(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_CallInstanceFunction));

#ifdef INTL_WINGLOB
    WindowsGlobalizationAdapter* IntlEngineInterfaceExtensionObject::GetWindowsGlobalizationAdapter(_In_ ScriptContext * scriptContext)
    {
        return scriptContext->GetThreadContext()->GetWindowsGlobalizationAdapter();
    }
#endif

    void IntlEngineInterfaceExtensionObject::Initialize()
    {
        if (wasInitialized)
        {
            return;
        }
        JavascriptLibrary* library = scriptContext->GetLibrary();
        DynamicObject* commonObject = library->GetEngineInterfaceObject()->GetCommonNativeInterfaces();
        if (scriptContext->IsIntlEnabled())
        {
            Assert(library->GetEngineInterfaceObject() != nullptr);
            this->intlNativeInterfaces = DynamicObject::New(library->GetRecycler(),
                DynamicType::New(scriptContext, TypeIds_Object, commonObject, nullptr,
                    DeferredTypeHandler<InitializeIntlNativeInterfaces>::GetDefaultInstance()));
            library->AddMember(library->GetEngineInterfaceObject(), Js::PropertyIds::Intl, this->intlNativeInterfaces);
        }
        // TODO: Move these to IntlNativeInterfaces?
        library->AddFunctionToLibraryObject(commonObject, Js::PropertyIds::builtInSetPrototype, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_SetPrototype, 1);
        library->AddFunctionToLibraryObject(commonObject, Js::PropertyIds::builtInGetArrayLength, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_GetArrayLength, 1);
        library->AddFunctionToLibraryObject(commonObject, Js::PropertyIds::builtInRegexMatch, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_RegexMatch, 1);
        library->AddFunctionToLibraryObject(commonObject, Js::PropertyIds::builtInCallInstanceFunction, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_BuiltIn_CallInstanceFunction, 1);
        wasInitialized = true;
    }

#if DBG
    void IntlEngineInterfaceExtensionObject::DumpByteCode()
    {
        Output::Print(_u("Dumping Intl Byte Code:"));
        this->EnsureIntlByteCode(scriptContext);
        Js::ByteCodeDumper::DumpRecursively(intlByteCode);
    }
#endif

    bool IntlEngineInterfaceExtensionObject::InitializeIntlNativeInterfaces(DynamicObject* intlNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(intlNativeInterfaces, mode, 16);

        ScriptContext* scriptContext = intlNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::raiseAssert, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_RaiseAssert, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::isWellFormedLanguageTag, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_IsWellFormedLanguageTag, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::normalizeLanguageTag, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_NormalizeLanguageTag, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::compareString, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CompareString, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::resolveLocaleLookup, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ResolveLocaleLookup, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::resolveLocaleBestFit, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ResolveLocaleBestFit, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::getDefaultLocale, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetDefaultLocale, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::getExtensions, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetExtensions, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::formatNumber, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_FormatNumber, 1);

        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::cacheNumberFormat, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CacheNumberFormat, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::createDateTimeFormat, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CreateDateTimeFormat, 1);

        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::currencyDigits, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_CurrencyDigits, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::formatDateTime, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_FormatDateTime, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::validateAndCanonicalizeTimeZone, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_ValidateAndCanonicalizeTimeZone, 2);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::getDefaultTimeZone, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetDefaultTimeZone, 1);

        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::registerBuiltInFunction, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_RegisterBuiltInFunction, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::getHiddenObject, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_GetHiddenObject, 1);
        library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::setHiddenObject, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_SetHiddenObject, 1);

        intlNativeInterfaces->SetHasNoEnumerableProperties(true);

        return true;
    }

    void IntlEngineInterfaceExtensionObject::deletePrototypePropertyHelper(ScriptContext* scriptContext, DynamicObject* intlObject, Js::PropertyId objectPropertyId, Js::PropertyId getterFunctionId)
    {
        DynamicObject *prototypeObject = nullptr;
        DynamicObject *functionObj = nullptr;
        Var propertyValue = nullptr;
        Var prototypeValue = nullptr;
        Var resolvedOptionsValue = nullptr;
        Var getter = nullptr;
        Var setter = nullptr;

        if (!JavascriptOperators::GetProperty(intlObject, objectPropertyId, &propertyValue, scriptContext) ||
            !JavascriptOperators::IsObject(propertyValue))
        {
            return;
        }

        if (!JavascriptOperators::GetProperty(DynamicObject::FromVar(propertyValue), Js::PropertyIds::prototype, &prototypeValue, scriptContext) ||
            !JavascriptOperators::IsObject(prototypeValue))
        {
            return;
        }

        prototypeObject = DynamicObject::FromVar(prototypeValue);

        if (!JavascriptOperators::GetProperty(prototypeObject, Js::PropertyIds::resolvedOptions, &resolvedOptionsValue, scriptContext) ||
            !JavascriptOperators::IsObject(resolvedOptionsValue))
        {
            return;
        }

        functionObj = DynamicObject::FromVar(resolvedOptionsValue);
        functionObj->SetConfigurable(Js::PropertyIds::prototype, true);
        functionObj->DeleteProperty(Js::PropertyIds::prototype, Js::PropertyOperationFlags::PropertyOperation_None);

        if (!JavascriptOperators::GetOwnAccessors(prototypeObject, getterFunctionId, &getter, &setter, scriptContext) ||
            !JavascriptOperators::IsObject(getter))
        {
            return;
        }

        functionObj = DynamicObject::FromVar(getter);
        functionObj->SetConfigurable(Js::PropertyIds::prototype, true);
        functionObj->DeleteProperty(Js::PropertyIds::prototype, Js::PropertyOperationFlags::PropertyOperation_None);
    }

    void IntlEngineInterfaceExtensionObject::cleanUpIntl(ScriptContext *scriptContext, DynamicObject* intlObject)
    {
        this->dateToLocaleString = nullptr;
        this->dateToLocaleTimeString = nullptr;
        this->dateToLocaleDateString = nullptr;
        this->numberToLocaleString = nullptr;
        this->stringLocaleCompare = nullptr;

        //Failed to setup Intl; Windows.Globalization.dll is most likely missing.
        if (Js::JavascriptOperators::HasProperty(intlObject, Js::PropertyIds::Collator))
        {
            intlObject->DeleteProperty(Js::PropertyIds::Collator, Js::PropertyOperationFlags::PropertyOperation_None);
        }
        if (Js::JavascriptOperators::HasProperty(intlObject, Js::PropertyIds::NumberFormat))
        {
            intlObject->DeleteProperty(Js::PropertyIds::NumberFormat, Js::PropertyOperationFlags::PropertyOperation_None);
        }
        if (Js::JavascriptOperators::HasProperty(intlObject, Js::PropertyIds::DateTimeFormat))
        {
            intlObject->DeleteProperty(Js::PropertyIds::DateTimeFormat, Js::PropertyOperationFlags::PropertyOperation_None);
        }
    }

    void IntlEngineInterfaceExtensionObject::EnsureIntlByteCode(_In_ ScriptContext * scriptContext)
    {
        if (this->intlByteCode == nullptr)
        {
            SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(Js::Constants::NoHostSourceContext, NULL);

            Assert(sourceContextInfo != nullptr);

            SRCINFO si;
            memset(&si, 0, sizeof(si));
            si.sourceContextInfo = sourceContextInfo;
            SRCINFO *hsi = scriptContext->AddHostSrcInfo(&si);
            uint32 flags = fscrIsLibraryCode | (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling() ? fscrAllowFunctionProxy : 0);

            HRESULT hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, (LPCUTF8)nullptr, hsi, (byte*)Library_Bytecode_Intl, nullptr, &this->intlByteCode);

            IfFailAssertMsgAndThrowHr(hr, "Failed to deserialize Intl.js bytecode - very probably the bytecode needs to be rebuilt.");
        }
    }

    void IntlEngineInterfaceExtensionObject::InjectIntlLibraryCode(_In_ ScriptContext * scriptContext, DynamicObject* intlObject, IntlInitializationType intlInitializationType)
    {
        JavascriptExceptionObject *pExceptionObject = nullptr;
#ifdef INTL_WINGLOB
        WindowsGlobalizationAdapter* globAdapter = GetWindowsGlobalizationAdapter(scriptContext);
#endif

        try {
            this->EnsureIntlByteCode(scriptContext);
            Assert(intlByteCode != nullptr);

#ifdef INTL_WINGLOB
            DelayLoadWindowsGlobalization *library = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
#endif

            JavascriptString* initType = nullptr;

#ifdef INTL_WINGLOB
            HRESULT hr;
            //Ensure we have initialized all appropriate COM objects for the adapter (we will be using them now)
            IfCOMFailIgnoreSilentlyAndReturn(globAdapter->EnsureCommonObjectsInitialized(library));
#endif
            switch (intlInitializationType)
            {
                default:
                    AssertMsg(false, "Not a valid intlInitializationType.");
                    // fall thru
                case IntlInitializationType::Intl:
#ifdef INTL_WINGLOB
                    IfCOMFailIgnoreSilentlyAndReturn(globAdapter->EnsureNumberFormatObjectsInitialized(library));
                    IfCOMFailIgnoreSilentlyAndReturn(globAdapter->EnsureDateTimeFormatObjectsInitialized(library));
#endif
                    initType = scriptContext->GetPropertyString(PropertyIds::Intl);
                    break;
                case IntlInitializationType::StringPrototype:
                    // No other windows globalization adapter needed. Common adapter should suffice
                    initType = scriptContext->GetPropertyString(PropertyIds::String);
                    break;
                case IntlInitializationType::DatePrototype:
#ifdef INTL_WINGLOB
                    IfCOMFailIgnoreSilentlyAndReturn(globAdapter->EnsureDateTimeFormatObjectsInitialized(library));
#endif
                    initType = scriptContext->GetPropertyString(PropertyIds::Date);
                    break;
                case IntlInitializationType::NumberPrototype:
#ifdef INTL_WINGLOB
                    IfCOMFailIgnoreSilentlyAndReturn(globAdapter->EnsureNumberFormatObjectsInitialized(library));
#endif
                    initType = scriptContext->GetPropertyString(PropertyIds::Number);
                    break;
            }

            Js::ScriptFunction *function = scriptContext->GetLibrary()->CreateScriptFunction(intlByteCode->GetNestedFunctionForExecution(0));

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

            Js::Var args[] = { scriptContext->GetLibrary()->GetUndefined(), scriptContext->GetLibrary()->GetEngineInterfaceObject(), initType };
            Js::CallInfo callInfo(Js::CallFlags_Value, _countof(args));

            // Clear disable implicit call bit as initialization code doesn't have any side effect
            Js::ImplicitCallFlags saveImplicitCallFlags = scriptContext->GetThreadContext()->GetImplicitCallFlags();
            scriptContext->GetThreadContext()->ClearDisableImplicitFlags();
            JavascriptFunction::CallRootFunctionInScript(function, Js::Arguments(callInfo, args));
            scriptContext->GetThreadContext()->SetImplicitCallFlags((Js::ImplicitCallFlags)(saveImplicitCallFlags));

            // Delete prototypes on functions if initialized Intl object
            if (intlInitializationType == IntlInitializationType::Intl)
            {
                deletePrototypePropertyHelper(scriptContext, intlObject, Js::PropertyIds::Collator, Js::PropertyIds::compare);
                deletePrototypePropertyHelper(scriptContext, intlObject, Js::PropertyIds::NumberFormat, Js::PropertyIds::format);
                deletePrototypePropertyHelper(scriptContext, intlObject, Js::PropertyIds::DateTimeFormat, Js::PropertyIds::format);
            }
        }
        catch (const JavascriptException& err)
        {
            pExceptionObject = err.GetAndClear();
        }

        if (pExceptionObject)
        {
            if (intlInitializationType == IntlInitializationType::Intl)
            {
                cleanUpIntl(scriptContext, intlObject);
            }

            if (pExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingOOMErrorObject() ||
                pExceptionObject == ThreadContext::GetContextForCurrentThread()->GetPendingSOErrorObject())
            {
                // Reset factory objects that are might not have fully initialized
#ifdef INTL_WINGLOB
                globAdapter->ResetCommonFactoryObjects();
#endif
                switch (intlInitializationType) {
                default:
                    AssertMsg(false, "Not a valid intlInitializationType.");
                    // fall thru
                case IntlInitializationType::Intl:
#ifdef INTL_WINGLOB
                    globAdapter->ResetNumberFormatFactoryObjects();
                    globAdapter->ResetDateTimeFormatFactoryObjects();
#endif
                    scriptContext->GetLibrary()->ResetIntlObject();
                    break;
                case IntlInitializationType::StringPrototype:
                    // No other windows globalization adapter is created. Resetting common adapter should suffice
                    break;
                case IntlInitializationType::DatePrototype:
#ifdef INTL_WINGLOB
                    globAdapter->ResetDateTimeFormatFactoryObjects();
#endif
                    break;
                case IntlInitializationType::NumberPrototype:
#ifdef INTL_WINGLOB
                    globAdapter->ResetNumberFormatFactoryObjects();
#endif
                    break;
                }

                JavascriptExceptionOperators::DoThrowCheckClone(pExceptionObject, scriptContext);
            }

#if DEBUG
            JavascriptExceptionOperators::DoThrowCheckClone(pExceptionObject, scriptContext);
#else
            JavascriptError::ThrowTypeError(scriptContext, JSERR_IntlNotAvailable);
#endif
        }
    }

    // First parameter is boolean.
    Var IntlEngineInterfaceExtensionObject::EntryIntl_RaiseAssert(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptError::Is(args.Values[1]))
        {
            AssertMsg(false, "Intl's Assert platform API was called incorrectly.");
            return scriptContext->GetLibrary()->GetUndefined();
        }

#if DEBUG
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_RaiseAssert\n"));
#endif
        JavascriptExceptionOperators::Throw(JavascriptError::FromVar(args.Values[1]), scriptContext);
#else
        return scriptContext->GetLibrary()->GetUndefined();
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_IsWellFormedLanguageTag(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // IsWellFormedLanguageTag of undefined or non-string is false
            return scriptContext->GetLibrary()->GetFalse();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);

#if defined(INTL_ICU)
        return TO_JSBOOL(scriptContext, IcuIntlAdapter::IsWellFormedLanguageTag(argString->GetSz(), argString->GetLength()));
#else
        return TO_JSBOOL(scriptContext, GetWindowsGlobalizationAdapter(scriptContext)->IsWellFormedLanguageTag(scriptContext, argString->GetSz()));
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_NormalizeLanguageTag(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);

        HRESULT hr;
        JavascriptString *retVal;

#if defined(INTL_ICU)
        // `normalized` will be filled by converting a char* (of utf8 bytes) to char16*
        // Since `len(utf8bytes) >= len(to_char16s(utf8bytes))`,
        // Therefore the max length of that char* (ULOC_FULLNAME_CAPACITY) is big enough to hold the result (including null terminator)
        char16 normalized[ULOC_FULLNAME_CAPACITY] = { 0 };
        size_t normalizedLength = 0;
        hr = IcuIntlAdapter::NormalizeLanguageTag(argString->GetSz(), argString->GetLength(), normalized, &normalizedLength);
        retVal = Js::JavascriptString::NewCopyBuffer(normalized, static_cast<charcount_t>(normalizedLength), scriptContext);
#else
        AutoHSTRING str;
        hr = GetWindowsGlobalizationAdapter(scriptContext)->NormalizeLanguageTag(scriptContext, argString->GetSz(), &str);
        DelayLoadWindowsGlobalization *wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        PCWSTR strBuf = wsl->WindowsGetStringRawBuffer(*str, NULL);
        retVal = Js::JavascriptString::NewCopySz(strBuf, scriptContext);
#endif

        if (FAILED(hr))
        {
            HandleOOMSOEHR(hr);
            //If we can't normalize the tag; return undefined.
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return retVal;
    }
    Var IntlEngineInterfaceExtensionObject::EntryIntl_ResolveLocaleLookup(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);
        PCWSTR passedLocale = argString->GetSz();

#if defined(INTL_ICU)
        char16 resolvedLocaleName[ULOC_FULLNAME_CAPACITY] = { 0 };
        if (IcuIntlAdapter::ResolveLocaleLookup(scriptContext, passedLocale, resolvedLocaleName))
        {
            return JavascriptString::NewCopySz(resolvedLocaleName, scriptContext);
        }

#ifdef INTL_ICU_DEBUG
        Output::Print(_u("IcuIntlAdapter::ResolveLocaleLookup returned false: EntryIntl_ResolveLocaleLookup returning null to fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#else
        // REVIEW should we zero the whole array for safety?
        WCHAR resolvedLocaleName[LOCALE_NAME_MAX_LENGTH];
        resolvedLocaleName[0] = '\0';

        ResolveLocaleName(passedLocale, resolvedLocaleName, _countof(resolvedLocaleName));
        if (resolvedLocaleName[0] == '\0')
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return JavascriptString::NewCopySz(resolvedLocaleName, scriptContext);
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_ResolveLocaleBestFit(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);
        PCWSTR passedLocale = argString->GetSz();

#if defined(INTL_ICU)
        char16 resolvedLocaleName[ULOC_FULLNAME_CAPACITY] = { 0 };
        if (IcuIntlAdapter::ResolveLocaleBestFit(scriptContext, passedLocale, resolvedLocaleName))
        {
            return JavascriptString::NewCopySz(resolvedLocaleName, scriptContext);
        }

#ifdef INTL_ICU_DEBUG
        Output::Print(_u("IcuIntlAdapter::ResolveLocaleBestFit returned false: EntryIntl_ResolveLocaleBestFit returning null to fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#else
        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        AutoCOMPtr<DateTimeFormatting::IDateTimeFormatter> formatter;
        HRESULT hr;
        if (FAILED(hr = wga->CreateDateTimeFormatter(scriptContext, _u("longdate"), &passedLocale, 1, nullptr, nullptr, &formatter)))
        {
            HandleOOMSOEHR(hr);
            return scriptContext->GetLibrary()->GetUndefined();
        }
        AutoHSTRING locale;
        if (FAILED(hr = wga->GetResolvedLanguage(formatter, &locale)))
        {
            HandleOOMSOEHR(hr);
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return JavascriptString::NewCopySz(wgl->WindowsGetStringRawBuffer(*locale, NULL), scriptContext);
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetDefaultLocale(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        char16 defaultLocale[LOCALE_NAME_MAX_LENGTH];
        defaultLocale[0] = '\0';

        if (
#if defined(INTL_WINGLOB)
            // XPLAT-TODO (doilij): Implement this in PlatformAgnostic
            GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) == 0
#else
            IcuIntlAdapter::GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) == 0
#endif
            || defaultLocale[0] == '\0')
        {
#if defined(INTL_WINGLOB)
            return scriptContext->GetLibrary()->GetUndefined();
#else
#ifdef INTL_ICU_DEBUG
            Output::Print(_u("EntryIntl_GetDefaultLocale > IcuIntlAdapter::GetUserDefaultLocaleName returned 0: returning null to fallback to JS\n"));
#endif
            return scriptContext->GetLibrary()->GetNull();
#endif
        }

        return JavascriptString::NewCopySz(defaultLocale, scriptContext);
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetExtensions(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        AutoCOMPtr<ILanguage> language;
        AutoCOMPtr<ILanguageExtensionSubtags> extensionSubtags;
        HRESULT hr;
        if (FAILED(hr = wga->CreateLanguage(scriptContext, JavascriptString::FromVar(args.Values[1])->GetSz(), &language)))
        {
            HandleOOMSOEHR(hr);
            return scriptContext->GetLibrary()->GetUndefined();
        }

        if (FAILED(hr = language->QueryInterface(__uuidof(ILanguageExtensionSubtags), reinterpret_cast<void**>(&extensionSubtags))))
        {
            HandleOOMSOEHR(hr);
            return scriptContext->GetLibrary()->GetUndefined();
        }
        Assert(extensionSubtags);

        AutoHSTRING singletonString;
        AutoCOMPtr<Windows::Foundation::Collections::IVectorView<HSTRING>> subtags;
        uint32 length;

        if (FAILED(hr = wgl->WindowsCreateString(_u("u"), 1, &singletonString)) || FAILED(hr = extensionSubtags->GetExtensionSubtags(*singletonString, &subtags)) || FAILED(subtags->get_Size(&length)))
        {
            HandleOOMSOEHR(hr);
            return scriptContext->GetLibrary()->GetUndefined();
        }
        JavascriptArray *toReturn = scriptContext->GetLibrary()->CreateArray(length);

        for (uint32 i = 0; i < length; i++)
        {
            AutoHSTRING str;
            if (!FAILED(hr = wga->GetItemAt(subtags, i, &str)))
            {
                toReturn->SetItem(i, JavascriptString::NewCopySz(wgl->WindowsGetStringRawBuffer(*str, NULL), scriptContext), Js::PropertyOperationFlags::PropertyOperation_None);
            }
            else
            {
                HandleOOMSOEHR(hr);
            }
        }

        return toReturn;
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_GetExtensions > returning null: fallback to JS function getExtensionSubtags\n"));
#endif
        return scriptContext->GetLibrary()->GetNull(); // fallback to Intl.js
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CacheNumberFormat(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        // The passed object is the hidden state object
        if (args.Info.Count < 2 || !DynamicObject::Is(args.Values[1]))
        {
            // Call with undefined or non-number is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        HRESULT hr;
        Var propertyValue = nullptr;
        JavascriptString* localeJSstr;
        DynamicObject* options = DynamicObject::FromVar(args.Values[1]);

        //Verify locale is present
        if (!GetTypedPropertyBuiltInFrom(options, __locale, JavascriptString) || (localeJSstr = JavascriptString::FromVar(propertyValue))->GetLength() <= 0)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        //First we have to determine which formatter(number, percent, or currency) we will be using.
        //Note some options might not be present.
        AutoCOMPtr<NumberFormatting::INumberFormatter> numberFormatter(nullptr);
        PCWSTR locale = localeJSstr->GetSz();
        uint16 formatterToUseVal = 0; // number is default, 1 is percent, 2 is currency
        if (GetTypedPropertyBuiltInFrom(options, __formatterToUse, TaggedInt) && (formatterToUseVal = TaggedInt::ToUInt16(propertyValue)) == 1)
        {
            //Use the percent formatter
            IfFailThrowHr(wga->CreatePercentFormatter(scriptContext, &locale, 1, &numberFormatter));
        }
        else if (formatterToUseVal == 2)
        {
            //Use the currency formatter
            AutoCOMPtr<NumberFormatting::ICurrencyFormatter> currencyFormatter(nullptr);
            if (!GetTypedPropertyBuiltInFrom(options, __currency, JavascriptString))
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            //API call retrieves a currency formatter, have to query its interface for numberFormatter
            IfFailThrowHr(GetWindowsGlobalizationAdapter(scriptContext)->CreateCurrencyFormatter(scriptContext, &locale, 1, JavascriptString::FromVar(propertyValue)->GetSz(), &currencyFormatter));

            if (GetTypedPropertyBuiltInFrom(options, __currencyDisplayToUse, TaggedInt)) // 0 is for symbol, 1 is for code, 2 is for name.
                                                                                         //Currently name isn't supported; so it will default to code in that case.
            {
                AutoCOMPtr<NumberFormatting::ICurrencyFormatter2> currencyFormatter2(nullptr);
                IfFailThrowHr(currencyFormatter->QueryInterface(__uuidof(NumberFormatting::ICurrencyFormatter2), reinterpret_cast<void**>(&currencyFormatter2)));

                if (TaggedInt::ToUInt16(propertyValue) == 0)
                {
                    IfFailThrowHr(currencyFormatter2->put_Mode(NumberFormatting::CurrencyFormatterMode::CurrencyFormatterMode_UseSymbol));
                }
                else
                {
                    IfFailThrowHr(currencyFormatter2->put_Mode(NumberFormatting::CurrencyFormatterMode::CurrencyFormatterMode_UseCurrencyCode));
                }
            }

            IfFailThrowHr(currencyFormatter->QueryInterface(__uuidof(NumberFormatting::INumberFormatter), reinterpret_cast<void**>(&numberFormatter)));
        }
        else
        {
            //Use the number formatter (default)
            IfFailThrowHr(wga->CreateNumberFormatter(scriptContext, &locale, 1, &numberFormatter));
        }
        Assert(numberFormatter);

        AutoCOMPtr<NumberFormatting::ISignedZeroOption> signedZeroOption(nullptr);
        IfFailThrowHr(numberFormatter->QueryInterface(__uuidof(NumberFormatting::ISignedZeroOption), reinterpret_cast<void**>(&signedZeroOption)));
        IfFailThrowHr(signedZeroOption->put_IsZeroSigned(true));

        //Configure non-digit related options
        AutoCOMPtr<NumberFormatting::INumberFormatterOptions> numberFormatterOptions(nullptr);
        IfFailThrowHr(numberFormatter->QueryInterface(__uuidof(NumberFormatting::INumberFormatterOptions), reinterpret_cast<void**>(&numberFormatterOptions)));
        Assert(numberFormatterOptions);

        if (GetTypedPropertyBuiltInFrom(options, __isDecimalPointAlwaysDisplayed, JavascriptBoolean))
        {
            IfFailThrowHr(numberFormatterOptions->put_IsDecimalPointAlwaysDisplayed((boolean)(JavascriptBoolean::FromVar(propertyValue)->GetValue())));
        }
        if (GetTypedPropertyBuiltInFrom(options, __useGrouping, JavascriptBoolean))
        {
            IfFailThrowHr(numberFormatterOptions->put_IsGrouped((boolean)(JavascriptBoolean::FromVar(propertyValue)->GetValue())));
        }

        //Get the numeral system and add it to the object since it will be located in the locale
        AutoHSTRING hNumeralSystem;
        AutoHSTRING hResolvedLanguage;
        uint32 length;
        IfFailThrowHr(wga->GetNumeralSystem(numberFormatterOptions, &hNumeralSystem));
        SetHSTRINGPropertyBuiltInOn(options, __numberingSystem, *hNumeralSystem);

        IfFailThrowHr(wga->GetResolvedLanguage(numberFormatterOptions, &hResolvedLanguage));
        SetHSTRINGPropertyBuiltInOn(options, __locale, *hResolvedLanguage);

        AutoCOMPtr<NumberFormatting::INumberRounderOption> rounderOptions(nullptr);
        IfFailThrowHr(numberFormatter->QueryInterface(__uuidof(NumberFormatting::INumberRounderOption), reinterpret_cast<void**>(&rounderOptions)));
        Assert(rounderOptions);

        if (HasPropertyBuiltInOn(options, __minimumSignificantDigits) || HasPropertyBuiltInOn(options, __maximumSignificantDigits))
        {
            uint16 minSignificantDigits = 1, maxSignificantDigits = 21;
            //Do significant digit rounding
            if (GetTypedPropertyBuiltInFrom(options, __minimumSignificantDigits, TaggedInt))
            {
                minSignificantDigits = max<uint16>(min<uint16>(TaggedInt::ToUInt16(propertyValue), 21), 1);
            }
            if (GetTypedPropertyBuiltInFrom(options, __maximumSignificantDigits, TaggedInt))
            {
                maxSignificantDigits = max<uint16>(min<uint16>(TaggedInt::ToUInt16(propertyValue), 21), minSignificantDigits);
            }
            prepareWithSignificantDigits(scriptContext, rounderOptions, numberFormatter, numberFormatterOptions, minSignificantDigits, maxSignificantDigits);
        }
        else
        {
            uint16 minFractionDigits = 0, maxFractionDigits = 3, minIntegerDigits = 1;
            //Do fraction/integer digit rounding
            if (GetTypedPropertyBuiltInFrom(options, __minimumIntegerDigits, TaggedInt))
            {
                minIntegerDigits = max<uint16>(min<uint16>(TaggedInt::ToUInt16(propertyValue), 21), 1);
            }
            if (GetTypedPropertyBuiltInFrom(options, __minimumFractionDigits, TaggedInt))
            {
                minFractionDigits = min<uint16>(TaggedInt::ToUInt16(propertyValue), 20);//ToUInt16 will get rid of negatives by making them high
            }
            if (GetTypedPropertyBuiltInFrom(options, __maximumFractionDigits, TaggedInt))
            {
                maxFractionDigits = max(min<uint16>(TaggedInt::ToUInt16(propertyValue), 20), minFractionDigits);//ToUInt16 will get rid of negatives by making them high
            }
            prepareWithFractionIntegerDigits(scriptContext, rounderOptions, numberFormatterOptions, minFractionDigits, maxFractionDigits + (formatterToUseVal == 1 ? 2 : 0), minIntegerDigits);//extend max fractions for percent
        }

        //Set the object as a cache
        numberFormatter->AddRef();
        options->SetInternalProperty(Js::InternalPropertyIds::HiddenObject, AutoCOMJSObject::New(scriptContext->GetRecycler(), numberFormatter), Js::PropertyOperationFlags::PropertyOperation_None, NULL);

        return scriptContext->GetLibrary()->GetUndefined();
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CacheNumberFormat > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    // Unlike CacheNumberFormat; this call takes an additional parameter to specify whether we are going to cache it.
    // We have to create this formatter twice; first time get the date/time patterns; and second time cache with correct format string.
    Var IntlEngineInterfaceExtensionObject::EntryIntl_CreateDateTimeFormat(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 3 || !DynamicObject::Is(args.Values[1]) || !JavascriptBoolean::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        DynamicObject* obj = DynamicObject::FromVar(args.Values[1]);

        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        HRESULT hr;
        Var propertyValue = nullptr;
        uint32 length;

        PCWSTR locale = GetTypedPropertyBuiltInFrom(obj, __locale, JavascriptString) ? JavascriptString::FromVar(propertyValue)->GetSz() : nullptr;
        PCWSTR templateString = GetTypedPropertyBuiltInFrom(obj, __templateString, JavascriptString) ? JavascriptString::FromVar(propertyValue)->GetSz() : nullptr;

        if (locale == nullptr || templateString == nullptr)
        {
            AssertMsg(false, "For some reason, locale and templateString aren't defined or aren't a JavascriptString.");
            return scriptContext->GetLibrary()->GetUndefined();
        }

        PCWSTR clock = GetTypedPropertyBuiltInFrom(obj, __windowsClock, JavascriptString) ? JavascriptString::FromVar(propertyValue)->GetSz() : nullptr;

        AutoHSTRING hDummyCalendar;
        if (clock != nullptr)
        {
            //Because both calendar and clock are needed to pass into the datetimeformatter constructor (or neither); create a dummy one to get the value of calendar out so clock can be passed in with it.
            AutoCOMPtr<DateTimeFormatting::IDateTimeFormatter> dummyFormatter;
            IfFailThrowHr(wga->CreateDateTimeFormatter(scriptContext, templateString, &locale, 1, nullptr, nullptr, &dummyFormatter));

            IfFailThrowHr(wga->GetCalendar(dummyFormatter, &hDummyCalendar));
        }

        //Now create the real formatter.
        AutoCOMPtr<DateTimeFormatting::IDateTimeFormatter> cachedFormatter;
        IfFailThrowHr(wga->CreateDateTimeFormatter(scriptContext, templateString, &locale, 1,
            clock == nullptr ? nullptr : wgl->WindowsGetStringRawBuffer(*hDummyCalendar, &length), clock, &cachedFormatter));

        AutoHSTRING hCalendar;
        AutoHSTRING hClock;
        AutoHSTRING hLocale;
        AutoHSTRING hNumberingSystem;
        //In case the upper code path wasn't hit; extract the calendar string again so it can be set.
        IfFailThrowHr(wga->GetCalendar(cachedFormatter, &hCalendar));
        SetHSTRINGPropertyBuiltInOn(obj, __windowsCalendar, *hCalendar);

        IfFailThrowHr(wga->GetClock(cachedFormatter, &hClock));
        SetHSTRINGPropertyBuiltInOn(obj, __windowsClock, *hClock);

        IfFailThrowHr(wga->GetResolvedLanguage(cachedFormatter, &hLocale));
        SetHSTRINGPropertyBuiltInOn(obj, __locale, *hLocale);

        //Get the numbering system
        IfFailThrowHr(wga->GetNumeralSystem(cachedFormatter, &hNumberingSystem));
        SetHSTRINGPropertyBuiltInOn(obj, __numberingSystem, *hNumberingSystem);

        //Extract the pattern strings
        AutoCOMPtr<Windows::Foundation::Collections::IVectorView<HSTRING>> dateResult;
        IfFailThrowHr(cachedFormatter->get_Patterns(&dateResult));

        IfFailThrowHr(dateResult->get_Size(&length));

        JavascriptArray *patternStrings = scriptContext->GetLibrary()->CreateArray(length);

        for (uint32 i = 0; i < length; i++)
        {
            AutoHSTRING item;
            IfFailThrowHr(wga->GetItemAt(dateResult, i, &item));
            patternStrings->SetItem(i, Js::JavascriptString::NewCopySz(wgl->WindowsGetStringRawBuffer(*item, NULL), scriptContext), PropertyOperation_None);
        }
        SetPropertyBuiltInOn(obj, __patternStrings, patternStrings);

        //This parameter tells us whether we are caching it this time around; or just validating pattern strings
        if ((boolean)(JavascriptBoolean::FromVar(args.Values[2])->GetValue()))
        {
            //If timeZone is undefined; then use the standard dateTimeFormatter to format in local time; otherwise use the IDateTimeFormatter2 to format using specified timezone (UTC)
            if (!GetPropertyBuiltInFrom(obj, __timeZone) || JavascriptOperators::IsUndefinedObject(propertyValue))
            {
                cachedFormatter->AddRef();
                obj->SetInternalProperty(Js::InternalPropertyIds::HiddenObject, AutoCOMJSObject::New(scriptContext->GetRecycler(), cachedFormatter), Js::PropertyOperationFlags::PropertyOperation_None, NULL);
            }
            else
            {
                AutoCOMPtr<DateTimeFormatting::IDateTimeFormatter2> tzCachedFormatter;
                IfFailThrowHr(cachedFormatter->QueryInterface(__uuidof(DateTimeFormatting::IDateTimeFormatter2), reinterpret_cast<void**>(&tzCachedFormatter)));
                tzCachedFormatter->AddRef();

                //Set the object as a cache
                obj->SetInternalProperty(Js::InternalPropertyIds::HiddenObject, AutoCOMJSObject::New(scriptContext->GetRecycler(), tzCachedFormatter), Js::PropertyOperationFlags::PropertyOperation_None, NULL);
            }
        }

        return scriptContext->GetLibrary()->GetUndefined();
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CreateDateTimeFormat > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    DWORD getFlagsForSensitivity(LPCWSTR sensitivity)
    {
        if (wcscmp(sensitivity, _u("base")) == 0)
        {
            return LINGUISTIC_IGNOREDIACRITIC | LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH;
        }
        else if (wcscmp(sensitivity, _u("accent")) == 0)
        {
            return LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH;
        }
        else if (wcscmp(sensitivity, _u("case")) == 0)
        {
            return  NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LINGUISTIC_IGNOREDIACRITIC;
        }
        else if (wcscmp(sensitivity, _u("variant")) == 0)
        {
            return NORM_LINGUISTIC_CASING;
        }
        return 0;
    }

    // Takes arguments as follows(all required):
    //     - [1] - String 1 for comparison
    //     - [2] - String 2 for comparison
    //     - [3] - Locale string (or undefined)
    //     - [4] - Sensitivity string (or undefined)
    //     - [5] - IgnorePunctuation boolean (or undefined)
    //     - [6] - Numeric boolean (or undefined)
    Var IntlEngineInterfaceExtensionObject::EntryIntl_CompareString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 7 || !JavascriptString::Is(args.Values[1]) || !JavascriptString::Is(args.Values[2]))
        {
            // CompareStringEx of undefined or non-strings is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DWORD compareFlags = 0;
        JavascriptString* str1 = JavascriptString::FromVar(args.Values[1]);
        JavascriptString* str2 = JavascriptString::FromVar(args.Values[2]);

        WCHAR defaultLocale[LOCALE_NAME_MAX_LENGTH];
        const char16 *givenLocale = nullptr;
        defaultLocale[0] = '\0';

        if (!JavascriptOperators::IsUndefinedObject(args.Values[3], scriptContext))
        {
            if (!JavascriptString::Is(args.Values[3]))
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            givenLocale = JavascriptString::FromVar(args.Values[3])->GetSz();
        }

        if (!JavascriptOperators::IsUndefinedObject(args.Values[4], scriptContext))
        {
            if (!JavascriptString::Is(args.Values[4]))
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            compareFlags |= getFlagsForSensitivity(JavascriptString::FromVar(args.Values[4])->GetSz());
        }
        else
        {
            compareFlags |= NORM_LINGUISTIC_CASING;
        }

        if (!JavascriptOperators::IsUndefinedObject(args.Values[5], scriptContext))
        {
            if (!JavascriptBoolean::Is(args.Values[5]))
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            else if ((boolean)(JavascriptBoolean::FromVar(args.Values[5])->GetValue()))
            {
                compareFlags |= NORM_IGNORESYMBOLS;
            }
        }

        if (!JavascriptOperators::IsUndefinedObject(args.Values[6], scriptContext))
        {
            if (!JavascriptBoolean::Is(args.Values[6]))
            {
                return scriptContext->GetLibrary()->GetUndefined();
            }
            else if ((boolean)(JavascriptBoolean::FromVar(args.Values[6])->GetValue()))
            {
                compareFlags |= SORT_DIGITSASNUMBERS;
            }
        }

        if (givenLocale == nullptr &&
#if defined(INTL_WINGLOB)
            // XPLAT-TODO (doilij): Implement this in PlatformAgnostic
            GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) == 0
#else
            IcuIntlAdapter::GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) == 0
#endif
            )
        {
            JavascriptError::MapAndThrowError(scriptContext, HRESULT_FROM_WIN32(GetLastError()));
        }

        int compareResult = 0;
        DWORD lastError = S_OK;
        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("localeCompare"))
        {
            using namespace PlatformAgnostic;
            char16 * aLeft = nullptr;
            char16 * aRight = nullptr;
            charcount_t size1 = 0;
            charcount_t size2 = 0;
            auto canonicalEquivalentForm = UnicodeText::NormalizationForm::C;
            if (!UnicodeText::IsNormalizedString(canonicalEquivalentForm, str1->GetSz(), -1))
            {
                aLeft = str1->GetNormalizedString(canonicalEquivalentForm, tempAllocator, size1);
            }

            if (!UnicodeText::IsNormalizedString(canonicalEquivalentForm, str2->GetSz(), -1))
            {
                aRight = str2->GetNormalizedString(canonicalEquivalentForm, tempAllocator, size2);
            }

            if (aLeft == nullptr)
            {
                aLeft = const_cast<char16*>(str1->GetSz());
                size1 = str1->GetLength();
            }
            if (aRight == nullptr)
            {
                aRight = const_cast<char16*>(str2->GetSz());
                size2 = str2->GetLength();
            }

#ifdef _WIN32
            // xplat-todo: Need to replace this with platform-agnostic API
            compareResult = CompareStringEx(givenLocale != nullptr ? givenLocale : defaultLocale, compareFlags, aLeft, size1, aRight, size2, NULL, NULL, 0);
#else
            // FIXME (doilij): when CompareStringEx is implemented in PlatformAgnostic, call that function here
            compareResult = 2; // 2 means strings are equal (reasonable default)
#endif

            // Get the last error code so that it won't be affected by END_TEMP_ALLOCATOR.
            if (compareResult == 0)
            {
                lastError = GetLastError();
            }
        }
        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);

        // CompareStringEx returns 1, 2, 3 on success; 2 if the strings are equal, 1 if the first string is lexically less than second, 3 otherwise.
        if (compareResult != 0)
        {
            return JavascriptNumber::ToVar(compareResult - 2, scriptContext); // Convert 1,2,3 to -1,0,1
        }

        JavascriptError::MapAndThrowError(scriptContext, HRESULT_FROM_WIN32(lastError));
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CurrencyDigits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // Call with undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetFalse();
        }

#ifdef INTL_WINGLOB
        HRESULT hr;
        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);
        AutoCOMPtr<NumberFormatting::ICurrencyFormatter> currencyFormatter(nullptr);
        IfFailThrowHr(GetWindowsGlobalizationAdapter(scriptContext)->CreateCurrencyFormatterCode(scriptContext, argString->GetSz(), &currencyFormatter));
        AutoCOMPtr<NumberFormatting::INumberFormatterOptions> numberFormatterOptions;
        IfFailThrowHr(currencyFormatter->QueryInterface(__uuidof(NumberFormatting::INumberFormatterOptions), reinterpret_cast<void**>(&numberFormatterOptions)));
        Assert(numberFormatterOptions);
        INT32 fractionDigits;
        IfFailThrowHr(numberFormatterOptions->get_FractionDigits(&fractionDigits));
        return JavascriptNumber::ToVar(fractionDigits, scriptContext);
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_CurrencyDigits > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

#ifdef INTL_WINGLOB
    //Helper, this just prepares based on fraction and integer format options
    void IntlEngineInterfaceExtensionObject::prepareWithFractionIntegerDigits(ScriptContext* scriptContext, NumberFormatting::INumberRounderOption* rounderOptions,
        NumberFormatting::INumberFormatterOptions* formatterOptions, uint16 minFractionDigits, uint16 maxFractionDigits, uint16 minIntegerDigits)
    {
        HRESULT hr;
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);
        AutoCOMPtr<NumberFormatting::INumberRounder> numberRounder(nullptr);
        AutoCOMPtr<NumberFormatting::IIncrementNumberRounder> incrementNumberRounder(nullptr);

        IfFailThrowHr(wga->CreateIncrementNumberRounder(scriptContext, &numberRounder));
        IfFailThrowHr(numberRounder->QueryInterface(__uuidof(NumberFormatting::IIncrementNumberRounder), reinterpret_cast<void**>(&incrementNumberRounder)));
        Assert(incrementNumberRounder);
        IfFailThrowHr(incrementNumberRounder->put_RoundingAlgorithm(Windows::Globalization::NumberFormatting::RoundingAlgorithm::RoundingAlgorithm_RoundHalfAwayFromZero));

        IfFailThrowHr(incrementNumberRounder->put_Increment(pow(10.0, -maxFractionDigits)));
        IfFailThrowHr(rounderOptions->put_NumberRounder(numberRounder));

        IfFailThrowHr(formatterOptions->put_FractionDigits(minFractionDigits));
        IfFailThrowHr(formatterOptions->put_IntegerDigits(minIntegerDigits));
    }

    //Helper, this just prepares based on significant digits format options
    void IntlEngineInterfaceExtensionObject::prepareWithSignificantDigits(ScriptContext* scriptContext, NumberFormatting::INumberRounderOption* rounderOptions, NumberFormatting::INumberFormatter *numberFormatter,
        NumberFormatting::INumberFormatterOptions* formatterOptions, uint16 minSignificantDigits, uint16 maxSignificantDigits)
    {
        HRESULT hr;
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);
        AutoCOMPtr<NumberFormatting::INumberRounder> numberRounder(nullptr);
        AutoCOMPtr<NumberFormatting::ISignificantDigitsNumberRounder> incrementNumberRounder(nullptr);
        AutoCOMPtr<NumberFormatting::ISignificantDigitsOption> significantDigitsOptions(nullptr);

        IfFailThrowHr(wga->CreateSignificantDigitsRounder(scriptContext, &numberRounder));
        IfFailThrowHr(numberRounder->QueryInterface(__uuidof(NumberFormatting::ISignificantDigitsNumberRounder), reinterpret_cast<void**>(&incrementNumberRounder)));
        Assert(incrementNumberRounder);
        IfFailThrowHr(incrementNumberRounder->put_RoundingAlgorithm(Windows::Globalization::NumberFormatting::RoundingAlgorithm::RoundingAlgorithm_RoundHalfAwayFromZero));

        IfFailThrowHr(incrementNumberRounder->put_SignificantDigits(maxSignificantDigits));
        IfFailThrowHr(rounderOptions->put_NumberRounder(numberRounder));

        IfFailThrowHr(numberFormatter->QueryInterface(__uuidof(NumberFormatting::ISignificantDigitsOption), reinterpret_cast<void**>(&significantDigitsOptions)));
        IfFailThrowHr(significantDigitsOptions->put_SignificantDigits(minSignificantDigits));
        Assert(significantDigitsOptions);

        //Clear minimum fraction digits as in the case of percent 2 is supplied
        IfFailThrowHr(formatterOptions->put_FractionDigits(0));
    }
#endif

    /*
    * This function has the following options:
    *  - Format as Percent.
    *  - Format as Number.
    *  - If significant digits are present, format using the significant digts;
    *  - Otherwise format using minimumFractionDigits, maximumFractionDigits, minimumIntegerDigits
    */
    Var IntlEngineInterfaceExtensionObject::EntryIntl_FormatNumber(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        //First argument is required and must be either a tagged integer or a number; second is also required and is the internal state object
        if (args.Info.Count < 3 || !(TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) || !DynamicObject::Is(args.Values[2]))
        {
            // Call with undefined or non-number is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        DynamicObject *obj = DynamicObject::FromVar(args.Values[2]);

        DelayLoadWindowsGlobalization* wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();

        NumberFormatting::INumberFormatter *numberFormatter;
        Var hiddenObject;
        obj->GetInternalProperty(obj, Js::InternalPropertyIds::HiddenObject, &hiddenObject, NULL, scriptContext);

        numberFormatter = static_cast<NumberFormatting::INumberFormatter *>(((AutoCOMJSObject *)hiddenObject)->GetInstance());

        AutoHSTRING result;
        HRESULT hr;
        if (TaggedInt::Is(args.Values[1]))
        {
            IfFailThrowHr(numberFormatter->FormatInt(TaggedInt::ToInt32(args.Values[1]), &result));
        }
        else
        {
            IfFailThrowHr(numberFormatter->FormatDouble(JavascriptNumber::GetValue(args.Values[1]), &result));
        }
        PCWSTR strBuf = wsl->WindowsGetStringRawBuffer(*result, NULL);
        JavascriptStringObject *retVal = scriptContext->GetLibrary()->CreateStringObject(Js::JavascriptString::NewCopySz(strBuf, scriptContext));
        return retVal;
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_FormatNumber > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_FormatDateTime(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 3 || !(TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) || !DynamicObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        Windows::Foundation::DateTime winDate;
        HRESULT hr;
        if (TaggedInt::Is(args.Values[1]))
        {
            hr = Js::DateUtilities::ES5DateToWinRTDate(TaggedInt::ToInt32(args.Values[1]), &(winDate.UniversalTime));
        }
        else
        {
            hr = Js::DateUtilities::ES5DateToWinRTDate(JavascriptNumber::GetValue(args.Values[1]), &(winDate.UniversalTime));
        }
        if (FAILED(hr))
        {
            HandleOOMSOEHR(hr);
            // If conversion failed, double value is outside the range of WinRT DateTime
            Js::JavascriptError::ThrowRangeError(scriptContext, JSERR_OutOfDateTimeRange);
        }

        DynamicObject* obj = DynamicObject::FromVar(args.Values[2]);
        Var hiddenObject;

        obj->GetInternalProperty(obj, Js::InternalPropertyIds::HiddenObject, &hiddenObject, NULL, scriptContext);

        //We are going to perform the same check for timeZone as when caching the formatter.
        Var propertyValue = nullptr;
        AutoHSTRING result;

        //If timeZone is undefined; then use the standard dateTimeFormatter to format in local time; otherwise use the IDateTimeFormatter2 to format using specified timezone (UTC)
        if (!GetPropertyBuiltInFrom(obj, __timeZone) || JavascriptOperators::IsUndefinedObject(propertyValue))
        {
            DateTimeFormatting::IDateTimeFormatter *formatter = static_cast<DateTimeFormatting::IDateTimeFormatter *>(((AutoCOMJSObject *)hiddenObject)->GetInstance());
            Assert(formatter);
            IfFailThrowHr(formatter->Format(winDate, &result));
        }
        else
        {
            DateTimeFormatting::IDateTimeFormatter2 *formatter = static_cast<DateTimeFormatting::IDateTimeFormatter2 *>(((AutoCOMJSObject *)hiddenObject)->GetInstance());
            Assert(formatter);
            HSTRING timeZone;
            HSTRING_HEADER timeZoneHeader;

            // IsValidTimeZone() has already verified that this is JavascriptString.
            JavascriptString* userDefinedTimeZoneId = JavascriptString::FromVar(propertyValue);
            IfFailThrowHr(WindowsCreateStringReference(userDefinedTimeZoneId->GetSz(), userDefinedTimeZoneId->GetLength(), &timeZoneHeader, &timeZone));
            Assert(timeZone);

            IfFailThrowHr(formatter->FormatUsingTimeZone(winDate, timeZone, &result));
        }
        PCWSTR strBuf = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary()->WindowsGetStringRawBuffer(*result, NULL);

        return Js::JavascriptString::NewCopySz(strBuf, scriptContext);
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_FormatDateTime > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    /*
    *   This function validates the timeZone passed by user has defined in IsValidTimeZoneName() section
    *   of ECMA-402 dated June 2015.
    *   Returns true if timeZoneId is a valid zone or link name of the IANA time zone database
    */
    Var IntlEngineInterfaceExtensionObject::EntryIntl_ValidateAndCanonicalizeTimeZone(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        // Return false if timeZoneId is not string
        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            AssertMsg(false, "Need valid timeZoneId");
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);

        AutoHSTRING canonicalizedTimeZone;
        boolean isValidTimeZone = GetWindowsGlobalizationAdapter(scriptContext)->ValidateAndCanonicalizeTimeZone(scriptContext, argString->GetSz(), &canonicalizedTimeZone);
        if (isValidTimeZone)
        {
            DelayLoadWindowsGlobalization* wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
            PCWSTR strBuf = wsl->WindowsGetStringRawBuffer(*canonicalizedTimeZone, NULL);
            return Js::JavascriptString::NewCopySz(strBuf, scriptContext);
        }
        else
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_ValidateAndCanonicalizeTimeZone > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    /*
    *   This function returns defaultTimeZone for host's current environment as specified in
    *   DefaultTimeZone () section of ECMA-402 dated June 2015.
    */
    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetDefaultTimeZone(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#ifdef INTL_WINGLOB
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);
        DelayLoadWindowsGlobalization* wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        AutoHSTRING str;

        HRESULT hr;
        if (FAILED(hr = wga->GetDefaultTimeZoneId(scriptContext, &str)))
        {
            HandleOOMSOEHR(hr);
            //If we can't get default timeZone, return undefined.
            return scriptContext->GetLibrary()->GetUndefined();
        }

        PCWSTR strBuf = wsl->WindowsGetStringRawBuffer(*str, NULL);
        return Js::JavascriptString::NewCopySz(strBuf, scriptContext);
#else
        // TODO (doilij): implement INTL_ICU version
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_GetDefaultTimeZone > returning null, fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#endif
    }

    /*
    * This function registers built in functions when Intl initializes.
    * Call with (Function : toRegister, integer : id)
    * ID Mappings:
    *  - 0 for Date.prototype.toLocaleString
    *  - 1 for Date.prototype.toLocaleDateString
    *  - 2 for Date.prototype.toLocaleTimeString
    *  - 3 for Number.prototype.toLocaleString
    *  - 4 for String.prototype.localeCompare
    */
    Var IntlEngineInterfaceExtensionObject::EntryIntl_RegisterBuiltInFunction(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        //This function will only be used during the construction of the Intl object, hence Asserts are in place.
        Assert(args.Info.Count >= 3 && JavascriptFunction::Is(args.Values[1]) && TaggedInt::Is(args.Values[2]));

        JavascriptFunction *func = JavascriptFunction::FromVar(args.Values[1]);
        int32 id = TaggedInt::ToInt32(args.Values[2]);

        Assert(id >= 0 && id < 5);
        EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
        IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));

        switch (id)
        {
        case 0:
            extensionObject->dateToLocaleString = func;
            break;
        case 1:
            extensionObject->dateToLocaleDateString = func;
            break;
        case 2:
            extensionObject->dateToLocaleTimeString = func;
            break;
        case 3:
            extensionObject->numberToLocaleString = func;
            break;
        case 4:
            extensionObject->stringLocaleCompare = func;
            break;
        default:
            Assert(false);//Shouldn't hit here, the previous assert should catch this.
            break;
        }

        //Don't need to return anything
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetHiddenObject(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 2 || !DynamicObject::Is(args.Values[1]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = DynamicObject::FromVar(args.Values[1]);
        Var hiddenObject;
        if (!obj->GetInternalProperty(obj, Js::InternalPropertyIds::HiddenObject, &hiddenObject, NULL, scriptContext))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        return hiddenObject;
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_SetHiddenObject(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (callInfo.Count < 3 || !DynamicObject::Is(args.Values[1]) || !DynamicObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = DynamicObject::FromVar(args.Values[1]);
        DynamicObject* value = DynamicObject::FromVar(args.Values[2]);

        if (obj->SetInternalProperty(Js::InternalPropertyIds::HiddenObject, value, Js::PropertyOperationFlags::PropertyOperation_None, NULL))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }
        else
        {
            return scriptContext->GetLibrary()->GetFalse();
        }
    }

    /*
    * First parameter is the object onto which prototype should be set; second is the value
    */
    Var IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_SetPrototype(RecyclableObject *function, CallInfo callInfo, ...)
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
    Var IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_GetArrayLength(RecyclableObject *function, CallInfo callInfo, ...)
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
    Var IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_RegexMatch(RecyclableObject *function, CallInfo callInfo, ...)
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
    Var IntlEngineInterfaceExtensionObject::EntryIntl_BuiltIn_CallInstanceFunction(RecyclableObject *function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        Assert(args.Info.Count <= 5);
        if (callInfo.Count < 3 || args.Info.Count > 5 || !JavascriptConversion::IsCallable(args.Values[1]) || !RecyclableObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        RecyclableObject *func = RecyclableObject::FromVar(args.Values[1]);

        //Shift the arguments by 2 so argument at index 2 becomes the 'this' argument at index 0
        Var newVars[3];
        Js::Arguments newArgs(callInfo, newVars);

        for (uint i = 0; i<args.Info.Count - 2; ++i)
        {
            newArgs.Values[i] = args.Values[i + 2];
        }

        newArgs.Info.Count = args.Info.Count - 2;

        return JavascriptFunction::CallFunction<true>(func, func->GetEntryPoint(), newArgs);
    }
#endif
}
#endif
