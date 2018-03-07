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
#include "Codex/Utf8Helper.h"

#ifdef INTL_WINGLOB
using namespace Windows::Globalization;
#endif

#ifdef INTL_ICU
#include <CommonPal.h>
#include "PlatformAgnostic/ChakraICU.h"
using namespace PlatformAgnostic::ICUHelpers;

template<typename ICUFunc>
static void RecyclerExecuteICUWithRetry(_In_ ICUFunc func, _In_ Recycler *recycler, _Out_writes_opt_(returnLen) char16 **ret, _Out_ int *returnLen, _In_ int firstTryLength = 32)
{
    bool success = ExecuteICUWithRetry<char16, ICUFunc>([&](int length) {
        return RecyclerNewArrayLeaf(recycler, char16, length);
    }, func, firstTryLength, ret, returnLen);
    AssertOrFailFastMsg(success, "Could not allocate buffer for ICU call");
}

#define ICU_ERROR_FMT _u("INTL: %S failed with error code %S\n")
#define ICU_EXPR_FMT _u("INTL: %S failed expression check %S\n")

#define ICU_ASSERT(e, expr)                                                   \
    do                                                                        \
    {                                                                         \
        if (ICU_FAILURE(e))                                                   \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_ERROR_FMT, ICU_ERRORMESSAGE(e));              \
            AssertOrFailFastMsg(false, ICU_ERRORMESSAGE(e));                  \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            ICU_DEBUG_PRINT(ICU_EXPR_FMT, ICU_ERRORMESSAGE(e));               \
            AssertOrFailFast(expr);                                           \
        }                                                                     \
    } while (false)

#define BCP47_TO_ICU(langtag, langtagLen, localeID, localeIDLen)              \
    do                                                                        \
    {                                                                         \
        UErrorCode __status = U_ZERO_ERROR;                                   \
        utf8::WideToNarrow __lt((langtag), (langtagLen));                     \
        int32_t __len = 0;                                                    \
        uloc_forLanguageTag(__lt, localeID, localeIDLen, &__len, &__status);  \
        ICU_ASSERT(__status, __len > 0 && __len < localeIDLen);               \
    } while (false)

#define ASSERT_ENUM(T, e)                                                     \
    do                                                                        \
    {                                                                         \
        if ((int)(e) < 0 || (e) >= T::Max)                                    \
        {                                                                     \
            AssertMsg(false, #e " of type " #T " has an invalid value");      \
        }                                                                     \
    } while (false)

#endif // INTL_ICU

// NOTE(jahorto): Keep these enums in sync with those by the same name in Intl.js
// These enums are used by both WinGlob- and ICU-backed Intl
enum class NumberFormatStyle
{
    Decimal, // Intl.NumberFormat(locale, { style: "decimal" }); // aka in our code as "number"
    Percent, // Intl.NumberFormat(locale, { style: "percent" });
    Currency, // Intl.NumberFormat(locale, { style: "currency", ... });

    Max,
    Default = Decimal,
};

enum class NumberFormatCurrencyDisplay
{
    Symbol, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "symbol" }); // e.g. "$" or "US$" depeding on locale
    Code, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "code" }); // e.g. "USD"
    Name, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "name" }); // e.g. "US dollar"

    Max,
    Default = Symbol,
};

enum class CollatorSensitivity
{
    Base,
    Accent,
    Case,
    Variant,

    Max,
    Default = Variant,
};

enum class CollatorCaseFirst
{
    Upper,
    Lower,
    False,

    Max,
    Default = False,
};

#pragma warning(push)
#pragma warning(disable:4309) // truncation of constant value
#pragma warning(disable:4838) // conversion from 'int' to 'const char' requires a narrowing conversion

#if DISABLE_JIT
#if TARGET_64
#include "InJavascript/Intl.js.nojit.bc.64b.h"
#else
#include "InJavascript/Intl.js.nojit.bc.32b.h"
#endif
#else
#if TARGET_64
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

#define INTL_CHECK_ARGS(argcheck) AssertOrFailFastMsg((argcheck), "Intl platform function given bad arguments")

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
#endif

// Defining Finalizable wrappers for Intl data
#if defined(INTL_WINGLOB)
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

#elif defined(INTL_ICU)

    template<typename TResource, void(__cdecl * CloseFunction)(TResource)>
    class FinalizableICUObject : public FinalizableObject
    {
    private:
        FieldNoBarrier(TResource) resource;
    public:
        FinalizableICUObject(TResource resource) : resource(resource)
        {

        }
        static FinalizableICUObject<TResource, CloseFunction> *New(Recycler *recycler, TResource resource)
        {
            return RecyclerNewFinalized(recycler, FinalizableICUObject, resource);
        }

        TResource GetInstance()
        {
            return resource;
        }

        operator TResource()
        {
            return resource;
        }

        void Finalize(bool isShutdown) override
        {

        }

        void Dispose(bool isShutdown) override
        {
            if (!isShutdown)
            {
                CloseFunction(resource);
            }
        }

        void Mark(Recycler *recycler) override
        {

        }
    };
    typedef FinalizableICUObject<UNumberFormat *, unum_close> FinalizableUNumberFormat;
    typedef FinalizableICUObject<UDateFormat *, udat_close> FinalizableUDateFormat;
    typedef FinalizableICUObject<UFieldPositionIterator *, ufieldpositer_close> FinalizableUFieldPositionIterator;
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

// Initializes the IntlEngineInterfaceExtensionObject::EntryInfo struct
#ifdef INTL_ENTRY
#undef INTL_ENTRY
#endif
#define INTL_ENTRY(id, func) \
    NoProfileFunctionInfo IntlEngineInterfaceExtensionObject::EntryInfo::Intl_##func##(FORCE_NO_WRITE_BARRIER_TAG(IntlEngineInterfaceExtensionObject::EntryIntl_##func##));
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY

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

        // TODO: JsBuiltIns fail without these being initialized here?
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
        Assert(this->intlByteCode);
        Js::ByteCodeDumper::DumpRecursively(intlByteCode);
    }
