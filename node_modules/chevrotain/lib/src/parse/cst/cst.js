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
var lang_extensions_1 = require("../../lang/lang_extensions");
var keys_1 = require("../grammar/keys");
var gast_public_1 = require("../grammar/gast/gast_public");
var gast_visitor_public_1 = require("../grammar/gast/gast_visitor_public");
function addTerminalToCst(node, token, tokenTypeName) {
    if (node.children[tokenTypeName] === undefined) {
        node.children[tokenTypeName] = [token];
    }
    else {
        node.children[tokenTypeName].push(token);
    }
}
exports.addTerminalToCst = addTerminalToCst;
function addNoneTerminalToCst(node, ruleName, ruleResult) {
    if (node.children[ruleName] === undefined) {
        node.children[ruleName] = [ruleResult];
    }
    else {
        node.children[ruleName].push(ruleResult);
    }
}
exports.addNoneTerminalToCst = addNoneTerminalToCst;
var NamedDSLMethodsCollectorVisitor = /** @class */ (function (_super) {
    __extends(NamedDSLMethodsCollectorVisitor, _super);
    function NamedDSLMethodsCollectorVisitor(ruleIdx) {
        var _this = _super.call(this) || this;
        _this.result = [];
        _this.ruleIdx = ruleIdx;
        return _this;
    }
    NamedDSLMethodsCollectorVisitor.prototype.collectNamedDSLMethod = function (node, newNodeConstructor, methodIdx) {
        // TODO: better hack to copy what we need here...
        if (!utils_1.isUndefined(node.name)) {
            // copy without name so this will indeed be processed later.
            var nameLessNode 
            /* istanbul ignore else */
            = void 0;
            /* istanbul ignore else */
            if (node instanceof gast_public_1.Option ||
                node instanceof gast_public_1.Repetition ||
                node instanceof gast_public_1.RepetitionMandatory ||
                node instanceof gast_public_1.Alternation) {
                nameLessNode = new newNodeConstructor({
                    definition: node.definition,
                    idx: node.idx
                });
            }
            else if (node instanceof gast_public_1.RepetitionMandatoryWithSeparator ||
                node instanceof gast_public_1.RepetitionWithSeparator) {
                nameLessNode = new newNodeConstructor({
                    definition: node.definition,
                    idx: node.idx,
                    separator: node.separator
                });
            }
            else {
                throw Error("non exhaustive match");
            }
            var def = [nameLessNode];
            var key = keys_1.getKeyForAutomaticLookahead(this.ruleIdx, methodIdx, node.idx);
            this.result.push({ def: def, key: key, name: node.name, orgProd: node });
        }
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitOption = function (node) {
        this.collectNamedDSLMethod(node, gast_public_1.Option, keys_1.OPTION_IDX);
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitRepetition = function (node) {
        this.collectNamedDSLMethod(node, gast_public_1.Repetition, keys_1.MANY_IDX);
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitRepetitionMandatory = function (node) {
        this.collectNamedDSLMethod(node, gast_public_1.RepetitionMandatory, keys_1.AT_LEAST_ONE_IDX);
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitRepetitionMandatoryWithSeparator = function (node) {
        this.collectNamedDSLMethod(node, gast_public_1.RepetitionMandatoryWithSeparator, keys_1.AT_LEAST_ONE_SEP_IDX);
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitRepetitionWithSeparator = function (node) {
        this.collectNamedDSLMethod(node, gast_public_1.RepetitionWithSeparator, keys_1.MANY_SEP_IDX);
    };
    NamedDSLMethodsCollectorVisitor.prototype.visitAlternation = function (node) {
        var _this = this;
        this.collectNamedDSLMethod(node, gast_public_1.Alternation, keys_1.OR_IDX);
        var hasMoreThanOneAlternative = node.definition.length > 1;
        utils_1.forEach(node.definition, function (currFlatAlt, altIdx) {
            if (!utils_1.isUndefined(currFlatAlt.name)) {
                var def = currFlatAlt.definition;
                if (hasMoreThanOneAlternative) {
                    def = [new gast_public_1.Option({ definition: currFlatAlt.definition })];
                }
                else {
                    // mandatory
                    def = currFlatAlt.definition;
                }
                var key = keys_1.getKeyForAltIndex(_this.ruleIdx, keys_1.OR_IDX, node.idx, altIdx);
                _this.result.push({
                    def: def,
                    key: key,
                    name: currFlatAlt.name,
                    orgProd: currFlatAlt
                });
            }
        });
    };
    return NamedDSLMethodsCollectorVisitor;
}(gast_visitor_public_1.GAstVisitor));
exports.NamedDSLMethodsCollectorVisitor = NamedDSLMethodsCollectorVisitor;
function analyzeCst(topRules, fullToShortName) {
    var result = {
        dictDef: new lang_extensions_1.HashTable(),
        allRuleNames: []
    };
    utils_1.forEach(topRules, function (currTopRule) {
        var currTopRuleShortName = fullToShortName.get(currTopRule.name);
        result.allRuleNames.push(currTopRule.name);
        var namedCollectorVisitor = new NamedDSLMethodsCollectorVisitor(currTopRuleShortName);
        currTopRule.accept(namedCollectorVisitor);
        utils_1.forEach(namedCollectorVisitor.result, function (_a) {
            var def = _a.def, key = _a.key, name = _a.name;
            result.allRuleNames.push(currTopRule.name + name);
        });
    });
    return result;
}
exports.analyzeCst = analyzeCst;
//# sourceMappingURL=cst.js.map