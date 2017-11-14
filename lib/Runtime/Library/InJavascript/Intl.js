//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

"use strict";
// Core intl lib
(function (EngineInterface, InitType) {
    var platform = EngineInterface.Intl;

    // allow unit tests to disable caching behavior for testing convenience but have this always `true` in real scenarios
    platform.useCaches = true;

    // determine what backing library we are using
    // making these vars in JS allows us to more change how we
    // determine the backing library
    const isPlatformUsingICU = !platform.winglob;
    const isPlatformUsingWinGlob = platform.winglob;

    // constants
    const LANGUAGE_NOT_FOUND = "NOT_FOUND";

    // Built-Ins
    var setPrototype = platform.builtInSetPrototype;
    var getArrayLength = platform.builtInGetArrayLength;
    var callInstanceFunc = platform.builtInCallInstanceFunction;

    // Helper for our extensive usage of null-prototyped objects
    const bare = (obj = {}) => setPrototype(obj, null);

    // REVIEW(jahorto): IntlCache replaces past use of raw objects and JS Maps to cache arbitrary data for a given locale
    // We use a raw object rather than a Map because we don't need any features specific to Maps
    // If the cache gets too big (arbitrarily, > 25 keys is "too big" by default), we delete the entire internal object and start from scratch
    // TODO(jahorto): Experiment with the performance benefit of using an LRU or random-delete cache here.
    class IntlCache {
        constructor(n = 25) {
            this.n = n;
            this._cache = bare();
        }

        get(key) {
            return platform.useCaches ? this._cache[key] : undefined;
        }

        set(key, value) {
            if (!platform.useCaches) {
                return;
            }

            if (_.keys(this._cache).length > this.n && this._cache[key] === undefined) {
                this._cache = bare();
            }

            this._cache[key] = value;
        }
    }

    var Boolean = platform.Boolean;
    var Object = platform.Object;
    var RegExp = platform.RegExp;
    var Number = platform.Number;
    var String = platform.String;
    var Date = platform.Date;
    var Error = platform.Error;
    var Map = platform.Map;

    var RaiseAssert = platform.raiseAssert;

    var ObjectGetPrototypeOf = platform.builtInJavascriptObjectEntryGetPrototypeOf;
    var ObjectIsExtensible = platform.builtInJavascriptObjectEntryIsExtensible;
    var ObjectGetOwnPropertyNames = platform.builtInJavascriptObjectEntryGetOwnPropertyNames;
    var ObjectInstanceHasOwnProperty = platform.builtInJavascriptObjectEntryHasOwnProperty;
    // Because we don't keep track of the attributes object, and neither does the internals of Object.defineProperty;
    // We don't need to restore it's prototype.
    var _objectDefineProperty = platform.builtInJavascriptObjectEntryDefineProperty;
    var ObjectDefineProperty = function (obj, prop, attributes) {
        _objectDefineProperty(obj, prop, setPrototype(attributes, null));
    };

    var ArrayInstanceForEach = platform.builtInJavascriptArrayEntryForEach;
    var ArrayInstanceIndexOf = platform.builtInJavascriptArrayEntryIndexOf;
    var ArrayInstancePush = platform.builtInJavascriptArrayEntryPush;
    var ArrayInstanceJoin = platform.builtInJavascriptArrayEntryJoin;

    var FunctionInstanceBind = platform.builtInJavascriptFunctionEntryBind;
    var DateInstanceGetDate = platform.builtInJavascriptDateEntryGetDate;
    var DateNow = platform.builtInJavascriptDateEntryNow;

    var StringInstanceReplace = platform.builtInJavascriptStringEntryReplace;
    var StringInstanceToLowerCase = platform.builtInJavascriptStringEntryToLowerCase;
    var StringInstanceToUpperCase = platform.builtInJavascriptStringEntryToUpperCase;

    var ObjectPrototype = ObjectGetPrototypeOf({});

    var isFinite = platform.builtInGlobalObjectEntryIsFinite;
    var isNaN = platform.builtInGlobalObjectEntryIsNaN;

    // Keep this "enum" in sync with IntlEngineInterfaceExtensionObject::EntryIntl_RegisterBuiltInFunction
    const IntlBuiltInFunctionID = setPrototype({
        MIN: 0,
        DateToLocaleString: 0,
        DateToLocaleDateString: 1,
        DateToLocaleTimeString: 2,
        NumberToLocaleString: 3,
        StringLocaleCompare: 4,
        MAX: 5
    }, null);

    const _ = {
        toUpperCase(str) { return callInstanceFunc(StringInstanceToUpperCase, str); },
        toLowerCase(str) { return callInstanceFunc(StringInstanceToLowerCase, str); },
        replace(str, pattern, replacement) { return callInstanceFunc(StringInstanceReplace, str, pattern, replacement); },
        split(str, pattern) { return callInstanceFunc(platform.builtInJavascriptStringEntrySplit, str, pattern); },
        substring(str, start, end) { return callInstanceFunc(platform.builtInJavascriptStringEntrySubstring, str, start, end); },

        forEach(array, func) { return callInstanceFunc(ArrayInstanceForEach, array, func); },
        push(array, ...els) { return callInstanceFunc(ArrayInstancePush, array, ...els); },
        join(array, sep) { return callInstanceFunc(ArrayInstanceJoin, array, sep); },
        indexOf(array, el) { return callInstanceFunc(ArrayInstanceIndexOf, array, el); },
        map(array, func) { return callInstanceFunc(platform.builtInJavascriptArrayEntryMap, array, func); },
        reduce(array, func, init) { return callInstanceFunc(platform.builtInJavascriptArrayEntryReduce, array, func, init); },
        slice(array, start, end) { return callInstanceFunc(platform.builtInJavascriptArrayEntrySlice, array, start, end); },
        concat(array, ...els) { return callInstanceFunc(platform.builtInJavascriptArrayEntryConcat, array, ...els); },
        filter(array, func) { return callInstanceFunc(platform.builtInJavascriptArrayEntryFilter, array, func); },

        keys: platform.builtInJavascriptObjectEntryKeys,
        hasOwnProperty(o, prop) { return callInstanceFunc(platform.builtInJavascriptObjectEntryHasOwnProperty, o, prop); },
        defineProperty: platform.builtInJavascriptObjectEntryDefineProperty,

        abs: platform.builtInMathAbs,
        floor: platform.builtInMathFloor,
        max: platform.builtInMathMax,
        pow: platform.builtInMathPow
    };

    let __defaultLocale = undefined;
    const GetDefaultLocale = function () {
        if (__defaultLocale && platform.useCaches) {
            return __defaultLocale;
        }

        const locale = platform.getDefaultLocale();
        if (!locale) {
            // if the system locale is undefined/null/empty string, we have to
            // do something or else we will crash
            __defaultLocale = "en";
        } else {
            __defaultLocale = locale;
        }

        if (isPlatformUsingWinGlob) {
            __defaultLocale = fromNLS(locale);
        }

        return __defaultLocale;
    };

    let CreateDateTimeFormat = function (dateTimeFormat, condition) {
        let retVal = platform.createDateTimeFormat(dateTimeFormat, condition);
        if (retVal === null) {
            // TODO (doilij): remove this fallback when implemented under ICU
            dateTimeFormat.__numberingSystem = "";
            dateTimeFormat.__patternStrings = [
                "{month.a}{day.b}{hour.c}{minute.d}{second.e}",
                "" // another entry for fun
            ]
        }
        // no return value
    };

    var forEachIfPresent = function (obj, length, func) {
        let current = 0;
        while (current < length) {
            if (current in obj) {
                func(obj[current]);
            }
            current++;
        }
    };

    // A helper function that is meant to rethrow SOE and OOM exceptions allowing them to propagate.
    var throwExIfOOMOrSOE = function (ex) {
        if (ex.number === -2146828260 || ex.number === -2146828281) {
            throw ex;
        }
    };

    var tagPublicFunction = function (name, f) {
        return platform.tagPublicLibraryCode(f, name);
    };

    const __resolveLocaleBestFit = new IntlCache();
    const resolveLocaleBestFit = function (locale) {
        var resolvedLocale = __resolveLocaleBestFit.get(locale);
        if (resolvedLocale === undefined) {
            resolvedLocale = platform.resolveLocaleBestFit(locale);
            // REVIEW(jahorto): when ICU returns null here, fall back to the JS version of resolveLocaleLookup
            // rather than the native version to take advantage of the JS implementation of BestAvailableLocale
            if (resolvedLocale === null) {
                resolvedLocale = resolveLocaleLookup(locale);
            }
            // If resolvedLocale is undefined, cache that we got undefined
            // so we don't try to resolve for `locale` in future.
            __resolveLocaleBestFit.set(locale, resolvedLocale === undefined ? LANGUAGE_NOT_FOUND : resolvedLocale);
        } else if (resolvedLocale === LANGUAGE_NOT_FOUND) {
            resolvedLocale = undefined;
        }

        // our best fit matcher for WinGlob will return the default locale if given an unsupported locale
        if (isPlatformUsingWinGlob) {
            const defaultLocale = GetDefaultLocale();
            if (defaultLocale === locale) {
                return resolvedLocale;
            } else if (defaultLocale === resolvedLocale) {
                return undefined;
            } else {
                return resolvedLocale;
            }
        }

        return resolvedLocale;
    }

    // ECMA402 section 9.2.2 (#sec-bestavailablelocale)
    // REVIEW(jahorto): We don't cache the result here because the only callsite is resolveLocaleLookup, which has its own cache
    const BestAvailableLocale = function (locale) {
        let candidate = locale;
        const hyphen = "-";
        while (true) {
            if (platform.isLocaleAvailable(candidate)) {
                return candidate;
            }

            let pos = -1;
            for (let i = candidate.length - 1; i >= 0; i--) {
                if (candidate[i] === hyphen) {
                    pos = i;
                    break;
                }
            }

            if (pos === -1) {
                return undefined;
            } else if (pos >= 2 && candidate[pos - 2] === hyphen) {
                pos -= 2;
            }

            candidate = _.substring(candidate, 0, pos);
        }
    };

    const __resolveLocaleLookup = new IntlCache();
    const resolveLocaleLookup = function (locale) {
        let resolvedLocale = __resolveLocaleLookup.get(locale);
        if (resolvedLocale === undefined) {
            if (isPlatformUsingICU) {
                resolvedLocale = BestAvailableLocale(locale);
            } else {
                resolvedLocale = fromNLS(platform.resolveLocaleLookup(toNLS(locale)));
            }

            // If resolvedLocale is undefined, cache that we got undefined
            // so we don't try to resolve for `locale` in future.
            __resolveLocaleLookup.set(locale, resolvedLocale === undefined ? LANGUAGE_NOT_FOUND : resolvedLocale);
        } else if (resolvedLocale === LANGUAGE_NOT_FOUND) {
            resolvedLocale = undefined;
        }

        return resolvedLocale;
    }

    // ECMA402 section 9.2.5 (#sec-unicodeextensionvalue)
    const UnicodeExtensionSubtags = function (extensionString) {
        if (!extensionString) {
            return [];
        }

        const extensions = _.filter(_.split(extensionString, "-"), (el) => !!el);
        let uStart = -1;
        for (let i = 0; i < extensions.length; i++) {
            const ex = extensions[i];
            if (ex === "u" && uStart === -1 && i + 1 < extensions.length) {
                uStart = i + 1;
            } else if (ex.length === 1 && uStart !== -1) {
                // in -u-k1-value1-k2-value2-x-private, we have hit -x, return [k1, value1, k2, value2]
                return _.slice(extensions, uStart, i);
            }
        }

        if (uStart === -1) {
            return [];
        } else {
            return _.slice(extensions, uStart);
        }
    };

    const resolveLocaleHelper = function (locale, fitter, extensionFilter = []) {
        const localeParts = platform.builtInRegexMatch(locale, LANG_TAG_RE);
        if (!localeParts || !localeParts[1]) {
            return undefined;
        }

        let subTags;
        if (isPlatformUsingWinGlob) {
            subTags = platform.getExtensions(locale);
            subTags = _.map(subTags, (el) => _.split(el, "-"));
            subTags = _.reduce(subTags, (newSubtags, el) => _.concat(newSubtags, el), []);
        } else {
            subTags = UnicodeExtensionSubtags(localeParts[4]);
        }
        let localeWithoutSubtags = localeParts[1];
        if (localeParts[2]) {
            localeWithoutSubtags += `-${localeParts[2]}`;
        }
        if (localeParts[3]) {
            localeWithoutSubtags += `-${localeParts[3]}`;
        }

        const resolvedWithoutSubtags = fitter(localeWithoutSubtags);
        let supportedExtension = "-u";
        let supportedExtensionAddition = "";
        if (extensionFilter !== undefined) {
            var filtered = [];
            for (let i = 0; i < subTags.length; i++) {
                const tag = subTags[i];
                if (_.indexOf(extensionFilter, tag) !== -1) {
                    // tag was an expected unicode extension key, check the value
                    const tagValue = subTags[i + 1];
                    if (i + 1 < subTags.length && tagValue.length > 2) {
                        supportedExtensionAddition += `-${tag}-${tagValue}`;
                        i += 1;
                    }
                }
            }

            supportedExtension += supportedExtensionAddition;
        }

        let resolved = resolvedWithoutSubtags;

        return bare({
            locale: supportedExtension.length > 2 ? resolvedWithoutSubtags + supportedExtension : resolvedWithoutSubtags,
            subTags: subTags,
            localeWithoutSubtags: resolvedWithoutSubtags
        });
    }

    var resolveLocales = function (givenLocales, matcher, extensionFilter) {
        const fitter = matcher === "lookup" ? resolveLocaleLookup : resolveLocaleBestFit;
        const length = getArrayLength(givenLocales) || 0;

        const defaultLocale = GetDefaultLocale();
        for (let i = 0; i < length; i++) {
            const resolved = resolveLocaleHelper(givenLocales[i], fitter, extensionFilter);
            if (resolved && resolved.locale !== undefined) {
                return resolved;
            }
        }
        return resolveLocaleHelper(defaultLocale, fitter, extensionFilter);
    };

    var Internal = bare({
        ToObject(o) {
            if (o === null) {
                platform.raiseNeedObject();
            }
            return o !== undefined ? Object(o) : undefined;
        },

        ToString(s) {
            return s !== undefined ? String(s) : undefined;
        },

        ToNumber(n) {
            return n !== undefined ? Number(n) : NaN;
        },

        ToLogicalBoolean(v) {
            return v !== undefined ? Boolean(v) : undefined;
        },

        ToUint32(n) {
            var num = Number(n),
                ret = 0;
            if (!isNaN(num) && isFinite(num)) {
                ret = _.abs(num % _.pow(2, 32));
            }
            return ret;
        }
    });

    // Internal ops implemented in JS:
    function GetOption(options, property, type, values, fallback) {
        let value = options[property];

        if (value !== undefined) {
            if (type == "boolean") {
                value = Internal.ToLogicalBoolean(value);
            }

            if (type == "string") {
                value = Internal.ToString(value);
            }

            if (type == "number") {
                value = Internal.ToNumber(value);
            }

            if (values !== undefined && _.indexOf(values, value) == -1) {
                platform.raiseOptionValueOutOfRange_3(String(value), String(property), `['${_.join(values, "', '")}']`);
            }

            return value;
        }

        return fallback;
    }

    function GetNumberOption(options, property, minimum, maximum, fallback) {
        const rawValue = options[property];

        if (typeof rawValue !== 'undefined') {
            const formattedValue = Internal.ToNumber(rawValue);

            if (isNaN(formattedValue) || formattedValue < minimum || formattedValue > maximum) {
                platform.raiseOptionValueOutOfRange_3(String(rawValue), String(property), `[${minimum} - ${maximum}]`);
            }

            return _.floor(formattedValue);
        } else {
            return fallback;
        }
    }

    let CURRENCY_CODE_RE;
    function InitializeCurrencyRegExp() {
        CURRENCY_CODE_RE = /^[A-Z]{3}$/i;
    }

    let LANG_TAG_BASE_RE; // language[-script[-region]]
    let LANG_TAG_EXT_RE; // extension part (variant, extension, privateuse)
    let LANG_TAG_RE; // full syntax of language tags (including privateuse and grandfathered)
    (function InitializeLangTagREs() {
        // Language Tag Syntax as described in RFC 5646 #section-2.1
        // Note: All language tags are comprised only of ASCII characters (makes our job easy here)
        // Note: Language tags in canonical form have case conventions, but language tags are case-insensitive for our purposes

        // Note: The ABNF syntax used in RFC 5646 #section-2.1 uses the following numeric quantifier conventions:
        // - (Parentheses) are used for grouping
        // - PRODUCTION => exactly 1 of PRODUCTION                /PRODUCTION/
        // - [PRODUCTION] => 0 or 1 of PRODUCTION                 /(PRODUCTION)?/
        // - #PRODUCTION => exactly # of PRODUCTION               /(PRODUCTION){#}/
        // - a*bPRODUCTION (where a and b are optional)
        //   - *PRODUCTION => any number of PRODUCTION            /(PRODUCTION)*/
        //   - 1*PRODUCTION => 1 or more of PRODUCTION            /(PRODUCTION)+/
        //   - #*PRODUCTION => # or more of PRODUCTION            /(PRODUCTION){#,}/
        //   - *#PRODUCTION => 0 to # (inclusive) of PRODUCTION   /(PRODUCTION){,#}/ or /(PRODUCTION){0,#}/
        //   - a*bPRODUCTION => a to b (inclusive) of PRODUCTION  /(PRODUCTION){a,b}/

        const ALPHA = "[A-Z]";
        const DIGIT = "[0-9]";
        const alphanum = `(?:${ALPHA}|${DIGIT})`;

        const regular = "\\b(?:art-lojban|cel-gaulish|no-bok|no-nyn|zh-guoyu|zh-hakka|zh-min|zh-min-nan|zh-xiang)\\b";
        const irregular = "\\b(?:en-GB-oed|i-ami|i-bnn|i-default|i-enochian|i-hak|i-klingon|i-lux|i-mingo" +
            "|i-navajo|i-pwn|i-tao|i-tay|i-tsu|sgn-BE-FR|sgn-BE-NL|sgn-CH-DE)\\b";
        const grandfathered = `\\b(?:${regular}|${irregular})\\b`;

        const privateuse = `\\b(?:x(?:-${alphanum}{1,8}\\b)+)\\b`;              // privateuse    = "x" 1*("-" (1*8alphanum))
        const singleton = `\\b(?:${DIGIT}|[A-WY-Z])\\b`;                        // singleton    ~= alphanum except for 'x'          ; (paraphrased)
        const extension = `\\b(?:${singleton}(?:-${alphanum}{2,8})+)\\b`;       // extension     = singleton 1*("-" (2*8alphanum))
        const variant = `\\b(?:${alphanum}{5,8}|${DIGIT}${alphanum}{3})\\b`;    // variant       = 5*8alphanum / (DIGIT 3alphanum)
        const region = `\\b(?:${ALPHA}{2}|${DIGIT}{3})\\b`;                     // region        = 2ALPHA / 3DIGIT

        const script = `\\b(?:${ALPHA}{4})\\b`;                                 // script        = 4ALPHA
        const extlang = `\\b(?:${ALPHA}{3}\\b(?:-${ALPHA}{3}){0,2})\\b`;        // extlang       = 3ALPHA *2("-" 3ALPHA)

        const language = '\\b(?:'     +                                         // language      =
            `${ALPHA}{2,3}`           +                                         //                 2*3ALPHA            ; shortest ISO 639 code
                `\\b(?:-${extlang})?` +                                         //                 ["-" extlang]       ; sometimes followed by extended language subtags
            // `|${ALPHA}{4}`         +                                         //               / 4ALPHA              ; or reserved for future use
            // `|${ALPHA}{5,8}`       +                                         //               / 5*8ALPHA            ; or registered language subtag
            `|${ALPHA}{4,8}`          +                                         //              ~/ 4*8ALPHA            ; (paraphrased: combined previous two lines)
            ')\\b';

        // below: ${language}, ${script}, and ${region} are wrapped in parens because matching groups are useful for replacement
        const LANG_TAG_BASE = `\\b(${language})\\b`         +                   // langtag       = language
                              `\\b(?:-(${script}))?\\b`     +                   //                 ["-" script]
                              `\\b(?:-(${region}))?\\b`     ;                   //                 ["-" region]
        const LANG_TAG_EXT  = `\\b(?:-${variant})*\\b`      +                   //                 *("-" variant)
                              `\\b((?:-${extension})*)\\b`  +                   //                 *("-" extension)
                              `\\b(?:-${privateuse})?\\b`   ;                   //                 ["-" privateuse]
        const langtag       = `\\b${LANG_TAG_BASE}\\b${LANG_TAG_EXT}\\b`;

        const LANG_TAG      = `\\b(?:${langtag}|${privateuse}|${grandfathered})\\b`;  // Language-Tag  = ...

        LANG_TAG_BASE_RE    = new RegExp(LANG_TAG_BASE, 'i'); // [1] language; [2] script; [3] region
        LANG_TAG_EXT_RE     = new RegExp(LANG_TAG_EXT,  'i'); //                                       [1] extensions /((${extension})-)*/
        LANG_TAG_RE         = new RegExp(LANG_TAG,      'i'); // [1] language; [2] script; [3] region; [4] extensions
    })();

    function IsWellFormedCurrencyCode(code) {
        code = Internal.ToString(code);

        if (!CURRENCY_CODE_RE) {
            InitializeCurrencyRegExp();
        }

        return platform.builtInRegexMatch(code, CURRENCY_CODE_RE) !== null;
    }

    // Make sure locales is an array, remove duplicate locales, make sure each locale is valid, and canonicalize each.
    function CanonicalizeLocaleList(locales) {
        if (typeof locales === 'undefined') {
            return [];
        }

        if (typeof locales === 'string') {
            locales = [locales];
        }

        locales = Internal.ToObject(locales);
        const length = Internal.ToUint32(locales.length);

        // TODO: Use sets here to prevent duplicates
        let seen = [];

        forEachIfPresent(locales, length, function (locale) {
            if ((typeof locale !== 'string' && typeof locale !== 'object') || locale === null) {
                platform.raiseNeedObjectOrString("Locale");
            }

            let tag = Internal.ToString(locale);

            if (!platform.isWellFormedLanguageTag(tag)) {
                platform.raiseLocaleNotWellFormed(String(tag));
            }

            tag = platform.normalizeLanguageTag(tag);

            if (tag !== undefined && _.indexOf(seen, tag) === -1) {
                _.push(seen, tag);
            }
        });

        return seen;
    }

    function LookupSupportedLocales(requestedLocales, fitter) {
        var subset = [];
        var count = 0;
        const defaultLocale = GetDefaultLocale();
        _.forEach(requestedLocales, function (locale) {
            try {
                var resolved = resolveLocaleHelper(locale, fitter);
                if (resolved.locale) {
                    ObjectDefineProperty(subset, count, { value: resolved.locale, writable: false, configurable: false, enumerable: true });
                    count = count + 1;
                }
            } catch (ex) {
                throwExIfOOMOrSOE(ex);
                // Expecting an error (other than OOM or SOE), same as fitter returning undefined
            }
        });
        ObjectDefineProperty(subset, "length", { value: count, writable: false, configurable: false });
        return subset;
    }

    var supportedLocalesOfWrapper = function (that, functionName, locales, options) {
        if (that === null || that === undefined) {
            platform.raiseNotAConstructor(functionName);
        }

        var hiddenObj = platform.getHiddenObject(that);
        if (!hiddenObj || hiddenObj.isValid !== "Valid") {
            platform.raiseNotAConstructor(functionName);
        }

        return supportedLocalesOf(locales, options);
    }

    var canonicalizeLocaleListWrapper = function (that, functionName, locales) {
        if (that === null || that === undefined) {
            platform.raiseNotAConstructor(functionName);
        }

        var hiddenObj = platform.getHiddenObject(that);
        if (!hiddenObj || hiddenObj.isValid !== "Valid") {
            platform.raiseNotAConstructor(functionName);
        }

        return CanonicalizeLocaleList(locales);
    }

    // Shared among all the constructors
    var supportedLocalesOf = function (locales, options) {
        var matcher;
        locales = CanonicalizeLocaleList(locales);

        if (typeof options !== 'undefined') {
            matcher = options.localeMatcher;

            if (typeof matcher !== 'undefined') {
                matcher = Internal.ToString(matcher);

                if (matcher !== 'lookup' && matcher !== 'best fit') {
                    platform.raiseOptionValueOutOfRange_3(String(matcher), "localeMatcher", "['best fit', 'lookup']");
                }
            }
        }

        if (typeof matcher === 'undefined' || matcher === 'best fit') {
            return LookupSupportedLocales(locales, resolveLocaleBestFit);
        } else {
            return LookupSupportedLocales(locales, resolveLocaleLookup);
        }
    };

    const intlStaticMethodThisArg = bare();
    platform.setHiddenObject(intlStaticMethodThisArg, bare({ isValid: "Valid" }));

    // We wrap these functions so that we can define the correct name for this function for each Intl constructor,
    // which allows us to display the correct error message for each Intl type.
    const collator_supportedLocalesOf_name = "Intl.Collator.supportedLocalesOf";
    const collator_supportedLocalesOf = callInstanceFunc(FunctionInstanceBind, tagPublicFunction(collator_supportedLocalesOf_name,
        function collator_supportedLocalesOf_dummyName(locales, options = undefined) {
            return supportedLocalesOfWrapper(this, collator_supportedLocalesOf_name, locales, options);
        }), intlStaticMethodThisArg);

    const numberFormat_supportedLocalesOf_name = "Intl.NumberFormat.supportedLocalesOf";
    const numberFormat_supportedLocalesOf = callInstanceFunc(FunctionInstanceBind, tagPublicFunction(numberFormat_supportedLocalesOf_name,
        function numberFormat_supportedLocalesOf_dummyName(locales, options = undefined) {
            return supportedLocalesOfWrapper(this, numberFormat_supportedLocalesOf_name, locales, options);
        }), intlStaticMethodThisArg);

    const dateTimeFormat_supportedLocalesOf_name = "Intl.DateTimeFormat.supportedLocalesOf";
    const dateTimeFormat_supportedLocalesOf = callInstanceFunc(FunctionInstanceBind, tagPublicFunction(dateTimeFormat_supportedLocalesOf_name,
        function dateTimeFormat_supportedLocalesOf_dummyName(locales, options = undefined) {
            return supportedLocalesOfWrapper(this, dateTimeFormat_supportedLocalesOf_name, locales, options);
        }), intlStaticMethodThisArg);

    const getCanonicalLocales_name = "Intl.getCanonicalLocales";
    const getCanonicalLocales = callInstanceFunc(FunctionInstanceBind, tagPublicFunction(getCanonicalLocales_name,
        function getCanonicalLocales_dummyName(locales) {
            return canonicalizeLocaleListWrapper(this, getCanonicalLocales_name, locales);
        }), intlStaticMethodThisArg);

    // TODO: Bound functions get the "bound" prefix by default, so we need to remove it.
    // When https://github.com/Microsoft/ChakraCore/issues/637 is fixed and we have a way
    // to make built-in functions non-constructible, we can remove the call to
    // Function.prototype.bind (i.e. FunctionInstanceBind) and just rely on tagging instead of setting the "name" manually.
    ObjectDefineProperty(collator_supportedLocalesOf, 'name', { value: 'supportedLocalesOf' });
    ObjectDefineProperty(numberFormat_supportedLocalesOf, 'name', { value: 'supportedLocalesOf' });
    ObjectDefineProperty(dateTimeFormat_supportedLocalesOf, 'name', { value: 'supportedLocalesOf' });
    ObjectDefineProperty(getCanonicalLocales, 'name', { value: 'getCanonicalLocales' });

    // Windows National Language Support (NLS) uses RFC4646 language tags, and NLS currently provides
    // the backing implementation for platform.getDefaultLocale() and platform.compareString() on Windows.
    // Windows Globalization, ICU, and the user-exposed Intl interface use RFC5646 language tags
    // The following code should be used immediately before or after calling an NLS-backed platform method
    const [toNLS, fromNLS] = (function () {
        // ICU does not need to worry about NLS locale strings
        if (isPlatformUsingICU) {
            const identity = (locale) => locale;
            return [identity, identity];
        }

        // NOTE: If either of the following two tables change, the other must be updated as well

        // If an empty string is encountered for the value of the property; that means that is by default.
        // So in the case of zh-TW; "default" and "stroke" are the same.
        // This list was discussed with AnBorod, AnGlass and SureshJa.
        // Source of this list: https://msdn.microsoft.com/en-us/library/cc233968.aspx (the table at the bottom)
        // for each sub-object, the key is a RFC5646 collation value and the value is an RFC4646 collation value
        const localesAcceptingCollationValues = bare({
            "es-ES": bare({ trad: "tradnl" }),
            "lv-LV": bare({ trad: "tradnl" }),
            "de-DE": bare({ phonebk: "phoneb" }),
            "ja-JP": bare({ unihan: "radstr" }),
            "zh-TW": bare({ unihan: "radstr", stroke: "", phonetic: "pronun" }),
            "zh-HK": bare({ unihan: "radstr", stroke: "" }),
            "zh-MO": bare({ unihan: "radstr", stroke: "" }),
            "zh-CN": bare({ stroke: "stroke", pinyin: "" }),
            "zh-SG": bare({ stroke: "stroke", pinyin: "" })

            // The following locales are supported by Windows; however, no BCP47 equivalent collation values were found for these.
            // In future releases; this list (plus most of the Collator implementation) will be changed/removed as the platform support is expected to change.
            // "hu-HU": ["technl"],
            // "ka-GE": ["modern"],
            // "x-IV": ["mathan"]
        });

        // reverses the keys and values in each locale's sub-object in localesAcceptingCollationValues
        // localesAcceptingCollationValues[locale][key] = value -> reverseLocalesAcceptingCollationValues[locale][value] = key
        // for each sub-object, the key is a RFC4646 collation value and the value is an RFC5646 collation value
        const reverseLocalesAcceptingCollationValues = bare({
            "es-ES": bare({ tradnl: "trad" }),
            "lv-LV": bare({ tradnl: "trad" }),
            "de-DE": bare({ phoneb: "phonebk" }),
            "ja-JP": bare({ radstr: "unihan" }),
            "zh-TW": bare({ pronun: "phonetic", radstr: "unihan" }),
            "zh-HK": bare({ radstr: "unihan" }),
            "zh-MO": bare({ radstr: "unihan" }),
            "zh-CN": bare({ stroke: "stroke" }),
            "zh-SG": bare({ stroke: "stroke" })
        });

        let __fromNLS = new IntlCache();
        const fromNLS = function (locale) {
            if (!locale) {
                return undefined;
            }

            if (__fromNLS.get(locale)) {
                return __fromNLS.get(locale);
            }

            const parts = platform.builtInRegexMatch(locale, /([^_]*)_?(.+)?/);
            const strippedLocale = parts[1];
            const collation = parts[2];
            let ret;

            if (collation === undefined) {
                return locale;
            }

            const collationMapForLocale = reverseLocalesAcceptingCollationValues[strippedLocale];
            if (collationMapForLocale === undefined) {
                // TODO(jahorto): NLS gave us a locale_collation back but we don't have a map for that locale -- why?
                ret = `${strippedLocale}-u-co-${collation}`;
                __fromNLS.set(locale, ret);
                return ret;
            }

            const mappedCollation = collationMapForLocale[collation];
            if (mappedCollation !== undefined) {
                ret = `${strippedLocale}-u-co-${mappedCollation}`;
            } else {
                // TODO(jahorto): NLS gave us a locale_collation back but we dont think that collation is supported for that locale -- why?
                ret = `${strippedLocale}-u-co-${collation}`;
            }

            __fromNLS.set(locale, ret);

            return ret;
        };

        let __toNLS = new IntlCache();
        const toNLS = function (locale) {
            if (!locale) {
                return undefined;
            }

            if (__toNLS.get(locale)) {
                return __toNLS.get(locale);
            }

            const extensionParts = platform.builtInRegexMatch(locale, LANG_TAG_RE);
            const collationParts = platform.builtInRegexMatch(extensionParts[4] || "", /(?:-co-([^-]+))/);
            const strippedLocale = platform.builtInRegexMatch(locale, LANG_TAG_BASE_RE)[0];
            if (!collationParts) {
                // the given locale had extensions but no -co- extension
                // NLS doesn't know what to do with anything more than strippedLocale + collation
                __toNLS.set(locale, strippedLocale);
                return strippedLocale;
            }

            const collation = collationParts[1];
            let ret;

            const collationMapForLocale = localesAcceptingCollationValues[strippedLocale];
            if (collationMapForLocale !== undefined) {
                const mappedCollation = collationMapForLocale[collation];
                if (mappedCollation) {
                    ret = `${strippedLocale}_${mappedCollation}`;
                } else {
                    // if theres no mappedCollation, then NLS doesn't support this collation value (or its the default)
                    ret = strippedLocale;
                }
            } else {
                // if theres no collationMapForLocale, then NLS doesn't support any collation values for this locale
                ret = strippedLocale;
            }

            __toNLS.set(locale, ret);

            return ret;
        }

        return [toNLS, fromNLS];
    })();

    // Intl.Collator, String.prototype.localeCompare
    var Collator = (function () {
        // Keep these "enums" in sync with lib/Runtime/PlatformAgnostic/Intl.h
        const CollatorSensitivity = bare({
            base: 0,
            accent: 1,
            case: 2,
            variant: 3,
            DEFAULT: 3
        });
        const CollatorCaseFirst = bare({
            upper: 0,
            lower: 1,
            false: 2,
            DEFAULT: 2
        });

        function toEnum(enumObject, key) {
            if (!key || typeof key !== "string") {
                return enumObject.DEFAULT;
            } else {
                return enumObject[key];
            }
        }

        if (InitType === 'Intl' || InitType === 'String') {

            function InitializeCollator(collator, localeList, options) {
                if (typeof collator !== "object") {
                    platform.raiseNeedObject();
                }

                if (callInstanceFunc(ObjectInstanceHasOwnProperty, collator, '__initializedIntlObject') && collator.__initializedIntlObject) {
                    platform.raiseObjectIsAlreadyInitialized("Collator", "Collator");
                }

                collator.__initializedIntlObject = true;

                // Extract options
                if (typeof options === 'undefined') {
                    options = bare();
                } else {
                    options = Internal.ToObject(options);
                }

                const matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
                const usage = GetOption(options, "usage", "string", ["sort", "search"], "sort");
                const sensitivity = GetOption(options, "sensitivity", "string", ["base", "accent", "case", "variant"], "variant");
                const ignorePunctuation = GetOption(options, "ignorePunctuation", "boolean", [true, false], false);
                let caseFirst = GetOption(options, "caseFirst", "string", ["upper", "lower", "false"], undefined);
                let numeric = GetOption(options, "numeric", "boolean", [true, false], undefined);

                const resolvedLocaleInfo = resolveLocales(CanonicalizeLocaleList(localeList), matcher, ["co", "kn", "kf"]);

                let collation = "default";
                if (resolvedLocaleInfo.subTags) {
                    for (let i = 0; i < getArrayLength(resolvedLocaleInfo.subTags); i += 2) {
                        const key = resolvedLocaleInfo.subTags[i];
                        const value = resolvedLocaleInfo.subTags[i + 1] || undefined; // normalize falsy values to undefined
                        if (key === "kf" && caseFirst === undefined) {
                            caseFirst = GetOption(bare({ caseFirst: value }), "caseFirst", "string", ["upper", "lower", "false"], undefined);
                        } else if (key === "kn" && numeric === undefined) {
                            if (value !== undefined) {
                                numeric = Internal.ToLogicalBoolean(_.toLowerCase(value) === "true");
                            } else {
                                numeric = true;
                            }
                        } else if (key === "co" && value !== undefined && value !== "default" && value !== "search" && value !== "sort" && value !== "standard") {
                            // Ignore these collation values as they shouldn't have any impact
                            collation = value;
                        }
                    }
                }

                // re-append the collation to the base locale string, if we have found one.
                // this is going to be the final locale that we report back to the user in resolvedOptions.
                // even if the user provided -kn or -kf tags, NLS doesnt support them, and its nice to have a common denominator of everyone understanding language + script + region + collation.
                // firefox (~57) keeps -kn, -kf, and -co in the returned locale (if provided), chrome (~61) will only keep -co
                let finalLocale; // at most a langauge, script, region, and collation value
                let platformLocale; // finalLocale as RFC5646 for ICU, RFC4646 for NLS
                const candidate = `${resolvedLocaleInfo.localeWithoutSubtags}-u-co-${collation}`;
                if (isPlatformUsingWinGlob) {
                    platformLocale = toNLS(candidate);
                    const parts = platform.builtInRegexMatch(platformLocale, /([^_]*)_?(.+)?/);
                    if (!parts || !parts[2]) {
                        // NLS does not have the requested collation option
                        finalLocale = resolvedLocaleInfo.localeWithoutSubtags;
                        platformLocale = resolvedLocaleInfo.localeWithoutSubtags;
                        collation = "default";
                    } else {
                        finalLocale = candidate;
                        // collation is already set correctly
                    }
                } else {
                    const maybeCollation = platform.collatorGetCollation(candidate);
                    if (!maybeCollation) {
                        collation = "default";
                        platformLocale = resolvedLocaleInfo.localeWithoutSubtags;
                    } else {
                        collation = maybeCollation;
                        platformLocale = `${resolvedLocaleInfo.localeWithoutSubtags}-u-co-${maybeCollation}`;
                    }

                    finalLocale = platformLocale;
                }

                // WinGlob doesn't respect the caseFirst setting regardless, so we shouldn't bother checking the default
                if (caseFirst === undefined && isPlatformUsingICU) {
                    let defaultCaseFirstCheck;
                    try {
                        defaultCaseFirstCheck = platform.compareString('A', 'a', finalLocale, undefined, undefined, undefined, undefined);
                    } catch (e) {
                        // Rethrow OOM or SOE
                        throwExIfOOMOrSOE(e);

                        // Otherwise, Generic message to cover the exception throw from the CompareStringEx api.
                        // The platform's exception is also generic and in most if not all cases specifies that "a" argument is invalid.
                        // We have no other information from the platform on the cause of the exception.
                        platform.raiseOptionValueOutOfRange();
                    }

                    if (defaultCaseFirstCheck === 0) {
                        caseFirst = 'false';
                    } else if (defaultCaseFirstCheck === -1) {
                        caseFirst = 'upper';
                    } else {
                        caseFirst = 'lower';
                    }
                }

                if (numeric === undefined) {
                    numeric = false;
                }

                // Set the options on the object
                // REVIEW(jahorto): all of the enum-able keys get special fields for their enum
                // so that we dont need to compute the key value from the enum in resolvedOptions()
                collator.__matcher = matcher;
                collator.__locale = finalLocale;
                collator.__platformLocale = platformLocale;
                collator.__usage = usage;
                collator.__sensitivity = sensitivity;
                collator.__sensitivityEnum = toEnum(CollatorSensitivity, sensitivity);
                collator.__ignorePunctuation = ignorePunctuation;
                collator.__caseFirst = caseFirst;
                collator.__caseFirstEnum = toEnum(CollatorCaseFirst, caseFirst);
                collator.__numeric = numeric;
                collator.__collation = collation;
                collator.__initializedCollator = true;
            }

            platform.registerBuiltInFunction(tagPublicFunction("String.prototype.localeCompare", function () {
                var that = arguments[0];
                if (this === undefined || this === null) {
                    platform.raiseThis_NullOrUndefined("String.prototype.localeCompare");
                }
                else if (that === null) {
                    platform.raiseNeedObject();
                }
                // ToString must be called on this/that argument before we do any other operation, as other operations in InitializeCollator may also be observable
                var thisArg = String(this);
                var that = String(that);
                var stateObject = setPrototype({}, null);
                InitializeCollator(stateObject, arguments[1], arguments[2]);
                return Number(platform.compareString(thisArg, that, stateObject.__platformLocale, stateObject.__sensitivityEnum, stateObject.__ignorePunctuation, stateObject.__numeric, stateObject.__caseFirstEnum));
            }), IntlBuiltInFunctionID.StringLocaleCompare);

            if (InitType === 'Intl') {

                function Collator(locales = undefined, options = undefined) {
                    if (this === Intl || this === undefined) {
                        return new Collator(locales, options);
                    }

                    let obj = Internal.ToObject(this);
                    if (!ObjectIsExtensible(obj)) {
                        platform.raiseObjectIsNonExtensible("Collator");
                    }

                    // Use the hidden object to store data
                    let hiddenObject = platform.getHiddenObject(obj);

                    if (hiddenObject === undefined) {
                        hiddenObject = setPrototype({}, null);
                        platform.setHiddenObject(obj, hiddenObject);
                    }

                    InitializeCollator(hiddenObject, locales, options);

                    // Add the bound compare
                    hiddenObject.__boundCompare = callInstanceFunc(FunctionInstanceBind, compare, obj);
                    delete hiddenObject.__boundCompare.name;
                    return obj;
                }
                tagPublicFunction("Intl.Collator", Collator);

                function compare(a, b) {
                    if (typeof this !== 'object') {
                        platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
                    }

                    var hiddenObject = platform.getHiddenObject(this);
                    if (hiddenObject === undefined || !hiddenObject.__initializedCollator) {
                        platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
                    }

                    a = String(a);
                    b = String(b);

                    return Number(platform.compareString(a, b, hiddenObject.__platformLocale, hiddenObject.__sensitivityEnum, hiddenObject.__ignorePunctuation, hiddenObject.__numeric, hiddenObject.__caseFirstEnum));
                }
                tagPublicFunction("Intl.Collator.prototype.compare", compare);

                ObjectDefineProperty(Collator, 'supportedLocalesOf', { value: collator_supportedLocalesOf, writable: true, configurable: true });

                ObjectDefineProperty(Collator, 'prototype', { value: new Collator(), writable: false, enumerable: false, configurable: false });
                setPrototype(Collator.prototype, Object.prototype);

                ObjectDefineProperty(Collator.prototype, 'constructor', { value: Collator, writable: true, enumerable: false, configurable: true });

                ObjectDefineProperty(Collator.prototype, 'resolvedOptions', {
                    value: function resolvedOptions() {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("Collator.prototype.resolvedOptions", "Collator");
                        }
                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedCollator) {
                            platform.raiseNeedObjectOfType("Collator.prototype.resolvedOptions", "Collator");
                        }

                        const options = {
                            locale: hiddenObject.__locale,
                            usage: hiddenObject.__usage,
                            sensitivity: hiddenObject.__sensitivity,
                            ignorePunctuation: hiddenObject.__ignorePunctuation,
                            collation: hiddenObject.__collation,
                            numeric: hiddenObject.__numeric
                        };

                        // the NLS implementation of platform.compareString does not support caseFirst, so we shouldn't report it
                        if (isPlatformUsingICU) {
                            options.caseFirst = hiddenObject.__caseFirst;
                        }

                        return options;
                    },
                    writable: true,
                    enumerable: false,
                    configurable: true
                });

                ObjectDefineProperty(Collator.prototype, 'compare', {
                    get: tagPublicFunction('get compare', function () {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
                        }

                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedCollator) {
                            platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
                        }

                        return hiddenObject.__boundCompare;
                    }),
                    enumerable: false,
                    configurable: true
                });

                return Collator;
            }
        }
        // 'Init.Collator' not defined if reached here. Return 'undefined'
        return undefined;
    })();

    // Intl.NumberFormat, Number.prototype.toLocaleString
    var NumberFormat = (function () {
        // Keep these "enums" in sync with lib/Runtime/PlatformAgnostic/Intl.h
        const NumberFormatStyle = setPrototype({
            DEFAULT: 0, // "decimal" is the default
            DECIMAL: 0, // Intl.NumberFormat(locale, { style: "decimal" }); // aka in our code as "number"
            PERCENT: 1, // Intl.NumberFormat(locale, { style: "percent" });
            CURRENCY: 2, // Intl.NumberFormat(locale, { style: "currency", ... });
        }, null);
        const NumberFormatCurrencyDisplay = setPrototype({
            DEFAULT: 0, // "symbol" is the default
            SYMBOL: 0, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "symbol" }); // e.g. "$" or "US$" depeding on locale
            CODE: 1, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "code" }); // e.g. "USD"
            NAME: 2, // Intl.NumberFormat(locale, { style: "currency", currencyDisplay: "name" }); // e.g. "US dollar"
        }, null);

        if (InitType === 'Intl' || InitType === 'Number') {
            function InitializeNumberFormat(numberFormat, localeList, options) {
                if (typeof numberFormat != "object") {
                    platform.raiseNeedObject();
                }

                if (callInstanceFunc(ObjectInstanceHasOwnProperty, numberFormat, '__initializedIntlObject') && numberFormat.__initializedIntlObject) {
                    platform.raiseObjectIsAlreadyInitialized("NumberFormat", "NumberFormat");
                }

                numberFormat.__initializedIntlObject = true;

                // Extract options
                if (typeof options === 'undefined') {
                    options = setPrototype({}, null);
                } else {
                    options = Internal.ToObject(options);
                }

                var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
                var style = GetOption(options, "style", "string", ["decimal", "percent", "currency"], "decimal");

                var formatterToUse = NumberFormatStyle.DECIMAL; // DEFAULT
                if (style === 'percent') {
                    formatterToUse = NumberFormatStyle.PERCENT;
                } else if (style === 'currency') {
                    formatterToUse = NumberFormatStyle.CURRENCY;
                }

                var currency = GetOption(options, "currency", "string", undefined, undefined);
                var currencyDisplay = GetOption(options, 'currencyDisplay', 'string', ['code', 'symbol', 'name'], 'symbol');
                var currencyDigits = undefined;

                var minimumIntegerDigits = GetNumberOption(options, 'minimumIntegerDigits', 1, 21, 1);
                var minimumFractionDigits = undefined;
                var maximumFractionDigits = undefined;
                var maximumFractionDigitsDefault = undefined;

                var minimumSignificantDigits = options.minimumSignificantDigits;
                var maximumSignificantDigits = options.maximumSignificantDigits;

                if (typeof minimumSignificantDigits !== 'undefined' || typeof maximumSignificantDigits !== 'undefined') {
                    minimumSignificantDigits = GetNumberOption(options, 'minimumSignificantDigits', 1, 21, 1);
                    maximumSignificantDigits = GetNumberOption(options, 'maximumSignificantDigits', minimumSignificantDigits, 21, 21);
                }

                var useGrouping = GetOption(options, 'useGrouping', 'boolean', undefined, true);

                // Deal with the locales and extensions
                localeList = CanonicalizeLocaleList(localeList);
                var resolvedLocaleInfo = resolveLocales(localeList, matcher, ["nu"]);

                // Correct the options if necessary
                if (typeof currency !== 'undefined' && !IsWellFormedCurrencyCode(currency)) {
                    platform.raiseInvalidCurrencyCode(String(currency));
                }

                if (style === "currency") {
                    if (typeof currency === 'undefined') {
                        platform.raiseMissingCurrencyCode();
                    }
                    currency = callInstanceFunc(StringInstanceToUpperCase, currency);
                    try {
                        currencyDigits = platform.currencyDigits(currency);
                    } catch (e) {
                        throwExIfOOMOrSOE(e);
                        platform.raiseInvalidCurrencyCode(String(currency));
                    }
                    minimumFractionDigits = GetNumberOption(options, 'minimumFractionDigits', 0, 20, currencyDigits);
                    maximumFractionDigitsDefault = _.max(currencyDigits, minimumFractionDigits);
                } else {
                    currency = undefined;
                    currencyDisplay = undefined;
                    minimumFractionDigits = GetNumberOption(options, 'minimumFractionDigits', 0, 20, 0);
                    if (style === "percent") {
                        maximumFractionDigitsDefault = _.max(minimumFractionDigits, 0);
                    } else {
                        maximumFractionDigitsDefault = _.max(minimumFractionDigits, 3)
                    }
                }

                maximumFractionDigits = GetNumberOption(options, 'maximumFractionDigits', minimumFractionDigits, 20, maximumFractionDigitsDefault);

                // Set the options on the object
                numberFormat.__localeMatcher = matcher;
                numberFormat.__locale = resolvedLocaleInfo.locale;
                numberFormat.__style = style;

                if (currency !== undefined) {
                    numberFormat.__currency = currency;
                }

                if (currencyDisplay !== undefined) {
                    numberFormat.__currencyDisplay = currencyDisplay;
                    numberFormat.__currencyDisplayToUse = NumberFormatCurrencyDisplay.DEFAULT;
                    if (currencyDisplay === "symbol") {
                        numberFormat.__currencyDisplayToUse = NumberFormatCurrencyDisplay.SYMBOL;
                    } else if (currencyDisplay === "code") {
                        numberFormat.__currencyDisplayToUse = NumberFormatCurrencyDisplay.CODE;
                    } else if (currencyDisplay === "name") {
                        numberFormat.__currencyDisplayToUse = NumberFormatCurrencyDisplay.NAME;
                    }
                }

                numberFormat.__minimumIntegerDigits = minimumIntegerDigits;
                numberFormat.__minimumFractionDigits = minimumFractionDigits;
                numberFormat.__maximumFractionDigits = maximumFractionDigits;

                if (maximumSignificantDigits !== undefined) {
                    numberFormat.__minimumSignificantDigits = minimumSignificantDigits;
                    numberFormat.__maximumSignificantDigits = maximumSignificantDigits;
                }

                numberFormat.__formatterToUse = formatterToUse;
                numberFormat.__useGrouping = useGrouping;

                try {
                    // Cache api instance and update numbering system on the object
                    platform.cacheNumberFormat(numberFormat);
                } catch (e) {
                    throwExIfOOMOrSOE(e);
                    // Generic message to cover the exception throw from the platform.
                    // The platform's exception is also generic and in most if not all cases specifies that "a" argument is invalid.
                    // We have no other information from the platform on the cause of the exception.
                    platform.raiseOptionValueOutOfRange();
                }

                if (!numberFormat.__numberingSystem) {
                    numberFormat.__numberingSystem = "latn"; // assume Latin numerals by default
                }

                numberFormat.__numberingSystem = callInstanceFunc(StringInstanceToLowerCase, numberFormat.__numberingSystem);
                numberFormat.__initializedNumberFormat = true;
            }

            platform.registerBuiltInFunction(tagPublicFunction("Number.prototype.toLocaleString", function () {
                if ((typeof this) !== 'number' && !(this instanceof Number)) {
                    platform.raiseNeedObjectOfType("Number.prototype.toLocaleString", "Number");
                }

                var stateObject = setPrototype({}, null);
                InitializeNumberFormat(stateObject, arguments[0], arguments[1]);

                var n = Internal.ToNumber(this);
                // Need to special case the '-0' case to format as 0 instead of -0.
                return String(platform.formatNumber(n === -0 ? 0 : n, stateObject));
            }), IntlBuiltInFunctionID.NumberToLocaleString);

            if (InitType === 'Intl') {
                function NumberFormat(locales = undefined, options = undefined) {
                    if (this === Intl || this === undefined) {
                        return new NumberFormat(locales, options);
                    }

                    let obj = Internal.ToObject(this);

                    if (!ObjectIsExtensible(obj)) {
                        platform.raiseObjectIsNonExtensible("NumberFormat");
                    }

                    // Use the hidden object to store data
                    let hiddenObject = platform.getHiddenObject(obj);

                    if (hiddenObject === undefined) {
                        hiddenObject = setPrototype({}, null);
                        platform.setHiddenObject(obj, hiddenObject);
                    }

                    InitializeNumberFormat(hiddenObject, locales, options);

                    hiddenObject.__boundFormat = callInstanceFunc(FunctionInstanceBind, format, obj)
                    delete hiddenObject.__boundFormat.name;

                    return obj;
                }
                tagPublicFunction("Intl.NumberFormat", NumberFormat);

                function format(n) {
                    n = Internal.ToNumber(n);

                    if (typeof this !== 'object') {
                        platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
                    }

                    var hiddenObject = platform.getHiddenObject(this);
                    if (hiddenObject === undefined || !hiddenObject.__initializedNumberFormat) {
                        platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
                    }

                    // Need to special case the '-0' case to format as 0 instead of -0.
                    return String(platform.formatNumber(n === -0 ? 0 : n, hiddenObject));
                }
                tagPublicFunction("Intl.NumberFormat.prototype.format", format);

                ObjectDefineProperty(NumberFormat, 'supportedLocalesOf', { value: numberFormat_supportedLocalesOf, writable: true, configurable: true });

                var options = ['locale', 'numberingSystem', 'style', 'currency', 'currencyDisplay', 'minimumIntegerDigits',
                    'minimumFractionDigits', 'maximumFractionDigits', 'minimumSignificantDigits', 'maximumSignificantDigits',
                    'useGrouping'];

                ObjectDefineProperty(NumberFormat, 'prototype', { value: new NumberFormat(), writable: false, enumerable: false, configurable: false });
                setPrototype(NumberFormat.prototype, Object.prototype);
                ObjectDefineProperty(NumberFormat.prototype, 'constructor', { value: NumberFormat, writable: true, enumerable: false, configurable: true });

                ObjectDefineProperty(NumberFormat.prototype, 'resolvedOptions', {
                    value: function resolvedOptions() {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("NumberFormat.prototype.resolvedOptions", "NumberFormat");
                        }
                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedNumberFormat) {
                            platform.raiseNeedObjectOfType("NumberFormat.prototype.resolvedOptions", "NumberFormat");
                        }

                        var resolvedOptions = setPrototype({}, null);

                        callInstanceFunc(ArrayInstanceForEach, options, function (option) {
                            if (typeof hiddenObject['__' + option] !== 'undefined') {
                                resolvedOptions[option] = hiddenObject['__' + option];
                            }
                        });

                        return setPrototype(resolvedOptions, {});
                    }, writable: true, enumerable: false, configurable: true
                });

                ObjectDefineProperty(NumberFormat.prototype, 'format', {
                    get: tagPublicFunction('get format', function () {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
                        }

                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedNumberFormat) {
                            platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
                        }

                        return hiddenObject.__boundFormat;
                    }), enumerable: false, configurable: true
                });

                return NumberFormat;
            }
        }
        // 'Init.NumberFormat' not defined if reached here. Return 'undefined'
        return undefined;
    })();

    // Intl.DateTimeFormat, Date.prototype.toLocaleString, Date.prototype.toLocaleDateString, Date.prototype.toLocaleTimeString
    var DateTimeFormat = (function () {

        if (InitType === 'Intl' || InitType === 'Date') {
            function ToDateTimeOptions(options, required, defaults) {
                if (options === undefined) {
                    options = setPrototype({}, null);
                } else {
                    options = Internal.ToObject(options);
                }

                var needDefaults = true;
                if (required === "date" || required === "any") {
                    if (options.weekday !== undefined || options.year !== undefined || options.month !== undefined || options.day !== undefined) {
                        needDefaults = false;
                    }
                }
                if (required === "time" || required === "any") {
                    if (options.hour !== undefined || options.minute !== undefined || options.second !== undefined) {
                        needDefaults = false;
                    }
                }

                if (needDefaults && (defaults === "date" || defaults === "all")) {
                    ObjectDefineProperty(options, "year", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                    ObjectDefineProperty(options, "month", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                    ObjectDefineProperty(options, "day", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                }
                if (needDefaults && (defaults === "time" || defaults === "all")) {
                    ObjectDefineProperty(options, "hour", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                    ObjectDefineProperty(options, "minute", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                    ObjectDefineProperty(options, "second", {
                        value: "numeric", writable: true,
                        enumerable: true, configurable: true
                    });
                }
                return options;
            }

            // Currently you cannot format date pieces and time pieces together, so this builds up a format template for each separately.
            function EcmaOptionsToWindowsTemplate(options) {
                var template = [];

                if (options.weekday) {
                    if (options.weekday === 'narrow' || options.weekday === 'short') {
                        callInstanceFunc(ArrayInstancePush, template, 'dayofweek.abbreviated');
                    } else if (options.weekday === 'long') {
                        callInstanceFunc(ArrayInstancePush, template, 'dayofweek.full');
                    }
                }

                // TODO: Era not supported
                if (options.year) {
                    if (options.year === '2-digit') {
                        callInstanceFunc(ArrayInstancePush, template, 'year.abbreviated');
                    } else if (options.year === 'numeric') {
                        callInstanceFunc(ArrayInstancePush, template, 'year.full');
                    }
                }

                if (options.month) {
                    if (options.month === '2-digit' || options.month === 'numeric') {
                        callInstanceFunc(ArrayInstancePush, template, 'month.numeric')
                    } else if (options.month === 'short' || options.month === 'narrow') {
                        callInstanceFunc(ArrayInstancePush, template, 'month.abbreviated');
                    } else if (options.month === 'long') {
                        callInstanceFunc(ArrayInstancePush, template, 'month.full');
                    }
                }

                if (options.day) {
                    callInstanceFunc(ArrayInstancePush, template, 'day');
                }

                if (options.timeZoneName) {
                    if (options.timeZoneName === "short") {
                        callInstanceFunc(ArrayInstancePush, template, 'timezone.abbreviated');
                    } else if (options.timeZoneName === "long") {
                        callInstanceFunc(ArrayInstancePush, template, 'timezone.full');
                    }
                }

                callInstanceFunc(ArrayInstanceForEach, ['hour', 'minute', 'second'], function (opt) {
                    if (options[opt]) {
                        callInstanceFunc(ArrayInstancePush, template, opt);
                    }
                });

                // TODO: Timezone Name not supported.
                return getArrayLength(template) > 0 ? callInstanceFunc(ArrayInstanceJoin, template, ' ') : undefined;
            }

            var WindowsToEcmaCalendarMap = {
                'GregorianCalendar': 'gregory',
                'HebrewCalendar': 'hebrew',
                'HijriCalendar': 'islamic',
                'JapaneseCalendar': 'japanese',
                'JulianCalendar': 'julian',
                'KoreanCalendar': 'korean',
                'UmAlQuraCalendar': 'islamic-civil',
                'ThaiCalendar': 'thai',
                'TaiwanCalendar': 'taiwan'
            };

            function WindowsToEcmaCalendar(calendar) {
                if (typeof calendar === 'undefined') {
                    return '';
                }

                return WindowsToEcmaCalendarMap[calendar] || 'gregory';
            }

            // Certain formats have similar patterns on both ecma and windows; will use helper methods for them
            function correctWeekdayEraMonthPattern(patternString, userValue, searchParam) {
                // parts[1] is either dayofweek.solo, dayofweek, era or month; parts[2] is either abbreviated or full
                var parts = platform.builtInRegexMatch(patternString, RegExp("{(" + searchParam + "(?:\\.solo)?)\\.([a-z]*)(?:\\([0-9]\\))?}"));
                // If this happens that means windows removed the specific pattern (which isn't expected; but better be safe)
                if (parts === null) {
                    RaiseAssert(new Error("Error when correcting windows returned weekday/Era/Month pattern; regex returned null. \nInput was: '" + patternString + "'\nRegex: '" + "{(" + searchParam + "(\\.solo)?)\\.([a-z]*)(\\([0-9]\\))?}'"));
                    return patternString;
                }

                if (parts[2] !== "full" && userValue === "long") {
                    return callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{" + parts[1] + "." + "full" + "}");
                } else if (userValue !== "long") {
                    return callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{" + parts[1] + "." + (userValue === "short" ? "abbreviated" : "abbreviated(1)") + "}");
                }
                return patternString;
            }

            function correctDayHourMinuteSecondMonthPattern(patternString, userValue, searchParam) {
                // parts[1] is either month, day, hour, minute, or second
                // REVIEW (doilij) is it even possible to have a '.solo' (i.e. /(?:\\.solo)?/ ) in the above cases?
                var parts = platform.builtInRegexMatch(patternString, RegExp("{(" + searchParam + ")(?:\\.solo)?\\.([a-z]*)(?:\\([0-9]\\))?}"));
                if (parts === null) {
                    RaiseAssert(new Error("Error when correcting windows returned day/hour/minute/second/month pattern; regex returned null. \nInput was: '" + patternString + "'\nRegex: '" + "{(" + searchParam + "(\\.solo)?)\\.([a-z]*)(\\([0-9]\\))?}'"));
                    return patternString;
                }

                // Only correct the 2 digit; unless part[2] isn't integer
                if (userValue === "2-digit") {
                    return callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{" + parts[1] + ".integer(2)}");
                } else if (parts[2] !== "integer") {
                    return callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{" + parts[1] + ".integer}");
                }

                return patternString;
            }

            // Perhaps the level of validation that we have might not be required for this method
            function updatePatternStrings(patternString, dateTimeFormat) {
                if (dateTimeFormat.__weekday !== undefined) {
                    patternString = correctWeekdayEraMonthPattern(patternString, dateTimeFormat.__weekday, "dayofweek");
                }

                if (dateTimeFormat.__era !== undefined) {
                    // This is commented because not all options are supported for locales that do have era;
                    // In addition, we can't force era to be part of a locale using templates.
                    // patternString = correctWeekdayEraMonthPattern(patternString, dateTimeFormat.__era, "era", 2);
                }

                if (dateTimeFormat.__year === "2-digit") {
                    var parts = platform.builtInRegexMatch(patternString, /\{year\.[a-z]*(\([0-9]\))?\}/);
                    if (parts === null) {
                        RaiseAssert(new Error("Error when correcting windows returned year; regex returned null"));
                    } else {
                        patternString = callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{year.abbreviated(2)}");
                    }
                } else if (dateTimeFormat.__year === "full") {
                    var parts = platform.builtInRegexMatch(patternString, /\{year\.[a-z]*(\([0-9]\))?\}/);
                    if (parts === null) {
                        RaiseAssert(new Error("Error when correcting windows returned year; regex returned null"));
                    } else {
                        patternString = callInstanceFunc(StringInstanceReplace, patternString, parts[0], "{year.full}");
                    }
                }

                // Month partially overlaps with weekday/month; unless it's 2-digit or numeric in which case it overlaps with day/hour/minute/second
                if (dateTimeFormat.__month !== undefined && dateTimeFormat.__month !== "2-digit" && dateTimeFormat.__month !== "numeric") {
                    patternString = correctWeekdayEraMonthPattern(patternString, dateTimeFormat.__month, "month");
                } else if (dateTimeFormat.__month !== undefined) {
                    patternString = correctDayHourMinuteSecondMonthPattern(patternString, dateTimeFormat.__month, "month");
                }

                if (dateTimeFormat.__day !== undefined) {
                    patternString = correctDayHourMinuteSecondMonthPattern(patternString, dateTimeFormat.__day, "day");
                }

                if (dateTimeFormat.__hour !== undefined) {
                    patternString = correctDayHourMinuteSecondMonthPattern(patternString, dateTimeFormat.__hour, "hour");
                }

                if (dateTimeFormat.__minute !== undefined) {
                    patternString = correctDayHourMinuteSecondMonthPattern(patternString, dateTimeFormat.__minute, "minute");
                }

                if (dateTimeFormat.__second !== undefined) {
                    patternString = correctDayHourMinuteSecondMonthPattern(patternString, dateTimeFormat.__second, "second");
                }

                if (dateTimeFormat.__timeZoneName !== undefined) {
                    patternString = correctWeekdayEraMonthPattern(patternString, dateTimeFormat.__timeZoneName, "timezone");
                }

                return patternString;
            }

            function InitializeDateTimeFormat(dateTimeFormat, localeList, options) {
                if (typeof dateTimeFormat != "object") {
                    platform.raiseNeedObject();
                }

                if (callInstanceFunc(ObjectInstanceHasOwnProperty, dateTimeFormat, '__initializedIntlObject') && dateTimeFormat.__initializedIntlObject) {
                    platform.raiseObjectIsAlreadyInitialized("DateTimeFormat", "DateTimeFormat");
                }

                dateTimeFormat.__initializedIntlObject = true;

                // Extract the options
                options = ToDateTimeOptions(options, "any", "date");

                var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
                var timeZone = GetOption(options, "timeZone", "string", undefined, undefined);

                if (timeZone !== undefined) {
                    timeZone = platform.validateAndCanonicalizeTimeZone(timeZone);
                } else {
                    timeZone = platform.getDefaultTimeZone();
                }

                if (timeZone === undefined) {
                    platform.raiseOptionValueOutOfRange();
                }

                // Format options
                var weekday = GetOption(options, "weekday", "string", ['narrow', 'short', 'long'], undefined);
                var era = GetOption(options, "era", "string", ['narrow', 'short', 'long'], undefined);
                var year = GetOption(options, "year", "string", ['2-digit', 'numeric'], undefined);
                var month = GetOption(options, "month", "string", ['2-digit', 'numeric', 'narrow', 'short', 'long'], undefined);
                var day = GetOption(options, "day", "string", ['2-digit', 'numeric'], undefined);
                var hour = GetOption(options, "hour", "string", ['2-digit', 'numeric'], undefined);
                var minute = GetOption(options, "minute", "string", ['2-digit', 'numeric'], undefined);
                var second = GetOption(options, "second", "string", ['2-digit', 'numeric'], undefined);
                var timeZoneName = GetOption(options, "timeZoneName", "string", ['short', 'long'], undefined);

                var hour12 = hour ? GetOption(options, "hour12", "boolean", undefined, undefined) : undefined;
                var formatMatcher = GetOption(options, "formatMatcher", "string", ["basic", "best fit"], "best fit");

                var windowsClock = hour12 !== undefined ? (hour12 ? "12HourClock" : "24HourClock") : undefined;

                var templateString = EcmaOptionsToWindowsTemplate(setPrototype({
                    weekday: weekday,
                    era: era,
                    year: year,
                    month: month,
                    day: day,
                    hour: hour,
                    minute: minute,
                    second: second,
                    timeZoneName: timeZoneName
                }, null));

                // Deal with the locale
                localeList = CanonicalizeLocaleList(localeList);
                var resolvedLocaleInfo = resolveLocales(localeList, matcher, ["nu", "ca"]);

                // Assign the options
                dateTimeFormat.__matcher = matcher;
                dateTimeFormat.__timeZone = timeZone;
                dateTimeFormat.__locale = resolvedLocaleInfo.locale;

                // Format options
                dateTimeFormat.__weekday = weekday;
                dateTimeFormat.__era = era;
                dateTimeFormat.__year = year;
                dateTimeFormat.__month = month;
                dateTimeFormat.__day = day;
                dateTimeFormat.__hour = hour;
                dateTimeFormat.__minute = minute;
                dateTimeFormat.__second = second;
                dateTimeFormat.__timeZoneName = timeZoneName;

                dateTimeFormat.__hour12 = hour12;
                dateTimeFormat.__formatMatcher = formatMatcher;
                dateTimeFormat.__windowsClock = windowsClock;

                dateTimeFormat.__templateString = templateString;

                /*
                 * NOTE:
                 * Pattern string's are position-sensitive; while templates are not.
                 * If we specify {hour.integer(2)}:{minute.integer(2)} pattern string; we will always format as HH:mm.
                 * On the other hand, template strings don't give as fine granularity for options; and the platform decides how long month.abbreviated should be.
                 * Therefore, we have to create using template strings; and then change the .abbreivated/.integer values to have correct digits count if necessary.
                 * Thus, this results in this redundant looking code to create dateTimeFormat twice.
                 */
                var errorThrown = false;

                try {
                    // Create the DateTimeFormatter to extract pattern strings
                    CreateDateTimeFormat(dateTimeFormat, false);
                } catch (e) {
                    // Rethrow SOE or OOM
                    throwExIfOOMOrSOE(e);

                    // We won't throw for the first exception, but assume the template strings were rejected.
                    // Instead, we will try to fall back to default template strings.
                    var defaultOptions = ToDateTimeOptions(options, "none", "all");
                    dateTimeFormat.__templateString = EcmaOptionsToWindowsTemplate(defaultOptions, null);
                    errorThrown = true;
                }

                if (!errorThrown) {
                    // Update the pattern strings
                    dateTimeFormat.__templateString = updatePatternStrings(dateTimeFormat.__patternStrings[0], dateTimeFormat);
                }

                try {
                    // Cache the date time formatter
                    CreateDateTimeFormat(dateTimeFormat, true);
                } catch (e) {
                    // Rethrow SOE or OOM
                    throwExIfOOMOrSOE(e);

                    // Otherwise, Generic message to cover the exception throw from the platform.
                    // The platform's exception is also generic and in most if not all cases specifies that "a" argument is invalid.
                    // We have no other information from the platform on the cause of the exception.
                    platform.raiseOptionValueOutOfRange();
                }

                // Correct the api's updated
                dateTimeFormat.__calendar = WindowsToEcmaCalendar(dateTimeFormat.__windowsCalendar);

                dateTimeFormat.__numberingSystem = callInstanceFunc(StringInstanceToLowerCase, dateTimeFormat.__numberingSystem);
                if (dateTimeFormat.__hour !== undefined) {
                    dateTimeFormat.__hour12 = dateTimeFormat.__windowsClock === "12HourClock";
                }
                dateTimeFormat.__initializedDateTimeFormat = true;
            }

            // caches for objects constructed with default parameters for each method
            let __DateInstanceToLocaleStringDefaultCache = [undefined, undefined, undefined];
            const __DateInstanceToLocaleStringDefaultCacheSlot = setPrototype({
                toLocaleString: 0,
                toLocaleDateString: 1,
                toLocaleTimeString: 2
            }, null);

            function DateInstanceToLocaleStringImplementation(name, option1, option2, cacheSlot, locales, options) {
                if (typeof this !== 'object' || !(this instanceof Date)) {
                    platform.raiseNeedObjectOfType(name, "Date");
                }
                let value = callInstanceFunc(DateInstanceGetDate, new Date(this));
                if (isNaN(value) || !isFinite(value)) {
                    return "Invalid Date";
                }

                let stateObject = undefined;
                if (platform.useCaches && !locales && !options) {
                    // All default parameters (locales and options): this is the most valuable case to cache.
                    if (__DateInstanceToLocaleStringDefaultCache[cacheSlot]) {
                        // retrieve cached value
                        stateObject = __DateInstanceToLocaleStringDefaultCache[cacheSlot];
                    } else {
                        // populate cache
                        stateObject = setPrototype({}, null);
                        InitializeDateTimeFormat(stateObject, undefined, ToDateTimeOptions(undefined, option1, option2));
                        __DateInstanceToLocaleStringDefaultCache[cacheSlot] = stateObject;
                    }
                }

                if (!stateObject) {
                    stateObject = setPrototype({}, null);
                    InitializeDateTimeFormat(stateObject, locales, ToDateTimeOptions(options, option1, option2));
                }

                return String(platform.formatDateTime(Internal.ToNumber(this), stateObject));
            }

            // Note: tagPublicFunction (platform.tagPublicLibraryCode) messes with declared name of the FunctionBody so that
            // the functions called appear correctly in the debugger and stack traces. Thus, we we cannot call tagPublicFunction in a loop.
            // Each entry point needs to have its own unique FunctionBody (which is a function as defined in the source code);
            // this is why we have seemingly repeated ourselves below, instead of having one function and calling it multiple times with
            // different parameters.
            //
            // The following invocations of `platform.registerBuiltInFunction(tagPublicFunction(name, entryPoint))` are enclosed in IIFEs.
            // The IIFEs are used to group all of the meaningful differences between each entry point into the arguments to the IIFE.
            // The exception to this are the different entryPoint names which are only significant for debugging (and cannot be passed in
            // as arguments, as the name is intrinsic to the function declaration).
            //
            // The `date_toLocale*String_entryPoint` function names are placeholder names that will never be seen from user code.
            // The function name property and FunctionBody declared name are overwritten by `tagPublicFunction`.
            // The fact that they are declared with unique names is helpful for debugging.
            // The functions *must not* be declared as anonymous functions (must be declared with a name);
            // converting from an unnnamed function to a named function is not readily supported by the platform code and
            // this has caused us to hit assertions in debug builds in the past.
            //
            // See invocations of `tagPublicFunction` on the `supportedLocalesOf` entry points for a similar pattern.
            //
            // The entryPoint functions will be called as `Date.prototype.toLocale*String` and thus their `this` parameters will be a Date.
            // `DateInstanceToLocaleStringImplementation` is not on `Date.prototype`, so we must propagate `this` into the call by using
            // `DateInstanceToLocaleStringImplementation.call(this, ...)`.

            (function (name, option1, option2, cacheSlot, platformFunctionID) {
                platform.registerBuiltInFunction(tagPublicFunction(name, function date_toLocaleString_entryPoint(locales = undefined, options = undefined) {
                    return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
                }), platformFunctionID);
            })("Date.prototype.toLocaleString", "any", "all", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleString, IntlBuiltInFunctionID.DateToLocaleString);

            (function (name, option1, option2, cacheSlot, platformFunctionID) {
                platform.registerBuiltInFunction(tagPublicFunction(name, function date_toLocaleDateString_entryPoint(locales = undefined, options = undefined) {
                    return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
                }), platformFunctionID);
            })("Date.prototype.toLocaleDateString", "date", "date", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleDateString, IntlBuiltInFunctionID.DateToLocaleDateString);

            (function (name, option1, option2, cacheSlot, platformFunctionID) {
                platform.registerBuiltInFunction(tagPublicFunction(name, function date_toLocaleTimeString_entryPoint(locales = undefined, options = undefined) {
                    return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
                }), platformFunctionID);
            })("Date.prototype.toLocaleTimeString", "time", "time", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleTimeString, IntlBuiltInFunctionID.DateToLocaleTimeString);

            if (InitType === 'Intl') {
                function DateTimeFormat(locales = undefined, options = undefined) {
                    if (this === Intl || this === undefined) {
                        return new DateTimeFormat(locales, options);
                    }

                    let obj = Internal.ToObject(this);
                    if (!ObjectIsExtensible(obj)) {
                        platform.raiseObjectIsNonExtensible("DateTimeFormat");
                    }

                    // Use the hidden object to store data
                    let hiddenObject = platform.getHiddenObject(obj);

                    if (hiddenObject === undefined) {
                        hiddenObject = setPrototype({}, null);
                        platform.setHiddenObject(obj, hiddenObject);
                    }

                    InitializeDateTimeFormat(hiddenObject, locales, options);

                    hiddenObject.__boundFormat = callInstanceFunc(FunctionInstanceBind, format, obj);
                    delete hiddenObject.__boundFormat.name;

                    return obj;
                }
                tagPublicFunction("Intl.DateTimeFormat", DateTimeFormat);

                function format(date) {
                    if (typeof this !== 'object') {
                        platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
                    }
                    let hiddenObject = platform.getHiddenObject(this);
                    if (hiddenObject === undefined || !hiddenObject.__initializedDateTimeFormat) {
                        platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
                    }

                    if (isNaN(date) || !isFinite(date)) {
                        platform.raiseInvalidDate();
                    }

                    let dateValue = undefined;
                    if (date) {
                        dateValue = Internal.ToNumber(date)
                    } else {
                        dateValue = DateNow();
                    }

                    return String(platform.formatDateTime(dateValue, hiddenObject));
                }
                tagPublicFunction("Intl.DateTimeFormat.prototype.format", format);

                DateTimeFormat.__relevantExtensionKeys = ['ca', 'nu'];

                ObjectDefineProperty(DateTimeFormat, 'prototype', { value: new DateTimeFormat(), writable: false, enumerable: false, configurable: false });
                setPrototype(DateTimeFormat.prototype, Object.prototype);
                ObjectDefineProperty(DateTimeFormat.prototype, 'constructor', { value: DateTimeFormat, writable: true, enumerable: false, configurable: true });

                ObjectDefineProperty(DateTimeFormat.prototype, 'format', {
                    get: tagPublicFunction('get format', function () {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
                        }

                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedDateTimeFormat) {
                            platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
                        }

                        return hiddenObject.__boundFormat;
                    }), enumerable: false, configurable: true
                });

                ObjectDefineProperty(DateTimeFormat.prototype, 'resolvedOptions', {
                    value: function resolvedOptions() {
                        if (typeof this !== 'object') {
                            platform.raiseNeedObjectOfType("DateTimeFormat.prototype.resolvedOptions", "DateTimeFormat");
                        }
                        var hiddenObject = platform.getHiddenObject(this);
                        if (hiddenObject === undefined || !hiddenObject.__initializedDateTimeFormat) {
                            platform.raiseNeedObjectOfType("DateTimeFormat.prototype.resolvedOptions", "DateTimeFormat");
                        }
                        var temp = setPrototype({
                            locale: hiddenObject.__locale,
                            calendar: hiddenObject.__calendar, // ca unicode extension
                            numberingSystem: hiddenObject.__numberingSystem, // nu unicode extension
                            timeZone: hiddenObject.__timeZone,
                            hour12: hiddenObject.__hour12,
                            weekday: hiddenObject.__weekday,
                            era: hiddenObject.__era,
                            year: hiddenObject.__year,
                            month: hiddenObject.__month,
                            day: hiddenObject.__day,
                            hour: hiddenObject.__hour,
                            minute: hiddenObject.__minute,
                            second: hiddenObject.__second,
                            timeZoneName: hiddenObject.__timeZoneName
                        }, null)
                        var options = setPrototype({}, null);
                        callInstanceFunc(ArrayInstanceForEach, ObjectGetOwnPropertyNames(temp), function (prop) {
                            if ((temp[prop] !== undefined || prop === 'timeZone') && callInstanceFunc(ObjectInstanceHasOwnProperty, hiddenObject, "__" + prop)) {
                                options[prop] = temp[prop];
                            }
                        }, hiddenObject);
                        return setPrototype(options, Object.prototype);
                    }, writable: true, enumerable: false, configurable: true
                });

                ObjectDefineProperty(DateTimeFormat, 'supportedLocalesOf', { value: dateTimeFormat_supportedLocalesOf, writable: true, configurable: true });

                return DateTimeFormat;
            }
        }
        // 'Init.DateTimeFormat' not defined if reached here. Return 'undefined'
        return undefined;
    })();

    // Initialize Intl properties only if needed
    if (InitType === 'Intl') {
        ObjectDefineProperty(Intl, "Collator",              { value: Collator,              writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "NumberFormat",          { value: NumberFormat,          writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "DateTimeFormat",        { value: DateTimeFormat,        writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "getCanonicalLocales",   { value: getCanonicalLocales,   writable: true, enumerable: false, configurable: true });
    }
});