#endif

    bool IntlEngineInterfaceExtensionObject::InitializeIntlNativeInterfaces(DynamicObject* intlNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode)
    {
        typeHandler->Convert(intlNativeInterfaces, mode, 16);

        ScriptContext* scriptContext = intlNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

// gives each entrypoint a property ID on the intlNativeInterfaces library object
#ifdef INTL_ENTRY
#undef INTL_ENTRY
#endif
#define INTL_ENTRY(id, func) \
    library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::##id, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_##func, 1);
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY

#if INTL_WINGLOB
        library->AddMember(intlNativeInterfaces, Js::PropertyIds::winglob, library->GetTrue());
#else
        library->AddMember(intlNativeInterfaces, Js::PropertyIds::winglob, library->GetFalse());
#endif

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

            this->SetHasBytecode();
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

#if DBG_DUMP
            if (PHASE_DUMP(Js::ByteCodePhase, function->GetFunctionProxy()) && Js::Configuration::Global.flags.Verbose)
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
#if defined(INTL_ICU)
        AssertOrFailFastMsg(false, "IsWellFormedLanguageTag is not implemented using ICU");
        return nullptr;
#else
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // IsWellFormedLanguageTag of undefined or non-string is false
            return scriptContext->GetLibrary()->GetFalse();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);

        return TO_JSBOOL(scriptContext, GetWindowsGlobalizationAdapter(scriptContext)->IsWellFormedLanguageTag(scriptContext, argString->GetSz()));
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_NormalizeLanguageTag(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#if defined(INTL_ICU)
        INTL_CHECK_ARGS(args.Info.Count == 2 && JavascriptString::Is(args[1]));

        UErrorCode status = U_ZERO_ERROR;
        JavascriptString *langtag = JavascriptString::UnsafeFromVar(args[1]);
        utf8::WideToNarrow langtag8(langtag->GetSz(), langtag->GetLength());

        // ICU doesn't have a full-fledged canonicalization implementation that correctly replaces all preferred values
        // and grandfathered tags, as required by #sec-canonicalizelanguagetag.
        // However, passing the locale through uloc_forLanguageTag -> uloc_toLanguageTag gets us most of the way there
        // by replacing some(?) values, correctly capitalizing the tag, and re-ordering extensions
        int parsedLength = 0;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int forLangTagResultLength = uloc_forLanguageTag(langtag8, localeID, ULOC_FULLNAME_CAPACITY, &parsedLength, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR)
        {
            // The string passed in to NormalizeLanguageTag has already passed IsStructurallyValidLanguageTag.
            // However, duplicate unicode extension keys, such as "de-u-co-phonebk-co-phonebk", are structurally
            // valid according to RFC5646 yet still trigger U_ILLEGAL_ARGUMENT_ERROR
            // V8 ~6.2 says that the above language tag is invalid, while SpiderMonkey ~58 handles it.
            // Until we have a more spec-compliant implementation of CanonicalizeLanguageTag, err on the side
            // of caution and say it is invalid.
            // NOTE: make sure we check for `undefined` at the platform.normalizeLanguageTag callsite.
            return scriptContext->GetLibrary()->GetUndefined();
        }

        // forLangTagResultLength can be 0 if langtag is "und".
        // uloc_toLanguageTag("") returns "und", so this works out (forLanguageTag can return >= 0 but toLanguageTag must return > 0)
        ICU_ASSERT(status, forLangTagResultLength >= 0 && ((charcount_t) parsedLength) == langtag->GetLength());

        char canonicalized[ULOC_FULLNAME_CAPACITY] = { 0 };
        int toLangTagResultLength = uloc_toLanguageTag(localeID, canonicalized, ULOC_FULLNAME_CAPACITY, true, &status);
        ICU_ASSERT(status, toLangTagResultLength > 0);

        // allocate toLangTagResultLength + 1 to leave room for null terminator
        char16 *canonicalized16 = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, toLangTagResultLength + 1);
        charcount_t canonicalized16Len = 0;
        HRESULT hr = utf8::NarrowStringToWideNoAlloc(
            canonicalized,
            toLangTagResultLength,
            canonicalized16,
            toLangTagResultLength + 1,
            &canonicalized16Len
        );
        AssertOrFailFast(hr == S_OK && ((int) canonicalized16Len) == toLangTagResultLength);

        return JavascriptString::NewWithBuffer(canonicalized16, toLangTagResultLength, scriptContext);
#else
        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);
        JavascriptString *retVal;
        HRESULT hr;
        AutoHSTRING str;
        hr = GetWindowsGlobalizationAdapter(scriptContext)->NormalizeLanguageTag(scriptContext, argString->GetSz(), &str);
        DelayLoadWindowsGlobalization *wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        PCWSTR strBuf = wsl->WindowsGetStringRawBuffer(*str, NULL);
        retVal = Js::JavascriptString::NewCopySz(strBuf, scriptContext);
        if (FAILED(hr))
        {
            HandleOOMSOEHR(hr);
            //If we can't normalize the tag; return undefined.
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return retVal;
#endif
    }

#ifdef INTL_ICU
    typedef const char * (*GetAvailableLocaleFunc)(int);
    typedef int (*CountAvailableLocaleFunc)(void);
    static bool findLocale(JavascriptString *langtag, CountAvailableLocaleFunc countAvailable, GetAvailableLocaleFunc getAvailable)
    {
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        BCP47_TO_ICU(langtag->GetSz(), langtag->GetLength(), localeID, ULOC_FULLNAME_CAPACITY);

        // ICU's "available locales" do not include (all? most?) aliases.
        // For example, searching for "zh_TW" will return false, even though
        // zh_TW is equivalent to zh_Hant_TW, which would return true.
        // We can work around this by searching for both the locale as requested and
        // the locale in addition to all of its "likely subtags."
        // This works in practice because, for instance, unum_open("zh_TW") will actually
        // use "zh_Hant_TW" (confirmed with unum_getLocaleByType(..., ULOC_VALID_LOCALE))
        // The code below performs, for example, the following mappings:
        // pa_PK -> pa_Arab_PK
        // sr_RS -> sr_Cyrl_RS
        // zh_CN -> zh_Hans_CN
        // zh_TW -> zh_Hant_TW
        // TODO(jahorto): Determine if there is any scenario where a language tag + likely subtags is
        // not exactly functionally equivalent to the language tag on its own -- basically, where
        // constructor_open(language_tag) behaves differently to constructor_open(language_tag_and_likely_subtags)
        // for all supported constructors.
        UErrorCode status = U_ZERO_ERROR;
        char localeIDWithLikelySubtags[ULOC_FULLNAME_CAPACITY] = { 0 };
        int likelySubtagLen = uloc_addLikelySubtags(localeID, localeIDWithLikelySubtags, ULOC_FULLNAME_CAPACITY, &status);
        ICU_ASSERT(status, likelySubtagLen > 0 && likelySubtagLen < ULOC_FULLNAME_CAPACITY);

        // TODO(jahorto): can we binary search this instead?
        for (int i = 0; i < countAvailable(); i++)
        {
            const char *cur = getAvailable(i);
            if (strcmp(localeID, cur) == 0 || strcmp(localeIDWithLikelySubtags, cur) == 0)
            {
                return true;
            }
        }

        return false;
    }
#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_IsDTFLocaleAvailable(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && JavascriptString::Is(args.Values[1]));

        return TO_JSBOOL(scriptContext, findLocale(JavascriptString::UnsafeFromVar(args.Values[1]), udat_countAvailable, udat_getAvailable));
#else
        AssertOrFailFastMsg(false, "Intl with Windows Globalization should never call IsDTFLocaleAvailable");
        return nullptr;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_IsCollatorLocaleAvailable(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && JavascriptString::Is(args.Values[1]));

        return TO_JSBOOL(scriptContext, findLocale(JavascriptString::UnsafeFromVar(args.Values[1]), ucol_countAvailable, ucol_getAvailable));
#else
        AssertOrFailFastMsg(false, "Intl with Windows Globalization should never call IsCollatorLocaleAvailable");
        return nullptr;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_IsNFLocaleAvailable(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && JavascriptString::Is(args.Values[1]));

        return TO_JSBOOL(scriptContext, findLocale(JavascriptString::UnsafeFromVar(args.Values[1]), unum_countAvailable, unum_getAvailable));
#else
        AssertOrFailFastMsg(false, "Intl with Windows Globalization should never call IsNFLocaleAvailable");
        return nullptr;
#endif
    }

#ifdef INTL_ICU
    enum class LocaleDataKind
    {
        Collation,
        CaseFirst,
        Numeric,
        Calendar,
        NumberingSystem,
        HourCycle
    };
