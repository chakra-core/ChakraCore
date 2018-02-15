//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// INTL_ENTRY is a macro that should be defined before this file is included
// It is intended to be passed a Js::PropertyId as the first argument and the corresponding unique substring of a method name as the second argument
#ifndef INTL_ENTRY
#error INTL_ENTRY must be defiend before including __FILE__
#endif

#ifdef CompareString
#undef CompareString
#endif

INTL_ENTRY(raiseAssert, RaiseAssert)
INTL_ENTRY(isWellFormedLanguageTag, IsWellFormedLanguageTag)
INTL_ENTRY(normalizeLanguageTag, NormalizeLanguageTag)
INTL_ENTRY(isDTFLocaleAvailable, IsDTFLocaleAvailable)
INTL_ENTRY(isCollatorLocaleAvailable, IsCollatorLocaleAvailable)
INTL_ENTRY(isNFLocaleAvailable, IsNFLocaleAvailable)
INTL_ENTRY(resolveLocaleLookup, ResolveLocaleLookup)
INTL_ENTRY(resolveLocaleBestFit, ResolveLocaleBestFit)
INTL_ENTRY(getDefaultLocale, GetDefaultLocale)
INTL_ENTRY(getExtensions, GetExtensions)
INTL_ENTRY(compareString, CompareString)
INTL_ENTRY(formatNumber, FormatNumber)
INTL_ENTRY(cacheNumberFormat, CacheNumberFormat)
INTL_ENTRY(createDateTimeFormat, CreateDateTimeFormat)
INTL_ENTRY(currencyDigits, CurrencyDigits)
INTL_ENTRY(formatDateTime, FormatDateTime)
INTL_ENTRY(validateAndCanonicalizeTimeZone, ValidateAndCanonicalizeTimeZone)
INTL_ENTRY(getDefaultTimeZone, GetDefaultTimeZone)
INTL_ENTRY(getPatternForSkeleton, GetPatternForSkeleton)
INTL_ENTRY(getLocaleData, GetLocaleData)

INTL_ENTRY(registerBuiltInFunction, RegisterBuiltInFunction)
INTL_ENTRY(getHiddenObject, GetHiddenObject)
INTL_ENTRY(setHiddenObject, SetHiddenObject)
INTL_ENTRY(builtInSetPrototype, BuiltIn_SetPrototype)
INTL_ENTRY(builtInGetArrayLength, BuiltIn_GetArrayLength)
INTL_ENTRY(builtInRegexMatch, BuiltIn_RegexMatch)
INTL_ENTRY(builtInCallInstanceFunction, BuiltIn_CallInstanceFunction)
