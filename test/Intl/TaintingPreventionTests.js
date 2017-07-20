//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var err = new Error();
var someDate = new Date(2000, 1, 1, 1, 1, 1);
function throwFunction() {
    throw err;
}

//To see if Intl tainting causes some internal issues.
var myIntl = Intl;

var functionCalls = [
    function () { return new myIntl.Collator(); },
    function () { return new myIntl.DateTimeFormat(); },
    function () { return new myIntl.NumberFormat(); },
    function () { return myIntl.Collator.supportedLocalesOf(); },
    function () { return new myIntl.Collator().compare("a", "b"); },
    function () { return new myIntl.NumberFormat().format("5.5555555"); },
    function () { return new myIntl.DateTimeFormat().format(someDate); },
    function () { return new myIntl.Collator().resolvedOptions(); },
    function () { return new myIntl.NumberFormat().resolvedOptions(); },
    function () { return new myIntl.DateTimeFormat().resolvedOptions(); },
    function () { try { new myIntl.Collator("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz") } catch (e) { return e.message; } },
    function () { try { new myIntl.NumberFormat.supportedLocalesOf(null) } catch (e) { return e.message; } },
];

function verifyGlobalTainting(global, func) {
    var original = this[global];
    this[global] = throwFunction;
    try {
        var result = func();
        this[global] = original;
        return result;
    } catch (e) {
        this[global] = original;
        throw e;
    }
}

Array.prototype.tryGlobalTainting = function (global) {
    for (var i = 0; i < this.length; i++) {
        try {
            assert.areEqual(verifyGlobalTainting(global, this[i]), this[i](), "Tainting Global '" + global + "'.");
        }
        catch (e) {
            assert.fail("Error thrown when testing Global '" + global + "'. Message: " + e + " \nStack:\n" + e.stack);
        }
    }
};

function verifyPropertyTainting(property, func, attributes) {
    try {
        attributes.configurable = true;
        Object.defineProperty(Object.prototype, property, attributes);
        var result = func();
        delete Object.prototype[property];
        return result;
    }
    catch (e) {
        delete Object.prototype[property];
        throw e;
    }
}

Array.prototype.tryUsingGettersSetters = function (property) {
    for (var i = 0; i < this.length; i++) {
        try {
            assert.areEqual(verifyPropertyTainting(property, this[i], { get: throwFunction }), this[i](), "Tainting property tainting '" + property + "' with getter.");
            assert.areEqual(verifyPropertyTainting(property, this[i], { set: throwFunction }), this[i](), "Tainting property tainting '" + property + "' with setter.");
            assert.areEqual(verifyPropertyTainting(property, this[i], { value: throwFunction }), this[i](), "Tainting property tainting '" + property + "' with value.");
        }
        catch (e) {
            assert.fail("Error thrown when testing Global '" + property + "'. Message: " + e.message + "\n" + this[i] + "\nStack:" + e.stack);
        }
    }
};

var globalsList = ["Error", "TypeError", "RangeError", "Math", "Object", "String", "Number", "Date", "RegExp", "Intl", "Boolean", "Array", "Function"];

