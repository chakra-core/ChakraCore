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

#if defined(DBG) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#define INTL_TRACE(fmt, ...) Output::Trace(Js::IntlPhase, _u("%S(): " fmt "\n"), __func__, __VA_ARGS__)
#else
#define INTL_TRACE(fmt, ...)
#endif

#define ICU_ASSERT(e, expr)                                                   \
    do                                                                        \
    {                                                                         \
        if (e == U_MEMORY_ALLOCATION_ERROR)                                   \
        {                                                                     \
            Js::Throw::OutOfMemory();                                         \
        }                                                                     \
        else if (ICU_FAILURE(e))                                              \
        {                                                                     \
            AssertOrFailFastMsg(false, ICU_ERRORMESSAGE(e));                  \
        }                                                                     \
        else if (!(expr))                                                     \
        {                                                                     \
            AssertOrFailFast(expr);                                           \
        }                                                                     \
    } while (false)

#endif // INTL_ICU

// The following macros allow the key-value pairs to be C++ enums as well as JS objects
// in Intl.js. When adding a new macro, follow the same format as the _VALUES macros below,
// and add your new _VALUES macro to PROJECTED_ENUMS along with the name of the enum.
// NOTE: make sure the last VALUE macro has the highest integer value, since the C++ enum's ::Max
// value is added to the end of the C++ enum definition as an increment of the previous value.
// The ::Max value is used in a defensive assert, and we want to make sure its always 1 greater
// than the highest valid value.

#define NUMBERFORMATSTYLE_VALUES(VALUE) \
VALUE(Default, default_, 0) \
VALUE(Decimal, decimal, 0) \
VALUE(Percent, percent, 1) \
VALUE(Currency, currency, 2)

#define NUMBERFORMATCURRENCYDISPLAY_VALUES(VALUE) \
VALUE(Default, default_, 0) \
VALUE(Symbol, symbol, 0) \
VALUE(Code, code, 1) \
VALUE(Name, name, 2)

#define COLLATORSENSITIVITY_VALUES(VALUE) \
VALUE(Default, default_, 3) \
VALUE(Base, base, 0) \
VALUE(Accent, accent, 1) \
VALUE(Case, case_, 2) \
VALUE(Variant, variant, 3)

#define COLLATORCASEFIRST_VALUES(VALUE) \
VALUE(Default, default_, 2) \
VALUE(Upper, upper, 0) \
VALUE(Lower, lower, 1) \
VALUE(False, false_, 2)

// LocaleDataKind intentionally has no Default value
#define LOCALEDATAKIND_VALUES(VALUE) \
VALUE(Collation, co, 0) \
VALUE(CaseFirst, kf, 1) \
VALUE(Numeric, kn, 2) \
VALUE(Calendar, ca, 3) \
VALUE(NumberingSystem, nu, 4) \
VALUE(HourCycle, hc, 5)

//BuiltInFunctionID intentionally has no Default value
#define BUILTINFUNCTIONID_VALUES(VALUE) \
VALUE(DateToLocaleString, DateToLocaleString, 0) \
VALUE(DateToLocaleDateString, DateToLocaleDateString, 1) \
VALUE(DateToLocaleTimeString, DateToLocaleTimeString, 2) \
VALUE(NumberToLocaleString, NumberToLocaleString, 3) \
VALUE(StringLocaleCompare, StringLocaleCompare, 4)

#define ENUM_VALUE(enumName, propId, value) enumName = value,
#define PROJECTED_ENUM(ClassName, VALUES) \
enum class ClassName \
{ \
    VALUES(ENUM_VALUE) \
    Max \
};

#define PROJECTED_ENUMS(PROJECT) \
PROJECT(LocaleDataKind, LOCALEDATAKIND_VALUES) \
PROJECT(CollatorCaseFirst, COLLATORCASEFIRST_VALUES) \
PROJECT(CollatorSensitivity, COLLATORSENSITIVITY_VALUES) \
PROJECT(NumberFormatCurrencyDisplay, NUMBERFORMATCURRENCYDISPLAY_VALUES) \
PROJECT(NumberFormatStyle, NUMBERFORMATSTYLE_VALUES) \
PROJECT(BuiltInFunctionID, BUILTINFUNCTIONID_VALUES)

PROJECTED_ENUMS(PROJECTED_ENUM)

#undef PROJECTED_ENUM
#undef ENUM_VALUE

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

#define IfFailAssertAndThrowHr(op) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, "HRESULT was a failure."); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#define IfFailAssertMsgAndThrowHr(op, msg) \
    if (FAILED(hr=(op))) \
    { \
    AssertMsg(false, msg); \
    JavascriptError::MapAndThrowError(scriptContext, hr); \
    } \

#ifdef INTL_WINGLOB

#define TO_JSBOOL(sc, b) ((b) ? (sc)->GetLibrary()->GetTrue() : (sc)->GetLibrary()->GetFalse())

