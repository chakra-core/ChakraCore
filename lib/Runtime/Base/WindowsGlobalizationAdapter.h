//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_ES6_CHAR_CLASSIFIER)

#ifdef INTL_ICU
#include <CommonPal.h>
#define U_STATIC_IMPLEMENTATION
#define U_SHOW_CPLUSPLUS_API 0
#pragma warning(push)
#pragma warning(disable:4995) // deprecation warning
#include <unicode/uloc.h>
#pragma warning(pop)
#endif // INTL_ICU

#ifdef INTL_WINGLOB
#include "Windows.Globalization.h"
#ifndef NTBUILD
#include "windows.globalization.numberformatting.h"
#include "windows.globalization.datetimeformatting.h"
#include "Windows.Data.Text.h"
#include "activation.h"
using namespace ABI;
#endif // NTBUILD
#endif // INTL_WINGLOB

class ThreadContext;

namespace Js
{
#ifdef INTL_WINGLOB
    class WindowsGlobalizationAdapter
    {
    private:
        bool initializedCommonGlobObjects;
        HRESULT hrForCommonGlobObjectsInit;

#ifdef ENABLE_INTL_OBJECT
        bool initializedDateTimeFormatObjects;
        HRESULT hrForDateTimeFormatObjectsInit;

        bool initializedNumberFormatObjects;
        HRESULT hrForNumberFormatObjectsInit;
#endif

#if ENABLE_UNICODE_API
        bool initializedCharClassifierObjects;
        HRESULT hrForCharClassifierObjectsInit;
#endif

#ifdef ENABLE_INTL_OBJECT
        // Common
        AutoCOMPtr<Windows::Globalization::ILanguageFactory> languageFactory;
        AutoCOMPtr<Windows::Globalization::ILanguageStatics> languageStatics;
        AutoCOMPtr<Windows::Globalization::DateTimeFormatting::IDateTimeFormatterFactory> dateTimeFormatterFactory;

        // DateTimeFormat
        AutoCOMPtr<Windows::Globalization::ICalendarFactory> calendarFactory;
        AutoCOMPtr<Windows::Globalization::ITimeZoneOnCalendar> timeZoneCalendar; // use to validate timeZone
        AutoCOMPtr<Windows::Globalization::ITimeZoneOnCalendar> defaultTimeZoneCalendar; // default current time zone

        // NumberFormat
        AutoCOMPtr<Windows::Globalization::NumberFormatting::ICurrencyFormatterFactory> currencyFormatterFactory;
        AutoCOMPtr<Windows::Globalization::NumberFormatting::IDecimalFormatterFactory> decimalFormatterFactory;
        AutoCOMPtr<Windows::Globalization::NumberFormatting::IPercentFormatterFactory> percentFormatterFactory;
        AutoCOMPtr<IActivationFactory> incrementNumberRounderActivationFactory;
        AutoCOMPtr<IActivationFactory> significantDigitsRounderActivationFactory;
#endif
        // CharClassifier
        AutoCOMPtr<Windows::Data::Text::IUnicodeCharactersStatics> unicodeStatics;

        DelayLoadWindowsGlobalization* GetWindowsGlobalizationLibrary(_In_ ScriptContext* scriptContext);
        DelayLoadWindowsGlobalization* GetWindowsGlobalizationLibrary(_In_ ThreadContext* threadContext);
        template <typename T>
        HRESULT GetActivationFactory(DelayLoadWindowsGlobalization *library, LPCWSTR factoryName, T** instance);

