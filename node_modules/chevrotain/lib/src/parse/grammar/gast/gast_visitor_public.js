"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var gast_public_1 = require("./gast_public");
var GAstVisitor = /** @class */ (function () {
    function GAstVisitor() {
    }
    GAstVisitor.prototype.visit = function (node) {
        /* istanbul ignore next */
        if (node instanceof gast_public_1.NonTerminal) {
            return this.visitNonTerminal(node);
        }
        else if (node instanceof gast_public_1.Flat) {
            return this.visitFlat(node);
        }
        else if (node instanceof gast_public_1.Option) {
            return this.visitOption(node);
        }
        else if (node instanceof gast_public_1.RepetitionMandatory) {
            return this.visitRepetitionMandatory(node);
        }
        else if (node instanceof gast_public_1.RepetitionMandatoryWithSeparator) {
            return this.visitRepetitionMandatoryWithSeparator(node);
        }
        else if (node instanceof gast_public_1.RepetitionWithSeparator) {
            return this.visitRepetitionWithSeparator(node);
        }
        else if (node instanceof gast_public_1.Repetition) {
            return this.visitRepetition(node);
        }
        else if (node instanceof gast_public_1.Alternation) {
            return this.visitAlternation(node);
        }
        else if (node instanceof gast_public_1.Terminal) {
            return this.visitTerminal(node);
        }
        else if (node instanceof gast_public_1.Rule) {
            return this.visitRule(node);
        }
        else {
            throw Error("non exhaustive match");
        }
    };
    GAstVisitor.prototype.visitNonTerminal = function (node) { };
    GAstVisitor.prototype.visitFlat = function (node) { };
    GAstVisitor.prototype.visitOption = function (node) { };
    GAstVisitor.prototype.visitRepetition = function (node) { };
    GAstVisitor.prototype.visitRepetitionMandatory = function (node) { };
    GAstVisitor.prototype.visitRepetitionMandatoryWithSeparator = function (node) { };
    GAstVisitor.prototype.visitRepetitionWithSeparator = function (node) { };
    GAstVisitor.prototype.visitAlternation = function (node) { };
    GAstVisitor.prototype.visitTerminal = function (node) { };
    GAstVisitor.prototype.visitRule = function (node) { };
    return GAstVisitor;
}());
exports.GAstVisitor = GAstVisitor;
//# sourceMappingURL=gast_visitor_public.js.map