#define IfCOMFailIgnoreSilentlyAndReturn(op) \
    if(FAILED(hr=(op))) \
    { \
        return; \
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
    (GetPropertyFrom(obj, Js::PropertyIds::builtInPropID) && VarIs<Type>(propertyValue)) \

#define HasPropertyOn(obj, propID) \
    Js::JavascriptOperators::HasProperty(obj, propID) \

#define HasPropertyBuiltInOn(obj, builtInPropID) \
    HasPropertyOn(obj, Js::PropertyIds::builtInPropID) \

#define HasPropertyLOn(obj, propertyName) \
    HasPropertyOn(obj, scriptContext->GetOrAddPropertyIdTracked(propertyName, wcslen(propertyName)))

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
    typedef FinalizableICUObject<UCollator *, ucol_close> FinalizableUCollator;
    typedef FinalizableICUObject<UPluralRules *, uplrules_close> FinalizableUPluralRules;

    template<typename TExecutor>
    static void EnsureBuffer(_In_ TExecutor executor, _In_ Recycler *recycler, _Outptr_result_buffer_(*returnLength) char16 **ret, _Out_ int *returnLength, _In_ bool allowZeroLengthStrings = false, _In_ int firstTryLength = 8)
    {
        UErrorCode status = U_ZERO_ERROR;
        *ret = RecyclerNewArrayLeaf(recycler, char16, firstTryLength);
        *returnLength = executor(reinterpret_cast<UChar *>(*ret), firstTryLength, &status);
        AssertOrFailFast(allowZeroLengthStrings ? *returnLength >= 0 : *returnLength > 0);
        if (ICU_BUFFER_FAILURE(status))
        {
            AssertOrFailFastMsg(*returnLength >= firstTryLength, "Executor reported buffer failure but did not require additional space");
            int secondTryLength = *returnLength + 1;
            INTL_TRACE("Buffer of length %d was too short, retrying with buffer of length %d", firstTryLength, secondTryLength);
            status = U_ZERO_ERROR;
            *ret = RecyclerNewArrayLeaf(recycler, char16, secondTryLength);
            *returnLength = executor(reinterpret_cast<UChar *>(*ret), secondTryLength, &status);
            AssertOrFailFastMsg(*returnLength == secondTryLength - 1, "Second try of executor returned unexpected length");
        }
        else
        {
            AssertOrFailFastMsg(*returnLength < firstTryLength, "Executor required additional length but reported successful status");
        }

        AssertOrFailFastMsg(!ICU_FAILURE(status), ICU_ERRORMESSAGE(status));
    }

    template <typename T>
    static T *AssertProperty(_In_ DynamicObject *state, _In_ PropertyIds propertyId)
    {
        Var propertyValue = nullptr;
        JavascriptOperators::GetProperty(state, propertyId, &propertyValue, state->GetScriptContext());

        AssertOrFailFast(propertyValue && VarIs<T>(propertyValue));

        return UnsafeVarTo<T>(propertyValue);
    }

    static JavascriptString *AssertStringProperty(_In_ DynamicObject *state, _In_ PropertyIds propertyId)
    {
        return AssertProperty<JavascriptString>(state, propertyId);
    }

    static int AssertIntegerProperty(_In_ DynamicObject *state, _In_ PropertyIds propertyId)
    {
        Var propertyValue = nullptr;
        JavascriptOperators::GetProperty(state, propertyId, &propertyValue, state->GetScriptContext());

        AssertOrFailFast(propertyValue);

        if (TaggedInt::Is(propertyValue))
        {
            return TaggedInt::ToInt32(propertyValue);
        }
        else
        {
            AssertOrFailFast(JavascriptNumber::Is(propertyValue));
            int ret;
            AssertOrFailFast(JavascriptNumber::TryGetInt32Value(JavascriptNumber::GetValue(propertyValue), &ret));

            return ret;
        }
    }

    static bool AssertBooleanProperty(_In_ DynamicObject *state, _In_ PropertyIds propertyId)
    {
        return AssertProperty<JavascriptBoolean>(state, propertyId)->GetValue();
    }

    template <typename T>
    static T AssertEnumProperty(_In_ DynamicObject *state, _In_ PropertyIds propertyId)
    {
        int p = AssertIntegerProperty(state, propertyId);
        T ret = static_cast<T>(p);
        AssertMsg(p >= 0 && ret < T::Max, "Invalid value for enum property");
        return ret;
    }

    template <typename T>
    static _Ret_notnull_ T ThrowOOMIfNull(_In_ T value)
    {
        if (value == nullptr)
        {
            Throw::OutOfMemory();
        }

        return value;
    }

    template <size_t N>
    static void LangtagToLocaleID(_In_count_(langtagLength) const char16 *langtag, _In_ charcount_t langtagLength, _Out_ char(&localeID)[N])
    {
        static_assert(N >= ULOC_FULLNAME_CAPACITY, "LocaleID must be large enough to fit the largest possible ICU localeID");

        UErrorCode status = U_ZERO_ERROR;
        utf8::WideToNarrow langtag8(langtag, langtagLength);
        int32_t localeIDLength = 0;
        uloc_forLanguageTag(langtag8, localeID, N, &localeIDLength, &status);
        ICU_ASSERT(status, localeIDLength > 0 && static_cast<size_t>(localeIDLength) < N);
    }

    template <size_t N>
    static void LangtagToLocaleID(_In_ JavascriptString *langtag, _Out_ char(&localeID)[N])
    {
        LangtagToLocaleID(langtag->GetString(), langtag->GetLength(), localeID);
    }

    template <typename Callback>
    static void ForEachUEnumeration(UEnumeration *enumeration, Callback callback)
    {
        int valueLength = 0;
        UErrorCode status = U_ZERO_ERROR;
        for (int index = 0, const char *value = uenum_next(enumeration, &valueLength, &status); value != nullptr; index++, value = uenum_next(enumeration, &valueLength, &status))
        {
            ICU_ASSERT(status, valueLength > 0);

            // cast valueLength now since we have verified its greater than 0
            callback(index, value, static_cast<charcount_t>(valueLength));
        }
    }

    template <typename Callback>
    static void ForEachUEnumeration16(UEnumeration *enumeration, Callback callback)
    {
        int valueLength = 0;
        UErrorCode status = U_ZERO_ERROR;
        int index = 0;
        for (const UChar *value = uenum_unext(enumeration, &valueLength, &status); value != nullptr; index++, value = uenum_unext(enumeration, &valueLength, &status))
        {
            ICU_ASSERT(status, valueLength > 0);

            // cast valueLength now since we have verified its greater than 0
            callback(index, reinterpret_cast<const char16 *>(value), static_cast<charcount_t>(valueLength));
        }
    }
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

        // Ensure JsBuiltIns are initialized before initializing Intl which uses some of them.
        library->EnsureBuiltInEngineIsReady();

        DynamicObject* commonObject = library->GetEngineInterfaceObject()->GetCommonNativeInterfaces();
        if (scriptContext->IsIntlEnabled())
        {
            Assert(library->GetEngineInterfaceObject() != nullptr);
            this->intlNativeInterfaces = DynamicObject::New(library->GetRecycler(),
                DynamicType::New(scriptContext, TypeIds_Object, commonObject, nullptr,
                    DeferredTypeHandler<InitializeIntlNativeInterfaces>::GetDefaultInstance()));
            library->AddMember(library->GetEngineInterfaceObject(), Js::PropertyIds::Intl, this->intlNativeInterfaces);

            // Only show the platform object publicly if -IntlPlatform is passed
            if (CONFIG_FLAG(IntlPlatform))
            {
                library->AddMember(library->GetIntlObject(), PropertyIds::platform, this->intlNativeInterfaces);
            }
        }
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
        int initSlotCapacity = 0;

        // automatically get the initSlotCapacity from everything we are about to add to intlNativeInterfaces
#define INTL_ENTRY(id, func) initSlotCapacity++;
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY

#define PROJECTED_ENUM(ClassName, VALUES) initSlotCapacity++;
PROJECTED_ENUMS(PROJECTED_ENUM)
#undef PROJECTED_ENUM

        // add capacity for platform.winglob and platform.FallbackSymbol
        initSlotCapacity += 2;

        typeHandler->Convert(intlNativeInterfaces, mode, initSlotCapacity);

        ScriptContext* scriptContext = intlNativeInterfaces->GetScriptContext();
        JavascriptLibrary* library = scriptContext->GetLibrary();

// gives each entrypoint a property ID on the intlNativeInterfaces library object
#define INTL_ENTRY(id, func) library->AddFunctionToLibraryObject(intlNativeInterfaces, Js::PropertyIds::##id, &IntlEngineInterfaceExtensionObject::EntryInfo::Intl_##func, 1);
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY

        library->AddMember(intlNativeInterfaces, PropertyIds::FallbackSymbol, library->CreateSymbol(BuiltInPropertyRecords::_intlFallbackSymbol));

        DynamicObject * enumObj = nullptr;

// Projects the exact layout of our C++ enums into Intl.js so that we dont have to remember to keep them in sync
#define ENUM_VALUE(enumName, propId, value) library->AddMember(enumObj, PropertyIds::##propId, JavascriptNumber::ToVar(value, scriptContext));
#define PROJECTED_ENUM(ClassName, VALUES) \
    enumObj = library->CreateObject(); \
    VALUES(ENUM_VALUE) \
    library->AddMember(intlNativeInterfaces, PropertyIds::##ClassName, enumObj); \

PROJECTED_ENUMS(PROJECTED_ENUM)

#undef PROJECTED_ENUM
#undef ENUM_VALUE

#if INTL_WINGLOB
        library->AddMember(intlNativeInterfaces, Js::PropertyIds::winglob, library->GetTrue());
#else
        library->AddMember(intlNativeInterfaces, Js::PropertyIds::winglob, library->GetFalse());

#if defined(NTBUILD)
        // when using ICU, we can call ulocdata_getCLDRVersion to ensure that ICU is functioning properly before allowing Intl to continue.
        // ulocdata_getCLDRVersion will cause the data file to be loaded, and if we don't have enough memory to do so, we can throw OutOfMemory here.
        // This is to protect against spurious U_MISSING_RESOURCE_ERRORs and U_FILE_ACCESS_ERRORs coming from early-lifecycle
        // functions that require ICU data.
        // See OS#16897150, OS#16896933, and others relating to bad statuses returned by GetLocaleData and IsLocaleAvailable
        // This was initially attempted using u_init, however u_init does not work with Node's default small-icu data file
        // because it contains no converters.
        UErrorCode status = U_ZERO_ERROR;
        UVersionInfo cldrVersion;
        ulocdata_getCLDRVersion(cldrVersion, &status);
        if (status == U_MEMORY_ALLOCATION_ERROR || status == U_FILE_ACCESS_ERROR || status == U_MISSING_RESOURCE_ERROR)
        {
            // Trace that this happens in case there are build system changes that actually cause the data file to be not found
            INTL_TRACE("Could not initialize ICU - ulocdata_getCLDRVersion returned status %S", u_errorName(status));
            Throw::OutOfMemory();
        }
        else
        {
            INTL_TRACE("Using CLDR version %d.%d.%d.%d", cldrVersion[0], cldrVersion[1], cldrVersion[2], cldrVersion[3]);
        }

        AssertOrFailFastMsg(U_SUCCESS(status), "ulocdata_getCLDRVersion returned non-OOM failure");
#endif // defined(NTBUILD)
#endif // else !INTL_WINGLOB

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

        if (!JavascriptOperators::GetProperty(VarTo<DynamicObject>(propertyValue), Js::PropertyIds::prototype, &prototypeValue, scriptContext) ||
            !JavascriptOperators::IsObject(prototypeValue))
        {
            return;
        }

        prototypeObject = VarTo<DynamicObject>(prototypeValue);

        if (!JavascriptOperators::GetProperty(prototypeObject, Js::PropertyIds::resolvedOptions, &resolvedOptionsValue, scriptContext) ||
            !JavascriptOperators::IsObject(resolvedOptionsValue))
        {
            return;
        }

        functionObj = VarTo<DynamicObject>(resolvedOptionsValue);
        functionObj->SetConfigurable(Js::PropertyIds::prototype, true);
        functionObj->DeleteProperty(Js::PropertyIds::prototype, Js::PropertyOperationFlags::PropertyOperation_None);

        if (!JavascriptOperators::GetOwnAccessors(prototypeObject, getterFunctionId, &getter, &setter, scriptContext) ||
            !JavascriptOperators::IsObject(getter))
        {
            return;
        }

        functionObj = VarTo<DynamicObject>(getter);
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

            Js::Arguments arguments(callInfo, args);
            scriptContext->GetThreadContext()->ExecuteImplicitCall(function, Js::ImplicitCall_Accessor, [=]()->Js::Var
            {
                return JavascriptFunction::CallRootFunctionInScript(function, arguments);
            });

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

        if (args.Info.Count < 2 || !VarIs<JavascriptError>(args.Values[1]))
        {
            AssertMsg(false, "Intl's Assert platform API was called incorrectly.");
            return scriptContext->GetLibrary()->GetUndefined();
        }

#if DEBUG
#ifdef INTL_ICU_DEBUG
        Output::Print(_u("EntryIntl_RaiseAssert\n"));
#endif
        JavascriptExceptionOperators::Throw(VarTo<JavascriptError>(args.Values[1]), scriptContext);
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

        if (args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
        {
            // IsWellFormedLanguageTag of undefined or non-string is false
            return scriptContext->GetLibrary()->GetFalse();
        }

        JavascriptString *argString = VarTo<JavascriptString>(args.Values[1]);

        return TO_JSBOOL(scriptContext, GetWindowsGlobalizationAdapter(scriptContext)->IsWellFormedLanguageTag(scriptContext, argString->GetSz()));
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_NormalizeLanguageTag(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#if defined(INTL_ICU)
        INTL_CHECK_ARGS(args.Info.Count == 2 && VarIs<JavascriptString>(args[1]));

        UErrorCode status = U_ZERO_ERROR;
        JavascriptString *langtag = UnsafeVarTo<JavascriptString>(args[1]);
        utf8::WideToNarrow langtag8(langtag->GetSz(), langtag->GetLength());

        // ICU doesn't have a full-fledged canonicalization implementation that correctly replaces all preferred values
        // and grandfathered tags, as required by #sec-canonicalizelanguagetag.
        // However, passing the locale through uloc_forLanguageTag -> uloc_toLanguageTag gets us most of the way there
        // by replacing some(?) values, correctly capitalizing the tag, and re-ordering extensions
        int parsedLength = 0;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int forLangTagResultLength = uloc_forLanguageTag(langtag8, localeID, ULOC_FULLNAME_CAPACITY, &parsedLength, &status);
        AssertOrFailFast(parsedLength >= 0);
        if (status == U_ILLEGAL_ARGUMENT_ERROR || ((charcount_t) parsedLength) < langtag->GetLength())
        {
            // The string passed in to NormalizeLanguageTag has already passed IsStructurallyValidLanguageTag.
            // However, duplicate unicode extension keys, such as "de-u-co-phonebk-co-phonebk", are structurally
            // valid according to RFC5646 yet still trigger U_ILLEGAL_ARGUMENT_ERROR
            // V8 ~6.2 says that the above language tag is invalid, while SpiderMonkey ~58 handles it.
            // Until we have a more spec-compliant implementation of CanonicalizeLanguageTag, err on the side
            // of caution and say it is invalid.
            // We also check for parsedLength < langtag->GetLength() because there are cases when status == U_ZERO_ERROR
            // but the langtag was not valid, such as "en-tesTER-TESter" (OSS-Fuzz #6657).
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
        if (args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

        JavascriptString *argString = VarTo<JavascriptString>(args.Values[1]);
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
    template <const char *(__cdecl *GetAvailableLocalesFunc)(int), int(__cdecl *CountAvailableLocalesFunc)(void)>
    static bool BinarySearchForLocale(const char *localeID)
    {
        const int count = CountAvailableLocalesFunc();
        int left = 0;
        int right = count - 1;
        int iterations = 0;
        while (true)
        {
            iterations += 1;
            if (left > right)
            {
                INTL_TRACE("Could not find localeID %S in %d iterations", localeID, iterations);
                return false;
            }

            int i = (left + right) / 2;
            Assert(i >= 0 && i < count);

            const char *cur = GetAvailableLocalesFunc(i);

            // Ensure that this list is actually binary searchable
            Assert(i > 0 ? strcmp(GetAvailableLocalesFunc(i - 1), cur) < 0 : true);
            Assert(i < count - 1 ? strcmp(GetAvailableLocalesFunc(i + 1), cur) > 0 : true);

            int res = strcmp(localeID, cur);
            if (res == 0)
            {
                INTL_TRACE("Found localeID %S in %d iterations", localeID, iterations);
                return true;
            }
            else if (res < 0)
            {
                right = i - 1;
            }
            else
            {
                left = i + 1;
            }
        }
    }

    template <const char *(__cdecl *GetAvailableLocalesFunc)(int), int(__cdecl *CountAvailableLocalesFunc)(void)>
    static bool IsLocaleAvailable(JavascriptString *langtag)
    {
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        LangtagToLocaleID(langtag, localeID);

        if (!BinarySearchForLocale<GetAvailableLocalesFunc, CountAvailableLocalesFunc>(localeID))
        {
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

            return BinarySearchForLocale<GetAvailableLocalesFunc, CountAvailableLocalesFunc>(localeIDWithLikelySubtags);
        }

        return true;
    }
#endif

#ifdef INTL_ICU
#define DEFINE_ISXLOCALEAVAILABLE(ctorShortName, icuNamespace) \
    Var IntlEngineInterfaceExtensionObject::EntryIntl_Is##ctorShortName##LocaleAvailable(RecyclableObject* function, CallInfo callInfo, ...) \
    { \
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo); \
        INTL_CHECK_ARGS(args.Info.Count == 2 && VarIs<JavascriptString>(args.Values[1])); \
        return scriptContext->GetLibrary()->GetTrueOrFalse( \
            IsLocaleAvailable<##icuNamespace##_getAvailable, ##icuNamespace##_countAvailable>(UnsafeVarTo<JavascriptString>(args.Values[1])) \
        ); \
    }
#else
#define DEFINE_ISXLOCALEAVAILABLE(ctorShortName, icuNamespace) \
    Var IntlEngineInterfaceExtensionObject::EntryIntl_Is##ctorShortName##LocaleAvailable(RecyclableObject* function, CallInfo callInfo, ...) \
    { \
        AssertOrFailFastMsg(false, "Intl with Windows Globalization should never call Is" #ctorShortName "LocaleAvailable"); \
        return nullptr; \
    }
#endif

DEFINE_ISXLOCALEAVAILABLE(Collator, ucol)
DEFINE_ISXLOCALEAVAILABLE(NF, unum)
DEFINE_ISXLOCALEAVAILABLE(DTF, udat)
// uplrules namespace doesn't have its own getAvailable/countAvailable
// assume it supports whatever is supported in the base data
DEFINE_ISXLOCALEAVAILABLE(PR, uloc)

#ifdef INTL_ICU


#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetLocaleData(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(
            args.Info.Count == 3 &&
            (JavascriptNumber::Is(args.Values[1]) || TaggedInt::Is(args.Values[1])) &&
            VarIs<JavascriptString>(args.Values[2])
        );

        LocaleDataKind kind = (LocaleDataKind) (TaggedInt::Is(args.Values[1])
            ? TaggedInt::ToInt32(args.Values[1])
            : (int) JavascriptNumber::GetValue(args.Values[1]));

        JavascriptArray *ret = nullptr;

        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        JavascriptString *langtag = UnsafeVarTo<JavascriptString>(args.Values[2]);
        LangtagToLocaleID(langtag, localeID);

        JavascriptLibrary *library = scriptContext->GetLibrary();
        PropertyOperationFlags flag = PropertyOperationFlags::PropertyOperation_None;

        if (kind == LocaleDataKind::Collation)
        {
            ScopedUEnumeration collations(ucol_getKeywordValuesForLocale("collation", localeID, false, &status));
            int collationsCount = uenum_count(collations, &status);

            // we expect collationsCount to have at least "standard" and "search" in it
            ICU_ASSERT(status, collationsCount > 2);

            // the return array can't include "standard" and "search", but must have its first element be null (count - 2 + 1) [#sec-intl-collator-internal-slots]
            ret = library->CreateArray(collationsCount - 1);
            ret->SetItem(0, library->GetNull(), flag);

            int collationLen = 0;
            const char *collation = nullptr;
            int i = 0;
            for (collation = uenum_next(collations, &collationLen, &status); collation != nullptr; collation = uenum_next(collations, &collationLen, &status))
            {
                ICU_ASSERT(status, collation != nullptr && collationLen > 0);
                if (strcmp(collation, "standard") == 0 || strcmp(collation, "search") == 0)
                {
                    // continue does not create holes in ret because i is set outside the loop
                    continue;
                }

                // OS#17172584: OOM during uloc_toUnicodeLocaleType can make this return nullptr even for known collations
                const char *unicodeCollation = ThrowOOMIfNull(uloc_toUnicodeLocaleType("collation", collation));
                const size_t unicodeCollationLen = strlen(unicodeCollation);

                // we only need strlen(unicodeCollation) + 1 char16s because unicodeCollation will always be ASCII (funnily enough)
                char16 *unicodeCollation16 = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, unicodeCollationLen + 1);
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
                // i + 1 to not overwrite leading null element
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
            ret = library->CreateArray(3);

            JavascriptString *falseStr = library->GetFalseDisplayString();
            JavascriptString *upperStr = library->GetIntlCaseFirstUpperString();
            JavascriptString *lowerStr = library->GetIntlCaseFirstLowerString();

            if (kf == UCOL_OFF)
            {
                ret->SetItem(0, falseStr, flag);
                ret->SetItem(1, upperStr, flag);
                ret->SetItem(2, lowerStr, flag);
            }
            else if (kf == UCOL_UPPER_FIRST)
            {
                ret->SetItem(0, upperStr, flag);
                ret->SetItem(1, lowerStr, flag);
                ret->SetItem(2, falseStr, flag);
            }
            else if (kf == UCOL_LOWER_FIRST)
            {
                ret->SetItem(0, lowerStr, flag);
                ret->SetItem(1, upperStr, flag);
                ret->SetItem(2, falseStr, flag);
            }
        }
        else if (kind == LocaleDataKind::Numeric)
        {
            ScopedUCollator collator(ucol_open(localeID, &status));
            UColAttributeValue kn = ucol_getAttribute(collator, UCOL_NUMERIC_COLLATION, &status);
            ICU_ASSERT(status, true);
            ret = library->CreateArray(2);

            JavascriptString *falseStr = library->GetFalseDisplayString();
            JavascriptString *trueStr = library->GetTrueDisplayString();

            if (kn == UCOL_OFF)
            {
                ret->SetItem(0, falseStr, flag);
                ret->SetItem(1, trueStr, flag);
            }
            else if (kn == UCOL_ON)
            {
                ret->SetItem(0, trueStr, flag);
                ret->SetItem(1, falseStr, flag);
            }
        }
        else if (kind == LocaleDataKind::Calendar)
        {
            ScopedUEnumeration calendars(ucal_getKeywordValuesForLocale("calendar", localeID, false, &status));
            ret = library->CreateArray(uenum_count(calendars, &status));
            ICU_ASSERT(status, true);

            int calendarLen = 0;
            const char *calendar = nullptr;
            int i = 0;
            for (calendar = uenum_next(calendars, &calendarLen, &status); calendar != nullptr; calendar = uenum_next(calendars, &calendarLen, &status))
            {
                ICU_ASSERT(status, calendar != nullptr && calendarLen > 0);

                // OS#17172584: OOM during uloc_toUnicodeLocaleType can make this return nullptr even for known calendars
                const char *unicodeCalendar = ThrowOOMIfNull(uloc_toUnicodeLocaleType("calendar", calendar));
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
                ), flag);
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
            ScopedUNumberingSystem numsys(unumsys_open(localeID, &status));
            ICU_ASSERT(status, true);
            utf8::NarrowToWide numsysName(unumsys_getName(numsys));

            // NOTE: update the initial array length if the list of available numbering systems changes in the future!
            ret = library->CreateArray(22);
            int i = 0;
            ret->SetItem(i++, JavascriptString::NewCopySz(numsysName, scriptContext), flag);

            // It doesn't matter that item 0 will be in the array twice (aside for size), because item 0 is the
            // preferred numbering system for the given locale, so it has precedence over everything else
            ret->SetItem(i++, library->GetIntlNumsysArabString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysArabextString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysBaliString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysBengString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysDevaString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysFullwideString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysGujrString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysGuruString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysHanidecString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysKhmrString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysKndaString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysLaooString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysLatnString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysLimbString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysMlymString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysMongString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysMymrString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysOryaString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysTamldecString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysTeluString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysThaiString(), flag);
            ret->SetItem(i++, library->GetIntlNumsysTibtString(), flag);
        }
        else if (kind == LocaleDataKind::HourCycle)
        {
            // #sec-intl.datetimeformat-internal-slots: "[[LocaleData]][locale].hc must be < null, h11, h12, h23, h24 > for all locale values"
            ret = library->CreateArray(5);
            int i = 0;
            ret->SetItem(i++, library->GetNull(), flag);
            ret->SetItem(i++, library->GetIntlHourCycle11String(), flag);
            ret->SetItem(i++, library->GetIntlHourCycle12String(), flag);
            ret->SetItem(i++, library->GetIntlHourCycle23String(), flag);
            ret->SetItem(i++, library->GetIntlHourCycle24String(), flag);
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

        if (args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
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
        JavascriptString *argString = VarTo<JavascriptString>(args.Values[1]);
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

        if (args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
        {
            // NormalizeLanguageTag of undefined or non-string is undefined
            return scriptContext->GetLibrary()->GetUndefined();
        }

#if defined(INTL_ICU)
        AssertOrFailFastMsg(false, "Intl-ICU does not implement ResolveLocaleBestFit");
        return nullptr;
#else // !INTL_ICU
        JavascriptString *localeStrings = VarTo<JavascriptString>(args.Values[1]);
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

        if (args.Info.Count < 2 || !VarIs<JavascriptString>(args.Values[1]))
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
        if (FAILED(hr = wga->CreateLanguage(scriptContext, VarTo<JavascriptString>(args.Values[1])->GetSz(), &language)))
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

#ifdef INTL_ICU
    // This is used by both NumberFormat and PluralRules
    static void SetUNumberFormatDigitOptions(UNumberFormat *fmt, DynamicObject *state)
    {
        if (JavascriptOperators::HasProperty(state, PropertyIds::minimumSignificantDigits))
        {
            unum_setAttribute(fmt, UNUM_SIGNIFICANT_DIGITS_USED, true);
            unum_setAttribute(fmt, UNUM_MIN_SIGNIFICANT_DIGITS, AssertIntegerProperty(state, PropertyIds::minimumSignificantDigits));
            unum_setAttribute(fmt, UNUM_MAX_SIGNIFICANT_DIGITS, AssertIntegerProperty(state, PropertyIds::maximumSignificantDigits));
        }
        else
        {
            unum_setAttribute(fmt, UNUM_MIN_INTEGER_DIGITS, AssertIntegerProperty(state, PropertyIds::minimumIntegerDigits));
            unum_setAttribute(fmt, UNUM_MIN_FRACTION_DIGITS, AssertIntegerProperty(state, PropertyIds::minimumFractionDigits));
            unum_setAttribute(fmt, UNUM_MAX_FRACTION_DIGITS, AssertIntegerProperty(state, PropertyIds::maximumFractionDigits));
        }
    }
#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CacheNumberFormat(RecyclableObject * function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && DynamicObject::IsBaseDynamicObject(args.Values[1]));

#if defined(INTL_ICU)
        DynamicObject *state = UnsafeVarTo<DynamicObject>(args.Values[1]);

        // always AssertOrFailFast that the properties we need are there, because if they aren't, Intl.js isn't functioning correctly
        NumberFormatStyle style = AssertEnumProperty<NumberFormatStyle>(state, PropertyIds::formatterToUse);

        UNumberFormatStyle unumStyle = UNUM_IGNORE;
        UErrorCode status = U_ZERO_ERROR;
        JavascriptString *currency = nullptr;

        JavascriptString *langtag = AssertStringProperty(state, PropertyIds::locale);
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        LangtagToLocaleID(langtag, localeID);

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
            NumberFormatCurrencyDisplay nfcd = AssertEnumProperty<NumberFormatCurrencyDisplay>(state, PropertyIds::currencyDisplayToUse);

            // TODO(jahorto): Investigate making our enum values equal to the corresponding UNumberFormatStyle values
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

            currency = AssertStringProperty(state, PropertyIds::currency);
        }

        AssertOrFailFast(unumStyle != UNUM_IGNORE);

        auto fmt = FinalizableUNumberFormat::New(scriptContext->GetRecycler(), unum_open(unumStyle, nullptr, 0, localeID, nullptr, &status));
        ICU_ASSERT(status, true);

        bool groupingUsed = AssertBooleanProperty(state, PropertyIds::useGrouping);
        unum_setAttribute(*fmt, UNUM_GROUPING_USED, groupingUsed);

        unum_setAttribute(*fmt, UNUM_ROUNDING_MODE, UNUM_ROUND_HALFUP);

        SetUNumberFormatDigitOptions(*fmt, state);

        if (currency != nullptr)
        {
            unum_setTextAttribute(*fmt, UNUM_CURRENCY_CODE, reinterpret_cast<const UChar *>(currency->GetSz()), currency->GetLength(), &status);
            ICU_ASSERT(status, true);
        }

        state->SetInternalProperty(
            InternalPropertyIds::CachedUNumberFormat,
            fmt,
            PropertyOperationFlags::PropertyOperation_None,
            nullptr
        );

        return scriptContext->GetLibrary()->GetUndefined();
#else
        HRESULT hr = S_OK;
        JavascriptString *localeJSstr = nullptr;
        DynamicObject *options = VarTo<DynamicObject>(args.Values[1]);
        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);
        Var propertyValue;

        // Verify locale is present
        // REVIEW (doilij): Fix comparison of the unsigned value <= 0
        if (!GetTypedPropertyBuiltInFrom(options, __locale, JavascriptString) || (localeJSstr = VarTo<JavascriptString>(propertyValue))->GetLength() <= 0)
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
            IfFailThrowHr(GetWindowsGlobalizationAdapter(scriptContext)->CreateCurrencyFormatter(scriptContext, &locale, 1, VarTo<JavascriptString>(propertyValue)->GetSz(), &currencyFormatter));

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
            IfFailThrowHr(numberFormatterOptions->put_IsGrouped((boolean)(VarTo<JavascriptBoolean>(propertyValue)->GetValue())));
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

        if (args.Info.Count < 3 || !DynamicObject::IsBaseDynamicObject(args.Values[1]) || !VarIs<JavascriptBoolean>(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

#ifdef INTL_WINGLOB
        DynamicObject* obj = VarTo<DynamicObject>(args.Values[1]);

        DelayLoadWindowsGlobalization* wgl = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary();
        WindowsGlobalizationAdapter* wga = GetWindowsGlobalizationAdapter(scriptContext);

        HRESULT hr;
        Var propertyValue = nullptr;
        uint32 length;

        PCWSTR locale = GetTypedPropertyBuiltInFrom(obj, __locale, JavascriptString) ? VarTo<JavascriptString>(propertyValue)->GetSz() : nullptr;
        PCWSTR templateString = GetTypedPropertyBuiltInFrom(obj, __templateString, JavascriptString) ? VarTo<JavascriptString>(propertyValue)->GetSz() : nullptr;

        if (locale == nullptr || templateString == nullptr)
        {
            AssertMsg(false, "For some reason, locale and templateString aren't defined or aren't a JavascriptString.");
            return scriptContext->GetLibrary()->GetUndefined();
        }

        PCWSTR clock = GetTypedPropertyBuiltInFrom(obj, __windowsClock, JavascriptString) ? VarTo<JavascriptString>(propertyValue)->GetSz() : nullptr;

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
        if ((boolean)(VarTo<JavascriptBoolean>(args.Values[2])->GetValue()))
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

    Var IntlEngineInterfaceExtensionObject::EntryIntl_LocaleCompare(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_WINGLOB
        AssertOrFailFastMsg(false, "platform.localeCompare should not be called in Intl-WinGlob");
        return nullptr;
#else
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(
            args.Info.Count == 5 &&
            VarIs<JavascriptString>(args[1]) &&
            VarIs<JavascriptString>(args[2]) &&
            DynamicObject::IsBaseDynamicObject(args[3]) &&
            VarIs<JavascriptBoolean>(args[4])
        );

        JavascriptString *left = UnsafeVarTo<JavascriptString>(args[1]);
        JavascriptString *right = UnsafeVarTo<JavascriptString>(args[2]);
        DynamicObject *state = UnsafeVarTo<DynamicObject>(args[3]);
        bool forStringPrototypeLocaleCompare = UnsafeVarTo<JavascriptBoolean>(args[4])->GetValue();
        if (forStringPrototypeLocaleCompare)
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(String_Prototype_localeCompare);
            INTL_TRACE("Calling '%s'.localeCompare('%s', ...)", left->GetSz(), right->GetSz());
        }
        else
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Collator_Prototype_compare);
            INTL_TRACE("Calling Collator.prototype.compare('%s', '%s')", left->GetSz(), right->GetSz());
        }

        // Below, we lazy-initialize the backing UCollator on the first call to localeCompare
        // On subsequent calls, the UCollator will be cached in state.CachedUCollator
        Var cachedUCollator = nullptr;
        FinalizableUCollator *coll = nullptr;
        UErrorCode status = U_ZERO_ERROR;
        if (state->GetInternalProperty(state, InternalPropertyIds::CachedUCollator, &cachedUCollator, nullptr, scriptContext))
        {
            coll = reinterpret_cast<FinalizableUCollator *>(cachedUCollator);
            INTL_TRACE("Using previously cached UCollator (0x%x)", coll);
        }
        else
        {
            // the object key is locale according to Intl spec, but its more accurately a BCP47 Language Tag, not an ICU LocaleID
            JavascriptString *langtag = AssertStringProperty(state, PropertyIds::locale);
            CollatorSensitivity sensitivity = AssertEnumProperty<CollatorSensitivity>(state, PropertyIds::sensitivityEnum);
            bool ignorePunctuation = AssertBooleanProperty(state, PropertyIds::ignorePunctuation);
            bool numeric = AssertBooleanProperty(state, PropertyIds::numeric);
            CollatorCaseFirst caseFirst = AssertEnumProperty<CollatorCaseFirst>(state, PropertyIds::caseFirstEnum);
            JavascriptString *usage = AssertStringProperty(state, PropertyIds::usage);

            char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
            LangtagToLocaleID(langtag, localeID);

            const char16 searchString[] = _u("search");
            if (usage->BufferEquals(searchString, _countof(searchString) - 1)) // minus the null terminator
            {
                uloc_setKeywordValue("collation", "search", localeID, _countof(localeID), &status);
                ICU_ASSERT(status, true);
            }

            coll = FinalizableUCollator::New(scriptContext->GetRecycler(), ucol_open(localeID, &status));
            ICU_ASSERT(status, true);

            // REVIEW(jahorto): Anything that requires a status will no-op if its in a failure state
            // Thus, we can ICU_ASSERT the status once after all of the properties are set for simplicity
            if (sensitivity == CollatorSensitivity::Base)
            {
                ucol_setStrength(*coll, UCOL_PRIMARY);
            }
            else if (sensitivity == CollatorSensitivity::Accent)
            {
                ucol_setStrength(*coll, UCOL_SECONDARY);
            }
            else if (sensitivity == CollatorSensitivity::Case)
            {
                // see "description" for the caseLevel default option: http://userguide.icu-project.org/collation/customization
                ucol_setStrength(*coll, UCOL_PRIMARY);
                ucol_setAttribute(*coll, UCOL_CASE_LEVEL, UCOL_ON, &status);
            }
            else if (sensitivity == CollatorSensitivity::Variant)
            {
                ucol_setStrength(*coll, UCOL_TERTIARY);
            }

            if (ignorePunctuation)
            {
                // see http://userguide.icu-project.org/collation/customization/ignorepunct
                ucol_setAttribute(*coll, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
            }

            if (numeric)
            {
                ucol_setAttribute(*coll, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);
            }

            if (caseFirst == CollatorCaseFirst::Upper)
            {
                ucol_setAttribute(*coll, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &status);
            }
            else if (caseFirst == CollatorCaseFirst::Lower)
            {
                ucol_setAttribute(*coll, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &status);
            }

            // Ensure that collator configuration was successfull
            ICU_ASSERT(status, true);

            INTL_TRACE(
                "Caching UCollator (0x%x) with langtag = %s, sensitivity = %d, caseFirst = %d, ignorePunctuation = %d, and numeric = %d",
                coll,
                langtag->GetSz(),
                sensitivity,
                caseFirst,
                ignorePunctuation,
                numeric
            );

            // cache coll for later use (so that the condition that brought us here returns true for future calls)
            state->SetInternalProperty(
                InternalPropertyIds::CachedUCollator,
                coll,
                PropertyOperationFlags::PropertyOperation_None,
                nullptr
            );
        }

        // As of ES2015, String.prototype.localeCompare must compare canonically equivalent strings as equal
        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("EntryIntl_LocaleCompare"));

        const char16 *leftNormalized = nullptr;
        charcount_t leftNormalizedLength = 0;
        if (UnicodeText::IsNormalizedString(UnicodeText::NormalizationForm::C, left->GetSz(), left->GetLength()))
        {
            leftNormalized = left->GetSz();
            leftNormalizedLength = left ->GetLength();
        }
        else
        {
            leftNormalized = left->GetNormalizedString(UnicodeText::NormalizationForm::C, tempAllocator, leftNormalizedLength);
        }

        const char16 *rightNormalized = nullptr;
        charcount_t rightNormalizedLength = 0;
        if (UnicodeText::IsNormalizedString(UnicodeText::NormalizationForm::C, right->GetSz(), right->GetLength()))
        {
            rightNormalized = right->GetSz();
            rightNormalizedLength = right->GetLength();
        }
        else
        {
            rightNormalized = right->GetNormalizedString(UnicodeText::NormalizationForm::C, tempAllocator, rightNormalizedLength);
        }

        static_assert(UCOL_LESS == -1 && UCOL_EQUAL == 0 && UCOL_GREATER == 1, "ucol_strcoll should return values compatible with localeCompare");
        Var ret = JavascriptNumber::ToVar(ucol_strcoll(
            *coll,
            reinterpret_cast<const UChar *>(leftNormalized),
            leftNormalizedLength,
            reinterpret_cast<const UChar *>(rightNormalized),
            rightNormalizedLength
        ), scriptContext);

        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);

        return ret;
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

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CompareString(RecyclableObject* function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        AssertOrFailFastMsg(false, "EntryIntl_CompareString should not be called in Intl-ICU");
        return nullptr;
#else
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count >= 3 && VarIs<JavascriptString>(args[1]) && VarIs<JavascriptString>(args[2]));

        const char16 *locale = nullptr; // args[3]
        char16 defaultLocale[LOCALE_NAME_MAX_LENGTH] = { 0 };

        CollatorSensitivity sensitivity = CollatorSensitivity::Default; // args[4]
        bool ignorePunctuation = false; // args[5]
        bool numeric = false; // args[6]

        JavascriptString *str1 = VarTo<JavascriptString>(args.Values[1]);
        JavascriptString *str2 = VarTo<JavascriptString>(args.Values[2]);
        CollatorCaseFirst caseFirst = CollatorCaseFirst::Default; // args[7]

        // we only need to parse arguments 3 through 7 if locale and options are provided
        // see fast path in JavascriptString::EntryLocaleCompare
        if (args.Info.Count > 3)
        {
            if (args.Info.Count < 8)
            {
                JavascriptError::MapAndThrowError(scriptContext, E_INVALIDARG);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[3]) && VarIs<JavascriptString>(args.Values[3]))
            {
                locale = VarTo<JavascriptString>(args.Values[3])->GetSz();
            }
            else
            {
                JavascriptError::MapAndThrowError(scriptContext, E_INVALIDARG);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[4]) && TaggedInt::Is(args.Values[4]))
            {
                sensitivity = static_cast<CollatorSensitivity>(TaggedInt::ToUInt16(args.Values[4]));
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[5]) && VarIs<JavascriptBoolean>(args.Values[5]))
            {
                ignorePunctuation = (VarTo<JavascriptBoolean>(args.Values[5])->GetValue() != 0);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[6]) && VarIs<JavascriptBoolean>(args.Values[6]))
            {
                numeric = (VarTo<JavascriptBoolean>(args.Values[6])->GetValue() != 0);
            }

            if (!JavascriptOperators::IsUndefinedObject(args.Values[7]) && TaggedInt::Is(args.Values[7]))
            {
                caseFirst = static_cast<CollatorCaseFirst>(TaggedInt::ToUInt16(args.Values[7]));
            }
        }
        else
        {
            if (GetUserDefaultLocaleName(defaultLocale, _countof(defaultLocale)) != 0)
            {
                locale = defaultLocale;
            }
            else
            {
                JavascriptError::MapAndThrowError(scriptContext, HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        Assert(locale != nullptr);
        Assert((int)sensitivity >= 0 && sensitivity < CollatorSensitivity::Max);
        Assert((int)caseFirst >= 0 && caseFirst < CollatorCaseFirst::Max);

        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("EntryIntl_CompareString"));

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

        // CompareStringEx on Windows returns 0 for error, 1 if less, 2 if equal, 3 if greater
        // Default to the strings being equal, because sorting with == causes no change in the order but converges, whereas < would cause an infinite loop.
        int compareResult = 2;
        HRESULT error = S_OK;
        DWORD comparisonFlags = GetCompareStringComparisonFlags(sensitivity, ignorePunctuation, numeric);
        compareResult = CompareStringEx(locale, comparisonFlags, left, leftLen, right, rightLen, NULL, NULL, 0);
        error = HRESULT_FROM_WIN32(GetLastError());

        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);

        if (compareResult == 0)
        {
            JavascriptError::MapAndThrowError(scriptContext, error);
        }

        return JavascriptNumber::ToVar(compareResult - 2, scriptContext);
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_CurrencyDigits(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

        INTL_CHECK_ARGS(
            args.Info.Count == 2 &&
            VarIs<JavascriptString>(args.Values[1])
        );

        const char16 *currencyCode = UnsafeVarTo<JavascriptString>(args.Values[1])->GetSz();

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

#ifdef INTL_ICU
    // Rationale for this data structure: ICU reports back a tree of parts where each node in the tree
    // has a type and a width, and there must be at least one node corresponding to each character in the tree
    // (nodes can be wider than one character). Nodes can have children, and the parent-child relationship is
    // that child node represents a more specific type for a given character range than the parent. Since ICU
    // doesn't ever report "literal" parts of strings (like spaces or other extra characters), the root node in
    // the tree will always be the entire width of the string with the type UnsetField, and we later map UnsetField
    // to the "literal" part. Then, for a string like "US$ 1,000", there will be two child nodes, one of type
    // currency with width [0, 3) and one of type integer with width [4, 9). The integer node will have a child
    // of type group and width [6, 7). So the most specific type for characters 0 to 3 is currency, 3 to 4 is unset
    // (literal), 4 to 5 is integer, 5 to 6 is group, and 6 to 9 is integer. This linear scan across the string,
    // determining the part of consecutive spans of characters, is what the NumberFormat.prototype.formatToParts
    // API needs to return to the user.
    //
    // I thought about this a bunch and I didn't like the idea of traversing an actual tree structure to get that
    // information because it felt awkward to encode the "width" of nodes with specific meaning during traveral.
    // So, I came up with an array structure where basically when we are told a part exists from position x to y,
    // we can figure out what type used to apply to that span and update that section of the array with the new type.
    // We skip over sections of the span [x, y) that have a type that doesn't match the start and end because that
    // means we have already gotten a more specific part for that sub-span (for instance if we got a grouping
    // separator before it's parent integer)
    class NumberFormatPartsBuilder
    {
    private:
        double num;
        Field(const char16 *) formatted;
        const charcount_t formattedLength;
        Field(ScriptContext *) sc;
        Field(UNumberFormatFields *) fields;

        static const UNumberFormatFields UnsetField = static_cast<UNumberFormatFields>(0xFFFFFFFF);

        JavascriptString *GetPartTypeString(UNumberFormatFields field)
        {
            JavascriptLibrary *library = sc->GetLibrary();

            // this is outside the switch because MSVC doesn't like that UnsetField is not a valid enum value
            if (field == UnsetField)
            {
                return library->GetIntlLiteralPartString();
            }

            switch (field)
            {
            case UNUM_INTEGER_FIELD:
            {
                if (JavascriptNumber::IsNan(num))
                {
                    return library->GetIntlNanPartString();
                }
                else if (!NumberUtilities::IsFinite(num))
                {
                    return library->GetIntlInfinityPartString();
                }
                else
                {
                    return library->GetIntlIntegerPartString();
                }
            }
            case UNUM_FRACTION_FIELD: return library->GetIntlFractionPartString();
            case UNUM_DECIMAL_SEPARATOR_FIELD: return library->GetIntlDecimalPartString();

            // The following three should only show up if UNUM_SCIENTIFIC is used, which Intl.NumberFormat doesn't currently use
            case UNUM_EXPONENT_SYMBOL_FIELD: AssertOrFailFastMsg(false, "Unexpected exponent symbol field");
            case UNUM_EXPONENT_SIGN_FIELD: AssertOrFailFastMsg(false, "Unexpected exponent sign field");
            case UNUM_EXPONENT_FIELD: AssertOrFailFastMsg(false, "Unexpected exponent field");

            case UNUM_GROUPING_SEPARATOR_FIELD: return library->GetIntlGroupPartString();
            case UNUM_CURRENCY_FIELD: return library->GetIntlCurrencyPartString();
            case UNUM_PERCENT_FIELD: return library->GetIntlPercentPartString();

            // TODO(jahorto): Determine if this would ever be returned and what it would map to
            case UNUM_PERMILL_FIELD: AssertOrFailFastMsg(false, "Unexpected permill field");

            case UNUM_SIGN_FIELD: return num < 0 ? library->GetIntlMinusSignPartString() : library->GetIntlPlusSignPartString();

            // At the ECMA-402 TC39 call for May 2017, it was decided that we should treat unmapped parts as type: "unknown"
            default: return library->GetIntlUnknownPartString();
            }
        }

    public:
        NumberFormatPartsBuilder(double num, const char16 *formatted, charcount_t formattedLength, ScriptContext *scriptContext)
            : num(num)
            , formatted(formatted)
            , formattedLength(formattedLength)
            , sc(scriptContext)
            , fields(RecyclerNewArrayLeaf(sc->GetRecycler(), UNumberFormatFields, formattedLength))
        {
            // this will allow InsertPart to tell what fields have been initialized or not, and will
            // be the value of resulting { type: "literal" } fields in ToPartsArray
            memset(fields, UnsetField, sizeof(UNumberFormatFields) * formattedLength);
        }

        // Note -- ICU reports fields as inclusive start, exclusive end, so thats how we handle the parameters here
        void InsertPart(UNumberFormatFields field, int start, int end)
        {
            AssertOrFailFast(start >= 0);
            AssertOrFailFast(start < end);

            // the asserts above mean the cast to charcount_t is safe
            charcount_t ccStart = static_cast<charcount_t>(start);
            charcount_t ccEnd = static_cast<charcount_t>(end);

            AssertOrFailFast(ccEnd <= formattedLength);

            // Make sure that the part does not overlap/is a strict subset or superset of other fields
            // The part builder could probably be built and tested to handle this structure not being so rigid,
            // but its safer for now to make sure this is true.
            UNumberFormatFields parentStartField = fields[ccStart];
            UNumberFormatFields parentEndField = fields[ccEnd - 1];
            AssertOrFailFast(parentStartField == parentEndField);

            // Actually insert the part now
            // Only overwrite fields in the span that match start and end, since fields that don't match are sub-parts that were already inserted
            for (charcount_t i = ccStart; i < ccEnd; ++i)
            {
                if (fields[i] == parentStartField)
                {
                    fields[i] = field;
                }
            }
        }

        JavascriptArray *ToPartsArray()
        {
            JavascriptArray *ret = sc->GetLibrary()->CreateArray(0);
            int retIndex = 0;

            // With the structure of the fields array, creating the resulting parts list to return to JS is simple
            // Map set parts to the corresponding `type` value (see ECMA-402 #sec-partitionnumberpattern), and unset parts to the `literal` type
            for (charcount_t start = 0; start < formattedLength; ++retIndex)
            {
                // At the top of the loop, fields[start] should always be different from fields[start - 1] (we should be at the start of a new part),
                // since start is adjusted later in the loop to the end of the part
                AssertOrFailFast(start == 0 || fields[start - 1] != fields[start]);

                charcount_t end = start + 1;
                while (end < formattedLength && fields[end] == fields[start])
                {
                    ++end;
                }

                JavascriptString *partType = GetPartTypeString(fields[start]);
                JavascriptString *partValue = JavascriptString::NewCopyBuffer(formatted + start, end - start, sc);

                DynamicObject* part = sc->GetLibrary()->CreateObject();
                JavascriptOperators::InitProperty(part, PropertyIds::type, partType);
                JavascriptOperators::InitProperty(part, PropertyIds::value, partValue);

                ret->SetItem(retIndex, part, PropertyOperationFlags::PropertyOperation_None);

                start = end;
            }

            return ret;
        }
    };
#endif

    Var IntlEngineInterfaceExtensionObject::EntryIntl_FormatNumber(RecyclableObject* function, CallInfo callInfo, ...)
    {
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);

#if defined(INTL_ICU)
        INTL_CHECK_ARGS(
            args.Info.Count == 5 &&
            (TaggedInt::Is(args[1]) || JavascriptNumber::Is(args[1])) &&
            DynamicObject::IsBaseDynamicObject(args[2]) &&
            VarIs<JavascriptBoolean>(args[3]) &&
            VarIs<JavascriptBoolean>(args[4])
        );

        double num = JavascriptConversion::ToNumber(args[1], scriptContext);
        DynamicObject *state = UnsafeVarTo<DynamicObject>(args[2]);
        bool toParts = UnsafeVarTo<JavascriptBoolean>(args[3])->GetValue();
        bool forNumberPrototypeToLocaleString = UnsafeVarTo<JavascriptBoolean>(args[4])->GetValue();
        Var cachedUNumberFormat = nullptr; // cached by EntryIntl_CacheNumberFormat
        AssertOrFailFast(state->GetInternalProperty(state, InternalPropertyIds::CachedUNumberFormat, &cachedUNumberFormat, NULL, scriptContext));

        if (forNumberPrototypeToLocaleString)
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Number_Prototype_toLocaleString);
            INTL_TRACE("Calling %f.toLocaleString(...)", num);
        }
        else if (toParts)
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(NumberFormat_Prototype_formatToParts);
            INTL_TRACE("Calling NumberFormat.prototype.formatToParts(%f)", num);
        }
        else
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(NumberFormat_Prototype_format);
            INTL_TRACE("Calling NumberFormat.prototype.format(%f)", num);
        }

        auto fmt = static_cast<FinalizableUNumberFormat *>(cachedUNumberFormat);
        char16 *formatted = nullptr;
        int formattedLen = 0;

        if (!toParts)
        {
            EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
            {
                return unum_formatDouble(*fmt, num, buf, bufLen, nullptr, status);
            }, scriptContext->GetRecycler(), &formatted, &formattedLen);

            return JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext);
        }

