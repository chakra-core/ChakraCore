"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var utils_1 = require("../../../utils/utils");
var keys_1 = require("../../grammar/keys");
var exceptions_public_1 = require("../../exceptions_public");
var lookahead_1 = require("../../grammar/lookahead");
var interpreter_1 = require("../../grammar/interpreter");
var parser_1 = require("../parser");
var recoverable_1 = require("./recoverable");
var tokens_public_1 = require("../../../scan/tokens_public");
var tokens_1 = require("../../../scan/tokens");
var lang_extensions_1 = require("../../../lang/lang_extensions");
/**
 * This trait is responsible for the runtime parsing engine
 * Used by the official API (recognizer_api.ts)
 */
var RecognizerEngine = /** @class */ (function () {
    function RecognizerEngine() {
    }
    RecognizerEngine.prototype.initRecognizerEngine = function (tokenVocabulary, config) {
        this.className = lang_extensions_1.classNameFromInstance(this);
        // TODO: would using an ES6 Map or plain object be faster (CST building scenario)
        this.shortRuleNameToFull = new lang_extensions_1.HashTable();
        this.fullRuleNameToShort = new lang_extensions_1.HashTable();
        this.ruleShortNameIdx = 256;
        this.tokenMatcher = tokens_1.tokenStructuredMatcherNoCategories;
        this.definedRulesNames = [];
        this.tokensMap = {};
        this.allRuleNames = [];
        this.isBackTrackingStack = [];
        this.RULE_STACK = [];
        this.RULE_OCCURRENCE_STACK = [];
        this.gastProductionsCache = new lang_extensions_1.HashTable();
        this.serializedGrammar = utils_1.has(config, "serializedGrammar")
            ? config.serializedGrammar
            : parser_1.DEFAULT_PARSER_CONFIG.serializedGrammar;
        if (utils_1.isArray(tokenVocabulary)) {
            // This only checks for Token vocabularies provided as arrays.
            // That is good enough because the main objective is to detect users of pre-V4.0 APIs
            // rather than all edge cases of empty Token vocabularies.
            if (utils_1.isEmpty(tokenVocabulary)) {
                throw Error("A Token Vocabulary cannot be empty.\n" +
                    "\tNote that the first argument for the parser constructor\n" +
                    "\tis no longer a Token vector (since v4.0).");
            }
            if (typeof tokenVocabulary[0].startOffset === "number") {
                throw Error("The Parser constructor no longer accepts a token vector as the first argument.\n" +
                    "\tSee: https://sap.github.io/chevrotain/docs/changes/BREAKING_CHANGES.html#_4-0-0\n" +
                    "\tFor Further details.");
            }
        }
        if (utils_1.isArray(tokenVocabulary)) {
            this.tokensMap = utils_1.reduce(tokenVocabulary, function (acc, tokenClazz) {
                acc[tokens_public_1.tokenName(tokenClazz)] = tokenClazz;
                return acc;
            }, {});
        }
        else if (utils_1.has(tokenVocabulary, "modes") &&
            utils_1.every(utils_1.flatten(utils_1.values(tokenVocabulary.modes)), tokens_1.isTokenType)) {
            var allTokenTypes = utils_1.flatten(utils_1.values(tokenVocabulary.modes));
            var uniqueTokens = utils_1.uniq(allTokenTypes);
            this.tokensMap = utils_1.reduce(uniqueTokens, function (acc, tokenClazz) {
                acc[tokens_public_1.tokenName(tokenClazz)] = tokenClazz;
                return acc;
            }, {});
        }
        else if (utils_1.isObject(tokenVocabulary)) {
            this.tokensMap = utils_1.cloneObj(tokenVocabulary);
        }
        else {
            throw new Error("<tokensDictionary> argument must be An Array of Token constructors," +
                " A dictionary of Token constructors or an IMultiModeLexerDefinition");
        }
        // always add EOF to the tokenNames -> constructors map. it is useful to assure all the input has been
        // parsed with a clear error message ("expecting EOF but found ...")
        /* tslint:disable */
        this.tokensMap["EOF"] = tokens_public_1.EOF;
        // TODO: This check may not be accurate for multi mode lexers
        var noTokenCategoriesUsed = utils_1.every(utils_1.values(tokenVocabulary), function (tokenConstructor) { return utils_1.isEmpty(tokenConstructor.categoryMatches); });
        this.tokenMatcher = noTokenCategoriesUsed
            ? tokens_1.tokenStructuredMatcherNoCategories
            : tokens_1.tokenStructuredMatcher;
        // Because ES2015+ syntax should be supported for creating Token classes
        // We cannot assume that the Token classes were created using the "extendToken" utilities
        // Therefore we must augment the Token classes both on Lexer initialization and on Parser initialization
        tokens_1.augmentTokenTypes(utils_1.values(this.tokensMap));
    };
    RecognizerEngine.prototype.defineRule = function (ruleName, impl, config) {
        if (this.selfAnalysisDone) {
            throw Error("Grammar rule <" + ruleName + "> may not be defined after the 'performSelfAnalysis' method has been called'\n" +
                "Make sure that all grammar rule definitions are done before 'performSelfAnalysis' is called.");
        }
        var resyncEnabled = utils_1.has(config, "resyncEnabled")
            ? config.resyncEnabled
            : parser_1.DEFAULT_RULE_CONFIG.resyncEnabled;
        var recoveryValueFunc = utils_1.has(config, "recoveryValueFunc")
            ? config.recoveryValueFunc
            : parser_1.DEFAULT_RULE_CONFIG.recoveryValueFunc;
        // performance optimization: Use small integers as keys for the longer human readable "full" rule names.
        // this greatly improves Map access time (as much as 8% for some performance benchmarks).
        /* tslint:disable */
        var shortName = this.ruleShortNameIdx <<
            (keys_1.BITS_FOR_METHOD_IDX + keys_1.BITS_FOR_OCCURRENCE_IDX);
        /* tslint:enable */
        this.ruleShortNameIdx++;
        this.shortRuleNameToFull.put(shortName, ruleName);
        this.fullRuleNameToShort.put(ruleName, shortName);
        function invokeRuleWithTry(args) {
            try {
                // TODO: dynamically get rid of this?
                if (this.outputCst === true) {
                    impl.apply(this, args);
                    return this.CST_STACK[this.CST_STACK.length - 1];
                }
                else {
                    return impl.apply(this, args);
                }
            }
            catch (e) {
                var isFirstInvokedRule = this.RULE_STACK.length === 1;
                // note the reSync is always enabled for the first rule invocation, because we must always be able to
                // reSync with EOF and just output some INVALID ParseTree
                // during backtracking reSync recovery is disabled, otherwise we can't be certain the backtracking
                // path is really the most valid one
                var reSyncEnabled = resyncEnabled &&
                    !this.isBackTracking() &&
                    this.recoveryEnabled;
                if (exceptions_public_1.isRecognitionException(e)) {
                    if (reSyncEnabled) {
                        var reSyncTokType = this.findReSyncTokenType();
                        if (this.isInCurrentRuleReSyncSet(reSyncTokType)) {
                            e.resyncedTokens = this.reSyncTo(reSyncTokType);
                            if (this.outputCst) {
                                var partialCstResult = this.CST_STACK[this.CST_STACK.length - 1];
                                partialCstResult.recoveredNode = true;
                                return partialCstResult;
                            }
                            else {
                                return recoveryValueFunc();
                            }
                        }
                        else {
                            if (this.outputCst) {
                                var partialCstResult = this.CST_STACK[this.CST_STACK.length - 1];
                                partialCstResult.recoveredNode = true;
                                e.partialCstResult = partialCstResult;
                            }
                            // to be handled Further up the call stack
                            throw e;
                        }
                    }
                    else if (isFirstInvokedRule) {
                        // otherwise a Redundant input error will be created as well and we cannot guarantee that this is indeed the case
                        this.moveToTerminatedState();
                        // the parser should never throw one of its own errors outside its flow.
                        // even if error recovery is disabled
                        return recoveryValueFunc();
                    }
                    else {
                        // to be handled Further up the call stack
                        throw e;
                    }
                }
                else {
                    // some other Error type which we don't know how to handle (for example a built in JavaScript Error)
                    throw e;
                }
            }
            finally {
                this.ruleFinallyStateUpdate();
            }
        }
        var wrappedGrammarRule;
        wrappedGrammarRule = function (idxInCallingRule, args) {
            if (idxInCallingRule === void 0) { idxInCallingRule = 0; }
            this.ruleInvocationStateUpdate(shortName, ruleName, idxInCallingRule);
            return invokeRuleWithTry.call(this, args);
        };
        var ruleNamePropName = "ruleName";
        wrappedGrammarRule[ruleNamePropName] = ruleName;
        return wrappedGrammarRule;
    };
    // Implementation of parsing DSL
    RecognizerEngine.prototype.optionInternal = function (actionORMethodDef, occurrence) {
        var key = this.getKeyForAutomaticLookahead(keys_1.OPTION_IDX, occurrence);
        var nestedName = this.nestedRuleBeforeClause(actionORMethodDef, key);
        try {
            return this.optionInternalLogic(actionORMethodDef, occurrence, key);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(key, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.optionInternalNoCst = function (actionORMethodDef, occurrence) {
        var key = this.getKeyForAutomaticLookahead(keys_1.OPTION_IDX, occurrence);
        return this.optionInternalLogic(actionORMethodDef, occurrence, key);
    };
    RecognizerEngine.prototype.optionInternalLogic = function (actionORMethodDef, occurrence, key) {
        var _this = this;
        var lookAheadFunc = this.getLookaheadFuncForOption(key, occurrence);
        var action;
        var predicate;
        if (actionORMethodDef.DEF !== undefined) {
            action = actionORMethodDef.DEF;
            predicate = actionORMethodDef.GATE;
            // predicate present
            if (predicate !== undefined) {
                var orgLookaheadFunction_1 = lookAheadFunc;
                lookAheadFunc = function () {
                    return (predicate.call(_this) && orgLookaheadFunction_1.call(_this));
                };
            }
        }
        else {
            action = actionORMethodDef;
        }
        if (lookAheadFunc.call(this) === true) {
            return action.call(this);
        }
        return undefined;
    };
    RecognizerEngine.prototype.atLeastOneInternal = function (prodOccurrence, actionORMethodDef) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.AT_LEAST_ONE_IDX, prodOccurrence);
        var nestedName = this.nestedRuleBeforeClause(actionORMethodDef, laKey);
        try {
            return this.atLeastOneInternalLogic(prodOccurrence, actionORMethodDef, laKey);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(laKey, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.atLeastOneInternalNoCst = function (prodOccurrence, actionORMethodDef) {
        var key = this.getKeyForAutomaticLookahead(keys_1.AT_LEAST_ONE_IDX, prodOccurrence);
        this.atLeastOneInternalLogic(prodOccurrence, actionORMethodDef, key);
    };
    RecognizerEngine.prototype.atLeastOneInternalLogic = function (prodOccurrence, actionORMethodDef, key) {
        var _this = this;
        var lookAheadFunc = this.getLookaheadFuncForAtLeastOne(key, prodOccurrence);
        var action;
        var predicate;
        if (actionORMethodDef.DEF !== undefined) {
            action = actionORMethodDef.DEF;
            predicate = actionORMethodDef.GATE;
            // predicate present
            if (predicate !== undefined) {
                var orgLookaheadFunction_2 = lookAheadFunc;
                lookAheadFunc = function () {
                    return (predicate.call(_this) && orgLookaheadFunction_2.call(_this));
                };
            }
        }
        else {
            action = actionORMethodDef;
        }
        if (lookAheadFunc.call(this) === true) {
            ;
            action.call(this);
            while (lookAheadFunc.call(this) === true) {
                this.doSingleRepetition(action);
            }
        }
        else {
            throw this.raiseEarlyExitException(prodOccurrence, lookahead_1.PROD_TYPE.REPETITION_MANDATORY, actionORMethodDef.ERR_MSG);
        }
        // note that while it may seem that this can cause an error because by using a recursive call to
        // AT_LEAST_ONE we change the grammar to AT_LEAST_TWO, AT_LEAST_THREE ... , the possible recursive call
        // from the tryInRepetitionRecovery(...) will only happen IFF there really are TWO/THREE/.... items.
        // Performance optimization: "attemptInRepetitionRecovery" will be defined as NOOP unless recovery is enabled
        this.attemptInRepetitionRecovery(this.atLeastOneInternal, [prodOccurrence, actionORMethodDef], lookAheadFunc, keys_1.AT_LEAST_ONE_IDX, prodOccurrence, interpreter_1.NextTerminalAfterAtLeastOneWalker);
    };
    RecognizerEngine.prototype.atLeastOneSepFirstInternal = function (prodOccurrence, options) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.AT_LEAST_ONE_SEP_IDX, prodOccurrence);
        var nestedName = this.nestedRuleBeforeClause(options, laKey);
        try {
            this.atLeastOneSepFirstInternalLogic(prodOccurrence, options, laKey);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(laKey, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.atLeastOneSepFirstInternalNoCst = function (prodOccurrence, options) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.AT_LEAST_ONE_SEP_IDX, prodOccurrence);
        this.atLeastOneSepFirstInternalLogic(prodOccurrence, options, laKey);
    };
    RecognizerEngine.prototype.atLeastOneSepFirstInternalLogic = function (prodOccurrence, options, key) {
        var _this = this;
        var action = options.DEF;
        var separator = options.SEP;
        var firstIterationLookaheadFunc = this.getLookaheadFuncForAtLeastOneSep(key, prodOccurrence);
        // 1st iteration
        if (firstIterationLookaheadFunc.call(this) === true) {
            ;
            action.call(this);
            //  TODO: Optimization can move this function construction into "attemptInRepetitionRecovery"
            //  because it is only needed in error recovery scenarios.
            var separatorLookAheadFunc = function () {
                return _this.tokenMatcher(_this.LA(1), separator);
            };
            // 2nd..nth iterations
            while (this.tokenMatcher(this.LA(1), separator) === true) {
                // note that this CONSUME will never enter recovery because
                // the separatorLookAheadFunc checks that the separator really does exist.
                this.CONSUME(separator);
                action.call(this);
            }
            // Performance optimization: "attemptInRepetitionRecovery" will be defined as NOOP unless recovery is enabled
            this.attemptInRepetitionRecovery(this.repetitionSepSecondInternal, [
                prodOccurrence,
                separator,
                separatorLookAheadFunc,
                action,
                interpreter_1.NextTerminalAfterAtLeastOneSepWalker
            ], separatorLookAheadFunc, keys_1.AT_LEAST_ONE_SEP_IDX, prodOccurrence, interpreter_1.NextTerminalAfterAtLeastOneSepWalker);
        }
        else {
            throw this.raiseEarlyExitException(prodOccurrence, lookahead_1.PROD_TYPE.REPETITION_MANDATORY_WITH_SEPARATOR, options.ERR_MSG);
        }
    };
    RecognizerEngine.prototype.manyInternal = function (prodOccurrence, actionORMethodDef) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.MANY_IDX, prodOccurrence);
        var nestedName = this.nestedRuleBeforeClause(actionORMethodDef, laKey);
        try {
            return this.manyInternalLogic(prodOccurrence, actionORMethodDef, laKey);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(laKey, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.manyInternalNoCst = function (prodOccurrence, actionORMethodDef) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.MANY_IDX, prodOccurrence);
        return this.manyInternalLogic(prodOccurrence, actionORMethodDef, laKey);
    };
    RecognizerEngine.prototype.manyInternalLogic = function (prodOccurrence, actionORMethodDef, key) {
        var _this = this;
        var lookaheadFunction = this.getLookaheadFuncForMany(key, prodOccurrence);
        var action;
        var predicate;
        if (actionORMethodDef.DEF !== undefined) {
            action = actionORMethodDef.DEF;
            predicate = actionORMethodDef.GATE;
            // predicate present
            if (predicate !== undefined) {
                var orgLookaheadFunction_3 = lookaheadFunction;
                lookaheadFunction = function () {
                    return (predicate.call(_this) && orgLookaheadFunction_3.call(_this));
                };
            }
        }
        else {
            action = actionORMethodDef;
        }
        while (lookaheadFunction.call(this)) {
            this.doSingleRepetition(action);
        }
        // Performance optimization: "attemptInRepetitionRecovery" will be defined as NOOP unless recovery is enabled
        this.attemptInRepetitionRecovery(this.manyInternal, [prodOccurrence, actionORMethodDef], lookaheadFunction, keys_1.MANY_IDX, prodOccurrence, interpreter_1.NextTerminalAfterManyWalker);
    };
    RecognizerEngine.prototype.manySepFirstInternal = function (prodOccurrence, options) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.MANY_SEP_IDX, prodOccurrence);
        var nestedName = this.nestedRuleBeforeClause(options, laKey);
        try {
            this.manySepFirstInternalLogic(prodOccurrence, options, laKey);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(laKey, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.manySepFirstInternalNoCst = function (prodOccurrence, options) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.MANY_SEP_IDX, prodOccurrence);
        this.manySepFirstInternalLogic(prodOccurrence, options, laKey);
    };
    RecognizerEngine.prototype.manySepFirstInternalLogic = function (prodOccurrence, options, key) {
        var _this = this;
        var action = options.DEF;
        var separator = options.SEP;
        var firstIterationLaFunc = this.getLookaheadFuncForManySep(key, prodOccurrence);
        // 1st iteration
        if (firstIterationLaFunc.call(this) === true) {
            action.call(this);
            var separatorLookAheadFunc = function () {
                return _this.tokenMatcher(_this.LA(1), separator);
            };
            // 2nd..nth iterations
            while (this.tokenMatcher(this.LA(1), separator) === true) {
                // note that this CONSUME will never enter recovery because
                // the separatorLookAheadFunc checks that the separator really does exist.
                this.CONSUME(separator);
                // No need for checking infinite loop here due to consuming the separator.
                action.call(this);
            }
            // Performance optimization: "attemptInRepetitionRecovery" will be defined as NOOP unless recovery is enabled
            this.attemptInRepetitionRecovery(this.repetitionSepSecondInternal, [
                prodOccurrence,
                separator,
                separatorLookAheadFunc,
                action,
                interpreter_1.NextTerminalAfterManySepWalker
            ], separatorLookAheadFunc, keys_1.MANY_SEP_IDX, prodOccurrence, interpreter_1.NextTerminalAfterManySepWalker);
        }
    };
    RecognizerEngine.prototype.repetitionSepSecondInternal = function (prodOccurrence, separator, separatorLookAheadFunc, action, nextTerminalAfterWalker) {
        while (separatorLookAheadFunc()) {
            // note that this CONSUME will never enter recovery because
            // the separatorLookAheadFunc checks that the separator really does exist.
            this.CONSUME(separator);
            action.call(this);
        }
        // we can only arrive to this function after an error
        // has occurred (hence the name 'second') so the following
        // IF will always be entered, its possible to remove it...
        // however it is kept to avoid confusion and be consistent.
        // Performance optimization: "attemptInRepetitionRecovery" will be defined as NOOP unless recovery is enabled
        /* istanbul ignore else */
        this.attemptInRepetitionRecovery(this.repetitionSepSecondInternal, [
            prodOccurrence,
            separator,
            separatorLookAheadFunc,
            action,
            nextTerminalAfterWalker
        ], separatorLookAheadFunc, keys_1.AT_LEAST_ONE_SEP_IDX, prodOccurrence, nextTerminalAfterWalker);
    };
    RecognizerEngine.prototype.doSingleRepetition = function (action) {
        var beforeIteration = this.getLexerPosition();
        var result = action.call(this);
        var afterIteration = this.getLexerPosition();
        if (afterIteration === beforeIteration) {
            throw Error("Infinite loop detected\n" +
                "\tSee: https://sap.github.io/chevrotain/docs/guide/resolving_grammar_errors.html#INFINITE_LOOP\n" +
                "\tFor Further details.");
        }
        return result;
    };
    RecognizerEngine.prototype.orInternalNoCst = function (altsOrOpts, occurrence) {
        var alts = utils_1.isArray(altsOrOpts)
            ? altsOrOpts
            : altsOrOpts.DEF;
        var laFunc = this.getLookaheadFuncForOr(occurrence, alts);
        var altIdxToTake = laFunc.call(this, alts);
        if (altIdxToTake !== undefined) {
            var chosenAlternative = alts[altIdxToTake];
            return chosenAlternative.ALT.call(this);
        }
        this.raiseNoAltException(occurrence, altsOrOpts.ERR_MSG);
    };
    RecognizerEngine.prototype.orInternal = function (altsOrOpts, occurrence) {
        var laKey = this.getKeyForAutomaticLookahead(keys_1.OR_IDX, occurrence);
        var nestedName = this.nestedRuleBeforeClause(altsOrOpts, laKey);
        try {
            var alts = utils_1.isArray(altsOrOpts)
                ? altsOrOpts
                : altsOrOpts.DEF;
            var laFunc = this.getLookaheadFuncForOr(occurrence, alts);
            var altIdxToTake = laFunc.call(this, alts);
            if (altIdxToTake !== undefined) {
                var chosenAlternative = alts[altIdxToTake];
                var nestedAltBeforeClauseResult = this.nestedAltBeforeClause(chosenAlternative, occurrence, keys_1.OR_IDX, altIdxToTake);
                try {
                    return chosenAlternative.ALT.call(this);
                }
                finally {
                    if (nestedAltBeforeClauseResult !== undefined) {
                        this.nestedRuleFinallyClause(nestedAltBeforeClauseResult.shortName, nestedAltBeforeClauseResult.nestedName);
                    }
                }
            }
            this.raiseNoAltException(occurrence, altsOrOpts.ERR_MSG);
        }
        finally {
            if (nestedName !== undefined) {
                this.nestedRuleFinallyClause(laKey, nestedName);
            }
        }
    };
    RecognizerEngine.prototype.ruleFinallyStateUpdate = function () {
        this.RULE_STACK.pop();
        this.RULE_OCCURRENCE_STACK.pop();
        // NOOP when cst is disabled
        this.cstFinallyStateUpdate();
        if (this.RULE_STACK.length === 0 && !this.isAtEndOfInput()) {
            var firstRedundantTok = this.LA(1);
            var errMsg = this.errorMessageProvider.buildNotAllInputParsedMessage({
                firstRedundant: firstRedundantTok,
                ruleName: this.getCurrRuleFullName()
            });
            this.SAVE_ERROR(new exceptions_public_1.NotAllInputParsedException(errMsg, firstRedundantTok));
        }
    };
    RecognizerEngine.prototype.subruleInternal = function (ruleToCall, idx, options) {
        var ruleResult;
        try {
            var args = options !== undefined ? options.ARGS : undefined;
            ruleResult = ruleToCall.call(this, idx, args);
            this.cstPostNonTerminal(ruleResult, options !== undefined && options.LABEL !== undefined
                ? options.LABEL
                : ruleToCall.ruleName);
            return ruleResult;
        }
        catch (e) {
            if (exceptions_public_1.isRecognitionException(e) && e.partialCstResult !== undefined) {
                this.cstPostNonTerminal(e.partialCstResult, options !== undefined && options.LABEL !== undefined
                    ? options.LABEL
                    : ruleToCall.ruleName);
                delete e.partialCstResult;
            }
            throw e;
        }
    };
    RecognizerEngine.prototype.consumeInternal = function (tokType, idx, options) {
        var consumedToken;
        try {
            var nextToken = this.LA(1);
            if (this.tokenMatcher(nextToken, tokType) === true) {
                this.consumeToken();
                consumedToken = nextToken;
            }
            else {
                var msg = void 0;
                var previousToken = this.LA(0);
                if (options !== undefined && options.ERR_MSG) {
                    msg = options.ERR_MSG;
                }
                else {
                    msg = this.errorMessageProvider.buildMismatchTokenMessage({
                        expected: tokType,
                        actual: nextToken,
                        previous: previousToken,
                        ruleName: this.getCurrRuleFullName()
                    });
                }
                throw this.SAVE_ERROR(new exceptions_public_1.MismatchedTokenException(msg, nextToken, previousToken));
            }
        }
        catch (eFromConsumption) {
            // no recovery allowed during backtracking, otherwise backtracking may recover invalid syntax and accept it
            // but the original syntax could have been parsed successfully without any backtracking + recovery
            if (this.recoveryEnabled &&
                // TODO: more robust checking of the exception type. Perhaps Typescript extending expressions?
                eFromConsumption.name === "MismatchedTokenException" &&
                !this.isBackTracking()) {
                var follows = this.getFollowsForInRuleRecovery(tokType, idx);
                try {
                    consumedToken = this.tryInRuleRecovery(tokType, follows);
                }
                catch (eFromInRuleRecovery) {
                    if (eFromInRuleRecovery.name === recoverable_1.IN_RULE_RECOVERY_EXCEPTION) {
                        // failed in RuleRecovery.
                        // throw the original error in order to trigger reSync error recovery
                        throw eFromConsumption;
                    }
                    else {
                        throw eFromInRuleRecovery;
                    }
                }
            }
            else {
                throw eFromConsumption;
            }
        }
        this.cstPostTerminal(options !== undefined && options.LABEL !== undefined
            ? options.LABEL
            : tokType.tokenName, consumedToken);
        return consumedToken;
    };
    RecognizerEngine.prototype.saveRecogState = function () {
        // errors is a getter which will clone the errors array
        var savedErrors = this.errors;
        var savedRuleStack = utils_1.cloneArr(this.RULE_STACK);
        return {
            errors: savedErrors,
            lexerState: this.exportLexerState(),
            RULE_STACK: savedRuleStack,
            CST_STACK: this.CST_STACK,
            LAST_EXPLICIT_RULE_STACK: this.LAST_EXPLICIT_RULE_STACK
        };
    };
    RecognizerEngine.prototype.reloadRecogState = function (newState) {
        this.errors = newState.errors;
        this.importLexerState(newState.lexerState);
        this.RULE_STACK = newState.RULE_STACK;
    };
    RecognizerEngine.prototype.ruleInvocationStateUpdate = function (shortName, fullName, idxInCallingRule) {
        this.RULE_OCCURRENCE_STACK.push(idxInCallingRule);
        this.RULE_STACK.push(shortName);
        // NOOP when cst is disabled
        this.cstInvocationStateUpdate(fullName, shortName);
    };
    RecognizerEngine.prototype.isBackTracking = function () {
        return !utils_1.isEmpty(this.isBackTrackingStack);
    };
    RecognizerEngine.prototype.getCurrRuleFullName = function () {
        var shortName = this.getLastExplicitRuleShortName();
        return this.shortRuleNameToFull.get(shortName);
    };
    RecognizerEngine.prototype.shortRuleNameToFullName = function (shortName) {
        return this.shortRuleNameToFull.get(shortName);
    };
    RecognizerEngine.prototype.isAtEndOfInput = function () {
        return this.tokenMatcher(this.LA(1), tokens_public_1.EOF);
    };
    RecognizerEngine.prototype.reset = function () {
        this.resetLexerState();
        this.isBackTrackingStack = [];
        this.errors = [];
        this.RULE_STACK = [];
        this.LAST_EXPLICIT_RULE_STACK = [];
        // TODO: extract a specific rest for TreeBuilder trait
        this.CST_STACK = [];
        this.RULE_OCCURRENCE_STACK = [];
    };
    return RecognizerEngine;
}());
exports.RecognizerEngine = RecognizerEngine;
//# sourceMappingURL=recognizer_engine.js.map