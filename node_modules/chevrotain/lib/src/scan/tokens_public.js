"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var utils_1 = require("../utils/utils");
var lang_extensions_1 = require("../lang/lang_extensions");
var lexer_public_1 = require("./lexer_public");
var tokens_1 = require("./tokens");
function tokenLabel(clazz) {
    if (hasTokenLabel(clazz)) {
        return clazz.LABEL;
    }
    else {
        return tokenName(clazz);
    }
}
exports.tokenLabel = tokenLabel;
function hasTokenLabel(obj) {
    return utils_1.isString(obj.LABEL) && obj.LABEL !== "";
}
exports.hasTokenLabel = hasTokenLabel;
function tokenName(obj) {
    // The tokenName property is needed under some old versions of node.js (0.10/0.12)
    // where the Function.prototype.name property is not defined as a 'configurable' property
    // enable producing readable error messages.
    /* istanbul ignore if -> will only run in old versions of node.js */
    if (utils_1.isObject(obj) &&
        obj.hasOwnProperty("tokenName") &&
        utils_1.isString(obj.tokenName)) {
        return obj.tokenName;
    }
    else {
        return lang_extensions_1.functionName(obj);
    }
}
exports.tokenName = tokenName;
var PARENT = "parent";
var CATEGORIES = "categories";
var LABEL = "label";
var GROUP = "group";
var PUSH_MODE = "push_mode";
var POP_MODE = "pop_mode";
var LONGER_ALT = "longer_alt";
var LINE_BREAKS = "line_breaks";
var START_CHARS_HINT = "start_chars_hint";
function createToken(config) {
    return createTokenInternal(config);
}
exports.createToken = createToken;
function createTokenInternal(config) {
    var tokenName = config.name;
    var pattern = config.pattern;
    var tokenType = {};
    // can be overwritten according to:
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Function/
    // name?redirectlocale=en-US&redirectslug=JavaScript%2FReference%2FGlobal_Objects%2FFunction%2Fname
    /* istanbul ignore if -> will only run in old versions of node.js */
    if (!lang_extensions_1.defineNameProp(tokenType, tokenName)) {
        // hack to save the tokenName in situations where the constructor's name property cannot be reconfigured
        tokenType.tokenName = tokenName;
    }
    if (!utils_1.isUndefined(pattern)) {
        tokenType.PATTERN = pattern;
    }
    if (utils_1.has(config, PARENT)) {
        throw "The parent property is no longer supported.\n" +
            "See: https://github.com/SAP/chevrotain/issues/564#issuecomment-349062346 for details.";
    }
    if (utils_1.has(config, CATEGORIES)) {
        tokenType.CATEGORIES = config[CATEGORIES];
    }
    tokens_1.augmentTokenTypes([tokenType]);
    if (utils_1.has(config, LABEL)) {
        tokenType.LABEL = config[LABEL];
    }
    if (utils_1.has(config, GROUP)) {
        tokenType.GROUP = config[GROUP];
    }
    if (utils_1.has(config, POP_MODE)) {
        tokenType.POP_MODE = config[POP_MODE];
    }
    if (utils_1.has(config, PUSH_MODE)) {
        tokenType.PUSH_MODE = config[PUSH_MODE];
    }
    if (utils_1.has(config, LONGER_ALT)) {
        tokenType.LONGER_ALT = config[LONGER_ALT];
    }
    if (utils_1.has(config, LINE_BREAKS)) {
        tokenType.LINE_BREAKS = config[LINE_BREAKS];
    }
    if (utils_1.has(config, START_CHARS_HINT)) {
        tokenType.START_CHARS_HINT = config[START_CHARS_HINT];
    }
    return tokenType;
}
exports.EOF = createToken({ name: "EOF", pattern: lexer_public_1.Lexer.NA });
tokens_1.augmentTokenTypes([exports.EOF]);
function createTokenInstance(tokType, image, startOffset, endOffset, startLine, endLine, startColumn, endColumn) {
    return {
        image: image,
        startOffset: startOffset,
        endOffset: endOffset,
        startLine: startLine,
        endLine: endLine,
        startColumn: startColumn,
        endColumn: endColumn,
        tokenTypeIdx: tokType.tokenTypeIdx,
        tokenType: tokType
    };
}
exports.createTokenInstance = createTokenInstance;
function tokenMatcher(token, tokType) {
    return tokens_1.tokenStructuredMatcher(token, tokType);
}
exports.tokenMatcher = tokenMatcher;
//# sourceMappingURL=tokens_public.js.map