#if defined(ICU_VERSION) && ICU_VERSION >= 61
        UErrorCode status = U_ZERO_ERROR;
        ScopedUFieldPositionIterator fpi(ufieldpositer_open(&status));

        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return unum_formatDoubleForFields(*fmt, num, buf, bufLen, fpi, status);
        }, scriptContext->GetRecycler(), &formatted, &formattedLen);

        NumberFormatPartsBuilder nfpb(num, formatted, formattedLen, scriptContext);

        int partStart = 0;
        int partEnd = 0;
        int i = 0;
        for (int kind = ufieldpositer_next(fpi, &partStart, &partEnd); kind >= 0; kind = ufieldpositer_next(fpi, &partStart, &partEnd), ++i)
        {
            nfpb.InsertPart(static_cast<UNumberFormatFields>(kind), partStart, partEnd);
        }

        return nfpb.ToPartsArray();
#else
        JavascriptLibrary *library = scriptContext->GetLibrary();
        JavascriptArray *ret = library->CreateArray(1);
        DynamicObject* part = library->CreateObject();
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return unum_formatDouble(*fmt, num, buf, bufLen, nullptr, status);
        }, scriptContext->GetRecycler(), &formatted, &formattedLen);
        JavascriptOperators::InitProperty(part, PropertyIds::type, library->GetIntlUnknownPartString());
        JavascriptOperators::InitProperty(part, PropertyIds::value, JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext));

        ret->SetItem(0, part, PropertyOperationFlags::PropertyOperation_None);
        return ret;