#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetLocaleData(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(
            args.Info.Count == 3 &&
            (JavascriptNumber::Is(args.Values[1]) || TaggedInt::Is(args.Values[1])) &&
            JavascriptString::Is(args.Values[2])
        );

        LocaleDataKind kind = (LocaleDataKind) (TaggedInt::Is(args.Values[1])
            ? TaggedInt::ToInt32(args.Values[1])
            : (int) JavascriptNumber::GetValue(args.Values[1]));

        JavascriptArray *ret = nullptr;

        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        JavascriptString *langtag = JavascriptString::UnsafeFromVar(args.Values[2]);
        BCP47_TO_ICU(langtag->GetSz(), langtag->GetLength(), localeID, ULOC_FULLNAME_CAPACITY);

        if (kind == LocaleDataKind::Collation)
        {
            ScopedUEnumeration collations(ucol_getKeywordValuesForLocale("collation", localeID, false, &status));
            ICU_ASSERT(status, true);

            // the return array can't include "standard" and "search", but must have its first element be null (count - 2 + 1) [#sec-intl-collator-internal-slots]
            ret = scriptContext->GetLibrary()->CreateArray(uenum_count(collations, &status) - 1);
            ICU_ASSERT(status, true);
            ret->SetItem(0, scriptContext->GetLibrary()->GetNull(), PropertyOperationFlags::PropertyOperation_None);

            int collationLen = 0;
            const char *collation = nullptr;
            int i = 0;
            for (collation = uenum_next(collations, &collationLen, &status); collation != nullptr; collation = uenum_next(collations, &collationLen, &status))
            {
                if (strcmp(collation, "standard") == 0 || strcmp(collation, "search") == 0)
                {
                    // continue does not create holes in ret because i is set outside the loop
                    continue;
                }

                const char *unicodeCollation = uloc_toUnicodeLocaleType("collation", collation);
                const size_t unicodeCollationLen = strlen(unicodeCollation);

                // we only need strlen(unicodeCollation) + 1 char16s because unicodeCollation will always be ASCII (funnily enough)
                char16 *unicodeCollation16 = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, strlen(unicodeCollation) + 1);
                charcount_t unicodeCollation16Len = 0;
                HRESULT hr = utf8::NarrowStringToWideNoAlloc(
                    unicodeCollation,
                    unicodeCollationLen,
                    unicodeCollation16,
                    unicodeCollationLen + 1,
                    &unicodeCollation16Len
                );
                AssertOrFailFastMsg(
                    hr == S_OK && unicodeCollation16Len == unicodeCollationLen && unicodeCollation16Len < MaxCharCount,
                    "Unicode collation char16 conversion was unsuccessful"
                );
                // i + 1 to not ovewrite leading null element
                ret->SetItem(i + 1, JavascriptString::NewWithBuffer(
                    unicodeCollation16,
                    unicodeCollation16Len,
                    scriptContext
                ), PropertyOperationFlags::PropertyOperation_None);
                i++;
            }
        }
        else if (kind == LocaleDataKind::CaseFirst)
        {
            ScopedUCollator collator(ucol_open(localeID, &status));
            UColAttributeValue kf = ucol_getAttribute(collator, UCOL_CASE_FIRST, &status);
            ICU_ASSERT(status, true);
            ret = scriptContext->GetLibrary()->CreateArray(3);

            if (kf == UCOL_OFF)
            {
                ret->SetItem(0, JavascriptString::NewCopySz(_u("false"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(1, JavascriptString::NewCopySz(_u("upper"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(2, JavascriptString::NewCopySz(_u("lower"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
            else if (kf == UCOL_UPPER_FIRST)
            {
                ret->SetItem(0, JavascriptString::NewCopySz(_u("upper"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(1, JavascriptString::NewCopySz(_u("lower"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(2, JavascriptString::NewCopySz(_u("false"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
            else if (kf == UCOL_LOWER_FIRST)
            {
                ret->SetItem(0, JavascriptString::NewCopySz(_u("lower"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(1, JavascriptString::NewCopySz(_u("upper"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(2, JavascriptString::NewCopySz(_u("false"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
        }
        else if (kind == LocaleDataKind::Numeric)
        {
            ScopedUCollator collator(ucol_open(localeID, &status));
            UColAttributeValue kn = ucol_getAttribute(collator, UCOL_NUMERIC_COLLATION, &status);
            ICU_ASSERT(status, true);
            ret = scriptContext->GetLibrary()->CreateArray(2);

            if (kn == UCOL_OFF)
            {
                ret->SetItem(0, JavascriptString::NewCopySz(_u("false"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(1, JavascriptString::NewCopySz(_u("true"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
            else if (kn == UCOL_ON)
            {
                ret->SetItem(0, JavascriptString::NewCopySz(_u("true"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
                ret->SetItem(1, JavascriptString::NewCopySz(_u("false"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
        }
        else if (kind == LocaleDataKind::Calendar)
        {
            ScopedUEnumeration calendars(ucal_getKeywordValuesForLocale("calendar", localeID, false, &status));
            ret = scriptContext->GetLibrary()->CreateArray(uenum_count(calendars, &status));
            ICU_ASSERT(status, true);

            int calendarLen = 0;
            const char *calendar = nullptr;
            int i = 0;
            for (calendar = uenum_next(calendars, &calendarLen, &status); calendar != nullptr; calendar = uenum_next(calendars, &calendarLen, &status))
            {
                const char *unicodeCalendar = uloc_toUnicodeLocaleType("calendar", calendar);
                const size_t unicodeCalendarLen = strlen(unicodeCalendar);

                // we only need strlen(unicodeCalendar) + 1 char16s because unicodeCalendar will always be ASCII (funnily enough)
                char16 *unicodeCalendar16 = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, strlen(unicodeCalendar) + 1);
                charcount_t unicodeCalendar16Len = 0;
                HRESULT hr = utf8::NarrowStringToWideNoAlloc(
                    unicodeCalendar,
                    unicodeCalendarLen,
                    unicodeCalendar16,
                    unicodeCalendarLen + 1,
                    &unicodeCalendar16Len
                );
                AssertOrFailFastMsg(
                    hr == S_OK && unicodeCalendar16Len == unicodeCalendarLen && unicodeCalendar16Len < MaxCharCount,
                    "Unicode calendar char16 conversion was unsuccessful"
                );
                ret->SetItem(i, JavascriptString::NewWithBuffer(
                    unicodeCalendar16,
                    static_cast<charcount_t>(unicodeCalendar16Len),
                    scriptContext
                ), PropertyOperationFlags::PropertyOperation_None);
                i++;
            }
        }
        else if (kind == LocaleDataKind::NumberingSystem)
        {
            // unumsys_openAvailableNames has multiple bugs (http://bugs.icu-project.org/trac/ticket/11908) and also
            // does not provide a locale-specific set of numbering systems
            // the Intl spec provides a list of required numbering systems to support in #table-numbering-system-digits
            // For now, assume that all of those numbering systems are supported, and just get the default using unumsys_open
            // unumsys_open will also ensure that "native", "traditio", and "finance" are not returned, as per #sec-intl.datetimeformat-internal-slots
            static const char16 *available[] = {
                _u("arab"),
                _u("arabext"),
                _u("bali"),
                _u("beng"),
                _u("deva"),
                _u("fullwide"),
                _u("gujr"),
                _u("guru"),
                _u("hanidec"),
                _u("khmr"),
                _u("knda"),
                _u("laoo"),
                _u("latn"),
                _u("limb"),
                _u("mlym"),
                _u("mong"),
                _u("mymr"),
                _u("orya"),
                _u("tamldec"),
                _u("telu"),
                _u("thai"),
                _u("tibt")
            };

            ScopedUNumberingSystem numsys(unumsys_open(localeID, &status));
            ICU_ASSERT(status, true);
            utf8::NarrowToWide numsysName(unumsys_getName(numsys));

            ret = scriptContext->GetLibrary()->CreateArray(_countof(available) + 1);
            ret->SetItem(0, JavascriptString::NewCopySz(numsysName, scriptContext), PropertyOperationFlags::PropertyOperation_None);
            for (int i = 0; i < _countof(available); i++)
            {
                ret->SetItem(i + 1, JavascriptString::NewCopySz(available[i], scriptContext), PropertyOperationFlags::PropertyOperation_None);
            }
        }
        else if (kind == LocaleDataKind::HourCycle)
        {
            // #sec-intl.datetimeformat-internal-slots: "[[LocaleData]][locale].hc must be < null, h11, h12, h23, h24 > for all locale values"
            ret = scriptContext->GetLibrary()->CreateArray(5);
            ret->SetItem(0, scriptContext->GetLibrary()->GetNull(), PropertyOperationFlags::PropertyOperation_None);
            ret->SetItem(1, JavascriptString::NewCopySz(_u("h11"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            ret->SetItem(2, JavascriptString::NewCopySz(_u("h12"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            ret->SetItem(3, JavascriptString::NewCopySz(_u("h23"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
            ret->SetItem(4, JavascriptString::NewCopySz(_u("h24"), scriptContext), PropertyOperationFlags::PropertyOperation_None);
        }
        else
        {
            AssertOrFailFastMsg(false, "GetLocaleData called with unknown kind parameter");
        }

        return ret;
#else
        AssertOrFailFastMsg(false, "Intl with Windows Globalization should never call GetLocaleData");
        return nullptr;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_ResolveLocaleLookup(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        if (args.Info.Count < 2 || !JavascriptString::Is(args.Values[1]))
        {
            // ResolveLocaleLookup of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

#if defined(INTL_ICU)
#if defined(INTL_ICU_DEBUG)
        Output::Print(_u("Intl::ResolveLocaleLookup returned false: EntryIntl_ResolveLocaleLookup returning null to fallback to JS\n"));
#endif
        return scriptContext->GetLibrary()->GetNull();
#else
        JavascriptString *argString = JavascriptString::FromVar(args.Values[1]);
        PCWSTR passedLocale = argString->GetSz();
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

#if defined(INTL_ICU)
        AssertOrFailFastMsg(false, "Intl-ICU does not implement ResolveLocaleBestFit");
        return nullptr;
#else // !INTL_ICU
        JavascriptString *localeStrings = JavascriptString::FromVar(args.Values[1]);
        PCWSTR passedLocale = localeStrings->GetSz();
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

#ifdef INTL_WINGLOB
        char16 defaultLocale[LOCALE_NAME_MAX_LENGTH];
        defaultLocale[0] = '\0';

        if (GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) == 0)
        {
            JavascriptError::MapAndThrowError(scriptContext, HRESULT_FROM_WIN32(GetLastError()));
        }

        return JavascriptString::NewCopySz(defaultLocale, scriptContext);
#else
        UErrorCode status = U_ZERO_ERROR;
        char defaultLangtag[ULOC_FULLNAME_CAPACITY] = { 0 };
        char defaultLocaleID[ULOC_FULLNAME_CAPACITY] = { 0 };

        int localeIDActual = uloc_getName(nullptr, defaultLocaleID, _countof(defaultLocaleID), &status);
        ICU_ASSERT(status, localeIDActual > 0 && localeIDActual < _countof(defaultLocaleID));

        int langtagActual = uloc_toLanguageTag(defaultLocaleID, defaultLangtag, _countof(defaultLangtag), true, &status);
        ICU_ASSERT(status, langtagActual > 0 && langtagActual < _countof(defaultLangtag));

        char16 *defaultLangtag16 = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, langtagActual + 1);
        charcount_t defaultLangtag16Actual = 0;
        utf8::NarrowStringToWideNoAlloc(defaultLangtag, static_cast<size_t>(langtagActual), defaultLangtag16, langtagActual + 1, &defaultLangtag16Actual);
        AssertOrFailFastMsg(defaultLangtag16Actual == static_cast<size_t>(langtagActual), "Language tags should always be ASCII");
        return JavascriptString::NewWithBuffer(defaultLangtag16, defaultLangtag16Actual, scriptContext);
#endif
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
        AssertOrFailFastMsg(false, "ICU should not be calling platform.getExtensions");
        return nullptr;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CacheNumberFormat(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && DynamicObject::Is(args.Values[1]));

        Var propertyValue = nullptr; // set by the GetTypedPropertyBuiltInFrom macro

#if defined(INTL_ICU)
        DynamicObject *state = DynamicObject::UnsafeFromVar(args.Values[1]);

        // always AssertOrFailFast that the properties we need are there, because if they aren't, Intl.js isn't functioning correctly
        AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, formatterToUse, TaggedInt));
        NumberFormatStyle style = NumberFormatStyle::Default;
        int formatterToUse = TaggedInt::ToInt32(propertyValue);
        AssertOrFailFast(formatterToUse >= 0 || formatterToUse < (int) NumberFormatStyle::Max);
        style = static_cast<NumberFormatStyle>(formatterToUse);

        UNumberFormatStyle unumStyle = UNUM_IGNORE;
        UErrorCode status = U_ZERO_ERROR;
        const char16 *currency = nullptr;

        AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, locale, JavascriptString));
        JavascriptString *langtag = JavascriptString::UnsafeFromVar(propertyValue);
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        BCP47_TO_ICU(langtag->GetSz(), langtag->GetLength(), localeID, ULOC_FULLNAME_CAPACITY);

        if (style == NumberFormatStyle::Decimal)
        {
            unumStyle = UNUM_DECIMAL;
        }
        else if (style == NumberFormatStyle::Percent)
        {
            unumStyle = UNUM_PERCENT;
        }
        else if (style == NumberFormatStyle::Currency)
        {
            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, currencyDisplayToUse, TaggedInt));
            NumberFormatCurrencyDisplay nfcd = NumberFormatCurrencyDisplay::Default;
            int currencyDisplayToUse = TaggedInt::ToInt32(propertyValue);
            AssertOrFailFast(currencyDisplayToUse >= 0 || currencyDisplayToUse < (int) NumberFormatCurrencyDisplay::Max);
            nfcd = static_cast<NumberFormatCurrencyDisplay>(currencyDisplayToUse);
            if (nfcd == NumberFormatCurrencyDisplay::Symbol)
            {
                unumStyle = UNUM_CURRENCY;
            }
            else if (nfcd == NumberFormatCurrencyDisplay::Code)
            {
                unumStyle = UNUM_CURRENCY_ISO;
            }
            else if (nfcd == NumberFormatCurrencyDisplay::Name)
            {
                unumStyle = UNUM_CURRENCY_PLURAL;
            }

            if (GetTypedPropertyBuiltInFrom(state, currency, JavascriptString))
            {
                currency = JavascriptString::UnsafeFromVar(propertyValue)->GetSz();
            }
        }

        AssertOrFailFast(unumStyle != UNUM_IGNORE);

        auto fmt = FinalizableUNumberFormat::New(scriptContext->GetRecycler(), unum_open(unumStyle, nullptr, 0, localeID, nullptr, &status));
        ICU_ASSERT(status, true);

        AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, useGrouping, JavascriptBoolean));
        unum_setAttribute(*fmt, UNUM_GROUPING_USED, JavascriptBoolean::UnsafeFromVar(propertyValue)->GetValue());

        unum_setAttribute(*fmt, UNUM_ROUNDING_MODE, UNUM_ROUND_HALFUP);

        if (HasPropertyBuiltInOn(state, minimumSignificantDigits))
        {
            unum_setAttribute(*fmt, UNUM_SIGNIFICANT_DIGITS_USED, true);

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, minimumSignificantDigits, TaggedInt));
            unum_setAttribute(*fmt, UNUM_MIN_SIGNIFICANT_DIGITS, TaggedInt::ToInt32(propertyValue));

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, maximumSignificantDigits, TaggedInt));
            unum_setAttribute(*fmt, UNUM_MAX_SIGNIFICANT_DIGITS, TaggedInt::ToInt32(propertyValue));
        }
        else
        {
            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, minimumIntegerDigits, TaggedInt));
            unum_setAttribute(*fmt, UNUM_MIN_INTEGER_DIGITS, TaggedInt::ToInt32(propertyValue));

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, minimumFractionDigits, TaggedInt));
            unum_setAttribute(*fmt, UNUM_MIN_FRACTION_DIGITS, TaggedInt::ToInt32(propertyValue));

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, maximumFractionDigits, TaggedInt));
            unum_setAttribute(*fmt, UNUM_MAX_FRACTION_DIGITS, TaggedInt::ToInt32(propertyValue));
        }

        if (currency != nullptr)
        {
            unum_setTextAttribute(*fmt, UNUM_CURRENCY_CODE, reinterpret_cast<const UChar *>(currency), -1, &status);
            ICU_ASSERT(status, true);
        }

        state->SetInternalProperty(
            InternalPropertyIds::HiddenObject,
            fmt,
            PropertyOperationFlags::PropertyOperation_None,
            nullptr
        );

        return scriptContext->GetLibrary()->GetUndefined();
#else
        HRESULT hr = S_OK;
        JavascriptString *localeJSstr = nullptr;
        DynamicObject *options = DynamicObject::FromVar(args.Values[1]);
        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        // Verify locale is present
        // REVIEW (doilij): Fix comparison of the unsigned value <= 0
        if (!GetTypedPropertyBuiltInFrom(options, __locale, JavascriptString) || (localeJSstr = JavascriptString::FromVar(propertyValue))->GetLength() <= 0)
        {
            // REVIEW (doilij): Should we throw? Or otherwise, from Intl.js, should detect something didn't work right here...
            return scriptContext->GetLibrary()->GetUndefined();
        }

        //First we have to determine which formatter(number, percent, or currency) we will be using.
        //Note some options might not be present.
        AutoCOMPtr<NumberFormatting::INumberFormatter> numberFormatter(nullptr);
        PCWSTR locale = localeJSstr->GetSz();

        uint16 formatterToUseVal = 0; // 0 (default) is number, 1 is percent, 2 is currency
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

#ifdef INTL_WINGLOB
    static DWORD GetCompareStringComparisonFlags(CollatorSensitivity sensitivity, bool ignorePunctuation, bool numeric)
    {
        DWORD flags = 0;

        if (sensitivity == CollatorSensitivity::Base)
        {
            flags |= LINGUISTIC_IGNOREDIACRITIC | LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH;
        }
        else if (sensitivity == CollatorSensitivity::Accent)
        {
            flags |= LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH;
        }
        else if (sensitivity == CollatorSensitivity::Case)
        {
            flags |= NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LINGUISTIC_IGNOREDIACRITIC;
        }
        else if (sensitivity == CollatorSensitivity::Variant)
        {
            flags |= NORM_LINGUISTIC_CASING;
        }

        if (ignorePunctuation)
        {
            flags |= NORM_IGNORESYMBOLS;
        }

        if (numeric)
        {
            flags |= SORT_DIGITSASNUMBERS;
        }

        return flags;
    }
#endif

#ifdef INTL_ICU
    static int CompareStringICU(_In_opt_count_(langtagLen) const char16 *langtag, _In_ charcount_t langtagLen, _In_z_ const char16 *left, _In_ charcount_t cchLeft, _In_z_ const char16 *right, _In_ charcount_t cchRight,
        _In_ CollatorSensitivity sensitivity, _In_ bool ignorePunctuation, _In_ bool numeric, _In_ CollatorCaseFirst caseFirst, _Out_ HRESULT *hr)
    {
        ASSERT_ENUM(CollatorSensitivity, sensitivity);
        ASSERT_ENUM(CollatorCaseFirst, caseFirst);
        Assert(left != nullptr && right != nullptr && hr != nullptr);
        int ret = 0;
        *hr = E_INVALIDARG;
        UErrorCode error = U_ZERO_ERROR;

        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int32_t length = 0;
        if (langtag == nullptr)
        {
            length = uloc_getName(nullptr, localeID, _countof(localeID), &error);
        }
        else
        {
            char langtag8[ULOC_FULLNAME_CAPACITY] = { 0 };
            AssertOrFailFast(utf8::WideStringToNarrowNoAlloc(langtag, langtagLen, langtag8, _countof(langtag8)) == S_OK);
            uloc_forLanguageTag(langtag8, localeID, _countof(localeID), &length, &error);
        }
        ICU_ASSERT(error, length > 0);

        ScopedUCollator collator(ucol_open(localeID, &error));
        ICU_ASSERT(error, true);

        if (sensitivity == CollatorSensitivity::Base)
        {
            ucol_setStrength(collator, UCOL_PRIMARY);
        }
        else if (sensitivity == CollatorSensitivity::Accent)
        {
            ucol_setStrength(collator, UCOL_SECONDARY);
        }
        else if (sensitivity == CollatorSensitivity::Case)
        {
            // see "description" for the caseLevel default option: http://userguide.icu-project.org/collation/customization
            ucol_setStrength(collator, UCOL_PRIMARY);
            ucol_setAttribute(collator, UCOL_CASE_LEVEL, UCOL_ON, &error);
            ICU_ASSERT(error, true);
        }
        else if (sensitivity == CollatorSensitivity::Variant)
        {
            ucol_setStrength(collator, UCOL_TERTIARY);
        }
        else
        {
            AssertOrFailFastMsg(false, "sensitivity is not one of the CollatorSensitivity values");
        }

        if (ignorePunctuation)
        {
            // see http://userguide.icu-project.org/collation/customization/ignorepunct
            ucol_setAttribute(collator, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &error);
            ICU_ASSERT(error, true);
        }

        if (numeric)
        {
            ucol_setAttribute(collator, UCOL_NUMERIC_COLLATION, UCOL_ON, &error);
            ICU_ASSERT(error, true);
        }

        if (caseFirst == CollatorCaseFirst::Upper)
        {
            ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &error);
            ICU_ASSERT(error, true);
        }
        else if (caseFirst == CollatorCaseFirst::Lower)
        {
            ucol_setAttribute(collator, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &error);
            ICU_ASSERT(error, true);
        }

        *hr = S_OK;
        UCollationResult result = ucol_strcoll(collator, reinterpret_cast<const UChar *>(left), cchLeft, reinterpret_cast<const UChar *>(right), cchRight);
        if (result == UCOL_LESS)
        {
            ret = 1;
        }
        else if (result == UCOL_EQUAL)
        {
            ret = 2;
        }
        else if (result == UCOL_GREATER)
        {
            ret = 3;
        }
        else
        {
            *hr = E_FAIL;
            ret = 0;
        }

        return ret;
    }
#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CompareString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count >= 3 && JavascriptString::Is(args[1]) && JavascriptString::Is(args[2]));

#ifdef INTL_WINGLOB
        const char16 *locale = nullptr; // args[3]
        char16 defaultLocale[LOCALE_NAME_MAX_LENGTH] = { 0 };
#else
        const char16 *langtag = nullptr;
        charcount_t langtagLen = 0;
#endif
        CollatorSensitivity sensitivity = CollatorSensitivity::Default; // args[4]
        bool ignorePunctuation = false; // args[5]
        bool numeric = false; // args[6]

        JavascriptString *str1 = JavascriptString::FromVar(args.Values[1]);
        JavascriptString *str2 = JavascriptString::FromVar(args.Values[2]);
        CollatorCaseFirst caseFirst = CollatorCaseFirst::Default; // args[7]

        // we only need to parse arguments 3 through 7 if locale and options are provided
        // see fast path in JavascriptString::EntryLocaleCompare
        if (args.Info.Count > 3)
        {
            if (args.Info.Count < 8)
            {
                JavascriptError::MapAndThrowError(scriptContext, E_INVALIDARG);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[3]) && JavascriptString::Is(args.Values[3]))
            {
#ifdef INTL_WINGLOB
                locale = JavascriptString::FromVar(args.Values[3])->GetSz();
#else
                langtag = JavascriptString::UnsafeFromVar(args[3])->GetSz();
                langtagLen = JavascriptString::UnsafeFromVar(args[3])->GetLength();
#endif
            }
            else
            {
                JavascriptError::MapAndThrowError(scriptContext, E_INVALIDARG);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[4]) && TaggedInt::Is(args.Values[4]))
            {
                sensitivity = static_cast<CollatorSensitivity>(TaggedInt::ToUInt16(args.Values[4]));
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[5]) && JavascriptBoolean::Is(args.Values[5]))
            {
                ignorePunctuation = (JavascriptBoolean::FromVar(args.Values[5])->GetValue() != 0);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[6]) && JavascriptBoolean::Is(args.Values[6]))
            {
                numeric = (JavascriptBoolean::FromVar(args.Values[6])->GetValue() != 0);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[7]) && TaggedInt::Is(args.Values[7]))
            {
                caseFirst = static_cast<CollatorCaseFirst>(TaggedInt::ToUInt16(args.Values[7]));
            }
        }
        else
        {
#ifdef INTL_WINGLOB
            if (GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) != 0)
            {
                locale = defaultLocale;
            }
            else
            {
                JavascriptError::MapAndThrowError(scriptContext, HRESULT_FROM_WIN32(GetLastError()));
            }
#endif
        }

#ifdef INTL_WINGLOB
        Assert(locale != nullptr);
#endif
        Assert((int)sensitivity >= 0 && sensitivity < CollatorSensitivity::Max);
        Assert((int)caseFirst >= 0 && caseFirst < CollatorCaseFirst::Max);

        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("EntryIntl_CompareString"));

        // TODO(jahorto): Investigate using ICU's built-in in-line normalization techniques if possible.
        const char16 *left = nullptr;
        charcount_t leftLen = 0;
        if (UnicodeText::IsNormalizedString(UnicodeText::NormalizationForm::C, str1->GetSz(), str1->GetLength()))
        {
            left = str1->GetSz();
            leftLen = str1->GetLength();
        }
        else
        {
            left = str1->GetNormalizedString(UnicodeText::NormalizationForm::C, tempAllocator, leftLen);
        }

        const char16 *right = nullptr;
        charcount_t rightLen = 0;
        if (UnicodeText::IsNormalizedString(UnicodeText::NormalizationForm::C, str2->GetSz(), str2->GetLength()))
        {
            right = str2->GetSz();
            rightLen = str2->GetLength();
        }
        else
        {
            right = str2->GetNormalizedString(UnicodeText::NormalizationForm::C, tempAllocator, rightLen);
        }

        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);

        // CompareStringEx on Windows returns 0 for error, 1 if less, 2 if equal, 3 if greater
        // Default to the strings being equal, because sorting with == causes no change in the order but converges, whereas < would cause an infinite loop.
        int compareResult = 2;
        HRESULT error = S_OK;
#if defined(INTL_WINGLOB)
        DWORD comparisonFlags = GetCompareStringComparisonFlags(sensitivity, ignorePunctuation, numeric);
        compareResult = CompareStringEx(locale, comparisonFlags, left, leftLen, right, rightLen, NULL, NULL, 0);
        error = HRESULT_FROM_WIN32(GetLastError());
#elif defined(INTL_ICU)
        compareResult = CompareStringICU(
            langtag,
            langtagLen,
            left,
            leftLen,
            right,
            rightLen,
            sensitivity,
            ignorePunctuation,
            numeric,
            caseFirst,
            &error
        );
#elif !_WIN32
        compareResult = wcsncmp(aLeft, aRight, min(size1, size2));
        if (compareResult == 0 && size1 != size2)
        {
            compareResult = size1 > size2 ? 1 : -1;
        }

        // return early because wcsncmp has a different return value format that CompareStringEx/CollatorCompare
        return JavascriptNumber::ToVar(compareResult, scriptContext);
#endif

        if (compareResult == 0)
        {
            JavascriptError::MapAndThrowError(scriptContext, error);
        }

        return JavascriptNumber::ToVar(compareResult - 2, scriptContext);
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CurrencyDigits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        INTL_CHECK_ARGS(
            args.Info.Count == 2 &&
            JavascriptString::Is(args.Values[1])
        );

        const char16 *currencyCode = JavascriptString::UnsafeFromVar(args.Values[1])->GetSz();

#if defined(INTL_ICU)
        UErrorCode status = U_ZERO_ERROR;
        ScopedUNumberFormat fmt(unum_open(UNUM_CURRENCY, nullptr, 0, nullptr, nullptr, &status));
        unum_setTextAttribute(fmt, UNUM_CURRENCY_CODE, reinterpret_cast<const UChar *>(currencyCode), -1, &status);
        ICU_ASSERT(status, true);

        int currencyDigits = unum_getAttribute(fmt, UNUM_FRACTION_DIGITS);
        return JavascriptNumber::ToVar(currencyDigits, scriptContext);
#else
        HRESULT hr;
        AutoCOMPtr<NumberFormatting::ICurrencyFormatter> currencyFormatter(nullptr);
        IfFailThrowHr(GetWindowsGlobalizationAdapter(scriptContext)->CreateCurrencyFormatterCode(scriptContext, currencyCode, &currencyFormatter));
        AutoCOMPtr<NumberFormatting::INumberFormatterOptions> numberFormatterOptions;
        IfFailThrowHr(currencyFormatter->QueryInterface(__uuidof(NumberFormatting::INumberFormatterOptions), reinterpret_cast<void**>(&numberFormatterOptions)));
        Assert(numberFormatterOptions);
        INT32 fractionDigits;
        IfFailThrowHr(numberFormatterOptions->get_FractionDigits(&fractionDigits));
        return JavascriptNumber::ToVar(fractionDigits, scriptContext);
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

        INTL_CHECK_ARGS(
            args.Info.Count == 3 &&
            (TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) &&
            DynamicObject::Is(args.Values[2])
        );

#if defined(INTL_ICU)
        DynamicObject *state = DynamicObject::UnsafeFromVar(args.Values[2]);
        Var cachedFormatter = nullptr; // cached by EntryIntl_CacheNumberFormat
        AssertOrFailFast(state->GetInternalProperty(state, Js::InternalPropertyIds::HiddenObject, &cachedFormatter, NULL, scriptContext));

        auto fmt = static_cast<FinalizableUNumberFormat *>(cachedFormatter);
        char16 *formatted = nullptr;
        int formattedLen = 0;

        if (TaggedInt::Is(args.Values[1]))
        {
            int num = TaggedInt::ToInt32(args.Values[1]);
            RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
                return unum_format(*fmt, num, buf, bufLen, nullptr, status);
            }, scriptContext->GetRecycler(), &formatted, &formattedLen);
        }
        else
        {
            double num = JavascriptNumber::GetValue(args.Values[1]);
            RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
                return unum_formatDouble(*fmt, num, buf, bufLen, nullptr, status);
            }, scriptContext->GetRecycler(), &formatted, &formattedLen);
        }

        return JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext);;
#else
        DynamicObject *options = DynamicObject::FromVar(args.Values[2]);
        Var hiddenObject = nullptr;
        AssertOrFailFastMsg(options->GetInternalProperty(options, Js::InternalPropertyIds::HiddenObject, &hiddenObject, NULL, scriptContext),
            "EntryIntl_FormatNumber: Could not retrieve hiddenObject.");
        DelayLoadWindowsGlobalization* wsl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();

        NumberFormatting::INumberFormatter *numberFormatter;
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

        if (strBuf == nullptr)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }
        else
        {
            JavascriptStringObject *retVal = scriptContext->GetLibrary()->CreateStringObject(Js::JavascriptString::NewCopySz(strBuf, scriptContext));
            return retVal;
        }
#endif
    }

#ifdef INTL_ICU
    static void AddPartToPartsArray(ScriptContext *scriptContext, JavascriptArray *arr, int arrIndex, const char16 *src, int start, int end, const char16 *kind)
    {
        JavascriptString *partValue = JavascriptString::NewCopyBuffer(
            src + start,
            end - start,
            scriptContext
        );

        JavascriptString *partType = JavascriptString::NewCopySz(kind, scriptContext);

        DynamicObject* part = scriptContext->GetLibrary()->CreateObject();
        JavascriptOperators::InitProperty(part, PropertyIds::type, partType);
        JavascriptOperators::InitProperty(part, PropertyIds::value, partValue);

        arr->SetItem(arrIndex, part, PropertyOperationFlags::PropertyOperation_None);
    }
#endif

    // For Windows Globalization, this function takes a number (date) and a state object and returns a string
    // For ICU, this function takes a state object, number, and boolean for whether or not to format to parts.
    //   if args.Values[3] ~= true, an array of objects is returned; else, a string is returned
    Var IntlEngineInterfaceExtensionObject::EntryIntl_FormatDateTime(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#ifdef INTL_WINGLOB
        if (args.Info.Count < 3 || !(TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) || !DynamicObject::Is(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

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
        Var hiddenObject = nullptr;
        AssertOrFailFastMsg(obj->GetInternalProperty(obj, Js::InternalPropertyIds::HiddenObject, &hiddenObject, NULL, scriptContext),
            "EntryIntl_FormatDateTime: Could not retrieve hiddenObject.");

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
        INTL_CHECK_ARGS(
            args.Info.Count == 4 &&
            DynamicObject::Is(args.Values[1]) &&
            (TaggedInt::Is(args.Values[2]) || JavascriptNumber::Is(args.Values[2])) &&
            JavascriptBoolean::Is(args.Values[3])
        );

        DynamicObject *state = DynamicObject::UnsafeFromVar(args.Values[1]);
        double date = TaggedInt::Is(args.Values[2]) ? TaggedInt::ToDouble(args.Values[2]) : JavascriptNumber::GetValue(args.Values[2]);
        bool toParts = Js::JavascriptBoolean::UnsafeFromVar(args.Values[3])->GetValue() ? true : false;

        // Below, we lazy-initialize the backing UDateFormat on the first call to format{ToParts}
        // On subsequent calls, the UDateFormat will be cached in state.hiddenObject
        // TODO(jahorto): Make these property IDs sane, so that hiddenObject doesn't have different meanings in different contexts
        Var hiddenObject = nullptr;
        FinalizableUDateFormat *dtf = nullptr;
        UErrorCode status = U_ZERO_ERROR;
        if (state->GetInternalProperty(state, Js::InternalPropertyIds::HiddenObject, &hiddenObject, nullptr, scriptContext))
        {
            dtf = reinterpret_cast<FinalizableUDateFormat *>(hiddenObject);
        }
        else
        {
            JavascriptString *langtag = nullptr;
            JavascriptString *timeZone = nullptr;
            JavascriptString *pattern = nullptr;

            Var propertyValue = nullptr; // set by the GetTypedPropertyBuiltInFrom macro

            // the object key is locale for legacy compat, but its more accurately a BCP47 Language Tag,
            // not an ICU LocaleID
            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, locale, JavascriptString));
            langtag = JavascriptString::UnsafeFromVar(propertyValue);

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, timeZone, JavascriptString));
            timeZone = JavascriptString::UnsafeFromVar(propertyValue);

            AssertOrFailFast(GetTypedPropertyBuiltInFrom(state, pattern, JavascriptString));
            pattern = JavascriptString::UnsafeFromVar(propertyValue);

            char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
            BCP47_TO_ICU(langtag->GetSz(), langtag->GetLength(), localeID, _countof(localeID));

            dtf = FinalizableUDateFormat::New(scriptContext->GetRecycler(), udat_open(
                UDAT_PATTERN,
                UDAT_PATTERN,
                localeID,
                reinterpret_cast<const UChar *>(timeZone->GetSz()),
                timeZone->GetLength(),
                reinterpret_cast<const UChar *>(pattern->GetSz()),
                pattern->GetLength(),
                &status
            ));
            ICU_ASSERT(status, true);

            // DateTimeFormat is expected to use the "proleptic Gregorian calendar", which means that the Julian calendar should never be used.
            // To accomplish this, we can set the switchover date between julian/gregorian
            // to the ECMAScript beginning of time, which is -8.64e15 according to ecma262 #sec-time-values-and-time-range
            UCalendar *cal = const_cast<UCalendar *>(udat_getCalendar(*dtf));
            ucal_setGregorianChange(cal, -8.64e15, &status);

            // status can be U_UNSUPPORTED_ERROR if the calendar isn't gregorian, which
            // there does not seem to be a way to check for ahead of time in the C API
            AssertOrFailFastMsg(U_SUCCESS(status) || status == U_UNSUPPORTED_ERROR, ICU_ERRORMESSAGE(status));

            // cache dtf for later use (so that the condition that brought us here returns true for future calls)
            state->SetInternalProperty(
                InternalPropertyIds::HiddenObject,
                dtf,
                PropertyOperationFlags::PropertyOperation_None,
                nullptr
            );
        }

        char16 *formatted = nullptr;
        int formattedLen = 0;
        if (!toParts)
        {
            // if we aren't formatting to parts, we simply want to call udat_format with retry
            RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
                return udat_format(*dtf, date, buf, bufLen, nullptr, status);
            }, scriptContext->GetRecycler(), &formatted, &formattedLen);
            return JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext);
        }

        ScopedUFieldPositionIterator fpi(ufieldpositer_open(&status));
        ICU_ASSERT(status, true);
        RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
            return udat_formatForFields(*dtf, date, buf, bufLen, fpi, status);
        }, scriptContext->GetRecycler(), &formatted, &formattedLen);

        JavascriptArray* ret = scriptContext->GetLibrary()->CreateArray(0);

        int partStart = 0;
        int partEnd = 0;
        int lastPartEnd = 0;
        for (
            int kind = ufieldpositer_next(fpi, &partStart, &partEnd), i = 0;
            kind > 0;
            kind = ufieldpositer_next(fpi, &partStart, &partEnd), ++i
            )
        {
            Assert(partStart < partEnd && partEnd <= formattedLen);
            const char16 *typeString = nullptr;
            UDateFormatField fieldKind = (UDateFormatField)kind;
            switch (fieldKind)
            {
            case UDAT_ERA_FIELD:
                typeString = _u("era"); break;
            case UDAT_YEAR_FIELD:
            case UDAT_EXTENDED_YEAR_FIELD:
            case UDAT_YEAR_NAME_FIELD:
                typeString = _u("year"); break;
            case UDAT_MONTH_FIELD:
            case UDAT_STANDALONE_MONTH_FIELD:
                typeString = _u("month"); break;
            case UDAT_DATE_FIELD:
                typeString = _u("day"); break;
            case UDAT_HOUR_OF_DAY1_FIELD:
            case UDAT_HOUR_OF_DAY0_FIELD:
            case UDAT_HOUR1_FIELD:
            case UDAT_HOUR0_FIELD:
                typeString = _u("hour"); break;
            case UDAT_MINUTE_FIELD:
                typeString = _u("minute"); break;
            case UDAT_SECOND_FIELD:
                typeString = _u("second"); break;
            case UDAT_DAY_OF_WEEK_FIELD:
            case UDAT_STANDALONE_DAY_FIELD:
            case UDAT_DOW_LOCAL_FIELD:
                typeString = _u("weekday"); break;
            case UDAT_AM_PM_FIELD:
                typeString = _u("dayPeriod"); break;
            case UDAT_TIMEZONE_FIELD:
            case UDAT_TIMEZONE_RFC_FIELD:
            case UDAT_TIMEZONE_GENERIC_FIELD:
            case UDAT_TIMEZONE_SPECIAL_FIELD:
            case UDAT_TIMEZONE_LOCALIZED_GMT_OFFSET_FIELD:
            case UDAT_TIMEZONE_ISO_FIELD:
                typeString = _u("timeZoneName"); break;
            default:
                typeString = _u("unknown"); break;
            }

            if (partStart > lastPartEnd)
            {
                // formatForFields does not report literal fields directly, so we have to detect them
                // by seeing if the current part starts after the previous one ended
                AddPartToPartsArray(scriptContext, ret, i, formatted, lastPartEnd, partStart, _u("literal"));
                i += 1;
            }

            AddPartToPartsArray(scriptContext, ret, i, formatted, partStart, partEnd, typeString);
            lastPartEnd = partEnd;
        }

        return ret;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetPatternForSkeleton(RecyclableObject *function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 3 && JavascriptString::Is(args.Values[1]) && JavascriptString::Is(args.Values[2]));

        JavascriptString *langtag = JavascriptString::UnsafeFromVar(args.Values[1]);
        JavascriptString *skeleton = JavascriptString::UnsafeFromVar(args.Values[2]);
        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        BCP47_TO_ICU(langtag->GetSz(), langtag->GetLength(), localeID, _countof(localeID));

        ScopedUDateTimePatternGenerator dtpg(udatpg_open(localeID, &status));
        ICU_ASSERT(status, true);

        char16 *formatted = nullptr;
        int formattedLen = 0;
        RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
            return udatpg_getBestPatternWithOptions(
                dtpg,
                reinterpret_cast<const UChar *>(skeleton->GetSz()),
                skeleton->GetLength(),
                UDATPG_MATCH_ALL_FIELDS_LENGTH,
                buf,
                bufLen,
                status
            );
        }, scriptContext->GetRecycler(), &formatted, &formattedLen);

        return JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext);
#else
        AssertOrFailFastMsg(false, "GetPatternForSkeleton is not implemented outside of ICU");
        return nullptr;
#endif
    }

    // given a timezone name as an argument, this will return a canonicalized version of that name, or undefined if the timezone is invalid
    Var IntlEngineInterfaceExtensionObject::EntryIntl_ValidateAndCanonicalizeTimeZone(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && JavascriptString::Is(args.Values[1]));

        JavascriptString *tz = JavascriptString::FromVar(args.Values[1]);

#ifdef INTL_WINGLOB
        AutoHSTRING canonicalizedTimeZone;
        boolean isValidTimeZone = GetWindowsGlobalizationAdapter(scriptContext)->ValidateAndCanonicalizeTimeZone(scriptContext, tz->GetSz(), &canonicalizedTimeZone);
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
        UErrorCode status = U_ZERO_ERROR;

        // TODO(jahorto): Is this the list of timeZones that we want? How is this different from
        // the other two enum values or ucal_openTimeZones?
        ScopedUEnumeration available(ucal_openTimeZoneIDEnumeration(UCAL_ZONE_TYPE_ANY, nullptr, nullptr, &status));
        int availableLength = uenum_count(available, &status);
        ICU_ASSERT(status, availableLength > 0);

        charcount_t matchLen = 0;
        UChar match[100];
        for (int a = 0; a < availableLength; ++a)
        {
            int curLen = -1;
            const UChar *cur = uenum_unext(available, &curLen, &status);
            ICU_ASSERT(status, curLen > 0);
            if (_wcsicmp(reinterpret_cast<const char16 *>(cur), tz->GetSz()) == 0)
            {
                ucal_getCanonicalTimeZoneID(cur, curLen, match, _countof(match), nullptr, &status);
                ICU_ASSERT(status, true);
                size_t len = wcslen(reinterpret_cast<const char16 *>(match));
                AssertMsg(len < MaxCharCount, "Returned canonicalized timezone is far too long");
                matchLen = static_cast<charcount_t>(len);
                break;
            }
        }

        if (matchLen == 0)
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        return JavascriptString::NewCopyBuffer(reinterpret_cast<const char16 *>(match), matchLen, scriptContext);
#endif
    }

    // returns the current system time zone
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
        int timeZoneLen = 0;
        char16 *timeZone = nullptr;
        RecyclerExecuteICUWithRetry([&](UChar *buf, int bufLen, UErrorCode *status) {
            return ucal_getDefaultTimeZone(buf, bufLen, status);
        }, scriptContext->GetRecycler(), &timeZone, &timeZoneLen);
        return JavascriptString::NewWithBuffer(timeZone, timeZoneLen, scriptContext);
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
        // Don't put this in a header or add it to the namespace even in this file. Keep it to the minimum scope needed.
        enum class IntlBuiltInFunctionID : int32 {
            Min = 0,
            DateToLocaleString = Min,
            DateToLocaleDateString,
            DateToLocaleTimeString,
            NumberToLocaleString,
            StringLocaleCompare,
            Max
        };

        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        // This function will only be used during the construction of the Intl object, hence Asserts are in place.
        Assert(args.Info.Count >= 3 && JavascriptFunction::Is(args.Values[1]) && TaggedInt::Is(args.Values[2]));

        JavascriptFunction *func = JavascriptFunction::FromVar(args.Values[1]);
        int32 id = TaggedInt::ToInt32(args.Values[2]);
        Assert(id >= (int32)IntlBuiltInFunctionID::Min && id < (int32)IntlBuiltInFunctionID::Max);

        EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
        IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));

        IntlBuiltInFunctionID functionID = static_cast<IntlBuiltInFunctionID>(id);
        switch (functionID)
        {
        case IntlBuiltInFunctionID::DateToLocaleString:
            extensionObject->dateToLocaleString = func;
            break;
        case IntlBuiltInFunctionID::DateToLocaleDateString:
            extensionObject->dateToLocaleDateString = func;
            break;
        case IntlBuiltInFunctionID::DateToLocaleTimeString:
            extensionObject->dateToLocaleTimeString = func;
            break;
        case IntlBuiltInFunctionID::NumberToLocaleString:
            extensionObject->numberToLocaleString = func;
            break;
        case IntlBuiltInFunctionID::StringLocaleCompare:
            extensionObject->stringLocaleCompare = func;
            break;
        default:
            AssertMsg(false, "functionID was not one of the allowed values. The previous assert should catch this.");
            break;
        }

        // Don't need to return anything
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
        Var hiddenObject = nullptr;
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

        AssertOrFailFastMsg(func != scriptContext->GetLibrary()->GetUndefined(), "Trying to callInstanceFunction(undefined, ...)");

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
