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
    const NOT_FOUND = "NOT_FOUND";

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
            this._cache = _.create();
        }

        get(key) {
            return platform.useCaches ? this._cache[key] : undefined;
        }

        set(key, value) {
            if (!platform.useCaches) {
                return;
            }

            if (_.keys(this._cache).length > this.n && this._cache[key] === undefined) {
                this._cache = _.create();
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

    var Math = setPrototype({
        abs: platform.builtInMathAbs,
        floor: platform.builtInMathFloor,
        max: platform.builtInMathMax,
        pow: platform.builtInMathPow
    }, null);

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

    var isFinite = platform.builtInGlobalObjectEntryIsFinite;
    var isNaN = platform.builtInGlobalObjectEntryIsNaN;

    const _ = {
        toUpperCase(str) { return callInstanceFunc(StringInstanceToUpperCase, str); },
        toLowerCase(str) { return callInstanceFunc(StringInstanceToLowerCase, str); },
        replace(str, pattern, replacement) { return callInstanceFunc(StringInstanceReplace, str, pattern, replacement); },
        split(str, pattern) { return callInstanceFunc(platform.builtInJavascriptStringEntrySplit, str, pattern); },
        substring(str, start, end) { return callInstanceFunc(platform.builtInJavascriptStringEntrySubstring, str, start, end); },
        stringIndexOf(str, el, from) { return callInstanceFunc(platform.builtInJavascriptStringEntryIndexOf, str, el, from); },
        match(str, re) { return platform.builtInRegexMatch(str, re); },
        repeat(str, count) { return callInstanceFunc(platform.builtInJavascriptStringEntryRepeat, str, count); },

        forEach(array, func) { return callInstanceFunc(ArrayInstanceForEach, array, func); },
        push(array, ...els) { return callInstanceFunc(ArrayInstancePush, array, ...els); },
        join(array, sep) { return callInstanceFunc(ArrayInstanceJoin, array, sep); },
        arrayIndexOf(array, el, from) { return callInstanceFunc(ArrayInstanceIndexOf, array, el, from); },
        map(array, func) { return callInstanceFunc(platform.builtInJavascriptArrayEntryMap, array, func); },
        reduce(array, func, init) { return callInstanceFunc(platform.builtInJavascriptArrayEntryReduce, array, func, init); },
        slice(array, start, end) { return callInstanceFunc(platform.builtInJavascriptArrayEntrySlice, array, start, end); },
        concat(array, ...els) { return callInstanceFunc(platform.builtInJavascriptArrayEntryConcat, array, ...els); },
        filter(array, func) { return callInstanceFunc(platform.builtInJavascriptArrayEntryFilter, array, func); },
        unique(array) { return _.filter(array, (v, i) => _.arrayIndexOf(array, v) === i); },

        keys: platform.builtInJavascriptObjectEntryKeys,
        hasOwnProperty(o, prop) { return callInstanceFunc(platform.builtInJavascriptObjectEntryHasOwnProperty, o, prop); },
        // If we don't set the descriptor's prototype to null, defining properties with `value`s can fail of Object.prototype.get is defined
        defineProperty(o, prop, desc) {
            _.setPrototypeOf(desc, null);
            platform.builtInJavascriptObjectEntryDefineProperty(o, prop, desc);
        },
        isExtensible: platform.builtInJavascriptObjectEntryIsExtensible,
        create(proto = null) { return platform.builtInJavascriptObjectEntryCreate(proto); },
        setPrototypeOf(target, proto = null) { return platform.builtInSetPrototype(target, proto); },

        abs: platform.builtInMathAbs,
        // Make _.floor more like ECMA262 #sec-mathematical-operations' floor by normalizing -0
        floor(x) { return x === 0 ? 0 : platform.builtInMathFloor(x) },
        max: platform.builtInMathMax,
        pow: platform.builtInMathPow,

        isFinite: platform.builtInGlobalObjectEntryIsFinite,
        isNaN: platform.builtInGlobalObjectEntryIsNaN,

        getDate(date) { return callInstanceFunc(platform.builtInJavascriptDateEntryGetDate, date); },

        bind(func, that) { return callInstanceFunc(platform.builtInJavascriptFunctionEntryBind, func, that); },
        apply(func, that, args) { return callInstanceFunc(platform.builtInJavascriptFunctionEntryApply, func, that, args); },
    };

    // When this file was originally written, it assumed Windows Globalization semantics.
    // Throughout the transition to ICU, we tried to share as much code as possible between WinGlob and ICU.
    // However, because ICU has different semantics and our ICU-based implementation tries to match a newer
    // version of the Intl spec, we have decided that the code sharing was causing more harm than good.
    // Thus, while we support both ICU and WinGlob, we have decided to duplicate a substantial amount of code.
    // The indentation of the below if block is intentionally incorrect so as to minimize diff.
    if (isPlatformUsingICU) {

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

        return __defaultLocale;
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

    const createPublicMethod = function (name, f) {
        return platform.tagPublicLibraryCode(f, name, false);
    }

    const OrdinaryCreateFromConstructor = function (constructor, intrinsicDefaultProto) {
        let proto = constructor.prototype;
        if (typeof proto !== "object") {
            proto = intrinsicDefaultProto;
        }

        return _.create(proto);
    };

    /**
     * Determines the best possible locale available in the system
     *
     * ECMA-402: #sec-bestavailablelocale
     *
     * @param {Function} isAvailableLocale A function that takes a locale and returns if the locale is supported
     * @param {String} locale the locale (including its fallbacks) that will be searched for
     * @returns {String} the given locale or one of its fallbacks, or undefined
     */
    const BestAvailableLocale = function (isAvailableLocale, locale) {
        if (locale === undefined) {
            return undefined;
        }

        let candidate = locale;
        const hyphen = "-";
        while (true) {
            if (isAvailableLocale(candidate)) {
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
                // This is spec code likely intended to skip over singletons,
                // such that if we just searched for "en-a-value",
                // pos would initially truncate the candidate to "en-a", which
                // is not a valid language tag.
                // See https://tools.ietf.org/html/rfc5646#section-4.4.2
                pos -= 2;
            }

            candidate = _.substring(candidate, 0, pos);
        }
    };

    /**
     * Determines which locale (or fallback) to use of an array of locales.
     *
     * ECMA-402: #sec-lookupmatcher
     *
     * @param {Function} isAvailableLocale A function that takes a locale and returns if the locale is supported
     * @param {String[]} requestedLocales An array of requested locales
     */
    const LookupMatcher = function (isAvailableLocale, requestedLocales) {
        const result = _.create();
        for (let i = 0; i < requestedLocales.length; ++i) {
            const parsedLangtag = parseLangtag(requestedLocales[i]);
            if (parsedLangtag === null) {
                continue;
            }

            const availableLocale = BestAvailableLocale(isAvailableLocale, parsedLangtag.base);
            if (availableLocale !== undefined) {
                result.locale = availableLocale;
                if (requestedLocales[i] !== parsedLangtag.base) {
                    result.extension = parsedLangtag.unicodeExtension;
                }

                return result;
            }
        }

        result.locale = GetDefaultLocale();
        return result;
    };

    const BestFitMatcher = LookupMatcher;

    /**
     * Determine a value for a given key in the given extension string
     *
     * ECMA-402: #sec-unicodeextensionvalue
     *
     * @param {String} extension the full unicode extension, such as "-u-co-phonebk-kf-true"
     * @param {String} key the specific key we are looking for in the extension, such as "co"
     */
    const UnicodeExtensionValue = function (extension, key) {
        const size = extension.length;

        // search for the key-value pair
        let pos = _.stringIndexOf(extension, `-${key}-`);
        if (pos !== -1) {
            const start = pos + 4;
            let end = start;
            let k = start;
            let done = false;
            while (!done) {
                const e = _.stringIndexOf(extension, "-", k);
                const len = e === -1 ? size - k : e - k;
                if (len === 2) {
                    done = true;
                } else if (e === -1) {
                    end = size;
                    done = true;
                } else {
                    end = e;
                    k = e + 1;
                }
            }

            return _.substring(extension, start, end);
        }

        // search for the key with no associated value
        pos = _.stringIndexOf(extension, `-${key}`);
        if (pos !== -1 && pos + 3 === size) {
            return "";
        } else {
            return undefined;
        }
    };

    /**
     * Resolves a locale by finding which base locale or fallback is available on the system,
     * then determines which provided unicode options are available for that locale.
     *
     * ECMA-402: #sec-resolvelocale
     *
     * @param {Function} isAvailableLocale A function that takes a locale and returns if the locale is supported
     * @param {String[]} requestedLocales The result of calling CanonicalizeLocaleList on the user-requested locale array
     * @param {Object} options An object containing a lookupMatcher value and any value given by the user's option object,
     * mapped to the correct unicode extension key
     * @param {String[]} relevantExtensionKeys An array of unicode extension keys that we care about for the current lookup
     */
    const ResolveLocale = function (isAvailableLocale, requestedLocales, options, relevantExtensionKeys) {
        const matcher = options.lookupMatcher;
        let r;
        if (matcher === "lookup") {
            r = LookupMatcher(isAvailableLocale, requestedLocales);
        } else {
            r = BestFitMatcher(isAvailableLocale, requestedLocales);
        }

        let foundLocale = r.locale;
        const result = bare({ dataLocale: foundLocale });
        let supportedExtension = "-u";
        _.forEach(relevantExtensionKeys, function (key) {
            const keyLocaleData = platform.getLocaleData(platform.LocaleDataKind[key], foundLocale);
            let value = keyLocaleData[0];
            let supportedExtensionAddition = "";
            if (r.extension) {
                const requestedValue = UnicodeExtensionValue(r.extension, key);
                if (requestedValue !== undefined) {
                    if (requestedValue !== "") {
                        if (_.arrayIndexOf(keyLocaleData, requestedValue) !== -1) {
                            value = requestedValue;
                            supportedExtensionAddition = `-${key}-${value}`;
                        }
                    } else if (_.arrayIndexOf(keyLocaleData, "true") !== -1) {
                        value = "true";
                    }
                }
            }

            if (_.hasOwnProperty(options, key)) {
                const optionsValue = options[key];
                if (_.arrayIndexOf(keyLocaleData, optionsValue) !== -1) {
                    if (optionsValue !== value) {
                        value = optionsValue;
                        supportedExtensionAddition = "";
                    }
                }
            }

            result[key] = value;
            supportedExtension += supportedExtensionAddition;
        });

        if (supportedExtension.length > 2) {
            const privateIndex = _.stringIndexOf(foundLocale, "-x-");
            if (privateIndex === -1) {
                foundLocale += supportedExtension;
            } else {
                const preExtension = _.substring(foundLocale, 0, privateIndex);
                const postExtension = _.substring(foundLocale, privateIndex);
                foundLocale = preExtension + supportedExtension + postExtension;
            }

            foundLocale = platform.normalizeLanguageTag(foundLocale);
        }

        result.locale = foundLocale;
        return result;
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

        ToInteger(n) {
            const number = Number(n);
            if (isNaN(number)) {
                return 0;
            } else if (number === 0 || !isFinite(number)) {
                return number;
            }

            const ret = _.floor(_.abs(number));
            if (number < 0) {
                return -ret
            } else {
                return ret;
            }
        },

        ToLength(n) {
            const len = Internal.ToInteger(n);
            if (len <= 0) {
                return 0;
            }

            const max = _.pow(2, 53) - 1;
            return max < len ? max : len;
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

            if (values !== undefined && _.arrayIndexOf(values, value) == -1) {
                platform.raiseOptionValueOutOfRange_3(String(value), String(property), `['${_.join(values, "', '")}']`);
            }

            return value;
        }

        return fallback;
    }

    /**
     * Extracts the value of the property named property from the provided options object,
     * converts it to a Number value, checks whether it is in the allowed range,
     * and fills in a fallback value if necessary.
     *
     * NOTE: this has known differences compared to the spec GetNumberOption in order to
     * support more verbose errors. It is more similar to DefaultNumberOption
     *
     * ECMA402: #sec-defaultnumberoption
     *
     * @param {Object} options user-provided options object
     * @param {String} property the property we are trying to get off of `options`
     * @param {Number} minimum minimum allowable value for options[property]
     * @param {Number} maximum maximum allowable value for options[property]
     * @param {Number} fallback return value if options[property] is undefined or invalid
     * @returns {Number}
     */
    const GetNumberOption = function (options, property, minimum, maximum, fallback) {
        let value = options[property];
        if (value !== undefined) {
            value = Internal.ToNumber(value);
            if (_.isNaN(value) || value < minimum || value > maximum) {
                platform.raiseOptionValueOutOfRange_3(String(value), property, `[${minimum} - ${maximum}]`);
            }
            return _.floor(value);
        }

        return fallback;
    };

    let CURRENCY_CODE_RE;
    function InitializeCurrencyRegExp() {
        CURRENCY_CODE_RE = /^[A-Z]{3}$/i;
    }

    /**
     * Returns an object representing the language, script, region, extension, and base of a language tag,
     * or null if the language tag isn't valid.
     *
     * @param {String} langtag a candidate BCP47 langtag
     */
    const parseLangtag = (function () {
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

        // Use matching groups only when needed
        const LANG_TAG_BASE = `\\b(${language})\\b`         +                   // langtag       = language
                              `\\b(?:-(${script}))?\\b`     +                   //                 ["-" script]
                              `\\b(?:-(${region}))?\\b`     ;                   //                 ["-" region]
        const LANG_TAG_EXT  = `\\b((?:-${variant})*)\\b`    +                   //                 *("-" variant)
                              `\\b((?:-${extension})*)\\b`  +                   //                 *("-" extension)
                              `\\b(?:-${privateuse})?\\b`   ;                   //                 ["-" privateuse]
        const langtag       = `\\b${LANG_TAG_BASE}\\b${LANG_TAG_EXT}\\b`;

        const LANG_TAG      = `\\b(?:${langtag}|${privateuse}|${grandfathered})\\b`;  // Language-Tag  = ...

        // Use ^ and $ to enforce that the entire input string is a langtag
        const LANG_TAG_BASE_RE    = new RegExp(`^${LANG_TAG_BASE}$`, 'i'); // [1] language; [2] script; [3] region;
        const LANG_TAG_EXT_RE     = new RegExp(`^${LANG_TAG_EXT}$`,  'i'); //                                       [1] variants; [2] extensions;
        const LANG_TAG_RE         = new RegExp(`^${LANG_TAG}$`,      'i'); // [1] language; [2] script; [3] region; [4] variants; [5] extensions;

        const parsedLangtagCache = new IntlCache();
        return function (langtag) {
            const cached = parsedLangtagCache.get(langtag);
            if (cached) {
                return cached;
            }

            const parts = _.match(langtag, LANG_TAG_RE);
            if (!parts) {
                return null;
            }

            const ret = _.create();
            ret.language = parts[1];
            ret.base = parts[1];
            if (parts[2]) {
                ret.script = parts[2];
                ret.base += "-" + parts[2];
            }

            if (parts[3]) {
                ret.region = parts[3];
                ret.base += "-" + parts[3];
            }

            if (parts[4]) {
                ret.variants = parts[4];
            }

            if (parts[5]) {
                ret.extensions = parts[5];

                // parse the extension to find the unicode (-u) extension
                const extensionParts = _.split(parts[5], "-");
                for (let i = 0; i < extensionParts.length; ++i) {
                    if (extensionParts[i] !== "u") {
                        continue;
                    }

                    let k;
                    for (k = i + 1; k < extensionParts.length && extensionParts[k].length > 1; k++) {
                        // do nothing, we just want k to equal the index of the next element whose length is 1
                        // or to equal the length of extensionParts
                        // We could have done this with Array.prototype.findIndex too
                    }

                    if (k > i + 1) {
                        // this creates u-(keys and values)*, which is good enough for the UnicodeExtensionValue,
                        // which is the only place that this return value is intended to be used
                        ret.unicodeExtension = _.join(_.slice(extensionParts, i, k), "-");
                    }

                    // if we have gotten this far, we have found -u-{values}, so we can break
                    break;
                }
            }

            parsedLangtagCache.set(langtag, ret);

            return ret;
        };
    })();

    const IsWellFormedCurrencyCode = function (code) {
        code = Internal.ToString(code);

        if (!CURRENCY_CODE_RE) {
            InitializeCurrencyRegExp();
        }

        return platform.builtInRegexMatch(code, CURRENCY_CODE_RE) !== null;
    }

    /**
     * Returns true if locale can be generated by RFC5646 section 2.1 and does not contain
     * duplicate variant or singleton subtags.
     *
     * Note that ICU does not implement this correctly for our usage because it is
     * extremely permissive about what it will allow -- completely invalid language tags can
     * pass through a round of uloc_forLanguageTag/uloc_toLanguageTag or uloc_canonicalize
     * even if they are completely bogus.
     *
     * ECMA402: #sec-isstructurallyvalidlanguagetag
     *
     * @param {String} locale The locale to check
     * @returns {Boolean}
     */
    const IsStructurallyValidLanguageTag = function (locale) {
        const parsed = parseLangtag(locale);
        if (parsed === null) {
            return false;
        }

        // check duplicate variants
        if (parsed.variants) {
            const variants = _.split(parsed.variants, "-");
            const uniqueVariants = _.unique(variants);

            if (variants.length !== uniqueVariants.length) {
                return false;
            }
        }

        if (parsed.extensions) {
            const extensionParts = _.split(parsed.extensions, "-");
            const singletons = _.map(_.filter(extensionParts, (element) => element.length === 1), (element) => _.toLowerCase(element));
            const uniqueSingletons = _.unique(singletons);

            return singletons.length === uniqueSingletons.length;
        }

        return true;
    };

    /**
     * Given a locale or list of locales, returns a corresponding list where each locale
     * is guaranteed to be "canonical" (proper capitalization, order, etc.).
     *
     * ECMA402: #sec-canonicalizelocalelist
     *
     * @param {String|String[]} locales the user-provided locales to be canonicalized
     */
    const CanonicalizeLocaleList = function (locales) {
        if (typeof locales === "undefined") {
            return [];
        }

        const seen = [];
        const O = typeof locales === "string" ? [locales] : Internal.ToObject(locales);
        const len = Internal.ToLength(O.length);
        let k = 0;

        while (k < len) {
            const Pk = Internal.ToString(k);
            if (Pk in O) {
                const kValue = O[Pk];
                if ((typeof kValue !== "string" && typeof kValue !== "object") || kValue === null) {
                    platform.raiseNeedObjectOrString("locale");
                }

                const tag = Internal.ToString(kValue);
                if (!IsStructurallyValidLanguageTag(tag)) {
                    platform.raiseLocaleNotWellFormed(tag);
                }

                const canonicalizedTag = platform.normalizeLanguageTag(tag);
                if (canonicalizedTag === undefined) {
                    // See comment in platform.normalizeLanguageTag about when this happens
                    platform.raiseLocaleNotWellFormed(tag);
                } else if (_.arrayIndexOf(seen, canonicalizedTag) === -1) {
                    _.push(seen, canonicalizedTag);
                }
            }

            k += 1;
        }

        return seen;
    };

    /**
     * Returns the subset of requestedLocales that has a matching locale according to BestAvailableLocale.
     *
     * ECMA402: #sec-lookupsupportedlocales
     *
     * @param {Function} isAvailableLocale A function that takes a locale and returns if the locale is supported
     * @param {String|String[]} requestedLocales
     */
    const LookupSupportedLocales = function (isAvailableLocale, requestedLocales) {
        const subset = [];
        _.forEach(requestedLocales, function (locale) {
            const noExtensionsLocale = parseLangtag(locale).base;
            if (BestAvailableLocale(isAvailableLocale, noExtensionsLocale) !== undefined) {
                _.push(subset, locale);
            }
        });

        return subset;
    };

    const BestFitSupportedLocales = LookupSupportedLocales;

    /**
     * Applies digit options used for number formatting onto the given intlObj
     *
     * This function is used by both NumberFormat and PluralRules, despite being defined
     * as a NumberFormat abstract operation
     *
     * ECMA 402: #sec-setnfdigitoptions
     *
     * @param {Object} intlObj The state object of either a NumberFormat or PluralRules on which to set the resolved number options
     * @param {Object} options The option object to pull min/max sigfigs, fraction digits, and integer digits
     * @param {Number} mnfdDefault The default minimumFractionDigits
     * @param {Number} mxfdDefault The default maximumFractionDigits
     */
    const SetNumberFormatDigitOptions = function (intlObj, options, mnfdDefault, mxfdDefault) {
        const mnid = GetNumberOption(options, "minimumIntegerDigits", 1, 21, 1);
        const mnfd = GetNumberOption(options, "minimumFractionDigits", 0, 20, mnfdDefault);
        const mxfdActualDefault = _.max(mnfd, mxfdDefault);
        const mxfd = GetNumberOption(options, "maximumFractionDigits", mnfd, 20, mxfdActualDefault);
        intlObj.minimumIntegerDigits = mnid;
        intlObj.minimumFractionDigits = mnfd;
        intlObj.maximumFractionDigits = mxfd;

        let mnsd = options.minimumSignificantDigits;
        let mxsd = options.maximumSignificantDigits;
        if (mnsd !== undefined || mxsd !== undefined) {
            // don't read options.minimumSignificantDigits below in order to pass
            // test262/test/intl402/NumberFormat/significant-digits-options-get-sequence.js
            mnsd = GetNumberOption({ minimumSignificantDigits: mnsd }, "minimumSignificantDigits", 1, 21, 1);
            mxsd = GetNumberOption({ maximumSignificantDigits: mxsd }, "maximumSignificantDigits", mnsd, 21, 21);
            intlObj.minimumSignificantDigits = mnsd;
            intlObj.maximumSignificantDigits = mxsd;
        }
    };

    /**
     * Returns the subset of requestedLocales that has a matching locale, according to
     * options.localeMatcher and isAvailableLocale.
     *
     * ECMA402: #sec-supportedlocales
     *
     * @param {Function} isAvailableLocale A function that takes a locale and returns if the locale is supported
     * @param {String|String[]} requestedLocales
     * @param {Object} options
     */
    const SupportedLocales = function (isAvailableLocale, requestedLocales, options) {
        const matcher = options === undefined
            ? "best fit"
            : GetOption(Internal.ToObject(options), "localeMatcher", "string", ["best fit", "lookup"], "best fit");
        const supportedLocales = matcher === "best fit"
            ? BestFitSupportedLocales(isAvailableLocale, requestedLocales)
            : LookupSupportedLocales(isAvailableLocale, requestedLocales);

        for (let i = 0; i < supportedLocales.length; i++) {
            _.defineProperty(supportedLocales, Internal.ToString(i), { configurable: false, writable: false });
        }

        // test262 supportedLocalesOf-returned-array-elements-are-frozen.js:
        // Property length of object returned by SupportedLocales should not be writable
        _.defineProperty(supportedLocales, "length", {
            writable: false,
            configurable: false,
            enumerable: false,
        });

        return supportedLocales;
    };

    if (InitType === "Intl") {
        const getCanonicalLocales = createPublicMethod("Intl.getCanonicalLocales", function getCanonicalLocales(locales) {
            return CanonicalizeLocaleList(locales);
        });
        _.defineProperty(Intl, "getCanonicalLocales", {
            value: getCanonicalLocales,
            writable: true,
            enumerable: false,
            configurable: true
        });
    }

    /**
     * Creates an object to be returned out of resolvedOptions() methods that avoids being tainted by Object.prototype
     *
     * @param {String[]} props The list of properties to extract from hiddenObject and add to the final resolved options
     * @param {Object} hiddenObject The hiddenObject of the calling constructor that contains values for each prop in props
     * @param {Function} func An optional custom function(prop, resolved) run for each prop; it should return true when
     * it handles a property itself. If it does not return true, the default logic will be used.
     */
    const createResolvedOptions = function (props, hiddenObject, func = null) {
        const resolved = _.create();
        _.forEach(props, function (prop) {
            if (func !== null && func(prop, resolved) === true) {
                // the callback returned true, which means this property was handled and we can go to the next one
                return;
            }

            if (typeof hiddenObject[prop] !== "undefined") {
                resolved[prop] = hiddenObject[prop];
            }
        });

        return _.setPrototypeOf(resolved, platform.Object.prototype);
    };

    // Intl.Collator, String.prototype.localeCompare
    const Collator = (function () {
        if (InitType !== "Intl" && InitType !== "String") {
            return;
        }

        const InitializeCollator = function (collator, locales, options) {
            const requestedLocales = CanonicalizeLocaleList(locales);
            options = options === undefined ? _.create() : Internal.ToObject(options);

            // The spec says that usage dictates whether to use "[[SearchLocaleData]]" or "[[SortLocaleData]]"
            // ICU has no concept of a difference between the two, and instead sort/search corresponds to
            // collation = "standard" or collation = "search", respectively. Standard/sort is the default.
            // Thus, when the usage is sort, we can accept and honor -u-co in the locale, while if usage is search,
            // we are going to overwrite any -u-co value provided before giving the locale to ICU anyways.
            // To make the logic simpler, we can simply pretend like we don't accept a -u-co value if the usage is search.
            // See the lazy UCollator initialization in EntryIntl_LocaleCompare for where the collation value
            // gets overwritten by "search".
            collator.usage = GetOption(options, "usage", "string", ["sort", "search"], "sort");
            const relevantExtensionKeys = collator.usage === "sort" ? ["co", "kn", "kf"] : ["kn", "kf"];

            const opt = _.create();
            opt.matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
            let kn = GetOption(options, "numeric", "boolean", undefined, undefined);
            opt.kn = kn === undefined ? kn : Internal.ToString(kn);
            opt.kf = GetOption(options, "caseFirst", "string", ["upper", "lower", "false"], undefined);

            const r = ResolveLocale(platform.isCollatorLocaleAvailable, requestedLocales, opt, relevantExtensionKeys);
            collator.locale = r.locale;
            // r.co is null when usage === "sort" and no -u-co is provided
            // r.co is undefined when usage === "search", since relevantExtensionKeys doesn't even look for -co
            collator.collation = r.co === null || r.co === undefined ? "default" : r.co;
            collator.numeric = r.kn === "true";
            collator.caseFirst = r.kf;
            collator.caseFirstEnum = platform.CollatorCaseFirst[collator.caseFirst];

            collator.sensitivity = GetOption(options, "sensitivity", "string", ["base", "accent", "case", "variant"], "variant");
            collator.sensitivityEnum = platform.CollatorSensitivity[collator.sensitivity];

            collator.ignorePunctuation = GetOption(options, "ignorePunctuation", "boolean", undefined, false);

            collator.initializedCollator = true;

            return collator;
        };

        let localeCompareStateCache;
        // Make arguments undefined to ensure that localeCompare.length === 1
        platform.registerBuiltInFunction(createPublicMethod("String.prototype.localeCompare", function localeCompare(that, locales = undefined, options = undefined) {
            if (this === undefined || this === null) {
                platform.raiseThis_NullOrUndefined("String.prototype.localeCompare");
            }

            const thisStr = String(this);
            const thatStr = String(that);

            // Performance optimization to cache the state object and UCollator when the default arguments are provided
            // TODO(jahorto): investigate caching when locales and/or options are provided
            let stateObject;
            if (locales === undefined && options === undefined) {
                if (localeCompareStateCache === undefined) {
                    localeCompareStateCache = _.create();
                    InitializeCollator(localeCompareStateCache, undefined, undefined);
                }

                stateObject = localeCompareStateCache;
            } else {
                stateObject = _.create();
                InitializeCollator(stateObject, locales, options);
            }

            return platform.localeCompare(thisStr, thatStr, stateObject, /* forStringPrototypeLocaleCompare */ true);
        }), platform.BuiltInFunctionID.StringLocaleCompare);

        // If we were only initializing Intl for String.prototype, don't initialize Intl.Collator
        if (InitType === "String") {
            return;
        }

        const CollatorPrototype = {};

        const Collator = tagPublicFunction("Intl.Collator", function (locales = undefined, options = undefined) {
            const newTarget = new.target === undefined ? Collator : new.target;
            const collator = OrdinaryCreateFromConstructor(newTarget, CollatorPrototype);

            // Use the hidden object to store data
            let hiddenObject = platform.getHiddenObject(collator);
            if (hiddenObject === undefined) {
                hiddenObject = _.create();
                platform.setHiddenObject(collator, hiddenObject);
            }

            InitializeCollator(hiddenObject, locales, options);

            // Add the bound compare
            hiddenObject.boundCompare = _.bind(compare, collator);
            delete hiddenObject.boundCompare.name;
            return collator;
        });

        const compare = createPublicMethod("Intl.Collator.prototype.compare", function compare(x, y) {
            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
            }

            const hiddenObject = platform.getHiddenObject(this);
            if (hiddenObject === undefined || !hiddenObject.initializedCollator) {
                platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
            }

            return platform.localeCompare(String(x), String(y), hiddenObject, /* forStringPrototypeLocaleCompare */ false);
        });

        const supportedLocalesOf = createPublicMethod("Intl.Collator.supportedLocalesOf", function supportedLocalesOf(locales, options = undefined) {
            return SupportedLocales(platform.isCollatorLocaleAvailable, CanonicalizeLocaleList(locales), options);
        });
        _.defineProperty(Collator, "supportedLocalesOf", {
            value: supportedLocalesOf,
            writable: true,
            enumerable: false,
            configurable: true,
        });

        _.defineProperty(Collator, "prototype", {
            value: CollatorPrototype,
            writable: false,
            enumerable: false,
            configurable: false
        });

        _.defineProperty(CollatorPrototype, "constructor", {
            value: Collator,
            writable: true,
            enumerable: false,
            configurable: true
        });
        _.defineProperty(CollatorPrototype, "resolvedOptions", {
            value: createPublicMethod("Intl.Collator.prototype.resolvedOptions", function resolvedOptions() {
                if (typeof this !== "object") {
                    platform.raiseNeedObjectOfType("Collator.prototype.resolvedOptions", "Collator");
                }
                const hiddenObject = platform.getHiddenObject(this);
                if (hiddenObject === undefined || !hiddenObject.initializedCollator) {
                    platform.raiseNeedObjectOfType("Collator.prototype.resolvedOptions", "Collator");
                }

                const options = [
                    "locale",
                    "usage",
                    "sensitivity",
                    "ignorePunctuation",
                    "collation",
                    "numeric",
                    "caseFirst",
                ];

                return createResolvedOptions(options, hiddenObject);
            }),
            writable: true,
            enumerable: false,
            configurable: true
        });

        // test262's test\intl402\Collator\prototype\compare\name.js checks the name of the descriptor's getter function
        const getCompare = createPublicMethod("get compare", function () {
            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
            }

            const hiddenObject = platform.getHiddenObject(this);
            if (hiddenObject === undefined || !hiddenObject.initializedCollator) {
                platform.raiseNeedObjectOfType("Collator.prototype.compare", "Collator");
            }

            return hiddenObject.boundCompare;
        });
        _.defineProperty(CollatorPrototype, "compare", {
            get: getCompare,
            enumerable: false,
            configurable: true
        });

        return Collator;
    })();

    // Intl.NumberFormat, Number.prototype.toLocaleString
    var NumberFormat = (function () {
        if (InitType !== "Intl" && InitType !== "Number") {
            return;
        }

        const InitializeNumberFormat = function (nf, locales, options) {
            const requestedLocales = CanonicalizeLocaleList(locales);
            options = options === undefined ? _.create() : Internal.ToObject(options);

            const opt = _.create();
            opt.localeMatcher = GetOption(options, "localeMatcher", "string", ["best fit", "lookup"], "best fit");

            const r = ResolveLocale(platform.isNFLocaleAvailable, requestedLocales, opt, ["nu"]);
            nf.locale = r.locale;
            nf.numberingSystem = r.nu;

            const style = GetOption(options, "style", "string", ["decimal", "percent", "currency"], "decimal");
            nf.style = style;
            nf.formatterToUse = platform.NumberFormatStyle[style];
            const useCurrency = style === "currency";

            let currency = GetOption(options, "currency", "string", undefined, undefined);
            if (currency !== undefined && !IsWellFormedCurrencyCode(currency)) {
                platform.raiseInvalidCurrencyCode(currency);
            } else if (currency === undefined && useCurrency) {
                platform.raiseMissingCurrencyCode();
            }

            let cDigits = 0;
            if (useCurrency) {
                currency = _.toUpperCase(currency);
                nf.currency = currency;
                cDigits = platform.currencyDigits(currency);
            }

            let currencyDisplay = GetOption(options, "currencyDisplay", "string", ["code", "symbol", "name"], "symbol");
            if (useCurrency) {
                nf.currencyDisplay = currencyDisplay;
                nf.currencyDisplayToUse = platform.NumberFormatCurrencyDisplay[currencyDisplay];
            }

            let mnfdDefault, mxfdDefault;
            if (useCurrency) {
                mnfdDefault = cDigits;
                mxfdDefault = cDigits;
            } else {
                mnfdDefault = 0;
                if (style === "percent") {
                    mxfdDefault = 0;
                } else {
                    mxfdDefault = 3;
                }
            }

            SetNumberFormatDigitOptions(nf, options, mnfdDefault, mxfdDefault);

            nf.useGrouping = GetOption(options, "useGrouping", "boolean", undefined, true);

            nf.initializedNumberFormat = true;

            // Cache api instance and update numbering system on the object
            platform.cacheNumberFormat(nf);

            return nf;
        };

        platform.registerBuiltInFunction(createPublicMethod("Number.prototype.toLocaleString", function toLocaleString() {
            if (typeof this !== "number" && !(this instanceof Number)) {
                platform.raiseNeedObjectOfType("Number.prototype.toLocaleString", "Number");
            }

            const stateObject = _.create();
            InitializeNumberFormat(stateObject, arguments[0], arguments[1]);

            const n = Internal.ToNumber(this);
            return platform.formatNumber(n, stateObject, /* toParts */ false, /* forNumberPrototypeToLocaleString */ true);
        }), platform.BuiltInFunctionID.NumberToLocaleString);

        if (InitType === "Number") {
            return;
        }

        const NumberFormatPrototype = {};

        const NumberFormat = tagPublicFunction("Intl.NumberFormat", function NumberFormat(locales = undefined, options = undefined) {
            const newTarget = new.target === undefined ? NumberFormat : new.target;
            const numberFormat = OrdinaryCreateFromConstructor(newTarget, NumberFormatPrototype);

            let hiddenObject = platform.getHiddenObject(numberFormat);
            if (hiddenObject === undefined) {
                hiddenObject = _.create();
                platform.setHiddenObject(numberFormat, hiddenObject);
            }

            InitializeNumberFormat(hiddenObject, locales, options);

            if (new.target === undefined && this instanceof NumberFormat) {
                _.defineProperty(this, platform.FallbackSymbol, {
                    value: numberFormat,
                    writable: false,
                    enumerable: false,
                    configurable: false
                });

                return this;
            }

            return numberFormat;
        });

        // format should always be bound to a valid NumberFormat's hiddenObject by getFormat()
        const format = createPublicMethod("Intl.NumberFormat.prototype.format", function format(n) {
            n = Internal.ToNumber(n);

            if (!this || !this.initializedNumberFormat) {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
            }

            return platform.formatNumber(n, this, /* toParts */ false, /* forNumberPrototypeToLocaleString */ false);
        });

        const formatToParts = createPublicMethod("Intl.NumberFormat.prototype.formatToParts", function formatToParts(n) {
            n = Internal.ToNumber(n);

            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.formatToParts", "NumberFormat");
            }

            const hiddenObject = platform.getHiddenObject(this);
            if (hiddenObject === undefined || !hiddenObject.initializedNumberFormat) {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.formatToParts", "NumberFormat");
            }

            return platform.formatNumber(n, hiddenObject, /* toParts */ true, /* forNumberPrototypeToLocaleString */ false);
        });

        const supportedLocalesOf = createPublicMethod("Intl.NumberFormat.supportedLocalesOf", function supportedLocalesOf(locales, options = undefined) {
            return SupportedLocales(platform.isNFLocaleAvailable, CanonicalizeLocaleList(locales), options);
        });
        _.defineProperty(NumberFormat, "supportedLocalesOf", {
            value: supportedLocalesOf,
            writable: true,
            enumerable: false,
            configurable: true,
        });

        _.defineProperty(NumberFormat, "prototype", {
            value: NumberFormatPrototype,
            writable: false,
            enumerable: false,
            configurable: false
        });

        _.defineProperty(NumberFormatPrototype, "constructor", {
            value: NumberFormat,
            writable: true,
            enumerable: false,
            configurable: true,
        });

        const UnwrapNumberFormat = function (nf) {
            let hiddenObject = platform.getHiddenObject(nf);
            if ((!hiddenObject || !hiddenObject.initializedNumberFormat) && nf instanceof NumberFormat) {
                nf = nf[platform.FallbackSymbol];
            }

            if (typeof nf !== "object") {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
            }

            hiddenObject = platform.getHiddenObject(nf);
            if (!hiddenObject.initializedNumberFormat) {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
            }

            return hiddenObject;
        };

        _.defineProperty(NumberFormatPrototype, "resolvedOptions", {
            value: createPublicMethod("Intl.NumberFormat.prototype.resolvedOptions", function resolvedOptions() {
                if (typeof this !== "object") {
                    platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
                }

                const hiddenObject = UnwrapNumberFormat(this);

                const options = ["locale", "numberingSystem", "style", "currency", "currencyDisplay", "minimumIntegerDigits",
                    "minimumFractionDigits", "maximumFractionDigits", "minimumSignificantDigits", "maximumSignificantDigits",
                    "useGrouping"];

                return createResolvedOptions(options, hiddenObject);
            }),
            writable: true,
            enumerable: false,
            configurable: true,
        });

        // test262's test\intl402\NumberFormat\prototype\format\name.js checks the name of the descriptor's getter function
        const getFormat = createPublicMethod("get format", function () {
            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("NumberFormat.prototype.format", "NumberFormat");
            }

            const hiddenObject = UnwrapNumberFormat(this);

            if (hiddenObject.boundFormat === undefined) {
                hiddenObject.boundFormat = _.bind(format, hiddenObject);
                delete hiddenObject.boundFormat.name;
            }

            return hiddenObject.boundFormat;
        });
        _.defineProperty(getFormat, "name", {
            value: "get format",
            writable: false,
            enumerable: false,
            configurable: true,
        });
        _.defineProperty(NumberFormatPrototype, "format", {
            get: getFormat,
            enumerable: false,
            configurable: true,
        });

        _.defineProperty(NumberFormatPrototype, "formatToParts", {
            value: formatToParts,
            enumerable: false,
            configurable: true,
            writable: true,
        });

        return NumberFormat;
    })();

    // Intl.DateTimeFormat, Date.prototype.toLocaleString, Date.prototype.toLocaleDateString, Date.prototype.toLocaleTimeString
    var DateTimeFormat = (function () {
        if (InitType !== "Intl" && InitType !== "Date") {
            return;
        }

        const narrowShortLong = ["narrow", "short", "long"];
        const twoDigitNumeric = ["2-digit", "numeric"];
        const allOptionValues = _.concat(twoDigitNumeric, narrowShortLong);
        const dateTimeComponents = [
            ["weekday", narrowShortLong],
            ["era", narrowShortLong],
            ["year", twoDigitNumeric],
            ["month", allOptionValues], // month has every option available to it
            ["day", twoDigitNumeric],
            ["hour", twoDigitNumeric],
            ["minute", twoDigitNumeric],
            ["second", twoDigitNumeric],
            ["timeZoneName", _.slice(narrowShortLong, 1)] // timeZoneName only allows "short" and "long"
        ];

        /**
         * Given a user-provided options object, getPatternForOptions generates a LDML/ICU pattern and then
         * sets the pattern and all of the relevant options implemented by the pattern on the provided dtf before returning.
         *
         * @param {Object} dtf the DateTimeFormat internal object
         * @param {Object} options the options object originally given by the user
         */
        const getPatternForOptions = (function () {
            // symbols come from the Unicode LDML: http://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
            const symbolForOption = {
                weekday: "E",
                era: "G",
                year: "y",
                month: "M",
                day: "d",
                // for hour, we have some special handling
                hour: "j", hour12: "h", hour24: "H",
                minute: "m",
                second: "s",
                timeZoneName: "z",
            };
            // NOTE - keep this up to date with the map in PlatformAgnostic::Intl::GetDateTimePartKind and the UDateFormatField enum
            const optionForSymbol = {
                E: "weekday", c: "weekday", e: "weekday",
                G: "era",
                y: "year", u: "year", U: "year",
                M: "month", L: "month",
                d: "day",
                h: "hour", H: "hour", K: "hour", k: "hour",
                m: "minute",
                s: "second",
                z: "timeZoneName", Z: "timeZoneName", v: "timeZoneName", V: "timeZoneName", O: "timeZoneName", X: "timeZoneName", x: "timeZoneName",
            };

            // lengths here are how many times a symbol is repeated in a skeleton for a given option
            // the Intl spec recommends that Intl "short" -> CLDR "abbreviated" and Intl "long" -> CLDR "wide"
            const symbolLengthForOption = {
                numeric: 1,
                "2-digit": 2,
                short: 3,
                long: 4,
                narrow: 5,
            };
            const optionForSymbolLength = {
                1: "numeric",
                2: "2-digit",
                3: "short",
                4: "long",
                5: "narrow",
            };

            // for fixing up the hour pattern later
            const patternForHourCycle = {
                h12: "h",
                h23: "H",
                h11: "K",
                h24: "k",
            };
            const hourCycleForPattern = {
                h: "h12",
                H: "h23",
                K: "h11",
                k: "h24",
            };

            // take the hour12 option by name so that we dont call the getter for options.hour12 twice
            return function (dtf, options, hour12) {
                const resolvedOptions = _.reduce(dateTimeComponents, function (resolved, component) {
                    const prop = component[0];
                    const value = GetOption(options, prop, "string", component[1], undefined);
                    if (value !== undefined) {
                        resolved[prop] = value;
                    }

                    return resolved;
                }, _.create());

                const hc = dtf.hourCycle;

                // Build up a skeleton by repeating skeleton keys (like "G", "y", etc) for a count corresponding to the intl option value.
                const skeleton = _.reduce(_.keys(resolvedOptions), function (skeleton, optionKey) {
                    let optionValue = resolvedOptions[optionKey];
                    if (optionKey === "hour") {
                        // hour12/hourCycle resolution in the spec has multiple issues:
                        // hourCycle and -hc can be out of sync: https://github.com/tc39/ecma402/issues/195
                        // hour12 has precedence over a more specific option in hourCycle/hc
                        // hour12 can force a locale that prefers h23 and h12 to use h11 or h24, according to the spec
                        // We temporarily work around these similarly to firefox and implement custom hourCycle/hour12 resolution.
                        // TODO(jahorto): follow up with Intl spec about these issues
                        if (hour12 === true || (hour12 === undefined && (hc === "h11" || hc === "h12"))) {
                            optionKey = "hour12";
                        } else if (hour12 === false || (hour12 === undefined && (hc === "h23" || hc === "h24"))) {
                            optionKey = "hour24";
                        }
                    }

                    return skeleton + _.repeat(symbolForOption[optionKey], symbolLengthForOption[optionValue]);
                }, "");

                let pattern = platform.getPatternForSkeleton(dtf.locale, skeleton);

                // getPatternForSkeleton (udatpg_getBestPattern) can ignore, add, and modify fields compared to the markers we gave in the skeleton.
                // Most importantly, udatpg_getBestPattern will determine the most-preferred hour field for a locale and time type (12 or 24).
                // Scan the generated pattern to extract the resolved fields, and fix up the hour field if the user requested an explicit hour cycle
                let inLiteral = false;
                let i = 0;
                while (i < pattern.length) {
                    let cur = pattern[i];
                    const isQuote = cur === "'";
                    if (inLiteral) {
                        if (isQuote) {
                            inLiteral = false;
                        }
                        ++i;
                        continue;
                    } else if (isQuote) {
                        inLiteral = true;
                        ++i;
                        continue;
                    } else if (cur === " ") {
                        ++i;
                        continue;
                    }

                    // we are not in a format literal, so we are in a symbolic section of the pattern
                    // now, we can force the correct hour pattern and set the internal slots correctly
                    if (cur === "h" || cur === "H" || cur === "K" || cur === "k") {
                        if (hc && hour12 === undefined) {
                            // if we have found an hour-like symbol and the user wanted a specific hour cycle,
                            // replace it and all such proceding contiguous symbols with the symbol corresponding
                            // to the user-requested hour cycle, if they are different
                            const replacement = patternForHourCycle[hc];
                            if (replacement !== cur) {
                                if (pattern[i + 1] === cur) {
                                    // 2-digit hour
                                    pattern = _.substring(pattern, 0, i) + replacement + replacement + _.substring(pattern, i + 2);
                                } else {
                                    // numeric hour
                                    pattern = _.substring(pattern, 0, i) + replacement + _.substring(pattern, i + 1);
                                }

                                // we have modified pattern[i] so we need to update cur
                                cur = pattern[i];
                            }
                        } else {
                            // if we have found an hour-like symbol and the user didnt request an hour cycle,
                            // set the internal hourCycle property from the resolved pattern
                            dtf.hourCycle = hourCycleForPattern[cur];
                        }
                    }

                    let k = i + 1;
                    while (k < pattern.length && pattern[k] === cur) {
                        ++k;
                    }

                    const resolvedKey = optionForSymbol[cur];
                    const resolvedValue = optionForSymbolLength[k - i];
                    dtf[resolvedKey] = resolvedValue;
                    i = k;
                }

                dtf.pattern = pattern;
            };
        })();

        /**
         * Initializes the dateTimeFormat argument with the given locales and options.
         *
         * ECMA-402: #sec-initializedatetimeformat
         *
         * @param {Object} dateTimeFormat the state object representing a DateTimeFormat instance or toLocale*String call
         * @param {String|String[]} locales a user-provided list of locales
         * @param {Object} options a user-provided options object
         */
        const InitializeDateTimeFormat = function (dateTimeFormat, locales, options) {
            const requestedLocales = CanonicalizeLocaleList(locales);
            options = ToDateTimeOptions(options, "any", "date");

            const opt = _.create();
            opt.localeMatcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");

            // Providing undefined for the `values` argument allows { hour12: "asd" } to become hour12 = true,
            // which is apparently a feature of the spec, rather than a bug.
            const hour12 = GetOption(options, "hour12", "boolean", undefined, undefined);
            let hourCycle = GetOption(options, "hourCycle", "string", ["h11", "h12", "h23", "h24"], undefined);
            if (hour12 !== undefined) {
                hourCycle = null;
            }
            opt.hc = hourCycle;

            const r = ResolveLocale(platform.isDTFLocaleAvailable, requestedLocales, opt, ["nu", "ca", "hc"]);
            dateTimeFormat.locale = r.locale;
            dateTimeFormat.calendar = r.ca;
            dateTimeFormat.hourCycle = r.hc;
            dateTimeFormat.numberingSystem = r.nu;

            const localeWithoutSubtags = r.dataLocale;
            let tz = options.timeZone;
            if (tz === undefined) {
                tz = platform.getDefaultTimeZone();
            } else {
                tz = Internal.ToString(tz);
            }

            // make tz uppercase here, as its easier to do now than in platform (even though the uppercase operation
            // is supposed to be done in #sec-isvalidtimezonename)
            const canonicalTZ = platform.validateAndCanonicalizeTimeZone(tz);
            if (canonicalTZ === undefined || canonicalTZ === "Etc/Unknown") {
                platform.raiseOptionValueOutOfRange_3(tz, "timeZone", "IANA Zone or Link name (Area/Location)");
            } else if (canonicalTZ === "Etc/UTC" || canonicalTZ === "Etc/GMT") {
                tz = "UTC";
            } else {
                tz = canonicalTZ;
            }

            dateTimeFormat.timeZone = tz;

            // get the formatMatcher for validation only
            GetOption(options, "formatMatcher", "string", ["basic", "best fit"], "best fit");

            // this call replaces most of the spec code related to hour12/hourCycle and format negotiation/handling
            getPatternForOptions(dateTimeFormat, options, hour12);
            dateTimeFormat.initializedDateTimeFormat = true;

            return dateTimeFormat;
        };

        /**
         * Modifies the options argument to have correct default values
         *
         * ECMA-402: #sec-todatetimeoptions
         *
         * @param {Object} options user-provided options object passed as second argument to Intl.DateTimeFormat/toLocale*String
         * @param {String} required which kind of options must be provided for the call (one of "date", "time", or "any")
         * @param {String} defaults which kind of options will be set to a default value (one of "date", "time", or "all")
         * @returns {Object} modified options object
         */
        const ToDateTimeOptions = function (options, required, defaults) {
            options = options === undefined ? null : Internal.ToObject(options);
            options = _.create(options);
            let needDefaults = true;
            if (required === "date" || required === "any") {
                _.forEach(["weekday", "year", "month", "day"], function (prop) {
                    const value = options[prop];
                    if (value !== undefined) {
                        needDefaults = false;
                    }
                });
            }

            if (required === "time" || required === "any") {
                _.forEach(["hour", "minute", "second"], function (prop) {
                    const value = options[prop];
                    if (value !== undefined) {
                        needDefaults = false;
                    }
                });
            }

            if (needDefaults === true && (defaults === "date" || defaults === "all")) {
                _.forEach(["year", "month", "day"], function (prop) {
                    _.defineProperty(options, prop, {
                        value: "numeric",
                        writable: true,
                        enumerable: true,
                        configurable: true,
                    });
                })
            }

            if (needDefaults === true && (defaults === "time" || defaults === "all")) {
                _.forEach(["hour", "minute", "second"], function (prop) {
                    _.defineProperty(options, prop, {
                        value: "numeric",
                        writable: true,
                        enumerable: true,
                        configurable: true,
                    });
                })
            }

            return options;
        };

        const FormatDateTimeToParts = function (dtf, x) {
            if (_.isNaN(x) || !_.isFinite(x)) {
                platform.raiseInvalidDate();
            }

            return platform.formatDateTime(dtf, x, /* toParts */ true, /* forDatePrototypeToLocaleString */ false);
        };

        // caches for objects constructed with default parameters for each method
        const __DateInstanceToLocaleStringDefaultCache = [undefined, undefined, undefined];
        const __DateInstanceToLocaleStringDefaultCacheSlot = bare({
            toLocaleString: 0,
            toLocaleDateString: 1,
            toLocaleTimeString: 2
        });

        function DateInstanceToLocaleStringImplementation(name, option1, option2, cacheSlot, locales, options) {
            if (typeof this !== 'object' || !(this instanceof Date)) {
                platform.raiseNeedObjectOfType(name, "Date");
            }
            const value = _.getDate(new Date(this));
            if (_.isNaN(value) || !_.isFinite(value)) {
                return "Invalid Date";
            }

            let stateObject = undefined;
            if (platform.useCaches && locales === undefined && options === undefined) {
                // All default parameters (locales and options): this is the most valuable case to cache.
                if (__DateInstanceToLocaleStringDefaultCache[cacheSlot]) {
                    // retrieve cached value
                    stateObject = __DateInstanceToLocaleStringDefaultCache[cacheSlot];
                } else {
                    // populate cache
                    stateObject = _.create();
                    InitializeDateTimeFormat(stateObject, undefined, ToDateTimeOptions(undefined, option1, option2));
                    __DateInstanceToLocaleStringDefaultCache[cacheSlot] = stateObject;
                }
            }

            if (!stateObject) {
                stateObject = _.create();
                InitializeDateTimeFormat(stateObject, locales, ToDateTimeOptions(options, option1, option2));
            }

            return platform.formatDateTime(stateObject, Internal.ToNumber(this), /* toParts */ false, /* forDatePrototypeToLocaleString */ true);
        }

        // Note: createPublicMethod (platform.tagPublicLibraryCode) messes with declared name of the FunctionBody so that
        // the functions called appear correctly in the debugger and stack traces. Thus, we we cannot call createPublicMethod in a loop.
        // Each entry point needs to have its own unique FunctionBody (which is a function as defined in the source code);
        // this is why we have seemingly repeated ourselves below, instead of having one function and calling it multiple times with
        // different parameters.
        //
        // The following invocations of `platform.registerBuiltInFunction(createPublicMethod(name, entryPoint))` are enclosed in IIFEs.
        // The IIFEs are used to group all of the meaningful differences between each entry point into the arguments to the IIFE.
        // The exception to this are the different entryPoint names which are only significant for debugging (and cannot be passed in
        // as arguments, as the name is intrinsic to the function declaration).
        //
        // The `date_toLocale*String_entryPoint` function names are placeholder names that will never be seen from user code.
        // The function name property and FunctionBody declared name are overwritten by `createPublicMethod`.
        // The fact that they are declared with unique names is helpful for debugging.
        // The functions *must not* be declared as anonymous functions (must be declared with a name);
        // converting from an unnnamed function to a named function is not readily supported by the platform code and
        // this has caused us to hit assertions in debug builds in the past.
        //
        // The entryPoint functions will be called as `Date.prototype.toLocale*String` and thus their `this` parameters will be a Date.
        // `DateInstanceToLocaleStringImplementation` is not on `Date.prototype`, so we must propagate `this` into the call by using
        // `DateInstanceToLocaleStringImplementation.call(this, ...)`.

        (function (name, option1, option2, cacheSlot, platformFunctionID) {
            platform.registerBuiltInFunction(createPublicMethod(name, function date_toLocaleString_entryPoint(locales = undefined, options = undefined) {
                return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
            }), platformFunctionID);
        })("Date.prototype.toLocaleString", "any", "all", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleString, platform.BuiltInFunctionID.DateToLocaleString);

        (function (name, option1, option2, cacheSlot, platformFunctionID) {
            platform.registerBuiltInFunction(createPublicMethod(name, function date_toLocaleDateString_entryPoint(locales = undefined, options = undefined) {
                return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
            }), platformFunctionID);
        })("Date.prototype.toLocaleDateString", "date", "date", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleDateString, platform.BuiltInFunctionID.DateToLocaleDateString);

        (function (name, option1, option2, cacheSlot, platformFunctionID) {
            platform.registerBuiltInFunction(createPublicMethod(name, function date_toLocaleTimeString_entryPoint(locales = undefined, options = undefined) {
                return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
            }), platformFunctionID);
        })("Date.prototype.toLocaleTimeString", "time", "time", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleTimeString, platform.BuiltInFunctionID.DateToLocaleTimeString);

        // if we were only initializing Date, dont bother initializing Intl.DateTimeFormat
        if (InitType !== "Intl") {
            return;
        }

        const DateTimeFormatPrototype = {};

        /**
         * The Intl.DateTimeFormat constructor
         *
         * ECMA-402: #sec-intl.datetimeformat
         *
         * @param {String|String[]} locales
         * @param {Object} options
         */
        const DateTimeFormat = tagPublicFunction("Intl.DateTimeFormat", function DateTimeFormat(locales = undefined, options = undefined) {
            const newTarget = new.target === undefined ? DateTimeFormat : new.target;
            const dateTimeFormat = OrdinaryCreateFromConstructor(newTarget, DateTimeFormatPrototype);

            let hiddenObject = platform.getHiddenObject(dateTimeFormat);
            if (hiddenObject === undefined) {
                hiddenObject = _.create();
                platform.setHiddenObject(dateTimeFormat, hiddenObject);
            }

            InitializeDateTimeFormat(hiddenObject, locales, options);

            if (new.target === undefined && this instanceof DateTimeFormat) {
                _.defineProperty(this, platform.FallbackSymbol, {
                    value: dateTimeFormat,
                    writable: false,
                    enumerable: false,
                    configurable: false
                });

                return this;
            }

            return dateTimeFormat;
        });

        const UnwrapDateTimeFormat = function (dtf) {
            let hiddenObject = platform.getHiddenObject(dtf);
            if ((!hiddenObject || !hiddenObject.initializedDateTimeFormat) && dtf instanceof DateTimeFormat) {
                dtf = dtf[platform.FallbackSymbol];
            }

            if (typeof dtf !== "object") {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
            }

            hiddenObject = platform.getHiddenObject(dtf);
            if (!hiddenObject.initializedDateTimeFormat) {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
            }

            return hiddenObject;
        };

        // format should always be bound to a valid DateTimeFormat's hiddenObject by getFormat()
        const format = createPublicMethod("Intl.DateTimeFormat.prototype.format", function format(date) {
            if (!this || !this.initializedDateTimeFormat) {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
            }

            let x;
            if (date === undefined) {
                x = platform.builtInJavascriptDateEntryNow();
            } else {
                x = Internal.ToNumber(date);

                if (_.isNaN(x) || !_.isFinite(x)) {
                    platform.raiseInvalidDate();
                }
            }

            return platform.formatDateTime(this, x, /* toParts */ false, /* forDatePrototypeToLocaleString */ false);
        });

        const formatToParts = createPublicMethod("Intl.DateTimeFormat.prototype.formatToParts", function formatToParts(date) {
            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.formatToParts", "DateTimeFormat");
            }

            const hiddenObject = platform.getHiddenObject(this);
            if (hiddenObject === undefined || !hiddenObject.initializedDateTimeFormat) {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.formatToParts", "DateTimeFormat");
            }

            let x;
            if (date === undefined) {
                x = platform.builtInJavascriptDateEntryNow();
            } else {
                x = Internal.ToNumber(date);

                if (_.isNaN(x) || !_.isFinite(x)) {
                    platform.raiseInvalidDate();
                }
            }

            return platform.formatDateTime(hiddenObject, x, /* toParts */ true, /* forDatePrototypeToLocaleString */ false);
        });

        _.defineProperty(DateTimeFormat, "prototype", {
            value: DateTimeFormatPrototype,
            writable: false,
            enumerable: false,
            configurable: false
        });

        _.defineProperty(DateTimeFormatPrototype, "constructor", {
            value: DateTimeFormat,
            writable: true,
            enumerable: false,
            configurable: true
        });

        // test262's test\intl402\DateTimeFormat\prototype\format\name.js checks the name of the descriptor's getter function
        const getFormat = createPublicMethod("get format", function () {
            if (typeof this !== "object") {
                platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
            }

            const hiddenObject = UnwrapDateTimeFormat(this);

            if (hiddenObject.boundFormat === undefined) {
                hiddenObject.boundFormat = _.bind(format, hiddenObject);
                delete hiddenObject.boundFormat.name;
            }

            return hiddenObject.boundFormat;
        });
        _.defineProperty(getFormat, "name", {
            value: "get format",
            writable: false,
            enumerable: false,
            configurable: true,
        });
        _.defineProperty(DateTimeFormatPrototype, "format", {
            get: getFormat,
            enumerable: false,
            configurable: true,
        });
        _.defineProperty(DateTimeFormatPrototype, "formatToParts", {
            value: formatToParts,
            enumerable: false,
            configurable: true,
            writable: true,
        });
        _.defineProperty(DateTimeFormatPrototype, "resolvedOptions", {
            value: createPublicMethod("Intl.DateTimeFormat.prototype.resolvedOptions", function resolvedOptions() {
                if (typeof this !== "object") {
                    platform.raiseNeedObjectOfType("DateTimeFormat.prototype.format", "DateTimeFormat");
                }

                const hiddenObject = UnwrapDateTimeFormat(this);
                const options = [
                    "locale",
                    "calendar",
                    "numberingSystem",
                    "timeZone",
                    "hourCycle",
                    "weekday",
                    "era",
                    "year",
                    "month",
                    "day",
                    "hour",
                    "minute",
                    "second",
                    "timeZoneName",
                ];

                return createResolvedOptions(options, hiddenObject, function (prop, resolved) {
                    if (prop === "hourCycle") {
                        const hc = hiddenObject.hourCycle;
                        if (hiddenObject.hour !== undefined && hc !== null) {
                            resolved.hourCycle = hc;
                            resolved.hour12 = hc === "h11" || hc === "h12";
                        }

                        return true;
                    }
                });
            }),
            writable: true,
            enumerable: false,
            configurable: true,
        });

        const supportedLocalesOf = createPublicMethod("Intl.DateTimeFormat.supportedLocalesOf", function supportedLocalesOf(locales, options = undefined) {
            return SupportedLocales(platform.isDTFLocaleAvailable, CanonicalizeLocaleList(locales), options);
        });
        _.defineProperty(DateTimeFormat, "supportedLocalesOf", {
            value: supportedLocalesOf,
            writable: true,
            enumerable: false,
            configurable: true,
        });

        return DateTimeFormat;
    })();

    const PluralRules = (function() {
        if (InitType !== "Intl") {
            return;
        }

        /**
         * Initializes the given pluralRules object
         *
         * ECMA 402: #sec-initializepluralrules
         *
         * @param {Object} pluralRules
         * @param {String|String[]} locales
         * @param {Object} options
         */
        const InitializePluralRules = function (pluralRules, locales, options) {
            const requestedLocales = CanonicalizeLocaleList(locales);
            options = options === undefined ? _.create() : Internal.ToObject(options);
            const opt = _.create();
            opt.matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
            pluralRules.type = GetOption(options, "type", "string", ["cardinal", "ordinal"], "cardinal");

            SetNumberFormatDigitOptions(pluralRules, options, 0, 3);

            // %PluralRules%.[[RelevantExtensionKeys]] = [] (#sec-intl.pluralrules-internal-slots)
            const r = ResolveLocale(platform.isPRLocaleAvailable, requestedLocales, opt, []);

            pluralRules.locale = r.locale;
            pluralRules.pluralCategories = platform.pluralRulesKeywords(pluralRules);

            pluralRules.initializedPluralRules = true;

            return pluralRules;
        };

        /**
         * Returns a String value representing the plural form of n according to
         * the effective locale and the options of pluralRules
         *
         * ECMA 402: #sec-resolveplural
         *
         * @param {Object} pluralRules
         * @param {Number} n
         */
        const ResolvePlural = function (pluralRules, n) {
            if (!_.isFinite(n)) {
                return "other";
            }

            return platform.pluralRulesSelect(pluralRules, n);
        };

        const PluralRulesPrototype = {};

        // params are explicitly `= undefined` to make PluralRules.length === 0
        const PluralRules = tagPublicFunction("Intl.PluralRules", function PluralRules(locales = undefined, options = undefined) {
            if (new.target === undefined) {
                platform.raiseNeedObjectOfType("Intl.PluralRules", "PluralRules");
            }

            const pluralRules = OrdinaryCreateFromConstructor(new.target, PluralRulesPrototype);

            const stateObject = _.create();
            platform.setHiddenObject(pluralRules, stateObject);

            InitializePluralRules(stateObject, locales, options);

            return pluralRules;
        });

        // ECMA 402: #sec-intl.pluralrules.prototype
        _.defineProperty(PluralRules, "prototype", {
            value: PluralRulesPrototype,
            writable: false,
            enumerable: false,
            configurable: false,
        });

        _.defineProperty(PluralRulesPrototype, "constructor", {
            value: PluralRules,
            writable: true,
            enumerable: false,
            configurable: true
        });

        const supportedLocalesOf = createPublicMethod("Intl.PluralRules.supportedLocalesOf", function supportedLocalesOf(locales, options = undefined) {
            return SupportedLocales(platform.isPRLocaleAvailable, CanonicalizeLocaleList(locales), options);
        });
        _.defineProperty(PluralRules, "supportedLocalesOf", {
            value: supportedLocalesOf,
            writable: true,
            enumerable: false,
            configurable: true,
        });

        // ECMA 402: #sec-intl.pluralrules.prototype.select
        const select = createPublicMethod("Intl.PluralRules.prototype.select", function select(value) {
            const pr = platform.getHiddenObject(this);
            if (!pr || !pr.initializedPluralRules) {
                platform.raiseNeedObjectOfType("Intl.PluralRules.prototype.select", "PluralRules");
            }

            const n = Internal.ToNumber(value);
            return ResolvePlural(pr, n);
        });
        _.defineProperty(PluralRulesPrototype, "select", {
            value: select,
            enumerable: false,
            configurable: true,
            writable: true,
        });

        const resolvedOptions = createPublicMethod("Intl.PluralRules.prototype.resolvedOptions", function resolvedOptions() {
            const pr = platform.getHiddenObject(this);
            if (!pr || !pr.initializedPluralRules) {
                platform.raiseNeedObjectOfType("Intl.PluralRules.prototype.select", "PluralRules");
            }

            return createResolvedOptions([
                "locale",
                "type",
                "minimumIntegerDigits",
                "minimumFractionDigits",
                "maximumFractionDigits",
                "minimumSignificantDigits",
                "maximumSignificantDigits",
                "pluralCategories"
            ], pr, (prop, resolved) => {
                if (prop === "pluralCategories") {
                    // https://github.com/tc39/ecma402/issues/224: create a copy of the pluralCategories array
                    resolved.pluralCategories = _.slice(pr.pluralCategories, 0);
                    return true;
                }
            });
        });
        _.defineProperty(PluralRulesPrototype, "resolvedOptions", {
            value: resolvedOptions,
            enumerable: false,
            configurable: true,
            writable: true,
        });

        return PluralRules;
    })();

    // Initialize Intl properties only if needed
    if (InitType === "Intl") {
        _.defineProperty(Intl, "Collator",              { value: Collator,              writable: true, enumerable: false, configurable: true });
        _.defineProperty(Intl, "NumberFormat",          { value: NumberFormat,          writable: true, enumerable: false, configurable: true });
        _.defineProperty(Intl, "DateTimeFormat",        { value: DateTimeFormat,        writable: true, enumerable: false, configurable: true });
        _.defineProperty(Intl, "PluralRules",           { value: PluralRules,           writable: true, enumerable: false, configurable: true });
    }

    }
    /**
     *
     *
     *
     *
     *
     *
     * END ICU, BEGIN WINGLOB
     *
     *
     *
     *
     *
     *
     */
    else {

    if (platform.localeLookupCache === undefined) {
        platform.localeLookupCache = new platform.Map();
    }
    if (platform.localeBestFitCache === undefined) {
        platform.localeBestFitCache = new platform.Map();
    }

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

    let IsWellFormedLanguageTag = function (langTag) {
        let retVal = platform.isWellFormedLanguageTag(langTag);
        if (retVal === null) {
            if (!LANG_TAG_RE) {
                InitializeLangTagREs();
            }
            let match = platform.builtInRegexMatch(langTag, LANG_TAG_RE);
            return !!match;
        } else {
            return retVal;
        }
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

    var resolveLocaleBestFit = function (locale, defaultLocale) {
        var resolvedLocale = platform.localeBestFitCache.get(locale);
        if (resolvedLocale === undefined) {
            resolvedLocale = platform.resolveLocaleBestFit(locale);
            if (resolvedLocale === null) {
                if (!LANG_TAG_BASE_RE) {
                    InitializeLangTagREs();
                }
                let match = platform.builtInRegexMatch(locale, LANG_TAG_BASE_RE);
                resolvedLocale = match[1] + (match[2] ? ('-' + match[2]) : '') + (match[3] ? ('-' + match[3]) : '');
            }
            // If resolvedLocale is undefined, cache that we got undefined
            // so we don't try to resolve for `locale` in future.
            platform.localeBestFitCache.set(locale, resolvedLocale === undefined ? NOT_FOUND : resolvedLocale);
        } else if (resolvedLocale === NOT_FOUND) {
            resolvedLocale = undefined;
        }

        if (defaultLocale === locale) {
            return resolvedLocale;
        } else if (defaultLocale === resolvedLocale) {
            return undefined;
        } else {
            return resolvedLocale;
        }
    }

    var resolveLocaleLookup = function (localeWithoutSubtags) {
        let resolvedLocale = platform.localeLookupCache.get(localeWithoutSubtags);
        if (resolvedLocale === undefined) {
            resolvedLocale = platform.resolveLocaleLookup(localeWithoutSubtags);
            if (resolvedLocale === null) {
                if (!LANG_TAG_BASE_RE) {
                    InitializeLangTagREs();
                }
                let match = platform.builtInRegexMatch(localeWithoutSubtags, LANG_TAG_BASE_RE);
                // match: [1] language; [2] script; [3] region (e.g. en-Latn-US)
                resolvedLocale = match[1]
                    + (match[2] ? ('-' + match[2]) : '')
                    + (match[3] ? ('-' + match[3]) : '');
            }
            // If resolvedLocale is undefined, cache that we got undefined
            // so we don't try to resolve for `locale` in future.
            platform.localeLookupCache.set(localeWithoutSubtags, resolvedLocale === undefined ? NOT_FOUND : resolvedLocale);
        } else if (resolvedLocale === NOT_FOUND) {
            resolvedLocale = undefined;
        }
        return resolvedLocale;
    }

    var getExtensionSubtags = function (locale) {
        if (!LANG_TAG_EXT_RE) {
            InitializeLangTagREs();
        }

        const match = platform.builtInRegexMatch(locale, LANG_TAG_EXT_RE);
        if (!match) {
            return undefined;
        }

        // Note: extensions are /((${extension})-)*/ and are made up of \\b(?:${singleton}(?:-${alphanum}{2,8})+)\\b
        // where the ${alphanum}{2,8} fields are of the form `${key}-${value}`.
        // TODO (doilij): return an array of `${key}-${value}` pairs

        // REVIEW (doilij): leading - might mean we need to filter: // ss.match(rr)[4].split('-').filter((x)=>!!x)
        // In that case:
        // TODO StringInstanceSplit
        // TODO ArrayInstanceFilter
        // let extSubtags = ArrayInstanceFilter(extensionsString.split('-'), (x)=>!!x);
        const extSubtags = match[0].split('-').filter((x) => !!x);
        // REVIEW (doilij): performance (testing for str[0]==='-' and using the string after that or updating the regex might be faster)

        return extSubtags;
    }

    var resolveLocaleHelper = function (locale, fitter, extensionFilter, defaultLocale) {
        var subTags = platform.getExtensions(locale);
        if (subTags === null) {
            // platform.getExtensions returns null to indicate fallback to JS implementation
            subTags = getExtensionSubtags(locale);
        }

        if (subTags) {
            callInstanceFunc(ArrayInstanceForEach, subTags, function (subTag) {
                locale = callInstanceFunc(StringInstanceReplace, locale, "-" + subTag, "");
            });
        }

        // Instead of using replace, we will match two groups, one capturing, one not. The non capturing group just strips away -u if present.
        // We are substituting for the function replace; which will only make a change if /-u$/ was found (-u at the end of the line)
        // And because match will return null if we don't match entire sequence, we are using the two groups stated above.
        locale = platform.builtInRegexMatch(locale, /(.*?)(?:-u)?$/)[1];
        var resolved = fitter(locale, defaultLocale);

        if (extensionFilter !== undefined) { // Filter to expected sub-tags
            var filtered = [];
            callInstanceFunc(ArrayInstanceForEach, subTags, (function (subTag) {
                var parts = platform.builtInRegexMatch(subTag, /([^-]*)-?(.*)?/); // [0] entire thing; [1] key; [2] value
                var key = parts[1];
                if (callInstanceFunc(ArrayInstanceIndexOf, extensionFilter, key) !== -1) {
                    callInstanceFunc(ArrayInstancePush, filtered, subTag);
                }
            }));
            subTags = filtered;
        }

        // As long as we are using the JS version of getExtensions on ICU, "u" will be considered an extension
        // of a locale like "de-u-co-phonebk"
        // Thus, we can't add the -u- ourselves here
        const withoutSubTags = resolved;
        if (resolved) {
            if (subTags && getArrayLength(subTags) > 0) {
                if (isPlatformUsingICU) {
                    resolved += "-";
                } else {
                    resolved += "-u-";
                }
            }

            resolved += callInstanceFunc(ArrayInstanceJoin, subTags, "-");
        } else {
            resolved = undefined;
        }

        return setPrototype({
            locale: resolved,
            subTags: subTags,
            localeWithoutSubtags: withoutSubTags
        }, null);
    }

    var resolveLocales = function (givenLocales, matcher, extensionFilter, defaultLocaleFunc) {
        var fitter = matcher === "lookup" ? resolveLocaleLookup : resolveLocaleBestFit;
        var length = getArrayLength(givenLocales);

        var defaultLocale = defaultLocaleFunc();

        length = length !== undefined ? length : 0;
        for (var i = 0; i < length; i++) {
            var resolved = resolveLocaleHelper(givenLocales[i], fitter, extensionFilter, defaultLocale);
            if (resolved.locale !== undefined) {
                return resolved;
            }
        }
        return resolveLocaleHelper(defaultLocale, fitter, undefined, defaultLocale);
    }

    // get just the language-script-region from the default locale
    let __strippedDefaultLocale = undefined;
    var strippedDefaultLocale = function () {
        if (__strippedDefaultLocale) {
            return __strippedDefaultLocale;
        }

        if (isPlatformUsingICU) {
            if (!LANG_TAG_BASE_RE) {
                InitializeLangTagREs();
            }

            const def = GetDefaultLocale();
            const match = platform.builtInRegexMatch(def, LANG_TAG_BASE_RE);
            if (match) {
                // strip extensions by matching only the base
                __strippedDefaultLocale = match[0];
            } else {
                __strippedDefaultLocale = def;
            }
        } else {
            // the only thing to strip off of a WinGlob locale is the collation,
            // which comes after the underscore
            __strippedDefaultLocale = platform.builtInRegexMatch(GetDefaultLocale(), /([^_]*).*/)[1];
        }

        return __strippedDefaultLocale;
    };

    var Internal = (function () {
        return setPrototype({
            ToObject: function (o) {
                if (o === null) {
                    platform.raiseNeedObject();
                }
                return o !== undefined ? Object(o) : undefined;
            },

            ToString: function (s) {
                return s !== undefined ? String(s) : undefined;
            },

            ToNumber: function (n) {
                return n === undefined ? NaN : Number(n);
            },

            ToLogicalBoolean: function (v) {
                return v !== undefined ? Boolean(v) : undefined;
            },

            ToUint32: function (n) {
                var num = Number(n),
                    ret = 0;
                if (!isNaN(num) && isFinite(num)) {
                    ret = Math.abs(num % Math.pow(2, 32));
                }
                return ret;
            },

            HasProperty: function (o, p) {
                // Walk the prototype chain
                while (o) {
                    if (callInstanceFunc(ObjectInstanceHasOwnProperty, o, p)) {
                        return true;
                    }
                    o = ObjectGetPrototypeOf(o);
                }
            }
        }, null)
    })();

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

            if (values !== undefined && callInstanceFunc(ArrayInstanceIndexOf, values, value) == -1) {
                platform.raiseOptionValueOutOfRange_3(String(value), String(property), "['" + callInstanceFunc(ArrayInstanceJoin, values, "', '") + "']");
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
                platform.raiseOptionValueOutOfRange_3(String(rawValue), String(property), "[" + minimum + " - " + maximum + "]");
            }

            return Math.floor(formattedValue);
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
    function InitializeLangTagREs() {
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
    }

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

            if (!IsWellFormedLanguageTag(tag)) {
                platform.raiseLocaleNotWellFormed(String(tag));
            }

            tag = platform.normalizeLanguageTag(tag);

            if (tag !== undefined && callInstanceFunc(ArrayInstanceIndexOf, seen, tag) === -1) {
                callInstanceFunc(ArrayInstancePush, seen, tag);
            }
        });

        return seen;
    }

    function LookupSupportedLocales(requestedLocales, fitter, defaultLocale) {
        var subset = [];
        var count = 0;
        callInstanceFunc(ArrayInstanceForEach, requestedLocales, function (locale) {
            try {
                var resolved = resolveLocaleHelper(locale, fitter, undefined, defaultLocale);
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
            return LookupSupportedLocales(locales, resolveLocaleBestFit, platform.normalizeLanguageTag(strippedDefaultLocale()));
        } else {
            return LookupSupportedLocales(locales, resolveLocaleLookup, strippedDefaultLocale());
        }
    };

    const intlStaticMethodThisArg = setPrototype({}, null);
    platform.setHiddenObject(intlStaticMethodThisArg, setPrototype({ isValid: "Valid" }, null));

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

    // If an empty string is encountered for the value of the property; that means that is by default.
    // So in the case of zh-TW; "default" and "stroke" are the same.
    // This list was discussed with AnBorod, AnGlass and SureshJa.
    var localesAcceptingCollationValues = setPrototype({
        "es-ES": setPrototype({ "trad": "tradnl" }, null),
        "lv-LV": setPrototype({ "trad": "tradnl" }, null),
        "de-DE": setPrototype({ "phonebk": "phoneb" }, null),
        "ja-JP": setPrototype({ "unihan": "radstr" }, null),
        // We believe "pronun" means "pronunciation"
        "zh-TW": setPrototype({ "phonetic": "pronun", "unihan": "radstr", "stroke": "" }, null),
        "zh-HK": setPrototype({ "unihan": "radstr", "stroke": "" }, null),
        "zh-MO": setPrototype({ "unihan": "radstr", "stroke": "" }, null),
        "zh-CN": setPrototype({ "stroke": "stroke", "pinyin": "" }, null),
        "zh-SG": setPrototype({ "stroke": "stroke", "pinyin": "" }, null)

        // The following locales are supported by Windows; however, no BCP47 equivalent collation values were found for these.
        // In future releases; this list (plus most of the Collator implementation) will be changed/removed as the platform support is expected to change.
        // "hu-HU": ["technl"],
        // "ka-GE": ["modern"],
        // "x-IV": ["mathan"]
    }, null);

    // reverses the keys and values in each locale's sub-object in localesAcceptingCollationValues
    // localesAcceptingCollationValues[locale][key] = value -> reverseLocalesAcceptingCollationValues[locale][value] = key
    var reverseLocalesAcceptingCollationValues = (function () {
        const toReturn = setPrototype({}, null);
        callInstanceFunc(ArrayInstanceForEach, ObjectGetOwnPropertyNames(localesAcceptingCollationValues), function (locale) {
            const collationValuesForLocale = localesAcceptingCollationValues[locale];
            const reversedCollationValues = setPrototype({}, null);

            callInstanceFunc(ArrayInstanceForEach, ObjectGetOwnPropertyNames(collationValuesForLocale), function (collation) {
                const windowsTag = collationValuesForLocale[collation];
                if (windowsTag !== "") {
                    reversedCollationValues[windowsTag] = collation;
                }
            });

            toReturn[locale] = reversedCollationValues;
        });
        return toReturn;
    }());

    // mappedDefaultLocale will get the default locale and update any deprecated
    // collation/sort order values it may use
    let __mappedDefaultLocale = undefined;
    var mappedDefaultLocale = function () {
        if (__mappedDefaultLocale && platform.useCaches) {
            return __mappedDefaultLocale;
        }

        let locale = undefined;
        let collation = undefined;
        if (isPlatformUsingICU) {
            // ICU's getDefaultLocale() will return a valid BCP-47/RFC 5646 langtag
            locale = GetDefaultLocale();
            const match = platform.builtInRegexMatch(locale, /-u(?:-[^\-][^\-]?-[^\-]+)*-co-([^\-]+).*/);
            if (match) {
                // if the system default locale had a collation, strip it for now
                // we will add the collation back later in this function
                collation = match[1];
                locale = callInstanceFunc(StringInstanceReplace, locale, `-co-${collation}`, "");
            }
        } else {
            // Windows' getDefaultLocale() will return a RFC4646 langtag
            const parts = platform.builtInRegexMatch(GetDefaultLocale(), /([^_]*)_?(.+)?/);
            locale = parts[1];
            collation = parts[2];
        }

        if (collation === undefined) {
            __mappedDefaultLocale = locale;
            return __mappedDefaultLocale;
        }

        // we stripped the -co-collation or _collation above, so this function adds it back
        const createLocaleCollationString = function (finalLocale, finalCollation) {
            if (isPlatformUsingICU) {
                return `${finalLocale}-co-${finalCollation}`;
            } else {
                return `${finalLocale}-u-co-${finalCollation}`;
            }
        };

        const collationMapForLocale = reverseLocalesAcceptingCollationValues[locale];
        if (collationMapForLocale === undefined) {
            // Assume the system wouldn't give us back a bad collation value
            __mappedDefaultLocale = createLocaleCollationString(locale, collation);
            return __mappedDefaultLocale;
        }

        const mappedCollation = collationMapForLocale[collation];
        if (mappedCollation !== undefined) {
            __mappedDefaultLocale = createLocaleCollationString(locale, mappedCollation);
        } else {
            __mappedDefaultLocale = createLocaleCollationString(locale, collation);
        }

        return __mappedDefaultLocale;
    };

    // Intl.Collator, String.prototype.localeCompare
    var Collator = (function () {

        if (InitType === 'Intl' || InitType === 'String') {

            function InitializeCollator(collator, localeList, options) {
                if (typeof collator != "object") {
                    platform.raiseNeedObject();
                }

                if (callInstanceFunc(ObjectInstanceHasOwnProperty, collator, '__initializedIntlObject') && collator.__initializedIntlObject) {
                    platform.raiseObjectIsAlreadyInitialized("Collator", "Collator");
                }

                collator.__initializedIntlObject = true;

                // Extract options
                if (typeof options === 'undefined') {
                    options = setPrototype({}, null);
                } else {
                    options = Internal.ToObject(options);
                }

                var matcher = GetOption(options, "localeMatcher", "string", ["lookup", "best fit"], "best fit");
                var usage = GetOption(options, "usage", "string", ["sort", "search"], "sort");
                var sensitivity = GetOption(options, "sensitivity", "string", ["base", "accent", "case", "variant"], undefined);
                var ignorePunctuation = GetOption(options, "ignorePunctuation", "boolean", undefined, false);
                var caseFirst = GetOption(options, "caseFirst", "string", ["upper", "lower", "false"], undefined);
                var numeric = GetOption(options, "numeric", "boolean", [true, false], undefined);

                // Deal with the locales and extensions
                localeList = CanonicalizeLocaleList(localeList);
                var resolvedLocaleInfo = resolveLocales(localeList, matcher, undefined, mappedDefaultLocale);

                var collation = "default";
                var resolvedLocaleLookup = resolveLocaleLookup(resolvedLocaleInfo.localeWithoutSubtags);
                var collationAugmentedLocale = resolvedLocaleLookup;

                if (resolvedLocaleInfo.subTags) {
                    callInstanceFunc(ArrayInstanceForEach, resolvedLocaleInfo.subTags, function (subTag) {
                        var parts = platform.builtInRegexMatch(subTag, /([^-]*)-?(.*)?/); // [0] entire thing; [1] key; [2] value
                        var key = parts[1];
                        var value = parts[2] === "" ? undefined : parts[2];
                        if (key === "kf" && caseFirst === undefined) {
                            caseFirst = GetOption(setPrototype({ caseFirst: value }, null), "caseFirst", "string", ["upper", "lower", "false"], undefined);
                        } else if (key === "kn" && numeric === undefined) {
                            if (value !== undefined) {
                                numeric = Internal.ToLogicalBoolean(callInstanceFunc(StringInstanceToLowerCase, value) === "true");
                            } else {
                                numeric = true;
                            }
                        } else if (key === "co" && value !== undefined && value !== "default" && value !== "search" && value !== "sort" && value !== "standard") {
                            // Ignore these collation values as they shouldn't have any impact
                            collation = value;
                        }
                    });
                }
                if (collation !== "default") {
                    var accepedCollationForLocale = localesAcceptingCollationValues[collationAugmentedLocale];
                    var windowsCollation = "";
                    if (accepedCollationForLocale !== undefined && (windowsCollation = accepedCollationForLocale[collation]) !== undefined) {
                        if (windowsCollation !== "") {
                            collationAugmentedLocale = collationAugmentedLocale + "_" + windowsCollation;
                        }
                    }
                    else {
                        collation = "default";
                    }
                }

                // Correct options if need be.
                if (caseFirst === undefined) {
                    try {
                        var num = platform.compareString('A', 'a', resolvedLocaleLookup, undefined, undefined, undefined, undefined);
                    } catch (e) {
                        // Rethrow OOM or SOE
                        throwExIfOOMOrSOE(e);

                        // Otherwise, Generic message to cover the exception throw from the CompareStringEx api.
                        // The platform's exception is also generic and in most if not all cases specifies that "a" argument is invalid.
                        // We have no other information from the platform on the cause of the exception.
                        platform.raiseOptionValueOutOfRange();
                    }

                    if (num === 0) {
                        caseFirst = 'false';
                    } else if (num === -1) {
                        caseFirst = 'upper';
                    } else {
                        caseFirst = 'lower';
                    }
                }

                if (sensitivity === undefined) {
                    sensitivity = "variant";
                }

                if (numeric === undefined) {
                    numeric = false;
                }

                // Set the options on the object
                collator.__matcher = matcher;
                collator.__locale = resolvedLocaleInfo.localeWithoutSubtags;
                collator.__localeForCompare = collationAugmentedLocale;
                collator.__usage = usage;
                collator.__sensitivity = sensitivity;
                collator.__ignorePunctuation = ignorePunctuation;
                collator.__caseFirst = caseFirst;
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
                return Number(platform.compareString(
                    thisArg,
                    that,
                    stateObject.__localeForCompare,
                    platform.CollatorSensitivity[stateObject.__sensitivity],
                    stateObject.__ignorePunctuation,
                    stateObject.__numeric,
                    platform.CollatorCaseFirst[stateObject.__caseFirst]
                ));
            }), platform.BuiltInFunctionID.StringLocaleCompare);

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

                    return Number(platform.compareString(
                        a,
                        b,
                        hiddenObject.__localeForCompare,
                        platform.CollatorSensitivity[hiddenObject.__sensitivity],
                        hiddenObject.__ignorePunctuation,
                        hiddenObject.__numeric,
                        platform.CollatorCaseFirst[hiddenObject.__caseFirst]
                    ));
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

                        return {
                            locale: hiddenObject.__locale,
                            usage: hiddenObject.__usage,
                            sensitivity: hiddenObject.__sensitivity,
                            ignorePunctuation: hiddenObject.__ignorePunctuation,
                            collation: hiddenObject.__collation, // "co" unicode extension
                            numeric: hiddenObject.__numeric,     // "ka" unicode extension TODO: Determine if this is supported (doesn't have to be)
                            caseFirst: hiddenObject.__caseFirst  // "kf" unicode extension TODO: Determine if this is supported (doesn't have to be)
                        }
                    }, writable: true, enumerable: false, configurable: true
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
                    }), enumerable: false, configurable: true
                });

                return Collator;
            }
        }
        // 'Init.Collator' not defined if reached here. Return 'undefined'
        return undefined;
    })();

    // Intl.NumberFormat, Number.prototype.toLocaleString
    var NumberFormat = (function () {
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

                var formatterToUse = platform.NumberFormatStyle[style];

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
                var resolvedLocaleInfo = resolveLocales(localeList, matcher, ["nu"], strippedDefaultLocale);

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
                    maximumFractionDigitsDefault = Math.max(currencyDigits, minimumFractionDigits);
                } else {
                    currency = undefined;
                    currencyDisplay = undefined;
                    minimumFractionDigits = GetNumberOption(options, 'minimumFractionDigits', 0, 20, 0);
                    if (style === "percent") {
                        maximumFractionDigitsDefault = Math.max(minimumFractionDigits, 0);
                    } else {
                        maximumFractionDigitsDefault = Math.max(minimumFractionDigits, 3)
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
                    numberFormat.__currencyDisplayToUse = platform.NumberFormatCurrencyDisplay[currencyDisplay];
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
                return String(platform.formatNumber(n, stateObject));
            }), platform.BuiltInFunctionID.NumberToLocaleString);

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

                    return String(platform.formatNumber(n, hiddenObject));
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
                var resolvedLocaleInfo = resolveLocales(localeList, matcher, ["nu", "ca"], strippedDefaultLocale);

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
                if (platform.useCaches && locales === undefined && options === undefined) {
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
            })("Date.prototype.toLocaleString", "any", "all", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleString, platform.BuiltInFunctionID.DateToLocaleString);

            (function (name, option1, option2, cacheSlot, platformFunctionID) {
                platform.registerBuiltInFunction(tagPublicFunction(name, function date_toLocaleDateString_entryPoint(locales = undefined, options = undefined) {
                    return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
                }), platformFunctionID);
            })("Date.prototype.toLocaleDateString", "date", "date", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleDateString, platform.BuiltInFunctionID.DateToLocaleDateString);

            (function (name, option1, option2, cacheSlot, platformFunctionID) {
                platform.registerBuiltInFunction(tagPublicFunction(name, function date_toLocaleTimeString_entryPoint(locales = undefined, options = undefined) {
                    return DateInstanceToLocaleStringImplementation.call(this, name, option1, option2, cacheSlot, locales, options);
                }), platformFunctionID);
            })("Date.prototype.toLocaleTimeString", "time", "time", __DateInstanceToLocaleStringDefaultCacheSlot.toLocaleTimeString, platform.BuiltInFunctionID.DateToLocaleTimeString);

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

                    if (date !== undefined && (isNaN(date) || !isFinite(date))) {
                        platform.raiseInvalidDate();
                    }

                    let dateValue = undefined;
                    if (date !== undefined) {
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
        ObjectDefineProperty(Intl, "Collator", { value: Collator, writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "NumberFormat", { value: NumberFormat, writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "DateTimeFormat", { value: DateTimeFormat, writable: true, enumerable: false, configurable: true });
        ObjectDefineProperty(Intl, "getCanonicalLocales", { value: getCanonicalLocales, writable: true, enumerable: false, configurable: true });
    }

    } // END WINGLOB
});