#endif // #if ICU_VERSION >= 61 ... #else
#else
        INTL_CHECK_ARGS(
            args.Info.Count == 3 &&
            (TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) &&
            DynamicObject::IsBaseDynamicObject(args.Values[2])
        );

        DynamicObject *options = VarTo<DynamicObject>(args.Values[2]);
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
    // Implementation of ECMA 262 #sec-timeclip
    // REVIEW(jahorto): Where is a better place for this function? JavascriptDate? DateUtilities? JavascriptConversion?
    static double TimeClip(Var x)
    {
        double time = 0.0;
        if (TaggedInt::Is(x))
        {
            time = TaggedInt::ToDouble(x);
        }
        else
        {
            AssertOrFailFast(JavascriptNumber::Is(x));
            time = JavascriptNumber::GetValue(x);

            // Only perform steps 1, 3, and 4 if the input was not a TaggedInt, since TaggedInts cant be infinite or -0
            if (!NumberUtilities::IsFinite(time))
            {
                return NumberConstants::NaN;
            }

            // This performs both steps 3 and 4
            time = JavascriptConversion::ToInteger(time);
        }

        // Step 2: If abs(time) > 8.64e15, return NaN.
        if (Math::Abs(time) > 8.64e15)
        {
            return NumberConstants::NaN;
        }

        return time;
    }

    static void AddPartToPartsArray(ScriptContext *scriptContext, JavascriptArray *arr, int arrIndex, const char16 *src, int start, int end, JavascriptString *partType)
    {
        JavascriptString *partValue = JavascriptString::NewCopyBuffer(
            src + start,
            end - start,
            scriptContext
        );

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
        if (args.Info.Count < 3 || !(TaggedInt::Is(args.Values[1]) || JavascriptNumber::Is(args.Values[1])) || !DynamicObject::IsBaseDynamicObject(args.Values[2]))
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

        DynamicObject* obj = VarTo<DynamicObject>(args.Values[2]);
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
            JavascriptString* userDefinedTimeZoneId = VarTo<JavascriptString>(propertyValue);
            IfFailThrowHr(WindowsCreateStringReference(userDefinedTimeZoneId->GetSz(), userDefinedTimeZoneId->GetLength(), &timeZoneHeader, &timeZone));
            Assert(timeZone);

            IfFailThrowHr(formatter->FormatUsingTimeZone(winDate, timeZone, &result));
        }
        PCWSTR strBuf = scriptContext->GetThreadContext()->GetWindowsGlobalizationLibrary()->WindowsGetStringRawBuffer(*result, NULL);

        return Js::JavascriptString::NewCopySz(strBuf, scriptContext);
