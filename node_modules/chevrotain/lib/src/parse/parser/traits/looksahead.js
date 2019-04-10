"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var lookahead_1 = require("../../grammar/lookahead");
var utils_1 = require("../../../utils/utils");
var parser_1 = require("../parser");
var keys_1 = require("../../grammar/keys");
/**
 * Trait responsible for the lookahead related utilities and optimizations.
 */
var LooksAhead = /** @class */ (function () {
    function LooksAhead() {
    }
    LooksAhead.prototype.initLooksAhead = function (config) {
        this.dynamicTokensEnabled = utils_1.has(config, "dynamicTokensEnabled")
            ? config.dynamicTokensEnabled
            : parser_1.DEFAULT_PARSER_CONFIG.dynamicTokensEnabled;
        this.maxLookahead = utils_1.has(config, "maxLookahead")
            ? config.maxLookahead
            : parser_1.DEFAULT_PARSER_CONFIG.maxLookahead;
        /* istanbul ignore next - Using plain array as dictionary will be tested on older node.js versions and IE11 */
        this.lookAheadFuncsCache = utils_1.isES2015MapSupported() ? new Map() : [];
        // Performance optimization on newer engines that support ES6 Map
        // For larger Maps this is slightly faster than using a plain object (array in our case).
        /* istanbul ignore else - The else branch will be tested on older node.js versions and IE11 */
        if (utils_1.isES2015MapSupported()) {
            this.getLaFuncFromCache = this.getLaFuncFromMap;
            this.setLaFuncCache = this.setLaFuncCacheUsingMap;
        }
        else {
            this.getLaFuncFromCache = this.getLaFuncFromObj;
            this.setLaFuncCache = this.setLaFuncUsingObj;
        }
    };
    LooksAhead.prototype.lookAheadBuilderForOptional = function (alt, tokenMatcher, dynamicTokensEnabled) {
        return lookahead_1.buildSingleAlternativeLookaheadFunction(alt, tokenMatcher, dynamicTokensEnabled);
    };
    LooksAhead.prototype.lookAheadBuilderForAlternatives = function (alts, hasPredicates, tokenMatcher, dynamicTokensEnabled) {
        return lookahead_1.buildAlternativesLookAheadFunc(alts, hasPredicates, tokenMatcher, dynamicTokensEnabled);
    };
    // this actually returns a number, but it is always used as a string (object prop key)
    LooksAhead.prototype.getKeyForAutomaticLookahead = function (dslMethodIdx, occurrence) {
        var currRuleShortName = this.getLastExplicitRuleShortName();
        return keys_1.getKeyForAutomaticLookahead(currRuleShortName, dslMethodIdx, occurrence);
    };
    LooksAhead.prototype.getLookaheadFuncForOr = function (occurrence, alts) {
        var key = this.getKeyForAutomaticLookahead(keys_1.OR_IDX, occurrence);
        var laFunc = this.getLaFuncFromCache(key);
        if (laFunc === undefined) {
            var ruleName = this.getCurrRuleFullName();
            var ruleGrammar = this.getGAstProductions().get(ruleName);
            // note that hasPredicates is only computed once.
            var hasPredicates = utils_1.some(alts, function (currAlt) {
                return utils_1.isFunction(currAlt.GATE);
            });
            laFunc = lookahead_1.buildLookaheadFuncForOr(occurrence, ruleGrammar, this.maxLookahead, hasPredicates, this.dynamicTokensEnabled, this.lookAheadBuilderForAlternatives);
            this.setLaFuncCache(key, laFunc);
            return laFunc;
        }
        else {
            return laFunc;
        }
    };
    // Automatic lookahead calculation
    LooksAhead.prototype.getLookaheadFuncForOption = function (key, occurrence) {
        return this.getLookaheadFuncFor(key, occurrence, this.maxLookahead, lookahead_1.PROD_TYPE.OPTION);
    };
    LooksAhead.prototype.getLookaheadFuncForMany = function (key, occurrence) {
        return this.getLookaheadFuncFor(key, occurrence, this.maxLookahead, lookahead_1.PROD_TYPE.REPETITION);
    };
    LooksAhead.prototype.getLookaheadFuncForManySep = function (key, occurrence) {
        return this.getLookaheadFuncFor(key, occurrence, this.maxLookahead, lookahead_1.PROD_TYPE.REPETITION_WITH_SEPARATOR);
    };
    LooksAhead.prototype.getLookaheadFuncForAtLeastOne = function (key, occurrence) {
        return this.getLookaheadFuncFor(key, occurrence, this.maxLookahead, lookahead_1.PROD_TYPE.REPETITION_MANDATORY);
    };
    LooksAhead.prototype.getLookaheadFuncForAtLeastOneSep = function (key, occurrence) {
        return this.getLookaheadFuncFor(key, occurrence, this.maxLookahead, lookahead_1.PROD_TYPE.REPETITION_MANDATORY_WITH_SEPARATOR);
    };
    LooksAhead.prototype.getLookaheadFuncFor = function (key, occurrence, maxLookahead, prodType) {
        var laFunc = this.getLaFuncFromCache(key);
        if (laFunc === undefined) {
            var ruleName = this.getCurrRuleFullName();
            var ruleGrammar = this.getGAstProductions().get(ruleName);
            laFunc = lookahead_1.buildLookaheadFuncForOptionalProd(occurrence, ruleGrammar, maxLookahead, this.dynamicTokensEnabled, prodType, this.lookAheadBuilderForOptional);
            this.setLaFuncCache(key, laFunc);
            return laFunc;
        }
        else {
            return laFunc;
        }
    };
    /* istanbul ignore next */
    LooksAhead.prototype.getLaFuncFromCache = function (key) {
        return undefined;
    };
    LooksAhead.prototype.getLaFuncFromMap = function (key) {
        return this.lookAheadFuncsCache.get(key);
    };
    /* istanbul ignore next - Using plain array as dictionary will be tested on older node.js versions and IE11 */
    LooksAhead.prototype.getLaFuncFromObj = function (key) {
        return this.lookAheadFuncsCache[key];
    };
    /* istanbul ignore next */
    LooksAhead.prototype.setLaFuncCache = function (key, value) { };
    LooksAhead.prototype.setLaFuncCacheUsingMap = function (key, value) {
        this.lookAheadFuncsCache.set(key, value);
    };
    /* istanbul ignore next - Using plain array as dictionary will be tested on older node.js versions and IE11 */
    LooksAhead.prototype.setLaFuncUsingObj = function (key, value) {
        this.lookAheadFuncsCache[key] = value;
    };
    return LooksAhead;
}());
exports.LooksAhead = LooksAhead;
//# sourceMappingURL=looksahead.js.map