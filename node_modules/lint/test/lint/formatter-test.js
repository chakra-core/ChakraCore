/*jslint nodejs: true, indent:4 */
var vows = require('vows');
var assert = require('assert');
var formatter = require('../../lib/lint/formatter');
var Report = require('../../lib/lint/formatter').Report;

function createFormatter(options) {
    return new formatter.Formatter(options);
}

function createBase(options) {
    return new formatter.Base(options);
}

function createReportEmpty() {
    var report;
    report = new Report();
    return report;
}

var FormatterTest = vows.describe('Formatter class').addBatch({
    "constructor()" : {
        topic : function (item) {
            return createFormatter();
        },
        'should not throw error for types cli, json, etc' : function (topic) {
            ['cli', 'json', 'xml', 'textmate'].forEach(function (type) {
                assert.doesNotThrow(function () {
                    createFormatter({type: type});
                });
            });
        }
    },
    "format()" : {
        topic : function (item) {
            return createFormatter({type: 'json'});
        },
        'should not throw error for types cli, json, etc' : function (topic) {
            ['cli', 'json', 'xml', 'textmate'].forEach(function (type) {
                assert.doesNotThrow(function () {
                    topic.format(createReportEmpty());
                });
            });
        }
    }
});

var BaseTest = vows.describe('Base class').addBatch({
    "format(), formatSimple(), formatNormal(), formatFull()" : {
        topic : function (item) {
            return createBase();
        },
        'should throw error' : function (topic) {
            ['format', 'formatSimple', 'formatNormal', 'formatFull'].forEach(function (method) {
                assert.throws(function () {
                    topic[method]();
                });
            });
        }
    }
});

exports.FormatterTest = FormatterTest;
exports.BaseTest = BaseTest;