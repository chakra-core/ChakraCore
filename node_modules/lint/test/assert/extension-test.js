/*jslint nodejs: true, indent:4 */
var vows = require('vows');
require('../../lib/assert/extension');

var fs = require('fs');
var path = require('path');
var assert = require('assert');

var FIXTURE_PATH = fs.realpathSync(path.join(path.dirname(path.dirname(__dirname)), 'resource', 'fixture'));

var AssertExtensionTest = vows.describe('assert module').addBatch({
    "validateLint()" : {
        topic : function (item) {
            return assert.validateLint;
        },
        'should return a function' : function (topic) {
            assert.isFunction(topic);
        },
        'should throw an error if wrong javascript is passed' : function (topic) {
            assert.throws(function () {
                topic("fdslfjsdlkj;");
            });
        },
        'should not throw an error if wrong javascript is passed' : function (topic) {
            assert.doesNotThrow(function () {
                topic("var foo = 'bar';");
            });
        }
    },
    "validateLintFile()" : {
        topic : function (item) {
            return assert.validateLintFile;
        },
        'should return a function' : function (topic) {
            assert.isFunction(topic);
        },
        'should throw an error if file does not exist' : function (topic) {
            assert.throws(function () {
                topic('nonexistent.js');
            });
        },
        'should throw an error if file does not contains valid javascript' : function (topic) {
            var fixtureFile = path.join(FIXTURE_PATH, 'invalid-test.js');

            assert.doesNotThrow(function () {
                fs.realpathSync(fixtureFile);
            });
            assert.throws(function () {
                topic(fixtureFile);
            });
        },
        'should not throw an error if wrong javascript is passed' : function (topic) {
            var fixtureFile = path.join(FIXTURE_PATH, 'valid-test.js');
            assert.doesNotThrow(function () {
                topic(fixtureFile);
            });
        }
    }
});

exports.AssertExtensionTest = AssertExtensionTest;