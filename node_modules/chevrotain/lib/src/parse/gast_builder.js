"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var range_1 = require("../text/range");
var utils_1 = require("../utils/utils");
var gast_public_1 = require("./grammar/gast/gast_public");
var ProdType;
(function (ProdType) {
    ProdType[ProdType["OPTION"] = 0] = "OPTION";
    ProdType[ProdType["OR"] = 1] = "OR";
    ProdType[ProdType["MANY"] = 2] = "MANY";
    ProdType[ProdType["MANY_SEP"] = 3] = "MANY_SEP";
    ProdType[ProdType["AT_LEAST_ONE"] = 4] = "AT_LEAST_ONE";
    ProdType[ProdType["AT_LEAST_ONE_SEP"] = 5] = "AT_LEAST_ONE_SEP";
    ProdType[ProdType["REF"] = 6] = "REF";
    ProdType[ProdType["TERMINAL"] = 7] = "TERMINAL";
    ProdType[ProdType["FLAT"] = 8] = "FLAT";
})(ProdType = exports.ProdType || (exports.ProdType = {}));
var namePropRegExp = /(?:\s*{\s*NAME\s*:\s*["'`]([\w$]*)["'`])?/;
var namePropRegExpNoCurlyFirstOfTwo = new RegExp(namePropRegExp.source
    // remove opening curly brackets
    .replace("{", "")
    // add the comma between the NAME prop and the following prop
    .replace(")?", "\\s*,)?"));
var terminalRegEx = /\.\s*CONSUME(\d+)?\s*\(\s*(?:[a-zA-Z_$]\w*\s*\.\s*)*([a-zA-Z_$]\w*)/;
var terminalRegGlobal = new RegExp(terminalRegEx.source, "g");
var refRegEx = /\.\s*SUBRULE(\d+)?\s*\(\s*(?:[a-zA-Z_$]\w*\s*\.\s*)*([a-zA-Z_$]\w*)/;
var refRegExGlobal = new RegExp(refRegEx.source, "g");
var optionPrefixRegEx = /\.\s*OPTION(\d+)?\s*\(/;
var optionRegEx = new RegExp(optionPrefixRegEx.source + namePropRegExp.source);
var optionRegExGlobal = new RegExp(optionPrefixRegEx.source, "g");
var manyPrefixRegEx = /\.\s*MANY(\d+)?\s*\(/;
var manyRegEx = new RegExp(manyPrefixRegEx.source + namePropRegExp.source);
var manyRegExGlobal = new RegExp(manyPrefixRegEx.source, "g");
var sepPropRegEx = /\s*SEP\s*:\s*(?:[a-zA-Z_$]\w*\s*\.\s*)*([a-zA-Z_$]\w*)/;
var manySepPrefixRegEx = /\.\s*MANY_SEP(\d+)?\s*\(\s*{/;
var manyWithSeparatorRegEx = new RegExp(manySepPrefixRegEx.source +
    namePropRegExpNoCurlyFirstOfTwo.source +
    sepPropRegEx.source);
var manyWithSeparatorRegExGlobal = new RegExp(manyWithSeparatorRegEx.source, "g");
var atLeastOneSepPrefixRegEx = /\.\s*AT_LEAST_ONE_SEP(\d+)?\s*\(\s*{/;
var atLeastOneWithSeparatorRegEx = new RegExp(atLeastOneSepPrefixRegEx.source +
    namePropRegExpNoCurlyFirstOfTwo.source +
    sepPropRegEx.source);
var atLeastOneWithSeparatorRegExGlobal = new RegExp(atLeastOneWithSeparatorRegEx.source, "g");
var atLeastOnePrefixRegEx = /\.\s*AT_LEAST_ONE(\d+)?\s*\(/;
var atLeastOneRegEx = new RegExp(atLeastOnePrefixRegEx.source + namePropRegExp.source);
var atLeastOneRegExGlobal = new RegExp(atLeastOnePrefixRegEx.source, "g");
var orPrefixRegEx = /\.\s*OR(\d+)?\s*\(/;
var orRegEx = new RegExp(orPrefixRegEx.source + namePropRegExp.source);
var orRegExGlobal = new RegExp(orPrefixRegEx.source, "g");
var orPartSuffixRegEx = /\s*(ALT)\s*:/;
var orPartRegEx = new RegExp(namePropRegExpNoCurlyFirstOfTwo.source + orPartSuffixRegEx.source);
var orPartRegExGlobal = new RegExp(orPartRegEx.source, "g");
exports.terminalNameToConstructor = {};
function buildTopProduction(impelText, name, terminals) {
    // pseudo state. so little state does not yet mandate the complexity of wrapping in a class...
    // TODO: this is confusing, might be time to create a class..
    exports.terminalNameToConstructor = terminals;
    // the top most range must strictly contain all the other ranges
    // which is why we prefix the text with " " (curr Range impel is only for positive ranges)
    var spacedImpelText = " " + impelText;
    // TODO: why do we add whitespace twice?
    var txtWithoutComments = removeComments(" " + spacedImpelText);
    var textWithoutCommentsAndStrings = removeStringLiterals(txtWithoutComments);
    var prodRanges = createRanges(textWithoutCommentsAndStrings);
    var topRange = new range_1.Range(0, impelText.length + 2);
    var topRule = buildTopLevel(name, topRange, prodRanges, impelText);
    return topRule;
}
exports.buildTopProduction = buildTopProduction;
function buildTopLevel(name, topRange, allRanges, orgText) {
    var topLevelProd = new gast_public_1.Rule({
        name: name,
        definition: [],
        orgText: orgText
    });
    return buildAbstractProd(topLevelProd, topRange, allRanges);
}
function buildProdGast(prodRange, allRanges) {
    switch (prodRange.type) {
        case ProdType.AT_LEAST_ONE:
            return buildAtLeastOneProd(prodRange, allRanges);
        case ProdType.AT_LEAST_ONE_SEP:
            return buildAtLeastOneSepProd(prodRange, allRanges);
        case ProdType.MANY_SEP:
            return buildManySepProd(prodRange, allRanges);
        case ProdType.MANY:
            return buildManyProd(prodRange, allRanges);
        case ProdType.OPTION:
            return buildOptionProd(prodRange, allRanges);
        case ProdType.OR:
            return buildOrProd(prodRange, allRanges);
        case ProdType.FLAT:
            return buildFlatProd(prodRange, allRanges);
        case ProdType.REF:
            return buildRefProd(prodRange);
        case ProdType.TERMINAL:
            return buildTerminalProd(prodRange);
        /* istanbul ignore next */
        default:
            throw Error("non exhaustive match");
    }
}
exports.buildProdGast = buildProdGast;
function buildRefProd(prodRange) {
    var reResult = refRegEx.exec(prodRange.text);
    var isImplicitOccurrenceIdx = reResult[1] === undefined;
    var refOccurrence = isImplicitOccurrenceIdx ? 0 : parseInt(reResult[1], 10);
    var refProdName = reResult[2];
    var newRef = new gast_public_1.NonTerminal({
        nonTerminalName: refProdName,
        idx: refOccurrence
    });
    return newRef;
}
function buildTerminalProd(prodRange) {
    var reResult = terminalRegEx.exec(prodRange.text);
    var isImplicitOccurrenceIdx = reResult[1] === undefined;
    var terminalOccurrence = isImplicitOccurrenceIdx
        ? 0
        : parseInt(reResult[1], 10);
    var terminalName = reResult[2];
    var terminalType = exports.terminalNameToConstructor[terminalName];
    if (!terminalType) {
        throw Error("Terminal Token name: " +
            terminalName +
            " not found\n" +
            "\tSee: https://sap.github.io/chevrotain/docs/guide/resolving_grammar_errors.html#TERMINAL_NAME_NOT_FOUND\n" +
            "\tFor Further details.");
    }
    var newTerminal = new gast_public_1.Terminal({
        terminalType: terminalType,
        idx: terminalOccurrence
    });
    return newTerminal;
}
function buildProdWithOccurrence(regEx, prodInstance, prodRange, allRanges) {
    var reResult = regEx.exec(prodRange.text);
    var isImplicitOccurrenceIdx = reResult[1] === undefined;
    prodInstance.idx = isImplicitOccurrenceIdx ? 0 : parseInt(reResult[1], 10);
    var nestedName = reResult[2];
    if (!utils_1.isUndefined(nestedName)) {
        ;
        prodInstance.name = nestedName;
    }
    return buildAbstractProd(prodInstance, prodRange.range, allRanges);
}
function buildAtLeastOneProd(prodRange, allRanges) {
    return buildProdWithOccurrence(atLeastOneRegEx, new gast_public_1.RepetitionMandatory({ definition: [] }), prodRange, allRanges);
}
function buildAtLeastOneSepProd(prodRange, allRanges) {
    return buildRepetitionWithSep(prodRange, allRanges, gast_public_1.RepetitionMandatoryWithSeparator, atLeastOneWithSeparatorRegEx);
}
function buildManyProd(prodRange, allRanges) {
    return buildProdWithOccurrence(manyRegEx, new gast_public_1.Repetition({ definition: [] }), prodRange, allRanges);
}
function buildManySepProd(prodRange, allRanges) {
    return buildRepetitionWithSep(prodRange, allRanges, gast_public_1.RepetitionWithSeparator, manyWithSeparatorRegEx);
}
function buildRepetitionWithSep(prodRange, allRanges, repConstructor, regExp) {
    var reResult = regExp.exec(prodRange.text);
    var isImplicitOccurrenceIdx = reResult[1] === undefined;
    var occurrenceIdx = isImplicitOccurrenceIdx ? 0 : parseInt(reResult[1], 10);
    var sepName = reResult[3];
    var separatorType = exports.terminalNameToConstructor[sepName];
    if (!separatorType) {
        throw Error("Separator Terminal Token name: " + sepName + " not found");
    }
    var repetitionInstance = new repConstructor({
        definition: [],
        separator: separatorType,
        idx: occurrenceIdx
    });
    var nestedName = reResult[2];
    if (!utils_1.isUndefined(nestedName)) {
        ;
        repetitionInstance.name = nestedName;
    }
    return (buildAbstractProd(repetitionInstance, prodRange.range, allRanges));
}
function buildOptionProd(prodRange, allRanges) {
    return buildProdWithOccurrence(optionRegEx, new gast_public_1.Option({ definition: [] }), prodRange, allRanges);
}
function buildOrProd(prodRange, allRanges) {
    return buildProdWithOccurrence(orRegEx, new gast_public_1.Alternation({ definition: [] }), prodRange, allRanges);
}
function buildFlatProd(prodRange, allRanges) {
    var prodInstance = new gast_public_1.Flat({ definition: [] });
    var reResult = orPartRegEx.exec(prodRange.text);
    var nestedName = reResult[1];
    if (!utils_1.isUndefined(nestedName)) {
        ;
        prodInstance.name = nestedName;
    }
    return buildAbstractProd(prodInstance, prodRange.range, allRanges);
}
function buildAbstractProd(prod, topLevelRange, allRanges) {
    var secondLevelProds = getDirectlyContainedRanges(topLevelRange, allRanges);
    var secondLevelInOrder = utils_1.sortBy(secondLevelProds, function (prodRng) {
        return prodRng.range.start;
    });
    var definition = [];
    utils_1.forEach(secondLevelInOrder, function (prodRng) {
        definition.push(buildProdGast(prodRng, allRanges));
    });
    prod.definition = definition;
    return prod;
}
function getDirectlyContainedRanges(y, prodRanges) {
    return utils_1.filter(prodRanges, function (x) {
        var isXDescendantOfY = y.strictlyContainsRange(x.range);
        var xDoesNotHaveAnyAncestorWhichIsDecendantOfY = utils_1.every(prodRanges, function (maybeAnotherParent) {
            var isParentOfX = maybeAnotherParent.range.strictlyContainsRange(x.range);
            var isChildOfY = maybeAnotherParent.range.isStrictlyContainedInRange(y);
            return !(isParentOfX && isChildOfY);
        });
        return isXDescendantOfY && xDoesNotHaveAnyAncestorWhichIsDecendantOfY;
    });
}
exports.getDirectlyContainedRanges = getDirectlyContainedRanges;
var singleLineCommentRegEx = /\/\/.*/g;
var multiLineCommentRegEx = /\/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+\//g;
var doubleQuoteStringLiteralRegEx = /(NAME\s*:\s*)?"([^\\"]|\\([bfnrtv"\\/]|u[0-9a-fA-F]{4}))*"/g;
var singleQuoteStringLiteralRegEx = /(NAME\s*:\s*)?'([^\\']|\\([bfnrtv'\\/]|u[0-9a-fA-F]{4}))*'/g;
function removeComments(text) {
    var noSingleLine = text.replace(singleLineCommentRegEx, "");
    var noComments = noSingleLine.replace(multiLineCommentRegEx, "");
    return noComments;
}
exports.removeComments = removeComments;
function replaceWithEmptyStringExceptNestedRules(match, nestedRuleGroup) {
    // do not replace with empty string if a nest rule (NAME:"bamba") was detected
    if (nestedRuleGroup !== undefined) {
        return match;
    }
    return "";
}
function removeStringLiterals(text) {
    var noDoubleQuotes = text.replace(doubleQuoteStringLiteralRegEx, replaceWithEmptyStringExceptNestedRules);
    var noSingleQuotes = noDoubleQuotes.replace(singleQuoteStringLiteralRegEx, replaceWithEmptyStringExceptNestedRules);
    return noSingleQuotes;
}
exports.removeStringLiterals = removeStringLiterals;
function createRanges(text) {
    var terminalRanges = createTerminalRanges(text);
    var refsRanges = createRefsRanges(text);
    var atLeastOneRanges = createAtLeastOneRanges(text);
    var atLeastOneSepRanges = createAtLeastOneSepRanges(text);
    var manyRanges = createManyRanges(text);
    var manySepRanges = createManySepRanges(text);
    var optionRanges = createOptionRanges(text);
    var orRanges = createOrRanges(text);
    return [].concat(terminalRanges, refsRanges, atLeastOneRanges, atLeastOneSepRanges, manyRanges, manySepRanges, optionRanges, orRanges);
}
exports.createRanges = createRanges;
function createTerminalRanges(text) {
    return createRefOrTerminalProdRangeInternal(text, ProdType.TERMINAL, terminalRegGlobal);
}
exports.createTerminalRanges = createTerminalRanges;
function createRefsRanges(text) {
    return createRefOrTerminalProdRangeInternal(text, ProdType.REF, refRegExGlobal);
}
exports.createRefsRanges = createRefsRanges;
function createAtLeastOneRanges(text) {
    return createOperatorProdRangeParenthesis(text, ProdType.AT_LEAST_ONE, atLeastOneRegExGlobal);
}
exports.createAtLeastOneRanges = createAtLeastOneRanges;
function createAtLeastOneSepRanges(text) {
    return createOperatorProdRangeParenthesis(text, ProdType.AT_LEAST_ONE_SEP, atLeastOneWithSeparatorRegExGlobal);
}
exports.createAtLeastOneSepRanges = createAtLeastOneSepRanges;
function createManyRanges(text) {
    return createOperatorProdRangeParenthesis(text, ProdType.MANY, manyRegExGlobal);
}
exports.createManyRanges = createManyRanges;
function createManySepRanges(text) {
    return createOperatorProdRangeParenthesis(text, ProdType.MANY_SEP, manyWithSeparatorRegExGlobal);
}
exports.createManySepRanges = createManySepRanges;
function createOptionRanges(text) {
    return createOperatorProdRangeParenthesis(text, ProdType.OPTION, optionRegExGlobal);
}
exports.createOptionRanges = createOptionRanges;
function createOrRanges(text) {
    var orRanges = createOperatorProdRangeParenthesis(text, ProdType.OR, orRegExGlobal);
    // have to split up the OR cases into separate FLAT productions
    // (A |BB | CDE) ==> or.def[0] --> FLAT(A) , or.def[1] --> FLAT(BB) , or.def[2] --> FLAT(CCDE)
    var orSubPartsRanges = createOrPartRanges(orRanges);
    return orRanges.concat(orSubPartsRanges);
}
exports.createOrRanges = createOrRanges;
var findClosingCurly = (utils_1.partial(findClosingOffset, "{", "}"));
var findClosingParen = (utils_1.partial(findClosingOffset, "(", ")"));
function createOrPartRanges(orRanges) {
    var orPartRanges = [];
    utils_1.forEach(orRanges, function (orRange) {
        var currOrParts = createOperatorProdRangeInternal(orRange.text, ProdType.FLAT, orPartRegExGlobal, findClosingCurly);
        var currOrRangeStart = orRange.range.start;
        // fix offsets as we are working on a subset of the text
        utils_1.forEach(currOrParts, function (orPart) {
            orPart.range.start += currOrRangeStart;
            orPart.range.end += currOrRangeStart;
        });
        orPartRanges = orPartRanges.concat(currOrParts);
    });
    var uniqueOrPartRanges = utils_1.uniq(orPartRanges, function (prodRange) {
        // using "~" as a separator for the identify function as its not a valid char in javascript
        return (prodRange.type +
            "~" +
            prodRange.range.start +
            "~" +
            prodRange.range.end +
            "~" +
            prodRange.text);
    });
    return uniqueOrPartRanges;
}
exports.createOrPartRanges = createOrPartRanges;
function createRefOrTerminalProdRangeInternal(text, prodType, pattern) {
    var prodRanges = [];
    var matched;
    while ((matched = pattern.exec(text))) {
        var start = matched.index;
        var stop_1 = pattern.lastIndex;
        var currRange = new range_1.Range(start, stop_1);
        var currText = matched[0];
        prodRanges.push({
            range: currRange,
            text: currText,
            type: prodType
        });
    }
    return prodRanges;
}
function createOperatorProdRangeParenthesis(text, prodType, pattern) {
    return createOperatorProdRangeInternal(text, prodType, pattern, findClosingParen);
}
function createOperatorProdRangeInternal(text, prodType, pattern, findTerminatorOffSet) {
    var operatorRanges = [];
    var matched;
    while ((matched = pattern.exec(text))) {
        var start = matched.index;
        // note that (start + matched[0].length) is the first character AFTER the match
        var stop_2 = findTerminatorOffSet(start + matched[0].length, text);
        var currRange = new range_1.Range(start, stop_2);
        var currText = text.substr(start, stop_2 - start + 1);
        operatorRanges.push({
            range: currRange,
            text: currText,
            type: prodType
        });
    }
    return operatorRanges;
}
function findClosingOffset(opening, closing, start, text) {
    var parenthesisStack = [1];
    var i = -1;
    while (!utils_1.isEmpty(parenthesisStack) && i + start < text.length) {
        i++;
        var nextChar = text.charAt(start + i);
        if (nextChar === opening) {
            parenthesisStack.push(1);
        }
        else if (nextChar === closing) {
            parenthesisStack.pop();
        }
    }
    // valid termination of the search loop
    if (utils_1.isEmpty(parenthesisStack)) {
        return i + start;
    }
    else {
        throw new Error("INVALID INPUT TEXT, UNTERMINATED PARENTHESIS");
    }
}
exports.findClosingOffset = findClosingOffset;
function deserializeGrammar(grammar, terminals) {
    return utils_1.map(grammar, function (production) {
        return deserializeProduction(production, terminals);
    });
}
exports.deserializeGrammar = deserializeGrammar;
function deserializeProduction(node, terminals) {
    switch (node.type) {
        case "NonTerminal":
            return new gast_public_1.NonTerminal({
                nonTerminalName: node.name,
                idx: node.idx
            });
        case "Flat":
            return new gast_public_1.Flat({
                name: node.name,
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "Option":
            return new gast_public_1.Option({
                name: node.name,
                idx: node.idx,
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "RepetitionMandatory":
            return new gast_public_1.RepetitionMandatory({
                name: node.name,
                idx: node.idx,
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "RepetitionMandatoryWithSeparator":
            return new gast_public_1.RepetitionMandatoryWithSeparator({
                name: node.name,
                idx: node.idx,
                separator: terminals[node.separator.name],
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "RepetitionWithSeparator":
            return new gast_public_1.RepetitionWithSeparator({
                name: node.name,
                idx: node.idx,
                separator: terminals[node.separator.name],
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "Repetition":
            return new gast_public_1.Repetition({
                name: node.name,
                idx: node.idx,
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "Alternation":
            return new gast_public_1.Alternation({
                name: node.name,
                idx: node.idx,
                definition: deserializeGrammar(node.definition, terminals)
            });
        case "Terminal":
            return new gast_public_1.Terminal({
                terminalType: terminals[node.name],
                idx: node.idx
            });
        case "Rule":
            return new gast_public_1.Rule({
                name: node.name,
                orgText: node.orgText,
                definition: deserializeGrammar(node.definition, terminals)
            });
        /* istanbul ignore next */
        default:
            var _never = node;
    }
}
exports.deserializeProduction = deserializeProduction;
//# sourceMappingURL=gast_builder.js.map