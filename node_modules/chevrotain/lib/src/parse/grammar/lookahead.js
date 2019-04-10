"use strict";
var __extends = (this && this.__extends) || (function () {
    var extendStatics = function (d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };
    return function (d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
var utils_1 = require("../../utils/utils");
var interpreter_1 = require("./interpreter");
var rest_1 = require("./rest");
var tokens_1 = require("../../scan/tokens");
var gast_public_1 = require("./gast/gast_public");
var gast_visitor_public_1 = require("./gast/gast_visitor_public");
var PROD_TYPE;
(function (PROD_TYPE) {
    PROD_TYPE[PROD_TYPE["OPTION"] = 0] = "OPTION";
    PROD_TYPE[PROD_TYPE["REPETITION"] = 1] = "REPETITION";
    PROD_TYPE[PROD_TYPE["REPETITION_MANDATORY"] = 2] = "REPETITION_MANDATORY";
    PROD_TYPE[PROD_TYPE["REPETITION_MANDATORY_WITH_SEPARATOR"] = 3] = "REPETITION_MANDATORY_WITH_SEPARATOR";
    PROD_TYPE[PROD_TYPE["REPETITION_WITH_SEPARATOR"] = 4] = "REPETITION_WITH_SEPARATOR";
    PROD_TYPE[PROD_TYPE["ALTERNATION"] = 5] = "ALTERNATION";
})(PROD_TYPE = exports.PROD_TYPE || (exports.PROD_TYPE = {}));
function getProdType(prod) {
    /* istanbul ignore else */
    if (prod instanceof gast_public_1.Option) {
        return PROD_TYPE.OPTION;
    }
    else if (prod instanceof gast_public_1.Repetition) {
        return PROD_TYPE.REPETITION;
    }
    else if (prod instanceof gast_public_1.RepetitionMandatory) {
        return PROD_TYPE.REPETITION_MANDATORY;
    }
    else if (prod instanceof gast_public_1.RepetitionMandatoryWithSeparator) {
        return PROD_TYPE.REPETITION_MANDATORY_WITH_SEPARATOR;
    }
    else if (prod instanceof gast_public_1.RepetitionWithSeparator) {
        return PROD_TYPE.REPETITION_WITH_SEPARATOR;
    }
    else if (prod instanceof gast_public_1.Alternation) {
        return PROD_TYPE.ALTERNATION;
    }
    else {
        throw Error("non exhaustive match");
    }
}
exports.getProdType = getProdType;
function buildLookaheadFuncForOr(occurrence, ruleGrammar, k, hasPredicates, dynamicTokensEnabled, laFuncBuilder) {
    var lookAheadPaths = getLookaheadPathsForOr(occurrence, ruleGrammar, k);
    var tokenMatcher = areTokenCategoriesNotUsed(lookAheadPaths)
        ? tokens_1.tokenStructuredMatcherNoCategories
        : tokens_1.tokenStructuredMatcher;
    return laFuncBuilder(lookAheadPaths, hasPredicates, tokenMatcher, dynamicTokensEnabled);
}
exports.buildLookaheadFuncForOr = buildLookaheadFuncForOr;
/**
 *  When dealing with an Optional production (OPTION/MANY/2nd iteration of AT_LEAST_ONE/...) we need to compare
 *  the lookahead "inside" the production and the lookahead immediately "after" it in the same top level rule (context free).
 *
 *  Example: given a production:
 *  ABC(DE)?DF
 *
 *  The optional '(DE)?' should only be entered if we see 'DE'. a single Token 'D' is not sufficient to distinguish between the two
 *  alternatives.
 *
 *  @returns A Lookahead function which will return true IFF the parser should parse the Optional production.
 */
function buildLookaheadFuncForOptionalProd(occurrence, ruleGrammar, k, dynamicTokensEnabled, prodType, lookaheadBuilder) {
    var lookAheadPaths = getLookaheadPathsForOptionalProd(occurrence, ruleGrammar, prodType, k);
    var tokenMatcher = areTokenCategoriesNotUsed(lookAheadPaths)
        ? tokens_1.tokenStructuredMatcherNoCategories
        : tokens_1.tokenStructuredMatcher;
    return lookaheadBuilder(lookAheadPaths[0], tokenMatcher, dynamicTokensEnabled);
}
exports.buildLookaheadFuncForOptionalProd = buildLookaheadFuncForOptionalProd;
function buildAlternativesLookAheadFunc(alts, hasPredicates, tokenMatcher, dynamicTokensEnabled) {
    var numOfAlts = alts.length;
    var areAllOneTokenLookahead = utils_1.every(alts, function (currAlt) {
        return utils_1.every(currAlt, function (currPath) {
            return currPath.length === 1;
        });
    });
    // This version takes into account the predicates as well.
    if (hasPredicates) {
        /**
         * @returns {number} - The chosen alternative index
         */
        return function (orAlts) {
            // unfortunately the predicates must be extracted every single time
            // as they cannot be cached due to references to parameters(vars) which are no longer valid.
            // note that in the common case of no predicates, no cpu time will be wasted on this (see else block)
            var predicates = utils_1.map(orAlts, function (currAlt) { return currAlt.GATE; });
            for (var t = 0; t < numOfAlts; t++) {
                var currAlt = alts[t];
                var currNumOfPaths = currAlt.length;
                var currPredicate = predicates[t];
                if (currPredicate !== undefined &&
                    currPredicate.call(this) === false) {
                    // if the predicate does not match there is no point in checking the paths
                    continue;
                }
                nextPath: for (var j = 0; j < currNumOfPaths; j++) {
                    var currPath = currAlt[j];
                    var currPathLength = currPath.length;
                    for (var i = 0; i < currPathLength; i++) {
                        var nextToken = this.LA(i + 1);
                        if (tokenMatcher(nextToken, currPath[i]) === false) {
                            // mismatch in current path
                            // try the next pth
                            continue nextPath;
                        }
                    }
                    // found a full path that matches.
                    // this will also work for an empty ALT as the loop will be skipped
                    return t;
                }
                // none of the paths for the current alternative matched
                // try the next alternative
            }
            // none of the alternatives could be matched
            return undefined;
        };
    }
    else if (areAllOneTokenLookahead && !dynamicTokensEnabled) {
        // optimized (common) case of all the lookaheads paths requiring only
        // a single token lookahead. These Optimizations cannot work if dynamically defined Tokens are used.
        var singleTokenAlts = utils_1.map(alts, function (currAlt) {
            return utils_1.flatten(currAlt);
        });
        var choiceToAlt_1 = utils_1.reduce(singleTokenAlts, function (result, currAlt, idx) {
            utils_1.forEach(currAlt, function (currTokType) {
                if (!utils_1.has(result, currTokType.tokenTypeIdx)) {
                    result[currTokType.tokenTypeIdx] = idx;
                }
                utils_1.forEach(currTokType.categoryMatches, function (currExtendingType) {
                    if (!utils_1.has(result, currExtendingType)) {
                        result[currExtendingType] = idx;
                    }
                });
            });
            return result;
        }, []);
        /**
         * @returns {number} - The chosen alternative index
         */
        return function () {
            var nextToken = this.LA(1);
            return choiceToAlt_1[nextToken.tokenTypeIdx];
        };
    }
    else {
        // optimized lookahead without needing to check the predicates at all.
        // this causes code duplication which is intentional to improve performance.
        /**
         * @returns {number} - The chosen alternative index
         */
        return function () {
            for (var t = 0; t < numOfAlts; t++) {
                var currAlt = alts[t];
                var currNumOfPaths = currAlt.length;
                nextPath: for (var j = 0; j < currNumOfPaths; j++) {
                    var currPath = currAlt[j];
                    var currPathLength = currPath.length;
                    for (var i = 0; i < currPathLength; i++) {
                        var nextToken = this.LA(i + 1);
                        if (tokenMatcher(nextToken, currPath[i]) === false) {
                            // mismatch in current path
                            // try the next pth
                            continue nextPath;
                        }
                    }
                    // found a full path that matches.
                    // this will also work for an empty ALT as the loop will be skipped
                    return t;
                }
                // none of the paths for the current alternative matched
                // try the next alternative
            }
            // none of the alternatives could be matched
            return undefined;
        };
    }
}
exports.buildAlternativesLookAheadFunc = buildAlternativesLookAheadFunc;
function buildSingleAlternativeLookaheadFunction(alt, tokenMatcher, dynamicTokensEnabled) {
    var areAllOneTokenLookahead = utils_1.every(alt, function (currPath) {
        return currPath.length === 1;
    });
    var numOfPaths = alt.length;
    // optimized (common) case of all the lookaheads paths requiring only
    // a single token lookahead.
    if (areAllOneTokenLookahead && !dynamicTokensEnabled) {
        var singleTokensTypes = utils_1.flatten(alt);
        if (singleTokensTypes.length === 1 &&
            utils_1.isEmpty(singleTokensTypes[0].categoryMatches)) {
            var expectedTokenType = singleTokensTypes[0];
            var expectedTokenUniqueKey_1 = expectedTokenType.tokenTypeIdx;
            return function () {
                return this.LA(1).tokenTypeIdx === expectedTokenUniqueKey_1;
            };
        }
        else {
            var choiceToAlt_2 = utils_1.reduce(singleTokensTypes, function (result, currTokType, idx) {
                result[currTokType.tokenTypeIdx] = true;
                utils_1.forEach(currTokType.categoryMatches, function (currExtendingType) {
                    result[currExtendingType] = true;
                });
                return result;
            }, []);
            return function () {
                var nextToken = this.LA(1);
                return choiceToAlt_2[nextToken.tokenTypeIdx] === true;
            };
        }
    }
    else {
        return function () {
            nextPath: for (var j = 0; j < numOfPaths; j++) {
                var currPath = alt[j];
                var currPathLength = currPath.length;
                for (var i = 0; i < currPathLength; i++) {
                    var nextToken = this.LA(i + 1);
                    if (tokenMatcher(nextToken, currPath[i]) === false) {
                        // mismatch in current path
                        // try the next pth
                        continue nextPath;
                    }
                }
                // found a full path that matches.
                return true;
            }
            // none of the paths matched
            return false;
        };
    }
}
exports.buildSingleAlternativeLookaheadFunction = buildSingleAlternativeLookaheadFunction;
var RestDefinitionFinderWalker = /** @class */ (function (_super) {
    __extends(RestDefinitionFinderWalker, _super);
    function RestDefinitionFinderWalker(topProd, targetOccurrence, targetProdType) {
        var _this = _super.call(this) || this;
        _this.topProd = topProd;
        _this.targetOccurrence = targetOccurrence;
        _this.targetProdType = targetProdType;
        return _this;
    }
    RestDefinitionFinderWalker.prototype.startWalking = function () {
        this.walk(this.topProd);
        return this.restDef;
    };
    RestDefinitionFinderWalker.prototype.checkIsTarget = function (node, expectedProdType, currRest, prevRest) {
        if (node.idx === this.targetOccurrence &&
            this.targetProdType === expectedProdType) {
            this.restDef = currRest.concat(prevRest);
            return true;
        }
        // performance optimization, do not iterate over the entire Grammar ast after we have found the target
        return false;
    };
    RestDefinitionFinderWalker.prototype.walkOption = function (optionProd, currRest, prevRest) {
        if (!this.checkIsTarget(optionProd, PROD_TYPE.OPTION, currRest, prevRest)) {
            _super.prototype.walkOption.call(this, optionProd, currRest, prevRest);
        }
    };
    RestDefinitionFinderWalker.prototype.walkAtLeastOne = function (atLeastOneProd, currRest, prevRest) {
        if (!this.checkIsTarget(atLeastOneProd, PROD_TYPE.REPETITION_MANDATORY, currRest, prevRest)) {
            _super.prototype.walkOption.call(this, atLeastOneProd, currRest, prevRest);
        }
    };
    RestDefinitionFinderWalker.prototype.walkAtLeastOneSep = function (atLeastOneSepProd, currRest, prevRest) {
        if (!this.checkIsTarget(atLeastOneSepProd, PROD_TYPE.REPETITION_MANDATORY_WITH_SEPARATOR, currRest, prevRest)) {
            _super.prototype.walkOption.call(this, atLeastOneSepProd, currRest, prevRest);
        }
    };
    RestDefinitionFinderWalker.prototype.walkMany = function (manyProd, currRest, prevRest) {
        if (!this.checkIsTarget(manyProd, PROD_TYPE.REPETITION, currRest, prevRest)) {
            _super.prototype.walkOption.call(this, manyProd, currRest, prevRest);
        }
    };
    RestDefinitionFinderWalker.prototype.walkManySep = function (manySepProd, currRest, prevRest) {
        if (!this.checkIsTarget(manySepProd, PROD_TYPE.REPETITION_WITH_SEPARATOR, currRest, prevRest)) {
            _super.prototype.walkOption.call(this, manySepProd, currRest, prevRest);
        }
    };
    return RestDefinitionFinderWalker;
}(rest_1.RestWalker));
/**
 * Returns the definition of a target production in a top level level rule.
 */
var InsideDefinitionFinderVisitor = /** @class */ (function (_super) {
    __extends(InsideDefinitionFinderVisitor, _super);
    function InsideDefinitionFinderVisitor(targetOccurrence, targetProdType) {
        var _this = _super.call(this) || this;
        _this.targetOccurrence = targetOccurrence;
        _this.targetProdType = targetProdType;
        _this.result = [];
        return _this;
    }
    InsideDefinitionFinderVisitor.prototype.checkIsTarget = function (node, expectedProdName) {
        if (node.idx === this.targetOccurrence &&
            this.targetProdType === expectedProdName) {
            this.result = node.definition;
        }
    };
    InsideDefinitionFinderVisitor.prototype.visitOption = function (node) {
        this.checkIsTarget(node, PROD_TYPE.OPTION);
    };
    InsideDefinitionFinderVisitor.prototype.visitRepetition = function (node) {
        this.checkIsTarget(node, PROD_TYPE.REPETITION);
    };
    InsideDefinitionFinderVisitor.prototype.visitRepetitionMandatory = function (node) {
        this.checkIsTarget(node, PROD_TYPE.REPETITION_MANDATORY);
    };
    InsideDefinitionFinderVisitor.prototype.visitRepetitionMandatoryWithSeparator = function (node) {
        this.checkIsTarget(node, PROD_TYPE.REPETITION_MANDATORY_WITH_SEPARATOR);
    };
    InsideDefinitionFinderVisitor.prototype.visitRepetitionWithSeparator = function (node) {
        this.checkIsTarget(node, PROD_TYPE.REPETITION_WITH_SEPARATOR);
    };
    InsideDefinitionFinderVisitor.prototype.visitAlternation = function (node) {
        this.checkIsTarget(node, PROD_TYPE.ALTERNATION);
    };
    return InsideDefinitionFinderVisitor;
}(gast_visitor_public_1.GAstVisitor));
function lookAheadSequenceFromAlternatives(altsDefs, k) {
    function getOtherPaths(pathsAndSuffixes, filterIdx) {
        return utils_1.reduce(pathsAndSuffixes, function (result, currPathsAndSuffixes, currIdx) {
            if (currIdx !== filterIdx) {
                var currPartialPaths = utils_1.map(currPathsAndSuffixes, function (singlePathAndSuffix) { return singlePathAndSuffix.partialPath; });
                return result.concat(currPartialPaths);
            }
            return result;
        }, []);
    }
    function isUniquePrefix(arr, item) {
        return (utils_1.find(arr, function (currOtherPath) {
            return utils_1.every(item, function (currPathTok, idx) { return currPathTok === currOtherPath[idx]; });
        }) === undefined);
    }
    function initializeArrayOfArrays(size) {
        var result = [];
        for (var i = 0; i < size; i++) {
            result.push([]);
        }
        return result;
    }
    var partialAlts = utils_1.map(altsDefs, function (currAlt) { return interpreter_1.possiblePathsFrom([currAlt], 1); });
    var finalResult = initializeArrayOfArrays(partialAlts.length);
    var newData = partialAlts;
    // maxLookahead loop
    for (var pathLength = 1; pathLength <= k; pathLength++) {
        var currDataset = newData;
        newData = initializeArrayOfArrays(currDataset.length);
        // alternatives loop
        for (var resultIdx = 0; resultIdx < currDataset.length; resultIdx++) {
            var currAltPathsAndSuffixes = currDataset[resultIdx];
            var otherPaths = getOtherPaths(currDataset, resultIdx);
            // paths in current alternative loop
            for (var currPathIdx = 0; currPathIdx < currAltPathsAndSuffixes.length; currPathIdx++) {
                var currPathPrefix = currAltPathsAndSuffixes[currPathIdx].partialPath;
                var suffixDef = currAltPathsAndSuffixes[currPathIdx].suffixDef;
                var isUnique = isUniquePrefix(otherPaths, currPathPrefix);
                // even if a path is not unique, but there are no longer alternatives to try
                // or if we have reached the maximum lookahead (k) permitted.
                if (isUnique ||
                    utils_1.isEmpty(suffixDef) ||
                    currPathPrefix.length === k) {
                    var currAltResult = finalResult[resultIdx];
                    if (!containsPath(currAltResult, currPathPrefix)) {
                        currAltResult.push(currPathPrefix);
                    }
                }
                else {
                    var newPartialPathsAndSuffixes = interpreter_1.possiblePathsFrom(suffixDef, pathLength + 1, currPathPrefix);
                    newData[resultIdx] = newData[resultIdx].concat(newPartialPathsAndSuffixes);
                }
            }
        }
    }
    return finalResult;
}
exports.lookAheadSequenceFromAlternatives = lookAheadSequenceFromAlternatives;
function getLookaheadPathsForOr(occurrence, ruleGrammar, k) {
    var visitor = new InsideDefinitionFinderVisitor(occurrence, PROD_TYPE.ALTERNATION);
    ruleGrammar.accept(visitor);
    return lookAheadSequenceFromAlternatives(visitor.result, k);
}
exports.getLookaheadPathsForOr = getLookaheadPathsForOr;
function getLookaheadPathsForOptionalProd(occurrence, ruleGrammar, prodType, k) {
    var insideDefVisitor = new InsideDefinitionFinderVisitor(occurrence, prodType);
    ruleGrammar.accept(insideDefVisitor);
    var insideDef = insideDefVisitor.result;
    var afterDefWalker = new RestDefinitionFinderWalker(ruleGrammar, occurrence, prodType);
    var afterDef = afterDefWalker.startWalking();
    var insideFlat = new gast_public_1.Flat({ definition: insideDef });
    var afterFlat = new gast_public_1.Flat({ definition: afterDef });
    return lookAheadSequenceFromAlternatives([insideFlat, afterFlat], k);
}
exports.getLookaheadPathsForOptionalProd = getLookaheadPathsForOptionalProd;
function containsPath(alternative, path) {
    var found = utils_1.find(alternative, function (otherPath) {
        return (path.length === otherPath.length &&
            utils_1.every(path, function (targetItem, idx) {
                return targetItem === otherPath[idx];
            }));
    });
    return found !== undefined;
}
exports.containsPath = containsPath;
function isStrictPrefixOfPath(prefix, other) {
    return (prefix.length < other.length &&
        utils_1.every(prefix, function (tokType, idx) {
            return tokType === other[idx];
        }));
}
exports.isStrictPrefixOfPath = isStrictPrefixOfPath;
function areTokenCategoriesNotUsed(lookAheadPaths) {
    return utils_1.every(lookAheadPaths, function (singleAltPaths) {
        return utils_1.every(singleAltPaths, function (singlePath) {
            return utils_1.every(singlePath, function (token) { return utils_1.isEmpty(token.categoryMatches); });
        });
    });
}
exports.areTokenCategoriesNotUsed = areTokenCategoriesNotUsed;
//# sourceMappingURL=lookahead.js.map