#else
        // This function vaguely implements ECMA 402 #sec-partitiondatetimepattern
        INTL_CHECK_ARGS(
            args.Info.Count == 5 &&
            DynamicObject::IsBaseDynamicObject(args[1]) &&
            (TaggedInt::Is(args[2]) || JavascriptNumber::Is(args[2])) &&
            VarIs<JavascriptBoolean>(args[3]) &&
            VarIs<JavascriptBoolean>(args[4])
        );

        DynamicObject *state = UnsafeVarTo<DynamicObject>(args[1]);
        bool toParts = Js::UnsafeVarTo<Js::JavascriptBoolean>(args[3])->GetValue();

        // 1. Let x be TimeClip(x)
        // 2. If x is NaN, throw a RangeError exception
        double date = TimeClip(args[2]);
        if (JavascriptNumber::IsNan(date))
        {
            JavascriptError::ThrowRangeError(scriptContext, JSERR_InvalidDate);
        }

        bool forDatePrototypeToLocaleString = UnsafeVarTo<JavascriptBoolean>(args[4])->GetValue();
        if (forDatePrototypeToLocaleString)
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Date_Prototype_toLocaleString);
            INTL_TRACE("Calling new Date(%f).toLocaleString(...)", date);
        }
        else if (toParts)
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(DateTimeFormat_Prototype_formatToParts);
            INTL_TRACE("Calling DateTimeFormat.prototype.formatToParts(new Date(%f))", date);
        }
        else
        {
            CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(DateTimeFormat_Prototype_format);
            INTL_TRACE("Calling DateTimeFormat.prototype.format(new Date(%f))", date);
        }

        // Below, we lazy-initialize the backing UDateFormat on the first call to format{ToParts}
        // On subsequent calls, the UDateFormat will be cached in state.CachedUDateFormat
        Var cachedUDateFormat = nullptr;
        FinalizableUDateFormat *dtf = nullptr;
        UErrorCode status = U_ZERO_ERROR;
        if (state->GetInternalProperty(state, InternalPropertyIds::CachedUDateFormat, &cachedUDateFormat, nullptr, scriptContext))
        {
            dtf = reinterpret_cast<FinalizableUDateFormat *>(cachedUDateFormat);
            INTL_TRACE("Using previously cached UDateFormat (0x%x)", dtf);
        }
        else
        {
            JavascriptString *langtag = AssertStringProperty(state, PropertyIds::locale);
            JavascriptString *timeZone = AssertStringProperty(state, PropertyIds::timeZone);
            JavascriptString *pattern = AssertStringProperty(state, PropertyIds::pattern);

            char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
            LangtagToLocaleID(langtag, localeID);

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
            const char *calType = ucal_getType(cal, &status);
            ICU_ASSERT(status, calType != nullptr);
            if (strcmp(calType, "gregorian") == 0)
            {
                double beginningOfTime = -8.64e15;
                ucal_setGregorianChange(cal, beginningOfTime, &status);
                double actualGregorianChange = ucal_getGregorianChange(cal, &status);
                ICU_ASSERT(status, beginningOfTime == actualGregorianChange);
            }

            INTL_TRACE("Caching new UDateFormat (0x%x) with langtag=%s, pattern=%s, timezone=%s", dtf, langtag->GetSz(), pattern->GetSz(), timeZone->GetSz());

            // cache dtf for later use (so that the condition that brought us here returns true for future calls)
            state->SetInternalProperty(
                InternalPropertyIds::CachedUDateFormat,
                dtf,
                PropertyOperationFlags::PropertyOperation_None,
                nullptr
            );
        }

        // We intentionally special-case the following two calls to EnsureBuffer to allow zero-length strings.
        // See comment in GetPatternForSkeleton.

        char16 *formatted = nullptr;
        int formattedLen = 0;
        if (!toParts)
        {
            // if we aren't formatting to parts, we simply want to call udat_format with retry
            EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
            {
                return udat_format(*dtf, date, buf, bufLen, nullptr, status);
            }, scriptContext->GetRecycler(), &formatted, &formattedLen, /* allowZeroLengthStrings */ true);
            return JavascriptString::NewWithBuffer(formatted, formattedLen, scriptContext);
        }

        // The rest of this function most closely corresponds to ECMA 402 #sec-partitiondatetimepattern
        ScopedUFieldPositionIterator fpi(ufieldpositer_open(&status));
        ICU_ASSERT(status, true);
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return udat_formatForFields(*dtf, date, buf, bufLen, fpi, status);
        }, scriptContext->GetRecycler(), &formatted, &formattedLen, /* allowZeroLengthStrings */ true);

        JavascriptLibrary *library = scriptContext->GetLibrary();
        JavascriptArray* ret = library->CreateArray(0);

        int partStart = 0;
        int partEnd = 0;
        int lastPartEnd = 0;
        int i = 0;
        for (
            int kind = ufieldpositer_next(fpi, &partStart, &partEnd);
            kind >= 0;
            kind = ufieldpositer_next(fpi, &partStart, &partEnd), ++i
        )
        {
            Assert(partStart < partEnd && partEnd <= formattedLen);
            JavascriptString *typeString = nullptr;
            UDateFormatField fieldKind = (UDateFormatField)kind;
            switch (fieldKind)
            {
            case UDAT_ERA_FIELD:
                typeString = library->GetIntlEraPartString(); break;
            case UDAT_YEAR_FIELD:
            case UDAT_EXTENDED_YEAR_FIELD:
            case UDAT_YEAR_NAME_FIELD:
                typeString = library->GetIntlYearPartString(); break;
            case UDAT_MONTH_FIELD:
            case UDAT_STANDALONE_MONTH_FIELD:
                typeString = library->GetIntlMonthPartString(); break;
            case UDAT_DATE_FIELD:
                typeString = library->GetIntlDayPartString(); break;
            case UDAT_HOUR_OF_DAY1_FIELD:
            case UDAT_HOUR_OF_DAY0_FIELD:
            case UDAT_HOUR1_FIELD:
            case UDAT_HOUR0_FIELD:
                typeString = library->GetIntlHourPartString(); break;
            case UDAT_MINUTE_FIELD:
                typeString = library->GetIntlMinutePartString(); break;
            case UDAT_SECOND_FIELD:
                typeString = library->GetIntlSecondPartString(); break;
            case UDAT_DAY_OF_WEEK_FIELD:
            case UDAT_STANDALONE_DAY_FIELD:
            case UDAT_DOW_LOCAL_FIELD:
                typeString = library->GetIntlWeekdayPartString(); break;
            case UDAT_AM_PM_FIELD:
                typeString = library->GetIntlDayPeriodPartString(); break;
            case UDAT_TIMEZONE_FIELD:
            case UDAT_TIMEZONE_RFC_FIELD:
            case UDAT_TIMEZONE_GENERIC_FIELD:
            case UDAT_TIMEZONE_SPECIAL_FIELD:
            case UDAT_TIMEZONE_LOCALIZED_GMT_OFFSET_FIELD:
            case UDAT_TIMEZONE_ISO_FIELD:
            case UDAT_TIMEZONE_ISO_LOCAL_FIELD:
                typeString = library->GetIntlTimeZoneNamePartString(); break;
#if defined(ICU_VERSION) && ICU_VERSION == 55
            case UDAT_TIME_SEPARATOR_FIELD:
                // ICU 55 (Ubuntu 16.04 system default) has the ":" in "5:23 PM" as a special field
                // Intl should just treat this as a literal
                typeString = library->GetIntlLiteralPartString(); break;
#endif
            default:
                typeString = library->GetIntlUnknownPartString(); break;
            }

            if (partStart > lastPartEnd)
            {
                // formatForFields does not report literal fields directly, so we have to detect them
                // by seeing if the current part starts after the previous one ended
                AddPartToPartsArray(scriptContext, ret, i, formatted, lastPartEnd, partStart, library->GetIntlLiteralPartString());
                i += 1;
            }

            AddPartToPartsArray(scriptContext, ret, i, formatted, partStart, partEnd, typeString);
            lastPartEnd = partEnd;
        }

        // Sometimes, there can be a literal at the end of the string, such as when formatting just the year in
        // the chinese calendar, where the pattern string will be `r(U)`. The trailing `)` will be a literal
        if (lastPartEnd != formattedLen)
        {
            AssertOrFailFast(lastPartEnd < formattedLen);

            // `i` was incremented by the consequence of the last iteration of the for loop
            AddPartToPartsArray(scriptContext, ret, i, formatted, lastPartEnd, formattedLen, library->GetIntlLiteralPartString());
        }

        return ret;