//These are almost all the keywords from Intl.js; to test
var keywords = ["__boundCompare", "__boundCompare", "__boundFormat", "__calendar", "__caseFirst", "__collation", "__currency", "__currencyDisplay", "__currencyDisplayToUse", "__day",
    "__era", "__formatMatcher", "__formatterToUse", "__hour", "__hour12", "__ignorePunctuation", "__initializedCollator", "__initializedDateTimeFormat", "__initializedIntlObject",
    "__initializedNumberFormat", "__locale", "__localeForCompare", "__localeMatcher", "__matcher", "__maximumFractionDigits", "__maximumSignificantDigits", "__minimumFractionDigits",
    "__minimumIntegerDigits", "__minimumSignificantDigits", "__minute", "__month", "__numberingSystem", "__numeric", "__patternStrings", "__relevantExtensionKeys", "__second",
    "__sensitivity", "__style", "__templateString", "__timeZone", "__timeZoneName", "__usage", "__useGrouping", "__weekday", "__windowsCalendar", "__windowsClock", "__year",
    "12HourClock", "24HourClock", "2-digit", "A", "abbreivated", "abbreviated", "abs", "accent", "accepedCollationForLocale", "all", "always", "and", "any", "are", "arguments",
    "ArrayInstanceForEach", "ArrayInstanceIndexOf", "ArrayInstanceJoin", "ArrayInstancePush", "as", "availableBcpTags", "b", "base", "basic", "bcp47Tag", "bcpTag", "be", "best",
    "Boolean", "builtInCallInstanceFunction", "builtInGetArrayLength", "builtInGlobalObjectEntryIsFinite", "builtInGlobalObjectEntryIsNaN", "builtInJavascriptArrayEntryForEach",
    "builtInJavascriptArrayEntryIndexOf", "builtInJavascriptArrayEntryJoin", "builtInJavascriptArrayEntryPush", "builtInJavascriptDateEntryGetDate", "builtInJavascriptDateEntryNow",
    "builtInJavascriptFunctionEntryBind", "builtInJavascriptObjectEntryDefineProperty", "builtInJavascriptObjectEntryGetOwnPropertyNames", "builtInJavascriptObjectEntryGetPrototypeOf",
    "builtInJavascriptObjectEntryHasOwnProperty", "builtInJavascriptObjectEntryIsExtensible", "builtInJavascriptRegExpEntryTest", "builtInJavascriptStringEntryMatch",
    "builtInJavascriptStringEntryReplace", "builtInJavascriptStringEntryToLowerCase", "builtInJavascriptStringEntryToUpperCase", "builtInMathAbs", "builtInMathFloor",
    "builtInMathMax", "builtInMathPow", "builtInSetPrototype", "ca", "cacheNumberFormat", "calendar", "callInstanceFunc", "CanonicalizeLocaleList", "case", "caseFirst",
    "catch", "change", "co", "code", "collation", "collationAugmentedLocale", "Collator", "compare", "compareString", "configurable", "constructor", "correct", "correctDayHourMinuteSecondMonthPattern",
    "correcting", "correctWeekdayEraMonthPattern", "count", "create", "createDateTimeFormat", "currency", "currencyCodeRE", "currencyDigits", "currencyDisplay", "current", "Date", "DateInstanceGetDate",
    "DateNow", "DateTimeFormat", "day", "dayofweek", "decides", "decimal", "de-DE", "default", "defaultLocale", "defaultLocaleFunc", "defaults", "digits", "don", "e", "EcmaOptionsToWindowsTemplate",
    "else", "EngineInterface", "Era", "Error", "es-ES", "ex", "extensionFilter", "fallback", "filtered", "fine", "fit", "fitter", "floor", "for", "forEach", "format", "formatDateTime",
    "formatMatcher", "formatNumber", "formattedValue", "formatterToUse", "full", "func", "function", "FunctionInstanceBind", "functionName", "getArrayLength", "getDefaultLocale",
    "getExtensions", "getHiddenObject", "GetNumberOption", "GetOption", "give", "givenLocales", "granularity", "GregorianCalendar", "gregory", "hand", "HasProperty", "have", "hebrew", "HebrewCalendar",
    "HH", "hiddenObject", "hidePlatform", "HijriCalendar", "hour", "hour12", "how", "i", "if", "ignorePunctuation", "in", "InitializeCollator", "InitializeDateTimeFormat", "InitializeNumberFormat",
    "instanceof", "integer", "Internal", "Intl", "Invalid", "is", "isFinite", "islamic", "islamic-civil", "isNaN", "isValid", "IsWellFormedCurrencyCode", "isWellFormedLanguageTag", "ja-JP",
    "japanese", "JapaneseCalendar", "julian", "JulianCalendar", "key", "kf", "kn", "korean", "KoreanCalendar", "length", "locale", "localeCompare", "localeList", "localeMatcher", "locales",
    "localesAcceptingCollationValues", "localeWithoutSubtags", "long", "looking", "lookup", "LookupSupportedLocales", "lower", "lv-LV", "mappedDefaultLocale", "matcher", "Math", "max",
    "maximum", "maximumFractionDigits", "maximumFractionDigitsDefault", "maximumSignificantDigits", "message", "minimum", "minimumFractionDigits", "minimumIntegerDigits", "minimumSignificantDigits",
    "minute", "mm", "month", "n", "name", "NaN", "narrow", "necessary", "needDefaults", "new", "newValues", "nInput", "normalizeLanguageTag", "not", "NOTE", "nRegex", "nu", "null", "num",
    "Number", "NumberFormat", "numberingSystem", "numeric", "o", "obj", "Object", "ObjectDefineProperty", "ObjectGetOwnPropertyNames", "ObjectGetPrototypeOf", "ObjectInstanceHasOwnProperty",
    "ObjectIsExtensible", "ObjectPrototype", "On", "opt", "option", "options", "other", "p", "parts", "pattern", "patternString", "percent", "phoneb", "phonebk", "phonetic", "pinyin",
    "platform", "position-sensitive", "pow", "pronun", "prop", "property", "prototype", "radstr", "RaiseAssert", "raiseInvalidCurrencyCode", "raiseInvalidDate", "raiseLocaleNotWellFormed",
    "raiseMissingCurrencyCode", "raiseNeedObject", "raiseNeedObjectOfType", "raiseNeedObjectOrString", "raiseNotAConstructor", "raiseObjectIsAlreadyInitialized", "raiseObjectIsNonExtensible",
    "raiseOptionValueOutOfRange", "raiseOptionValueOutOfRange_3", "raiseThis_NullOrUndefined", "rawValue", "redundant", "regex", "RegExp", "RegExpInstanceTest", "registerBuiltInFunction",
    "requestedLocales", "required", "resolved", "resolvedLocale", "resolvedLocaleInfo", "resolvedLocaleLookup", "resolvedOptions", "resolveLocaleBestFit", "resolveLocaleHelper", "resolveLocaleLookup",
    "resolveLocales", "results", "ret", "return", "returned", "reverseLocaleAcceptingCollationValues", "rror", "s", "search", "searchParam", "second", "seen", "sensitivity",
    "setHiddenObject", "setPrototype", "short", "shortLength", "should", "solo", "sort", "speicfy", "standard", "strict", "String", "StringInstanceMatch", "StringInstanceReplace",
    "StringInstanceToLowerCase", "StringInstanceToUpperCase", "strings", "strippedDefaultLocale", "stroke", "style", "subset", "subTag", "subTags", "SupportedLocalesOf",
    "supportedLocalesOfThisArg", "symbol", "t", "tag", "taiwan", "TaiwanCalendar", "temp", "template", "templates", "templateString", "thai", "ThaiCalendar", "that", "the", "then",
    "Therefore", "this", "Thus", "time", "timeZone", "timeZoneName", "to", "ToDateTimeOptions", "toLocaleDateString", "toLocaleString", "toLocaleTimeString", "ToLogicalBoolean",
    "ToNumber", "ToObject", "toReturn", "ToString", "ToUint32", "trad", "tradnl", "try", "twice", "type", "typeof", "-u-", "-u-co-", "UmAlQuraCalendar", "undefined", "unihan",
    "updatePatternStrings", "upper", "usage", "use", "useGrouping", "userValue", "using", "UTC", "v", "Valid", "value", "values", "var", "variant", "was", "we", "weekday", "when",
    "while", "will", "windows", "windowsClock", "windowsCollation", "windowsTag", "WindowsToEcmaCalendar", "WindowsToEcmaCalendarMap", "writable", "year", "FALSE", "TRUE",
    "get", "set", "enumerable"];

var tests = [
    {
        name: "Tainting Constructors",
        body: function () {
            try {
                //Sanity check of the above function used for tainting
                assert.throws(function () { verifyGlobalTainting("RegExp", function () { new RegExp(); }) }, Error, "Sanity check to test helper function.");
                globalsList.forEach(functionCalls.tryGlobalTainting, functionCalls);
            }
            catch (e) {
                assert.fail("Exception wasn't expected. Message: " + e);
            }
        }
    },
    {
        name: "Tainting Properties",
        body: function () {
            //Sanity check of the above function used for tainting
            assert.areEqual(verifyPropertyTainting("test", function () { return {}.test; }, { value: "Pass" }), "Pass", "Sanity check to test helper function.");
            keywords.forEach(functionCalls.tryUsingGettersSetters, functionCalls);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
