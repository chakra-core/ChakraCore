"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var tokens_public_1 = require("../scan/tokens_public");
var utils = require("../utils/utils");
var utils_1 = require("../utils/utils");
var gast_public_1 = require("./grammar/gast/gast_public");
var gast_1 = require("./grammar/gast/gast");
var checks_1 = require("./grammar/checks");
var version_1 = require("../version");
var parser_1 = require("./parser/parser");
exports.defaultParserErrorProvider = {
    buildMismatchTokenMessage: function (_a) {
        var expected = _a.expected, actual = _a.actual, previous = _a.previous, ruleName = _a.ruleName;
        var hasLabel = tokens_public_1.hasTokenLabel(expected);
        var expectedMsg = hasLabel
            ? "--> " + tokens_public_1.tokenLabel(expected) + " <--"
            : "token of type --> " + tokens_public_1.tokenName(expected) + " <--";
        var msg = "Expecting " + expectedMsg + " but found --> '" + actual.image + "' <--";
        return msg;
    },
    buildNotAllInputParsedMessage: function (_a) {
        var firstRedundant = _a.firstRedundant, ruleName = _a.ruleName;
        return ("Redundant input, expecting EOF but found: " + firstRedundant.image);
    },
    buildNoViableAltMessage: function (_a) {
        var expectedPathsPerAlt = _a.expectedPathsPerAlt, actual = _a.actual, previous = _a.previous, customUserDescription = _a.customUserDescription, ruleName = _a.ruleName;
        var errPrefix = "Expecting: ";
        // TODO: issue: No Viable Alternative Error may have incomplete details. #502
        var actualText = utils_1.first(actual).image;
        var errSuffix = "\nbut found: '" + actualText + "'";
        if (customUserDescription) {
            return errPrefix + customUserDescription + errSuffix;
        }
        else {
            var allLookAheadPaths = utils_1.reduce(expectedPathsPerAlt, function (result, currAltPaths) { return result.concat(currAltPaths); }, []);
            var nextValidTokenSequences = utils_1.map(allLookAheadPaths, function (currPath) {
                return "[" + utils_1.map(currPath, function (currTokenType) {
                    return tokens_public_1.tokenLabel(currTokenType);
                }).join(", ") + "]";
            });
            var nextValidSequenceItems = utils_1.map(nextValidTokenSequences, function (itemMsg, idx) { return "  " + (idx + 1) + ". " + itemMsg; });
            var calculatedDescription = "one of these possible Token sequences:\n" + nextValidSequenceItems.join("\n");
            return errPrefix + calculatedDescription + errSuffix;
        }
    },
    buildEarlyExitMessage: function (_a) {
        var expectedIterationPaths = _a.expectedIterationPaths, actual = _a.actual, customUserDescription = _a.customUserDescription, ruleName = _a.ruleName;
        var errPrefix = "Expecting: ";
        // TODO: issue: No Viable Alternative Error may have incomplete details. #502
        var actualText = utils_1.first(actual).image;
        var errSuffix = "\nbut found: '" + actualText + "'";
        if (customUserDescription) {
            return errPrefix + customUserDescription + errSuffix;
        }
        else {
            var nextValidTokenSequences = utils_1.map(expectedIterationPaths, function (currPath) {
                return "[" + utils_1.map(currPath, function (currTokenType) {
                    return tokens_public_1.tokenLabel(currTokenType);
                }).join(",") + "]";
            });
            var calculatedDescription = "expecting at least one iteration which starts with one of these possible Token sequences::\n  " +
                ("<" + nextValidTokenSequences.join(" ,") + ">");
            return errPrefix + calculatedDescription + errSuffix;
        }
    }
};
Object.freeze(exports.defaultParserErrorProvider);
exports.defaultGrammarResolverErrorProvider = {
    buildRuleNotFoundError: function (topLevelRule, undefinedRule) {
        var msg = "Invalid grammar, reference to a rule which is not defined: ->" +
            undefinedRule.nonTerminalName +
            "<-\n" +
            "inside top level rule: ->" +
            topLevelRule.name +
            "<-";
        return msg;
    }
};
exports.defaultGrammarValidatorErrorProvider = {
    buildDuplicateFoundError: function (topLevelRule, duplicateProds) {
        function getExtraProductionArgument(prod) {
            if (prod instanceof gast_public_1.Terminal) {
                return tokens_public_1.tokenName(prod.terminalType);
            }
            else if (prod instanceof gast_public_1.NonTerminal) {
                return prod.nonTerminalName;
            }
            else {
                return "";
            }
        }
        var topLevelName = topLevelRule.name;
        var duplicateProd = utils_1.first(duplicateProds);
        var index = duplicateProd.idx;
        var dslName = gast_1.getProductionDslName(duplicateProd);
        var extraArgument = getExtraProductionArgument(duplicateProd);
        var msg = "->" + dslName + "<- with numerical suffix: ->" + index + "<-\n                  " + (extraArgument ? "and argument: ->" + extraArgument + "<-" : "") + "\n                  appears more than once (" + duplicateProds.length + " times) in the top level rule: ->" + topLevelName + "<-.\n                  " + (index === 0
            ? "Also note that numerical suffix 0 means " + dslName + " without any suffix."
            : "") + "\n                  To fix this make sure each usage of " + dslName + " " + (extraArgument ? "with the argument: ->" + extraArgument + "<-" : "") + "\n                  in the rule ->" + topLevelName + "<- has a different occurrence index (0-5), as that combination acts as a unique\n                  position key in the grammar, which is needed by the parsing engine.\n                  \n                  For further details see: https://sap.github.io/chevrotain/docs/FAQ.html#NUMERICAL_SUFFIXES \n                  ";
        // white space trimming time! better to trim afterwards as it allows to use WELL formatted multi line template strings...
        msg = msg.replace(/[ \t]+/g, " ");
        msg = msg.replace(/\s\s+/g, "\n");
        return msg;
    },
    buildInvalidNestedRuleNameError: function (topLevelRule, nestedProd) {
        var msg = "Invalid nested rule name: ->" + nestedProd.name + "<- inside rule: ->" + topLevelRule.name + "<-\n" +
            ("it must match the pattern: ->" + checks_1.validNestedRuleName.toString() + "<-.\n") +
            "Note that this means a nested rule name must start with the '$'(dollar) sign.";
        return msg;
    },
    buildDuplicateNestedRuleNameError: function (topLevelRule, nestedProd) {
        var duplicateName = utils_1.first(nestedProd).name;
        var errMsg = "Duplicate nested rule name: ->" + duplicateName + "<- inside rule: ->" + topLevelRule.name + "<-\n" +
            "A nested name must be unique in the scope of a top level grammar rule.";
        return errMsg;
    },
    buildNamespaceConflictError: function (rule) {
        var errMsg = "Namespace conflict found in grammar.\n" +
            ("The grammar has both a Terminal(Token) and a Non-Terminal(Rule) named: <" + rule.name + ">.\n") +
            "To resolve this make sure each Terminal and Non-Terminal names are unique\n" +
            "This is easy to accomplish by using the convention that Terminal names start with an uppercase letter\n" +
            "and Non-Terminal names start with a lower case letter.";
        return errMsg;
    },
    buildAlternationPrefixAmbiguityError: function (options) {
        var pathMsg = utils_1.map(options.prefixPath, function (currTok) {
            return tokens_public_1.tokenLabel(currTok);
        }).join(", ");
        var occurrence = options.alternation.idx === 0 ? "" : options.alternation.idx;
        var errMsg = "Ambiguous alternatives: <" + options.ambiguityIndices.join(" ,") + "> due to common lookahead prefix\n" +
            ("in <OR" + occurrence + "> inside <" + options.topLevelRule.name + "> Rule,\n") +
            ("<" + pathMsg + "> may appears as a prefix path in all these alternatives.\n") +
            "https://sap.github.io/chevrotain/docs/guide/resolving_grammar_errors.html#COMMON_PREFIX\n" +
            "For Further details.";
        return errMsg;
    },
    buildAlternationAmbiguityError: function (options) {
        var pathMsg = utils_1.map(options.prefixPath, function (currtok) {
            return tokens_public_1.tokenLabel(currtok);
        }).join(", ");
        var occurrence = options.alternation.idx === 0 ? "" : options.alternation.idx;
        var currMessage = "Ambiguous alternatives: <" + options.ambiguityIndices.join(" ,") + "> in <OR" + occurrence + ">" +
            (" inside <" + options.topLevelRule.name + "> Rule,\n") +
            ("<" + pathMsg + "> may appears as a prefix path in all these alternatives.\n");
        var docs_version = version_1.VERSION.replace(/\./g, "_");
        // Should this information be on the error message or in some common errors docs?
        currMessage =
            currMessage +
                "To Resolve this, try one of of the following: \n" +
                ("1. Refactor your grammar to be LL(K) for the current value of k (by default k=" + parser_1.DEFAULT_PARSER_CONFIG.maxLookahead + "})\n") +
                "2. Increase the value of K for your grammar by providing a larger 'maxLookahead' value in the parser's config\n" +
                "3. This issue can be ignored (if you know what you are doing...), see" +
                " https://sap.github.io/chevrotain/documentation/" +
                docs_version +
                "/interfaces/iparserconfig.html#ignoredissues for more" +
                " details\n";
        return currMessage;
    },
    buildEmptyRepetitionError: function (options) {
        var dslName = gast_1.getProductionDslName(options.repetition);
        if (options.repetition.idx !== 0) {
            dslName += options.repetition.idx;
        }
        var errMsg = "The repetition <" + dslName + "> within Rule <" + options.topLevelRule.name + "> can never consume any tokens.\n" +
            "This could lead to an infinite loop.";
        return errMsg;
    },
    buildTokenNameError: function (options) {
        var tokTypeName = tokens_public_1.tokenName(options.tokenType);
        var errMsg = "Invalid Grammar Token name: ->" + tokTypeName + "<- it must match the pattern: ->" + options.expectedPattern.toString() + "<-";
        return errMsg;
    },
    buildEmptyAlternationError: function (options) {
        var errMsg = "Ambiguous empty alternative: <" + (options.emptyChoiceIdx + 1) + ">" +
            (" in <OR" + options.alternation.idx + "> inside <" + options.topLevelRule.name + "> Rule.\n") +
            "Only the last alternative may be an empty alternative.";
        return errMsg;
    },
    buildTooManyAlternativesError: function (options) {
        var errMsg = "An Alternation cannot have more than 256 alternatives:\n" +
            ("<OR" + options.alternation.idx + "> inside <" + options.topLevelRule.name + "> Rule.\n has " + (options.alternation.definition.length +
                1) + " alternatives.");
        return errMsg;
    },
    buildLeftRecursionError: function (options) {
        var ruleName = options.topLevelRule.name;
        var pathNames = utils.map(options.leftRecursionPath, function (currRule) { return currRule.name; });
        var leftRecursivePath = ruleName + " --> " + pathNames
            .concat([ruleName])
            .join(" --> ");
        var errMsg = "Left Recursion found in grammar.\n" +
            ("rule: <" + ruleName + "> can be invoked from itself (directly or indirectly)\n") +
            ("without consuming any Tokens. The grammar path that causes this is: \n " + leftRecursivePath + "\n") +
            " To fix this refactor your grammar to remove the left recursion.\n" +
            "see: https://en.wikipedia.org/wiki/LL_parser#Left_Factoring.";
        return errMsg;
    },
    buildInvalidRuleNameError: function (options) {
        var ruleName = options.topLevelRule.name;
        var expectedPatternString = options.expectedPattern.toString();
        var errMsg = "Invalid grammar rule name: ->" + ruleName + "<- it must match the pattern: ->" + expectedPatternString + "<-";
        return errMsg;
    },
    buildDuplicateRuleNameError: function (options) {
        var ruleName;
        if (options.topLevelRule instanceof gast_public_1.Rule) {
            ruleName = options.topLevelRule.name;
        }
        else {
            ruleName = options.topLevelRule;
        }
        var errMsg = "Duplicate definition, rule: ->" + ruleName + "<- is already defined in the grammar: ->" + options.grammarName + "<-";
        return errMsg;
    }
};
//# sourceMappingURL=errors_public.js.map