    public:
        WindowsGlobalizationAdapter()
            : initializedCommonGlobObjects(false),
            hrForCommonGlobObjectsInit(S_OK),
#ifdef ENABLE_INTL_OBJECT
            initializedDateTimeFormatObjects(false),
            hrForDateTimeFormatObjectsInit(S_OK),
            initializedNumberFormatObjects(false),
            hrForNumberFormatObjectsInit(S_OK),
#endif
#if ENABLE_UNICODE_API
            initializedCharClassifierObjects(false),
            hrForCharClassifierObjectsInit(S_OK),
#endif
#ifdef ENABLE_INTL_OBJECT
            languageFactory(nullptr),
            languageStatics(nullptr),
            dateTimeFormatterFactory(nullptr),
            calendarFactory(nullptr),
            timeZoneCalendar(nullptr),
            defaultTimeZoneCalendar(nullptr),
            currencyFormatterFactory(nullptr),
            decimalFormatterFactory(nullptr),
            percentFormatterFactory(nullptr),
            incrementNumberRounderActivationFactory(nullptr),
            significantDigitsRounderActivationFactory(nullptr),
#endif // ENABLE_INTL_OBJECT
            unicodeStatics(nullptr)
        { }

#ifdef ENABLE_INTL_OBJECT
        HRESULT EnsureCommonObjectsInitialized(DelayLoadWindowsGlobalization *library);
        HRESULT EnsureDateTimeFormatObjectsInitialized(DelayLoadWindowsGlobalization *library);
        HRESULT EnsureNumberFormatObjectsInitialized(DelayLoadWindowsGlobalization *library);
#endif
#if ENABLE_UNICODE_API
        HRESULT EnsureDataTextObjectsInitialized(DelayLoadWindowsGlobalization *library);
#endif
#ifdef ENABLE_INTL_OBJECT
        HRESULT CreateLanguage(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR languageTag, Windows::Globalization::ILanguage** language);
        boolean IsWellFormedLanguageTag(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR languageTag);
        HRESULT NormalizeLanguageTag(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR languageTag, HSTRING *result);
        HRESULT CreateCurrencyFormatterCode(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR currencyCode, Windows::Globalization::NumberFormatting::ICurrencyFormatter** currencyFormatter);
        HRESULT CreateCurrencyFormatter(_In_ ScriptContext* scriptContext, PCWSTR* localeStrings, uint32 numLocaleStrings, _In_z_ PCWSTR currencyCode, Windows::Globalization::NumberFormatting::ICurrencyFormatter** currencyFormatter);
        HRESULT CreateNumberFormatter(_In_ ScriptContext* scriptContext, PCWSTR* localeStrings, uint32 numLocaleStrings, Windows::Globalization::NumberFormatting::INumberFormatter** numberFormatter);
        HRESULT CreatePercentFormatter(_In_ ScriptContext* scriptContext, PCWSTR* localeStrings, uint32 numLocaleStrings, Windows::Globalization::NumberFormatting::INumberFormatter** numberFormatter);
        HRESULT CreateDateTimeFormatter(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR formatString, _In_z_ PCWSTR* localeStrings, uint32 numLocaleStrings,
            _In_opt_z_ PCWSTR calendar, _In_opt_z_ PCWSTR clock, _Out_ Windows::Globalization::DateTimeFormatting::IDateTimeFormatter** formatter);
        HRESULT CreateIncrementNumberRounder(_In_ ScriptContext* scriptContext, Windows::Globalization::NumberFormatting::INumberRounder** numberRounder);
        HRESULT CreateSignificantDigitsRounder(_In_ ScriptContext* scriptContext, Windows::Globalization::NumberFormatting::INumberRounder** numberRounder);
        boolean ValidateAndCanonicalizeTimeZone(_In_ ScriptContext* scriptContext, _In_z_ PCWSTR timeZoneId, HSTRING* result);
        HRESULT GetDefaultTimeZoneId(_In_ ScriptContext* scriptContext, HSTRING* result);
        HRESULT GetResolvedLanguage(_In_ Windows::Globalization::DateTimeFormatting::IDateTimeFormatter* formatter, HSTRING * locale);
        HRESULT GetResolvedLanguage(_In_ Windows::Globalization::NumberFormatting::INumberFormatterOptions* formatter, HSTRING * locale);
        HRESULT GetNumeralSystem(_In_ Windows::Globalization::DateTimeFormatting::IDateTimeFormatter* formatter, HSTRING * hNumeralSystem);
        HRESULT GetNumeralSystem(_In_ Windows::Globalization::NumberFormatting::INumberFormatterOptions* formatter, HSTRING * hNumeralSystem);
        HRESULT GetCalendar(_In_ Windows::Globalization::DateTimeFormatting::IDateTimeFormatter* formatter, HSTRING * hCalendar);
        HRESULT GetClock(_In_ Windows::Globalization::DateTimeFormatting::IDateTimeFormatter* formatter, HSTRING * hClock);
        HRESULT GetItemAt(_In_ Windows::Foundation::Collections::IVectorView<HSTRING>* vector, _In_ uint32 index, HSTRING * item);
        void ResetCommonFactoryObjects();
        void ResetTimeZoneFactoryObjects();
        void ResetDateTimeFormatFactoryObjects();
        void ResetNumberFormatFactoryObjects();
#endif // ENABLE_INTL_OBJECT
#if ENABLE_UNICODE_API
        // This is currently used only be the Platform Agnostic Layer
        // TODO: Move the management of this interface to the Platform Agnostic Interface Layer
        // TODO: Subsume the Windows Globalization Adapter into the Platform Agnostic Interface Layer
        Windows::Data::Text::IUnicodeCharactersStatics* GetUnicodeStatics()
        {
            return unicodeStatics;
        }
#endif
#ifdef ENABLE_INTL_OBJECT 
    private:
        HRESULT CreateTimeZoneOnCalendar(_In_ DelayLoadWindowsGlobalization *library, __out Windows::Globalization::ITimeZoneOnCalendar**  result);
        static HRESULT VerifyResult(HSTRING * result, HRESULT errCode);
#endif
    };
#endif // INTL_WINGLOB

#ifdef INTL_ICU
#ifdef ENABLE_INTL_OBJECT
    class IcuIntlAdapter
    {
    public:
        static bool IsWellFormedLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch);
        static HRESULT NormalizeLanguageTag(_In_z_ const char16 *languageTag, _In_ const charcount_t cch,
            _Out_ char16 *normalized, _Out_ size_t *normalizedLength);

        //static bool ResolveLocaleLookup(_In_ ScriptContext *scriptContext, _In_ JavascriptString *locale, _Out_ char16 *resolved);
        static bool ResolveLocaleLookup(_In_ ScriptContext *scriptContext, _In_z_ const char16 *locale, _Out_ char16 *resolved);
        //static bool ResolveLocaleBestFit(_In_ ScriptContext *scriptContext, _In_ JavascriptString *locale, _Out_ char16 *resolved);
        static bool ResolveLocaleBestFit(_In_ ScriptContext *scriptContext, _In_z_ const char16 *locale, _Out_ char16 *resolved);
        static int GetUserDefaultLocaleName(_Out_ LPWSTR lpLocaleName, _In_ int cchLocaleName);
    };
#endif // ENABLE_INTL_OBJECT
#endif // INTL_ICU
}
#endif // defined(ENABLE_INTL_OBJECT) || defined(ENABLE_ES6_CHAR_CLASSIFIER)
