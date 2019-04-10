"use strict";
/**
 * @license
 * Copyright 2017 Palantir Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
Object.defineProperty(exports, "__esModule", { value: true });
var tslib_1 = require("tslib");
var tsutils_1 = require("tsutils");
var ts = require("typescript");
var Lint = require("../index");
var Rule = /** @class */ (function (_super) {
    tslib_1.__extends(Rule, _super);
    function Rule() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    Rule.prototype.apply = function (sourceFile) {
        return this.applyWithFunction(sourceFile, walk);
    };
    Rule.metadata = {
        description: "Prevents blank constructors, as they are redundant.",
        optionExamples: [true],
        options: null,
        optionsDescription: "Not configurable.",
        rationale: Lint.Utils.dedent(templateObject_1 || (templateObject_1 = tslib_1.__makeTemplateObject(["\n            JavaScript implicitly adds a blank constructor when there isn't one.\n            It's not necessary to manually add one in.\n        "], ["\n            JavaScript implicitly adds a blank constructor when there isn't one.\n            It's not necessary to manually add one in.\n        "]))),
        ruleName: "unnecessary-constructor",
        type: "functionality",
        typescriptOnly: false,
    };
    Rule.FAILURE_STRING = "Remove unnecessary empty constructor.";
    return Rule;
}(Lint.Rules.AbstractRule));
exports.Rule = Rule;
var isEmptyConstructor = function (node) {
    return node.body !== undefined && node.body.statements.length === 0;
};
var containsConstructorParameter = function (node) {
    // If this has any parameter properties
    return node.parameters.some(tsutils_1.isParameterProperty);
};
var isAccessRestrictingConstructor = function (node) {
    // No modifiers implies public
    return node.modifiers != undefined &&
        // If this has any modifier that isn't public, it's doing something
        node.modifiers.some(function (modifier) { return modifier.kind !== ts.SyntaxKind.PublicKeyword; });
};
function walk(context) {
    var callback = function (node) {
        if (tsutils_1.isConstructorDeclaration(node) &&
            isEmptyConstructor(node) &&
            !containsConstructorParameter(node) &&
            !isAccessRestrictingConstructor(node)) {
            context.addFailureAtNode(node, Rule.FAILURE_STRING);
        }
        else {
            ts.forEachChild(node, callback);
        }
    };
    return ts.forEachChild(context.sourceFile, callback);
}
var templateObject_1;
