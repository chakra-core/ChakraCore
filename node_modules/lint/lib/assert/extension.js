/*jslint nodejs: true, indent:4 */
var assert = require('assert');
var fs = require('fs');
var lint = require('../lint/parser');

if (! assert.validateLint) {
    assert.validateLint = function (actual, message) {
        var parser, errorCount;

        parser = new lint.Parser();
        errorCount = parser.update(actual).getReport().length;

        if (errorCount > 0) {
            assert.fail(errorCount, 0, message || "LINT validation returned {actual} error(s)", "===", assert.validateLint);
        }
    };
}

if (! assert.validateLintFile) {
    assert.validateLintFile = function (actual, message) {
        var parser, errorCount, data;

        data = fs.readFileSync(actual);
        parser = new lint.Parser();
        errorCount = parser.update(data).getReport().length;

        if (errorCount > 0) {
            assert.fail(errorCount, 0, message || "LINT validation returned {actual} error(s)", "===", assert.validateLintFile);
        }
    };
}