#endif
    }

    Var IntlEngineInterfaceExtensionObject::EntryIntl_GetPatternForSkeleton(RecyclableObject *function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 3 && VarIs<JavascriptString>(args.Values[1]) && VarIs<JavascriptString>(args.Values[2]));

        JavascriptString *langtag = UnsafeVarTo<JavascriptString>(args.Values[1]);
        JavascriptString *skeleton = UnsafeVarTo<JavascriptString>(args.Values[2]);
        UErrorCode status = U_ZERO_ERROR;
        char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
        LangtagToLocaleID(langtag, localeID);

        // See https://github.com/tc39/ecma402/issues/225
        // When picking a format, we should be using the locale data of the basename of the resolved locale,
        // compared to when we actually format the date using the format string, where we use the full locale including extensions
        //
        // ECMA 402 #sec-initializedatetimeformat
        // 10: Let localeData be %DateTimeFormat%.[[LocaleData]].
        // 11: Let r be ResolveLocale( %DateTimeFormat%.[[AvailableLocales]], requestedLocales, opt, %DateTimeFormat%.[[RelevantExtensionKeys]], localeData).
        // 16: Let dataLocale be r.[[dataLocale]].
        // 23: Let dataLocaleData be localeData.[[<dataLocale>]].
        // 24: Let formats be dataLocaleData.[[formats]].
        char baseLocaleID[ULOC_FULLNAME_CAPACITY] = { 0 };
        int baseLocaleIDLength = uloc_getBaseName(localeID, baseLocaleID, _countof(baseLocaleID), &status);
        ICU_ASSERT(status, baseLocaleIDLength > 0 && baseLocaleIDLength < ULOC_FULLNAME_CAPACITY);

        INTL_TRACE("Converted input langtag '%s' to base locale ID '%S' for pattern generation", langtag->GetSz(), baseLocaleID);

        ScopedUDateTimePatternGenerator dtpg(udatpg_open(baseLocaleID, &status));
        ICU_ASSERT(status, true);

        char16 *formatted = nullptr;
        int formattedLen = 0;

        // OS#17513493 (OSS-Fuzz 7950): It is possible for the skeleton to be a zero-length string
        // because [[Get]] operations are performed on the options object twice, according to spec.
        // Follow-up spec discussion here: https://github.com/tc39/ecma402/issues/237.
        // We need to special-case this because calling udatpg_getBestPatternWithOptions on an empty skeleton
        // will produce an empty pattern, which causes an assert in EnsureBuffer by default.
        // As a result, we pass a final optional parameter to EnsureBuffer to say that zero-length results are OK.
        // TODO(jahorto): re-visit this workaround and the one in FormatDateTime upon resolution of the spec issue.
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return udatpg_getBestPatternWithOptions(
                dtpg,
                reinterpret_cast<const UChar *>(skeleton->GetSz()),
                skeleton->GetLength(),
                UDATPG_MATCH_ALL_FIELDS_LENGTH,
                buf,
                bufLen,
                status
            );
        }, scriptContext->GetRecycler(), &formatted, &formattedLen, /* allowZeroLengthStrings */ true);

        INTL_TRACE("Best pattern '%s' will be used for skeleton '%s' and langtag '%s'", formatted, skeleton->GetSz(), langtag->GetSz());

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
        INTL_CHECK_ARGS(args.Info.Count == 2 && VarIs<JavascriptString>(args.Values[1]));

        JavascriptString *tz = VarTo<JavascriptString>(args.Values[1]);

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
        UChar match[100] = { 0 };
        for (int a = 0; a < availableLength; ++a)
        {
            int curLen = -1;
            const UChar *cur = uenum_unext(available, &curLen, &status);
            ICU_ASSERT(status, true);
            if (curLen == 0)
            {
                // OS#17175014: in rare cases, ICU will return U_ZERO_ERROR and a valid `cur` string but a length of 0
                // This only happens in an OOM during uenum_(u)next
                // Tracked by ICU: http://bugs.icu-project.org/trac/ticket/13739
                Throw::OutOfMemory();
            }

            AssertOrFailFast(curLen > 0);
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
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return ucal_getDefaultTimeZone(buf, bufLen, status);
        }, scriptContext->GetRecycler(), &timeZone, &timeZoneLen);
        return JavascriptString::NewWithBuffer(timeZone, timeZoneLen, scriptContext);
