/*jslint nodejs: true, indent:4 */
var vows = require('vows');
var assert = require('assert');
var formatter = require('../../../lib/lint/formatter/cli');

var Report = require('../../../lib/lint/formatter').Report;

function createFormatter(options) {
    options = options || {};
    options.colors = options.colors === undefined ? false : options.colors;
    return new formatter.Formatter(options);
}

function createReportEmpty() {
    var report;
    report = new Report();
    return report;
}

function createReportValid() {
    var report;
    report = new Report();
    report.addFile('foo', []);
    return report;
}

function createReportInvalid() {
    var report;
    report = new Report();
    report.addFile('foo', [{
        file: 'foo',
        line: 12,
        character: 5,
        evidence: 'e',
        reason: 'r'
    }]);

    return report;
}

var FormatterTest = vows.describe('Cli Formatter class').addBatch({
    "format() / mode=simple" : {
        topic : function (item) {
            return createFormatter({mode: 'simple'});
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '✓ Valid » 0 file ∙ 0 error\n');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '✓ Valid » 1 file ∙ 0 error\n');
        },
        'should format invalid report ' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), '✗ Invalid » 1 file ∙ 1 error\n');
        }
    },
    "format() / mode=normal" : {
        topic : function (item) {
            return createFormatter({mode: 'normal'});
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '✓ Valid » 0 file ∙ 0 error\n');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '✓ Valid » 1 file ∙ 0 error\n');
        },
        'should format invalid report ' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), '♢ foo\n  1) e  //line 12\n    » r\n\n✗ Invalid » 1 file ∙ 1 error\n');
        }
    },
    "format() / mode=full" : {
        topic : function (item) {
            return createFormatter({mode: 'full'});
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '✓ Valid » 0 file ∙ 0 error\n');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '✓ Valid » 1 file ∙ 0 error\n');
        },
        'should format invalid report ' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), '♢ foo (1 errors)\n  1) e  //line 12, character 5\n    » r\n\n✗ Invalid » 1 file ∙ 1 error\n');
        }
    }
});

exports.FormatterTest = FormatterTest;