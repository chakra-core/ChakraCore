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
var regexp_to_ast_1 = require("regexp-to-ast");
var utils_1 = require("../utils/utils");
var regExpParser = new regexp_to_ast_1.RegExpParser();
var complementErrorMessage = "Complement Sets are not supported for first char optimization";
exports.failedOptimizationPrefixMsg = 'Unable to use "first char" lexer optimizations:\n';
function getStartCodes(regExp, ensureOptimizations) {
    if (ensureOptimizations === void 0) { ensureOptimizations = false; }
    try {
        var ast = regExpParser.pattern(regExp.toString());
        var firstChars = firstChar(ast.value);
        if (ast.flags.ignoreCase) {
            firstChars = applyIgnoreCase(firstChars);
        }
        return firstChars;
    }
    catch (e) {
        /* istanbul ignore next */
        // Testing this relies on the regexp-to-ast library having a bug... */
        // TODO: only the else branch needs to be ignored, try to fix with newer prettier / tsc
        if (e.message === complementErrorMessage) {
            if (ensureOptimizations) {
                utils_1.PRINT_WARNING("" + exports.failedOptimizationPrefixMsg +
                    ("\tUnable to optimize: < " + regExp.toString() + " >\n") +
                    "\tComplement Sets cannot be automatically optimized.\n" +
                    "\tThis will disable the lexer's first char optimizations.\n" +
                    "\tSee: https://sap.github.io/chevrotain/docs/guide/resolving_lexer_errors.html#COMPLEMENT for details.");
            }
        }
        else {
            var msgSuffix = "";
            if (ensureOptimizations) {
                msgSuffix =
                    "\n\tThis will disable the lexer's first char optimizations.\n" +
                        "\tSee: https://sap.github.io/chevrotain/docs/guide/resolving_lexer_errors.html#REGEXP_PARSING for details.";
            }
            utils_1.PRINT_ERROR(exports.failedOptimizationPrefixMsg + "\n" +
                ("\tFailed parsing: < " + regExp.toString() + " >\n") +
                ("\tUsing the regexp-to-ast library version: " + regexp_to_ast_1.VERSION + "\n") +
                "\tPlease open an issue at: https://github.com/bd82/regexp-to-ast/issues" +
                msgSuffix);
        }
    }
    return [];
}
exports.getStartCodes = getStartCodes;
function firstChar(ast) {
    switch (ast.type) {
        case "Disjunction":
            return utils_1.flatten(utils_1.map(ast.value, firstChar));
        case "Alternative":
            var startChars_1 = [];
            var terms = ast.value;
            for (var i = 0; i < terms.length; i++) {
                var term = terms[i];
                if (utils_1.contains([
                    // A group back reference cannot affect potential starting char.
                    // because if a back reference is the first production than automatically
                    // the group being referenced has had to come BEFORE so its codes have already been added
                    "GroupBackReference",
                    // assertions do not affect potential starting codes
                    "Lookahead",
                    "NegativeLookahead",
                    "StartAnchor",
                    "EndAnchor",
                    "WordBoundary",
                    "NonWordBoundary"
                ], term.type)) {
                    continue;
                }
                var atom = term;
                switch (atom.type) {
                    case "Character":
                        startChars_1.push(atom.value);
                        break;
                    case "Set":
                        if (atom.complement === true) {
                            throw Error(complementErrorMessage);
                        }
                        // TODO: this may still be slow when there are many codes
                        utils_1.forEach(atom.value, function (code) {
                            if (typeof code === "number") {
                                startChars_1.push(code);
                            }
                            else {
                                //range
                                var range = code;
                                for (var rangeCode = range.from; rangeCode <= range.to; rangeCode++) {
                                    startChars_1.push(rangeCode);
                                }
                            }
                        });
                        break;
                    case "Group":
                        var groupCodes = firstChar(atom.value);
                        utils_1.forEach(groupCodes, function (code) { return startChars_1.push(code); });
                        break;
                    /* istanbul ignore next */
                    default:
                        throw Error("Non Exhaustive Match");
                }
                // reached a mandatory production, no more start codes can be found on this alternative
                if (
                //
                atom.quantifier === undefined ||
                    (atom.quantifier !== undefined &&
                        atom.quantifier.atLeast > 0)) {
                    break;
                }
            }
            return startChars_1;
        /* istanbul ignore next */
        default:
            throw Error("non exhaustive match!");
    }
}
exports.firstChar = firstChar;
function applyIgnoreCase(firstChars) {
    var firstCharsCase = [];
    utils_1.forEach(firstChars, function (charCode) {
        firstCharsCase.push(charCode);
        var char = String.fromCharCode(charCode);
        /* istanbul ignore else */
        if (char.toUpperCase() !== char) {
            firstCharsCase.push(char.toUpperCase().charCodeAt(0));
        }
        else if (char.toLowerCase() !== char) {
            firstCharsCase.push(char.toLowerCase().charCodeAt(0));
        }
    });
    return firstCharsCase;
}
exports.applyIgnoreCase = applyIgnoreCase;
function findCode(setNode, targetCharCodes) {
    return utils_1.find(setNode.value, function (codeOrRange) {
        if (typeof codeOrRange === "number") {
            return utils_1.contains(targetCharCodes, codeOrRange);
        }
        else {
            // range
            var range_1 = codeOrRange;
            return (utils_1.find(targetCharCodes, function (targetCode) {
                return range_1.from <= targetCode && targetCode <= range_1.to;
            }) !== undefined);
        }
    });
}
var CharCodeFinder = /** @class */ (function (_super) {
    __extends(CharCodeFinder, _super);
    function CharCodeFinder(targetCharCodes) {
        var _this = _super.call(this) || this;
        _this.targetCharCodes = targetCharCodes;
        _this.found = false;
        return _this;
    }
    CharCodeFinder.prototype.visitChildren = function (node) {
        // switch lookaheads as they do not actually consume any characters thus
        // finding a charCode at lookahead context does not mean that regexp can actually contain it in a match.
        switch (node.type) {
            case "Lookahead":
                this.visitLookahead(node);
                return;
            case "NegativeLookahead":
                this.visitNegativeLookahead(node);
                return;
        }
        _super.prototype.visitChildren.call(this, node);
    };
    CharCodeFinder.prototype.visitCharacter = function (node) {
        if (utils_1.contains(this.targetCharCodes, node.value)) {
            this.found = true;
        }
    };
    CharCodeFinder.prototype.visitSet = function (node) {
        if (node.complement) {
            if (findCode(node, this.targetCharCodes) === undefined) {
                this.found = true;
            }
        }
        else {
            if (findCode(node, this.targetCharCodes) !== undefined) {
                this.found = true;
            }
        }
    };
    return CharCodeFinder;
}(regexp_to_ast_1.BaseRegExpVisitor));
function canMatchCharCode(charCodes, pattern) {
    if (pattern instanceof RegExp) {
        var ast = regExpParser.pattern(pattern.toString());
        var charCodeFinder = new CharCodeFinder(charCodes);
        charCodeFinder.visit(ast);
        return charCodeFinder.found;
    }
    else {
        return (utils_1.find(pattern, function (char) {
            return utils_1.contains(charCodes, char.charCodeAt(0));
        }) !== undefined);
    }
}
exports.canMatchCharCode = canMatchCharCode;
//# sourceMappingURL=reg_exp.js.map