#endif
    }

#ifdef INTL_ICU
    static FinalizableUPluralRules *GetOrCreateCachedUPluralRules(DynamicObject *stateObject, ScriptContext *scriptContext)
    {
        Var cachedUPluralRules = nullptr;
        FinalizableUPluralRules *pr = nullptr;
        if (stateObject->GetInternalProperty(stateObject, InternalPropertyIds::CachedUPluralRules, &cachedUPluralRules, nullptr, scriptContext))
        {
            pr = reinterpret_cast<FinalizableUPluralRules *>(cachedUPluralRules);
            INTL_TRACE("Using previously cached UPluralRules (0x%x)", pr);
        }
        else
        {
            UErrorCode status = U_ZERO_ERROR;

            JavascriptString *langtag = AssertStringProperty(stateObject, PropertyIds::locale);
            JavascriptString *type = AssertStringProperty(stateObject, PropertyIds::type);

            UPluralType prType = UPLURAL_TYPE_CARDINAL;
            if (wcscmp(type->GetSz(), _u("ordinal")) == 0)
            {
                prType = UPLURAL_TYPE_ORDINAL;
            }
            else
            {
                AssertOrFailFast(wcscmp(type->GetSz(), _u("cardinal")) == 0);
            }

            char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
            LangtagToLocaleID(langtag, localeID);

            pr = FinalizableUPluralRules::New(scriptContext->GetRecycler(), uplrules_openForType(localeID, prType, &status));
            ICU_ASSERT(status, true);

            INTL_TRACE("Caching UPluralRules object (0x%x) with langtag %s and type %s", pr, langtag->GetSz(), type->GetSz());

            stateObject->SetInternalProperty(InternalPropertyIds::CachedUPluralRules, pr, PropertyOperationFlags::PropertyOperation_None, nullptr);
        }

        return pr;
    }
