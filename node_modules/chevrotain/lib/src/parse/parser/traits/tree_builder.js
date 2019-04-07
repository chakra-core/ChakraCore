"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var cst_1 = require("../../cst/cst");
var utils_1 = require("../../../utils/utils");
var cst_visitor_1 = require("../../cst/cst_visitor");
var keys_1 = require("../../grammar/keys");
var parser_1 = require("../parser");
/**
 * This trait is responsible for the CST building logic.
 */
var TreeBuilder = /** @class */ (function () {
    function TreeBuilder() {
    }
    TreeBuilder.prototype.initTreeBuilder = function (config) {
        this.LAST_EXPLICIT_RULE_STACK = [];
        this.CST_STACK = [];
        this.outputCst = utils_1.has(config, "outputCst")
            ? config.outputCst
            : parser_1.DEFAULT_PARSER_CONFIG.outputCst;
        if (!this.outputCst) {
            this.cstInvocationStateUpdate = utils_1.NOOP;
            this.cstFinallyStateUpdate = utils_1.NOOP;
            this.cstPostTerminal = utils_1.NOOP;
            this.cstPostNonTerminal = utils_1.NOOP;
            this.getLastExplicitRuleShortName = this.getLastExplicitRuleShortNameNoCst;
            this.getPreviousExplicitRuleShortName = this.getPreviousExplicitRuleShortNameNoCst;
            this.getLastExplicitRuleOccurrenceIndex = this.getLastExplicitRuleOccurrenceIndexNoCst;
            this.manyInternal = this.manyInternalNoCst;
            this.orInternal = this.orInternalNoCst;
            this.optionInternal = this.optionInternalNoCst;
            this.atLeastOneInternal = this.atLeastOneInternalNoCst;
            this.manySepFirstInternal = this.manySepFirstInternalNoCst;
            this.atLeastOneSepFirstInternal = this.atLeastOneSepFirstInternalNoCst;
        }
    };
    // CST
    TreeBuilder.prototype.cstNestedInvocationStateUpdate = function (nestedName, shortName) {
        this.CST_STACK.push({
            name: nestedName,
            fullName: this.shortRuleNameToFull.get(this.getLastExplicitRuleShortName()) + nestedName,
            children: {}
        });
    };
    TreeBuilder.prototype.cstInvocationStateUpdate = function (fullRuleName, shortName) {
        this.LAST_EXPLICIT_RULE_STACK.push(this.RULE_STACK.length - 1);
        this.CST_STACK.push({
            name: fullRuleName,
            children: {}
        });
    };
    TreeBuilder.prototype.cstFinallyStateUpdate = function () {
        this.LAST_EXPLICIT_RULE_STACK.pop();
        this.CST_STACK.pop();
    };
    TreeBuilder.prototype.cstNestedFinallyStateUpdate = function () {
        this.CST_STACK.pop();
    };
    TreeBuilder.prototype.cstPostTerminal = function (key, consumedToken) {
        // TODO: would save the "current rootCST be faster than locating it for each terminal?
        var rootCst = this.CST_STACK[this.CST_STACK.length - 1];
        cst_1.addTerminalToCst(rootCst, consumedToken, key);
    };
    TreeBuilder.prototype.cstPostNonTerminal = function (ruleCstResult, ruleName) {
        cst_1.addNoneTerminalToCst(this.CST_STACK[this.CST_STACK.length - 1], ruleName, ruleCstResult);
    };
    TreeBuilder.prototype.getBaseCstVisitorConstructor = function () {
        if (utils_1.isUndefined(this.baseCstVisitorConstructor)) {
            var newBaseCstVisitorConstructor = cst_visitor_1.createBaseSemanticVisitorConstructor(this.className, this.allRuleNames);
            this.baseCstVisitorConstructor = newBaseCstVisitorConstructor;
            return newBaseCstVisitorConstructor;
        }
        return this.baseCstVisitorConstructor;
    };
    TreeBuilder.prototype.getBaseCstVisitorConstructorWithDefaults = function () {
        if (utils_1.isUndefined(this.baseCstVisitorWithDefaultsConstructor)) {
            var newConstructor = cst_visitor_1.createBaseVisitorConstructorWithDefaults(this.className, this.allRuleNames, this.getBaseCstVisitorConstructor());
            this.baseCstVisitorWithDefaultsConstructor = newConstructor;
            return newConstructor;
        }
        return this.baseCstVisitorWithDefaultsConstructor;
    };
    TreeBuilder.prototype.nestedRuleBeforeClause = function (methodOpts, laKey) {
        var nestedName;
        if (methodOpts.NAME !== undefined) {
            nestedName = methodOpts.NAME;
            this.nestedRuleInvocationStateUpdate(nestedName, laKey);
            return nestedName;
        }
        else {
            return undefined;
        }
    };
    TreeBuilder.prototype.nestedAltBeforeClause = function (methodOpts, occurrence, methodKeyIdx, altIdx) {
        var ruleIdx = this.getLastExplicitRuleShortName();
        var shortName = keys_1.getKeyForAltIndex(ruleIdx, methodKeyIdx, occurrence, altIdx);
        var nestedName;
        if (methodOpts.NAME !== undefined) {
            nestedName = methodOpts.NAME;
            this.nestedRuleInvocationStateUpdate(nestedName, shortName);
            return {
                shortName: shortName,
                nestedName: nestedName
            };
        }
        else {
            return undefined;
        }
    };
    TreeBuilder.prototype.nestedRuleFinallyClause = function (laKey, nestedName) {
        var cstStack = this.CST_STACK;
        var nestedRuleCst = cstStack[cstStack.length - 1];
        this.nestedRuleFinallyStateUpdate();
        // this return a different result than the previous invocation because "nestedRuleFinallyStateUpdate" pops the cst stack
        var parentCstNode = cstStack[cstStack.length - 1];
        cst_1.addNoneTerminalToCst(parentCstNode, nestedName, nestedRuleCst);
    };
    TreeBuilder.prototype.getLastExplicitRuleShortName = function () {
        var lastExplictIndex = this.LAST_EXPLICIT_RULE_STACK[this.LAST_EXPLICIT_RULE_STACK.length - 1];
        return this.RULE_STACK[lastExplictIndex];
    };
    TreeBuilder.prototype.getLastExplicitRuleShortNameNoCst = function () {
        var ruleStack = this.RULE_STACK;
        return ruleStack[ruleStack.length - 1];
    };
    TreeBuilder.prototype.getPreviousExplicitRuleShortName = function () {
        var lastExplicitIndex = this.LAST_EXPLICIT_RULE_STACK[this.LAST_EXPLICIT_RULE_STACK.length - 2];
        return this.RULE_STACK[lastExplicitIndex];
    };
    TreeBuilder.prototype.getPreviousExplicitRuleShortNameNoCst = function () {
        var ruleStack = this.RULE_STACK;
        return ruleStack[ruleStack.length - 2];
    };
    TreeBuilder.prototype.getLastExplicitRuleOccurrenceIndex = function () {
        var lastExplicitIndex = this.LAST_EXPLICIT_RULE_STACK[this.LAST_EXPLICIT_RULE_STACK.length - 1];
        return this.RULE_OCCURRENCE_STACK[lastExplicitIndex];
    };
    TreeBuilder.prototype.getLastExplicitRuleOccurrenceIndexNoCst = function () {
        var occurrenceStack = this.RULE_OCCURRENCE_STACK;
        return occurrenceStack[occurrenceStack.length - 1];
    };
    TreeBuilder.prototype.nestedRuleInvocationStateUpdate = function (nestedRuleName, shortNameKey) {
        this.RULE_OCCURRENCE_STACK.push(1);
        this.RULE_STACK.push(shortNameKey);
        this.cstNestedInvocationStateUpdate(nestedRuleName, shortNameKey);
    };
    TreeBuilder.prototype.nestedRuleFinallyStateUpdate = function () {
        this.RULE_STACK.pop();
        this.RULE_OCCURRENCE_STACK.pop();
        // NOOP when cst is disabled
        this.cstNestedFinallyStateUpdate();
    };
    return TreeBuilder;
}());
exports.TreeBuilder = TreeBuilder;
//# sourceMappingURL=tree_builder.js.map