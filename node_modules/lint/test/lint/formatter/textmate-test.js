/*jslint nodejs: true, indent:4 */

var vows = require('vows');
var assert = require('assert');
var formatter = require('../../../lib/lint/formatter/textmate');

var Report = require('../../../lib/lint/formatter').Report;

function createFormatter(options) {
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

var FormatterTest = vows.describe('VIM Formatter class').addBatch({
    "format() / mode=normal" : {
        topic : function (item) {
            return createFormatter();
        },
        'should format empty report' : function (topic) {
            assert.equal(topic.format(createReportEmpty()), '0 error\n');
        },
        'should format valid report' : function (topic) {
            assert.equal(topic.format(createReportValid()), '0 error\n');
        },
        'should format invalid report' : function (topic) {
            assert.equal(topic.format(createReportInvalid()), 'foo: line 12, character 5, r\ne\n\n1 error\n');
        }
    }
});

exports.FormatterTest = FormatterTest;