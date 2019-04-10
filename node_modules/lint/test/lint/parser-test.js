/*jslint nodejs: true, indent:4 */
var vows = require('vows');
var assert = require('assert');
var fs = require('fs');
var path = require('path');

var parser = require('../../lib/lint/parser');

var FIXTURE_PATH = fs.realpathSync(path.join(path.dirname(path.dirname(__dirname)), 'resource', 'fixture'));

function createParser(options) {
    return new parser.Parser(options);
}

var ParserTest = vows.describe('Parser class').addBatch({
    "update()" : {
        topic : function (item) {
            return createParser();
        },
        'should return this' : function (topic) {
            assert.equal(topic.update(), topic);
        },
        'should concatenate source' : function (topic) {
            topic.update();
            assert.equal(topic._source, '');
            topic.update('foo');
            assert.equal(topic._source, 'foo');
            topic.update();
            assert.equal(topic._source, 'foo');
            topic.update('');
            assert.equal(topic._source, 'foo');
            topic.update('bar');
            assert.equal(topic._source, 'foobar');
        }
    },
    "reset()" : {
        topic : function (item) {
            return createParser();
        },
        'should return this' : function (topic) {
            assert.equal(topic.reset(), topic);
        },
        'should empty source' : function (topic) {
            topic.reset();
            topic.update('foo-bar').update('-baz');
            assert.equal(topic._source, 'foo-bar-baz');
            topic.reset();
            assert.equal(topic._source, '');
        }
    },
    "validate()" : {
        topic : function (item) {
            return createParser();
        },
        'should return this' : function (topic) {
            assert.equal(topic.validate(), topic);
        }
    },
    "isValid()" : {
        topic : function (item) {
            return createParser();
        },
        'should return true if empty' : function (topic) {
            topic.reset();
            assert.equal(topic.isValid(), true);
        },
        'should return true if valid javascript (simple)' : function (topic) {
            topic.reset();
            topic.update('var foo = "bar";');
            assert.equal(topic.isValid(), true);

            topic.reset();
            topic.update('var foo = "baz";');
            assert.equal(topic.isValid(), true);
        },
        'should return false if invalid javascript (simple)' : function (topic) {
            topic.reset();
            topic.update('var foo = "bar"');// (missing semicolon)
            assert.equal(topic.isValid(), false);

            topic.update(';');
            assert.equal(topic.isValid(), true);
        }
    },
    "getReport()" : {
        topic : function (item) {
            return createParser();
        },
        'should return new object' : function (topic) {
            assert.ok(typeof (topic.getReport()) === 'object');
            topic.update('foo');
            assert.ok(typeof (topic.getReport()) === 'object');
            topic.reset();
            assert.ok(typeof (topic.getReport()) === 'object');
        }
    }

});

var ParserModuleTest = vows.describe('parser module').addBatch({
    "isValid()" : {
        topic : function (item) {
            return parser.isValid;
        },
        'should return true if empty' : function (topic) {
            assert.equal(topic(''), true);
        },
        'should return true if valid javascript (simple)' : function (topic) {
            assert.equal(topic('var foo = "bar";'), true);
            assert.equal(topic('var foo = "baz";'), true);
        },
        'should return false if invalid javascript (simple)' : function (topic) {
            assert.equal(topic('var foo = "bar"'), false);// (missing  semicolon)
        }
    },
    "isValidFile()" : {
        topic : function (item) {
            var self = this,
                report = {};

            parser.isValidFile('nonexistent.js', null, function (error, result) {
                report.nonExistent = {
                    error : error,
                    result : result
                };

                var validFile = path.join(FIXTURE_PATH, 'valid-test.js');
                parser.isValidFile(validFile, null, function (error, result) {
                    report.validFile = {
                        error : error,
                        result : result
                    };

                    var invalidFile = path.join(FIXTURE_PATH, 'invalid-test.js');
                    parser.isValidFile(invalidFile, null, function (error, result) {
                        report.invalidFile = {
                            error : error,
                            result : result
                        };

                        self.callback(null, report);
                    });
                });
            });

        },
        'should set error in callback if file does not exist' : function (topic) {
            assert.notEqual(topic.nonExistent.error, undefined);
        },
        'should return true if valid javascript file' : function (topic) {
            assert.isUndefined(topic.validFile.error);
            assert.equal(topic.validFile.result, true);
        },
        'should return false if invalid javascript file' : function (topic) {
            assert.isUndefined(topic.invalidFile.error);
            assert.equal(topic.invalidFile.result, false);
        }
    },
    "isValidFileSync()" : {
        topic : function (item) {
            return parser.isValidFileSync;
        },
        'should throw an error if file does not exist' : function (topic) {
            assert.throws(function () {
                topic('nonexistent.js');
            });
        },
        'should return true if valid javascript file' : function (topic) {
            var fixtureFile = path.join(FIXTURE_PATH, 'valid-test.js');
            assert.equal(topic(fixtureFile), true);
            assert.equal(topic(fixtureFile), true);
        },
        'should return false if invalid javascript file' : function (topic) {
            var fixtureFile = path.join(FIXTURE_PATH, 'invalid-test.js');
            assert.equal(topic(fixtureFile), false);
        }
    }
});

exports.ParserTest = ParserTest;
exports.ParserModuleTest = ParserModuleTest;