"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var parser_1 = require("./parse/parser/parser");
var lexer_public_1 = require("./scan/lexer_public");
var tokens_public_1 = require("./scan/tokens_public");
var exceptions_public_1 = require("./parse/exceptions_public");
var version_1 = require("./version");
var errors_public_1 = require("./parse/errors_public");
var render_public_1 = require("./diagrams/render_public");
var gast_visitor_public_1 = require("./parse/grammar/gast/gast_visitor_public");
var gast_public_1 = require("./parse/grammar/gast/gast_public");
var gast_resolver_public_1 = require("./parse/grammar/gast/gast_resolver_public");
var generate_public_1 = require("./generate/generate_public");
var lexer_errors_public_1 = require("./scan/lexer_errors_public");
/**
 * defines the public API of
 * changes here may require major version change. (semVer)
 */
var API = {};
// semantic version
API.VERSION = version_1.VERSION;
// runtime API
API.Parser = parser_1.Parser;
// TypeCheck Multi Trait Parser API against official Chevrotain API
// The only thing this does not check is the constructor signature.
var mixedDummyInstance = null;
var officalDummyInstance = mixedDummyInstance;
API.ParserDefinitionErrorType = parser_1.ParserDefinitionErrorType;
API.Lexer = lexer_public_1.Lexer;
API.LexerDefinitionErrorType = lexer_public_1.LexerDefinitionErrorType;
API.EOF = tokens_public_1.EOF;
// Tokens utilities
API.tokenName = tokens_public_1.tokenName;
API.tokenLabel = tokens_public_1.tokenLabel;
API.tokenMatcher = tokens_public_1.tokenMatcher;
API.createToken = tokens_public_1.createToken;
API.createTokenInstance = tokens_public_1.createTokenInstance;
//
// // Other Utilities
API.EMPTY_ALT = parser_1.EMPTY_ALT;
API.defaultParserErrorProvider = errors_public_1.defaultParserErrorProvider;
API.isRecognitionException = exceptions_public_1.isRecognitionException;
API.EarlyExitException = exceptions_public_1.EarlyExitException;
API.MismatchedTokenException = exceptions_public_1.MismatchedTokenException;
API.NotAllInputParsedException = exceptions_public_1.NotAllInputParsedException;
API.NoViableAltException = exceptions_public_1.NoViableAltException;
API.defaultLexerErrorProvider = lexer_errors_public_1.defaultLexerErrorProvider;
//
// // grammar reflection API
API.Flat = gast_public_1.Flat;
API.Repetition = gast_public_1.Repetition;
API.RepetitionWithSeparator = gast_public_1.RepetitionWithSeparator;
API.RepetitionMandatory = gast_public_1.RepetitionMandatory;
API.RepetitionMandatoryWithSeparator = gast_public_1.RepetitionMandatoryWithSeparator;
API.Option = gast_public_1.Option;
API.Alternation = gast_public_1.Alternation;
API.NonTerminal = gast_public_1.NonTerminal;
API.Terminal = gast_public_1.Terminal;
API.Rule = gast_public_1.Rule;
// // GAST Utilities
API.GAstVisitor = gast_visitor_public_1.GAstVisitor;
API.serializeGrammar = gast_public_1.serializeGrammar;
API.serializeProduction = gast_public_1.serializeProduction;
API.resolveGrammar = gast_resolver_public_1.resolveGrammar;
API.defaultGrammarResolverErrorProvider = errors_public_1.defaultGrammarResolverErrorProvider;
API.validateGrammar = gast_resolver_public_1.validateGrammar;
API.defaultGrammarValidatorErrorProvider = errors_public_1.defaultGrammarValidatorErrorProvider;
API.assignOccurrenceIndices = gast_resolver_public_1.assignOccurrenceIndices;
/* istanbul ignore next */
API.clearCache = function () {
    console.warn("The clearCache function was 'soft' removed from the Chevrotain API." +
        "\n\t It performs no action other than printing this message." +
        "\n\t Please avoid using it as it will be completely removed in the future");
};
API.createSyntaxDiagramsCode = render_public_1.createSyntaxDiagramsCode;
API.generateParserFactory = generate_public_1.generateParserFactory;
API.generateParserModule = generate_public_1.generateParserModule;
module.exports = API;
//# sourceMappingURL=api.js.map