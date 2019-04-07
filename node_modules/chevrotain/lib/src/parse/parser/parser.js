"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var lang_extensions_1 = require("../../lang/lang_extensions");
var utils_1 = require("../../utils/utils");
var follow_1 = require("../grammar/follow");
var tokens_public_1 = require("../../scan/tokens_public");
var gast_builder_1 = require("../gast_builder");
var cst_1 = require("../cst/cst");
var errors_public_1 = require("../errors_public");
var gast_resolver_public_1 = require("../grammar/gast/gast_resolver_public");
var recoverable_1 = require("./traits/recoverable");
var looksahead_1 = require("./traits/looksahead");
var tree_builder_1 = require("./traits/tree_builder");
var lexer_adapter_1 = require("./traits/lexer_adapter");
var recognizer_api_1 = require("./traits/recognizer_api");
var recognizer_engine_1 = require("./traits/recognizer_engine");
var error_handler_1 = require("./traits/error_handler");
var context_assist_1 = require("./traits/context_assist");
exports.END_OF_FILE = tokens_public_1.createTokenInstance(tokens_public_1.EOF, "", NaN, NaN, NaN, NaN, NaN, NaN);
Object.freeze(exports.END_OF_FILE);
exports.DEFAULT_PARSER_CONFIG = Object.freeze({
    recoveryEnabled: false,
    maxLookahead: 4,
    ignoredIssues: {},
    dynamicTokensEnabled: false,
    outputCst: true,
    errorMessageProvider: errors_public_1.defaultParserErrorProvider,
    serializedGrammar: null
});
exports.DEFAULT_RULE_CONFIG = Object.freeze({
    recoveryValueFunc: function () { return undefined; },
    resyncEnabled: true
});
var ParserDefinitionErrorType;
(function (ParserDefinitionErrorType) {
    ParserDefinitionErrorType[ParserDefinitionErrorType["INVALID_RULE_NAME"] = 0] = "INVALID_RULE_NAME";
    ParserDefinitionErrorType[ParserDefinitionErrorType["DUPLICATE_RULE_NAME"] = 1] = "DUPLICATE_RULE_NAME";
    ParserDefinitionErrorType[ParserDefinitionErrorType["INVALID_RULE_OVERRIDE"] = 2] = "INVALID_RULE_OVERRIDE";
    ParserDefinitionErrorType[ParserDefinitionErrorType["DUPLICATE_PRODUCTIONS"] = 3] = "DUPLICATE_PRODUCTIONS";
    ParserDefinitionErrorType[ParserDefinitionErrorType["UNRESOLVED_SUBRULE_REF"] = 4] = "UNRESOLVED_SUBRULE_REF";
    ParserDefinitionErrorType[ParserDefinitionErrorType["LEFT_RECURSION"] = 5] = "LEFT_RECURSION";
    ParserDefinitionErrorType[ParserDefinitionErrorType["NONE_LAST_EMPTY_ALT"] = 6] = "NONE_LAST_EMPTY_ALT";
    ParserDefinitionErrorType[ParserDefinitionErrorType["AMBIGUOUS_ALTS"] = 7] = "AMBIGUOUS_ALTS";
    ParserDefinitionErrorType[ParserDefinitionErrorType["CONFLICT_TOKENS_RULES_NAMESPACE"] = 8] = "CONFLICT_TOKENS_RULES_NAMESPACE";
    ParserDefinitionErrorType[ParserDefinitionErrorType["INVALID_TOKEN_NAME"] = 9] = "INVALID_TOKEN_NAME";
    ParserDefinitionErrorType[ParserDefinitionErrorType["INVALID_NESTED_RULE_NAME"] = 10] = "INVALID_NESTED_RULE_NAME";
    ParserDefinitionErrorType[ParserDefinitionErrorType["DUPLICATE_NESTED_NAME"] = 11] = "DUPLICATE_NESTED_NAME";
    ParserDefinitionErrorType[ParserDefinitionErrorType["NO_NON_EMPTY_LOOKAHEAD"] = 12] = "NO_NON_EMPTY_LOOKAHEAD";
    ParserDefinitionErrorType[ParserDefinitionErrorType["AMBIGUOUS_PREFIX_ALTS"] = 13] = "AMBIGUOUS_PREFIX_ALTS";
    ParserDefinitionErrorType[ParserDefinitionErrorType["TOO_MANY_ALTS"] = 14] = "TOO_MANY_ALTS";
})(ParserDefinitionErrorType = exports.ParserDefinitionErrorType || (exports.ParserDefinitionErrorType = {}));
function EMPTY_ALT(value) {
    if (value === void 0) { value = undefined; }
    return function () {
        return value;
    };
}
exports.EMPTY_ALT = EMPTY_ALT;
var Parser = /** @class */ (function () {
    function Parser(tokenVocabulary, config) {
        if (config === void 0) { config = exports.DEFAULT_PARSER_CONFIG; }
        this.ignoredIssues = exports.DEFAULT_PARSER_CONFIG.ignoredIssues;
        this.definitionErrors = [];
        this.selfAnalysisDone = false;
        var that = this;
        that.initErrorHandler(config);
        that.initLexerAdapter();
        that.initLooksAhead(config);
        that.initRecognizerEngine(tokenVocabulary, config);
        that.initRecoverable(config);
        that.initTreeBuilder(config);
        that.initContentAssist();
        this.ignoredIssues = utils_1.has(config, "ignoredIssues")
            ? config.ignoredIssues
            : exports.DEFAULT_PARSER_CONFIG.ignoredIssues;
    }
    /**
     *  @deprecated use the **instance** method with the same name instead
     */
    Parser.performSelfAnalysis = function (parserInstance) {
        ;
        parserInstance.performSelfAnalysis();
    };
    Parser.prototype.performSelfAnalysis = function () {
        var _this = this;
        var defErrorsMsgs;
        this.selfAnalysisDone = true;
        var className = lang_extensions_1.classNameFromInstance(this);
        var productions = this.gastProductionsCache;
        if (this.serializedGrammar) {
            var rules = gast_builder_1.deserializeGrammar(this.serializedGrammar, this.tokensMap);
            utils_1.forEach(rules, function (rule) {
                _this.gastProductionsCache.put(rule.name, rule);
            });
        }
        var resolverErrors = gast_resolver_public_1.resolveGrammar({
            rules: productions.values()
        });
        this.definitionErrors.push.apply(this.definitionErrors, resolverErrors); // mutability for the win?
        // only perform additional grammar validations IFF no resolving errors have occurred.
        // as unresolved grammar may lead to unhandled runtime exceptions in the follow up validations.
        if (utils_1.isEmpty(resolverErrors)) {
            var validationErrors = gast_resolver_public_1.validateGrammar({
                rules: productions.values(),
                maxLookahead: this.maxLookahead,
                tokenTypes: utils_1.values(this.tokensMap),
                ignoredIssues: this.ignoredIssues,
                errMsgProvider: errors_public_1.defaultGrammarValidatorErrorProvider,
                grammarName: className
            });
            this.definitionErrors.push.apply(this.definitionErrors, validationErrors); // mutability for the win?
        }
        if (utils_1.isEmpty(this.definitionErrors)) {
            // this analysis may fail if the grammar is not perfectly valid
            var allFollows = follow_1.computeAllProdsFollows(productions.values());
            this.resyncFollows = allFollows;
        }
        var cstAnalysisResult = cst_1.analyzeCst(productions.values(), this.fullRuleNameToShort);
        this.allRuleNames = cstAnalysisResult.allRuleNames;
        if (!Parser.DEFER_DEFINITION_ERRORS_HANDLING &&
            !utils_1.isEmpty(this.definitionErrors)) {
            defErrorsMsgs = utils_1.map(this.definitionErrors, function (defError) { return defError.message; });
            throw new Error("Parser Definition Errors detected:\n " + defErrorsMsgs.join("\n-------------------------------\n"));
        }
    };
    // Set this flag to true if you don't want the Parser to throw error when problems in it's definition are detected.
    // (normally during the parser's constructor).
    // This is a design time flag, it will not affect the runtime error handling of the parser, just design time errors,
    // for example: duplicate rule names, referencing an unresolved subrule, ect...
    // This flag should not be enabled during normal usage, it is used in special situations, for example when
    // needing to display the parser definition errors in some GUI(online playground).
    Parser.DEFER_DEFINITION_ERRORS_HANDLING = false;
    return Parser;
}());
exports.Parser = Parser;
utils_1.applyMixins(Parser, [
    recoverable_1.Recoverable,
    looksahead_1.LooksAhead,
    tree_builder_1.TreeBuilder,
    lexer_adapter_1.LexerAdapter,
    recognizer_engine_1.RecognizerEngine,
    recognizer_api_1.RecognizerApi,
    error_handler_1.ErrorHandler,
    context_assist_1.ContentAssist
]);
//# sourceMappingURL=parser.js.map