#endif

    // This method implements Step 13 and 14 of ECMA 402 #sec-initializepluralrules
    Var IntlEngineInterfaceExtensionObject::EntryIntl_PluralRulesKeywords(RecyclableObject *function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 2 && DynamicObject::IsBaseDynamicObject(args[1]));

        JavascriptArray *ret = scriptContext->GetLibrary()->CreateArray(0);

        // uplrules_getKeywords is only stable since ICU 61.
        // For ICU < 61, we can fake it by creating an array of ["other"], which
        // uplrules_getKeywords is guaranteed to return at minimum.
        // This array is only used in resolved options, so the majority of the functionality can remain (namely, select() still works)
#if defined(ICU_VERSION) && ICU_VERSION >= 61
        DynamicObject *state = UnsafeVarTo<DynamicObject>(args[1]);
        FinalizableUPluralRules *pr = GetOrCreateCachedUPluralRules(state, scriptContext);

        UErrorCode status = U_ZERO_ERROR;
        ScopedUEnumeration keywords(uplrules_getKeywords(*pr, &status));
        ICU_ASSERT(status, true);

        ForEachUEnumeration16(keywords, [&](int index, const char16 *kw, charcount_t kwLength)
        {
            ret->SetItem(index, JavascriptString::NewCopyBuffer(kw, kwLength, scriptContext), PropertyOperation_None);
        });
#else
        ret->SetItem(0, scriptContext->GetLibrary()->GetIntlPluralRulesOtherString(), PropertyOperation_None);
#endif

        return ret;
#else
        AssertOrFailFastMsg(false, "Intl-WinGlob should not be using PluralRulesKeywords");
        return nullptr;
#endif
    }

    // This method roughly implements ECMA 402 #sec-pluralruleselect, except ICU takes care of handling the operand logic for us
    Var IntlEngineInterfaceExtensionObject::EntryIntl_PluralRulesSelect(RecyclableObject *function, CallInfo callInfo, ...)
    {
#ifdef INTL_ICU
        EngineInterfaceObject_CommonFunctionProlog(function, callInfo);
        INTL_CHECK_ARGS(args.Info.Count == 3 && DynamicObject::IsBaseDynamicObject(args[1]));

        DynamicObject *state = UnsafeVarTo<DynamicObject>(args[1]);
        double n = 0.0;
        if (TaggedInt::Is(args[2]))
        {
            n = TaggedInt::ToDouble(args[2]);
        }
        else
        {
            AssertOrFailFast(JavascriptNumber::Is(args[2]));
            n = JavascriptNumber::GetValue(args[2]);
        }

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(PluralRules_Prototype_select);
        INTL_TRACE("Calling PluralRules.prototype.select(%f)", n);

        UErrorCode status = U_ZERO_ERROR;

        FinalizableUPluralRules *pr = GetOrCreateCachedUPluralRules(state, scriptContext);

        // ICU has an internal API, uplrules_selectWithFormat, that is equivalent to uplrules_select but will respect digit options of the passed UNumberFormat.
        // Since its an internal API, we can't use it -- however, we can work around it by creating a UNumberFormat with provided digit options,
        // formatting the requested number to a string, and then converting the string back to a double which we can pass to uplrules_select.
        // This workaround was suggested during the May 2018 ECMA-402 discussion.
        // The below is similar to GetOrCreateCachedUPluralRules, but since creating a UNumberFormat for Intl.NumberFormat is much more involved and no one else
        // uses this functionality, it makes more sense to me to just put the logic inline.
        Var cachedUNumberFormat = nullptr;
        FinalizableUNumberFormat *nf = nullptr;
        if (state->GetInternalProperty(state, InternalPropertyIds::CachedUNumberFormat, &cachedUNumberFormat, nullptr, scriptContext))
        {
            nf = reinterpret_cast<FinalizableUNumberFormat *>(cachedUNumberFormat);
            INTL_TRACE("Using previously cached UNumberFormat (0x%x)", nf);
        }
        else
        {
            char localeID[ULOC_FULLNAME_CAPACITY] = { 0 };
            LangtagToLocaleID(AssertStringProperty(state, PropertyIds::locale), localeID);
            nf = FinalizableUNumberFormat::New(scriptContext->GetRecycler(), unum_open(UNUM_DECIMAL, nullptr, 0, localeID, nullptr, &status));

            SetUNumberFormatDigitOptions(*nf, state);

            INTL_TRACE("Caching UNumberFormat object (0x%x) with localeID %S", nf, localeID);

            state->SetInternalProperty(InternalPropertyIds::CachedUNumberFormat, nf, PropertyOperationFlags::PropertyOperation_None, nullptr);
        }

        char16 *formattedN = nullptr;
        int formattedNLength = 0;
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return unum_formatDouble(*nf, n, buf, bufLen, nullptr, status);
        }, scriptContext->GetRecycler(), &formattedN, &formattedNLength);

        double nWithOptions = unum_parseDouble(*nf, reinterpret_cast<UChar *>(formattedN), formattedNLength, nullptr, &status);
        double roundtripDiff = n - nWithOptions;
        ICU_ASSERT(status, roundtripDiff <= 1.0 && roundtripDiff >= -1.0);

        char16 *selected = nullptr;
        int selectedLength = 0;
        EnsureBuffer([&](UChar *buf, int bufLen, UErrorCode *status)
        {
            return uplrules_select(*pr, nWithOptions, buf, bufLen, status);
        }, scriptContext->GetRecycler(), &selected, &selectedLength);

        return JavascriptString::NewWithBuffer(selected, static_cast<charcount_t>(selectedLength), scriptContext);
#else
        AssertOrFailFastMsg(false, "Intl-WinGlob should not be using PluralRulesSelect");
        return nullptr;
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

        // This function will only be used during the construction of the Intl object, hence Asserts are in place.
        Assert(args.Info.Count >= 3 && VarIs<JavascriptFunction>(args.Values[1]) && TaggedInt::Is(args.Values[2]));

        JavascriptFunction *func = VarTo<JavascriptFunction>(args.Values[1]);
        int32 id = TaggedInt::ToInt32(args.Values[2]);
        Assert(id >= 0 && id < (int32)BuiltInFunctionID::Max);

        EngineInterfaceObject* nativeEngineInterfaceObj = scriptContext->GetLibrary()->GetEngineInterfaceObject();
        IntlEngineInterfaceExtensionObject* extensionObject = static_cast<IntlEngineInterfaceExtensionObject*>(nativeEngineInterfaceObj->GetEngineExtension(EngineInterfaceExtensionKind_Intl));

        BuiltInFunctionID functionID = static_cast<BuiltInFunctionID>(id);
        switch (functionID)
        {
        case BuiltInFunctionID::DateToLocaleString:
            extensionObject->dateToLocaleString = func;
            break;
        case BuiltInFunctionID::DateToLocaleDateString:
            extensionObject->dateToLocaleDateString = func;
            break;
        case BuiltInFunctionID::DateToLocaleTimeString:
            extensionObject->dateToLocaleTimeString = func;
            break;
        case BuiltInFunctionID::NumberToLocaleString:
            extensionObject->numberToLocaleString = func;
            break;
        case BuiltInFunctionID::StringLocaleCompare:
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

        if (callInfo.Count < 2 || !DynamicObject::IsBaseDynamicObject(args.Values[1]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = VarTo<DynamicObject>(args.Values[1]);
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

        if (callInfo.Count < 3 || !DynamicObject::IsBaseDynamicObject(args.Values[1]) || !DynamicObject::IsBaseDynamicObject(args.Values[2]))
        {
            return scriptContext->GetLibrary()->GetUndefined();
        }

        DynamicObject* obj = VarTo<DynamicObject>(args.Values[1]);
        DynamicObject* value = VarTo<DynamicObject>(args.Values[2]);

        if (obj->SetInternalProperty(Js::InternalPropertyIds::HiddenObject, value, Js::PropertyOperationFlags::PropertyOperation_None, NULL))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }
        else
        {
            return scriptContext->GetLibrary()->GetFalse();
        }
    }
#endif
}
#endif
